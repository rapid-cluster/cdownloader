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

#include "downloader.hxx"

#include <iomanip>
#include <memory>
#include <thread>
#include <ctime>
#include <type_traits>

#include <boost/algorithm/string/case_conv.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/log/trivial.hpp>

#include <curl/curl.h>

#include "config.h"

namespace {
		struct RunningRequestUserData {
		RunningRequestUserData(std::ostream& s, const cdownload::curl::RunningRequest* r )
			: os(s)
			, request(r)
		{
		}

		std::ostream& os;
		const cdownload::curl::RunningRequest* request;
	};
}

cdownload::curl::DownloadManager::DownloadManager()
{
	curl_global_init(CURL_GLOBAL_ALL);
}

cdownload::curl::DownloadManager::~DownloadManager()
{
	cancelAllRequests();
}

std::string cdownload::curl::DownloadManager::decorateUrl(const std::string& url) const
{
	return url;
}

cdownload::curl::DownloadManager::RunningRequestWeakPtr
cdownload::curl::DownloadManager::beginDownloading(const std::string& url, std::ostream& output)
{
	ignoreDownloadingErrors_ = false;
	const std::string requestUrl = decorateUrl(url);
	std::unique_lock<std::mutex> lk(activeRequestsMutex_);
	auto i = activeRequests_.find(requestUrl);
	if (i != activeRequests_.end()) {
		throw std::logic_error("Url '" + requestUrl + " is downloaded already");
	}
	RunningRequestSharedPtr request(new RunningRequest(requestUrl, this));
	activeRequests_[requestUrl] = request;
	request->beginDownloading(output);
	return RunningRequestWeakPtr(request);
}

void cdownload::curl::DownloadManager::requestCompleted(cdownload::curl::RunningRequest* request)
{
	std::unique_lock<std::mutex> lk (completedRequestsMutex_);
	completedRequests_.push_back(request->url());
// 	request->downloadingThread_.join();
// 	activeRequests_.erase(request->url());
	requestsCV_.notify_one();
}

void cdownload::curl::DownloadManager::cancelAllRequests()
{
	ignoreDownloadingErrors_ = true;
	for (auto& p: activeRequests_) {
		p.second->scheduleCancelling();
	}
	waitForFinished();
}

bool cdownload::curl::DownloadManager::removeCompletedRequests()
{
	std::unique_lock<std::mutex> lk(completedRequestsMutex_);
	for (const auto& url: completedRequests_) {
		auto it = activeRequests_.find(url);
		if (it != activeRequests_.end()) {
			RunningRequestSharedPtr rq = it->second;
			if (rq->downloadingThread_.joinable()) {
				rq->downloadingThread_.join();
			}
			if (!ignoreDownloadingErrors_) {
				if (rq->completedSuccefully()) {
					constexpr const long HTTP_CODE_NO_ERROR = 200;
					constexpr const long FTP_CODE_TRANSFER_COMPLETE = 226;
					const auto protocol = rq->protocol();
					if (protocol == RunningRequest::Protocol::HTTP && rq->statusCode() != HTTP_CODE_NO_ERROR) {
						throw TransferError(rq->url(), rq->statusCode());
					} else if (protocol == RunningRequest::Protocol::FTP && rq->statusCode() != FTP_CODE_TRANSFER_COMPLETE) {

					}
				} else {
					throw DownloadError(rq->url(), rq->errorMessage());
				}
			}
			activeRequests_.erase(it);
		}
	}
	completedRequests_.clear();
	return activeRequests_.empty();
}

void cdownload::curl::DownloadManager::waitForFinished()
{
	std::unique_lock<std::mutex> lk(activeRequestsMutex_);
	requestsCV_.wait(lk, [this](){return this->removeCompletedRequests();});
}

cdownload::curl::RunningRequest::RunningRequest(const std::string& url, DownloadManager* manager)
	: url_(url)
	, manager_(manager)
{
}

cdownload::curl::RunningRequest::~RunningRequest()
{
}

int cdownload::curl::RunningRequest::statusCode() const
{
	return protocolStatusCode_;
}

bool cdownload::curl::RunningRequest::completedSuccefully() const
{
	return completedSuccefully_;
}

void cdownload::curl::RunningRequest::waitForFinished()
{
	downloadingThread_.join();
}

void cdownload::curl::RunningRequest::beginDownloading(std::ostream& output)
{
	downloadingThread_ = std::thread([this,&output](){
		this->download(output);
	});
}

void cdownload::curl::RunningRequest::download(std::ostream& output)
{
	completedSuccefully_ = false;
	session_ = curl_easy_init();
	if (!session_) {
		throw std::runtime_error("Error initializing CURL");
	}
	curl_easy_setopt(session_, CURLOPT_WRITEFUNCTION, &RunningRequest::curlWriteCallback);
	curl_easy_setopt(session_, CURLOPT_XFERINFOFUNCTION, &RunningRequest::curlProgressCallback);

	RunningRequestUserData userData {output, this};

	curl_easy_setopt(session_, CURLOPT_URL, url_.c_str());
	curl_easy_setopt(session_, CURLOPT_WRITEDATA, &userData);
	curl_easy_setopt(session_, CURLOPT_XFERINFODATA, &userData);
	curl_easy_setopt(session_, CURLOPT_NOPROGRESS, 0);
#ifdef DEBUG_DOWNLOADING_ACTIONS
	BOOST_LOG_TRIVIAL(debug) << "Downloading data from url " << url_;
#endif
	CURLcode curl_status_code = curl_easy_perform(session_);
	if (curl_status_code != CURLE_OK) {
		errorMessage_ = std::string(curl_easy_strerror(curl_status_code));
	} else {
		completedSuccefully_ = true;
	}
	curl_easy_getinfo(session_, CURLINFO_RESPONSE_CODE, &protocolStatusCode_);
	curl_easy_cleanup(session_);
	completed_ = true;
	manager_->requestCompleted(this);
}

size_t cdownload::curl::RunningRequest::curlWriteCallback(char* ptr, size_t size, size_t nmemb, void* userdata)
{
	RunningRequestUserData* cud = static_cast<RunningRequestUserData*>(userdata);
	if (cud->request->scheduleCancellation_) {
		return 0; // which will tell CURL to cancel downloading
	}
	cud->os.write(ptr, static_cast<std::streamsize>(size * nmemb));
	if (cud->os) {
		return size * nmemb;
	} else {
		return 0;
	}
}

int cdownload::curl::RunningRequest::curlProgressCallback(void* clientp,
	curl_off_t /*dltotal*/, curl_off_t /*dlnow*/, curl_off_t /*ultotal*/, curl_off_t /*ulnow*/)
{
	static_assert(std::is_same<curl_off_t, my_curl_off_t>::value, "");
	RunningRequestUserData* cud = static_cast<RunningRequestUserData*>(clientp);
	if (cud->request->scheduleCancellation_) {
		return 1; // which will tell CURL to cancel downloading
	}
	return 0;
}

void cdownload::curl::RunningRequest::scheduleCancelling()
{
	scheduleCancellation_ = true;
}

cdownload::curl::RunningRequest::Protocol cdownload::curl::RunningRequest::protocol() const
{
	if (url().find("http") == 0) {
		return Protocol::HTTP;
	}
	if (url().find("ftp") == 0) {
		return Protocol::FTP;
	}
	throw std::runtime_error("Unknown URL scheme");
}

cdownload::curl::DownloadManager::DownloadError::DownloadError(const std::string& url, const std::string& message)
	: std::runtime_error(message + " : Error occurred while downloading data from URL " + url)
{
}

cdownload::curl::DownloadManager::TransferError::TransferError(const std::string& url, int statusCode)
	: DownloadError(url, std::string(protocolName(url) + " request returned status ")
		+ boost::lexical_cast<std::string>(statusCode))
	, statusCode_{statusCode}
{
}

std::string cdownload::curl::DownloadManager::TransferError::protocolName(const std::string& url)
{
	const std::size_t colonPos = url.find(':');
	if (colonPos == std::string::npos) {
		return {};
	}
	return boost::algorithm::to_upper_copy(url.substr(0, colonPos));
}
