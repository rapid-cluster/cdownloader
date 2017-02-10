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

#include <boost/log/trivial.hpp>

cdownload::ASCIIWriter::ASCIIWriter() = default;

cdownload::ASCIIWriter::~ASCIIWriter() = default;

void cdownload::ASCIIWriter::open(const path& fileName)
{
	output_.reset(new std::fstream(fileName.c_str(), std::ios_base::ate | std::ios_base::in | std::ios_base::out));
	fileName_ = fileName;
}

void cdownload::ASCIIWriter::truncate()
{
	output_.reset(new std::fstream(fileName_.c_str(), std::ios_base::trunc | std::ios_base::in | std::ios_base::out));
}

bool cdownload::ASCIIWriter::canAppend(std::size_t& lastWrittenCellNumber)
{
	lastWrittenCellNumber = 0;
	BOOST_LOG_TRIVIAL(trace) << "Seeking backwards in " << fileName_ << " looking for EOL...";
	// if results file exists
// 	auto curPos = output_->tellg();
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

void cdownload::AveragedDataASCIIWriter::write(std::size_t cellNumber, const datetime& dt,
                                   const std::vector<AveragedVariable>& cells)
{
	outputStream() << cellNumber << '\t' << dt;
	for (const Field& f: fields()) {
		const AveragedVariable& av = f.data(cells);
		for (const AveragingRegister& ac: av) {
			outputStream() << '\t' << ac.mean() << '\t' << ac.count() << '\t' << ac.stdDev();
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
				os << '\t' << name << "___" << elem +1 << ":mean"
					<< '\t' << name << "___" << elem +1 << ":count"
					<< '\t' << name << "___" << elem +1 << ":stddev";
			}
		}
	}

	void printDirectFieldHeader(std::ostream& os, const std::string& name, std::size_t elementsCount)
	{
		if (elementsCount == 1) {
			os << '\t' << name;
		} else {
			for (std::size_t elem = 0; elem < elementsCount; ++elem) {
				os << '\t' << name << "___" << elem +1;
			}
		}
	}
}

void cdownload::AveragedDataASCIIWriter::writeHeader()
{
	// writing a header line
	outputStream() << "CellNo\tMidTime";
	for (const FieldDesc& f: fields()) {
		printAveragedFieldHeader(outputStream(), f.name().name(), f.elementCount());
	}
	outputStream() << std::endl << std::flush;
}

void cdownload::ASCIIWriter::initialize(const std::vector<Field>& fields)
{
	base::initialize(fields);
}

void cdownload::DirectASCIIWriter::writeHeader()
{
	// writing a header line
	outputStream() << "CellNo\tMidTimeEpoch";
	for (const FieldDesc& f: fields()) {
		printDirectFieldHeader(outputStream(), f.name().name(), f.elementCount());
	}
	outputStream() << std::endl << std::flush;
}

namespace {
	template <class Data>
	void printArray(std::ostream& os, const void* ptr, std::size_t elementsCount) {
		const Data* array = static_cast<const Data*>(ptr);
		for (std::size_t i = 0; i<elementsCount; ++i) {
			os << '\t' << array[i];
		}
	}
}

void cdownload::DirectASCIIWriter::write(std::size_t cellNumber, const datetime& dt, const std::vector<const void *>& line)
{
	outputStream() << cellNumber << '\t' << dt.milliseconds();
	for (const Field& f: fields()) {
		switch (f.dataType()) {
			case FieldDesc::DataType::Real:
				switch (f.dataSize()) {
					case 4:
						printArray<float>(outputStream(), line[f.offset()], f.elementCount());
						break;
					case 8:
						printArray<double>(outputStream(), line[f.offset()], f.elementCount());
						break;
				}
				break;
			case FieldDesc::DataType::SignedInt:
				switch (f.dataSize()) {
					case 1:
						printArray<char>(outputStream(), line[f.offset()], f.elementCount());
						break;
					case 2:
						printArray<short>(outputStream(), line[f.offset()], f.elementCount());
						break;
					case 4:
						printArray<int>(outputStream(), line[f.offset()], f.elementCount());
						break;
					case 8:
						printArray<long>(outputStream(), line[f.offset()], f.elementCount());
						break;
				}
				break;
			case FieldDesc::DataType::UnsignedInt:
				switch (f.dataSize()) {
					case 1:
						printArray<unsigned char>(outputStream(), line[f.offset()], f.elementCount());
						break;
					case 2:
						printArray<unsigned short>(outputStream(), line[f.offset()], f.elementCount());
						break;
					case 4:
						printArray<unsigned int>(outputStream(), line[f.offset()], f.elementCount());
						break;
					case 8:
						printArray<unsigned long>(outputStream(), line[f.offset()], f.elementCount());
						break;
				}
				break;
			default:
				throw std::runtime_error("This data type is not supported");
		}
	}
	outputStream() << std::endl;
}

