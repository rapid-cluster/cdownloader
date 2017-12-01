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

#ifndef CDOWNLOAD_METADATA_HXX
#define CDOWNLOAD_METADATA_HXX

#include "commonDefinitions.hxx"

#include <iosfwd>
#include <stdexcept>
#include <vector>

namespace cdownload {

class DataSetMetadataNotFound: public std::runtime_error {
public:
	DataSetMetadataNotFound(const DatasetName& name);
};

class DataSetMetadata {
public:
	DataSetMetadata() = default;
	DataSetMetadata(const DatasetName& name, const string& title,
					const datetime& minDate, const datetime& maxDate,
					const std::vector<string>& fields);

	const DatasetName& name() const {
		return name_;
	}

	const string& title() const {
		return title_;
	}

	const datetime& minTime() const {
		return minDate_;
	}
	const datetime& maxTime() const {
		return maxDate_;
	}

	const std::vector<string>& fields() const {
		return fields_;
	}
private:
	DatasetName name_;
	string title_;
	datetime minDate_;
	datetime maxDate_;
	std::vector<string> fields_;
};

std::ostream& operator<<(std::ostream&, const DataSetMetadata&);

class Metadata {
	public:
		virtual ~Metadata();
		virtual DataSetMetadata dataset(const DatasetName& datasetName) const = 0;
};

}

#endif // CDOWNLOAD_METADATA_HXX
