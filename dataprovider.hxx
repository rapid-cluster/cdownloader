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

#ifndef CDOWNLOAD_DATAPROVIDER_HXX
#define CDOWNLOAD_DATAPROVIDER_HXX

#include "datasource.hxx"
#include "metadata.hxx"

#include <map>
#include <memory>

namespace cdownload {

	class Parameters;

	class DataProvider {
	public:
		virtual std::unique_ptr<Metadata> metadata() const = 0;
		virtual std::unique_ptr<DataSource> datasource(const DatasetName& dataset, const Parameters& parameters) const = 0;

		virtual ~DataProvider();
	};

	class DataProviderRegistry {
	public:
		static DataProviderRegistry& instance();

		const DataProvider& provider(const std::string& name) const;

		void registerProvider(const std::string& name, std::unique_ptr<DataProvider>&& provider);
		std::unique_ptr<DataProvider> unregisterProvider(const std::string& name);

	private:
		std::map<std::string, std::unique_ptr<DataProvider>> providers_;
	};
}

#endif // CDOWNLOAD_DATAPROVIDER_HXX
