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

#include "metadata.hxx"

#include "downloader.hxx"
#include "omnidb.hxx"

#include <algorithm>

std::vector<cdownload::DatasetName> cdownload::omni::Metadata::datasets() const
{
	return {OmniTableDesc::HRODatasetName};
}

cdownload::DataSetMetadata cdownload::omni::Metadata::dataset(const cdownload::DatasetName& datasetName) const
{

	Downloader downloader;
	const auto yearRange = downloader.availableYearRange();
	std::vector<std::string> fields;
	const auto products = OmniTableDesc::highResOmniFields();
	std::transform(products.begin(), products.end(), std::back_inserter(fields),
				   [](const FieldDesc& f){return f.name().name();});

	return DataSetMetadata(datasetName, "",
		makeDateTime(yearRange.first, 1, 1, 0, 0, 0), makeDateTime(yearRange.second, 12, 31, 23, 59, 59),
		fields);
}
