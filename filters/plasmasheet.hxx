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

#ifndef CDOWNLOAD_FILTER_PLASMASHEET_HXX
#define CDOWNLOAD_FILTER_PLASMASHEET_HXX

#include "../filter.hxx"

namespace cdownload {
namespace Filters {

	class PlasmaSheetModeFilter: public RawDataFilter {
		using base = RawDataFilter;
	public:
		PlasmaSheetModeFilter();

		static string filterName();

	private:
		bool test(const std::vector<const void *> & line, const DatasetName& ds, std::vector<void*>& variables) const override;
		const Field& cis_mode_;
	};

	class PlasmaSheet : public AveragedDataFilter {
		using base = AveragedDataFilter;
	public:
		PlasmaSheet(double minR);

		static string filterName();
	private:
		bool test(const std::vector<AveragedVariable>& line, std::vector<void*>& variables) const override;

		double minR_;
		const Field& H1density_;
		const Field& H1T_;
		const Field& O1density_;
		const Field& O1T_;
		const Field& BMag_;
		const Field& sc_pos_xyz_gse_;
		// variables for export
		const Field& reportBeta_;
		const Field& reportPlasmaPressure_;
		const Field& reportMagneticPressure_;
		std::size_t reportBetaIndex_;
		std::size_t reportPlasmaPressureIndex_;
		std::size_t reportMagneticPressureIndex_;
	};
}
}

#endif // CDOWNLOAD_FILTER_PLASMASHEET_HXX
