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

#include "downloader.hxx"

#include "omnidb.hxx"

#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/log/trivial.hpp>

#include <regex>
#include <sstream>

std::pair<unsigned short, unsigned short> cdownload::omni::Downloader::availableYearRange()
{
	std::stringstream dirList;
	base::beginDownloading(OmniTableDesc::HRODirectoryUrl, dirList);
	base::waitForFinished();

	unsigned short minYear = 9999, maxYear = 0;

	BOOST_LOG_TRIVIAL(trace) << "OMNI: got directory listing" << dirList.str();

	// filename example
	// omni_min2017.asc

	std::regex fileNameRx (R"(omni_min(\d\d\d\d)\.asc)");

	std::vector<std::string> parts;
	std::string line;
	while(dirList) {
		std::getline(dirList, line);
		boost::algorithm::split(parts, line, boost::is_any_of(" \t"), boost::token_compress_on);
		std::smatch match;
		if (std::regex_match(parts.back(), match, fileNameRx)) {
			unsigned short year = boost::lexical_cast<unsigned short>(match[1]);
			if (year < minYear) {
				minYear = year;
			}
			if (year > maxYear) {
				maxYear = year;
			}
		}
	}

	return {minYear, maxYear};
}

void cdownload::omni::Downloader::beginDownloading(unsigned short year, std::ostream& output)
{
	const std::string requestUrl = OmniTableDesc::urlForHRODbFile(year);
	base::beginDownloading(requestUrl, output);
}
