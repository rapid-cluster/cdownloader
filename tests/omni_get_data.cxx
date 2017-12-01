#include "../omni/downloader.hxx"
#include "../omni/datasource.hxx"

#include "../parameters.hxx"

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

	Parameters params{"/tmp", "/tmp", "/home/eugene/data/Cluster/data/cache/"};

	omni::DataSource ds{params};
	ds.nextChunk();

	return 0;
}
