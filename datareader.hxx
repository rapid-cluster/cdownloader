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

#ifndef CDOWNLOAD_DATAREADER_HXX
#define CDOWNLOAD_DATAREADER_HXX

#include "average.hxx"
#include "cdfreader.hxx"
#include "filter.hxx"
#include <memory>

namespace cdownload {

	class DataSource;
	class Field;

	/**
	 * @brief Synchronously reads all required datasets (synchronization by timestamps)
	 *
	 */
	class DataReader {
	public:
		DataReader(const datetime& startTime, const datetime& endTime, timeduration cellLength,
		           const std::vector<std::shared_ptr<RawDataFilter> >& filters,
		           std::map<DatasetName, std::shared_ptr<DataSource>>& datasources,
		           const DatasetProductsMap& fieldsToRead,
		           std::vector<AveragedVariable>& cells,
		           const std::vector<Field>& fields);
		bool readNextCell(); //! returns @true if cell was read successfully
		bool eof() const;
		bool fail() const;
//      const void* buffer(const ProductName& var) const;

		datetime cellMidTime() const;

	private:

		struct DataSetReadingContext {
			DataSetReadingContext();
			DataSetReadingContext(const DatasetName& dataset, std::unique_ptr<CDF::Reader>&& reader,
				const std::vector<std::size_t>& indiciesInCells,
				std::size_t timestampVariableIndex,
				const std::vector<std::shared_ptr<RawDataFilter> >& filters,
				std::shared_ptr<DataSource> datasource,
				const std::vector<ProductName> variablesToReadFromDataset);
			DatasetName datasetName;
			std::unique_ptr<CDF::Reader> reader;
			std::vector<std::size_t> indiciesInCells;
			std::size_t readRecordsCount;
			std::size_t timestampVariableIndex;
			double lastReadTimeStamp;
// 			CDF::Info info;
			std::vector<std::shared_ptr<RawDataFilter> > filters;
			bool eof;
			std::shared_ptr<DataSource> datasource;
			std::vector<ProductName> variablesToReadFromDataset;
		};

		enum class CellReadStatus {
			OK,
			NoRecordSurviedFiltering,
			EoF,
			ReadError
		};
		CellReadStatus readNextCell(const datetime& cellStart, DataSetReadingContext& ds);
		void copyValuesToAveragingCells(const DataSetReadingContext& ds);
		bool advanceDataSource(DataSetReadingContext& context);

		datetime startTime_;
		datetime endTime_;
		timeduration cellLength_;
		std::map<DatasetName, std::shared_ptr<DataSource>> datasources_;
		std::map<DatasetName, DataSetReadingContext> readers_;
		std::vector<AveragedVariable>& cells_;
		std::vector<std::shared_ptr<RawDataFilter> > filters_;
		std::vector<const void*> bufferPointers_; // this is for filters, i.e. without time_tags fields
		std::vector<Field> fields_;

		bool fail_;
		bool eof_;
// 		datetime lastReadTimeStamp_;
	};
}

#endif // CDOWNLOAD_DATAREADER_HXX
