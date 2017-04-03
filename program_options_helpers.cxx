/*
 * cdownload lib: downloads, unpacks, and reads data from the Cluster CSA arhive
 * Copyright (C) 2016-2017  Eugene Shalygin <eugene.shalygin@gmail.com>
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

#include "commonDefinitions.hxx"

#include <boost/program_options.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>

namespace boost {
namespace posix_time {
	void validate(boost::any& v,
	              const std::vector<std::string>& values,
	              ptime* /*target_type*/, int)
	{
		using namespace boost::program_options;

		// Extract the first string from 'values'. If there is more than
		// one string, it's an error, and exception will be thrown.
		const std::string& s = validators::get_single_string(values);
		std::istringstream is(s);

		cdownload::datetime dt;
		is >> dt;
		v = boost::any(dt);
	}

	void validate(boost::any& v,
	              const std::vector<std::string>& values,
	              cdownload::timeduration* /*target_type*/, int)
	{
		using namespace boost::program_options;

		// Extract the first string from 'values'. If there is more than
		// one string, it's an error, and exception will be thrown.
		const std::string& s = validators::get_single_string(values);
		std::istringstream is(s);

		cdownload::timeduration dt;
		is >> dt;
		v = boost::any(dt);
	}
}
}
