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

#ifndef CDOWNLOAD_OMNIDB_HXX
#define CDOWNLOAD_OMNIDB_HXX

#include "../field.hxx"

namespace cdownload {
namespace omni {
	class OmniTableDesc {
	public:
		OmniTableDesc();

		static string fileNameForHRODbFile(unsigned year, bool perMinuteFile = true);
		static string urlForHRODbFile(unsigned year, bool perMinuteFile = true);

		static std::vector<FieldDesc> highResOmniFields();
		const FieldDesc& fieldDesc(const std::string& name) const;

		std::size_t fieldIndex(const ProductName& name) const;
		const FieldDesc& operator[](std::size_t index) const;

		static ProductName makeOmniHROName(const string& fieldName);

		static const std::string HRODirectoryUrl;

		static const DatasetName HRODatasetName;
	private:
		std::vector<FieldDesc> fields_;
	};
}
}


#endif // CDOWNLOAD_OMNIDB_HXX
