#include "../downloader.hxx"
#include "../unpacker.hxx"

#include <iostream>
#include <fstream>
#include <boost/filesystem/operations.hpp>
#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>

#include <boost/lexical_cast.hpp>

using namespace cdownload;
namespace logging = boost::log;

int main(int argc, char** argv)
{

	logging::core::get()->set_filter
	(
		logging::trivial::severity >= logging::trivial::debug
	);

	std::vector<cdownload::DatasetName> datasets;
	if (argc >= 1) {
		for (int a = 1; a<argc; ++a) {
			datasets.emplace_back(argv[a]);
		}
	} else {
		datasets.emplace_back("C4_CP_RAP_ISPCT_CNO");
		datasets.emplace_back("C4_CP_RAP_ISPCT_CNO");
	}
	DataDownloader downloader;

	for (const auto& ds: datasets) {
		std::string outputDir = "/tmp/test-product-" + ds;
		std::ofstream of (outputDir.c_str());
		downloader.download(ds, of,
		                    datetime(date(2014, 10, 1), datetime::time_duration_type()),
		                    datetime(date(2014, 10, 2), boost::posix_time::hours(0) + boost::posix_time::minutes(0) + boost::posix_time::seconds(0)));

		path unpackedDir = path("/tmp/unpacked") / path(ds);

		if (!boost::filesystem::exists(unpackedDir)) {
			boost::filesystem::create_directories(unpackedDir);
		}


		DownloadedProductFile prFile {outputDir, unpackedDir, ds};
		std::cout << "Final file name is " << prFile.fileName() << std::endl;
		prFile.release();
	}
}
