#include "../omni/downloader.hxx"

#include <iostream>
#include <fstream>
#include <boost/filesystem/operations.hpp>
#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>

#include <boost/lexical_cast.hpp>

using namespace cdownload;
namespace logging = boost::log;

int main(int /*argc*/, char** /*argv*/)
{
	logging::core::get()->set_filter
	(
		logging::trivial::severity >= logging::trivial::debug
	);

	omni::Downloader downloader;
	const auto yearRange = downloader.availableYearRange();
	std::cout << "Available year range: [" << yearRange.first << ':' << yearRange.second << ']' << std::endl;
	return 0;
}
