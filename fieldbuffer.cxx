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

#include "fieldbuffer.hxx"

#include <algorithm>
#include <numeric>

cdownload::FieldBuffer::FieldBuffer(const cdownload::FieldDesc& desc)
	: alignmentSize_{fieldBufferSize(desc)}
	, buffer_(alignmentSize_, 0)
	, offsets_(1, 0)
	, descs_{desc}
{
}


cdownload::FieldBuffer::FieldBuffer(const std::vector<FieldDesc>& fields, std::size_t alignment)
	: alignmentSize_{alignment}
	, buffer_(fieldBufferSize(fields, alignment), 0)
	, offsets_(fieldBufferOffsets(fields, alignment))
	, descs_{fields}
{
}

std::size_t cdownload::FieldBuffer::fieldBufferSize(const cdownload::FieldDesc& desc)
{
	return desc.dataSize() * desc.elementCount();
}

namespace {
	class FieldSize {
	public:
		FieldSize(std::size_t minSize)
			: minSize_{minSize} {
		}

		std::size_t operator()(std::size_t v, const cdownload::FieldDesc& desc) const {
			const std::size_t required = desc.dataSize() * desc.elementCount();

			return v + required + (required % minSize_);
		}
	private:
		std::size_t minSize_;
	};
}

std::size_t cdownload::FieldBuffer::fieldBufferSize(const std::vector<FieldDesc>& fields, std::size_t alignment)
{
	return std::accumulate(fields.begin(), fields.end(), std::size_t(0), FieldSize(alignment));
}

std::vector<std::size_t> cdownload::FieldBuffer::fieldBufferOffsets(const std::vector<FieldDesc>& fields, std::size_t alignment)
{
	std::vector<std::size_t> res;
	res.reserve(fields.size());
	std::size_t currentOffset = 0;
	FieldSize size(alignment);
	for (std::size_t i = 0; i < fields.size(); ++i) {
		res.push_back(currentOffset);
		currentOffset += size(0, fields[i]);
	}
	return res;
}

std::vector<const void*> cdownload::FieldBuffer::readBuffers() const
{
	std::vector<const void*> res;
	res.reserve(offsets_.size());
	for (std::size_t i = 0; i < offsets_.size(); ++i) {
		res.push_back((*this)[i]);
	}
	return res;
}

std::vector<void*> cdownload::FieldBuffer::writeBuffers()
{
	std::vector<void*> res;
	res.reserve(offsets_.size());
	for (std::size_t i = 0; i < offsets_.size(); ++i) {
		res.push_back((*this)[i]);
	}
	return res;
}

std::vector<cdownload::Field> cdownload::FieldBuffer::fields() const
{
	std::vector<Field> res;
	res.reserve(offsets_.size());
	for (std::size_t i = 0; i < offsets_.size(); ++i) {
		res.emplace_back(descs_[i], offset(i));
	}
	return res;
}
