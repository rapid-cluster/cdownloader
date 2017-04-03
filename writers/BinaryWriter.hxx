
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

#ifndef CDOWNLOAD_BINARYWRITER_HXX
#define CDOWNLOAD_BINARYWRITER_HXX

#include "../writer.hxx"

namespace cdownload {


	/**
	 * @brief Writes output files as binary tables + ASCII header in a separate file
	 *
	 */
	class BinaryWriter: public virtual Writer {
		using base = Writer;

	public:
		~BinaryWriter();

		void open(const path& fileName) override;
		bool canAppend(std::size_t& lastWrittenCellNumber) override;
		void truncate() override;

	protected:
		BinaryWriter(bool writeEpochColumn, std::size_t stride);

		FILE* outputStream()
		{
			return output_.get();
		}

		const path& outputFileName() const {
			return outputFileName_;
		}

	private:
		struct FileClose {
			void operator()(::FILE*) const;
		};
		std::unique_ptr<::FILE, FileClose> output_;
		path outputFileName_;
		std::size_t stride_;
	};

	class DirectBinaryWriter: public DirectDataWriter, public BinaryWriter {
	public:
		DirectBinaryWriter(const Types::Fields& fields, bool writeEpochColumn);
	private:
		void writeHeader() override;
		void write(std::size_t cellNumber, const datetime& dt, const Types::Data& lines) override;
	};

	class AveragedDataBinaryWriter: public AveragedDataWriter, public BinaryWriter {
	public:
		AveragedDataBinaryWriter(const AveragedTypes::Fields& averagedFields,
		                   const RawTypes::Fields& rawFields,
		                   bool writeEpochColumn);

	private:
		void writeHeader() override;
		void write(std::size_t cellNumber, const datetime& dt,
		                   const AveragedTypes::Data& averagedCells,
		                   const RawTypes::Data& rawCells) override;
	};
}
#endif
