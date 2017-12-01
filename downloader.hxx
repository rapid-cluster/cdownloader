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

	class DownloadManager;

	class RunningRequest {
	public:

		enum class Protocol {
			FTP,
			HTTP
		};

		void scheduleCancelling();

		~RunningRequest();

		void beginDownloading(std::ostream& output);
		int statusCode() const;
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

		Protocol protocol() const;
	private:
		RunningRequest(const std::string& url, DownloadManager* manager);

		void download(std::ostream& output);

		static size_t curlWriteCallback(char* ptr, size_t size, size_t nmemb, void* userdata);
		typedef long my_curl_off_t;
		static int curlProgressCallback(void* clientp,
			my_curl_off_t dltotal, my_curl_off_t dlnow, my_curl_off_t ultotal, my_curl_off_t ulnow);

		typedef void CURL;
		CURL* session_ = nullptr;
		bool scheduleCancellation_ = false;
		bool completed_= false;
		bool completedSuccefully_ = false;
		std::string errorMessage_;
		std::thread downloadingThread_;
		long protocolStatusCode_;
		std::string url_;
		friend class DownloadManager;
		DownloadManager* manager_;
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

		class TransferError: public DownloadError {
		public:
			TransferError(const std::string& url, int statusCode);

			int statusCode() const {
				return statusCode_;
			}

		private:
			static std::string protocolName(const std::string& url);
			int statusCode_;
		};

		RunningRequestWeakPtr beginDownloading(const std::string& url, std::ostream& output);
		void cancelAllRequests();
		void waitForFinished();
	private:

		friend class RunningRequest;
		void requestCompleted(RunningRequest* request);
		bool removeCompletedRequests();
		virtual std::string decorateUrl(const std::string& url) const;

		std::map<std::string, RunningRequestSharedPtr> activeRequests_;
		std::mutex activeRequestsMutex_;
		std::mutex completedRequestsMutex_;
		std::condition_variable requestsCV_;
		std::atomic<bool> ignoreDownloadingErrors_;
		std::vector<std::string> completedRequests_;
	};

}
}

#endif // CDOWNLOADER_DOWNLOADER_HXX
