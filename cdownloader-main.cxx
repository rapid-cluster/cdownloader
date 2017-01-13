#include "config.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

#include "driver.hxx"
#include "parameters.hxx"

#include <boost/program_options.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/support/date_time.hpp>

#include <unistd.h>
#include <errno.h>

namespace po = boost::program_options;

#if 1
// namespace cdownload {
namespace boost {
namespace posix_time {
	void validate(boost::any& v,
	              const std::vector<std::string>& values,
	              ptime* /*target_type*/, int)
	{
		using namespace boost::program_options;

		// Extract the first string from 'values'. If there is more than
		// one string, it's an error, and exception will be thrown.
		const std::string& s = validators::get_single_string(values);
		std::istringstream is(s);

		cdownload::datetime dt;
		is >> dt;
		v = boost::any(dt);
	}

	void validate(boost::any& v,
	              const std::vector<std::string>& values,
	              cdownload::timeduration* /*target_type*/, int)
	{
		using namespace boost::program_options;

		// Extract the first string from 'values'. If there is more than
		// one string, it's an error, and exception will be thrown.
		const std::string& s = validators::get_single_string(values);
		std::istringstream is(s);

		cdownload::timeduration dt;
		is >> dt;
		v = boost::any(dt);
	}
}
}
#endif

void assureDirectoryExistsAndWritable(const cdownload::path& p, const std::string& dirNameForUser)
{
	if (!boost::filesystem::exists(p)) {
		throw std::runtime_error(dirNameForUser + " directory '" + p.string() + "' does not exist");
	}
	if (!boost::filesystem::is_directory(p)) {
		throw std::runtime_error(dirNameForUser + " path '" + p.string() + "' exists, but is not a directory");
	}
	if (::access(p.c_str(), W_OK) != 0) {
		int errsv = errno;
		char errMessageBuf[512];
		char* errMessage = strerror_r(errsv, errMessageBuf, 512);
		throw std::runtime_error(dirNameForUser + " directory '" + p.string() + "' is not writable: " + std::string(errMessage));
	}
}

struct null_deleter {
	//! Function object result type
	typedef void result_type;
	/*!
	 * Does nothing
	 */
	template< typename T >
	void operator() (T*) const BOOST_NOEXCEPT
	{
	}
};

int main(int ac, char** av)
{
#ifndef NODEBUG

#endif

	using path = cdownload::path;
	namespace logging = boost::log;

	// Declare the supported options.
	po::options_description desc("Allowed options");

	desc.add_options()
	    ("help", "produce this help message")
	    ("expansion-dict",
	    po::value<path>()->default_value(DEFAULT_EXPANSION_DICTIONARY_FILE),
	    "Location of the expansion dictionary file")
	    ("verbosity-level,v",
	    po::value<logging::trivial::severity_level>()->default_value(logging::trivial::info),
	    "Verbosity level")
	    ("log-file", po::value<path>(), "Log file location")
	    ("continue", po::value<bool>()->default_value(false)->implicit_value(true), "Continue downloading existing output files")
	    ("cache-dir", po::value<path>(), "Directory with pre-downloaded CDF files")
	    ("download-missing", po::value<bool>()->default_value(true)->implicit_value(true),
	         "Download missing from cache data");

	po::options_description timeOptions("Time interval");

	timeOptions.add_options()
	    ("start", po::value<cdownload::datetime>()->default_value(cdownload::makeDateTime(2000, 8, 10)), "Start time")
	    ("end", po::value<cdownload::datetime>()->default_value(cdownload::datetime::utcNow()), "End time")
	    ("cell-size", po::value<cdownload::timeduration>()->required(), "Size of the averaging cell")
	    ("valid-time-ranges", po::value<path>(), "File with time cells")
	;
	desc.add(timeOptions);

	po::options_description qualityOptions("Quality options");
	qualityOptions.add_options()
		("quality-product-name", po::value<std::vector<cdownload::ProductName>>()->composing(), "Product name")
		("quality-min-value", po::value<std::vector<int>>()->composing(), "Min quality value")
	;

	desc.add(qualityOptions);

	po::options_description densityOptions("Density filter");
	densityOptions.add_options()
		("density-source", po::value<std::string>(), "Density source. Either 'CODIF' or 'HIA'")
		("min-density-value", po::value<double>(), "Minimal density value")
	;

	desc.add(densityOptions);

	po::options_description optionalFilters("Optional filters");
	optionalFilters.add_options()
		("nighttime-only", po::value<bool>()->implicit_value(true), "Use only night time data")
		("allow-blanks", po::value<bool>()->implicit_value(true), "Do not remove blank values")
	;

	desc.add(optionalFilters);

	po::options_description outputOptions("Outputs");

	outputOptions.add_options()
	    ("output-dir", po::value<cdownload::path>()->default_value("/tmp"), "Output directory")
	    ("work-dir", po::value<cdownload::path>()->default_value("/tmp"), "Working directory")
	    ("output", po::value<std::vector<cdownload::path> >()->composing()->required(), "List of files with output descriptions")
	;

	desc.add(outputOptions);

	po::variables_map vm;
	try {
		po::store(po::parse_command_line(ac, av, desc), vm);
		po::notify(vm);
	} catch (std::exception& ex) {
		std::cerr << "Error parsing command line: " << ex.what() << std::endl << std::endl;
		std::cerr << desc << std::endl;
		return 2;
	}

	if (vm.count("help")) {
		std::cout << desc << std::endl;
		return 1;
	}

	path cacheDir = vm.count("cache-dir") ? vm["cache-dir"].as<path>() : path();
	cdownload::Parameters parameters {vm["output-dir"].as<path>(), vm["work-dir"].as<path>(), cacheDir};
	parameters.setContinueMode(vm["continue"].as<bool>());
	parameters.setDownloadMissingData(vm["download-missing"].as<bool>());

	std::vector<cdownload::ProductName> qualityFilterProducts;
	std::vector<int> qualityFilterMinQualities;

	if (vm.count("quality-product-name")) {
		qualityFilterProducts = vm["quality-product-name"].as<std::vector<cdownload::ProductName>>();
	}

	if (vm.count("quality-min-value")) {
		qualityFilterMinQualities = vm["quality-min-value"].as<std::vector<int>>();
	}

	if (qualityFilterProducts.size() != qualityFilterMinQualities.size()) {
		std::cerr << "Number of quality product names (" << qualityFilterProducts.size()
			<< ") is not equal to that of quality levels (" << qualityFilterMinQualities.size()
			<< ")" << std::endl;
		return 2;
	}

	for (std::size_t i = 0; i < qualityFilterProducts.size(); ++i) {
		parameters.addQualityFilter(qualityFilterProducts[i], qualityFilterMinQualities[i]);
	}

	if (vm.count("density-source") != vm.count("min-density-value")) {
		std::cerr << "Density source and its minimal value must be specified together" << std::endl;
		return 2;
	}

	if (vm.count("density-source")) {
		std::string densitySource = vm["density-source"].as<std::string>();
		if ((densitySource != "CODIF") && (densitySource != "HIA")) {
			std::cerr << "Only 'CODIF' and 'HIA' are valid velues for 'density-source' parameter" << std::endl;
			return 2;
		}
		parameters.addDensityFilter(
			densitySource == "CODIF" ? cdownload::DensitySource::CODIF : cdownload::DensitySource::HIA,
			vm["min-density-value"].as<double>());
	}

	parameters.onlyNightSide(vm.count("nighttime-only"));
	if (vm.count("allow-blanks")) {
		parameters.allowBlanks(vm["allow-blanks"].as<bool>());
	}

	if (vm.count("valid-time-ranges")) {
		parameters.timeRangesFileName(vm["valid-time-ranges"].as<path>());
	}

	try {
		assureDirectoryExistsAndWritable(parameters.outputDir(), "Output");
		assureDirectoryExistsAndWritable(parameters.workDir(), "Working");
	} catch (std::exception& ex) {
		std::cerr << ex.what() << std::endl;
		return 2;
	}

	if (!vm.count("output")) {
		std::cerr << "No outputs specified. Exiting." << std::endl;
		return 2;
	}

	std::vector<path> outputDefinitions = vm["output"].as<std::vector<path> >();

	for (path p: outputDefinitions) {
		try {
			parameters.addOuput(cdownload::parseOutputDefinitionFile(p));
		} catch (std::exception& e) {
			std::cerr << "Error adding output defined in file " << p << ": " << e.what() << std::endl;
			return 2;
		}
	}

	logging::trivial::severity_level logLevel = vm["verbosity-level"].as<logging::trivial::severity_level>();

	logging::core::get()->set_filter
	(
		logging::trivial::severity >= logLevel
	);

	if (vm.count("log-file")) {
		path logFile = vm["log-file"].as<path>();
		if (!logFile.parent_path().empty()) {
			assureDirectoryExistsAndWritable(logFile.parent_path(), "Log file directory");
		}

		logging::add_file_log
		(
			logging::keywords::file_name = logFile.string(),
//          logging::keywords::rotation_size = 10 * 1024 * 1024,
//          logging::keywords::time_based_rotation = logging::sinks::file::rotation_at_time_point(0, 0, 0),
			logging::keywords::format = (
				logging::expressions::stream
				    << logging::expressions::format_date_time< boost::posix_time::ptime >("TimeStamp", "[%Y-%m-%dT%H:%M:%S.%f]")
				    << ": <" << logging::trivial::severity
				    << "> " << logging::expressions::smessage
				),
			logging::keywords::auto_flush = true
		);

		logging::add_common_attributes();
	}

	parameters.setExpansionDictFile(vm["expansion-dict"].as<path>());
	parameters.setTimeRange(vm["start"].as<cdownload::datetime>(), vm["end"].as<cdownload::datetime>());
	parameters.setTimeInterval(vm["cell-size"].as<cdownload::timeduration>());

	cdownload::Driver driver {parameters};
	driver.doTask();

	return 0;
}
