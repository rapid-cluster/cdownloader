

#ifndef CDOWNLOAD_READER_HXX
#define CDOWNLOAD_READER_HXX

#include "field.hxx"

#include <cstddef>
#include <functional>
#include <memory>
#include <vector>

namespace cdownload {
	class Reader {
	public:
		virtual ~Reader();
		virtual bool readRecord(std::size_t index, bool omitTimestamp = false) = 0;
		virtual bool readTimeStampRecord(std::size_t index) = 0;
		const void* bufferForVariable(std::size_t variableIndex) const {
			return buffers_[variableIndex].get();
		}
		virtual bool eof() const = 0;
		virtual std::size_t findTimestamp(double timeStamp, std::size_t startIndex) = 0;

	protected:
		struct FoundField {
			FieldDesc description;
			std::size_t fieldIndex;
		};

		using BufferPtr = std::unique_ptr<char[]>;

		using DescriptionForVariable = std::function<FoundField (const ProductName& variableName)>;
		using VariableCallback = std::function<void (std::size_t index, const ProductName& variableName, const FoundField& fd)>;
		Reader(const std::vector<ProductName>& variables, DescriptionForVariable descriptor, VariableCallback cb,
			   const ProductName& timeStampVariableName);

		std::vector<BufferPtr>& buffers() {
			return buffers_;
		}

		std::size_t timeStampVariableIndex() const {
			return timeStampVariableIndex_;
		}

		template <class T>
		void setFieldValue(const Field& f, T value) {
			f.setData(buffers_, value);
		}

		std::vector<BufferPtr> buffers_; // a buffer for each variable
		std::size_t timeStampVariableIndex_;
	};
}

#endif // CDOWNLOAD_READER_HXX
