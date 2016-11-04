#include "baddata.hxx"

#include <cmath>

// #define TRACING_BADDATA_FILTER

#ifdef TRACING_BADDATA_FILTER
#include <boost/log/trivial.hpp>
#endif

bool cdownload::Filters::BadDataFilter::test(const std::vector<const void*>& line, const DatasetName& ds) const
{
	for (const Field& f: availableProducts()) {
		if (f.name().dataset() == ds) {
#ifdef TRACING_BADDATA_FILTER
			BOOST_LOG_TRIVIAL(trace) << "Testing var " << f.name() << " at offset " << f.offset();
#endif
			if (f.dataType() == Field::DataType::Real) {
				for (std::size_t i = 0; i < f.elementCount(); ++i) {
					if (std::isnan(f.getReal(line, i))) {
						return false;
					}
				}
			}
		}
	}
	return true;
}
