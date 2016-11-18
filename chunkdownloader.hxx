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

#ifndef CDOWNLOAD_CHUNK_DOWNLOADER_HXX
#define CDOWNLOAD_CHUNK_DOWNLOADER_HXX

#include "commonDefinitions.hxx"
#include "unpacker.hxx"

#include <map>
#include <vector>

namespace cdownload {

	class DataDownloader;

	struct Chunk {
		datetime startTime;
		datetime endTime;
		std::map<std::string, DownloadedProductFile> files;

		bool empty() const;
	};


	/**
	 * @brief Downloads data from CSA archive server, managing data chunk size
	 *
	 * We adjust chunk size dynamically, to be close enough to the maximal file limit in the
	 * synchronous mode (1 GiB). To do so we measure size of the downloaded files, and scale
	 * time interval targeting 0.85 GiB. The server may return HTTP 503 error if we are close enough
	 * the limit, hence the 0.85 coefficient (see nextChunk() implementation).
	 *
	 */
	class ChunkDownloader {
	public:
		ChunkDownloader(const path& tmpDir, DataDownloader& downloader,
		                const std::vector<DatasetName>& datasets,
		                const datetime& startTime, const datetime& endTime);

		Chunk nextChunk();
		bool eof() const;
		void setNextChunkStartTime(const datetime& startTime);
	private:
		std::map<std::string, DownloadedProductFile>
		downloadChunk(const datetime& startTime, const timeduration& duration,
		              std::size_t& maxDownloadedFileSize);

		DataDownloader& downloader_;
		std::vector<DatasetName> datasets_;
		path tmpDir_;
		datetime start_, end_;
		datetime currentChunkStart_;
		timeduration currentChunkLength_;
// 		std::size_t lastMaxDownloadedFileSize_;
		bool eof_;
	};
}

#endif // CDOWNLOAD_CHUNK_DOWNLOADER_HXX
