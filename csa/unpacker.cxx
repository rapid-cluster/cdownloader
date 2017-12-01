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

#include "unpacker.hxx"

#include <archive.h>
#include <archive_entry.h>

#include <boost/filesystem/operations.hpp>

#include "config.h"

#include <boost/log/trivial.hpp>

namespace {
	int
copy_data(struct archive *ar, struct archive *aw)
{
	int r;
	const void *buff;
	size_t size;
#if ARCHIVE_VERSION_NUMBER >= 3000000
	int64_t offset;
#else
	off_t offset;
#endif

	for (;;) {
		r = archive_read_data_block(ar, &buff, &size, &offset);
		if (r == ARCHIVE_EOF)
			return (ARCHIVE_OK);
		if (r != ARCHIVE_OK) {
			BOOST_LOG_TRIVIAL(warning) << "archive_read_data_block()" << archive_error_string(ar) << std::endl;
			return (r);
		}
		r = archive_write_data_block(aw, buff, size, offset);
		if (r != ARCHIVE_OK) {
			BOOST_LOG_TRIVIAL(warning) << "archive_write_data_block()" << archive_error_string(aw) << std::endl;
			return (r);
		}
	}
}
}

cdownload::path cdownload::extractDataFile(const boost::filesystem::path& fileName,
                                const boost::filesystem::path& dirName,
                                const std::string& datasetId, const std::string& /*fileId*/)
{
	struct archive *a;
	struct archive_entry *entry;
	struct archive *extracted;
	int r;

	a = archive_read_new();
	extracted = archive_write_disk_new();

	archive_read_support_filter_all(a);
	archive_read_support_format_all(a);
	r = archive_read_open_filename(a, fileName.c_str(), 10240); // Note 1
	if (r != ARCHIVE_OK)
		throw std::runtime_error("Can not read archive file '" + fileName.string() + '\'');

	path res;

	while (archive_read_next_header(a, &entry) == ARCHIVE_OK) {
		auto fn = archive_entry_pathname(entry);
		if (strstr(fn, datasetId.c_str())) {
			//extract entry
			const char* currentFile = archive_entry_pathname(entry);
			path cf {currentFile};
			res = dirName / cf.filename();
			archive_entry_set_pathname(entry, res.c_str());
			r = archive_write_header(extracted, entry);
			if (r != ARCHIVE_OK)
				BOOST_LOG_TRIVIAL(warning) << "archive_write_header()" <<
					archive_error_string(extracted) << std::endl;
			else {
				copy_data(a, extracted);
				r = archive_write_finish_entry(extracted);
				if (r != ARCHIVE_OK)
					BOOST_LOG_TRIVIAL(error) << "archive_write_finish_entry()" <<
						archive_error_string(extracted) << std::endl;
			}
			break;
		}
		archive_read_data_skip(a);  // Note 2
	}
	r = archive_read_free(a);  // Note 3
	if (r != ARCHIVE_OK) {
		throw std::runtime_error("Error freeing source archive");
	}
	r = archive_write_free(extracted);
	if (r != ARCHIVE_OK) {
		throw std::runtime_error("Error freeing extracted archive");
	}

	return res;
}

cdownload::DownloadedChunkFile
cdownload::extractAndAccureDataFile(const path& fileName, const path& dirName, const std::string& datasetId, const std::string& fileId)
{
	DownloadedChunkFile res = DownloadedChunkFile(extractDataFile(fileName, dirName, datasetId, fileId), true);
	boost::filesystem::remove(fileName);
	return res;
}
