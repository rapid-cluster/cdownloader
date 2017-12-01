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

#ifndef CDOWNLOAD_CSA_DOWNLOADER_HXX
#define CDOWNLOAD_CSA_DOWNLOADER_HXX

#include "../downloader.hxx"

namespace cdownload {
namespace csa {

	/**
	 * @brief Utility class for accessing the CSA archive
	 *
	 * This class loads CSACOOKIE and does HTTP requests via libcurl
	 *
	 */
	class Downloader: public curl::DownloadManager {
	public:
		Downloader(bool addCookie = true);

		std::string encode(const std::string& s) const;

	private:
		std::string decorateUrl(const std::string & url) const override;
		bool addCookie_;
		std::string cookie_;
	};

	/**
	 * @brief Encapsulates data downloading via synchronous requests
	 *
	 */
	class DataDownloader: public Downloader {
		using base = Downloader;

	public:
		DataDownloader() = default;
		void beginDownloading(const std::string& datasetName, std::ostream& output,
		              const datetime& startDate, const datetime& endDate);

	private:
		std::string buildRequest(const std::string& datasetName,
		                         const datetime& startDate, const datetime& endDate) const;

		static const char* DATASET_ID_PARAMETER_NAME;
	};

	/**
	 * @brief Encapsulates metadata downloading
	 *
	 */
	class MetadataDownloader: public Downloader {
	public:
		MetadataDownloader();
		std::vector<DatasetName> downloadDatasetsList();
		Json::Value download(const std::vector<DatasetName>& datasets, const std::vector<std::string>& fields);

	private:
		static void writeConditions(std::ostream& os,
		                            const std::string& keyName, const std::string& operation,
		                            const std::vector<std::string>& values);
		static const char* DATASET_ID_QUERY_PARAMETER;
	};

}
}


#endif // CDOWNLOAD_CSA_DOWNLOADER_HXX
