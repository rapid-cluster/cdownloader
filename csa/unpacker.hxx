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

#ifndef CDWONLOAD_UPACKER_HXX
#define CDWONLOAD_UPACKER_HXX

#include "../util.hxx"

namespace cdownload {

	//! Unpacks downloaded .tar.gz file, removes everything except the
    //! data file (which is found by datasetId in its name
	path extractDataFile(const boost::filesystem::path& fileName,
	                     const boost::filesystem::path& dirName,
	                     const std::string& datasetId, const std::string& fileId = "");

	DownloadedChunkFile extractAndAccureDataFile(const boost::filesystem::path& fileName,
	                     const boost::filesystem::path& dirName,
	                     const std::string& datasetId, const std::string& fileId = "");
}

#endif // CDWONLOAD_UPACKER_HXX

