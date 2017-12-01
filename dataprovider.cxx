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

#include <utility>

cdownload::DataProvider::~DataProvider() = default;

cdownload::DataProviderRegistry& cdownload::DataProviderRegistry::instance()
{
	static DataProviderRegistry instance;
	return instance;
}

const cdownload::DataProvider& cdownload::DataProviderRegistry::provider(const std::string& name) const
{
	return *providers_.at(name);
}

void cdownload::DataProviderRegistry::registerProvider(const std::string& name, std::unique_ptr<DataProvider>&& provider)
{
	providers_[name] = std::move(provider);
}

std::unique_ptr<cdownload::DataProvider> cdownload::DataProviderRegistry::unregisterProvider(const std::string& name)
{
	auto it = providers_.find(name);
	if (it == providers_.end()) {
		return {};
	}

	std::unique_ptr<cdownload::DataProvider> res = std::move(it->second);
	providers_.erase(it);
	return res;
}
