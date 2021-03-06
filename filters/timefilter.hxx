/*
 * cdownload lib: downloads, unpacks, and reads data from the Cluster CSA arhive
 * Copyright (C) 2016-2017 Eugene Shalygin <eugene.shalygin@gmail.com>
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

#ifndef CDOWNLOAD_FILTER_TIMEFILTER_HXX
#define CDOWNLOAD_FILTER_TIMEFILTER_HXX

#include <vector>
#include "../epochrange.hxx"
#include "../commonDefinitions.hxx"

namespace cdownload {
namespace Filters {

	class TimeFilter {
	public:
		TimeFilter(const path& fileName);
		bool test(EpochRange::EpochType epoch) const;
	private:
		static std::vector<EpochRange> loadRanges(const path& fileName);
		std::vector<EpochRange> ranges_;
	};
}
}

#endif
