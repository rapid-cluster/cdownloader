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

#ifndef CDOWNLOAD_DATASOURCE_HXX
#define CDOWNLOAD_DATASOURCE_HXX

#include "commonDefinitions.hxx"

#include <memory>
#include <vector>

namespace cdownload {

	struct DatasetChunk {
		datetime startTime;
		datetime endTime;
		path file;

		bool empty() const;
	};

	inline bool operator<(const DatasetChunk& left, const DatasetChunk& right)
	{
		return left.startTime < right.startTime;
	}

	class DataSource {
	public:
		virtual ~DataSource();

		datetime minAvailableTime() const;
		datetime maxAvailableTime() const;

		DatasetChunk nextChunk();
		bool eof() const;
		void setNextChunkStartTime(const datetime& startTime);

	protected:
		DataSource(const std::string& name, const timeduration& timeGranularity);

		void setAvailableTimeRange(const datetime& min, const datetime& max);
		void setEof(bool eof = true);
		void setCache(std::vector<DatasetChunk>&& cache);

		timeduration timeGranularity() const {
			return timeGranularity_;
		}

	private:
		virtual DatasetChunk getNewChunk(const datetime& min, const datetime& max) = 0;

		datetime minAvailableTime_;
		datetime maxAvailableTime_;
		bool eof_;

		datetime currentChunkStartTime_;
		datetime lastServedChunkEndTime_;

// 		DatasetChunk lastServedChunk_;

		std::vector<DatasetChunk> cachedFiles_;

		timeduration timeGranularity_;
		std::string name_;
	};
}


#endif // CDOWNLOAD_DATASOURCE_HXX
