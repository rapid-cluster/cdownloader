
#include "../csa/datasource.hxx"
#include "../parameters.hxx"
#include "../csa/metadata.hxx"
#include "../util.hxx"

#include <iostream>

int main(int /*argc*/, char** /*argv*/)
{
	using namespace cdownload;

	path cachDir = "/home/eugene/data/Cluster/data/cache";

	Parameters parameters ("/tmp", "/tmp", cachDir);
	csa::Metadata meta;
	csa::DataSource ds("C4_CP_CIS_MODES", parameters, meta);

	std::cout << "Time range: " << ds.minAvailableTime() << ", " << ds.maxAvailableTime() << std::endl;
}
