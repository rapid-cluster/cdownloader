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

#include "omnidb.hxx"

#include "../downloader.hxx"

#include <sstream>

const std::string cdownload::omni::OmniTableDesc::HRODirectoryUrl{"ftp://spdf.gsfc.nasa.gov/pub/data/omni/high_res_omni/"};
const cdownload::DatasetName cdownload::omni::OmniTableDesc::HRODatasetName{"OMNI_HRO"};

cdownload::omni::OmniTableDesc::OmniTableDesc()
	: fields_{highResOmniFields()}
{
}

cdownload::string cdownload::omni::OmniTableDesc::fileNameForHRODbFile(unsigned int year, bool perMinuteFile)
{
	// ftp://spdf.gsfc.nasa.gov/pub/data/omni/high_res_omni/omni_min1981.asc
	// ftp://spdf.gsfc.nasa.gov/pub/data/omni/high_res_omni/omni_5min1981.asc
	std::ostringstream res;
	res << "omni_";
	if (!perMinuteFile) {
		res << 5;
	}
	res << "min" << year << ".asc";
	return res.str();
}

cdownload::string cdownload::omni::OmniTableDesc::urlForHRODbFile(unsigned int year, bool perMinuteFile)
{
	// ftp://spdf.gsfc.nasa.gov/pub/data/omni/high_res_omni/omni_min1981.asc
	// ftp://spdf.gsfc.nasa.gov/pub/data/omni/high_res_omni/omni_5min1981.asc

	return HRODirectoryUrl + fileNameForHRODbFile(year, perMinuteFile);
}

cdownload::ProductName cdownload::omni::OmniTableDesc::makeOmniHROName(const cdownload::string& fieldName)
{
	return ProductName(HRODatasetName, fieldName);
}

std::vector<cdownload::FieldDesc> cdownload::omni::OmniTableDesc::highResOmniFields()
{
	using DT = cdownload::FieldDesc::DataType;
	return {
		{makeOmniHROName("year"), 0., DT::UnsignedInt, 4, 1, "Year I4 1995 ... 2006"},
		{makeOmniHROName("day"), 0., DT::UnsignedInt, 4, 1, "Day 1 ... 365 or 366"},
		{makeOmniHROName("hour"), -1., DT::UnsignedInt, 4, 1, "Hour I3 0 ... 23"},
		{makeOmniHROName("minute"), -1., DT::UnsignedInt, 4, 1, "Minute I3 0 ... 59 at start of average"},
		{makeOmniHROName("id-imf"), -1, DT::UnsignedInt, 4, 1, "ID for IMF spacecraft I3"},
		{makeOmniHROName("id-sw"), -1., DT::UnsignedInt, 4, 1, "ID for SW Plasma spacecraft I3"},
		{makeOmniHROName("count-imf"), -1., DT::UnsignedInt, 4, 1, "# of points in IMF averages I4"},
		{makeOmniHROName("count-sw"), -1., DT::UnsignedInt, 4, 1, "# of points in Plasma averages I4"},
		{makeOmniHROName("percent-interp"), -1., DT::UnsignedInt, 4, 1, "Percent interp I4"},
		{makeOmniHROName("timeshift"), -1., DT::SignedInt, 8, 1, "Timeshift, sec I7"},
		{makeOmniHROName("timeshift-rms"), -1., DT::UnsignedInt, 8, 1, "RMS, Timeshift I7"},
		{makeOmniHROName("phase-front-rms"), -1., DT::Real, 8, 1, "RMS, Phase front normal F6.2"},
		{makeOmniHROName("observations-gap"), -1., DT::UnsignedInt, 8, 1, "Time btwn observations, sec I7"},
		{makeOmniHROName("field-mag-avg"), -1., DT::Real, 8, 1, "Field magnitude average, nT F8.2"},

		{makeOmniHROName("bx-gse-gsm"), -1., DT::Real, 8, 1, "Bx, nT (GSE, GSM) F8.2"},
		{makeOmniHROName("by-gse"), -1., DT::Real, 8, 1, "By, nT (GSE) F8.2"},
		{makeOmniHROName("bz-gse"), -1., DT::Real, 8, 1, "Bz, nT (GSE) F8.2"},
		{makeOmniHROName("by-gsm"), -1., DT::Real, 8, 1, "By, nT (GSM) F8.2"},
		{makeOmniHROName("bz-gsm"), -1., DT::Real, 8, 1, "Bz, nT (GSM) F8.2"},

		{makeOmniHROName("b-scalar-rms"), -1., DT::Real, 8, 1, "RMS SD B scalar, nT F8.2"},
		{makeOmniHROName("sd-field-vector-rms"), -1., DT::Real, 8, 1, "RMS SD field vector, nT F8.2"},
		{makeOmniHROName("flow-speed"), -1., DT::Real, 8, 1, "Flow speed, km/s F8.1"},
		{makeOmniHROName("vx-velocity"), -1., DT::Real, 8, 1, "Vx Velocity, km/s, GSE F8.1"},
		{makeOmniHROName("vy-velocity"), -1., DT::Real, 8, 1, "Vy Velocity, km/s, GSE F8.1"},
		{makeOmniHROName("vz-velocity"), -1., DT::Real, 8, 1, "Vz Velocity, km/s, GSE F8.1"},
		{makeOmniHROName("proton-density"), -1, DT::Real, 8, 1, "Proton Density, n/cc F7.2"},
		{makeOmniHROName("temperature"), -1., DT::Real, 8, 1, "Temperature, K F9.0"},
		{makeOmniHROName("flow-pressure"), -1., DT::Real, 8, 1, "Flow pressure, nPa F6.2"},
		{makeOmniHROName("electric-field"), -1., DT::Real, 8, 1, "Electric field, mV/m F7.2"},
		{makeOmniHROName("plasma-beta"), -1., DT::Real, 8, 1, "Plasma beta F7.2"},
		{makeOmniHROName("alfven-mach-number"), -1., DT::Real, 8, 1, "Alfven mach number F6.1"},
		{makeOmniHROName("x-sc-gse"), -1., DT::Real, 8, 1, "X(s/c), GSE, Re F8.2"},
		{makeOmniHROName("y-sc-gse"), -1., DT::Real, 8, 1, "Y(s/c), GSE, Re F8.2"},
		{makeOmniHROName("z-sc-gse"), -1., DT::Real, 8, 1, "Z(s/c), GSE, Re F8.2"},
		{makeOmniHROName("x-bsn-location-gse"), -1., DT::Real, 8, 1, "BSN location, Xgse, Re F8.2, BSN = bow shock nose"},
		{makeOmniHROName("y-bsn-location-gse"), -1., DT::Real, 8, 1, "BSN location, Ygse, Re F8.2, BSN = bow shock nose"},
		{makeOmniHROName("z-bsn-location-gse"), -1., DT::Real, 8, 1, "BSN location, Zgse, Re F8.2, BSN = bow shock nose"},

//               Ancilary Data
		{makeOmniHROName("index-ae"), -1., DT::SignedInt, 8, 1, "AE-index, nT I6"},
		{makeOmniHROName("index-al"), -1., DT::SignedInt, 8, 1, "AL-index, nT I6"},
		{makeOmniHROName("index-au"), -1., DT::SignedInt, 8, 1, "AU-index, nT I6"},
		{makeOmniHROName("index-sym-d"), -1., DT::SignedInt, 8, 1, "SYM/D index, nT I6"},
		{makeOmniHROName("index-sym-h"), -1., DT::SignedInt, 8, 1, "SYM/H index, nT I6"},
		{makeOmniHROName("index-asy-d"), -1., DT::SignedInt, 8, 1, "ASY/D index, nT I6"},
		{makeOmniHROName("index-asy-h"), -1., DT::SignedInt, 8, 1, "ASY/H index, nT I6"},
		{makeOmniHROName("index-pc-h"), -1., DT::Real, 8, 1, "PC(N) index F7.2"},
		{makeOmniHROName("mag-mach-number"), -1., DT::Real, 8, 1, "Magnetosonic mach number F5.1"}
	};
}
