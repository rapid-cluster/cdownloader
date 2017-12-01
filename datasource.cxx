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

#include <boost/log/trivial.hpp>

bool cdownload::DatasetChunk::empty() const
{
	return file.empty();
}

cdownload::DataSource::DataSource(const std::string& name, const timeduration& timeGranularity)
	: eof_{false}
	, timeGranularity_{timeGranularity}
	, name_{name}
{
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

bool cdownload::DataSource::eof() const
{
	return eof_;
}

void cdownload::DataSource::setEof(bool eof)
{
	eof_ = eof;
}

void cdownload::DataSource::setAvailableTimeRange(const cdownload::datetime& min, const cdownload::datetime& max)
{
	minAvailableTime_ = min;
	maxAvailableTime_ = max;

	currentChunkStartTime_ = minAvailableTime_;
	lastServedChunkEndTime_ = currentChunkStartTime_ - timeGranularity();

	BOOST_LOG_TRIVIAL(trace) << "Datasource for '" << name_ << "' detected available time range:"
		<< '[' << this->minAvailableTime() << ',' << this->maxAvailableTime()<<']';
}

void cdownload::DataSource::setNextChunkStartTime(const cdownload::datetime& startTime)
{
	if (startTime >= minAvailableTime() && startTime <= maxAvailableTime()) {
		currentChunkStartTime_ = startTime;
		lastServedChunkEndTime_ = currentChunkStartTime_ - timeGranularity();
		setEof(false);
	} else {
		throw std::logic_error("Requested chunk start time is not within available time range");
	}
}

cdownload::DatasetChunk cdownload::DataSource::nextChunk()
{
	BOOST_LOG_TRIVIAL(trace) << "Datasource for '" << name_ << "' received request for next chunk:"
		<< " last served chunk end time is " << lastServedChunkEndTime_;
	BOOST_LOG_TRIVIAL(trace) << "Datasource for '" << name_ << "': max available time is "
		<<  maxAvailableTime();

	if (lastServedChunkEndTime_ >= maxAvailableTime()) {
		setEof();
		return {};
	}
	// step 1: find next cached chunk past lastServedChunkEndTime_
	// TODO: implement handling of intersecting cached chunks
	auto nexCachedChunkIter = std::find_if(cachedFiles_.begin(), cachedFiles_.end(),
		[this](const DatasetChunk& c){
			return c.endTime > this->lastServedChunkEndTime_ + timeGranularity_;
		});

	// step 2: cache is exhausted, we have to download data if we can
	DatasetChunk newChunk;
	if (nexCachedChunkIter == cachedFiles_.end()) {
		newChunk = getNewChunk(lastServedChunkEndTime_ + timeGranularity(), maxAvailableTime());
		if (newChunk.empty()) {
			setEof();
			return {};
		} else {
			return newChunk;
		}
#if 0
		if (!downloader_) {
			setEof();
			return {};
		} else {
			downloader_->setTimeRange(lastServedChunkEndTime_ + timeGranularity(), maxAvailableTime());
			return downloadNextChunk();
		}
#endif
	} else {
		if (lastServedChunkEndTime_ + timeGranularity() < nexCachedChunkIter->startTime) {
			// there is a gap in cache, data have to be downloaded
			newChunk = getNewChunk(lastServedChunkEndTime_ + timeGranularity(),
												nexCachedChunkIter->startTime - timeGranularity());

			if (newChunk.empty()) {
				throw std::logic_error("Data gap was found when downloading is not permitted");
			}
			return newChunk;
		} else {
			lastServedChunkEndTime_ = nexCachedChunkIter->endTime;
			return *nexCachedChunkIter;
		}
	}

	lastServedChunkEndTime_ = newChunk.endTime;
	return newChunk;
}

void cdownload::DataSource::setCache(std::vector<DatasetChunk>&& cache)
{
	cachedFiles_ = cache;
}
