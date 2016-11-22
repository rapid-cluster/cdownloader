#include "../metadata.hxx"
#include "../util.hxx"
#include <sstream>

int main(int argc, char** argv) {
	if (argc == 1) {
		std::cerr << "No datasets specified. Exiting" << std::endl;
		return 2;
	}

	cdownload::Metadata meta;
	using namespace cdownload::csa_time_formatting;

	for (int i = 1; i < argc; ++i) {
		cdownload::DatasetName ds(argv[i]);
		auto dsMeta = meta.dataset(ds);

		std::ostringstream os;

		os << "https://csa.esac.esa.int/csa/aio/async-product-action?"
			<< "DATASET_ID=" << ds
			<< "&START_DATE=" << dsMeta.minTime()
			<< "&END_DATE=" << dsMeta.maxTime()
			<< "&DELIVERY_FORMAT=CDF&DELIVERY_INTERVAL=All";
		std::cout << os.str() << std::endl;
	}

	return 0;
}
