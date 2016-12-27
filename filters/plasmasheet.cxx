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
	: base(1)
	, cis_mode_(addField("cis_mode__C4_CP_CIS_MODES"))
//  , cis_mode_key_(addField("cis_mode_key__C4_CP_CIS_MODES"))
{
}

bool cdownload::Filters::PlasmaSheetModeFilter::test(const std::vector<const void*>& line, const DatasetName& /*ds*/) const
{
	// CIS_mode=13
	const int* cis_mode = cis_mode_.data<int>(line);
	if (*cis_mode != 13) {
		return false;
	}

	return true;
}

cdownload::Filters::PlasmaSheet::PlasmaSheet()
	: base(7)
	, H1density_(addField("density__C4_CP_CIS-CODIF_HS_H1_MOMENTS"))
	, H1T_(addField("T__C4_CP_CIS-CODIF_HS_H1_MOMENTS"))
	, O1density_(addField("density__C4_CP_CIS-CODIF_HS_O1_MOMENTS"))
	, O1T_(addField("T__C4_CP_CIS-CODIF_HS_O1_MOMENTS"))
	, BMag_(addField("B_mag__C4_CP_FGM_SPIN"))
	, sc_pos_xyz_gse_(addField("sc_pos_xyz_gse__C4_CP_FGM_SPIN"))
#ifdef USE_HIA_DENSITY
	, hiaH1density_(addField("density__C1_CP_CIS-HIA_ONBOARD_MOMENTS"))
#endif
{
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

bool cdownload::Filters::PlasmaSheet::test(const std::vector<AveragedVariable>& line) const
{
	// check for R > 4 R_E
	const AveragedVariable& pos = sc_pos_xyz_gse_.data(line);
	constexpr const double RE = 6371;

	if (std::sqrt(sqr(pos[0].mean()) + sqr(pos[1].mean()) + sqr(pos[2].mean())) < 4 * RE) {
		return false;
	}

#if 0
	// proton density is greater than 2 cm-3
	const double MIN_H1_DENSITY_IN_PLASMASHEET = 2.;
#ifdef USE_HIA_DENSITY
	const Field& h1densityFiled = hiaH1density_;
#else
	const Field& h1densityFiled = H1density_;
#endif
	if (h1densityFiled.data(line)[0].mean() < MIN_H1_DENSITY_IN_PLASMASHEET) {
		return false;
	}
#endif

	// 0.2 <plasma beta< 10
	// beta is defined as ratio between plasma pressure and magnetic pressure
	// for plasma pressure we sum H1 pressure and O1 pressure

	const double totalPlasma =
		gasPressure(H1density_.data(line)[0].mean(), H1T_.data(line)[0].mean()) +
		gasPressure(O1density_.data(line)[0].mean(), O1T_.data(line)[0].mean());

	const double magnetic = magneticPressure(BMag_.data(line)[0].mean());

	const double beta = totalPlasma / magnetic;

	if (beta < 0.2 || beta > 10) {
		return false;
	}
	return true;
}
