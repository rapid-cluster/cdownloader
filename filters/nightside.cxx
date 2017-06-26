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

#include "./nightside.hxx"

cdownload::Filters::NightSide::NightSide()
	: base("NightSide", 1)
	, sc_pos_xyz_gse_(addField("sc_pos_xyz_gse__C4_CP_FGM_SPIN"))
{
}

bool cdownload::Filters::NightSide::test(const std::vector<const void *>& line, const DatasetName& /*ds*/,
                                         std::vector<void*>& /*variables*/) const
{
	if (!enabled()) {
		return true;
	}
	const double* pos = sc_pos_xyz_gse_.data<double>(line);
	if (pos[0] > 0) {
		return false;
	}

	return true;
}
