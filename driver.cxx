/*
 * cdownload lib: downloads, unpacks, and reads data from the Cluster CSA arhive
 * Copyright (C) 2016  Eugene Shalygin <eugene.shalygin@gmail.com>
 *
 * The development was partially supported by the Volkswagen Foundation
 * (VolkswagenStiftung).
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "driver.hxx"

#include "average.hxx"
#include "cdfreader.hxx"
#include "chunkdownloader.hxx"
#include "datareader.hxx"
#include "downloader.hxx"
#include "field.hxx"
#include "metadata.hxx"
#include "parameters.hxx"
#include "unpacker.hxx"

#include "filters/baddata.hxx"
#include "filters/plasmasheet.hxx"

#include "writers/ASCIIWriter.hxx"
#include "writers/BinaryWriter.hxx"

#include <boost/filesystem/operations.hpp>
#include <boost/log/trivial.hpp>

#include <fstream>
#include <algorithm>
#include <set>


namespace {

	void extractInfo(const std::map<cdownload::DatasetName, cdownload::DownloadedProductFile>& files,
	                 std::map<cdownload::DatasetName, cdownload::CDF::Info>& info,
	                 std::vector<cdownload::ProductName>& cdfKeys)
	{
		for (const std::pair<cdownload::DatasetName, cdownload::DownloadedProductFile>& p: files) {
			cdownload::CDF::Info i = cdownload::CDF::Info(cdownload::CDF::File(p.second));
			info[p.first] = i;
			auto varNames = i.variableNames();
			std::copy(varNames.begin(), varNames.end(), std::back_inserter(cdfKeys));
		}
	}

	std::vector<cdownload::Output> expandOutputs(const std::vector<cdownload::Output>& outputs,
	                                             const std::map<cdownload::DatasetName, cdownload::CDF::Info>& avaliableProducts)
	{
		using cdownload::ProductName;
		using cdownload::Output;

		std::vector<Output> res;
		for (const Output& output: outputs) {
			std::map<std::string, std::vector<ProductName> > expandedProductsMap;
			for (const std::string& ds: output.datasetNames()) {
				auto i = avaliableProducts.find(ds);
				if (i == avaliableProducts.end()) {
					throw std::runtime_error("Dataset '" + ds + "' is not present in the downloaded files");
				}
				std::vector<cdownload::ProductName> expandedProducts =
					cdownload::expandWildcardsCaseSensitive(
						output.productsForDataset(ds), i->second.variableNames());
				expandedProductsMap[ds] = expandedProducts;
			}
			res.emplace_back(output.name(), output.format(), expandedProductsMap);
		}
		return res;
	}
}

using namespace cdownload::csa_time_formatting;

cdownload::Driver::Driver(const cdownload::Parameters& params)
	: params_{params}
{
}

void cdownload::Driver::doTask()
{
	std::vector<std::shared_ptr<RawDataFilter> > rawFilters;
	std::vector<std::shared_ptr<AveragedDataFilter> > averageDataFilters;
	createFilters(rawFilters, averageDataFilters);

	std::vector<std::shared_ptr<Filter> > allFilters;
	std::copy(rawFilters.begin(), rawFilters.end(), std::back_inserter(allFilters));
	std::copy(averageDataFilters.begin(), averageDataFilters.end(), std::back_inserter(allFilters));

	using namespace csa_time_formatting;

	path tmpDirName = params_.workDir() / boost::filesystem::unique_path();
	boost::filesystem::create_directories(tmpDirName);
	BOOST_LOG_TRIVIAL(info) << "Working in " << tmpDirName;

	// dumping and saving parameters
	BOOST_LOG_TRIVIAL(debug) << "Parameters: " << std::endl << params_;
	{
		std::ofstream paramsOutput((params_.outputDir() / "parameters").c_str());
		paramsOutput << params_;
	}

	BOOST_LOG_TRIVIAL(info) << "Starting downloading";

	timeduration chunkDuration = params_.timeInterval();
	while (chunkDuration < timeduration(24, 0, 0, 0)) {
		chunkDuration += params_.timeInterval();
	}

	Metadata meta;
	DataDownloader downloader;
	// sure enough this is before Cluster II launch date
	datetime availableStartDateTime = makeDateTime(1999, 1, 1);
	datetime availableEndDateTime = boost::posix_time::second_clock::universal_time();

	std::vector<DatasetName> requiredDatasets = collectRequireddDatasets(params_.outputs(), allFilters);

	for (const auto& ds: requiredDatasets) {
		auto dataset = meta.dataset(ds);
		if (dataset.minTime() > availableStartDateTime) {
			availableStartDateTime = dataset.minTime();
		}
		if (dataset.maxTime() < availableEndDateTime) {
			availableEndDateTime = dataset.maxTime();
		}
	}

	BOOST_LOG_TRIVIAL(info) << "Found available time range: ["
	                        << availableStartDateTime << ',' << availableEndDateTime << ']';

//  datetime currentChunkStartTime = availableStartDateTime;

	// have to download first chunk separately in order to detect dataset products
	ChunkDownloader chunkDownloader {tmpDirName, downloader, requiredDatasets, availableStartDateTime, availableEndDateTime};

	Chunk currentChunk = chunkDownloader.nextChunk();

	std::map<cdownload::DatasetName, CDF::Info> availableProducts;
	std::vector<ProductName> orderedCDFKeys;
	extractInfo(currentChunk.files, availableProducts, orderedCDFKeys);

	BOOST_LOG_TRIVIAL(debug) << "Collected CDF variables: " << put_list(orderedCDFKeys);

	std::vector<Output> expandedOutputs = expandOutputs(params_.outputs(), availableProducts);

	productsToRead_.clear();
	datasetsToLoad_.clear();

	ProductsToRead fieldsToRead = collectAllProductsToRead(availableProducts, expandedOutputs, allFilters);

	std::set<DatasetName> datasets;
	for (const auto& pr: fieldsToRead.productsToWrite) {
		datasets.insert(pr.dataset());
		productsToRead_.push_back(pr);
	}
	numberOfProductsToWrite_ = productsToRead_.size();
	for (const auto& pr: fieldsToRead.productsForFiltersOnly) {
		datasets.insert(pr.dataset());
		productsToRead_.push_back(pr);
	}

	std::copy(datasets.begin(), datasets.end(), std::back_inserter(datasetsToLoad_));
	BOOST_LOG_TRIVIAL(info) << "the following datasets will be downloaded: " << put_list(datasetsToLoad_);

	// prepare averaging cells
	std::vector<AveragedVariable> averagingCells;
	std::size_t totalSize;
	std::vector<Field> fields;

	DatasetProductsMap productsToRead = parseProductsList(productsToRead_);
	for (const auto& dsp: productsToRead) {
		for (const auto& pr: dsp.second) {
			const FieldDesc& f = availableProducts[pr.dataset()].variable(pr.name());
			fields.emplace_back(f, totalSize);
			averagingCells.emplace_back(f.elementCount());
			totalSize++;//d? += f.elementCount();
		}
	}

	std::vector<std::unique_ptr<Writer> > writers;
	for (const Output& o: params_.outputs()) {
		writers.push_back(createWriterForOutput(o));
		writers.back()->initialize(fields);
	}

	initializeFilters(fields, rawFilters, averageDataFilters);

	std::size_t cellNo = 0;

	while (!chunkDownloader.eof()) {
		BOOST_LOG_TRIVIAL(info) << "Processing chunk [" << currentChunk.startTime << ','
		                        << currentChunk.endTime << ']' << std::endl;

		DataReader reader {currentChunk.startTime, params_.timeInterval(), rawFilters,
			               currentChunk.files, productsToRead, averagingCells, fields};

		while (!reader.eof() && !reader.fail()) {
			if (reader.readNextCell()) {
				bool cellPassedFiltering = true;
				for (const auto& filter: averageDataFilters) {
					if (!filter->test(averagingCells)) {
						cellPassedFiltering = false;
						break;
					}
				}
				if (cellPassedFiltering) {
					for (const std::unique_ptr<Writer>& writer: writers) {
						writer->write(cellNo, reader.cellMidTime(), averagingCells);
					}
				}
				++cellNo;
			}
		}

		currentChunk = chunkDownloader.nextChunk();
		if (currentChunk.empty()) {
			continue;
		}
	}
}

std::unique_ptr<cdownload::Writer> cdownload::Driver::createWriterForOutput(const cdownload::Output& output) const
{
	switch (output.format()) {
	case Output::Format::ASCII:
		return std::unique_ptr<cdownload::Writer>{new ASCIIWriter(params_.outputDir() / (output.name() + ".txt"))};
	case Output::Format::Binary:
		return std::unique_ptr<cdownload::Writer>{new BinaryWriter(params_.outputDir() / (output.name() + ".bin"))};
	default:
		throw std::logic_error("Support for this writer format is not implemented");
	}
}

void cdownload::Driver::createFilters(std::vector<std::shared_ptr<RawDataFilter> >& rawDataFilters,
                                      std::vector<std::shared_ptr<AveragedDataFilter> >& averagedDataFilters)
{
	// so far, filters list is static
	rawDataFilters.emplace_back(new Filters::BadDataFilter());
	rawDataFilters.emplace_back(new Filters::PlasmaSheetModeFilter());

	averagedDataFilters.emplace_back(new Filters::PlasmaSheet());
}

void cdownload::Driver::initializeFilters(const std::vector<Field>& fields,
                                          std::vector<std::shared_ptr<RawDataFilter> >& rawDataFilters,
                                          std::vector<std::shared_ptr<AveragedDataFilter> >& averagedDataFilters)
{
	for (auto& filter: rawDataFilters) {
		filter->initialize(fields);
	}

	for (auto& filter: averagedDataFilters) {
		filter->initialize(fields);
	}
}

cdownload::Driver::ProductsToRead
cdownload::Driver::collectAllProductsToRead(const std::map<cdownload::DatasetName, CDF::Info>& available,
                                            const std::vector<Output>& outputs,
                                            const std::vector<std::shared_ptr<Filter> >& filters)
{
	ProductsToRead res;
	for (const Output& o: outputs) {
		for (const auto& dsPrPair: o.products()) {
			auto i = available.find(dsPrPair.first);
			if (i == available.end()) {
				throw std::runtime_error("Dataset '" + dsPrPair.first + "' is unavailable");
			}
			std::vector<ProductName> dsVariables = i->second.variableNames();
//          const CDF::Info& info = i->second;
			for (const auto& pr: dsPrPair.second) {
				if (std::find(dsVariables.begin(), dsVariables.end(), pr) == dsVariables.end()) {
					throw std::runtime_error("Dataset '" + dsPrPair.first
					                         + "' does not contain product '" + pr.name() + '\'');
				}
				res.productsToWrite.push_back(pr);
			}
		}
	}

	for (const auto& filter: filters) {
		for (const ProductName& pr: filter->requiredProducts()) {
			if (std::find(res.productsToWrite.begin(), res.productsToWrite.end(), pr) == res.productsToWrite.end()) {
				res.productsForFiltersOnly.push_back(pr);
			}
		}
	}

	return res;
}

std::vector<cdownload::DatasetName>
cdownload::Driver::collectRequireddDatasets(const std::vector<Output>& outputs, const std::vector<std::shared_ptr<Filter> >& filters)
{
	std::set<DatasetName> res;
	for (const Output& o: outputs) {
		for (const auto& ds: o.datasetNames()) {
			res.insert(ds);
		}
	}

	for (const auto& filter: filters) {
		for (const ProductName& pr: filter->requiredProducts()) {
			res.insert(pr.dataset());
		}
	}

	std::vector<DatasetName> resVector;
	std::copy(res.begin(), res.end(), std::back_inserter(resVector));
	return resVector;
}
