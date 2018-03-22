/*
 * cdownload lib: downloads data from the Cluster CSA arhive
 * Copyright (C) 2016  Eugene Shalygin <eugene.shalygin@gmail.com>
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

#ifndef CDOWNLOAD_DRIVER_H
#define CDOWNLOAD_DRIVER_H

#include "parameters.hxx"
#include "csa/unpacker.hxx"

#include <map>
#include <memory>

namespace cdownload {
namespace CDF {
	class Info;
}

namespace csa {
	class DataDownloader;
}

	class FieldDesc;
	class Output;
	class Writer;
	class Field;

	class Filter;
	class AveragedDataFilter;
	class RawDataFilter;

	/**
	 * @brief Main class for data downloader which drives interaction between downloader,
	 * data reader, filters and output writers
	 *
	 */
	class Driver {
	public:
		Driver(const Parameters& params);
		void doTask();

		void listDatasets();
		void listProducts(const DatasetName& dataset);

	private:
		void createFilters(std::vector<std::shared_ptr<RawDataFilter> >& rawDataFilters,
		                   std::vector<std::shared_ptr<AveragedDataFilter> >& averagedDataFilters);

		void addBlankDataFilters(const std::vector<Field>& fields, std::vector<std::shared_ptr<RawDataFilter> >& rawDataFilters);

		void initializeFilters(const std::vector<Field>& fields, const std::vector<Field>& filterVariables,
		                       std::vector<std::shared_ptr<RawDataFilter> >& rawDataFilters,
		                       std::vector<std::shared_ptr<AveragedDataFilter> >& averagedDataFilters);

		struct ProductsToRead {
			using ProductNameArray = std::vector<ProductName>;
			ProductNameArray productsToWrite;
			ProductNameArray productsForFiltersOnly;
			std::map<const Filter*, ProductNameArray> filterVariables;
			std::map<const Filter*, std::vector<FieldDesc>> filterFields;
		};

		static ProductsToRead collectAllProductsToRead(const std::map<cdownload::DatasetName, CDF::Info>& available,
		                                               const std::vector<Output>& outputs,
		                                               const std::vector<std::shared_ptr<Filter> >& filters);

		static std::vector<DatasetName> collectRequiredDatasets(const std::vector<Output>& outputs,
		                                                         const std::vector<std::shared_ptr<Filter> >& filters);

		std::unique_ptr<Writer> createWriterForOutput(const Output& output,
		                                      const std::vector<Field>& dataFields, const std::vector<Field>& filterVariables) const;

		Parameters params_;
		std::vector<DatasetName> datasetsToLoad_;
		std::vector<ProductName> productsToRead_;
		std::size_t numberOfProductsToWrite_; // products in productsToRead_ array after this number
		                                      // are for filters only, but not for writers
	};
}

#endif // CDOWNLOAD_DRIVER_H
