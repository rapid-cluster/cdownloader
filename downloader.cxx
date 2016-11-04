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
#include <ctime>

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
	, session_{curl_easy_init()}
{
	if (!session_) {
		throw std::runtime_error("Error initializing CURL");
	}
	curl_easy_setopt(session_, CURLOPT_WRITEFUNCTION, &CSADownloader::curlWriteCallback);
}

cdownload::CSADownloader::~CSADownloader()
{
	curl_easy_cleanup(session_);
}

void cdownload::CSADownloader::downloadData(const std::string& url, std::ostream& output) const
{
	std::string requestUrl = addCookie_ ? url + "&CSACOOKIE=" + cookie_ : url;
#ifdef DEBUG_DOWNLOADING_ACTIONS
	BOOST_LOG_TRIVIAL(debug) << "Downloading data from url " << requestUrl;
#endif

	CurlUserData userData {output};

	curl_easy_setopt(session_, CURLOPT_URL, requestUrl.c_str());
	curl_easy_setopt(session_, CURLOPT_WRITEDATA, &userData);

	CURLcode curl_status_code = curl_easy_perform(session_);
	if (curl_status_code != CURLE_OK) {
		throw DownloadError(requestUrl, "Downloading unsuccessful");
	}
	long httpCode = 0;
	curl_easy_getinfo (session_, CURLINFO_RESPONSE_CODE, &httpCode);
	constexpr const long HTTP_CODE_NO_ERROR = 200;
	if (httpCode != HTTP_CODE_NO_ERROR) {
		BOOST_LOG_TRIVIAL(info) << "HTTP request returned code " << httpCode;
		throw HTTPError(requestUrl, httpCode);
	}
}

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

size_t cdownload::CSADownloader::curlWriteCallback(char* ptr, size_t size, size_t nmemb, void* userdata)
{
	CurlUserData* cud = static_cast<CurlUserData*>(userdata);
	cud->os.write(ptr, static_cast<std::streamsize>(size * nmemb));
	if (cud->os) {
		return size * nmemb;
	} else {
		return 0;
	}
}

void cdownload::DataDownloader::download(const std::string& datasetName, std::ostream& output, const datetime& startDate, const datetime& endDate) const
{
	std::string requestUrl = this->buildRequest(datasetName, startDate, endDate);
	downloadData(requestUrl, output);
}

std::string cdownload::DataDownloader::buildRequest(const std::string& datasetName, const datetime& startDate, const datetime& endDate) const
{
#ifdef STD_CHRONO
	std::time_t stTime = std::chrono::system_clock::to_time_t(startDate);
	auto stTimeUTC = std::gmtime(&stTime);

	std::time_t endTime = std::chrono::system_clock::to_time_t(endDate);
	auto endTimeUTC = std::gmtime(&endTime);
#else
	std::string stTimeString = boost::posix_time::to_iso_extended_string(startDate);
	std::string stEndString = boost::posix_time::to_iso_extended_string(endDate);
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
	downloadData(url, dataStream);
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
                                                    const std::vector<std::string>& fields) const
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
	downloadData(url, dataStream);
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

cdownload::CSADownloader::DownloadError::DownloadError(const std::string& url, const std::string& message)
	: std::runtime_error(message + " : Error occurred while downloading data from URL " + url)
{
}

cdownload::CSADownloader::HTTPError::HTTPError(const std::string& url, int httpStatusCode)
	: DownloadError(url, std::string("HTTP request returned status ")
		+ boost::lexical_cast<std::string>(httpStatusCode))
	, httpStatusCode_{httpStatusCode}
{
}
