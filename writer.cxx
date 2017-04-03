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

#include "writer.hxx"

#include "field.hxx"

#include <numeric>

// intentionally intialize numOfCellsToWrite_ to large number
// that will lead to a crash if Writer::initialize() was not called
cdownload::Writer::Writer(bool writeEpochColumn)
	: writeEpochColumn_{writeEpochColumn}
{
}

#if 0
void cdownload::Writer::initialize(const std::vector<cdownload::Field>& fields, bool writeEpochColumn)
{
	fields_ = fields;
	writeEpochColumn_ = writeEpochColumn;
}

#endif

cdownload::DirectDataWriter::DirectDataWriter(const Types::Fields& fields, bool writeEpochColumn)
	: Writer{writeEpochColumn}
	, fields_{fields}
{
}

std::size_t cdownload::DirectDataWriter::numOfCellsToWrite() const
{
	return std::accumulate(fields_.begin(), fields_.end(), std::size_t(0),
			[](std::size_t a, const Types::FieldArray& fa) {
				return a + fa.size();
			});
}

cdownload::AveragedDataWriter::AveragedDataWriter(const AveragedTypes::Fields& averagedFields,
		                                          const RawTypes::Fields& rawFields, bool writeEpochColumn)
	: Writer{writeEpochColumn}
	, averagedFields_{averagedFields}
	, rawFields_{rawFields}
{
}
