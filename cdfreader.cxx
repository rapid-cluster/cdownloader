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

#include "cdfreader.hxx"

#include <cdf.h>

#include <algorithm>
#include <stdexcept>
#include <iostream>
#include <numeric>

// #define TRACING_READING

#include <boost/log/trivial.hpp>

namespace cdownload {
namespace CDF {
	class File::Impl {
	public:
		~Impl();
		bool idIsValid_ = false;
		CDFid id_;
		std::size_t numRVars_ = 0;
		std::size_t numZVars_ = 0;
		std::vector<Variable> variables_;
	};
}
}

cdownload::CDF::File::Impl::~Impl()
{
	if (idIsValid_) {
		CDFclose(id_);
	}
}

namespace {
	void CDFStatusHandler(CDFstatus status)
	{
		char message[CDF_STATUSTEXT_LEN + 1];
		CDFgetStatusText (status, message);

		if (status < CDF_WARN) {
			BOOST_LOG_TRIVIAL(error) << "An error has occurred, halting...";
			throw std::runtime_error(message);
		} else if (status < CDF_OK) {
			BOOST_LOG_TRIVIAL(warning) << "Warning, function may not have completed as expected..." << std::endl
			                           << message;
		}/* else if (status > CDF_OK) {
		    BOOST_LOG_TRIVIAL(info) << "Function completed successfully, but be advised that..." << std::endl
		                            << message;
		    }*/
	}

	bool checkCDFStatus(CDFstatus status)
	{
		if (status != CDF_OK) {
			CDFStatusHandler(status);
			return false;
		}
		return true;
	}
}

cdownload::CDF::File::File(const path& fileName)
	: impl_ {new Impl()}
{
	if (checkCDFStatus(CDFopenCDF(fileName.c_str(), &impl_->id_))) {
		impl_->idIsValid_ = true;
	}

	long numRVars, numZVars;
	checkCDFStatus(CDFgetNumrVars(impl_->id_, &numRVars));
	checkCDFStatus(CDFgetNumzVars(impl_->id_, &numZVars));

	impl_->numRVars_ = static_cast<std::size_t>(numRVars);
	impl_->numZVars_ = static_cast<std::size_t>(numZVars);

	for (std::size_t rVarIndex = 0; rVarIndex < impl_->numRVars_; ++rVarIndex) {
		impl_->variables_.push_back(Variable(this, rVarIndex, true));
	}
	for (std::size_t zVarIndex = 0; zVarIndex < impl_->numZVars_; ++zVarIndex) {
		impl_->variables_.push_back(Variable(this, zVarIndex, false));
	}
}

cdownload::CDF::File::~File() = default;

cdownload::CDF::File::File(const cdownload::CDF::File& other)
	: impl_{other.impl_}
{
	for (auto& variable: impl_->variables_) {
		variable.setFile(this);
	}
}

cdownload::CDF::File::File(const cdownload::CDF::File&& other)
	: impl_{other.impl_}
{
	for (auto& variable: impl_->variables_) {
		variable.setFile(this);
	}
}

std::size_t cdownload::CDF::File::variablesCount() const
{
	return static_cast<std::size_t>(impl_->numRVars_ + impl_->numZVars_);
}

void* cdownload::CDF::File::cdfId() const
{
	return impl_->id_;
}

const cdownload::CDF::Variable& cdownload::CDF::File::variable(std::size_t num) const
{
	if (num < variablesCount()) {
		return impl_->variables_[num];
	} else {
		throw std::range_error("Variable number is out of range");
	}
}

std::size_t cdownload::CDF::File::variableIndex(const std::string& name) const
{
	const auto i = std::find_if(impl_->variables_.begin(), impl_->variables_.end(),
	                            [&name](const Variable& v) {return v.name() == name;});
	if (i != impl_->variables_.end()) {
		return static_cast<std::size_t>(std::distance(impl_->variables_.begin(), i));
	} else {
		throw std::range_error("Can not find variable named '" + name + "' in CDF file");
	}
}

const cdownload::CDF::Variable& cdownload::CDF::File::variable(const std::string& name) const
{
	return variable(variableIndex(name));
}

cdownload::CDF::Variable::Variable(cdownload::CDF::File* file, std::size_t index, bool isRVar)
	: file_{file}
	, isRVar_{isRVar}
	, index_{index}
{
	if (isRVar) {
		throw std::logic_error("R-variables are not supported yet");
	}

	long recNum;
	checkCDFStatus(CDFgetzVarMaxWrittenRecNum(file_->cdfId(), index_, &recNum));
	recordsCount_ = static_cast<std::size_t>(recNum) + 1;

	long allocated;
	checkCDFStatus(CDFgetzVarMaxAllocRecNum(file_->cdfId(), index_, &allocated));
	assert(allocated == recNum);
}

void cdownload::CDF::Variable::setFile(cdownload::CDF::File* file)
{
	file_ = file;
}

bool cdownload::CDF::Variable::isZVariable() const
{
	return !isRVar_;
}

cdownload::CDF::DataType cdownload::CDF::Variable::datatype() const
{
	if (isZVariable()) {
		long dt;
		checkCDFStatus(CDFgetzVarDataType(file_->cdfId(), index_, &dt));
		return static_cast<DataType>(dt);
	} else {
		throw std::logic_error("R-variables are not supported yet");
	}
}

std::vector<std::size_t> cdownload::CDF::Variable::dimension() const
{
	if (!isZVariable()) {
		throw std::logic_error("R-variables are not supported yet");
	}

	long numDims;
	checkCDFStatus(CDFgetzVarNumDims(file_->cdfId(), index_, &numDims));
	if (numDims == 0) {
		return {1};
	}

	long dimSizes[CDF_MAX_DIMS];
	checkCDFStatus(CDFgetzVarDimSizes(file_->cdfId(), index_, dimSizes));
	std::vector<std::size_t> res;
	for (std::size_t i = 0; i < static_cast<std::size_t>(numDims); ++i) {
		res.push_back(static_cast<std::size_t>(dimSizes[i]));
	}
	return res;
}

std::string cdownload::CDF::Variable::name() const
{
	if (!isZVariable()) {
		throw std::logic_error("R-variables are not supported yet");
	}

	char varName[CDF_VAR_NAME_LEN256];
	checkCDFStatus(CDFgetzVarName(file_->cdfId(), index_, varName));
	varName[CDF_VAR_NAME_LEN256 - 1] = 0;
	return std::string(varName);
}

std::size_t cdownload::CDF::Variable::elementsCount() const
{
	auto dims = dimension();
	return std::accumulate(dims.begin(), dims.end(),
	                       static_cast<std::size_t>(1), std::multiplies<std::size_t>());
}

std::size_t cdownload::CDF::Variable::recordSize() const
{
	return elementsCount() * datatypeSize(datatype());
}

std::size_t cdownload::CDF::Variable::datatypeSize(cdownload::CDF::DataType dt)
{
	switch (dt) {
	case DataType::INT1:
		return 1;
	case DataType::INT2:
		return 2;
	case DataType::INT4:
		return 4;
	case DataType::INT8:
		return 8;
	case DataType::UINT1:
		return 1;
	case DataType::UINT2:
		return 2;
	case DataType::UINT4:
		return 4;
	case DataType::REAL4:
		return 4;
	case DataType::REAL8:
		return 8;
	case DataType::EPOCH:
		return 8;
	case DataType::EPOCH16:
		return 16;
	case DataType::TIME_TT2000:
		return 8;
	case DataType::BYTE:
		return 1;
	case DataType::FLOAT:
		return 4;
	case DataType::DOUBLE:
		return 8;
	case DataType::CHAR:
		return 1;
	case DataType::UCHAR:
		return 1;
	}
	throw std::logic_error("Unexpected DataType value");
}

std::size_t cdownload::CDF::Variable::read(void* dest, std::size_t startIndex, std::size_t numRecords) const
{
#if SOPHISITICATED_READ
	std::vector<std::size_t> dims = dimension();
	long recStart = static_cast<long>(startIndex);
	long recCount = static_cast<long>(numRecords);
	long recInterval = 1;
	std::vector<long> indices(dims.size(), 0l);
	std::vector<long> counts;
	std::copy(dims.begin(), dims.end(), std::back_inserter(counts));
	std::vector<long> intervals(dims.size(), 1l);
	checkCDFStatus(CDFhyperGetzVarData(file_->cdfId(), index_, recStart, recCount, recInterval,
	                                   &indices[0], &counts[0], &intervals[0], dest));
#else

	const std::size_t maxRecordIndex = std::min(recordsCount_, startIndex + numRecords) - 1;
	if (startIndex > maxRecordIndex) {
		return 0;
	}
	const std::size_t recordsToRead = maxRecordIndex - startIndex + 1;
	const std::size_t dataStride = this->recordSize();
#ifdef TRACING_READING
	BOOST_LOG_TRIVIAL(trace) << "Reading " << name() << "[" << startIndex << ':' << startIndex + recordsToRead << ']';
#endif
	assert(startIndex <= startIndex + recordsToRead);
	for (std::size_t i = 0; i < recordsToRead; ++i) {
		checkCDFStatus(CDFgetzVarRecordData(file_->cdfId(), static_cast<long>(index_), static_cast<long>(startIndex + i),
		                                    static_cast<char*>(dest) + i * dataStride));
	}
#endif
	return recordsToRead;
}

cdownload::CDF::VariableMetaPrinter::VariableMetaPrinter(const cdownload::CDF::Variable& v, std::size_t identLevel)
	: variable_(v)
	, identLevel_(identLevel)
{
}

void cdownload::CDF::VariableMetaPrinter::print(std::ostream& os) const
{
	std::string identString;
	for (std::size_t i = 0; i<identLevel_; ++i) {
		identString += '\t';
	}
	os
	    << identString << "Name: " << variable_.name() << std::endl
	    << identString << "Is Z-Variable: " << variable_.isZVariable() << std::endl
	    << identString << "Datatype: " << dataTypeToString(variable_.datatype())
	    << put_list(variable_.dimension());
}

std::string cdownload::CDF::VariableMetaPrinter::dataTypeToString(cdownload::CDF::DataType dt)
{
	switch (dt) {
	case DataType::INT1:
		return "INT1";
	case DataType::INT2:
		return "INT2";
	case DataType::INT4:
		return "INT4";
	case DataType::INT8:
		return "INT8";
	case DataType::UINT1:
		return "UINT1";
	case DataType::UINT2:
		return "UINT2";
	case DataType::UINT4:
		return "UINT4";
	case DataType::REAL4:
		return "REAL4";
	case DataType::REAL8:
		return "REAL8";
	case DataType::EPOCH:
		return "EPOCH";
	case DataType::EPOCH16:
		return "EPOCH16";
	case DataType::TIME_TT2000:
		return "TIME_TT2000";
	case DataType::BYTE:
		return "BYTE";
	case DataType::FLOAT:
		return "FLOAT";
	case DataType::DOUBLE:
		return "DOUBLE";
	case DataType::CHAR:
		return "CHAR";
	case DataType::UCHAR:
		return "UCHAR";
	}
	throw std::logic_error("Unexpected DataType value");
}

// ---------------------------------- CDFInfo ------------------------------------------------------

cdownload::CDF::Info::Info()
{
}

cdownload::CDF::Info::Info(const cdownload::CDF::File& file)
{
	for (std::size_t i = 0; i<file.variablesCount(); ++i) {
		Variable v = file.variable(i);
		variables_.push_back(describeVariable(v));
		if (v.datatype() == DataType::EPOCH ||
		    v.datatype() == DataType::EPOCH16) {
			if (timestampVariableName_.empty()) {
				timestampVariableName_ = v.name();
			} else {
				throw std::logic_error("Timestamp detection failed");
			}
		}
	}

	BOOST_LOG_TRIVIAL(trace) << "Loaded CDF Info. Variables: " << put_list(variables_);
}

cdownload::ProductName cdownload::CDF::Info::timestampVariableName() const
{
	return timestampVariableName_;
}

const cdownload::FieldDesc& cdownload::CDF::Info::variable(const std::string& name) const
{
	auto i = std::find_if(variables_.begin(), variables_.end(),
	                      [&name](const FieldDesc& f) {return f.name() == name;});
	if (i != variables_.end()) {
		return *i;
	} else {
		throw std::runtime_error("Can not find variable named '" + name + "' in the CDF info list");
	}
}

std::vector<cdownload::ProductName> cdownload::CDF::Info::variableNames() const
{
	std::vector<ProductName> res;
	std::transform(variables_.begin(), variables_.end(), std::back_inserter(res),
	               [](const FieldDesc& f) {return f.name();});

	return res;
}

std::vector<cdownload::FieldDesc> cdownload::CDF::Info::variables() const
{
	return variables_;
}

namespace {
	std::pair<cdownload::FieldDesc::DataType, int> cdfDatatypeToFieldDatatype(cdownload::CDF::DataType dt)
	{
		using DTS = cdownload::CDF::DataType;
		using DTR = cdownload::FieldDesc::DataType;

		switch (dt) {
		case DTS::INT1:
			return std::make_pair(DTR::SignedInt, 1);
		case DTS::INT2:
			return std::make_pair(DTR::SignedInt, 2);
		case DTS::INT4:
			return std::make_pair(DTR::SignedInt, 4);
		case DTS::INT8:
			return std::make_pair(DTR::SignedInt, 8);
		case DTS::UINT1:
			return std::make_pair(DTR::UnsignedInt, 1);
		case DTS::UINT2:
			return std::make_pair(DTR::UnsignedInt, 2);
		case DTS::UINT4:
			return std::make_pair(DTR::UnsignedInt, 4);
		case DTS::REAL4:
			return std::make_pair(DTR::Real, 4);
		case DTS::REAL8:
			return std::make_pair(DTR::Real, 8);
		case DTS::EPOCH:
			return std::make_pair(DTR::Real, 8);
		case DTS::EPOCH16:
			return std::make_pair(DTR::Real, 16);
		case DTS::TIME_TT2000:
			return std::make_pair(DTR::Real, 8);
		case DTS::BYTE:
			return std::make_pair(DTR::SignedInt, 1);
		case DTS::FLOAT:
			return std::make_pair(DTR::Real, 4);
		case DTS::DOUBLE:
			return std::make_pair(DTR::Real, 8);
		case DTS::CHAR:
			return std::make_pair(DTR::SignedInt, 1);
		case DTS::UCHAR:
			return std::make_pair(DTR::UnsignedInt, 1);
		}
		throw std::logic_error("Unexpected DataType value");
	}
}

cdownload::FieldDesc cdownload::CDF::Info::describeVariable(const cdownload::CDF::Variable& v)
{
	auto dataTypeAndSize = cdfDatatypeToFieldDatatype(v.datatype());
	return {v.name(), dataTypeAndSize.first, static_cast<std::size_t>(dataTypeAndSize.second),
			v.elementsCount()};
}

cdownload::CDF::Reader::Reader(const cdownload::CDF::File& f, const std::vector<ProductName>& variables)
	: variables_(variables.size())
	, buffers_(variables.size())
	, file_{f}
	, info_{file_}
	, eof_{false}
	, timeStampVariableIndex_{static_cast<std::size_t>(-1)}
{
	const auto timeStampVariableName = info_.timestampVariableName();
	for (std::size_t i = 0; i< variables.size(); ++i) {
		const ProductName& varName = variables[i];
		const FieldDesc fd = info_.variable(varName.name());
		const std::size_t requiredBufferSize = fd.dataSize() * fd.elementCount();
		buffers_[i] = std::unique_ptr<char[]>(new char[requiredBufferSize]);
		variables_[i] = &file_.variable(varName.name());
		if (timeStampVariableName == varName.name()) {
			timeStampVariableIndex_ = i;
		}
	}
	assert(timeStampVariableIndex_ != static_cast<std::size_t>(-1));
}

bool cdownload::CDF::Reader::readRecord(std::size_t index, bool omitTimeStamp)
{
	if (eof_) {
		return false;
	}
	std::size_t read = 0;
	for (std::size_t i = 0; i< variables_.size(); ++i) {
		if (omitTimeStamp && (timeStampVariableIndex_ != i)) {
			read += variables_[i]->read(buffers_[i].get(), index, 1);
		}
	}
	if (!read) {
		eof_ = true;
	}
	return read == (omitTimeStamp ? variables_.size() - 1 : variables_.size());
}

bool cdownload::CDF::Reader::readTimeStampRecord(std::size_t index)
{
	if (eof_) {
		return false;
	}

	return variables_[timeStampVariableIndex_]->read(buffers_[timeStampVariableIndex_].get(), index, 1);
}

const void* cdownload::CDF::Reader::bufferForVariable(std::size_t variableIndex) const
{
	return buffers_.at(variableIndex).get();
}

std::size_t cdownload::CDF::Reader::findTimestamp(double timeStamp, std::size_t startIndex)
{
	struct Finder {
		Finder(const Variable* v, double timeStamp)
			: var_{v}
			, timeStamp_{timeStamp}
		{
		}

		std::size_t operator()(std::size_t startIndex)
		{
			ConstVariableView<double> view (var_);
			auto begin = view.begin();
			auto end = view.end();
			auto iter = find(begin + static_cast<ptrdiff_t>(startIndex), end);
			if (iter == end) {
				iter = find(begin, end);
				if (iter == end) {
					throw std::runtime_error("No record for specified timestamp found");
				}
			}
			return static_cast<std::size_t>(std::distance(begin, iter));
		}

	private:
		typedef ConstVariableView<double>::const_iterator Iter;
		Iter find(Iter begin, Iter end)
		{
			auto iter = std::upper_bound(begin, end, timeStamp_);
			if (iter == end) {
				--iter;
				if (*iter < timeStamp_) {
					return iter;
				} else {
					return end;
				}
			} else {
				return --iter;
			}
		}

		const Variable* var_;
		double timeStamp_;
	};

	Finder finder(variables_[timeStampVariableIndex_], timeStamp);
	return finder(startIndex);
}
