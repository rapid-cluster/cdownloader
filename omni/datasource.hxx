/*
 * cdownload lib: downloads, unpacks, and reads data from the Cluster CSA arhive
 * Copyright (C) 2017  Eugene Shalygin <eugene.shalygin@gmail.com>
 *
 * The development was partially supported by the Volkswagen Foundation
 * (VolkswagenStiftung).
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef CDOWNLOAD_OMNI_DATASOURCE_HXX
#define CDOWNLOAD_OMNI_DATASOURCE_HXX

#include "../datasource.hxx"

namespace cdownload {
	class Parameters;

namespace omni {

	class Downloader;

	class DataSource: public cdownload::DataSource {
		using base = cdownload::DataSource;

	public:
		DataSource(const Parameters& parameters);
		~DataSource();

	private:
		cdownload::DatasetChunk getNewChunk(const cdownload::datetime & min, const cdownload::datetime & max) override;
		std::vector<DatasetChunk> loadCachedFiles(const path& dir);

		path cacheDir_;

		std::unique_ptr<Downloader> downloader_;

		datetime currentChunkStartTime_;
		datetime lastServedChunkEndTime_;

		DatasetChunk lastServedChunk_;
		path tempDir_;
	};
}
}

#endif // CDOWNLOAD_OMNI_DATASOURCE_HXX
