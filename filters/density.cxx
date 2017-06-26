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

#include "density.hxx"

cdownload::Filters::H1DensityFilter::H1DensityFilter(const cdownload::ProductName& densityProduct, double minDensity)
	: base("H1Density", 1)
	, minDensity_{minDensity}
	, H1density_(addField(densityProduct.name()))
{
}

bool cdownload::Filters::H1DensityFilter::test(const std::vector<AveragedVariable>& line,
                                               std::vector<void*>& /*variables*/) const
{
	if (!enabled()) {
		return true;
	}

	if (H1density_.data(line)[0].mean() < minDensity_) {
		return false;
	}
	return true;
}

