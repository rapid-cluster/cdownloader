/*
 * cdownload lib: downloads, unpacks, and reads data from the Cluster CSA arhive
 * Copyright (C) 2017 Eugene Shalygin <eugene.shalygin@gmail.com>
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

#include "./blankdata.hxx"

#include "../floatcomparison.hxx"

cdownload::Filters::BlankDataFilter::BlankDataFilter(const std::map<ProductName, double>& blanks)
	: base("Blank", blanks.size())
{
	for (const auto& pb: blanks) {
		const Field& f = addField(pb.first.name());
		fields_.emplace_back(f, pb.second);
	}
}

bool cdownload::Filters::BlankDataFilter::test(const std::vector<const void *>& line, const DatasetName& /*ds*/,
                                               std::vector<void*>& /*variables*/) const
{
	if (!enabled()) {
		return true;
	}

	for (const auto& p: fields_) {
		switch (p.first.dataType()) {
			case FieldDesc::DataType::UnsignedInt:
				if (p.first.getULong(line, 0) == static_cast<unsigned long>(p.second)) {
					return false;
				}
				break;
			case FieldDesc::DataType::SignedInt:
				if (p.first.getLong(line, 0) == static_cast<long>(p.second)) {
					return false;
				}
				break;
			case FieldDesc::DataType::Real:
				if (gtest::areFloatsEqual(p.first.getReal(line, 0), p.second, 1)) {
					return false;
				}
				break;
			default:
				break;
		}
	}
	return true;
}
