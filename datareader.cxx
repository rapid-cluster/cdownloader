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

#include "datareader.hxx"

#include "datasource.hxx"
#include "cdfreader.hxx"
#include "filters/timefilter.hxx"

#include <algorithm>
#include <cassert>

#ifndef NDEBUG
#include <boost/lexical_cast.hpp>
#endif
#include <boost/log/trivial.hpp>

#include "config.h"

constexpr const std::size_t INVALID_INDEX = static_cast<std::size_t>(-1);

cdownload::DataReader::~DataReader() = default;

cdownload::DataReader::DataReader(const datetime& startTime, const datetime& endTime,
                                  const std::vector<std::shared_ptr<RawDataFilter> >& filters,
                                  std::map<DatasetName, std::shared_ptr<DataSource>>& datasources,
                                  const DatasetProductsMap& fieldsToRead,
                                  const std::vector<Field>& fields,
                                  const Filters::TimeFilter* timeFilter)
	: startTime_{startTime}
	, endTime_{endTime}
	, datasources_{datasources}
	, readers_{}
	, filters_{filters}
	, fields_{fields}
	, state_{ReaderState::OK}
	, timeFilter_{timeFilter}
{
	std::vector<ProductName> orderedProducts = expandProductsMap(fieldsToRead);
	if (fields_.size() != orderedProducts.size()) {
		throw std::logic_error("Fields array may not differ in size from CDF variables list");
	}

	for (std::size_t i = 0; i < fields_.size(); ++i) {
		assert(fields_[i].name() == orderedProducts[i]);
	}

	const DatasetProductsMap& productsToRead = fieldsToRead;

	// for each dataset we create a CDFReader object and store a map
	// variable index -> orderedCDFKeys index (and the last one is equal to the index in the
	// averaging cells array)

	for (const std::pair<DatasetName, std::vector<ProductName> >& p: productsToRead) {
		auto datasource = datasources_.at(p.first);
		datasource->setNextChunkStartTime(startTime_);
		auto chunk = datasource->nextChunk();
		assert(!chunk.empty());
		CDF::File cdf {chunk.file};
		CDF::Info info {cdf};
		std::vector<ProductName> variablesToReadFromDataset; // those we export plus timestamp
		std::vector<std::size_t> variableIndicies;

		const auto timestampVarName = info.timestampVariableName();
		const auto timeStampVarIter = std::find(p.second.begin(), p.second.end(), timestampVarName);
		bool timestampIsInOutput = timeStampVarIter != p.second.end();
		std::size_t timeStampVarIndex =
			timestampIsInOutput ? static_cast<std::size_t>(std::distance(p.second.begin(), timeStampVarIter)) : 0;

		if (!timestampIsInOutput) {
			variablesToReadFromDataset.push_back(info.timestampVariableName());
			variableIndicies.push_back(INVALID_INDEX);
		}
		std::transform(p.second.begin(), p.second.end(), std::back_inserter(variablesToReadFromDataset),
		               [](const ProductName& pr) {return pr.name();});

		for (std::size_t i = 0; i < p.second.size(); ++i) {
			const ProductName& pr = p.second[i];
			auto keyIterator = std::find(orderedProducts.begin(), orderedProducts.end(), pr);
			if (keyIterator == orderedProducts.end()) {
				throw std::logic_error("Error initializing DataReader: can not find product key '" + pr.name() + '\'');
			}
			variableIndicies.push_back(static_cast<std::size_t>(std::distance(orderedProducts.begin(), keyIterator)));
		}

		std::unique_ptr<CDF::Reader> reader {new CDF::Reader {cdf, variablesToReadFromDataset}};
		auto indexToStartFrom = reader->findTimestamp(startTime_.milliseconds(), 0);
		if (indexToStartFrom != 0) {
			BOOST_LOG_TRIVIAL(debug) << "Fast-forward to record " << indexToStartFrom << " for " << p.first;
		}

		const std::size_t firstVarIndex = timestampIsInOutput ? 0 : 1;
		for (std::size_t i = firstVarIndex; i < variablesToReadFromDataset.size(); ++i) {
			bufferPointers_.push_back(reader->bufferForVariable(i));
		}

		std::vector<std::shared_ptr<RawDataFilter>> filtersForDataset;
		for (const auto& f: filters) {
			std::vector<ProductName> filterProducts = f->requiredProducts();
			bool filterIsNeededForThisDS = false;
			if (filterProducts.empty()) {
				filterIsNeededForThisDS = true;
			} else {
				for (const auto& pr: filterProducts) {
					if (pr.dataset() == p.first) {
						filterIsNeededForThisDS = true;
						break;
					}
				}
			}
			if (filterIsNeededForThisDS) {
				filtersForDataset.push_back(f);
			}
		}

		DataSetReadingContext st (p.first, std::move(reader), variableIndicies,timeStampVarIndex,
			filtersForDataset, datasource, variablesToReadFromDataset);
		st.readRecordsCount = indexToStartFrom;

		readers_[p.first] = std::move(st);
	}
}

bool cdownload::DataReader::fail() const
{
	return static_cast<int>(state_) & static_cast<int>(ReaderState::Fail);
}

bool cdownload::DataReader::eof() const
{
	return static_cast<int>(state_) & static_cast<int>(ReaderState::EoF);
}

cdownload::DataReader::DataSetReadingContext::DataSetReadingContext()
	: datasetName()
	, reader()
	, indiciesInCells()
	, readRecordsCount(0)
	, timestampVariableIndex(0)
	, lastReadTimeStamp(0)
	, filters()
	, eof(false)
{
}


cdownload::DataReader::DataSetReadingContext::DataSetReadingContext(const DatasetName& aDataset,
	std::unique_ptr<CDF::Reader> && aReader,
	const std::vector<std::size_t>& indiciesInCellsParam,
	std::size_t aTimestampVariableIndex,
	const std::vector<std::shared_ptr<RawDataFilter> >& filtersParam,
	std::shared_ptr<DataSource> adatasource,
	const std::vector<ProductName> variablesToReadFromDatasetParam)
	: datasetName(aDataset)
	, reader{std::move(aReader)}
	, indiciesInCells(indiciesInCellsParam)
	, readRecordsCount(0)
	, timestampVariableIndex(aTimestampVariableIndex)
	, lastReadTimeStamp(-1)
	, filters(filtersParam)
	, eof(false)
	, datasource(adatasource)
	, variablesToReadFromDataset(variablesToReadFromDatasetParam)
{
}

bool cdownload::DataReader::advanceDataSource(cdownload::DataReader::DataSetReadingContext& context)
{
	DatasetChunk nextChunk = context.datasource->nextChunk();
	if (nextChunk.empty()) {
		return false;
	}
	CDF::File cdfFile(nextChunk.file);
	context.reader.reset(new CDF::Reader(cdfFile, context.variablesToReadFromDataset));
	context.readRecordsCount = 0;
	return true;
}

void cdownload::DataReader::setStateFlag(cdownload::DataReader::ReaderState flag, bool on)
{
	if (on) {
		state_ = static_cast<ReaderState>(static_cast<int>(state_) | static_cast<int>(flag));
	} else {
		state_ = static_cast<ReaderState>(static_cast<int>(state_) & ~static_cast<int>(flag));
	}
}



cdownload::AveragingDataReader::AveragingDataReader(const datetime& startTime, const datetime& endTime, timeduration cellLength, const std::vector<std::shared_ptr<RawDataFilter> >& filters, std::map<DatasetName, std::shared_ptr<DataSource> >& datasources, const DatasetProductsMap& fieldsToRead, std::vector<AveragedVariable>& cells, const std::vector<Field>& fields, const Filters::TimeFilter* timeFilter)
	: base(startTime, endTime, filters, datasources, fieldsToRead, fields, timeFilter)
	, currentStartTime_{startTime}
	, cellLength_{cellLength}
	, cells_{cells}
{
	if (cells.size() != fields.size()) {
		throw std::logic_error("Averaging cells array may not differ in size from CDF variables list");
	}
}

// this function reads next cell from the given reader (CDF file) and dumps values into the
// averaging cells
cdownload::DataReader::CellReadStatus
cdownload::AveragingDataReader::readNextCell(const datetime& cellStart, cdownload::DataReader::DataSetReadingContext& ds)
{
	const EpochRange outputCell = EpochRange::fromRange(cellStart.milliseconds(), (cellStart + cellLength_).milliseconds());
	double epoch = ds.lastReadTimeStamp;
#ifndef NDEBUG
	std::string outputCellString = boost::lexical_cast<std::string>(cellStart) + " + "
		+ boost::lexical_cast<std::string>(cellLength_);
	std::string epochCellString;
#endif
	bool anyRecordSurviedFiltering = false;
	do {
		bool readOk = false;
		do {
			ds.lastReadTimeStamp = epoch;
			readOk = ds.reader->readTimeStampRecord(ds.readRecordsCount++);
			if (!readOk && ds.reader->eof()) {
				if (advanceDataSource(ds)) {
					continue;
				} else {
					return CellReadStatus::EoF;
				}
			}
			epoch = *static_cast<const double*>(ds.reader->bufferForVariable(ds.timestampVariableIndex)); // EPCH16?
#ifndef NDEBUG
		epochCellString = datetime(epoch).CSAString();
#endif
		} while ((epoch < outputCell.begin()) && readOk); // we skip records with
		// epoch == 0, which indicates absence of data

		if (epoch > outputCell.end()) {
			--ds.readRecordsCount;
			if (ds.lastReadTimeStamp <= 0) {
				return CellReadStatus::NoRecordSurviedFiltering;
			} else {
				break;
			}
		}
		ds.lastReadTimeStamp = epoch;

		if (timeFilter() && !timeFilter()->test(epoch)) {
			continue;
		}

		readOk = ds.reader->readRecord(ds.readRecordsCount - 1, true); // we've read timestamp already
		if (!readOk) {
			return CellReadStatus::ReadError;
		}

		assert(outputCell.contains(epoch));

		bool filtersPassed = true;
		// the record was read successfully and belongs to the current output cell -> test by filters
		for (const auto& f: ds.filters) {
			if (!f->test(bufferPointers(), ds.datasetName)) {
				filtersPassed = false;
#ifdef DEBUG_LOG_EVERY_CELL
				BOOST_LOG_TRIVIAL(trace) << "\t Rejected by " << f->name() << " filter";
#endif
				break;
			}
		}

		if (!filtersPassed) {
			continue;
		} else {
			anyRecordSurviedFiltering = true;
		}

		// warning: it is important to set lastReadTimeStamp after filtering!
		// otherwise we might consider filtered out record as valid one on the next cycle step

		copyValuesToAveragingCells(ds);
	} while (outputCell.end() > epoch); // which means that currentDatasetCell completely falls inside of outputCell
	return anyRecordSurviedFiltering ? CellReadStatus::OK : CellReadStatus::NoRecordSurviedFiltering;

// 	return true;
}

cdownload::datetime cdownload::AveragingDataReader::cellMidTime() const
{
	return currentStartTime_ + cellLength_ / 2;
}

void cdownload::AveragingDataReader::copyValuesToAveragingCells(const cdownload::DataReader::DataSetReadingContext& ds)
{
	// test finished successfully -> append to the averaging cell
		// TODO: optimize inner loops
	for (std::size_t i = 0; i < ds.indiciesInCells.size(); ++i) {
		const std::size_t cellIndex = ds.indiciesInCells[i];
		if (cellIndex == INVALID_INDEX) {
			continue;
		}
		Field& f = fields()[cellIndex];
		AveragedVariable& av = cells_[cellIndex];
		switch (f.dataType()) {
			case FieldDesc::DataType::Real:
				for (std::size_t i = 0; i < f.elementCount(); ++i) {
					av[i].add(f.getReal(bufferPointers(), i));
				}
				break;
			case FieldDesc::DataType::SignedInt:
				for (std::size_t i = 0; i < f.elementCount(); ++i) {
					av[i].add(f.getLong(bufferPointers(), i));
				}
				break;
			case FieldDesc::DataType::UnsignedInt:
				for (std::size_t i = 0; i < f.elementCount(); ++i) {
					av[i].add(f.getULong(bufferPointers(), i));
				}
				break;
			default:
				throw std::runtime_error("Unexpected datatype");
		}
	}
}

bool cdownload::AveragingDataReader::readNextCell()
{
	for (AveragedVariable& cell: cells_) {
		cell.scheduleReset();
	}

	if (currentStartTime_ >= endTime()) {
		setStateFlag(ReaderState::EoF);
		return false;
	}

#ifdef DEBUG_LOG_EVERY_CELL
	BOOST_LOG_TRIVIAL(trace) << "Reading cell " << currentStartTime_ << " +- " << cellLength_;
#endif

	bool anyCellWasReadSuccesfully = false;
	bool eofInOneOfTheDatasets = false;
	for (auto& dsp: readers()) {
		CellReadStatus cellReadStatus = readNextCell(currentStartTime_, dsp.second);
		if (cellReadStatus == CellReadStatus::NoRecordSurviedFiltering) {
			anyCellWasReadSuccesfully = false;
#ifdef DEBUG_LOG_EVERY_CELL
			BOOST_LOG_TRIVIAL(trace) << "\t NRSF in dataset " << dsp.second.datasetName;
#endif
			break;
		}
		if (cellReadStatus == CellReadStatus::EoF) {
			eofInOneOfTheDatasets = true;
			break;
		}
		anyCellWasReadSuccesfully |= (cellReadStatus == CellReadStatus::OK);
	}
#ifdef DEBUG_LOG_EVERY_CELL
	BOOST_LOG_TRIVIAL(trace) << "Cell " << currentStartTime_ << " +- " << cellLength_ << " read " << anyCellWasReadSuccesfully;
#endif
	currentStartTime_ += cellLength_;
	if (eofInOneOfTheDatasets) {
		setStateFlag(ReaderState::EoF);
		return false;
	}
	if (!anyCellWasReadSuccesfully) {
		// check for EOF
		bool eof = false;
		for (auto& dsp: readers()) {
			if (dsp.second.reader->eof()) {
				eof = true;
				break;
			}
		}
		setStateFlag(ReaderState::EoF, eof);
	}

	return anyCellWasReadSuccesfully;
}


// DirectDataReader

cdownload::DirectDataReader::DirectDataReader(const datetime& startTime, const datetime& endTime, const std::vector<std::shared_ptr<RawDataFilter> >& filters, std::map<DatasetName, std::shared_ptr<DataSource> >& datasources, const DatasetProductsMap& fieldsToRead, const std::vector<Field>& fields, const Filters::TimeFilter* timeFilter)
	: base{startTime, endTime, filters, datasources, fieldsToRead, fields, timeFilter}
	, dsContext_{&readers().begin()->second}
{
	if (readers().size() != 1) {
		throw std::runtime_error("Direct reading is supported from single dataset only");
	}

	skipToTime(startTime, *dsContext_);
}


bool cdownload::DirectDataReader::readNextCell()
{
#ifdef DEBUG_LOG_EVERY_CELL
	BOOST_LOG_TRIVIAL(trace) << "Reading record #" << dsContext_->readRecordsCount;
#endif

	while (true) {
		double epoch = dsContext_->reader->readTimeStampRecord(dsContext_->readRecordsCount - 1);
		if (epoch > endTime().milliseconds()) {
			setStateFlag(ReaderState::EoF, true);
			return false;
		}
		bool readOk = dsContext_->reader->readRecord(dsContext_->readRecordsCount - 1, true); // we've read timestamp already
		if (!readOk) {
			setStateFlag(ReaderState::Fail);
			return false;
		}

		bool filtersPassed = true;
		// the record was read successfully and belongs to the current output cell -> test by filters
		for (const auto& f: dsContext_->filters) {
			if (!f->test(bufferPointers(), dsContext_->datasetName)) {
				filtersPassed = false;
#ifdef DEBUG_LOG_EVERY_CELL
				BOOST_LOG_TRIVIAL(trace) << "\t Rejected by " << f->name() << " filter";
#endif
				break;
			}
		}

		if (!filtersPassed) {
			continue;
		}

		return true;
	}
}

bool cdownload::DirectDataReader::skipToTime(const datetime& time, DataSetReadingContext& ds)
{
	while(true) {
		try {
			ds.readRecordsCount = ds.reader->findTimestamp(time.milliseconds(), ds.readRecordsCount);
		} catch (std::runtime_error&) {
			if (advanceDataSource(ds)) {
				continue;
			} else {
				throw;
			}
		}
	}
}
