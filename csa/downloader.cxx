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

#include "./downloader.hxx"

#include <fstream>
#include <sstream>

#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/lexical_cast.hpp>

#include <json/reader.h>

const char* cdownload::csa::DataDownloader::DATASET_ID_PARAMETER_NAME = "DATASET_ID";
const char* cdownload::csa::MetadataDownloader::DATASET_ID_QUERY_PARAMETER = "DATASET.DATASET_ID";

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

cdownload::csa::Downloader::Downloader(bool addCookie)
	: addCookie_{addCookie}
	, cookie_(addCookie_ ? readCookie() : std::string())
{
}

std::string cdownload::csa::Downloader::decorateUrl(const std::string& url) const
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

std::string cdownload::csa::Downloader::encode(const std::string& s) const
{
	std::unique_ptr<char, CURLDeleter> output {curl_easy_escape(session_, s.c_str(), s.size())};
	return output ? std::string(output.get()) : std::string();
}
#endif

void cdownload::csa::DataDownloader::beginDownloading(const std::string& datasetName, std::ostream& output, const datetime& startDate, const datetime& endDate)
{
	std::string requestUrl = this->buildRequest(datasetName, startDate, endDate);
	base::beginDownloading(requestUrl, output);
}

std::string cdownload::csa::DataDownloader::buildRequest(const std::string& datasetName, const datetime& startDate, const datetime& endDate) const
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

cdownload::csa::MetadataDownloader::MetadataDownloader()
	: csa::Downloader(false)
{
}

std::vector<cdownload::DatasetName> cdownload::csa::MetadataDownloader::downloadDatasetsList()
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

Json::Value cdownload::csa::MetadataDownloader::download(const std::vector<DatasetName>& datasets,
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

void cdownload::csa::MetadataDownloader::writeConditions(std::ostream& os, const std::string& keyName, const std::string& operation, const std::vector<std::string>& values)
{
	std::vector<string> expressions;
	expressions.reserve(values.size());
	std::transform(values.begin(), values.end(), std::back_inserter(expressions),
	               [&keyName](const string& s) { return keyName + " == '" + s + '\'';});

	os << put_list(expressions, ' ' + operation + ' ', "", "");
}


