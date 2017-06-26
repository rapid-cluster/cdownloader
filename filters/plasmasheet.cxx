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

/* letter from Lena dated 2016.09.23
 * here is the list of criteria for selection of the plasma sheet:
 *
 * 1. 0.2 <plasma beta< 10 (using pressure__C4_CP_CIS-CODIF_HS_H1_MOMENTS, to be more precise one
 * can use pressure__C4_CP_CIS-CODIF_HS_H1_MOMENTS+pressure__C4_CP_CIS-CODIF_HS_O1_MOMENTS because
 * oxygen also adds pressure, also quite small in the most cases;
 *
 * In case the pressure is a tensor one could calculate pressure using formula P=nk_bT and
 * products density__C4_CP_CIS-CODIF_HS_H1_MOMENTS, T__C4_CP_CIS-CODIF_HS_H1_MOMENTS and
 * density__C4_CP_CIS-CODIF_HS_O1_MOMENTS, T__C4_CP_CIS-CODIF_HS_O1_MOMENTS
 *
 * (I forgot which way I used and unfortunately cannot access computer at MPS right now.)
 *
 * 2. CIS_mode=13 (cis_mode__C4_CP_CIS_MODES --> cis_mode_key__C4_CP_CIS_MODES)
 *
 * CIS mode criteria is useful because when CIS_mode=13 this instrument works in the magnetospheric
 * mode. Therefore the spacecraft was very likely in the magnetosphere and not in the solar wind or
 * magnetosheath.
 *
 * 3. R>4RE (calculated from sc_pos_xyz_gse__C4_CP_FGM_SPIN and normed to the Earth' radius)
 * By this criteria we exclude hazardous radiation belts.
 *
 * 4. To be even cooler one could add (this is new for me): atan(Bx/Bz)<45 grad
 */

/* Eugene: the pressure__C4_CP_CIS-CODIF_HS_H1_MOMENTS is a tensor, indeed. Thus taking density and T
 * density__C4_CP_CIS-CODIF_HS_H1_MOMENTS is in cm^-3
 * T__C4_CP_CIS-CODIF_HS_H1_MOMENTS is in "MK" (seems to be micro Kelvin, but TODO check with Lena)
 * The same units are used in C4_CP_CIS-CODIF_HS_O1_MOMENTS dataset
 *
 * B_mag__C4_CP_FGM_SPIN is in nT TODO check with Lena that B_mag__C4_CP_FGM_SPIN is the correct quantity
 */

#include "plasmasheet.hxx"

#include <cmath>

cdownload::Filters::PlasmaSheetModeFilter::PlasmaSheetModeFilter()
	: base("PlasmaSheetMode", 1)
	, cis_mode_(addField("cis_mode__C4_CP_CIS_MODES"))
//  , cis_mode_key_(addField("cis_mode_key__C4_CP_CIS_MODES"))
{
}

bool cdownload::Filters::PlasmaSheetModeFilter::test(const std::vector<const void*>& line, const DatasetName& /*ds*/,
                                                     std::vector<void*>& /*variables*/) const
{
	if (!enabled()) {
		return true;
	}
	// CIS_mode is 13 or 8
	const int* cis_mode = cis_mode_.data<int>(line);
	if (*cis_mode != 13 && *cis_mode != 8) {
		return false;
	}

	return true;
}

cdownload::Filters::PlasmaSheet::PlasmaSheet()
	: base(filterName(), 7, 3)
	, H1density_(addField("density__C4_CP_CIS-CODIF_HS_H1_MOMENTS"))
	, H1T_(addField("T__C4_CP_CIS-CODIF_HS_H1_MOMENTS"))
	, O1density_(addField("density__C4_CP_CIS-CODIF_HS_O1_MOMENTS"))
	, O1T_(addField("T__C4_CP_CIS-CODIF_HS_O1_MOMENTS"))
	, BMag_(addField("B_mag__C4_CP_FGM_SPIN"))
	, sc_pos_xyz_gse_(addField("sc_pos_xyz_gse__C4_CP_FGM_SPIN"))
	, reportBeta_{addVariable(FieldDesc(composeProductName("beta"), -1., FieldDesc::DataType::Real, sizeof(double), 1), &reportBetaIndex_)}
	, reportPlasmaPressure_{addVariable(
		FieldDesc(composeProductName("PlasmaPressure"), -1., FieldDesc::DataType::Real, sizeof(double), 1),
		&reportPlasmaPressureIndex_)}
	, reportMagneticPressure_{addVariable(
		FieldDesc(composeProductName("MagneticPressure"), -1., FieldDesc::DataType::Real, sizeof(double), 1),
		&reportMagneticPressureIndex_)}
{
}

cdownload::string cdownload::Filters::PlasmaSheet::filterName()
{
	return "PlasmaSheet";
}


namespace {
	template <class T>
	inline constexpr T sqr(T v)
	{
		return v * v;
	}

	// returns pressure in SI, takes density and T directly as they are in CDFs
	double gasPressure(double density, double T)
	{
		constexpr const double k_B = 1.38064852e-23; // Boltzmann constant
		return density * 1e6 * k_B * T * 1e6;
	}

	// return pressure in SI, takes B directory from CDF
	double magneticPressure(double B)
	{
		// P = B^2/2/mu_0 (SI)
		constexpr const double mu_0 = 1.2566370614e-6;
		return sqr(B)*1e-18/2/mu_0;
	}
}

bool cdownload::Filters::PlasmaSheet::test(const std::vector<AveragedVariable>& line, std::vector<void*>& variables) const
{
	// check for R > 4 R_E
	const AveragedVariable& pos = sc_pos_xyz_gse_.data(line);
	constexpr const double RE = 6371;

	if (std::sqrt(sqr(pos[0].mean()) + sqr(pos[1].mean()) + sqr(pos[2].mean())) < 4 * RE) {
		return false;
	}

	// 0.2 <plasma beta< 10
	// beta is defined as ratio between plasma pressure and magnetic pressure
	// for plasma pressure we sum H1 pressure and O1 pressure

	const double totalPlasma =
		gasPressure(H1density_.data(line)[0].mean(), H1T_.data(line)[0].mean()) +
		gasPressure(O1density_.data(line)[0].mean(), O1T_.data(line)[0].mean());

	const double magnetic = magneticPressure(BMag_.data(line)[0].mean());

	const double beta = totalPlasma / magnetic;

	if (isVariableEnabled(reportBetaIndex_)) {
		*(reportBeta_.data<double>(variables)) = beta;
	}

	if (isVariableEnabled(reportPlasmaPressureIndex_)) {
		*(reportPlasmaPressure_.data<double>(variables)) = totalPlasma;
	}

	if (isVariableEnabled(reportMagneticPressureIndex_)) {
		*(reportMagneticPressure_.data<double>(variables)) = magnetic;
	}

	if (enabled() && (beta < 0.2 || beta > 10)) {
		return false;
	}
	return true;
}

