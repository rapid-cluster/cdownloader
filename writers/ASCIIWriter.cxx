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

cdownload::ASCIIWriter::ASCIIWriter(const path& fileName)
	: output_(new std::ofstream(fileName.c_str()))
{
}

cdownload::ASCIIWriter::~ASCIIWriter() = default;

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

void cdownload::ASCIIWriter::initialize(const std::vector<Field>& fields)
{
	// writing a header line
	(*output_) << "CellNo\tMidTime";
	for (const FieldDesc& f: fields) {
		printFieldHeader((*output_), f.name().name(), f.elementCount());
	}
	(*output_) << std::endl << std::flush;
	base::initialize(fields);
}
