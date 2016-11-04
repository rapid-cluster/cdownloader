#include "../downloader.hxx"
#include "../unpacker.hxx"
#include "../cdfreader.hxx"

#include <iostream>
#include <fstream>
#include <boost/filesystem/operations.hpp>
#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>

using namespace cdownload;
namespace logging = boost::log;


int main(int /*argc*/, char** /*argv*/)
{

	logging::core::get()->set_filter
    (
        logging::trivial::severity >= logging::trivial::debug
    );

	DataDownloader downloader;

	{
		std::ofstream of ("/tmp/test-product-1");
		downloader.download("C4_CP_RAP_ISPCT_CNO", of,
			datetime(date(2014, 10, 1), datetime::time_duration_type()),
			datetime(date(2014, 10, 2), boost::posix_time::hours(23)+boost::posix_time::minutes(59)+boost::posix_time::seconds(59)));
	}

	path unpackedDir = path("/tmp/unpacked");

	if (!boost::filesystem::exists(unpackedDir)) {
		boost::filesystem::create_directories(unpackedDir);
	}


	DownloadedProductFile prFile {"/tmp/test-product-1", unpackedDir, "C4_CP_RAP_ISPCT_CNO"};
	std::cout << "Final file name is " << prFile.fileName() << std::endl;

	CDF::File cdf {prFile};

	std::cout << "Downloaded CDF file variables:" << std::endl;
	for (std::size_t i = 0; i < cdf.variablesCount(); ++i) {
		const CDF::Variable v = cdf.variable(i);
		std::cout << CDF::VariableMetaPrinter(v, 1) << std::endl;
	}

}

