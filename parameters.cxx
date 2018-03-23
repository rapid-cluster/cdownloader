/*
 * cdownload lib: downloads, unpacks, and reads data from the Cluster CSA arhive
 * Copyright (C) 2016  Eugene Shalygin <eugene.shalygin@gmail.com>
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

#include "parameters.hxx"

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/trim_all.hpp>
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/log/trivial.hpp>


#include <algorithm>
#include <fstream>
#include <string>
#include <sstream>
#include <set>
#include <vector>


cdownload::Output::Output(const std::string& name, cdownload::Output::Format format, const std::vector<ProductName>& products)
	: Output(name, format, parseProductsList(products))
{
}

cdownload::Output::Output(const std::string& name, cdownload::Output::Format format, const DatasetProductsMap& products)
	: name_{name}
	, format_{format}
	, products_{products}
{
}


std::vector<std::string> cdownload::Output::datasetNames() const {
	std::vector<std::string> res;
	for (const auto& p: products_) {
		res.push_back(p.first);
	}
	return res;
}

const std::vector<cdownload::ProductName>& cdownload::Output::productsForDataset(const std::string& dataset) const {
	auto i = products_.find(dataset);
	if (i != products_.end()) {
		return i->second;
	}
	throw std::runtime_error("No dataset with name");
}

const std::vector<cdownload::ProductName> cdownload::Output::productsForDatasetOrDefault(
	const std::string& dataset, const std::vector<ProductName>& defaultValue) const
{
	auto i = products_.find(dataset);
	if (i != products_.end()) {
		return i->second;
	}
	return defaultValue;
}

cdownload::Parameters::Parameters(const path& outputDir, const path& workDir, const path& cacheDir)
	: startDate_{makeDateTime(2000, 7, 16)}
	, endDate_{datetime::utcNow()}
	, timeInterval_{0, 1, 0}
	, outputs_{}
	, expansionDictionaryFile_{"/usr/share/csadownloader/expansion.dict"}
	, outputDir_{outputDir}
	, workDir_{workDir}
	, cacheDir_{cacheDir}
	, downloadMissingData_{true}
	, allowBlanks_{false} {
}

void cdownload::Parameters::setExpansionDictFile(const path& fileName) {
	expansionDictionaryFile_ = fileName;
}

void cdownload::Parameters::setTimeRange(const datetime& startDate, const datetime& endDate) {
	if (endDate <= startDate) {
		throw std::runtime_error("start date may not be lower than end date");
	}
	startDate_ = startDate;
	endDate_ = endDate;
}

void cdownload::Parameters::setTimeInterval(const timeduration& interval) {
	timeInterval_ = interval;
}

void cdownload::Parameters::setContinueMode(bool continueDownloading) {
	continue_ = continueDownloading;
}

void cdownload::Parameters::setDownloadMissingData(bool download)
{
	downloadMissingData_ = download;
}

namespace {
	[[noreturn]]
	void signalParsingError(const cdownload::path& fileName, std::size_t lineNo, const std::string& errorMessage)
	{
		std::ostringstream errorMessageStream;
		errorMessageStream << "Error parsing output definition file " << fileName << ':' << std::endl;
		errorMessageStream << "Invalid format at line " << lineNo << ": " << errorMessage;
		BOOST_LOG_TRIVIAL(fatal) << errorMessageStream.str() << std::endl;
		throw std::runtime_error(errorMessageStream.str());
	}

	void removeTrailingComment(std::string& s)
	{
		// if a string contains a '#' (comment char), which is not preceded by another '#',
		// remove the '#' and everything after it
		// It there is a "##", replace it with '#'

		constexpr const char commentSign = '#';
		std::ostringstream res;
		for (std::string::size_type i = 0; i < s.size(); ++i) {
			if (s[i] != commentSign) {
				res << s[i];
			} else {
				if ((i < s.size() - 1) && (s[i+1] == commentSign)) { // double '#'
					res << commentSign;
					++i;
				} else { // this is a comment
					break;
				}
			}
		}

		s = res.str();
	}

	// convert string into format. Note: string has to be valid!
	cdownload::Output::Format parseFormatString(const std::string& s)
	{
		using Format = cdownload::Output::Format;
		if (s == "ASCII") {
			return Format::ASCII;
		} else if (s == "BINARY") {
			return Format::Binary;
		} else if (s == "CDF") {
			return Format::CDF;
		}
		throw std::logic_error("Uknown format string");
	}

	void printFormat(std::ostream& os, cdownload::Output::Format f)
	{
		using Format = cdownload::Output::Format;
		switch(f) {
			case Format::ASCII:
				os << "ASCII";
				break;
			case Format::Binary:
				os << "BINARY";
				break;
			case Format::CDF:
				os << "CDF";
				break;
		}
	}
}

cdownload::Output cdownload::parseOutputDefinitionFile(const path& filePath)
{
	/* The file format is extremely simple "key=value" format with three possible keys
	 * allowed: "name", "format", and "products"
	 * Format may be one of "ASCII", "BINARY", or "CDF"
	 * Lines that start with '#' are comments
	 * Products list is comma or semicolon separated list of strings
	 * If a line does not contain '=' character, it is a continuation of the previous line
	 */
	std::ifstream inp(filePath.c_str());
	std::string name;
	std::string formatString;
	std::vector<std::string> productsList;

	class LineReader {
	public:
		LineReader(std::istream& is)
			: is_{is}
			, curLine_{}
			, lineNo_{0}
		{
		}

		std::pair<std::string,std::string> nextLine()
		{
			while (is_) {
				std::string newLine;
				std::getline(is_, newLine);
				++lineNo_;
				boost::algorithm::trim_all(newLine);
				if (newLine.empty() || newLine[0] == '#') {
					continue;
				}
				removeTrailingComment(newLine);
				std::vector<std::string> parts;
				boost::algorithm::split(parts, newLine, boost::is_any_of("="), boost::token_compress_on);
				if (parts.size() == 1 && !curLine_.second.empty()) { // this is continuation of the previous line
					curLine_.second += newLine;
					continue;
				}
				if (parts.size() == 2) {
					if (curLine_.second.empty()) {
						curLine_ = {parts[0], parts[1]};
						continue;
					} else {
						const auto res = curLine_;
						curLine_ = {parts[0], parts[1]};
						return res;
					}
				}
				if (parts.size() > 2) {
					throw std::runtime_error("each line has to follow 'key=value' scheme");
				}
			}
			const auto res = curLine_;
			curLine_ = {};
			return res;
		}

		std::size_t lineNo() const {
			return lineNo_;
		}

	private:
		std::istream& is_;
		std::pair<std::string,std::string> curLine_;
		std::size_t lineNo_;
	};

	LineReader reader(inp);
	try {
		for(;;) {
			const auto rec = reader.nextLine();
			if (rec.first.empty() && rec.second.empty()) {
				break;
			}
			if (rec.first.empty()) {
				signalParsingError(filePath, reader.lineNo(), "key may not be empty");
			}
			if (rec.second.empty()) {
				signalParsingError(filePath, reader.lineNo(), "value may not be empty");
			}

			if (rec.first == "name") {
				name = rec.second;
				if (name.empty()) {
					signalParsingError(filePath, reader.lineNo(), "'name' may not be empty");
				}
			} else if (rec.first == "format") {
				std::string formatStringCI = boost::algorithm::to_upper_copy(rec.second, std::locale::classic());
				if ((formatStringCI == "ASCII") ||
					(formatStringCI == "BINARY") ||
					(formatStringCI == "CDF")) {
					formatString = formatStringCI;
				} else {
					signalParsingError(filePath, reader.lineNo(), "Format name '" + rec.second + "' is not valid");
				}
			} else if (rec.first == "products") {
				boost::algorithm::split(productsList, rec.second, boost::is_any_of(",;"), boost::token_compress_on);
				if (productsList.empty()) {
					signalParsingError(filePath, reader.lineNo(), "'products' may not be empty");
				}
			} else {
				signalParsingError(filePath, reader.lineNo(), "unknown key");
			}

		}
	} catch (std::runtime_error& e) {
		signalParsingError(filePath, reader.lineNo(), e.what());
	}

	if (name.empty()) {
		signalParsingError(filePath, reader.lineNo(), "'name' may not be undefined");
	}

	if (productsList.empty()) {
		signalParsingError(filePath, reader.lineNo(), "'products' may not be undefined");
	}

	std::vector<ProductName> products;
	std::transform(productsList.begin(), productsList.end(), std::back_inserter(products),
	              [](const std::string& s) { return ProductName(s);});

	return Output(name, parseFormatString(formatString), products);
}

std::vector<std::string> cdownload::Parameters::allDatasetNames() const
{
	std::set<string> names;
	for (const Output& output: outputs_) {
		for (const auto& ds: output.datasetNames()) {
			names.insert(ds);
		}
	}

	std::vector<string> res;
	std::copy(names.begin(), names.end(), std::back_inserter(res));
	return res;
}

void cdownload::Parameters::addQualityFilter(const cdownload::ProductName& product, int minQuality)
{
	qualityFilters_.push_back({product, minQuality});
}

void cdownload::Parameters::addDensityFilter(cdownload::DensitySource source, double minDensity)
{
	densityFilters_.push_back({source, minDensity});
}

void cdownload::Parameters::onlyNightSide(bool v)
{
	onlyNightSide_ = v;
}

void cdownload::Parameters::timeRangesFileName(const path& v)
{
	timeRangesFileName_ = v;
}

void cdownload::Parameters::allowBlanks(bool v)
{
	allowBlanks_ = v;
}

void cdownload::Parameters::disableAveraging(bool v)
{
	disableAveraging_ = v;
}

void cdownload::Parameters::writeEpoch(bool v)
{
	writeEpoch_ = v;
}

void cdownload::Parameters::plasmaSheetFilter(bool v)
{
	plasmaSheetFilter_ = v;
}

void cdownload::Parameters::plasmaSheetMinR(double v)
{
	plasmaSheetMinR_ = v;
}

void cdownload::Parameters::spacecraftName(const string& name)
{
	spacecraftName_ = name;
}

namespace {
	void printOutput(std::ostream& os, const cdownload::Output& o,
		             const std::string& fieldDelim, const std::string& ident)
	{
		os << ident << "Name: " << o.name() << fieldDelim
				<< ident << "Format: ";
			printFormat(os, o.format());
			os << fieldDelim
				<< ident <<"Products: " << put_list(expandProductsMap(o.products())) << std::endl;
	}
}

std::ostream& cdownload::operator<<(std::ostream& os, const cdownload::Parameters& p)
{
	os << "Output dir: " << p.outputDir() << std::endl
		<< "Work dir: " << p.workDir() << std::endl
		<< "Time range: [" << p.startDate() << ", " << p.endDate() << ']' << std::endl
		<< "Interval: " << p.timeInterval() << std::endl
		<< "Cache dir: " << p.cacheDir() <<
			" (download missing: " << std::boolalpha << p.downloadMissingData() << ")" << std::endl
		<< "Options:" << std::endl
			<< '\t' << "night-side" << ": " << p.onlyNightSide() << std::endl
			<< '\t' << "allow-blanks" << ": " << p.allowBlanks() << std::endl
			<< '\t' << "plasma-sheet" << ": " << p.plasmaSheetFilter() << std::endl
			<< '\t' << "plasma-sheet-min-r" << ": " << p.plasmaSheetMinR() << std::endl
			<< '\t' << "valid-time-ranges" << ": " << p.timeRangesFileName() << std::endl
			<< '\t' << "no-averaging" << ": " << p.disableAveraging() << std::endl
			<< '\t' << "write-epoch-column" << ": " << p.writeEpoch() << std::endl
			<< '\t' << "quality filters" << ": " << put_list(p.qualityFilters()) << std::endl
			<< '\t' << "density filters" << ": " << put_list(p.densityyFilters()) << std::endl
			<< '\t' << "spacecraft" << ": " << p.spacecraftName() << std::endl

		<< "Outputs:" << std::endl;
		for (const Output& o: p.outputs()) {
			printOutput(os, o, "\n", "\t");
		}
	return os;
}


std::ostream& cdownload::operator<<(std::ostream& os, const cdownload::Output& o)
{
	printOutput(os, o, "\t", "");
	return os;
}

std::ostream& cdownload::operator<<(std::ostream& os, const cdownload::QualityFilterParameters& p)
{
	os << p.product << ':' << p.minQuality;
	return os;
}

std::ostream& cdownload::operator<<(std::ostream& os, const cdownload::DensityFilterParameters& p)
{
	os << p.source << ':' << p.minDensity;
	return os;
}

std::ostream& cdownload::operator<<(std::ostream& os, cdownload::DensitySource ds)
{
	switch(ds) {
		case DensitySource::CODIF:
			os << "CODIF";
			break;
		case DensitySource::HIA:
			os << "HIA";
			break;
	}
	return os;
}
