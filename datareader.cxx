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
	, cells_{cells}
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

		DataSetReadingContext st {p.first, std::move(reader), variableIndicies, 0, timeStampVarIndex,
			0., -1., filtersForDataset, false};

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

// this function reads next cell from the given reader (CDF file) and dumps values into the
// averaging cells
cdownload::DataReader::CellReadStatus
cdownload::DataReader::readNextCell(const datetime& cellStart, cdownload::DataReader::DataSetReadingContext& ds)
{
	const double startEpoch = CDF::dateTimeToEpoch(cellStart);
	const double endEpoch = CDF::dateTimeToEpoch(cellStart + cellLength_);
	const double ourCellLength = endEpoch - startEpoch;
	double weight = 1.0; // will be later modified if the cell to be read extends
	double epoch = 0;
	do {
		bool readOk = false;
		do {
			readOk = ds.reader->readTimeStampRecord(ds.readRecordsCount++);
			epoch = *static_cast<const double*>(ds.reader->bufferForVariable(ds.timestampVariableIndex)); // EPCH16?
		} while ((epoch < startEpoch) && readOk); // we skip records with epoch == 0, which indicate absence of data

		if (ds.lastWeight < 1 && ds.lastWeight > 0 &&
			(epoch - ds.lastReadTimeStamp <= ourCellLength) // we did not skip any cell
		) {
			copyValuesToAveragingCells(ds, 1. - ds.lastWeight); // copy leftover of the cell, see below
		}

		readOk = ds.reader->readRecord(ds.readRecordsCount, true); // we've read timestamp already
		if (!readOk) {
			if (ds.reader->eof()) {
				return CellReadStatus::EoF;
			} else {
				return CellReadStatus::ReadError;
			}
		}

		const double dsCellLength = epoch - ds.lastReadTimeStamp;

		if (epoch + dsCellLength > endEpoch) {
			// compute what part of the DS cell is inside our current cell
			weight = (endEpoch - epoch) / dsCellLength + 0.5;
			// just return
		}
		ds.lastReadTimeStamp = epoch;

		// the record was read successfully -> test by filters

		for (const auto& f: ds.filters) {
			if (!f->test(bufferPointers_, ds.datasetName)) {
				continue;
			}
		}

		copyValuesToAveragingCells(ds, weight);
		ds.lastWeight = weight;
	} while (weight >= 1.);
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
