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

#include <fstream>
#include <iomanip>
#include <memory>
#include <sstream>
#include <thread>
#include <ctime>
#include <type_traits>

#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/log/trivial.hpp>

#include <curl/curl.h>

#include <json/reader.h>

const char* cdownload::DataDownloader::DATASET_ID_PARAMETER_NAME = "DATASET_ID";
const char* cdownload::MetadataDownloader::DATASET_ID_QUERY_PARAMETER = "DATASET.DATASET_ID";

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
					if (rq->httpStatusCode() != HTTP_CODE_NO_ERROR) {
						throw HTTPError(rq->url(), rq->httpStatusCode());
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

int cdownload::curl::RunningRequest::httpStatusCode() const
{
	return httpStatusCode_;
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
	curl_easy_getinfo(session_, CURLINFO_RESPONSE_CODE, &httpStatusCode_);
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

namespace {
	std::string readCookie()
	{
		boost::filesystem::path cookieFile = cdownload::homeDirectory() / ".csacookie";
		if (boost::filesystem::exists(cookieFile)) {
			std::ifstream ifs(cookieFile.c_str());
			std::string res;
			std::getline(ifs, res);
			return res;
		}
		throw std::runtime_error("Cookie file not found");
	}

	struct CurlUserData {
		CurlUserData(std::ostream& s)
			: os(s)
		{
		}

		std::ostream& os;
	};

	const std::string DATA_DOWNLOAD_BASE_URL = "https://csa.esac.esa.int/csa/aio/product-action";
	const std::string METADATA_DOWNLOAD_BASE_URL = "https://csa.esac.esa.int/csa/aio/metadata-action";
}

cdownload::CSADownloader::CSADownloader(bool addCookie)
	: addCookie_{addCookie}
	, cookie_(addCookie_ ? readCookie() : std::string())
{
}

std::string cdownload::CSADownloader::decorateUrl(const std::string& url) const
{
	return addCookie_ ? url + "&CSACOOKIE=" + cookie_ : url;
}

#if 0
namespace {
	struct CURLDeleter {
		void operator()(void* ptr) const
		{
			curl_free(ptr);
		}
	};
}

std::string cdownload::CSADownloader::encode(const std::string& s) const
{
	std::unique_ptr<char, CURLDeleter> output {curl_easy_escape(session_, s.c_str(), s.size())};
	return output ? std::string(output.get()) : std::string();
}
#endif

void cdownload::DataDownloader::beginDownloading(const std::string& datasetName, std::ostream& output, const datetime& startDate, const datetime& endDate)
{
	std::string requestUrl = this->buildRequest(datasetName, startDate, endDate);
	CSADownloader::beginDownloading(requestUrl, output);
}

std::string cdownload::DataDownloader::buildRequest(const std::string& datasetName, const datetime& startDate, const datetime& endDate) const
{
#ifdef STD_CHRONO
	std::time_t stTime = std::chrono::system_clock::to_time_t(startDate);
	auto stTimeUTC = std::gmtime(&stTime);

	std::time_t endTime = std::chrono::system_clock::to_time_t(endDate);
	auto endTimeUTC = std::gmtime(&endTime);
#else
	std::string stTimeString = startDate.isoExtendedString();
	std::string stEndString = endDate.isoExtendedString();
#endif

	std::ostringstream res;
	res << DATA_DOWNLOAD_BASE_URL
	    << "?&RETRIEVALTYPE=PRODUCT"
	    << "&NON_BROWSER"
	    << "&DELIVERY_FORMAT=CDF"
	    << "&REF_DOC=0"
	    << "&DELIVERY_INTERVAL=All"
	    << '&' << DATASET_ID_PARAMETER_NAME << "=" << datasetName
	    << "&START_DATE=" <<
#ifdef STD_CHRONO
	std::put_time(stTimeUTC, "%FT%TZ")
#else
	stTimeString << 'Z'
#endif
	    << "&END_DATE=" <<
#ifdef STD_CHRONO
	std::put_time(endTimeUTC, "%FT%TZ")
#else
	stEndString << 'Z'
#endif
	;
	return res.str();
}

cdownload::MetadataDownloader::MetadataDownloader()
	: CSADownloader(false)
{
}

std::vector<cdownload::DatasetName> cdownload::MetadataDownloader::downloadDatasetsList()
{
	// we need just "DATASET.DATASET_ID" records
	std::string url = METADATA_DOWNLOAD_BASE_URL +
	                  "?RETURN_TYPE=JSON&RESOURCE_CLASS=DATASET&NON_BROWSER&SELECTED_FIELDS=" + DATASET_ID_QUERY_PARAMETER;

#ifdef DEBUG_DOWNLOADING_ACTIONS
	BOOST_LOG_TRIVIAL(debug) << "Downloading datasets list from url " << url;
#endif
	std::ostringstream dataStream;
	beginDownloading(url, dataStream);
	waitForFinished();
	std::string data = dataStream.str();

#ifdef DEBUG_DOWNLOADING_ACTIONS
	BOOST_LOG_TRIVIAL(debug) << "Downloaded datasets list: " << data;
#endif

	std::istringstream iss(data);
	Json::Value meta;
	iss >> meta;

	std::vector<DatasetName> res;
	for (const auto& ds: meta["data"]) {
		if (ds.isMember(DATASET_ID_QUERY_PARAMETER)) {
			res.push_back(ds[DATASET_ID_QUERY_PARAMETER].asString());
		}
	}
	return res;
}

Json::Value cdownload::MetadataDownloader::download(const std::vector<DatasetName>& datasets,
                                                    const std::vector<std::string>& fields)
{

	std::string url = METADATA_DOWNLOAD_BASE_URL + "?RETURN_TYPE=JSON&RESOURCE_CLASS=DATASET&NON_BROWSER&SELECTED_FIELDS=" +
	                  boost::algorithm::join(fields, ",");

	std::ostringstream queryStream;
	queryStream << "&QUERY=(";
	writeConditions(queryStream, DATASET_ID_QUERY_PARAMETER, "OR", datasets);
	queryStream << ")";

	url += boost::algorithm::replace_all_copy(queryStream.str(), " ", "%20");

#ifdef DEBUG_DOWNLOADING_ACTIONS
	BOOST_LOG_TRIVIAL(debug) << "Downloading metadata from url " << url;
#endif
	std::ostringstream dataStream;
	beginDownloading(url, dataStream);
	waitForFinished();
	std::string data = dataStream.str();
#ifdef DEBUG_DOWNLOADING_ACTIONS
	BOOST_LOG_TRIVIAL(debug) << "Downloaded metadata: " << data;
#endif
	std::istringstream iss(data);
	Json::Value res;
	iss >> res;
	return res;
//         logging.debug("Loaded metadata: {0}".format(self.__metaObj))
//         logging.info("Loaded datasets: {0}".format(self.datasetNames()))
}

#if 0
namespace {
	template<typename T>
	class PrefexOutputIterator {
		std::ostream&       ostream;
		std::string prefix;
		bool first;

	public:

		typedef std::size_t difference_type;
		typedef T value_type;
		typedef T*                          pointer;
		typedef T reference;
		typedef std::output_iterator_tag iterator_category;

		PrefexOutputIterator(std::ostream& o, std::string const& p = "") : ostream(o)
			, prefix(p)
			, first(true)
		{
		}

		PrefexOutputIterator& operator*()
		{
			return *this;
		}

		PrefexOutputIterator& operator++()
		{
			return *this;
		}

		PrefexOutputIterator& operator++(int)
		{
			return *this;
		}

		void operator=(T const& value)
		{
			if (first) {
				ostream << value; first = false;
			} else {
				ostream << prefix << value;
			}
		}
	};
}
#endif

void cdownload::MetadataDownloader::writeConditions(std::ostream& os, const std::string& keyName, const std::string& operation, const std::vector<std::string>& values)
{
	std::vector<string> expressions;
	expressions.reserve(values.size());
	std::transform(values.begin(), values.end(), std::back_inserter(expressions),
	               [&keyName](const string& s) { return keyName + " == '" + s + '\'';});

	os << put_list(expressions, ' ' + operation + ' ', "", "");
}

cdownload::curl::DownloadManager::DownloadError::DownloadError(const std::string& url, const std::string& message)
	: std::runtime_error(message + " : Error occurred while downloading data from URL " + url)
{
}

cdownload::curl::DownloadManager::HTTPError::HTTPError(const std::string& url, int httpStatusCode)
	: DownloadError(url, std::string("HTTP request returned status ")
		+ boost::lexical_cast<std::string>(httpStatusCode))
	, httpStatusCode_{httpStatusCode}
{
}
