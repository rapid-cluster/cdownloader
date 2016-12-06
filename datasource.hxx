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

namespace cdownload {

	class Metadata;
	class Parameters;
	class ChunkDownloader;
	class DataDownloader;

	struct DatasetChunk {
		datetime startTime;
		datetime endTime;
		path file;

		bool empty() const;
	};

	class DataSource {
	public:
		DataSource(const DatasetName& dataset, const Parameters& parameters, const Metadata& meta);
		~DataSource();
#if 0
		DataSource(const DataSource& other);
		DataSource(DataSource&&) = default;
		DataSource& operator=(const DataSource& other);
#endif

		datetime minAvailableTime() const;
		datetime maxAvailableTime() const;

		DatasetChunk nextChunk();
		void reset();
		bool eof() const;
		void setNextChunkStartTime(const datetime& startTime);

	private:
		void loadCachedFiles(const DatasetName& dataset, const path& dir);
		DatasetChunk downloadNextChunk();

		DatasetName dsName_;
		path cacheDir_;

		std::unique_ptr<DataDownloader> dataDownloader_;
		std::unique_ptr<ChunkDownloader> downloader_;
		std::vector<DatasetChunk> cachedFiles_;
		datetime minAvailableTime_;
		datetime maxAvailableTime_;


		datetime currentChunkStartTime_;
		datetime lastServedChunkEndTime_;

		DatasetChunk lastServedChunk_;
		bool eof_;
	};
}


#endif // CDOWNLOAD_DATASOURCE_HXX
