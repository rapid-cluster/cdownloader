#include "../csa/metadata.hxx"

#include <iostream>
#include <iomanip>
#include <json/writer.h>

using namespace cdownload;
using namespace cdownload::csa;

void printDateTime(std::ostream& os, const cdownload::datetime& d)
{
#ifdef STD_CHRONO
	std::time_t t = std::chrono::system_clock::to_time_t(d);
	auto tgm = std::gmtime(&t);
	os << std::put_time(tgm, "%FT%TZ");
#else
	os << d.isoExtendedString() << 'Z';
#endif
}

int main(int argc, char** argv)
{
	std::vector<cdownload::DatasetName> datasets;
	if (argc > 1) {
		for (int i = 1; i < argc; ++i) {
			datasets.emplace_back(argv[i]);
		}
	} else {
		datasets.emplace_back("C4_CP_CIS-CODIF_HS_H1_MOMENTS");
	}

	csa::Metadata meta;

	for (const auto& ds: datasets) {
		std::cout << "-----------------------------------------" << std::endl;
		std::cout << meta.dataset(ds) << std::endl;
		auto dsSP_PEA = meta.dataset(ds);
		std::cout << "Data range: ";
		printDateTime(std::cout, dsSP_PEA.minTime());
		std::cout << ", ";
		printDateTime(std::cout, dsSP_PEA.maxTime());
		std::cout << std::endl;
	// 	std::cout << meta.timeResolution("D2_SP_PEA") << std::endl;
	}

	return 0;
}
