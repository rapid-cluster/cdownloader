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

#ifndef CDOWNLOADER_UTIL_HXX
#define CDOWNLOADER_UTIL_HXX

#include "commonDefinitions.hxx"
#include <algorithm>
#include <map>
#include <vector>
#include <iosfwd>
#include <type_traits>

namespace cdownload {

	class ProductName {
		static constexpr const char* delimiter = "__";

	public:
		ProductName();
		ProductName(const std::string& name);
		ProductName(const DatasetName& datasetName, const std::string& shortVariableName);

		const DatasetName& dataset() const {
			return datasetName_;
		}

		const std::string& variable() const {
			return variableName_;
		}

		std::string name() const {
			return variable();
		}

		bool empty() const {
			return variableName_.empty();
		}


		std::string shortName() const; //!< without dataset name

		bool isPseudoDataset() const {
			return isPseudoDataset(datasetName_);
		}

		/**
		 * @brief Checks whether the @ref name points to a special (fake) dataset
		 *
		 * These fake datasets are used to get parameters from filters. A name has to star with '$'
		 * to be treated as special
		 * @param name name for testing
		 * @return Result of the test:
		 * @retval true if the name is special
		 * @retval false otherwise
		 */
		static bool isPseudoDataset(const DatasetName& name);

		/**
		 * @brief Create special dataset name out of a string
		 *
		 * Just prepends '$' to the name
		 */
		static DatasetName makePseudoDatasetName(const std::string& name);

	private:
		std::string variableName_; //!< full, including dataset name
		DatasetName datasetName_;
	};

	std::ostream& operator<<(std::ostream& os, const ProductName& product);
	std::istream& operator>>(std::istream& is, ProductName& product);

	using DatasetProductsMap = std::map<std::string, std::vector<ProductName> >;

	DatasetProductsMap parseProductsList(const std::vector<std::string>& products);
	DatasetProductsMap parseProductsList(const std::vector<ProductName>& products);

	std::vector<ProductName> expandProductsMap(const DatasetProductsMap& map);

	inline bool operator==(const ProductName& left, const ProductName& right) {
		return left.dataset() == right.dataset() &&
		       left.variable() == right.variable();
	}

	inline bool operator<(const ProductName& left, const ProductName& right) {
		if (left.dataset() < right.dataset()) {
			return true;
		}
		if (left.dataset() > right.dataset()) {
			return false;
		}
		return left.variable() < right.variable();
	}

	path homeDirectory();


	template <class ListStored, class ListArg, class Delim, class Bracket>
	class collection_printer {
	public:
		collection_printer(ListArg list, Delim delim, Bracket opening, Bracket closing)
			: list_(list)
			, delim_ {delim}
			, opening_{opening}
			, closing_{closing} {
		}

	private:
		template <class ListStoredType, class ListArgType, class DelimType, class BracketType>
		friend std::ostream& operator << (std::ostream&, const collection_printer<ListStoredType, ListArgType, DelimType, BracketType>&);
		ListStored list_;
		const Delim delim_;
		const Bracket opening_, closing_;
	};

	template <class ListStored, class ListArg, class Delim, class Bracket>
	std::ostream& operator<<(std::ostream& os, const collection_printer<ListStored, ListArg, Delim, Bracket>& list) {
		os << list.opening_;
		if (list.list_.begin() != list.list_.end()) {
			auto i = list.list_.begin();
			os << *i;
			++i;
			for (;i != list.list_.end(); ++i) {
				os << list.delim_ << *i;
			}
		}
		os << list.closing_;
		return os;
	}

	template <class List, class Delim, class Bracket>
	inline collection_printer<const List&, const List&, Delim, Bracket>
	put_list(const List& list, Delim delim, Bracket opening, Bracket closing) {
		return collection_printer<const List&, const List&, Delim, Bracket>(list, delim, opening, closing);
	}

	template <class List>
	inline collection_printer<const List&, const List&, char, char>
	put_list(const List& list, char delim = ',', char opening = '[', char closing = ']') {
		return collection_printer<const List&, const List&, char, char>(list, delim, opening, closing);
	}

	template <class List, class Delim, class Bracket, class = typename std::enable_if
            <
                !std::is_lvalue_reference<List>::value
            >::type>
	inline collection_printer<List, List&&, Delim, Bracket>
	put_list(List&& list, Delim delim, Bracket opening, Bracket closing) {
		return collection_printer<List, List&&, Delim, Bracket>(std::move(list), delim, opening, closing);
	}

	template <class List, class = typename std::enable_if
            <
                !std::is_lvalue_reference<List>::value
            >::type>
	inline collection_printer<List, List&&, char, char>
	put_list(List&& list, char delim = ',', char opening = '[', char closing = ']') {
		return collection_printer<List, List&&, char, char>(std::move(list), delim, opening, closing);
	}


	std::vector<std::string>
	expandWildcardsCaseSensitive(const std::vector<std::string>& wildcards,
	                             const std::vector<std::string>& avaliable);
	std::vector<ProductName>
	expandWildcardsCaseSensitive(const std::vector<ProductName>& wildcards,
	                             const std::vector<ProductName>& avaliable);


	template <class Container, class Value>
	bool contains(const Container& c, const Value& v) {
		return std::find(c.begin(), c.end(), v) != c.end();
	}

	template <class Container, class Predicate>
	bool contains_if(const Container& c, Predicate p) {
		return std::find_if(c.begin(), c.end(), p) != c.end();
	}
}

#endif // CDOWNLOADER_UTIL_HXX
