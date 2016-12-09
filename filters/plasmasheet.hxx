#ifndef CDOWNLOAD_FILTER_PLASMASHEET_HXX
#define CDOWNLOAD_FILTER_PLASMASHEET_HXX

#include "../filter.hxx"
#include "config.h"

namespace cdownload {
namespace Filters {

	class PlasmaSheetModeFilter: public RawDataFilter {
		using base = RawDataFilter;
	public:
		PlasmaSheetModeFilter();
		bool test(const std::vector<const void *> & line, const DatasetName& ds) const override;
	private:
		const Field& cis_mode_;
// 		const Field& cis_mode_key_;
	};

	class PlasmaSheet : public AveragedDataFilter {
		using base = AveragedDataFilter;
	public:
		PlasmaSheet();
		bool test(const std::vector<AveragedVariable>& line) const override;
	private:
		const Field& H1density_;
		const Field& H1T_;
		const Field& O1density_;
		const Field& O1T_;
		const Field& BMag_;
		const Field& sc_pos_xyz_gse_;
#ifdef USE_HIA_DENSITY
		const Field& hiaH1density_;
#endif
	};
}
}

#endif // CDOWNLOAD_FILTER_PLASMASHEET_HXX
