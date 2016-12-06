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

#include "./datasource.hxx"

#include "metadata.hxx"
#include "downloader.hxx"
#include "chunkdownloader.hxx"
#include "parameters.hxx"
#include "unpacker.hxx"

#include <boost/filesystem/operations.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/log/trivial.hpp>

#include <algorithm>
#include <regex>

namespace {
	const cdownload::timeduration CSA_GRANULARITY = cdownload::timeduration(0,0,1); // 1 second
}

namespace cdownload {
	inline bool operator<(const cdownload::DatasetChunk& left, const cdownload::DatasetChunk& right)
	{
		return left.startTime < right.startTime;
	}
}

bool cdownload::DatasetChunk::empty() const
{
	return file.empty();
}

cdownload::DataSource::DataSource(const DatasetName& dataset, const Parameters& parameters,
		                  const cdownload::Metadata& meta)
	: dsName_{dataset}
	, cacheDir_{parameters.cacheDir()}
	, downloader_{}
	, eof_{false}
{
	if (!cacheDir_.empty()) {
		loadCachedFiles(dsName_, cacheDir_);
	}

	if (parameters.downloadMissingData()) {
		auto dsMeta = meta.dataset(dataset);
		minAvailableTime_ = dsMeta.minTime();
		maxAvailableTime_ = dsMeta.maxTime();
		dataDownloader_.reset(new DataDownloader());
		downloader_.reset(new ChunkDownloader(parameters.workDir(), cacheDir_, *dataDownloader_,
		                                      {dsName_}, minAvailableTime_, maxAvailableTime_));
	} else {
		// we look for the longest continious range in the cache
		if (!cachedFiles_.size()) {
			minAvailableTime_ = maxAvailableTime_ = datetime();
		} else {
			minAvailableTime_ = cachedFiles_[0].startTime;
			maxAvailableTime_ = cachedFiles_[0].endTime;
			timeduration maxDuration = maxAvailableTime_ - maxAvailableTime_;
			datetime chunkStart = minAvailableTime_;
			datetime chunkEnd = maxAvailableTime_;
			for (std::size_t i = 1; i < cachedFiles_.size(); ++i) {
				const bool isCurrentChunkAdjacent =
					cachedFiles_[i].startTime - chunkEnd <= CSA_GRANULARITY;
				chunkEnd = cachedFiles_[i].endTime;
				if (!isCurrentChunkAdjacent) {
					chunkStart = cachedFiles_[i].startTime;
				}
				if (chunkEnd - chunkStart > maxDuration) {
					minAvailableTime_ = chunkStart;
					maxAvailableTime_ = chunkEnd;
					maxDuration = chunkEnd - chunkStart;
				}
			}
		}
	}

	BOOST_LOG_TRIVIAL(trace) << "Datasource for '" << dataset << "' detected available time range:"
		<< '[' << minAvailableTime_ << ',' << maxAvailableTime_<<']';

	currentChunkStartTime_ = minAvailableTime_;
	lastServedChunkEndTime_ = currentChunkStartTime_ - CSA_GRANULARITY;
}

cdownload::DataSource::~DataSource() = default;

cdownload::datetime cdownload::DataSource::minAvailableTime() const
{
	return minAvailableTime_;
}

cdownload::datetime cdownload::DataSource::maxAvailableTime() const
{
	return maxAvailableTime_;
}

cdownload::DatasetChunk cdownload::DataSource::nextChunk()
{
	BOOST_LOG_TRIVIAL(trace) << "Datasource for '" << dsName_ << "' received request for next chunk:"
		<< " last served chunk end time is " << lastServedChunkEndTime_;
	BOOST_LOG_TRIVIAL(trace) << "Datasource for '" << dsName_ << "': max available time is "
		<<  maxAvailableTime();

	if (lastServedChunkEndTime_ >= maxAvailableTime()) {
		eof_ = true;
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
			eof_ = true;
			return {};
		} else {
			downloader_->setTimeRange(lastServedChunkEndTime_ + CSA_GRANULARITY, maxAvailableTime_);
			return downloadNextChunk();
		}
	} else {
		if (lastServedChunkEndTime_ + CSA_GRANULARITY < nexCachedChunkIter->startTime) {
			// there is a gap in cache, data have to be downloaded
			if (!downloader_) {
				throw std::logic_error("Data gap found when downloading is not permitted");
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

cdownload::DatasetChunk cdownload::DataSource::downloadNextChunk()
{
	auto downloadedChunk = downloader_->nextChunk();
	if (downloadedChunk.empty() && downloader_->eof()) {
		eof_ = true;
		return {};
	}
	lastServedChunkEndTime_ = downloadedChunk.endTime;
	downloadedChunk.files[dsName_].release();
	return {downloadedChunk.startTime, downloadedChunk.endTime, downloadedChunk.files[dsName_]};
}


bool cdownload::DataSource::eof() const
{
	return eof_;
}

void cdownload::DataSource::setNextChunkStartTime(const datetime& startTime)
{
	if (startTime >= minAvailableTime() && startTime <= maxAvailableTime()) {
		currentChunkStartTime_ = startTime;
		lastServedChunkEndTime_ = currentChunkStartTime_ - CSA_GRANULARITY;
		eof_ = false;
	} else {
		throw std::logic_error("Requested chunk start time is not within available time range");
	}
}

void cdownload::DataSource::loadCachedFiles(const DatasetName& dataset, const path& dir)
{
	namespace fs = boost::filesystem;
	cachedFiles_.clear();

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
			cachedFiles_.push_back({begin, end, it->path()});
		}
	}

	std::sort(cachedFiles_.begin(), cachedFiles_.end());
}
