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

#include <boost/signals2/signal.hpp>

#include <atomic>
#include <condition_variable>
#include <iosfwd>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <stdexcept>
#include <thread>
#include <vector>


/** \file downloader.hxx Provides classes for accessing the CSA archive server to download metadata
 * and data files
 */

namespace cdownload {
	namespace curl {

	class RunningRequest {
	public:

		void scheduleCancelling();
		boost::signals2::signal<void (const RunningRequest*)> cancelled;
		boost::signals2::signal<void (RunningRequest*)> completed;

		~RunningRequest();

		void beginDownloading(std::ostream& output);
		int httpStatusCode() const;
		bool completedSuccefully() const;

		void waitForFinished();

		const std::string url() const {
			return url_;
		}
		const std::string errorMessage() const {
			return errorMessage_;
		}

		bool isCompleted() const {
			return completed_;
		}
	private:
		RunningRequest(const std::string& url);

		void download(std::ostream& output);

		static size_t curlWriteCallback(char* ptr, size_t size, size_t nmemb, void* userdata);

		typedef void CURL;
		CURL* session_ = nullptr;
		bool scheduleCancellation_ = false;
		bool completed_= false;
		bool completedSuccefully_ = false;
		std::string errorMessage_;
		std::thread downloadingThread_;
		long httpStatusCode_;
		std::string url_;
		friend class DownloadManager;
	};

	class DownloadManager {
	public:
		DownloadManager();
		virtual ~DownloadManager();

		using RunningRequestSharedPtr = std::shared_ptr<RunningRequest>;
		using RunningRequestWeakPtr = std::weak_ptr<RunningRequest>;


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


		RunningRequestWeakPtr beginDownloading(const std::string& url, std::ostream& output);
		void cancelAllRequests();
		void waitForFinished();
	private:
		void requestCompleted(RunningRequest* request);
		bool removeCompletedRequests();
		virtual std::string decorateUrl(const std::string& url) const;

		std::map<std::string, RunningRequestSharedPtr> activeRequests_;
		std::mutex requestsMutex_;
		std::condition_variable requestsCV_;
		std::atomic<bool> ignoreDownloadingErrors_;
		std::atomic<std::size_t> completedRequests_;
	};
	}


	/**
	 * @brief Utility class for accessing the CSA archive
	 *
	 * This class loads CSACOOKIE and does HTTP requests via libcurl
	 *
	 */
	class CSADownloader: public curl::DownloadManager {
	public:
		CSADownloader(bool addCookie = true);

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
	class DataDownloader: public CSADownloader {
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
	class MetadataDownloader: public CSADownloader {
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

#endif // CDOWNLOADER_DOWNLOADER_HXX
