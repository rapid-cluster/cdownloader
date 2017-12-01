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

#include "datasource.hxx"

#include "../parameters.hxx"
#include "downloader.hxx"
#include "omnidb.hxx"

#include <boost/filesystem/operations.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/log/trivial.hpp>

#include <fstream>
#include <regex>

cdownload::omni::DataSource::DataSource(const cdownload::Parameters& parameters)
	: base{"OMNI", cdownload::timeduration(0,1,0)} // 1 minute
	, cacheDir_{parameters.cacheDir()}
	, downloader_{new Downloader()}
	, tempDir_{parameters.workDir()}
{
	std::vector<DatasetChunk> cacheFiles =
		parameters.cacheDir().empty() ? std::vector<DatasetChunk>() : loadCachedFiles(parameters.cacheDir());

	datetime minAvailableTime, maxAvailableTime;
	if (parameters.downloadMissingData()) {
		const auto minMaxYears = downloader_->availableYearRange();
		minAvailableTime = makeDateTime(minMaxYears.first, 1, 1, 0, 0, 0);
		maxAvailableTime = makeDateTime(minMaxYears.second, 12, 31, 23, 59,50);
	} else {
		// cache files are sorted
		minAvailableTime = cacheFiles.front().startTime;
		maxAvailableTime = cacheFiles.back().endTime;
	}

	setAvailableTimeRange(minAvailableTime, maxAvailableTime);
	setCache(std::move(cacheFiles));
}

cdownload::omni::DataSource::~DataSource() = default;

std::vector<cdownload::DatasetChunk> cdownload::omni::DataSource::loadCachedFiles(const cdownload::path& dir)
{
	namespace fs = boost::filesystem;
	std::vector<cdownload::DatasetChunk> res;

	// filename example
	// omni_min2017.asc

	std::regex fileNameRx (R"(omni_min(\d\d\d\d)\.asc)");

	for (fs::directory_iterator it(dir);
		 it != fs::directory_iterator(); ++it) {
		string fn = it->path().filename().string();
		std::smatch match;
		if (std::regex_match(fn, match, fileNameRx)) {
			unsigned int year = boost::lexical_cast<unsigned int>(match[1]);

			datetime begin = makeDateTime(year, 1, 1, 0, 0, 0);
			datetime end = makeDateTime(year, 12, 31, 23, 59, 59);
			res.push_back({begin, end, it->path()});
		}
	}

	std::sort(res.begin(), res.end());
	return res;
}

cdownload::DatasetChunk cdownload::omni::DataSource::getNewChunk(const cdownload::datetime& min, const cdownload::datetime& /*max*/)
{
	unsigned short int year = static_cast<unsigned short>(min.breakdown().year);

	path fn = cacheDir_ / OmniTableDesc::fileNameForHRODbFile(year);
	{
		std::ofstream tempFile{fn.string()};
		downloader_->beginDownloading(year, tempFile);
		downloader_->waitForFinished();
	}
	return {makeDateTime(year, 1, 1, 0, 0, 0), makeDateTime(year, 12, 31, 23, 59, 59), fn};
}
