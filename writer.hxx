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

#ifndef CDOWNLOAD_WRITER_HXX
#define CDOWNLOAD_WRITER_HXX

#include "average.hxx"
#include "commonDefinitions.hxx"
#include <vector>

namespace cdownload {

	class Field;


	/**
	 * @brief Basic interface class for writing result files
	 *
	 * Provides output format agnostic interface for writing data records to an output stream
	 */
	class Writer {
	public:

		virtual ~Writer() = default;

		/**
		 * @brief Provides this instance with a data record structure
		 *
		 * @param fields List of fields in the data record array. This list contains the same
		 * fields (and in the same order) as they need to be written into the output file.
		 */
		virtual void initialize(const std::vector<Field>& fields);

		/**
		 * @brief Writes a data record into the output stream
		 *
		 * @param cellNumber cardinal number of the cell
		 * @param dt middle time in cell
		 * @param cells record data
		 */
		virtual void write(std::size_t cellNumber, const datetime& dt,
		                   const std::vector<AveragedVariable>& cells) = 0;
	protected:
		Writer();
		std::size_t numOfCellsToWrite() const {
			return numOfCellsToWrite_;
		}
	private:
		std::size_t numOfCellsToWrite_;
	};
}

#endif // CDOWNLOAD_WRITER_HXX
