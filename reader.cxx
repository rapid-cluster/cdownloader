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

#include "./reader.hxx"

cdownload::Reader::Reader(const std::vector<ProductName>& variables,
						  cdownload::Reader::DescriptionForVariable descriptor, cdownload::Reader::VariableCallback cb,
							const ProductName& timeStampVariableName)
	: buffers_(variables.size())
	, timeStampVariableIndex_{static_cast<std::size_t>(-1)}
{
	for (std::size_t i = 0; i < variables.size(); ++i) {
		const ProductName& varName = variables[i];
		const FoundField f = descriptor(varName);
		const std::size_t requiredBufferSize = f.description.dataSize() * f.description.elementCount();
		buffers_[i] = std::unique_ptr<char[]>(new char[requiredBufferSize]);
		cb(i, varName, f);
		if (timeStampVariableName == varName.name()) {
			timeStampVariableIndex_ = i;
		}
	}
	assert(timeStampVariableIndex_ != static_cast<std::size_t>(-1));
}


cdownload::Reader::~Reader() = default;

