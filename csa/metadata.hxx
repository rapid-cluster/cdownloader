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

#ifndef CDOWNLOAD_CSA_METADATA_HXX
#define CDOWNLOAD_CSA_METADATA_HXX


#include "../metadata.hxx"

#include <map>

namespace cdownload {
namespace csa {

	/**
	 * @brief Utility class for parsing JSON documents, returned by metadata CSA requests
	 *
	 */
	class Metadata: public cdownload::Metadata {
	public:
		Metadata();

		DataSetMetadata dataset(const DatasetName& datasetName) const override;

	private:
		DataSetMetadata downloadDatasetMetadata(const DatasetName& datasetName) const;
		static std::vector<std::string> requiredFields();
		std::vector<DatasetName> datasetNames_;
		mutable std::map<DatasetName, DataSetMetadata> datasets_;
	};
}
}
#endif // CDOWNLOAD_CSA_METADATA_HXX
