/*
 * cdownload lib: downloads, unpacks, and reads data from the Cluster CSA arhive
 * Copyright (C) 2016-2017 Eugene Shalygin <eugene.shalygin@gmail.com>
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

#include "./timefilter.hxx"

#include "../csatime.hxx"

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

#include <algorithm>
#include <fstream>
#include <stdexcept>
#include <sstream>
#include <vector>

namespace {
	struct RangeComparisonByBegin {
		bool operator()(const cdownload::EpochRange& left, const cdownload::EpochRange& right) const
		{
			return left.begin() < right.begin();
		}
	};
}

cdownload::Filters::TimeFilter::TimeFilter(const path& fileName)
	: ranges_{loadRanges(fileName)}
{
}

bool cdownload::Filters::TimeFilter::test(EpochRange::EpochType epoch) const
{
	auto it = std::upper_bound(ranges_.begin(), ranges_.end(), epoch,
	                           [](EpochRange::EpochType epoch, const EpochRange& range) {
		return epoch < range.end();
	});

	if (it != ranges_.end()) {
		if (it->contains(epoch) || (it != ranges_.begin() && (it - 1)->contains(epoch))) {
			return true;
		}
	}
	return false;
}

namespace {
	cdownload::EpochRange parseRange(const cdownload::string& str)
	{
		// string example: [2008-12-21T08:06:32Z;2008-12-23T05:21:32Z]
		std::istringstream is(str);
		cdownload::DateTime begin, end;
		char delim, bracketL, bracketR;
		is >> bracketL >> begin >> delim >> end >> bracketR;
		if (bracketL != '[' || bracketR != ']') {
			throw std::runtime_error("Malformed date range string");
		}
		return cdownload::EpochRange::fromRange(begin.milliseconds(), end.milliseconds());
	}
}

std::vector<cdownload::EpochRange> cdownload::Filters::TimeFilter::loadRanges(const path& fileName)
{
	std::ifstream inp(fileName.c_str());
	string line;
	std::getline(inp, line); // read header
	std::vector<string> headerParts;
	boost::algorithm::split(headerParts, line, boost::is_any_of("\t"), boost::token_compress_on);
	const std::size_t entriesColumnIndex = headerParts.size() - 1; // the last one

	std::vector<cdownload::EpochRange> res;

	std::vector<string> parts;
	while (inp) {
		std::getline(inp, line);
		boost::algorithm::split(parts, line, boost::is_any_of("\t,"), boost::token_compress_on);
		for (std::size_t i = entriesColumnIndex; i < parts.size(); ++i) {
			res.push_back(parseRange(parts[i]));
		}
	}

	std::sort(res.begin(), res.end(), RangeComparisonByBegin());
	return res;
}
