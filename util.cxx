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

#include "util.hxx"

#include <boost/algorithm/string/replace.hpp>
#include <boost/log/trivial.hpp>


#include <algorithm>
#include <stdexcept>
#include <iostream>
#include <set>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>

#include "config.h"

#ifdef USE_BOOST_REGEX
#include <boost/regex.hpp>
#else
#include <regex>
#endif

cdownload::ProductName::ProductName()
	: variableName_()
	, datasetName_()
{
}

cdownload::ProductName::ProductName(const std::string& name)
	: variableName_{name}
{
	auto delimPos = name.find(delimiter);
	if (delimPos == std::string::npos ||
	    delimPos + 3 > name.size()) {
		throw std::runtime_error("product name '" + name + "' is malformed");
	}
//  variableName_ = name.substr(0, delimPos);
	datasetName_ = name.substr(delimPos + 2);
}

std::ostream& cdownload::operator<<(std::ostream& os, const cdownload::ProductName& pr)
{
	os << pr.name();
	return os;
}

std::istream & cdownload::operator>>(std::istream& is, cdownload::ProductName& product)
{
	std::string name;
	is >> name;
	product = ProductName(name);
	return is;
}


cdownload::path cdownload::homeDirectory()
{
	const char* homedir;

	if ((homedir = getenv("HOME")) == NULL) {
		homedir = getpwuid(getuid())->pw_dir;
	}
	return path(homedir);
}

namespace {
	cdownload::string convertWildcardToRegex(const cdownload::string& wildcard)
	{
		cdownload::string res(wildcard);
		boost::algorithm::replace_all(res, "*", ".*");
		boost::algorithm::replace_all(res, "?", ".");
		boost::algorithm::replace_all(res, "\\", "\\\\");
		boost::algorithm::replace_all(res, ":", "\\:");
		BOOST_LOG_TRIVIAL(trace) << "Wildcard " << wildcard << " converted to regex: " << res;
		return "^" + res + "$";
	}
}

std::vector<cdownload::ProductName>
cdownload::expandWildcardsCaseSensitive(const std::vector<cdownload::ProductName>& wildcards, const std::vector<cdownload::ProductName>& avaliable)
{
	std::vector<string> wcStrings;
	std::transform(wildcards.begin(), wildcards.end(), std::back_inserter(wcStrings),
	               [](const ProductName& n) {return n.name();});

	std::vector<string> avStrings;;
	std::transform(avaliable.begin(), avaliable.end(), std::back_inserter(avStrings),
	               [](const ProductName& n) {return n.name();});

	std::vector<string> resStrings = expandWildcardsCaseSensitive(wcStrings, avStrings);

	std::vector<ProductName> res;
	std::transform(resStrings.begin(), resStrings.end(), std::back_inserter(res),
	               [](const string& s) {return ProductName(s);});
	return res;
}

std::vector<std::string>
cdownload::expandWildcardsCaseSensitive(const std::vector<std::string>& wildcards, const std::vector<std::string>& available)
{
	BOOST_LOG_TRIVIAL(trace) << "Expanding wildcarded list " << put_list(wildcards) << " against " << put_list(available);
	std::vector<std::string> res;
	for (const string& wc: wildcards) {
#ifdef USE_BOOST_REGEX
		boost::regex rx(convertWildcardToRegex(wc));
		for (const string& a: available) {
			if (boost::regex_match(a, rx)) {
#else
		std::regex rx(convertWildcardToRegex(wc));
		for (const string& a: available) {
			if (std::regex_match(a, rx)) {
#endif
				res.push_back(a);
			}
		}
	}
	BOOST_LOG_TRIVIAL(trace) << "Expanded list " << put_list(res);

	return res;
}

cdownload::DatasetProductsMap cdownload::parseProductsList(const std::vector<ProductName>& products)
{
	std::map<DatasetName, std::set<ProductName> > tmp;
	for (const auto& pr: products) {
		auto it = tmp.find(pr.dataset());
		if (it == tmp.end()) {
			it = tmp.insert(std::make_pair(pr.dataset(), std::set<ProductName>())).first;
		}
		it->second.insert(pr);
	}

	cdownload::DatasetProductsMap res;
	for (const auto& p: tmp) {
		auto it = res.insert(std::make_pair(p.first, std::vector<ProductName>())).first;
		std::copy(p.second.begin(), p.second.end(), std::back_inserter(it->second));
	}
	return res;
}

cdownload::DatasetProductsMap cdownload::parseProductsList(const std::vector<std::string>& products)
{
	std::vector<ProductName> tmp;
	std::transform(products.begin(), products.end(), std::back_inserter(tmp),
	               [](const std::string& s) {return ProductName(s);});
	return parseProductsList(tmp);
}

std::vector<cdownload::ProductName> cdownload::expandProductsMap(const DatasetProductsMap& map)
{
	std::vector<cdownload::ProductName> res;
	for (const auto& p: map) {
		std::copy(p.second.begin(), p.second.end(), std::back_inserter(res));
	}
	return res;
}
