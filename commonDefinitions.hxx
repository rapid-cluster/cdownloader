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

#ifndef CDOWNLOAD_COMMON_DEFINITIONS_HXX
#define CDOWNLOAD_COMMON_DEFINITIONS_HXX

#include <string>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/filesystem/path.hpp>

namespace cdownload {

	using string = std::string;

	using DatasetName = std::string;

	using datetime = boost::posix_time::ptime;
	using date = boost::gregorian::date;
	using timeduration = boost::posix_time::time_duration;

	using path = boost::filesystem::path;
}


#endif //CDOWNLOAD_COMMON_DEFINITIONS_HXX
