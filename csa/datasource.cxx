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

#include "./datasource.hxx"

#include "./metadata.hxx"
#include "downloader.hxx"
#include "./chunkdownloader.hxx"
#include "../parameters.hxx"
#include "./unpacker.hxx"

#include <boost/filesystem/operations.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/log/trivial.hpp>

#include <algorithm>
#include <regex>

cdownload::csa::DataSource::DataSource(const DatasetName& dataset, const Parameters& parameters,
		                  const cdownload::csa::Metadata& meta)
	: base{"CSA:"+dataset, cdownload::timeduration(0,0,1)} // 1 second
	, dsName_{dataset}
	, cacheDir_{parameters.cacheDir()}
	, downloader_{}
{
	std::vector<DatasetChunk> cachedFiles =
		cacheDir_.empty() ? std::vector<DatasetChunk>() : loadCachedFiles(dsName_, cacheDir_);

	datetime minAvailableTime, maxAvailableTime;

	if (parameters.downloadMissingData()) {
		auto dsMeta = meta.dataset(dataset);
		minAvailableTime = dsMeta.minTime();
		maxAvailableTime = dsMeta.maxTime();
		dataDownloader_.reset(new DataDownloader());
		downloader_.reset(new ChunkDownloader(parameters.workDir(), cacheDir_, *dataDownloader_,
		                                      {dsName_}, minAvailableTime, maxAvailableTime));
	} else {
		// we look for the longest continious range in the cache
		if (!cachedFiles.size()) {
			minAvailableTime = maxAvailableTime = datetime();
		} else {
			minAvailableTime = cachedFiles[0].startTime;
			maxAvailableTime = cachedFiles[0].endTime;
			timeduration maxDuration = maxAvailableTime - maxAvailableTime;
			datetime chunkStart = minAvailableTime;
			datetime chunkEnd = maxAvailableTime;
			for (std::size_t i = 1; i < cachedFiles.size(); ++i) {
				const bool isCurrentChunkAdjacent =
					cachedFiles[i].startTime - chunkEnd <= timeGranularity();
				chunkEnd = cachedFiles[i].endTime;
				if (!isCurrentChunkAdjacent) {
					chunkStart = cachedFiles[i].startTime;
				}
				if (chunkEnd - chunkStart > maxDuration) {
					minAvailableTime = chunkStart;
					maxAvailableTime = chunkEnd;
					maxDuration = chunkEnd - chunkStart;
				}
			}
		}
	}

	setAvailableTimeRange(minAvailableTime, maxAvailableTime);
	setCache(std::move(cachedFiles));
}

cdownload::csa::DataSource::~DataSource() = default;


cdownload::DatasetChunk cdownload::csa::DataSource::getNewChunk(const cdownload::datetime& min, const cdownload::datetime& /*max*/)
{
	if (!downloader_) {
		return {};
	}
	downloader_->setNextChunkStartTime(min);
	return downloadNextChunk();
}

#if 0
cdownload::DatasetChunk cdownload::csa::DataSource::nextChunk()
{
	BOOST_LOG_TRIVIAL(trace) << "Datasource for '" << dsName_ << "' received request for next chunk:"
		<< " last served chunk end time is " << lastServedChunkEndTime_;
	BOOST_LOG_TRIVIAL(trace) << "Datasource for '" << dsName_ << "': max available time is "
		<<  maxAvailableTime();

	if (lastServedChunkEndTime_ >= maxAvailableTime()) {
		setEof();
		return {};
	}
	// step 1: find next cached chunk past lastServedChunkEndTime_
	// TODO: implement cases when cached chunks intersect
	auto nexCachedChunkIter = std::find_if(cachedFiles_.begin(), cachedFiles_.end(),
		[this](const DatasetChunk& c){
			return c.endTime > this->lastServedChunkEndTime_ + CSA_GRANULARITY;
		});

	// step 2: cache is exhausted, we have to download data if we can
	if (nexCachedChunkIter == cachedFiles_.end()) {
		if (!downloader_) {
			setEof();
			return {};
		} else {
			downloader_->setTimeRange(lastServedChunkEndTime_ + CSA_GRANULARITY, maxAvailableTime());
			return downloadNextChunk();
		}
	} else {
		if (lastServedChunkEndTime_ + CSA_GRANULARITY < nexCachedChunkIter->startTime) {
			// there is a gap in cache, data have to be downloaded
			if (!downloader_) {
				throw std::logic_error("Data gap that was found when downloading is not permitted");
			} else {
				downloader_->setTimeRange(lastServedChunkEndTime_ + CSA_GRANULARITY,
				                          nexCachedChunkIter->startTime - CSA_GRANULARITY);
				return downloadNextChunk();
			}
		} else {
			lastServedChunkEndTime_ = nexCachedChunkIter->endTime;
			return *nexCachedChunkIter;
		}
	}
}
#endif

cdownload::DatasetChunk cdownload::csa::DataSource::downloadNextChunk()
{
	auto downloadedChunk = downloader_->nextChunk();
	if (downloadedChunk.empty() && downloader_->eof()) {
		setEof();
		return {};
	}
// 	lastServedChunkEndTime_ = downloadedChunk.endTime;
	downloadedChunk.files[dsName_].release();
	return {downloadedChunk.startTime, downloadedChunk.endTime, downloadedChunk.files[dsName_]};
}

#if 0
void cdownload::csa::DataSource::setNextChunkStartTime(const datetime& startTime)
{
	if (startTime >= minAvailableTime() && startTime <= maxAvailableTime()) {
		currentChunkStartTime_ = startTime;
		lastServedChunkEndTime_ = currentChunkStartTime_ - CSA_GRANULARITY;
		setEof(false);
	} else {
		throw std::logic_error("Requested chunk start time is not within available time range");
	}
}
#endif

std::vector<cdownload::DatasetChunk>
cdownload::csa::DataSource::loadCachedFiles(const DatasetName& dataset, const path& dir)
{
	namespace fs = boost::filesystem;
	std::vector<cdownload::DatasetChunk> res;

	// filename example
	// C4_CP_CIS-CODIF_HS_H1_MOMENTS__20010130_000000_20151231_235959_V161019.cdf

	std::regex cdfNameRx (dataset +
		R"(__(\d\d\d\d)(\d\d)(\d\d)_(\d\d)(\d\d)(\d\d)_(\d\d\d\d)(\d\d)(\d\d)_(\d\d)(\d\d)(\d\d)_V\d+\.cdf)");

	for (fs::directory_iterator it(dir);
		 it != fs::directory_iterator(); ++it) {
		string fn = it->path().filename().string();
		std::smatch match;
		if (std::regex_match(fn, match, cdfNameRx)) {
			unsigned int yearBegin = boost::lexical_cast<unsigned int>(match[1]);
			unsigned int monthBegin = boost::lexical_cast<unsigned int>(match[2]);
			unsigned int dayBegin = boost::lexical_cast<unsigned int>(match[3]);
			unsigned int hoursBegin = boost::lexical_cast<unsigned int>(match[4]);
			unsigned int minutesBegin = boost::lexical_cast<unsigned int>(match[5]);
			unsigned int secondsBegin = boost::lexical_cast<unsigned int>(match[6]);

			unsigned int yearEnd = boost::lexical_cast<unsigned int>(match[7]);
			unsigned int monthEnd = boost::lexical_cast<unsigned int>(match[8]);
			unsigned int dayEnd = boost::lexical_cast<unsigned int>(match[9]);
			unsigned int hoursEnd = boost::lexical_cast<unsigned int>(match[10]);
			unsigned int minutesEnd = boost::lexical_cast<unsigned int>(match[11]);
			unsigned int secondsEnd = boost::lexical_cast<unsigned int>(match[12]);

			datetime begin = makeDateTime(yearBegin, monthBegin, dayBegin, hoursBegin, minutesBegin, secondsBegin);
			datetime end = makeDateTime(yearEnd, monthEnd, dayEnd, hoursEnd, minutesEnd, secondsEnd);
			res.push_back({begin, end, it->path()});
		}
	}

	std::sort(res.begin(), res.end());
	return res;
}
