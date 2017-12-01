#include "../csa/downloader.hxx"
#include "../csa/unpacker.hxx"
#include "../cdf/reader.hxx"

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

	csa::DataDownloader downloader;

	{
		std::ofstream of ("/tmp/test-product-1");
		downloader.beginDownloading("C4_CP_RAP_ISPCT_CNO", of,
		                            makeDateTime(2014, 10, 1, 0, 0, 0),
		                            makeDateTime(2014, 10, 2, 23, 59, 59));
		downloader.waitForFinished();
	}

	path unpackedDir = path("/tmp/unpacked");

	if (!boost::filesystem::exists(unpackedDir)) {
		boost::filesystem::create_directories(unpackedDir);
	}


	DownloadedChunkFile prFile = extractAndAccureDataFile("/tmp/test-product-1", unpackedDir, "C4_CP_RAP_ISPCT_CNO");
	std::cout << "Final file name is " << prFile.fileName() << std::endl;

	CDF::File cdf {prFile};

	std::cout << "Downloaded CDF file variables:" << std::endl;
	for (std::size_t i = 0; i < cdf.variablesCount(); ++i) {
		const CDF::Variable v = cdf.variable(i);
		std::cout << CDF::VariableMetaPrinter(v, 1) << std::endl;
	}
}
