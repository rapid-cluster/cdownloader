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

#include <chrono>
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
		virtual void initialize(const std::vector<Field>& availableProducts);

		const std::string name() const {
			return name_;
		}
	protected:
		Filter(const std::string& name, std::size_t maxFieldsCount = 0);

		const Field& addField(const std::string& productName);
		const Field& field(const std::string& name);
		const std::vector<Field>& availableProducts() const {
			return availableProducts_;
		}

	private:
		std::vector<Field> activeFields_; //! for requiredProducts()
		std::vector<Field> availableProducts_;
		std::size_t maxFieldsCount_;
		std::string name_;
	};

	/**
	 * @brief Provides interface for filtering raw data, i.e. a buffer used by data reader
	 *
	 */
	class RawDataFilter: public Filter {
	public:
		virtual bool test(const std::vector<const void*>& line, const DatasetName& ds) const = 0;
	protected:
		RawDataFilter(const std::string& name, std::size_t maxFieldsCount = 0);
	};

	/**
	 * @brief Provides interface for filtering averaged data, i.e. a list of averaging registers
	 *
	 */
	class AveragedDataFilter: public Filter {
	public:
		virtual bool test(const std::vector<AveragedVariable>& line) const = 0;
	protected:
		AveragedDataFilter(const std::string& name, std::size_t maxFieldsCount = 0);
	};
}

#endif // CLUSTER_CDOWNLOADER_FILTER_HXX
