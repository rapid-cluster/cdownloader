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

// intentionally intialize numOfCellsToWrite_ to large number
// that will lead to a crash if Writer::initialize() was not called
cdownload::Writer::Writer()
	: numOfCellsToWrite_{static_cast<std::size_t>(-1)}
{
}

void cdownload::Writer::initialize(const std::vector<cdownload::Field>& fields)
{
	numOfCellsToWrite_ = fields.size();
}
