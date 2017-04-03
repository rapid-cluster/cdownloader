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

#include "ASCIIWriter.hxx"

#include "../util.hxx"
#include "../field.hxx"

#include <fstream>
#include <limits>

#include <boost/log/trivial.hpp>

namespace {
	template <class Data>
	void writeArray(std::ostream& os, const void* ptr, std::size_t elementsCount)
	{
		const Data* array = static_cast<const Data*>(ptr);
		for (std::size_t i = 0; i<elementsCount; ++i) {
			os << '\t' << array[i];
		}
	}

	void writeRawField(const cdownload::Field& f, const std::vector<const void*>& line, std::ostream& os)
	{
		using cdownload::FieldDesc;
		switch (f.dataType()) {
		case FieldDesc::DataType::Real:
			switch (f.dataSize()) {
			case 4:
				writeArray<float>(os, line[f.offset()], f.elementCount());
				break;
			case 8:
				writeArray<double>(os, line[f.offset()], f.elementCount());
				break;
			}
			break;
		case FieldDesc::DataType::SignedInt:
			switch (f.dataSize()) {
			case 1:
				writeArray<char>(os, line[f.offset()], f.elementCount());
				break;
			case 2:
				writeArray<short>(os, line[f.offset()], f.elementCount());
				break;
			case 4:
				writeArray<int>(os, line[f.offset()], f.elementCount());
				break;
			case 8:
				writeArray<long>(os, line[f.offset()], f.elementCount());
				break;
			}
			break;
		case FieldDesc::DataType::UnsignedInt:
			switch (f.dataSize()) {
			case 1:
				writeArray<unsigned char>(os, line[f.offset()], f.elementCount());
				break;
			case 2:
				writeArray<unsigned short>(os, line[f.offset()], f.elementCount());
				break;
			case 4:
				writeArray<unsigned int>(os, line[f.offset()], f.elementCount());
				break;
			case 8:
				writeArray<unsigned long>(os, line[f.offset()], f.elementCount());
				break;
			}
			break;
		default:
			throw std::runtime_error("This data type is not supported");
		}
	}
}

cdownload::ASCIIWriter::ASCIIWriter(bool writeEpochColumn)
	: Writer{writeEpochColumn}
{
}

cdownload::ASCIIWriter::~ASCIIWriter() = default;

void cdownload::ASCIIWriter::open(const path& fileName)
{
	output_.reset(new std::fstream(fileName.c_str(), std::ios_base::ate | std::ios_base::in | std::ios_base::out));
	output_->precision(std::numeric_limits<double>::digits10);
	fileName_ = fileName;
}

void cdownload::ASCIIWriter::truncate()
{
	output_.reset(new std::fstream(fileName_.c_str(), std::ios_base::trunc | std::ios_base::in | std::ios_base::out));
	output_->precision(std::numeric_limits<double>::digits10);
}

bool cdownload::ASCIIWriter::canAppend(std::size_t& lastWrittenCellNumber)
{
	lastWrittenCellNumber = 0;
	BOOST_LOG_TRIVIAL(trace) << "Seeking backwards in " << fileName_ << " looking for EOL...";
	// if results file exists
//  auto curPos = output_->tellg();
	output_->seekg(-4, std::ios::end); // the file ends with '\n' most likely, but no line can be shorter than 4 chars
	if (output_->fail()) {
		BOOST_LOG_TRIVIAL(error) << "Output is in failed state, reading is impossible";
	}
	char ch = 0;
	for (; output_->get(ch); output_->seekg(-2, std::ios::cur)) {
		if (ch == '\n') {
			(*output_) >> lastWrittenCellNumber;
			break;
		}
	}
	if (output_->fail()) {
		BOOST_LOG_TRIVIAL(error) << "Output came to failed state";
	}
	if (output_->eof()) {
		BOOST_LOG_TRIVIAL(error) << "Output is in EOF state";
	}
	output_->seekg(0, std::ios::end);
	output_->seekp(0, std::ios::end);
	return lastWrittenCellNumber != 0; // because there might be only header in the file
}

std::ostream& cdownload::ASCIIWriter::outputStream()
{
	return *output_.get();
}

cdownload::AveragedDataASCIIWriter::AveragedDataASCIIWriter(const AveragedTypes::Fields& averagedFields,
                                                            const RawTypes::Fields& rawFields, bool writeEpochColumn)
	: Writer(writeEpochColumn)
	, AveragedDataWriter(averagedFields, rawFields, writeEpochColumn)
	, ASCIIWriter(writeEpochColumn)
{
}

void cdownload::AveragedDataASCIIWriter::write(std::size_t cellNumber, const datetime& dt,
                                               const AveragedTypes::Data& averagedCells,
                                               const RawTypes::Data& rawCells)
{
	assert(averagedCells.size() == averagedFields().size());
	assert(rawCells.size() == rawFields().size());

	outputStream() << cellNumber << '\t' << dt;
	if (writeEpochColumn()) {
		outputStream() << '\t' << dt.milliseconds();
	}

	for (std::size_t i = 0; i < averagedFields().size(); ++i) {
		const auto& fieldsArray = averagedFields()[i];
		const auto& cells = averagedCells[i];
		for (const Field& f: fieldsArray) {
			const AveragedVariable& av = f.data(cells);
			for (const AveragingRegister& ac: av) {
				outputStream() << '\t' << ac.mean() << '\t' << ac.count() << '\t' << ac.stdDev();
			}
		}
	}

	for (std::size_t i = 0; i < rawFields().size(); ++i) {
		const auto& fieldsArray = rawFields()[i];
		const auto& cells = rawCells[i];
		for (const Field& f: fieldsArray) {
			writeRawField(f, cells, outputStream());
		}
	}
	outputStream() << std::endl;
}

namespace {
	void printAveragedFieldHeader(std::ostream& os, const std::string& name, std::size_t elementsCount)
	{
		if (elementsCount == 1) {
			os << '\t' << name << ":mean"
			   << '\t' << name << ":count"
			   << '\t' << name << ":stddev";
		} else {
			for (std::size_t elem = 0; elem < elementsCount; ++elem) {
				os << '\t' << name << "___" << elem + 1 << ":mean"
				   << '\t' << name << "___" << elem + 1 << ":count"
				   << '\t' << name << "___" << elem + 1 << ":stddev";
			}
		}
	}

	void printDirectFieldHeader(std::ostream& os, const std::string& name, std::size_t elementsCount)
	{
		if (elementsCount == 1) {
			os << '\t' << name;
		} else {
			for (std::size_t elem = 0; elem < elementsCount; ++elem) {
				os << '\t' << name << "___" << elem + 1;
			}
		}
	}
}

void cdownload::AveragedDataASCIIWriter::writeHeader()
{
	// writing a header line
	outputStream() << "CellNo\tMidTime";
	if (writeEpochColumn()) {
		outputStream() << "\tEpoch";
	}
	for (const auto& fields: averagedFields()) {
		for (const FieldDesc& f: fields) {
			printAveragedFieldHeader(outputStream(), f.name().name(), f.elementCount());
		}
	}

	for (const auto& fields: rawFields()) {
		for (const FieldDesc& f: fields) {
			printDirectFieldHeader(outputStream(), f.name().name(), f.elementCount());
		}
	}
	outputStream() << std::endl << std::flush;
}

cdownload::DirectASCIIWriter::DirectASCIIWriter(const Types::Fields& fields, bool writeEpochColumn)
	: Writer(writeEpochColumn)
	, DirectDataWriter(fields, writeEpochColumn)
	, ASCIIWriter(writeEpochColumn)
{
}


void cdownload::DirectASCIIWriter::writeHeader()
{
	// writing a header line
	outputStream() << "CellNo\tMidTime";
	if (writeEpochColumn()) {
		outputStream() << "\tEpoch";
	}

	for (const auto& fieldsArray: fields()) {
		for (const FieldDesc& f: fieldsArray) {
			printDirectFieldHeader(outputStream(), f.name().name(), f.elementCount());
		}
	}
	outputStream() << std::endl << std::flush;
}

void cdownload::DirectASCIIWriter::write(std::size_t cellNumber, const datetime& dt,
                                         const Types::Data& lines)
{
	assert(lines.size() == fields().size());

	outputStream() << cellNumber << '\t' << dt;
	if (writeEpochColumn()) {
		outputStream() << '\t' << dt.milliseconds();
	}

	for (std::size_t i = 0; i < fields().size(); ++i) {
		const auto& fieldsArray = fields()[i];
		const auto& line = lines[i];
		for (const Field& f: fieldsArray) {
			writeRawField(f, line, outputStream());
		}
	}
	outputStream() << std::endl;
}
