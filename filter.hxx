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

#ifndef CLUSTER_CDOWNLOADER_FILTER_HXX
#define CLUSTER_CDOWNLOADER_FILTER_HXX

#include "field.hxx"
#include "average.hxx"

#include <cassert>
#include <vector>

namespace cdownload {

	/**
	 * @brief Basic data filer interface.
	 *
	 * A filter provides list of required products for its work, accepts list of avaliable
	 * products.
	 *
	 */
	class Filter {
	public:
		virtual ~Filter() = default;
		std::vector<ProductName> requiredProducts() const;
		virtual void initialize(const std::vector<Field>& availableProducts,
		                        const std::vector<Field>& filterVariables);

		const std::string name() const {
			return name_;
		}

		static std::pair<std::string, std::string> splitParameterName(const std::string& name);
		static ProductName composeProductName(const std::string& shortName, const std::string& filterName);
		virtual std::vector<FieldDesc> variables() const;

		static bool productBelongsToFilter(const ProductName& pr, const string& filterName);

		bool enabled() const {
			return enabled_;
		}

		void enable(bool b);
	protected:
		Filter(const std::string& name, std::size_t maxFieldsCount = 0, std::size_t maxVariablesCount = 0);

		const Field& addField(const std::string& productName);
		const Field& addVariable(const FieldDesc& desc, std::size_t* index);
		const Field& field(const std::string& name);
		const std::vector<Field>& availableProducts() const {
			return availableProducts_;
		}

		static std::string composeParameterName(const std::string& shortName, const std::string& filterName);
		ProductName composeProductName(const std::string& shortName) const;

		bool isVariableEnabled(std::size_t index) const {
			assert(index < enabledVariables_.size());
			return enabledVariables_[index];
		}
	private:

		static const DatasetName FAKE_FILTER_DATASET;
		bool enabled_;

		std::vector<Field> requiredFields_; //! for requiredProducts()
		std::vector<Field> availableProducts_;

		std::vector<Field> availableVariables_;
		std::vector<bool> enabledVariables_;

		std::size_t maxFieldsCount_;
		std::size_t maxVariablesCount_;
		std::string name_;
	};

	/**
	 * @brief Provides interface for filtering raw data, i.e. a buffer used by data reader
	 *
	 */
	class RawDataFilter: public Filter {
	public:
		virtual bool test(const std::vector<const void*>& line, const DatasetName& ds, std::vector<void*>& variables) const = 0;
	protected:
		RawDataFilter(const std::string& name, std::size_t maxFieldsCount = 0, std::size_t maxVariablesCount = 0);
	};

	/**
	 * @brief Provides interface for filtering averaged data, i.e. a list of averaging registers
	 *
	 */
	class AveragedDataFilter: public Filter {
	public:
		virtual bool test(const std::vector<AveragedVariable>& line, std::vector<void*>& variables) const = 0;
	protected:
		AveragedDataFilter(const std::string& name, std::size_t maxFieldsCount = 0, std::size_t maxVariablesCount = 0);
	};
}

#endif // CLUSTER_CDOWNLOADER_FILTER_HXX
