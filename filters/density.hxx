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
#ifndef CDOWNLOAAD_FILTER_DENSITY_HXX
#define CDOWNLOAAD_FILTER_DENSITY_HXX

#include "../filter.hxx"

namespace cdownload {
namespace Filters {
	class H1DensityFilter: public AveragedDataFilter {
		using base = AveragedDataFilter;
	public:
		H1DensityFilter(const ProductName& densityProduct, double minDensity);
	private:
		bool test(const std::vector<AveragedVariable>& line, std::vector<void*>& variables) const override;
		double minDensity_;
		const Field& H1density_;
	};
}
}

#endif // CDOWNLOAAD_FILTER_DENSITY_HXX
