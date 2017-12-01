#include "../csa/chunkdownloader.hxx"
#include "../csa/downloader.hxx"
#include "../csa/metadata.hxx"

#include <boost/filesystem/operations.hpp>

#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>

namespace logging = boost::log;

int main(int /*argc*/, char** /*argv*/)
{
	logging::core::get()->set_filter
    (
        logging::trivial::severity >= logging::trivial::trace
    );

	using namespace cdownload;
	using namespace cdownload::csa;

	DataDownloader downloader;

	std::vector<DatasetName> datasets;
	datasets.emplace_back("C4_CP_RAP_ISPCT_CNO");
	datasets.emplace_back("D2_SP_PEA");

	datetime availableStartDateTime = makeDateTime(1999, 1, 1);
	datetime availableEndDateTime = datetime::utcNow();

	csa::Metadata meta;

	BOOST_LOG_TRIVIAL(debug) << "Initial time range: [" << availableStartDateTime << ", " << availableEndDateTime << "]";

	for (const auto& ds: datasets) {
		auto dataset = meta.dataset(ds);
		BOOST_LOG_TRIVIAL(debug) << "Dataset " << ds << ":" << dataset;
		if (dataset.minTime() > availableStartDateTime) {
			availableStartDateTime = dataset.minTime();
		}
		if (dataset.maxTime() < availableEndDateTime) {
			availableEndDateTime = dataset.maxTime();
		}
	}

	path tmpDir = "/tmp/test-chunk-download";
	boost::filesystem::create_directories(tmpDir);
	ChunkDownloader chunkDownloader {tmpDir, tmpDir, downloader, datasets, availableStartDateTime, availableEndDateTime};

	while(!chunkDownloader.eof()) {
		chunkDownloader.nextChunk();
	}

	return 0;
}
