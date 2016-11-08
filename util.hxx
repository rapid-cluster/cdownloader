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
#include <map>
#include <vector>
#include <iosfwd>
#include <type_traits>

namespace cdownload {


	datetime makeDateTime(unsigned year, unsigned month, unsigned day, unsigned hours = 0, unsigned minutes = 0, double seconds = 0.);

	class ProductName {
		static constexpr const char* delimiter = "__";

	public:
		ProductName();
		ProductName(const std::string& name);

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

	private:
		std::string variableName_; //!< full, including dataset name
		DatasetName datasetName_;
	};


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

	std::ostream& operator<<(std::ostream& os, const ProductName& pr);

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


namespace csa_time_formatting {
	std::ostream& operator<<(std::ostream& os, const datetime& dt);
	std::istream& operator>>(std::istream& is, datetime& dt);
}

	std::vector<std::string>
	expandWildcardsCaseSensitive(const std::vector<std::string>& wildcards,
	                             const std::vector<std::string>& avaliable);
	std::vector<ProductName>
	expandWildcardsCaseSensitive(const std::vector<ProductName>& wildcards,
	                             const std::vector<ProductName>& avaliable);
}

#endif // CDOWNLOADER_UTIL_HXX
