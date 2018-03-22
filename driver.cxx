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
#include "cdf/reader.hxx"
#include "datareader.hxx"
#include "dataprovider.hxx"
#include "field.hxx"
#include "fieldbuffer.hxx"
#include "parameters.hxx"

#include "filters/baddata.hxx"
#include "filters/blankdata.hxx"
#include "filters/density.hxx"
#include "filters/nightside.hxx"
#include "filters/plasmasheet.hxx"
#include "filters/quality.hxx"
#include "filters/timefilter.hxx"

#include "writers/ASCIIWriter.hxx"
#include "writers/BinaryWriter.hxx"

#include <boost/filesystem/operations.hpp>
#include <boost/log/trivial.hpp>

#include <fstream>
#include <iostream>
#include <algorithm>
#include <memory>
#include <set>

#include "config.h"

#include "csa/dataprovider.hxx"
#include "omni/dataprovider.hxx"

namespace {

	const std::string CSA_PROVIDER_NAME = "CSA";
	const std::string OMNI_PROVIDER_NAME = "OMNI";

	struct DataProvidersRegistrator {
		DataProvidersRegistrator() {
			cdownload::DataProviderRegistry::instance().registerProvider(CSA_PROVIDER_NAME,
					std::unique_ptr<cdownload::DataProvider>(new cdownload::csa::DataProvider()));
			cdownload::DataProviderRegistry::instance().registerProvider(OMNI_PROVIDER_NAME,
					std::unique_ptr<cdownload::DataProvider>(new cdownload::omni::DataProvider()));

		}
		~DataProvidersRegistrator() {
			cdownload::DataProviderRegistry::instance().unregisterProvider(CSA_PROVIDER_NAME);
			cdownload::DataProviderRegistry::instance().unregisterProvider(OMNI_PROVIDER_NAME);
		}
	};

	DataProvidersRegistrator dataProvidersRegistrator;

	const cdownload::DataProvider& dataProvider(const cdownload::DatasetName& name) {
		if (name == OMNI_PROVIDER_NAME) {
			return cdownload::DataProviderRegistry::instance().provider(OMNI_PROVIDER_NAME);
		}
		return cdownload::DataProviderRegistry::instance().provider(CSA_PROVIDER_NAME);
	}

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
				if (ProductName::isPseudoDataset(ds)) {
					// TODO make expansion for those too
					expandedProductsMap[ds] = output.productsForDataset(ds);
				} else {
					auto i = availableProducts.find(ds);
					if (i == availableProducts.end()) {
						throw std::runtime_error("Dataset '" + ds + "' is not present in the downloaded files");
					}
					std::vector<cdownload::ProductName> expandedProducts =
						cdownload::expandWildcardsCaseSensitive(
							output.productsForDataset(ds), i->second.variableNames());
					expandedProductsMap[ds] = expandedProducts;
				}
			}
			res.emplace_back(output.name(), output.format(), expandedProductsMap);
		}
		return res;
	}
}

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

	// sure enough this is before Cluster II launch date
	datetime availableStartDateTime = makeDateTime(1999, 1, 1);
	datetime availableEndDateTime = datetime::utcNow();

	std::vector<DatasetName> requiredDatasets = collectRequiredDatasets(params_.outputs(), allFilters);

	for (const auto& ds: requiredDatasets) {
		if (ProductName::isPseudoDataset(ds)) {
			continue;
		}
		auto dataset = dataProvider(ds).metadata()->dataset(ds);
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

	const datetime actualStartDateTime = std::max(params_.startDate(), availableStartDateTime);
	const datetime actualEndtDateTime = std::min(params_.endDate(), availableEndDateTime);

	BOOST_LOG_TRIVIAL(info) << "Using time range: ["
	                        << actualStartDateTime << ',' << actualEndtDateTime << ']';


//  datetime currentChunkStartTime = availableStartDateTime;

	// have to get first chunks separately in order to detect dataset products
	std::map<DatasetName, std::shared_ptr<DataSource> > datasources;
	std::map<DatasetName, DatasetChunk> chunks;
	for (const auto& ds: requiredDatasets) {
		datasources[ds] = std::shared_ptr<DataSource>(dataProvider(ds).datasource(ds, params_));
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
	BOOST_LOG_TRIVIAL(trace) << "Filter variables: ";
	for (const auto& fp: fieldsToRead.filterVariables) {
		BOOST_LOG_TRIVIAL(trace) << '\t' << fp.first->name() << ": " << put_list(fp.second);
	}

	std::vector<FieldDesc> allFilterFields;
	for (const auto& fp: fieldsToRead.filterFields) {
		std::copy(fp.second.begin(), fp.second.end(), std::back_inserter(allFilterFields));
	}

	FieldBuffer filterVariablesBuffer(allFilterFields);

	std::vector<void*> filterVariables = filterVariablesBuffer.writeBuffers();
	std::vector<const void*> filterVariablesForWriters = filterVariablesBuffer.readBuffers();

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

	for (const Output& o: expandedOutputs) {
		for (const std::pair<DatasetName, std::vector<ProductName> >& dsp: o.products()) {
			for (const ProductName& pr: dsp.second) {
				if (pr.isPseudoDataset()) {
					continue;
				}
				auto fi = std::find_if(fields.begin(), fields.end(), [&pr](const Field& f) {
					return f.name() == pr.name();
				});
				if (fi == fields.end()) {
					BOOST_LOG_TRIVIAL(error) << "Can not find product " << pr << " in fields array";
				}
			}
		}
	}

	BOOST_LOG_TRIVIAL(debug) << "Creating writers";

	std::vector<std::unique_ptr<Writer> > writers;
	for (const Output& o: params_.outputs()) {
		writers.push_back(createWriterForOutput(o, fields, filterVariablesBuffer.fields()));
	}

	if (!params_.allowBlanks()) {
		addBlankDataFilters(fields, rawFilters);
	}

	initializeFilters(fields, filterVariablesBuffer.fields(), rawFilters, averageDataFilters);

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
		datetime startTime = actualStartDateTime + params_.timeInterval() * cellNo;
		BOOST_LOG_TRIVIAL(info) << "Fast forwarding to " << startTime;
		for (auto& dsp: datasources) {
			dsp.second->setNextChunkStartTime(startTime);
		}
		// 3. ...and download it
//      currentChunk = chunkDownloader.nextChunk();
	} else {
		BOOST_LOG_TRIVIAL(debug) << "Truncating output files";
		for (auto& writer: writers) {
			writer->truncate();
			writer->writeHeader();
		}
	}

	std::unique_ptr<Filters::TimeFilter> timeFilter;
	if (!params_.timeRangesFileName().empty()) {
		timeFilter.reset(new Filters::TimeFilter(params_.timeRangesFileName()));
	}


	if (params_.disableAveraging()) {
		DirectDataReader reader(actualStartDateTime, actualEndtDateTime,
		                                  rawFilters, datasources, productsToRead, fields, timeFilter.get());
		std::vector<DirectDataWriter*> dWriters;
		for (const std::unique_ptr<Writer>& writer: writers) {
			dWriters.push_back(dynamic_cast<DirectDataWriter*>(writer.get()));
		}

		for (; !reader.eof() && !reader.fail(); ++cellNo) {
			auto readResult = reader.readNextCell();
			if (readResult.first) {
				bool cellPassedFiltering = true;
#if 0
				for (const auto& filter: averageDataFilters) {
					if (!filter->test(averagingCells)) {
						cellPassedFiltering = false;
#ifdef DEBUG_LOG_EVERY_CELL
						BOOST_LOG_TRIVIAL(trace) << "Rejecting Cell " << cellNo << " (" << readResult.second << ")";
#endif
						break;
					}
				}
#endif
				if (cellPassedFiltering) {
#ifdef DEBUG_LOG_EVERY_CELL
					BOOST_LOG_TRIVIAL(trace) << "Writing Cell " << readResult.second;
#endif
					for (DirectDataWriter* writer: dWriters) {
						writer->write(cellNo, readResult.second, {reader.bufferPointers(), filterVariablesForWriters});
					}
				}
			}
		}
	} else {
		AveragingDataReader reader(actualStartDateTime, actualEndtDateTime, params_.timeInterval(),
		                           rawFilters, datasources, productsToRead, averagingCells, fields, timeFilter.get());
		std::vector<AveragedDataWriter*> aWriters;
		for (const std::unique_ptr<Writer>& writer: writers) {
			aWriters.push_back(dynamic_cast<AveragedDataWriter*>(writer.get()));
		}

		for (; !reader.eof() && !reader.fail(); ++cellNo) {
			auto readResult = reader.readNextCell();
			if (readResult.first) {
				bool cellPassedFiltering = true;
				for (const auto& filter: averageDataFilters) {
					if (!filter->test(averagingCells, filterVariables)) {
						cellPassedFiltering = false;
#ifdef DEBUG_LOG_EVERY_CELL
						BOOST_LOG_TRIVIAL(trace) << "Rejecting Cell " << cellNo << " (" << readResult.second << ")";
#endif
						break;
					}
				}
				if (cellPassedFiltering) {
#ifdef DEBUG_LOG_EVERY_CELL
					BOOST_LOG_TRIVIAL(trace) << "Writing Cell " << readResult.second;
#endif
					for (AveragedDataWriter* writer: aWriters) {
						writer->write(cellNo, readResult.second, {averagingCells}, {filterVariablesForWriters});
					}
				}
			}
		}
	}
}

std::unique_ptr<cdownload::Writer>
cdownload::Driver::createWriterForOutput(const cdownload::Output& output,
                     const std::vector<Field>& dataFields, const std::vector<Field>& filterVariables) const
{

// 	fieldsForWriters[o.name()], params_.writeEpoch()
	std::vector<Field> fieldsForWriters;
	std::vector<Field> filterVariableForWriter;

	std::copy_if(dataFields.begin(), dataFields.end(), std::back_inserter(fieldsForWriters),
				 [&](const Field& f) {
					 return contains(output.productsForDatasetOrDefault(f.name().dataset(), {}), f.name());
				 });

	std::copy_if(filterVariables.begin(), filterVariables.end(), std::back_inserter(filterVariableForWriter),
				 [&](const Field& f) {
					 return contains(output.productsForDatasetOrDefault(f.name().dataset(), {}), f.name());
				 });

	std::unique_ptr<cdownload::Writer> res;
	switch (output.format()) {
	case Output::Format::ASCII: {
		if (params_.disableAveraging()) {
			res.reset(new DirectASCIIWriter({fieldsForWriters, filterVariableForWriter}, params_.writeEpoch()));
		} else {
			res.reset(new AveragedDataASCIIWriter({fieldsForWriters}, {filterVariableForWriter}, params_.writeEpoch()));
		}
		res->open(params_.outputDir() / (output.name() + ".txt"));
		return res;
	}
	case Output::Format::Binary: {
		if (params_.disableAveraging()) {
			res.reset(new DirectBinaryWriter({fieldsForWriters, filterVariableForWriter}, params_.writeEpoch()));
		} else {
			res.reset(new AveragedDataBinaryWriter({fieldsForWriters}, {filterVariableForWriter}, params_.writeEpoch()));
		}
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

	// 1. Find all products from (virtual) dataset $FILTER, if any
	// 2. get filter names from this list
	// 3. create all required filters

	bool isPlasmaSheetFilterNeeded = params_.plasmaSheetFilter();
	const bool isPlasmaSheetFilterActive = params_.plasmaSheetFilter();

	for (const auto& output: params_.outputs()) {
		const auto& productsMap = output.products();
		if (productsMap.count("$FILTER") == 0) {
			continue;
		}

		const auto& filterProducts = productsMap.at("$FILTER");
		for (const ProductName& pr: filterProducts) {
			// here dataset name is the name of the filter
			if (Filter::productBelongsToFilter(pr, Filters::PlasmaSheet::filterName())) {
				isPlasmaSheetFilterNeeded = true;
			}
		}
	}

	// better to be the first one to simplify debugging
	rawDataFilters.emplace_back(new Filters::BadDataFilter());

	if (params_.onlyNightSide()) {
		rawDataFilters.emplace_back(new Filters::NightSide(params_.spacecraftName()));
	}

	if (params_.plasmaSheetFilter()) {
		rawDataFilters.emplace_back(new Filters::PlasmaSheetModeFilter(params_.spacecraftName()));
	}

	for (const auto& qfp: params_.qualityFilters()) {
		rawDataFilters.emplace_back(new Filters::QualityFilter(qfp.product, qfp.minQuality));
	}


	if (!params_.disableAveraging()) {
		for (const auto& dfp: params_.densityyFilters()) {
			const ProductName product = dfp.source == DensitySource::CODIF ?
			                            ProductName("density", params_.spacecraftName(), "CP_CIS-CODIF_HS_H1_MOMENTS") : ProductName("density", params_.spacecraftName(), "CP_CIS-HIA_ONBOARD_MOMENTS");
			averagedDataFilters.emplace_back(new Filters::H1DensityFilter(product, dfp.minDensity));
		}

		if (isPlasmaSheetFilterNeeded) {
			averagedDataFilters.emplace_back(new Filters::PlasmaSheet(params_.plasmaSheetMinR(), params_.spacecraftName()));
			averagedDataFilters.back()->enable(isPlasmaSheetFilterActive);
		}
	}
}

void cdownload::Driver::addBlankDataFilters(const std::vector<Field>& fields,
                                            std::vector<std::shared_ptr<RawDataFilter> >& rawDataFilters)
{
	std::map<ProductName, double> productsForFiltering;
	for (const auto& field: fields) {
		if (!std::isnan(field.fillValue())) {
			productsForFiltering[field.name()] = field.fillValue();
		}
	}
	if (!productsForFiltering.empty()) {
		rawDataFilters.emplace_back(new Filters::BlankDataFilter(productsForFiltering));
	}
}

void cdownload::Driver::initializeFilters(const std::vector<Field>& fields,
                                          const std::vector<Field>& filterVariables,
                                          std::vector<std::shared_ptr<RawDataFilter> >& rawDataFilters,
                                          std::vector<std::shared_ptr<AveragedDataFilter> >& averagedDataFilters)
{
	for (auto& filter: rawDataFilters) {
		filter->initialize(fields, filterVariables);
	}

	for (auto& filter: averagedDataFilters) {
		filter->initialize(fields, filterVariables);
	}
}


cdownload::Driver::ProductsToRead
cdownload::Driver::collectAllProductsToRead(const std::map<cdownload::DatasetName, CDF::Info>& available,
                                            const std::vector<Output>& outputs,
                                            const std::vector<std::shared_ptr<Filter> >& filters)
{
	std::vector<ProductName> requestedFilterVariables;
	ProductsToRead res;
	for (const Output& o: outputs) {
		if (o.products().empty()) {
			throw std::runtime_error("Output '" + o.name() + "' does not contain products");
		}

		for (const auto& dsPrPair: o.products()) {
			// store filter variables for later
			if (ProductName::isPseudoDataset(dsPrPair.first)) {
				std::copy(dsPrPair.second.begin(), dsPrPair.second.end(), std::back_inserter(requestedFilterVariables));
				continue;
			}
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

	// split filter variables array onto filter name -> variables dictionary
	std::map<std::string, std::vector<std::string>> variablesPerFilter;
	std::size_t foundFiltersWithVariables = 0;
	for (const ProductName& rpn: requestedFilterVariables) {
		auto rpnPair = Filter::splitParameterName(rpn.shortName());
		auto i = variablesPerFilter.find(rpnPair.first);
		if (i == variablesPerFilter.end()) {
			i = variablesPerFilter.insert({rpnPair.first, std::vector<std::string>()}).first;
		}
		i->second.push_back(rpnPair.second);
	}

	std::set<std::string> filterNames;

	for (const auto& filter: filters) {
		res.filterVariables[filter.get()] = {};
		const auto filterName = filter->name();
		if (filterNames.count(filterName)) {
			throw std::runtime_error("Two filters with the same name ('" + filterName + "') are not allowed");
		}
		filterNames.insert(filterName);

		auto iv = variablesPerFilter.find(filterName);
		if (iv != variablesPerFilter.end()) {
			res.filterFields[filter.get()] = filter->variables();
			const auto& filterVariables = res.filterFields[filter.get()];
			std::set<std::string> shortNamedFilterVariables;
			std::transform(filterVariables.begin(), filterVariables.end(),
			               std::inserter(shortNamedFilterVariables, shortNamedFilterVariables.begin()),
			               [](const FieldDesc& fd){
			                   return Filter::splitParameterName(fd.name().shortName()).second;
			                });


			for (const std::string& rsn: iv->second) {
				if (shortNamedFilterVariables.count(rsn)) {
					res.filterVariables[filter.get()].push_back(Filter::composeProductName(rsn, filter->name()));
				}
			}
			++foundFiltersWithVariables;
		}

		for (const ProductName& pr: filter->requiredProducts()) {
			if (std::find(res.productsToWrite.begin(), res.productsToWrite.end(), pr) == res.productsToWrite.end()) {
				res.productsForFiltersOnly.push_back(pr);
			}
		}
	}

	if (foundFiltersWithVariables != variablesPerFilter.size()) {
		throw std::runtime_error("Not all filter variables have been found");
	}

	return res;
}

std::vector<cdownload::DatasetName>
cdownload::Driver::collectRequiredDatasets(const std::vector<Output>& outputs, const std::vector<std::shared_ptr<Filter> >& filters)
{
	std::set<DatasetName> res;
	for (const Output& o: outputs) {
		for (const auto& ds: o.datasetNames()) {
			if (!ProductName::isPseudoDataset(ds)) {
				res.insert(ds);
			}
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

void cdownload::Driver::listDatasets()
{
	std::cerr << "Available datasets:" << std::endl << std::flush;
	for (const auto& pp: DataProviderRegistry::instance()) {
		std::unique_ptr<Metadata> meta = pp.second->metadata();
		const auto datasets = meta->datasets();
		for (const auto& ds: datasets) {
			std::cout << pp.first << '.' << ds << std::endl;
		}
	}
}

void cdownload::Driver::listProducts(const cdownload::DatasetName& dataset)
{
	const auto dotPos = dataset.find('.');
	if (dotPos == DatasetName::npos) {
		std::cerr << "Dataset name has to contain data provider name (<data_provider>.<dataset>)" << std::endl;
		return;
	}

	const auto providerName = dataset.substr(0, dotPos);
	const auto dsName = dataset.substr(dotPos + 1);

	const auto& provider = DataProviderRegistry::instance().provider(providerName);
	const auto ds = provider.metadata()->dataset(dsName);
	for (const auto& f: ds.fields()) {
		std::cout << f << std::endl;
	}
}
