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

#ifndef CDOWNLOAD_FIELD_HXX
#define CDOWNLOAD_FIELD_HXX

#include "util.hxx"
#include "average.hxx"

#include <climits>
#include <iosfwd>
#include <unistd.h>
#include <cassert>

namespace cdownload {

	/**
	 * @brief Encapsulates format and size of a CDF variable data record
	 *
	 */
	class FieldDesc {
	public:
		enum class DataType {
			SignedInt,
			UnsignedInt,
			Real,
			Char
		};

		FieldDesc(const ProductName& name, double fillValue, DataType dt = DataType::SignedInt,
		          std::size_t dataSize = 0, std::size_t elementsCount = 0);
		FieldDesc(const std::string& name, double fillValue, DataType dt = DataType::SignedInt,
		          std::size_t dataSize = 0, std::size_t elementsCount = 0);

		const ProductName& name() const
		{
			return name_;
		}

		DataType dataType() const
		{
			return dt_;
		}

		//! in bytes
		std::size_t dataSize() const
		{
			return dataSize_;
		}

		std::size_t elementCount() const
		{
			return elementCount_;
		}

		double fillValue() const
		{
			return fillValue_;
		}

	private:
		ProductName name_;
		DataType dt_;
		std::size_t dataSize_;
		std::size_t elementCount_;
		double fillValue_;
	};


	std::string datatypeName(FieldDesc::DataType dt);
	std::ostream& operator<<(std::ostream& os, const FieldDesc& f);

	/**
	 * @brief Adds buffer offset to the @ref FieldDesc information
	 *
	 * This object is used to handle over access to data inside of a large buffer.
	 *
	 */
	class Field: public FieldDesc {
	public:
		Field(const FieldDesc& f, std::size_t offset);

		std::size_t offset() const
		{
			return offset_;
		}

		template <class T>
		const T* data(const std::vector<const void*>& line) const
		{
			return static_cast<const T*>(line[offset_]);
		}

		template <class T>
		const T* operator()(const std::vector<const void*>& line) const
		{
			return data<T>(line);
		}

		template <class T>
		T* data(const std::vector<void*>& line) const
		{
			return static_cast<T*>(line[offset_]);
		}

		template <class T>
		T* operator()(const std::vector<void*>& line) const
		{
			return data<T>(line);
		}

		const AveragedVariable& data(const std::vector<AveragedVariable>& line) const
		{
			return line[offset_];
		}

		long getLong(const std::vector<const void*>& line, std::size_t index = 0) const
		{
			assert(index < elementCount());
			if (dataType() == DataType::SignedInt) {
				switch (dataSize()) {
				case 1:
					return data<int8_t>(line)[index];
				case 2:
					return data<int16_t>(line)[index];
				case 4:
					return data<int32_t>(line)[index];
				case 8:
					return data<int64_t>(line)[index];
				default:
					throw std::logic_error("Unexpected int data size");
				}
			}
			throw std::logic_error("DataType is not signed int");
		}

		unsigned long getULong(const std::vector<const void*>& line, std::size_t index = 0) const
		{
			assert(index < elementCount());
			if (dataType() == DataType::UnsignedInt) {
				switch (dataSize()) {
				case 1:
					return data<uint8_t>(line)[index];
				case 2:
					return data<uint16_t>(line)[index];
				case 4:
					return data<uint32_t>(line)[index];
				case 8:
					return data<uint64_t>(line)[index];
				default:
					throw std::logic_error("Unexpected uint data size");
				}
			}
			throw std::logic_error("DataType is not unsigned int");
		}

		double getReal(const std::vector<const void*>& line, std::size_t index = 0) const
		{
			assert(index < elementCount());
			if (dataType() == DataType::Real) {
				if (dataSize() == 4) {
					return data<float>(line)[index];
				} else if (dataSize() == 8) {
					return data<double>(line)[index];
				}
				throw std::logic_error("Unexpected float data size");
			}
			throw std::logic_error("DataType is not real");
		}

	private:
		std::size_t offset_;
	};
}
#endif // CDOWNLOAD_FIELD_HXX
