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

#include "unpacker.hxx"
#include "cdfreader.hxx"

#include <algorithm>
#include <cassert>

constexpr const std::size_t INVALID_INDEX = static_cast<std::size_t>(-1);

cdownload::DataReader::DataReader(const datetime& startTime, timeduration cellLength,
                                  const std::vector<std::shared_ptr<RawDataFilter> >& filters,
                                  const std::map<DatasetName, DownloadedProductFile>& cdfFiles,
                                  const DatasetProductsMap& fieldsToRead,
                                  std::vector<AveragedVariable>& cells,
                                  const std::vector<Field>& fields)
	: startTime_{startTime}
	, cellLength_{cellLength}
	, readers_{}
	, cells_(cells)
	, filters_{filters}
	, fields_{fields}
	, fail_{false}
	, eof_{false}
{
	std::vector<ProductName> orderedProducts = expandProductsMap(fieldsToRead);
	if (cells.size() != orderedProducts.size()) {
		throw std::logic_error("Averaging cells array may not differ in size from CDF variables list");
	}
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
		CDF::File cdf {cdfFiles.at(p.first)};
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
					if (pr.name() == p.first) {
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
			filtersForDataset);

		readers_[p.first] = std::move(st);
	}
}

bool cdownload::DataReader::fail() const
{
	return fail_;
}

bool cdownload::DataReader::eof() const
{
	return eof_;
}

// const void * cdownload::DataReader::buffer(const cdownload::ProductName& var) const
// {
//  std::size_t index = std::distance(order)
// }

bool cdownload::DataReader::readNextCell()
{
	for (AveragedVariable& cell: cells_) {
		cell.scheduleReset();
	}

	bool anyCellWasReadSuccesfully = false;
	for (auto& dsp: readers_) {
		CellReadStatus cellReadStatus = readNextCell(startTime_, dsp.second);
		anyCellWasReadSuccesfully |=
			(cellReadStatus != CellReadStatus::EoF) &&
			(cellReadStatus != CellReadStatus::ReadError);
	}
	startTime_ += cellLength_;
	if (!anyCellWasReadSuccesfully) {
		// check for EOF
		// EOF only when all readers are in EOF state
		bool eof = true;
		for (auto& dsp: readers_) {
			if (!dsp.second.reader->eof()) {
				eof = false;
				break;
			}
		}
		eof_ = eof;
	}
	return anyCellWasReadSuccesfully;
}

namespace {
	class Cell {
	public:
		Cell(double mid, double halfWidth)
			: midEpoch_(mid)
			, halfWidth_(halfWidth) {
		}

		static Cell fromRange(double begin, double end) {
			assert(end >= begin);
			return Cell((begin+end)/2, (end - begin)/2);
		}

		double begin() const {
			return midEpoch_ - halfWidth_;
		}

		double end() const {
			return midEpoch_ + halfWidth_;
		}

		double mid() const {
			return midEpoch_;
		}

		double halfWidth() const {
			return halfWidth_;
		}

		double width() const {
			return halfWidth_ * 2;
		}

	private:
		double midEpoch_;
		double halfWidth_;
	};

	inline std::pair<Cell,bool> intersection(const Cell& c1, const Cell& c2)
	{

		const Cell cLower = c1.begin() < c2.begin() ? c1: c2;
		const Cell cUpper = c1.begin() < c2.begin() ? c2: c1;
		if (cLower.end() < cUpper.begin()) {
			return std::make_pair(Cell{0., 0.}, false);
		} else {
			double min = std::max(cLower.begin(), cUpper.begin());
			double max = std::min(cLower.end(), cUpper.end());
			return std::make_pair(Cell::fromRange(min, max), true);
		}
	}
}

// this function reads next cell from the given reader (CDF file) and dumps values into the
// averaging cells
cdownload::DataReader::CellReadStatus
cdownload::DataReader::readNextCell(const datetime& cellStart, cdownload::DataReader::DataSetReadingContext& ds)
{
	const Cell outputCell = Cell::fromRange(CDF::dateTimeToEpoch(cellStart), CDF::dateTimeToEpoch(cellStart + cellLength_));
	double weight = 1.0; // will be later modified if the cell to be read extends
	double epoch = ds.lastReadTimeStamp;
	do {
		bool readOk = false;
		do {
			ds.lastReadTimeStamp = epoch;
			readOk = ds.reader->readTimeStampRecord(ds.readRecordsCount++);
			epoch = *static_cast<const double*>(ds.reader->bufferForVariable(ds.timestampVariableIndex)); // EPCH16?
		} while ((epoch < outputCell.begin()) && readOk); // we skip records with
		// epoch == 0, which indicates absence of data

		const Cell previousDatasetCell {ds.lastWroteTimeStamp, (epoch - ds.lastReadTimeStamp)/2};
		const Cell currentDatasetCell {epoch, (epoch - ds.lastReadTimeStamp)/2};
		ds.lastReadTimeStamp = epoch;

		std::pair<Cell,bool> intersectionWithPrevious = intersection(previousDatasetCell, outputCell);
		std::pair<Cell,bool> intersectionWithCurrent = intersection(currentDatasetCell, outputCell);

		if (ds.lastWeight < 1 && ds.lastWeight > 0 && intersectionWithPrevious.second) {// we did not skip any cell
			// copy leftover of the cell, see below
			copyValuesToAveragingCells(ds, intersectionWithPrevious.first.width() / outputCell.width());
		}

		readOk = ds.reader->readRecord(ds.readRecordsCount - 1, true); // we've read timestamp already
		if (!readOk) {
			if (ds.reader->eof()) {
				return CellReadStatus::EoF;
			} else {
				return CellReadStatus::ReadError;
			}
		}

		// statistical weight is proportional to
		// the fraction of the DS cell which is inside our current cell
		weight = intersectionWithCurrent.first.width() / outputCell.width();
		if (!(weight > 0)) {
			ds.lastWeight = 0;
			break;
		}

		// the record was read successfully and belongs to the current output cell -> test by filters
		for (const auto& f: ds.filters) {
			if (!f->test(bufferPointers_, ds.datasetName)) {
				ds.lastWeight = 0; // this is too prevent making the current record into the next cell
				continue;
			}
		}

		// warning: it is important to set lastReadTimeStamp after filtering!
		// otherwise we might consider filtered out record as valid one on the next cycle step

		assert(weight > 0);
		assert(weight <= 1.);
		copyValuesToAveragingCells(ds, weight);
		ds.lastWeight = weight;
		ds.lastWroteTimeStamp = epoch;
	} while (weight >= 1.); // which means that currentDatasetCell completely falls inside of outputCell
	return CellReadStatus::PastCellEnd;

// 	return true;
}

cdownload::datetime cdownload::DataReader::cellMidTime() const
{
	return startTime_ + cellLength_ / 2;
}

void cdownload::DataReader::copyValuesToAveragingCells(const cdownload::DataReader::DataSetReadingContext& ds, double weight)
{
	// test finished successfully -> append to the averaging cell
		// TODO: optimize inner loops
	for (std::size_t i = 0; i < ds.indiciesInCells.size(); ++i) {
		const std::size_t cellIndex = ds.indiciesInCells[i];
		if (cellIndex == INVALID_INDEX) {
			continue;
		}
		Field& f = fields_[cellIndex];
		AveragedVariable& av = cells_[cellIndex];
		switch (f.dataType()) {
			case FieldDesc::DataType::Real:
				for (std::size_t i = 0; i < f.elementCount(); ++i) {
					av[i].add(f.getReal(bufferPointers_, i), weight);
				}
				break;
			case FieldDesc::DataType::SignedInt:
				for (std::size_t i = 0; i < f.elementCount(); ++i) {
					av[i].add(f.getLong(bufferPointers_, i), weight);
				}
				break;
			case FieldDesc::DataType::UnsignedInt:
				for (std::size_t i = 0; i < f.elementCount(); ++i) {
					av[i].add(f.getULong(bufferPointers_, i), weight);
				}
				break;
			default:
				throw std::runtime_error("Unexpected datatype");
		}
	}
}

cdownload::DataReader::DataSetReadingContext::DataSetReadingContext()
	: datasetName()
	, reader()
	, indiciesInCells()
	, readRecordsCount(0)
	, timestampVariableIndex(0)
	, lastReadTimeStamp(0)
	, lastWroteTimeStamp(0)
	, lastWeight(-1.)
	, filters()
	, eof(false)
{
}


cdownload::DataReader::DataSetReadingContext::DataSetReadingContext(const DatasetName& aDataset,
	std::unique_ptr<CDF::Reader> && aReader,
	const std::vector<std::size_t>& indiciesInCellsParam,
	std::size_t aTimestampVariableIndex,
	const std::vector<std::shared_ptr<RawDataFilter> >& filtersParam)
	: datasetName(aDataset)
	, reader{std::move(aReader)}
	, indiciesInCells(indiciesInCellsParam)
	, readRecordsCount(0)
	, timestampVariableIndex(aTimestampVariableIndex)
	, lastReadTimeStamp(0)
	, lastWroteTimeStamp(0)
	, lastWeight(-1.)
	, filters(filtersParam)
	, eof(false)
{
}
