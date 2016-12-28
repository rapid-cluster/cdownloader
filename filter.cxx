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

cdownload::Filter::Filter(const std::string& name, std::size_t maxFieldsCount)
	: maxFieldsCount_{maxFieldsCount}
	, name_{name}
{
	activeFields_.reserve(maxFieldsCount_);
}


std::vector<cdownload::ProductName> cdownload::Filter::requiredProducts() const {
	std::vector<ProductName> res;
	std::transform(activeFields_.begin(), activeFields_.end(), std::back_inserter(res),
	               [](const Field& f) {return f.name();});
	return res;
}

void cdownload::Filter::initialize(const std::vector<Field>& availableProducts) {
	availableProducts_ = availableProducts;
	for (std::size_t i = 0; i< availableProducts.size(); ++i) {
		const Field& af = availableProducts[i];
		for (Field& f: activeFields_) {
			if (f.name() == af.name()) {
				f = af;
			}
		}
	}
	for (const Field& f: activeFields_) {
		if (f.offset() == static_cast<std::size_t>(-1)) {
			throw std::logic_error("Field '" + f.name().name() + "' offset is unknown");
		}
	}

}

const cdownload::Field& cdownload::Filter::addField(const std::string& productName) {
	activeFields_.push_back(Field(FieldDesc(ProductName(productName)), static_cast<std::size_t>(-1)));
	if (activeFields_.size() > maxFieldsCount_) {
		throw std::logic_error("Relocation in fields array makes all field references invalid");
	}
	return activeFields_.back();
}

const cdownload::Field& cdownload::Filter::field(const std::string& name) {
	for (const Field& f: activeFields_) {
		if (f.name() == name) {
			return f;
		}
	}
	throw std::runtime_error("There is no field with name '" + name + "' in this filter");
}

cdownload::RawDataFilter::RawDataFilter(const std::string& name, std::size_t maxFieldsCount)
	: Filter(name, maxFieldsCount)
{
}

cdownload::AveragedDataFilter::AveragedDataFilter(const std::string& name, std::size_t maxFieldsCount)
	: Filter(name, maxFieldsCount)
{
}
