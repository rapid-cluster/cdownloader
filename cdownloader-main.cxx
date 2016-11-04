#include "config.h"

#include <iostream>
#include <sstream>

#include "driver.hxx"
#include "parameters.hxx"

#include <boost/program_options.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>

#include <unistd.h>
#include <errno.h>

namespace po = boost::program_options;

#if 1
// namespace cdownload {
namespace boost{
	namespace posix_time {
void validate(boost::any& v,
              const std::vector<std::string>& values,
              ptime* /*target_type*/, int)
{
	using namespace cdownload::csa_time_formatting;
	using namespace boost::program_options;

	// Extract the first string from 'values'. If there is more than
	// one string, it's an error, and exception will be thrown.
	const std::string& s = validators::get_single_string(values);
	std::istringstream is(s);

	cdownload::datetime dt;
	cdownload::csa_time_formatting::operator>>(is, dt);
	v = boost::any(dt);
}

void validate(boost::any& v,
              const std::vector<std::string>& values,
              cdownload::timeduration* /*target_type*/, int)
{
	using namespace cdownload::csa_time_formatting;
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
		char errMessage[512];
		strerror_r(errsv, errMessage, 512);
		throw std::runtime_error(dirNameForUser + " directory '" + p.string() + "' is not writable: " + std::string(errMessage));
	}
}

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
			po::value<logging::trivial::severity_level>()->default_value(logging::trivial::warning),
			"Verbosity level");
	;

	po::options_description timeOptions("Time interval");

	timeOptions.add_options()
		("start", po::value<cdownload::datetime>()->default_value(cdownload::makeDateTime(2000, 8, 10)), "Start time")
		("end", po::value<cdownload::datetime>()->default_value(boost::posix_time::second_clock::universal_time()), "End time")
		("cell-size", po::value<cdownload::timeduration>()->required(), "Size of the averaging cell")
	;
	desc.add(timeOptions);

	po::options_description outputOptions("Outputs");

	outputOptions.add_options()
		("output-dir", po::value<cdownload::path>()->default_value("/tmp"), "Output directory")
		("work-dir", po::value<cdownload::path>()->default_value("/tmp"), "Working directory")
		("output", po::value<std::vector<cdownload::path>>()->composing()->required(), "List of files with output descriptions")
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

	cdownload::Parameters parameters {vm["output-dir"].as<path>(), vm["work-dir"].as<path>()};

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

	std::vector<path> outputDefinitions = vm["output"].as<std::vector<path>>();

	for (path p:outputDefinitions) {
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

	parameters.setExpansionDictFile(vm["expansion-dict"].as<path>());
	parameters.setTimeRange(vm["start"].as<cdownload::datetime>(), vm["end"].as<cdownload::datetime>());
	parameters.setTimeInterval(vm["cell-size"].as<cdownload::timeduration>());

	cdownload::Driver driver {parameters};
	driver.doTask();

	return 0;
}
