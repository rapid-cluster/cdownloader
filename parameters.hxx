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
		const std::vector<ProductName> productsForDatasetOrDefault(const std::string& dataset,
																   const std::vector<ProductName>& defaultValue) const;

	private:
		std::string name_;
		Format format_;
		DatasetProductsMap products_;
	};

	Output parseOutputDefinitionFile(const path& filePath);

	struct QualityFilterParameters {
		ProductName product;
		int minQuality;
	};

	enum class DensitySource {
		CODIF,
		HIA
	};

	struct DensityFilterParameters {
		DensitySource source;
		double minDensity;
	};
	/**
	 * @brief Encapsulates all settings related to outputs
	 *
	 */
	class Parameters {
	public:

		Parameters(const path& outputDir = path("/tmp"), const path& workDir = path("/tmp"),
		           const path& cacheDir = path());

		void setTimeRange(const datetime& startDate, const datetime& endDate);
		void setTimeInterval(const timeduration& interval);
		void setExpansionDictFile(const path& fileName);
		void setContinueMode(bool continueDownloading);
		void setDownloadMissingData(bool download);

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

		const path& cacheDir() const
		{
			return cacheDir_;
		}

		std::vector<std::string> allDatasetNames() const;

		bool continueDownloading() const {
			return continue_;
		}

		bool downloadMissingData() const {
			return downloadMissingData_;
		}

		// optional filters
		const std::vector<QualityFilterParameters>& qualityFilters() const {
			return qualityFilters_;
		}
		void addQualityFilter(const ProductName& product, int minQuality);

		const std::vector<DensityFilterParameters>& densityyFilters() const {
			return densityFilters_;
		}
		void addDensityFilter(DensitySource source, double minDensity);

		bool onlyNightSide() const {
			return onlyNightSide_;
		}

		void onlyNightSide(bool v);

		const path& timeRangesFileName() const {
			return timeRangesFileName_;
		}

		void timeRangesFileName(const path& v);

		bool allowBlanks() const {
			return allowBlanks_;
		}

		void allowBlanks(bool v);

		bool disableAveraging() const {
			return disableAveraging_;
		}
		void disableAveraging(bool v);

		bool writeEpoch() const {
			return writeEpoch_;
		}
		void writeEpoch(bool v);

		bool plasmaSheetFilter() const {
			return plasmaSheetFilter_;
		}
		void plasmaSheetFilter(bool v);

		double plasmaSheetMinR() const {
			return plasmaSheetMinR_;
		}
		void plasmaSheetMinR(double v);
	private:
		datetime startDate_;
		datetime endDate_;
		timeduration timeInterval_;
		std::vector<Output> outputs_;
		path expansionDictionaryFile_;
		path outputDir_;
		path workDir_;
		bool continue_ = false;
		path cacheDir_;
		bool downloadMissingData_ = true;
		std::vector<QualityFilterParameters> qualityFilters_;
		std::vector<DensityFilterParameters> densityFilters_;
		bool onlyNightSide_ = false;
		path timeRangesFileName_;
		bool allowBlanks_ = false;
		bool disableAveraging_ = false;
		bool writeEpoch_ = false;
		bool plasmaSheetFilter_ = true;
		double plasmaSheetMinR_;
	};

	std::ostream& operator<<(std::ostream& os, const Parameters& p);
	std::ostream& operator<<(std::ostream& os, const Output& o);
	std::ostream& operator<<(std::ostream& os, const QualityFilterParameters& p);
	std::ostream& operator<<(std::ostream& os, const DensityFilterParameters& p);
	std::ostream& operator<<(std::ostream& os, DensitySource ds);
}

#endif // CDOWNLOAD_PARAMETERS_H
