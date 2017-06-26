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

#ifndef CDOWNLOAD_FIELDBUFFER_HXX
#define CDOWNLOAD_FIELDBUFFER_HXX

#include "field.hxx"

#include <memory>

namespace cdownload {

	class FieldBuffer {
	public:
		FieldBuffer(const FieldDesc& desc);
		FieldBuffer(const std::vector<FieldDesc>& fields, std::size_t alignment = sizeof(void*));

		std::size_t size() const {
			return offsets_.size();
		}

		std::vector<Field> fields() const;

		std::vector<void*> writeBuffers();
		std::vector<const void*> readBuffers() const;

	private:
		std::size_t offset(std::size_t index) const {
			return offsets_[index];
		}

		void* operator[](std::size_t index) {
			return &buffer_[offset(index)];
		}

		const void* operator[](std::size_t index) const {
			return &buffer_[offset(index)];
		}

		static std::size_t fieldBufferSize(const FieldDesc& desc);
		static std::size_t fieldBufferSize(const std::vector<FieldDesc>& fields, std::size_t alignment);
		static std::vector<std::size_t> fieldBufferOffsets(const std::vector<FieldDesc>& fields, std::size_t alignment);

		const std::size_t alignmentSize_;
		std::vector<char> buffer_;
		std::vector<std::size_t> offsets_;
		std::vector<FieldDesc> descs_;
	};
}

#endif // CDOWNLOAD_FIELDBUFFER_HXX
