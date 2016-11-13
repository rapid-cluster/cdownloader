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

#ifndef CDOWNLOAD_PARAMETERS_H
#define CDOWNLOAD_PARAMETERS_H

#include "util.hxx"

#include <iosfwd>
#include <map>

namespace cdownload {

	//! Stores configuration of an output file, i.e. name and list of products
	class Output {
	public:
		enum class Format {
			ASCII,
			Binary,
			CDF
		};

		Output(const std::string& name, Format format, const std::vector<ProductName>& products);
		Output(const std::string& name, Format format, const DatasetProductsMap& products);

		const std::string& name() const
		{
			return name_;
		}

		Format format() const
		{
			return format_;
		}

		const DatasetProductsMap& products() const
		{
			return products_;
		}

		void setProductsList(const std::vector<ProductName>& products)
		{
			products_ = parseProductsList(products);
		}

		std::vector<std::string> datasetNames() const;

		const std::vector<ProductName>& productsForDataset(const std::string& dataset) const;

	private:
		std::string name_;
		Format format_;
		DatasetProductsMap products_;
	};

	Output parseOutputDefinitionFile(const path& filePath);

	/**
	 * @brief Encapsulates all settings related to outputs
	 *
	 */
	class Parameters {
	public:

		Parameters(const path& outputDir = path("/tmp"), const path& workDir = path("/tmp"));

		void setTimeRange(const datetime& startDate, const datetime& endDate);
		void setTimeInterval(const timeduration& interval);
		void setExpansionDictFile(const path& fileName);
		void setContinueMode(bool continueDownloading);

		const datetime& startDate() const
		{
			return startDate_;
		}

		const datetime& endDate() const
		{
			return endDate_;
		}

		const timeduration timeInterval() const
		{
			return timeInterval_;
		}

		const path& expansionDictFile() const
		{
			return expansionDictionaryFile_;
		}

		void addOuput(const Output& output)
		{
			outputs_.push_back(output);
		}

		const std::vector<Output>& outputs() const
		{
			return outputs_;
		}

		const path& outputDir() const
		{
			return outputDir_;
		}

		const path& workDir() const
		{
			return workDir_;
		}

		std::vector<std::string> allDatasetNames() const;

		bool continueDownloading() const {
			return continue_;
		}

	private:
		datetime startDate_;
		datetime endDate_;
		timeduration timeInterval_;
		std::vector<Output> outputs_;
		path expansionDictionaryFile_;
		path outputDir_;
		path workDir_;
		bool continue_;
	};

	std::ostream& operator<<(std::ostream& os, const Parameters& p);
}

#endif // CDOWNLOAD_PARAMETERS_H
