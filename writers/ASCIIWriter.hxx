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
	public:
		ASCIIWriter();
		~ASCIIWriter();

		void initialize(const std::vector<Field>& fields, bool writeEpochColumn) override;
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
	private:
		void writeHeader() override;
		void write(std::size_t cellNumber, const datetime& dt, const std::vector<const void*>& line) override;
	};

	class AveragedDataASCIIWriter: public AveragedDataWriter, public ASCIIWriter {

	private:
		void writeHeader() override;
		void write(std::size_t cellNumber, const datetime& dt,
		           const std::vector<AveragedVariable>& cells) override;
	};


}

#endif // CDOWNLOAD_ASCIIWRITER_HXX
