/*
 * cdownload lib: downloads, unpacks, and reads data from the Cluster CSA arhive
 * Copyright (C) 2017  Eugene Shalygin <eugene.shalygin@gmail.com>
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

#include "dataprovider.hxx"

#include "datasource.hxx"
#include "metadata.hxx"

std::unique_ptr<cdownload::DataSource> cdownload::omni::DataProvider::datasource(const cdownload::DatasetName& /*dataset*/,
																				 const cdownload::Parameters& parameters) const
{
	return std::unique_ptr<cdownload::DataSource>(new omni::DataSource(parameters));
}

std::unique_ptr<cdownload::Metadata> cdownload::omni::DataProvider::metadata() const
{
	return std::unique_ptr<cdownload::Metadata>(new omni::Metadata());
}

