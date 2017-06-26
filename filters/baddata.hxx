#ifndef CDOWNLOAD_FILTER_BADDATA_HXX
#define CDOWNLOAD_FILTER_BADDATA_HXX

#include "../filter.hxx"

namespace cdownload {
namespace Filters {
	class BadDataFilter: public RawDataFilter {
		using base = RawDataFilter;
	public:
		BadDataFilter();
	private:
		bool test(const std::vector<const void*>& line, const DatasetName& ds, std::vector<void*>& variables) const override;
	};
}
}

#endif // CDOWNLOAD_FILTER_BADDATA_HXX
