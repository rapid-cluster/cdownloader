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

#include "chunkdownloader.hxx"

#include "downloader.hxx"
#include "unpacker.hxx"
#include "util.hxx"

#include <boost/lexical_cast.hpp>
#include <boost/log/trivial.hpp>
#include <boost/filesystem/operations.hpp>

#include <fstream>

cdownload::ChunkDownloader::ChunkDownloader(const path& tmpDir,
                                            const cdownload::DataDownloader& downloader,
                                            const std::vector<DatasetName>& datasets,
                                            const datetime& startTime, const datetime& endTime)
	: downloader_{downloader}
	, datasets_{datasets}
	, tmpDir_{tmpDir}
	, start_{startTime}
	, end_{endTime}
	, currentChunkStart_{startTime}
	, currentChunkLength_{timeduration(24, 0, 0, 0)}
	, eof_{false}
{
	BOOST_LOG_TRIVIAL(info) << "Datasets to be downloaded: " << put_list(datasets);
}

cdownload::Chunk cdownload::ChunkDownloader::nextChunk()
{
	std::size_t maxDownloadedFileSize = 0;
	Chunk res;
	bool downloaded = false;
	while (!downloaded) {
		try {
			res.files = downloadChunk(currentChunkStart_, currentChunkLength_, maxDownloadedFileSize);
			downloaded = true;
		} catch (CSADownloader::HTTPError& er) {
			// there might be error 413 (Request Entity Too Large), we decrease chunk length then
			if (er.httpStatusCode() == 413 || er.httpStatusCode() == 502) {
				// So, we have to decrease the chunk length, but, if the chunk is shorter than
				// 1h already, something wrong is happening, and we are exiting
				if (currentChunkLength_ < timeduration(1, 0, 0, 0)) {
					throw;
				}

				// the server returned a string with exact requested size and maximal allowed size,
				// but we just decrease currentChunkLength_ by the factor of 2
				currentChunkLength_ /= 2;
				BOOST_LOG_TRIVIAL(trace) << "Received HTTP error " << er.httpStatusCode()
					<< ", decreasing chunk length to " << currentChunkLength_;
			} else {
				throw;
			}
		}
	}
	res.startTime = currentChunkStart_;
	res.endTime = currentChunkStart_ + currentChunkLength_;

	if (res.endTime < end_) {

		currentChunkStart_ = res.endTime + timeduration(0, 0, 1, 0); // 1 second

		constexpr const std::size_t MAX_SYNCHRONOUS_MODE_FILE_SIZE = 1 << 30; // 1 GiB

		constexpr const std::size_t MAX_CHUNK_TARGET_SIZE = 0.85 * MAX_SYNCHRONOUS_MODE_FILE_SIZE;
		int ratio = MAX_CHUNK_TARGET_SIZE / maxDownloadedFileSize;
		if (ratio == 0) {
			ratio = 1;
		}

		timeduration newChunkLength = currentChunkLength_ * ratio;
		if (currentChunkStart_ + newChunkLength > end_) {
			newChunkLength = end_ - currentChunkStart_;
		}

		currentChunkLength_ = newChunkLength;
	} else {
		eof_ = true;
	}

	return res;
}

std::map<std::string, cdownload::DownloadedProductFile>
cdownload::ChunkDownloader::downloadChunk(const datetime& startTime, const timeduration& duration, std::size_t& maxDownloadedFileSize)
{
	maxDownloadedFileSize = 0;
	std::map<DatasetName, DownloadedProductFile> res;
	std::size_t maxFileSize = 0;
	BOOST_LOG_TRIVIAL(debug) << "Downloading datasets for time range ["
	                         << startTime << ',' << startTime + duration << "]";

	// if several downloads are going in parallel, we need a unique ID for each
	// to prevent file overwriting. Let's generate one from startTime
	// as total number of seconds from the start_
	const std::string chunkId = boost::lexical_cast<std::string>((startTime - start_).total_seconds());

	for (auto ds: datasets_) {
		BOOST_LOG_TRIVIAL(trace) << "Downloading dataset '" << ds << '\'';
		auto destFileName = tmpDir_ / (ds + '_' + chunkId + '_' + ".tar.gz");
		if (boost::filesystem::exists(destFileName)) {
			BOOST_LOG_TRIVIAL(trace) << "Chunk downloader: destination file " << destFileName << " already exists";
			boost::filesystem::remove(destFileName);
		}

		{
			std::ofstream of {destFileName.c_str()};
			downloader_.download(ds, of, startTime, startTime + duration);
		}
		boost::uintmax_t curFileSize = boost::filesystem::file_size(destFileName);
		if (curFileSize > maxFileSize) {
			maxFileSize = curFileSize;
		}
		res[ds] = DownloadedProductFile(destFileName, tmpDir_, ds);
	}

	maxDownloadedFileSize = maxFileSize;
	return res;
}

bool cdownload::ChunkDownloader::eof() const
{
	return eof_;
}

bool cdownload::Chunk::empty() const
{
	return files.empty();
}
