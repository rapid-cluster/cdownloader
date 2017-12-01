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

#ifndef CDOWNLOAD_OMNI_HRO_READER_HXX
#define CDOWNLOAD_OMNI_HRO_READER_HXX

#include "../reader.hxx"
#include "../field.hxx"

#include <iosfwd>
#include <memory>

namespace cdownload {
namespace omni {

	class OmniTableDesc;

	class Reader: public cdownload::Reader {
		using base = cdownload::Reader;
	public:
		Reader(const path& hroFileName, const std::vector<ProductName>& variables);

		bool readRecord(std::size_t index, bool omitTimestamp) override;
		bool readTimeStampRecord(std::size_t index) override;
		bool eof() const override;

		std::size_t findTimestamp(double timeStamp, std::size_t startIndex) override;

		static ProductName epochFieldName;
	private:

		Reader(const path& hroFileName, const std::vector<ProductName>& variables, const OmniTableDesc& omniFileDesc,
			   std::vector<Field>&& requestedVariables, std::vector<std::size_t>&& indicies);
		struct Line {
			double epoch;
			std::vector<std::string> fields;
		};

		void readNextLine();

		template <class T>
		void assignField(std::size_t index);

		path fileName_;
		std::unique_ptr<std::ifstream> input_;
		std::vector<std::size_t> indicies_;
		std::vector<Field> requestedVariables_;

		Line currentLine_;
		std::size_t currentLineIndex_;
	};
}
}

#endif // CDOWNLOAD_OMNI_HRO_READER_HXX
