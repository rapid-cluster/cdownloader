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

#include "./metadata.hxx"

#include "util.hxx"

#include <iostream>

cdownload::DataSetMetadataNotFound::DataSetMetadataNotFound(const cdownload::DatasetName& name)
	: std::runtime_error("Metadata for data set '" + name + "' could not be found.")
{
}

cdownload::DataSetMetadata::DataSetMetadata(const DatasetName& name, const string& title, const datetime& minDate, const datetime& maxDate, const std::vector<string>& fields)
	: name_{name}
	, title_{title}
	, minDate_{minDate}
	, maxDate_{maxDate}
	, fields_{fields}
{
}

std::ostream& cdownload::operator<<(std::ostream& os, const DataSetMetadata& ds)
{
	os << "Name: " << ds.name() << std::endl
		<< "Title: " << ds.title() << std::endl
		<< "Time range: [" << ds.minTime() << ", " << ds.maxTime() << ']' << std::endl
		<< "Fields: " << put_list(ds.fields()) << std::endl;
	return os;
}

cdownload::Metadata::~Metadata() = default;
