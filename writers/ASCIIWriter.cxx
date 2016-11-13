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


void cdownload::ASCIIWriter::write(std::size_t cellNumber, const datetime& dt,
                                   const std::vector<AveragedVariable>& cells)
{
	using namespace cdownload::csa_time_formatting;

	(*output_) << cellNumber << '\t' << dt;
	for (std::size_t i = 0; i< numOfCellsToWrite(); ++i) {
		const AveragedVariable& av = cells[i];
		for (const AveragingRegister& ac: av) {
			(*output_) << '\t' << ac.mean() << '\t' << ac.count() << '\t' << ac.stdDev();
		}
	}
	(*output_) << std::endl;
}


namespace {

	void printFieldHeader(std::ostream& os, const std::string& name, std::size_t elementsCount)
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
}

void cdownload::ASCIIWriter::writeHeader()
{
	// writing a header line
	(*output_) << "CellNo\tMidTime";
	for (const FieldDesc& f: fields()) {
		printFieldHeader((*output_), f.name().name(), f.elementCount());
	}
	(*output_) << std::endl << std::flush;
}

void cdownload::ASCIIWriter::initialize(const std::vector<Field>& fields)
{
	base::initialize(fields);
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
