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

#include "metadata.hxx"

#include "downloader.hxx"

#include <ctime>
#include <cstring>

#include "config.h"

#ifdef LOG_METADATA_ACTIONS
	#include <boost/log/trivial.hpp>
#endif

cdownload::Metadata::Metadata()
	: datasetNames_(MetadataDownloader().downloadDatasetsList())
{
}

std::vector<std::string> cdownload::Metadata::requiredFields()
{
	return {"DATASET.DATASET_ID",
			"DATASET.TITLE",
			"DATASET.DESCRIPTION",
			"DATASET.TIME_RESOLUTION",
			"DATASET.MIN_TIME_RESOLUTION",
			"DATASET.MAX_TIME_RESOLUTION",
			"DATASET.START_DATE",
			"DATASET.END_DATE"};
}

cdownload::Metadata::DataSetMetadata cdownload::Metadata::dataset(const DatasetName& datasetName) const
{
	auto i = datasets_.find(datasetName);
	if (i == datasets_.end()) {
		i = datasets_.insert(std::make_pair(datasetName, downloadDatasetMetadata(datasetName))).first;
	}
	return i->second;
}


namespace {
	std::pair<cdownload::datetime, cdownload::datetime> datesRangeForDataset(const Json::Value& ds)
	{
#ifdef LOG_METADATA_ACTIONS
		BOOST_LOG_TRIVIAL(debug) << "Detecting dates in: " << ds.toStyledString();
		BOOST_LOG_TRIVIAL(debug) << "Which contains members: " << cdownload::put_list(ds.getMemberNames());
#endif
		std::string startString = ds["DATASET.START_DATE"].asString();
		std::string endString = ds["DATASET.END_DATE"].asString();

#ifdef STD_CHRONO
		struct tm tm;

//	FormatString = '%Y-%m-%d %H:%M:%S.%f'  # '2008-05-19 23:59:59.0'
		std::memset(&tm, 0, sizeof(tm));
		::strptime(startString.c_str(), "%Y-%m-%d %H:%M:%S.%f", &tm);
		datetime start = std::chrono::system_clock::from_time_t(timegm(&tm));

		std::memset(&tm, 0, sizeof(tm));
		::strptime(startString.c_str(), "%Y-%m-%d %H:%M:%S.%f", &tm);
		datetime end = std::chrono::system_clock::from_time_t(timegm(&tm));
#else
		cdownload::datetime start = boost::posix_time::time_from_string(startString);
		cdownload::datetime end = boost::posix_time::time_from_string(endString);

#endif

		return std::make_pair(start, end);
	}

	std::vector<cdownload::string> parameterList(const Json::Value& json)
	{
		std::vector<cdownload::string> res;
		for (const auto& ds: json) {
			if (ds.isMember("PARAMETER.PARAMETER_ID")) {
				res.push_back(ds["PARAMETER.PARAMETER_ID"].asString());
			}
		}
		return res;
	}
}

cdownload::Metadata::DataSetMetadata cdownload::Metadata::downloadDatasetMetadata(const DatasetName& datasetName) const
{
	std::vector<DatasetName> names;
	names.push_back(datasetName);
	MetadataDownloader downloader;
	Json::Value ds = downloader.download(names, requiredFields())["data"][0];

	Json::Value params = downloader.download(names, {"PARAMETER.PARAMETER_ID" /*, "PARAMETER.TYPE"*/})["data"];

	std::pair<datetime, datetime> dates = datesRangeForDataset(ds);

	return DataSetMetadata(ds["DATASET.DATASET_ID"].asString(),
	                       ds["DATASET.TITLE"].asString(),
	                       dates.first, dates.second,
	                       parameterList(params));
}

cdownload::Metadata::DataSetMetadata::DataSetMetadata(const DatasetName& name, const string& title, const datetime& minDate, const datetime& maxDate, const std::vector<string>& parameters)
	: name_{name}
	, title_{title}
	, minDate_{minDate}
	, maxDate_{maxDate}
	, parameters_{parameters}
{
}

std::ostream& cdownload::operator<<(std::ostream& os, const Metadata::DataSetMetadata& ds)
{
	os << "Name: " << ds.name() << std::endl
		<< "Title: " << ds.title() << std::endl
		<< "Time range: [" << ds.minTime() << ", " << ds.maxTime() << ']' << std::endl
		<< "Parameters: " << put_list(ds.parameters()) << std::endl;
	return os;
}
