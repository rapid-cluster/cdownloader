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

#ifndef CDOWNLOADER_DOWNLOADER_HXX
#define CDOWNLOADER_DOWNLOADER_HXX

#include "util.hxx"

#include <json/value.h>

#include <iosfwd>
#include <string>
#include <stdexcept>
#include <vector>


/** \file downloader.hxx Provides classes for accessing the CSA archive server to download metadata
 * and data files
 */

namespace cdownload {

	/**
	 * @brief Utility class for accessing the CSA archive
	 *
	 * This class loads CSACOOKIE and does HTTP requests via libcurl
	 *
	 */
	class CSADownloader {
	public:
		CSADownloader(bool addCookie = true);
		~CSADownloader();
		void downloadData(const std::string& url, std::ostream& output) const;

		class DownloadError: public std::runtime_error {
		public:
			DownloadError(const std::string& url, const std::string& message);
		};

		class HTTPError: public DownloadError {
		public:
			HTTPError(const std::string& url, int httpStatusCode);

			int httpStatusCode() const {
				return httpStatusCode_;
			}

		private:
			int httpStatusCode_;
		};

		std::string encode(const std::string& s) const;

	private:
		static size_t curlWriteCallback(char* ptr, size_t size, size_t nmemb, void* userdata);
		bool addCookie_;
		std::string cookie_;
		typedef void CURL;
		CURL* session_;
	};

	/**
	 * @brief Encapsulates data downloading via synchronous requests
	 *
	 */
	class DataDownloader: protected CSADownloader {
	public:
		DataDownloader() = default;
		void download(const std::string& datasetName, std::ostream& output,
		              const datetime& startDate, const datetime& endDate) const;

	private:
		std::string buildRequest(const std::string& datasetName,
		                         const datetime& startDate, const datetime& endDate) const;

		static const char* DATASET_ID_PARAMETER_NAME;
	};

	/**
	 * @brief Encapsulates metadata downloading
	 *
	 */
	class MetadataDownloader: protected CSADownloader {
	public:
		MetadataDownloader();
		std::vector<DatasetName> downloadDatasetsList();
		Json::Value download(const std::vector<DatasetName>& datasets, const std::vector<std::string>& fields) const;

	private:
		static void writeConditions(std::ostream& os,
		                            const std::string& keyName, const std::string& operation,
		                            const std::vector<std::string>& values);
		static const char* DATASET_ID_QUERY_PARAMETER;
	};
}

#endif // CDOWNLOADER_DOWNLOADER_HXX
