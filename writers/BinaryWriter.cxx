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

#include "./BinaryWriter.hxx"

#include "../util.hxx"
#include "../field.hxx"

#include <cstdio>
#include <fstream>
#include <numeric>
#include <type_traits>

#include "config.h"

namespace datatypenaming {
extern const std::string IEEE_REAL;
extern const std::string PC_REAL;
extern const std::string VAX_REAL;
extern const std::string VAXG_REAL;
extern const std::string LSB_INTEGER;
extern const std::string PC_INTEGER;
extern const std::string LSB_UNSIGNED_INTEGER;
extern const std::string UNSIGNED_INTEGER;
extern const std::string MSB_INTEGER;
extern const std::string MSB_UNSIGNED_INTEGER;

extern const std::string NATIVE_INT;
extern const std::string NATIVE_UNSIGNED_INT;
extern const std::string NATIVE_REAL;
}

namespace {
std::size_t fieldsStride(const std::vector<cdownload::Field>& fields, bool averagedFields)
{
	std::size_t stride = 0;
	if (averagedFields) {
		const std::size_t elementSize = (sizeof(cdownload::AveragingRegister::mean_value_type) +
					sizeof(cdownload::AveragingRegister::counter_type) +
					sizeof(cdownload::AveragingRegister::stddev_value_type));
		for (const cdownload::FieldDesc& f: fields) {
			std::size_t fieldSize = elementSize * f.elementCount();
			stride += fieldSize;
		}
	} else {
		for (const cdownload::FieldDesc& f: fields) {
			std::size_t fieldSize = f.dataSize() * f.elementCount();
			stride += fieldSize;
		}
	}
	return stride;
}

// std::size_t stride(const std::vector<cdownload::Field>& fields, bool writeEpochColumn, bool averagedFields)
// {
// 	std::size_t stride = sizeof(std::size_t) + sizeof(decltype(cdownload::timeduration().seconds()));
// 	if (writeEpochColumn) {
// 		stride += sizeof(decltype(cdownload::timeduration().milliseconds()));
// 	}
// 	return stride + fieldsStride(fields, averagedFields);
// }

// std::size_t stride(const std::vector<cdownload::Field>& averagedFields,
// 		                   const std::vector<cdownload::Field>& rawFields,
// 		                   bool writeEpochColumn)
// {
// 	return stride(averagedFields, writeEpochColumn, true) + fieldsStride(rawFields, false);
// }

std::size_t stride(const cdownload::DirectBinaryWriter::Types::Fields& fields, bool writeEpochColumn, bool averagedFields)
{
	std::size_t stride = std::accumulate(fields.begin(), fields.end(), std::size_t(0),
					[averagedFields](std::size_t a, const cdownload::DirectBinaryWriter::Types::FieldArray& ar) {
						return a + fieldsStride(ar, averagedFields);
					});
	if (writeEpochColumn) {
		stride += sizeof(decltype(cdownload::timeduration().milliseconds()));
	}
	return stride;
}

}

const std::string datatypenaming::IEEE_REAL = std::string("IEEE_REAL");
const std::string datatypenaming::PC_REAL = std::string("PC_REAL");
const std::string datatypenaming::VAX_REAL = std::string("VAX_REAL");
const std::string datatypenaming::VAXG_REAL = std::string("VAXG_REAL");
const std::string datatypenaming::LSB_INTEGER = std::string("LSB_INTEGER");
const std::string datatypenaming::PC_INTEGER = std::string("PC_INTEGER");
const std::string datatypenaming::LSB_UNSIGNED_INTEGER = std::string("LSB_UNSIGNED_INTEGER");
const std::string datatypenaming::UNSIGNED_INTEGER = std::string("UNSIGNED_INTEGER");
const std::string datatypenaming::MSB_INTEGER = std::string("MSB_INTEGER");
const std::string datatypenaming::MSB_UNSIGNED_INTEGER = std::string("MSB_UNSIGNED_INTEGER");


#if SYSTEM_IS_BIG_ENDIAN
const std::string datatypenaming::NATIVE_INT = datatypenaming::MSB_INTEGER;
const std::string datatypenaming::NATIVE_UNSIGNED_INT = datatypenaming::MSB_UNSIGNED_INTEGER;
const std::string datatypenaming::NATIVE_REAL = datatypenaming::IEEE_REAL;
#else
const std::string datatypenaming::NATIVE_INT = datatypenaming::LSB_INTEGER;
const std::string datatypenaming::NATIVE_UNSIGNED_INT = datatypenaming::LSB_UNSIGNED_INTEGER;
const std::string datatypenaming::NATIVE_REAL = datatypenaming::PC_REAL;
#endif

void cdownload::BinaryWriter::FileClose::operator()(::FILE* f) const
{
	std::fclose(f);
}

cdownload::BinaryWriter::BinaryWriter(bool writeEpochColumn, std::size_t stride)
	: Writer{writeEpochColumn}
	, stride_{stride}
{
}

cdownload::BinaryWriter::~BinaryWriter() = default;

void cdownload::BinaryWriter::open(const path& fileName)
{
	output_.reset(std::fopen(fileName.c_str(), "a+"));
	std::fseek(output_.get(), 0, SEEK_END);
	outputFileName_ = fileName;
}

void cdownload::BinaryWriter::truncate()
{
	output_.reset(std::fopen(outputFileName_.c_str(), "w+"));
}

bool cdownload::BinaryWriter::canAppend(std::size_t& lastWrittenCellNumber)
{
	auto curPos = std::ftell(output_.get());
	auto signedStride = static_cast<decltype(curPos)>(stride_);
	if (curPos > signedStride) {
		std::fseek(output_.get(), -signedStride, SEEK_CUR);
		auto read = std::fread(&lastWrittenCellNumber, sizeof(std::size_t), 1, output_.get());
		std::fseek(output_.get(), curPos, SEEK_SET);
		return read == 1;
	}
	return false;
}

cdownload::DirectBinaryWriter::DirectBinaryWriter(const Types::Fields& fields, bool writeEpochColumn)
	: Writer(writeEpochColumn)
	, DirectDataWriter(fields, writeEpochColumn)
	, BinaryWriter(writeEpochColumn, stride(fields, writeEpochColumn, false))
{
}

void cdownload::AveragedDataBinaryWriter::write(std::size_t cellNumber, const datetime& dt,
		                   const AveragedTypes::Data& averagedCells,
		                   const RawTypes::Data& rawCells)
{

	assert(averagedCells.size() == averagedFields().size());
	assert(rawCells.size() == rawFields().size());

	std::fwrite(&cellNumber, sizeof(std::size_t), 1, outputStream());

	const auto dtSeconds = dt.seconds();

	std::fwrite(&dtSeconds, sizeof(dtSeconds), 1, outputStream());

	if (writeEpochColumn()) {
		const auto epoch = dt.milliseconds();
		std::fwrite(&epoch, sizeof(epoch), 1, outputStream());
	}

	struct CellValues {
		double mean;
		std::size_t count;
		double stdDev;
	};

	for (std::size_t i = 0; i < averagedFields().size(); ++i) {
		const auto& fields = averagedFields()[i];
		const auto& cells = averagedCells[i];
		for (const Field& f: fields) {
			const AveragedVariable& av = f.data(cells);
			for (const AveragingRegister& ac: av) {
				CellValues cv {ac.mean(), ac.count(),  ac.stdDev()};
				std::fwrite(&cv, sizeof(cv), 1, outputStream());
			}
		}
	}

	for (std::size_t i = 0; i < rawFields().size(); ++i) {
		const auto& fields = rawFields()[i];
		const auto& cells = rawCells[i];
		for (const Field& f: fields) {
			const double* data = f.data<double>(cells);
			std::fwrite(data, sizeof(double), f.elementCount(), outputStream());
		}
	}
}

namespace {

	void printAveragedFieldHeader(std::ostream& os, const std::string& name, std::size_t elementsCount)
	{
		using cdownload::AveragingRegister;
		if (elementsCount == 1) {
			os << '\t' << name << ":mean <" << datatypenaming::NATIVE_REAL << "["
				<< sizeof(AveragingRegister::mean_value_type) * CHAR_BIT << "]>"
				<< '\t' << name << ":count <" << datatypenaming::NATIVE_UNSIGNED_INT<< "["
				<< sizeof(AveragingRegister::counter_type) * CHAR_BIT << "]>"
				<< '\t' << name << ":stddev <" << datatypenaming::NATIVE_REAL << "["
				<< sizeof(AveragingRegister::stddev_value_type) * CHAR_BIT << "]>";
		} else {
			for (std::size_t elem = 0; elem < elementsCount; ++elem) {
				os << '\t' << name << "___" << elem +1 << ":mean <" << datatypenaming::NATIVE_REAL << "["
					<< sizeof(AveragingRegister::mean_value_type) * CHAR_BIT << "]>"
					<< '\t' << name << "___" << elem +1 << ":count <" << datatypenaming::NATIVE_UNSIGNED_INT << "["
					<< sizeof(AveragingRegister::counter_type) * CHAR_BIT << "]>"
					<< '\t' << name << "___" << elem +1 << ":stddev <" << datatypenaming::NATIVE_REAL << "["
					<< sizeof(AveragingRegister::stddev_value_type) * CHAR_BIT << "]>";
			}
		}
	}

	void printRawFieldHeader(std::ostream& os, const std::string& name, std::size_t elementsCount)
	{
		using cdownload::AveragingRegister;
		if (elementsCount == 1) {
			os << '\t' << name << datatypenaming::NATIVE_REAL << "["
				<< sizeof(AveragingRegister::mean_value_type) * CHAR_BIT << "]>";
		} else {
			for (std::size_t elem = 0; elem < elementsCount; ++elem) {
				os << '\t' << name << elem +1 << datatypenaming::NATIVE_REAL << "["
					<< sizeof(AveragingRegister::mean_value_type) * CHAR_BIT << "]>";
			}
		}
	}
}

void cdownload::AveragedDataBinaryWriter::writeHeader()
{
	string headerFileName = outputFileName().string() + ".hdr";

	std::ofstream headerFile(headerFileName.c_str());
	// writing a header line
	headerFile << "CellNo <[" <<
		datatypenaming::NATIVE_UNSIGNED_INT << sizeof(std::size_t) * CHAR_BIT << "]>\t" <<
		"MidTime <[" <<
		datatypenaming::NATIVE_REAL << sizeof(decltype(timeduration().seconds())) * CHAR_BIT << "]>";
	if (writeEpochColumn()) {
		headerFile << "Epoch <[" <<
		datatypenaming::NATIVE_REAL << sizeof(decltype(timeduration().milliseconds())) * CHAR_BIT << "]>";

	}
	for (const auto& fields: averagedFields()) {
		for (const FieldDesc& f: fields) {
			printAveragedFieldHeader(headerFile, f.name().name(), f.elementCount());
		}
	}

	for (const auto& fields: rawFields()) {
		for (const FieldDesc& f: fields) {
			printRawFieldHeader(headerFile, f.name().name(), f.elementCount());
		}
	}
	headerFile << std::endl;
}


void cdownload::DirectBinaryWriter::writeHeader()
{
	string headerFileName = outputFileName().string() + ".hdr";

	std::ofstream headerFile(headerFileName.c_str());
	// writing a header line
	headerFile << "CellNo <[" <<
		datatypenaming::NATIVE_UNSIGNED_INT << sizeof(std::size_t) * CHAR_BIT << "]>\t" <<
		"MidTimeEpoch <[" <<
		datatypenaming::NATIVE_REAL << sizeof(decltype(datetime().seconds())) * CHAR_BIT << "]>";
	if (writeEpochColumn()) {
		headerFile << "Epoch <[" <<
		datatypenaming::NATIVE_REAL << sizeof(decltype(timeduration().milliseconds())) * CHAR_BIT << "]>";

	}
	for (const auto& fieldArray: fields()) {
		for (const FieldDesc& f: fieldArray) {
			printRawFieldHeader(headerFile, f.name().name(), f.elementCount());
		}
	}
	headerFile << std::endl;
}

cdownload::AveragedDataBinaryWriter::AveragedDataBinaryWriter(const AveragedTypes::Fields& averagedFields,
		                   const RawTypes::Fields& rawFields,
		                   bool writeEpochColumn)
	: Writer(writeEpochColumn)
	, AveragedDataWriter(averagedFields, rawFields, writeEpochColumn)
	, BinaryWriter(writeEpochColumn, stride(averagedFields, writeEpochColumn, true) + stride(rawFields, false, false))
{
	// we support only double values so far
	for (const auto& fields: averagedFields) {
		for (const Field& f: fields) {
			if (f.dataType() != FieldDesc::DataType::Real ||
				f.dataSize() != sizeof(double)) {
				throw std::runtime_error("Only double columns are supported");
			}
		}
	}
}

void cdownload::DirectBinaryWriter::write(std::size_t cellNumber, const datetime& dt, const Types::Data& lines)
{
	assert(lines.size() == fields().size());
	std::fwrite(&cellNumber, sizeof(std::size_t), 1, outputStream());

	const auto dtSeconds = dt.seconds();

	std::fwrite(&dtSeconds, sizeof(dtSeconds), 1, outputStream());

	if (writeEpochColumn()) {
		const auto epoch = dt.milliseconds();
		std::fwrite(&epoch, sizeof(epoch), 1, outputStream());
	}

	for (std::size_t i = 0; i < lines.size(); ++i) {
		const auto& fieldsArray = fields()[i];
		const auto& line = lines[i];
		for (const Field& f: fieldsArray) {
			const double* data = f.data<double>(line);
			std::fwrite(data, sizeof(double), f.elementCount(), outputStream());
		}
	}
}
