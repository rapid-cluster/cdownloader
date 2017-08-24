#include "../metadata.hxx"
#include "../util.hxx"
#include <iostream>
#include <sstream>

#ifdef __GNUG__
#include <cstdlib>
#include <memory>
#include <cxxabi.h>

std::string demangle(const char* name) {

    int status = -4; // some arbitrary value to eliminate the compiler warning

    // enable c++11 by passing the flag -std=c++11 to g++
    std::unique_ptr<char, void(*)(void*)> res {
        abi::__cxa_demangle(name, NULL, NULL, &status),
        std::free
    };

    return (status==0) ? res.get() : name ;
}
#else
std::string demangle(const char* name) {
	return std::String(name);
}
#endif

int main(int argc, char** argv) {
	if (argc == 1) {
		std::cerr << "No datasets specified. Exiting" << std::endl;
		return 2;
	}

	try {
		cdownload::Metadata meta;

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
	} catch (std::exception& ex) {
		std::cerr << "Execution was terminated due to '" << demangle(typeid(ex).name()) << "' exception." << std::endl
			<< "Error message: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}
