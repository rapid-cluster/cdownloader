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

#ifndef CDOWNLOAD_ASCIIWRITER_HXX
#define CDOWNLOAD_ASCIIWRITER_HXX

#include "../writer.hxx"
#include <iosfwd>
#include <memory>

namespace cdownload {


	/**
	 * @brief Writes output files as ASCII tables
	 *
	 */
	class ASCIIWriter: public virtual Writer {
		using base = Writer;
	protected:
		ASCIIWriter(bool writeEpochColumn);
	public:

		~ASCIIWriter();

		void open(const path & fileName) override;
		bool canAppend(std::size_t& lastWrittenCellNumber) override;
		void truncate() override;

	protected:
		std::ostream& outputStream();

	private:
		std::unique_ptr<std::fstream> output_;
		path fileName_;
	};

	class DirectASCIIWriter: public DirectDataWriter, public ASCIIWriter {
	public:
		DirectASCIIWriter(const Types::Fields& fields, bool writeEpochColumn);
	private:
		void writeHeader() override;
		void write(std::size_t cellNumber, const datetime& dt,
		                   const Types::Data& lines) override;
	};

	class AveragedDataASCIIWriter: public AveragedDataWriter, public ASCIIWriter {
	public:
		AveragedDataASCIIWriter(const AveragedTypes::Fields& averagedFields,
		                   const RawTypes::Fields& rawFields,
		                   bool writeEpochColumn);
	private:
		void writeHeader() override;
		void write(std::size_t cellNumber, const datetime& dt,
		                   const AveragedTypes::Data& averagedCells,
		                   const RawTypes::Data& rawCells) override;
	};


}

#endif // CDOWNLOAD_ASCIIWRITER_HXX
