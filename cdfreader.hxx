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

#ifndef CDOWNLOAD_CDFREADER_HXX
#define CDOWNLOAD_CDFREADER_HXX

#include "util.hxx"
#include "field.hxx"

#include <iosfwd>
#include <iterator>
#include <memory>

namespace cdownload {
	/**
	 * @brief Contains tools for parsing and reading CDF files, using the CDF library
	 *
	 */
namespace CDF {

	enum class DataType: long {
		INT1        = 1L,
		INT2        = 2L,
		INT4        = 4L,
		INT8        = 8L,
		UINT1       = 11L,
		UINT2       = 12L,
		UINT4       = 14L,
		REAL4       = 21L,
		REAL8       = 22L,
		EPOCH       = 31L,  /* Standard style. */
		EPOCH16     = 32L,  /* Extended style. */
		TIME_TT2000     = 33L,  /* One more style with leap seconds
			                       and J2000 base time. */
		BYTE        = 41L,     /* same as CDF_INT1 (signed) */
		FLOAT       = 44L,     /* same as CDF_REAL4 */
		DOUBLE      = 45L,     /* same as CDF_REAL8 */
		CHAR        = 51L,     /* a "string" data type */
		UCHAR       = 52L     /* a "string" data type */
	};

	class File;

	class Variable {
	public:
		std::string name() const;
		bool isZVariable() const;
		DataType datatype() const;
		std::vector<std::size_t> dimension() const;

		std::size_t elementsCount() const;

		std::size_t recordSize() const; //! Just for simplicity: takes datatype and dimensions to
		                                //! compute total record length

		std::size_t read(void* dest, std::size_t startIndex, std::size_t numRecords) const;

		std::size_t recordsCount() const {
			return recordsCount_;
		}

		double fillValue() const {
			return fillValue_;
		}
	private:
		Variable(File* file, std::size_t index, bool isRVar);
		static std::size_t datatypeSize(DataType dt);

		void setFile(File* file);

		friend class File;
		File* file_;
		bool isRVar_;
		std::size_t index_;
		std::size_t recordsCount_;
		double fillValue_;
	};

	class File {
	public:
		File(const path& fileName);
		~File();

		File(const File& other);
		File(const File&& other);

		std::size_t variablesCount() const;
		const Variable& variable(std::size_t num) const;
		const Variable& variable(const std::string& name) const;
		std::size_t variableIndex(const std::string& name) const;

	private:
		void* cdfId() const;
		friend class Variable;
		class Impl;
		std::shared_ptr<Impl> impl_;
	};


	class Info {
	public:
		Info();
		Info(const File& file);
		std::vector<FieldDesc> variables() const;
		std::vector<ProductName> variableNames() const;
		ProductName timestampVariableName() const;
		const FieldDesc& variable(const std::string& name) const;

	private:
		static FieldDesc describeVariable(const Variable& v);
		std::vector<FieldDesc> variables_;
		ProductName timestampVariableName_;
	};

	class VariableMetaPrinter {
	public:
		VariableMetaPrinter(const Variable& v, std::size_t identLevel = 0);
		void print(std::ostream& os) const;

	private:
		static std::string dataTypeToString(DataType dt);
		const Variable& variable_;
		std::size_t identLevel_;
	};

	inline std::ostream& operator<<(std::ostream& os, const VariableMetaPrinter& vmp)
	{
		vmp.print(os);
		return os;
	}

	class Reader {
	public:
		Reader(const File& f, const std::vector<ProductName>& variables);
//      Reader(const Reader&);

//      Reader& operator=(const Reader&);

		bool readRecord(std::size_t index, bool omitTimestamp = false);
		bool readTimeStampRecord(std::size_t index);

		const void* bufferForVariable(std::size_t variableIndex) const;

		bool eof() const
		{
			return eof_;
		}

		std::size_t findTimestamp(double timeStamp, std::size_t startIndex);

	private:
		using BufferPtr = std::unique_ptr<char[]>;
		std::vector<const Variable*> variables_;
		std::vector<BufferPtr> buffers_; // a buffer for each variable
		File file_;
		Info info_;
		bool eof_;
		std::size_t timeStampVariableIndex_;
	};

	template <class T>
	class ConstVariableView;

	template <class T>
	class VariableConstIterator {
	public:
		typedef std::random_access_iterator_tag iterator_category;
		typedef T value_type;
		typedef std::ptrdiff_t difference_type;
		typedef const T*   pointer;
		typedef T& reference;

		typedef VariableConstIterator<T> this_type;

		value_type operator*() const
		{
			return value_;
		}

		operator bool() const {
			return isIndexValid(currentIndex_);
		}

		this_type& operator++()
		{
			moveTo(currentIndex_ + 1); return *this;
		}

		this_type operator++(int)
		{
			auto temp = *this; moveTo(currentIndex_ + 1); return temp;
		}

		this_type& operator--()
		{
			moveTo(currentIndex_ - 1); return *this;
		}

		this_type operator--(int)
		{
			auto temp = *this;
			moveTo(currentIndex_ + 1);
			return temp;
		}

		this_type& operator+=(const ptrdiff_t& movement)
		{
			moveTo(currentIndex_ + movement);
			return (*this);
		}

		this_type& operator-=(const ptrdiff_t& movement)
		{
			moveTo(currentIndex_ - movement);
			return (*this);
		}

		this_type operator+(const ptrdiff_t& movement)
		{
			return this_type(var_, currentIndex_ + movement);
		}

		this_type operator-(const ptrdiff_t& movement)
		{
			return this_type(var_, currentIndex_ - movement);
		}

		std::ptrdiff_t operator-(const this_type& other)
		{
			assert(other.var_ == this->var_);
			return currentIndex_ - other.currentIndex_;
		}

		bool operator==(const this_type& other) const {
			return this->var_ == other.var_ &&
				this->currentIndex_ == other.currentIndex_;
		}

		bool operator!=(const this_type& other) const {
			return !(*this == other);
		}

	private:
		explicit VariableConstIterator(const Variable* var)
			: VariableConstIterator(var, 0)
		{
		}
		explicit VariableConstIterator(const Variable* var, std::ptrdiff_t index, bool onlySetIndex = false)
			: var_{var} {
			if (onlySetIndex) {
				currentIndex_ = index;
			} else {
				moveTo(index);
			}
		}

		void swap(this_type& other) {
			std::swap(var_, other.var_);
		}

		friend class ConstVariableView<T>;

		void moveTo(std::ptrdiff_t index)
		{
			if (isIndexValid(index)) {
				var_->read(&value_, static_cast<std::size_t>(index), 1);
				currentIndex_ = index;
			}
		}

		bool isIndexValid(std::ptrdiff_t index) const
		{
			return (index >= 0) && (static_cast<std::size_t>(index) < var_->recordsCount());
		}

		const Variable* var_;
		std::ptrdiff_t currentIndex_;
		T value_;
	};

	template <class T>
	class ConstVariableView {
	public:
		typedef VariableConstIterator<T> const_iterator;
		ConstVariableView(const Variable* var)
			: var_{var} {
		}

		VariableConstIterator<T> begin() const {
			return VariableConstIterator<T>(var_, 0);
		}

		VariableConstIterator<T> end() const {
			return VariableConstIterator<T>(var_, static_cast<std::ptrdiff_t>(var_->recordsCount()), true);
		}

	private:
		const Variable* var_;
	};
}
}

#endif // CDOWNLOAD_CDFREADER_HXX
