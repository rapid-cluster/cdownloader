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

#ifndef CDOWNLOADER_METADATA_HXX
#define CDOWNLOADER_METADATA_HXX

#include "util.hxx"
#include <json/value.h>

#include <iosfwd>
#include <string>
#include <vector>
#include <map>

namespace cdownload {

	/**
	 * @brief Utility class for parsing JSON documents, returned by metadata CSA requests
	 *
	 */
	class Metadata {
	public:
		Metadata();

		class DataSetMetadata {
		public:
			DataSetMetadata() = default;
			DataSetMetadata(const DatasetName& name, const string& title,
			                const datetime& minDate, const datetime& maxDate,
			                const std::vector<string>& parameters);

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

			const std::vector<string>& parameters() const {
				return parameters_;
			}
		private:
			DatasetName name_;
			string title_;
			datetime minDate_;
			datetime maxDate_;
			std::vector<string> parameters_;
		};

		DataSetMetadata dataset(const DatasetName& datasetName) const;

	private:
		DataSetMetadata downloadDatasetMetadata(const DatasetName& datasetName) const;
		static std::vector<std::string> requiredFields();
		std::vector<DatasetName> datasetNames_;
		mutable std::map<DatasetName, DataSetMetadata> datasets_;
	};

	std::ostream& operator<<(std::ostream&, const Metadata::DataSetMetadata&);
}
#endif // CDOWNLOADER_METADATA_HXX
