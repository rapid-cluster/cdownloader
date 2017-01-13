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

#include "field.hxx"

static_assert(sizeof(float) * CHAR_BIT == 32, "float type is not 32-bit");
static_assert(sizeof(double) * CHAR_BIT == 64, "double type is not 32-bit");
static_assert(sizeof(long) * CHAR_BIT == 64, "long type is not 64-bit");

cdownload::FieldDesc::FieldDesc(const std::string& name, double fillValue, DataType dt,
                                std::size_t dataSize, std::size_t elementsCount)
	: FieldDesc(ProductName(name), fillValue, dt, dataSize, elementsCount)
{
}

cdownload::FieldDesc::FieldDesc(const cdownload::ProductName& name, double fillValue, DataType dt,
                                std::size_t dataSize, std::size_t elementsCount)
	: name_(name)
	, dt_{dt}
	, dataSize_{dataSize}
	, elementCount_{elementsCount}
	, fillValue_{fillValue} {
}

cdownload::Field::Field(const cdownload::FieldDesc& f, std::size_t offset)
	: FieldDesc(f)
	, offset_(offset)
{
}

std::string cdownload::datatypeName(FieldDesc::DataType dt)
{
	switch (dt) {
		case FieldDesc::DataType::SignedInt:
			return "Int";
		case FieldDesc::DataType::UnsignedInt:
			return "UInt";
		case FieldDesc::DataType::Real:
			return "Real";
		case FieldDesc::DataType::Char:
			return "Char";
	}
	throw std::logic_error("Unknown datatype");
}


std::ostream& cdownload::operator<<(std::ostream& os, const cdownload::FieldDesc& f)
{
	os << f.name() << '<' << datatypeName(f.dataType()) << '[' << f.dataSize() << "]>";
	return os;
}
