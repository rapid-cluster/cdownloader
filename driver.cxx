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
#include "datareader.hxx"
#include "datasource.hxx"
#include "downloader.hxx"
#include "field.hxx"
#include "metadata.hxx"
#include "parameters.hxx"
#include "unpacker.hxx"

#include "filters/baddata.hxx"
#include "filters/plasmasheet.hxx"
#include "filters/quality.hxx"

#include "writers/ASCIIWriter.hxx"
#include "writers/BinaryWriter.hxx"

#include <boost/filesystem/operations.hpp>
#include <boost/log/trivial.hpp>

#include <fstream>
#include <algorithm>
#include <set>


namespace {

	void extractInfo(const std::map<cdownload::DatasetName, cdownload::DatasetChunk>& files,
	                 std::map<cdownload::DatasetName, cdownload::CDF::Info>& info,
	                 std::vector<cdownload::ProductName>& cdfKeys)
	{
		for (const std::pair<cdownload::DatasetName, cdownload::DatasetChunk>& p: files) {
			cdownload::CDF::Info i = cdownload::CDF::Info(cdownload::CDF::File(p.second.file));
			info[p.first] = i;
			auto varNames = i.variableNames();
			std::copy(varNames.begin(), varNames.end(), std::back_inserter(cdfKeys));
		}
	}

	std::vector<cdownload::Output> expandOutputs(const std::vector<cdownload::Output>& outputs,
	                                             const std::map<cdownload::DatasetName, cdownload::CDF::Info>& availableProducts)
	{
		using cdownload::ProductName;
		using cdownload::Output;

		assert(!availableProducts.empty());
		std::vector<Output> res;
		for (const Output& output: outputs) {
			BOOST_LOG_TRIVIAL(trace) << "Expanding output: " << output;
			std::map<std::string, std::vector<ProductName> > expandedProductsMap;
			for (const std::string& ds: output.datasetNames()) {
				auto i = availableProducts.find(ds);
				if (i == availableProducts.end()) {
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
		BOOST_LOG_TRIVIAL(trace) << "Time range for dataset " << dataset.name() << ": [" <<
			dataset.minTime() << ',' << dataset.maxTime() << ']';
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

	// have to get first chunks separately in order to detect dataset products
	std::map<DatasetName, std::shared_ptr<DataSource>> datasources;
	std::map<DatasetName, DatasetChunk> chunks;
	for (const auto& ds: requiredDatasets) {
		datasources[ds] = std::shared_ptr<DataSource>(new DataSource(ds, params_, meta));
		chunks[ds] = datasources[ds]->nextChunk();
	}

	std::map<cdownload::DatasetName, CDF::Info> availableProducts;
	std::vector<ProductName> orderedCDFKeys;
	extractInfo(chunks, availableProducts, orderedCDFKeys);

	BOOST_LOG_TRIVIAL(debug) << "Collected CDF variables: " << put_list(orderedCDFKeys);

	std::vector<Output> expandedOutputs = expandOutputs(params_.outputs(), availableProducts);

	productsToRead_.clear();
	datasetsToLoad_.clear();

	ProductsToRead fieldsToRead = collectAllProductsToRead(availableProducts, expandedOutputs, allFilters);
	BOOST_LOG_TRIVIAL(trace) << "Fields to read (write): " << put_list(fieldsToRead.productsToWrite);
	BOOST_LOG_TRIVIAL(trace) << "Fields to read (filter): " << put_list(fieldsToRead.productsForFiltersOnly);

	assert(!fieldsToRead.productsToWrite.empty());

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
	BOOST_LOG_TRIVIAL(info) << "The following datasets will be downloaded: " << put_list(datasetsToLoad_);
	BOOST_LOG_TRIVIAL(trace) << "The following products will be read: " << put_list(productsToRead_);

	// prepare averaging cells
	std::vector<AveragedVariable> averagingCells;
	std::size_t totalSize = 0;
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

	BOOST_LOG_TRIVIAL(trace) << "Collected fields: " << put_list(fields);

	std::map<std::string,std::vector<Field>> fieldsForWriters;
	for (const Output& o: expandedOutputs) {
		fieldsForWriters[o.name()] = std::vector<Field>();
		for (const std::pair<DatasetName, std::vector<ProductName>>& dsp: o.products()) {
			for (const ProductName& pr: dsp.second) {
				auto fi = std::find_if(fields.begin(), fields.end(), [&pr](const Field& f){
					return f.name() == pr.name();
				});
				if (fi == fields.end()) {
					BOOST_LOG_TRIVIAL(error) << "Can not find product " << pr << " in fields array";
				}
				assert(fi != fields.end());
				fieldsForWriters[o.name()].push_back(*fi);
			}
		}
	}

	BOOST_LOG_TRIVIAL(debug) << "Creating writers";

	std::vector<std::unique_ptr<Writer> > writers;
	for (const Output& o: params_.outputs()) {
		writers.push_back(createWriterForOutput(o));
		writers.back()->initialize(fieldsForWriters[o.name()]);
	}

	initializeFilters(fields, rawFilters, averageDataFilters);

	std::size_t cellNo = 0;

	if (params_.continueDownloading()) {
		// check that all outputs are at the same cell number
		// first collect cell numbers
		std::vector<std::size_t> lastCellNumbers;
		std::size_t lcn;
		for (std::size_t outputIndex = 0; outputIndex < writers.size(); ++outputIndex) {
			if (writers[outputIndex]->canAppend(lcn)) {
				lastCellNumbers.push_back(lcn);
			} else {
				BOOST_LOG_TRIVIAL(error) << "Can not append to output '" <<
					params_.outputs()[outputIndex].name() << '\'';
				throw std::runtime_error("Can not append to output '" + params_.outputs()[outputIndex].name() + '\'');
			}
		}

		BOOST_LOG_TRIVIAL(trace) << "Found last cell indexes:";
		for (std::size_t outputIndex = 0; outputIndex < writers.size(); ++outputIndex) {
			BOOST_LOG_TRIVIAL(trace) << "\t" << params_.outputs()[outputIndex].name() <<
				" : " << lastCellNumbers[outputIndex];
		}

		// not check collected values
		for (std::size_t i = 1; i < lastCellNumbers.size(); ++i) {
			if (lastCellNumbers[i] != lastCellNumbers[0]) {
				BOOST_LOG_TRIVIAL(error) << "All last cell indexes have to be equal. Exiting";
				throw std::runtime_error("All last cell indexes have to be equal");
			}
		}

		BOOST_LOG_TRIVIAL(debug) << "Appending to output files seems possible";
		// everything seems to be OK, then:
		// 1. fast-forward cellNo
		cellNo = lastCellNumbers[0] + 1;
		// 2. reinitialize chunkDownloader
		datetime startTime = availableStartDateTime + params_.timeInterval() * cellNo;
		BOOST_LOG_TRIVIAL(info) << "Fast forwarding to " << startTime;
		for (auto& dsp: datasources) {
			dsp.second->setNextChunkStartTime(startTime);
		}
		// 3. ...and download it
// 		currentChunk = chunkDownloader.nextChunk();
	} else {
		BOOST_LOG_TRIVIAL(debug) << "Truncating output files";
		for (auto& writer: writers) {
			writer->truncate();
			writer->writeHeader();
		}
	}

	DataReader reader {availableStartDateTime, availableEndDateTime, params_.timeInterval(),
		rawFilters, datasources,  productsToRead, averagingCells, fields};
// 			BOOST_LOG_TRIVIAL(info) << "Processing chunk [" << currentChunk.startTime << ','
// 			                        << currentChunk.endTime << ']' << std::endl;

	while (!reader.eof() && !reader.fail()) {
		for (;reader.readNextCell(); ++cellNo) {
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
		}
	}
}

std::unique_ptr<cdownload::Writer> cdownload::Driver::createWriterForOutput(const cdownload::Output& output) const
{
	switch (output.format()) {
	case Output::Format::ASCII: {
		auto res = std::unique_ptr<cdownload::Writer>{new ASCIIWriter()};
		res->open(params_.outputDir() / (output.name() + ".txt"));
		return res;
	}
	case Output::Format::Binary: {
		auto res = std::unique_ptr<cdownload::Writer>{new BinaryWriter()};
		res->open(params_.outputDir() / (output.name() + ".bin"));
		return res;
	}
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

	for (const auto& qfp: params_.qualityFilters()) {
		rawDataFilters.emplace_back(new Filters::QualityFilter(qfp.product, qfp.minQuality));
	}
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
		assert(!o.products().empty());
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
