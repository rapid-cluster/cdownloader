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

#include "./hroreader.hxx"

#include "./omnidb.hxx"
#include "../csatime.hxx"

#include <fstream>
#include <boost/current_function.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

cdownload::ProductName cdownload::omni::Reader::epochFieldName = cdownload::omni::OmniTableDesc::makeOmniHROName("$EPOCH");

cdownload::omni::Reader::Reader(const cdownload::path& hroFileName, const std::vector<ProductName>& variables,
								const OmniTableDesc& omniFileDesc,
								std::vector<Field>&& requestedVariables, std::vector<std::size_t>&& indicies)
	: base(variables, [&](const ProductName& name) -> FoundField {
							const auto index = omniFileDesc.fieldIndex(name);
							if (index == static_cast<std::size_t>(-1)) {
								throw std::runtime_error("Could not find field " + name.shortName() + " in OMNI DB");
							}
							return {omniFileDesc[index], index};
						},
					[&](std::size_t index, const ProductName& /*name*/, const FoundField& fd) {
						requestedVariables.push_back(Field(fd.description, index));
						indicies.push_back(fd.fieldIndex);
					},
					epochFieldName)
	, fileName_{hroFileName}
	, input_{new std::ifstream(hroFileName.string())}
	, indicies_{indicies}
	, requestedVariables_{requestedVariables}
	, currentLineIndex_{static_cast<std::size_t>(-1)}
{
	readNextLine();
}

cdownload::omni::Reader::Reader(const cdownload::path& hroFileName, const std::vector<ProductName>& variables)
	: Reader(hroFileName, variables, OmniTableDesc(), std::vector<Field>(), std::vector<std::size_t>())
{
// 	OmniTableDesc omnidb;
//
// 	const std::vector<FieldDesc> availabelFields = OmniTableDesc::highResOmniFields();
// 	for (std::size_t i = 0; i < availabelFields.size(); ++i) {
// 		const auto vi = std::find(variables.begin(), variables.end(), availabelFields[i].name());
// 		if (vi != variables.end()) {
// 			indicies_.push_back(i);
// 			requestedVariables_.push_back(Field(omnidb.fieldDesc(vi->shortName()), requestedVariables_.size()));
// 			requestedVariables_.back().
// 		}
// 	}
//
// 	assert(indicies_.size() == variables.size());
// 	readNextLine();
}

template <class T>
void cdownload::omni::Reader::assignField(std::size_t i)
{
	setFieldValue(requestedVariables_[i], boost::lexical_cast<T>(currentLine_.fields[indicies_[i]]));
}

bool cdownload::omni::Reader::readRecord(std::size_t index, bool /*omitTimestamp*/)
{
	if (!readTimeStampRecord(index)) {
		return false;
	}

	// now parse currentLine_.fields into variable buffers
	for (std::size_t i = 0; i < requestedVariables_.size(); ++i) {
		const Field& f = requestedVariables_[i];
		switch (f.dataType()) {
			case FieldDesc::DataType::SignedInt:
				switch (f.dataSize()) {
					case 1:
						assignField<signed char>(i);
						break;
					case 2:
						assignField<signed short>(i);
						break;
					case 4:
						assignField<signed int>(i);
						break;
					case 8:
						assignField<signed long int>(i);
						break;
				}
			case FieldDesc::DataType::UnsignedInt:
				switch (f.dataSize()) {
					case 1:
						assignField<unsigned char>(i);
						break;
					case 2:
						assignField<unsigned short>(i);
						break;
					case 4:
						assignField<unsigned int>(i);
						break;
					case 8:
						assignField<unsigned long int>(i);
						break;
				}
			case FieldDesc::DataType::Real:
				switch (f.dataSize()) {
					case 4:
						assignField<float>(i);
						break;
					case 8:
						assignField<double>(i);
						break;
				}
			default:
				throw std::logic_error(std::string("Unexpected field type in ") + BOOST_CURRENT_FUNCTION);
		}
	}
	return true;
}

bool cdownload::omni::Reader::eof() const
{
	return input_->eof();
}

bool cdownload::omni::Reader::readTimeStampRecord(std::size_t index)
{
	assert(index >= currentLineIndex_);
	while(index > currentLineIndex_) {
		readNextLine();
		if (eof()) {
			return false;
		}
	}
	setFieldValue(requestedVariables_[timeStampVariableIndex_], currentLine_.epoch);
	return true;
}

void cdownload::omni::Reader::readNextLine()
{
	if (!input_->eof()) {
		std::string line;
		std::getline(*input_, line);
		boost::algorithm::split(currentLine_.fields, line, boost::is_any_of(" "), boost::token_compress_on);
		currentLine_.epoch = makeDateTime(
			boost::lexical_cast<unsigned short>(currentLine_.fields[0]),
			0U,
			boost::lexical_cast<unsigned short>(currentLine_.fields[1]),
			boost::lexical_cast<unsigned short>(currentLine_.fields[2]),
			boost::lexical_cast<unsigned short>(currentLine_.fields[3])
		).milliseconds();
		++currentLineIndex_;
	}
}
