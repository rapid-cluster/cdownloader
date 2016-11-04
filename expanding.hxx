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

#ifndef CDOWNLOAD_EXPANDING_HXX
#define CDOWNLOAD_EXPANDING_HXX

#include <memory>
#include <vector>

namespace cdownload {

	class Expander {
	public:
		Expander();
		virtual ~Expander() = default;

		void setSubExpander(std::unique_ptr<Expander> subExpander);
		void setDimensions(const std::vector<std::size_t>& dimensions);

		std::size_t expandedSizeShallow() const;
		std::size_t expandedSize() const;
		std::vector<std::string> expandNames(const std::vector<std::string>& names) const;

	private:
		virtual std::vector<std::string> expandName(const std::string& name) const;
		virtual void expandValue(const void* val, std::vector<double>& expanded, std::size_t startIndex) const;
	};
}
#endif // CDOWNLOAD_EXPANDING_HXX
