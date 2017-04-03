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
	namespace Filters {
		class TimeFilter;
	}

	/**
	 * @brief Synchronously reads all required datasets (synchronization by timestamps)
	 *
	 */
	class DataReader {
	public:
		virtual ~DataReader();
		DataReader(const datetime& startTime, const datetime& endTime,
		           const std::vector<std::shared_ptr<RawDataFilter> >& filters,
		           std::map<DatasetName, std::shared_ptr<DataSource>>& datasources,
		           const DatasetProductsMap& fieldsToRead,
		           const std::vector<Field>& fields, const Filters::TimeFilter* timeFilter);

		/*! \brief Reads next cell
		 *
		 * \returns pair of @true, <cell middle time> if cell was read successfully and
		 * pair of @false, <zero time> otherwise
		 */
		virtual std::pair<bool,datetime> readNextCell() = 0; //! returns @true if cell was read successfully
		bool eof() const;
		bool fail() const;
//      const void* buffer(const ProductName& var) const;

	protected:
		enum class CellReadStatus {
			OK,
			NoRecordSurviedFiltering,
			EoF,
			ReadError
		};

		enum class ReaderState {
			OK = 0,
			EoF = 1,
			Fail = 2
		};

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

		const datetime& startTime() const {
			return startTime_;
		}

		const datetime& endTime() const {
			return endTime_;
		}

		bool advanceDataSource(DataSetReadingContext& context);

		const Filters::TimeFilter* timeFilter() const {
			return timeFilter_;
		}

		std::vector<const void*>& bufferPointers() {
			return bufferPointers_;
		}

		std::vector<void*>& filterVariables() {
			return filterVariables_;
		}

		std::vector<Field>& fields() {
			return fields_;
		}

		std::map<DatasetName, DataSetReadingContext>& readers() {
			return readers_;
		}

		void setStateFlag(ReaderState flag, bool on = true);

	private:
		datetime startTime_;
		datetime endTime_;
		std::map<DatasetName, std::shared_ptr<DataSource>> datasources_;
		std::map<DatasetName, DataSetReadingContext> readers_;
		std::vector<std::shared_ptr<RawDataFilter> > filters_;
		std::vector<const void*> bufferPointers_; // this is for filters, i.e. without time_tags fields
		std::vector<void*> filterVariables_;
		std::vector<Field> fields_;

		ReaderState state_;
// 		datetime lastReadTimeStamp_;
		const Filters::TimeFilter* timeFilter_;
	};

	class AveragingDataReader: public DataReader {
		using base = DataReader;
	public:
		AveragingDataReader(const datetime& startTime, const datetime& endTime, timeduration cellLength,
		           const std::vector<std::shared_ptr<RawDataFilter> >& filters,
		           std::map<DatasetName, std::shared_ptr<DataSource>>& datasources,
		           const DatasetProductsMap& fieldsToRead,
		           std::vector<AveragedVariable>& cells,
		           const std::vector<Field>& fields, const Filters::TimeFilter* timeFilter);
		std::pair<bool,datetime> readNextCell() override;
	private:
		CellReadStatus readNextCell(const datetime& cellStart, DataSetReadingContext& ds);
		void copyValuesToAveragingCells(const DataSetReadingContext& ds);

		datetime currentStartTime_;
		timeduration cellLength_;
		std::vector<AveragedVariable>& cells_;
	};

	class DirectDataReader: public DataReader {
		using base = DataReader;
	public:
		DirectDataReader(const datetime& startTime, const datetime& endTime,
		           const std::vector<std::shared_ptr<RawDataFilter> >& filters,
		           std::map<DatasetName, std::shared_ptr<DataSource>>& datasources,
		           const DatasetProductsMap& fieldsToRead,
		           const std::vector<Field>& fields, const Filters::TimeFilter* timeFilter);
		std::pair<bool,datetime> readNextCell() override;
#if 0
		datetime cellMidTime() const {
			return datetime(dsContext_->lastReadTimeStamp);
		}
#endif
		using DataReader::bufferPointers;
	private:
		bool skipToTime(const datetime& time, DataSetReadingContext& ds);
		DataSetReadingContext* dsContext_;
	};
}

#endif // CDOWNLOAD_DATAREADER_HXX
