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

#include "filter.hxx"

#include <algorithm>
#include <limits>

const cdownload::DatasetName cdownload::Filter::FAKE_FILTER_DATASET = "FILTER";

namespace {
	const char FilterVarSeparator = '_';
}

cdownload::Filter::Filter(const std::string& name, std::size_t maxFieldsCount, std::size_t maxVariablesCount)
	: enabled_{true}
	, maxFieldsCount_{maxFieldsCount}
	, maxVariablesCount_{maxVariablesCount}
	, name_{name}
{
	requiredFields_.reserve(maxFieldsCount_);
	availableVariables_.reserve(maxVariablesCount_);
	enabledVariables_.reserve(maxVariablesCount_);
}

void cdownload::Filter::enable(bool b)
{
	enabled_ = b;
}

std::vector<cdownload::FieldDesc> cdownload::Filter::variables() const
{
	std::vector<cdownload::FieldDesc> res;
	res.reserve(availableVariables_.size());
	std::transform(availableVariables_.begin(), availableVariables_.end(), std::back_inserter(res),
		[](const Field& f) {return f;});
	return res;
}

std::vector<cdownload::ProductName> cdownload::Filter::requiredProducts() const {
	std::vector<ProductName> res;
	std::transform(requiredFields_.begin(), requiredFields_.end(), std::back_inserter(res),
	               [](const Field& f) {return f.name();});
	return res;
}

void cdownload::Filter::initialize(const std::vector<Field>& availableProducts, const std::vector<Field>& filterVariables) {
	availableProducts_ = availableProducts;
// 	requestedFilterVariables_ = filterVariables;

	for (std::size_t i = 0; i< availableProducts.size(); ++i) {
		const Field& af = availableProducts[i];
		for (Field& f: requiredFields_) {
			if (f.name() == af.name()) {
				f = af;
			}
		}
	}
	for (const Field& f: requiredFields_) {
		if (f.offset() == static_cast<std::size_t>(-1)) {
			throw std::logic_error("Field '" + f.name().name() + "' offset is unknown");
		}
	}

	for (std::size_t i = 0; i< filterVariables.size(); ++i) {
		const Field& vf = filterVariables[i];
		for (std::size_t j = 0; j < availableVariables_.size(); ++j) {
			if (vf.name() == availableVariables_[j].name()) {
				availableVariables_[j] = vf;
				enabledVariables_[j] = true;
				break;
			}
		}
	}
}

cdownload::ProductName cdownload::Filter::composeProductName(const std::string& shortName, const std::string& filterName)
{
	return ProductName(ProductName::makePseudoDatasetName(FAKE_FILTER_DATASET),
	                   composeParameterName(shortName,filterName ));
}

std::string cdownload::Filter::composeParameterName(const std::string& shortName, const std::string& filterName)
{
	return filterName + FilterVarSeparator + shortName;
}

bool cdownload::Filter::productBelongsToFilter(const cdownload::ProductName& pr, const string& filterName)
{
	return pr.shortName().find(filterName + FilterVarSeparator) == 0;
}

std::pair<std::string, std::string> cdownload::Filter::splitParameterName(const std::string& name)
{
	auto sepPos = name.find(FilterVarSeparator);
	if (sepPos == std::string::npos || sepPos == 0 || sepPos + 2 >= name.size()) {
		throw std::runtime_error("Incorrect name");
	}
	return {name.substr(0, sepPos), name.substr(sepPos+1)};
}

cdownload::ProductName cdownload::Filter::composeProductName(const std::string& shortName) const
{
	return composeProductName(shortName, name());
}

const cdownload::Field& cdownload::Filter::addField(const std::string& productName) {
	requiredFields_.push_back(Field(FieldDesc(ProductName(productName), std::numeric_limits<double>::quiet_NaN()),
	                              static_cast<std::size_t>(-1)));
	if (requiredFields_.size() > maxFieldsCount_) {
		throw std::logic_error("Relocation in fields array makes all field references invalid");
	}
	return requiredFields_.back();
}

const cdownload::Field& cdownload::Filter::addVariable(const cdownload::FieldDesc& desc, std::size_t* index)
{
	*index = availableVariables_.size();
	availableVariables_.push_back(Field(desc, static_cast<std::size_t>(-1)));
	enabledVariables_.push_back(false);
	return availableVariables_.back();
}


const cdownload::Field& cdownload::Filter::field(const std::string& name) {
	for (const Field& f: requiredFields_) {
		if (f.name() == name) {
			return f;
		}
	}
	throw std::runtime_error("There is no field with name '" + name + "' in this filter");
}

cdownload::RawDataFilter::RawDataFilter(const std::string& name, std::size_t maxFieldsCount, std::size_t maxVariablesCount)
	: Filter(name, maxFieldsCount, maxVariablesCount)
{
}

cdownload::AveragedDataFilter::AveragedDataFilter(const std::string& name, std::size_t maxFieldsCount, std::size_t maxVariablesCount)
	: Filter(name, maxFieldsCount, maxVariablesCount)
{
}
