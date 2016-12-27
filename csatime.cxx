/*
 * cdownload lib: downloads, unpacks, and reads data from the Cluster CSA arhive
 * Copyright (C) 2016 Eugene Shalygin <eugene.shalygin@gmail.com>
 *
 * Development was partially supported by the Volkswagen Foundation
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

#include "commonDefinitions.hxx"

#include <cdf.h>

#include <cctype>
#include <cmath>
#include <ctime>
#include <iostream>
#include <iomanip>
#include <ios>
#include <locale>
#include <sstream>

cdownload::DateTime cdownload::DateTime::fromString(const string& dt)
{
	std::istringstream is(dt);
	DateTime res;
	is >> res;
	return res;
}

cdownload::DateTime cdownload::DateTime::utcNow()
{
	std::time_t now = std::time(0);
	std::tm utcNow;
	gmtime_r(&now, &utcNow);
	return makeDateTime(static_cast<unsigned>(utcNow.tm_year + 1900),
	                    static_cast<unsigned>(utcNow.tm_mon),
	                    static_cast<unsigned>(utcNow.tm_mday),
	                    static_cast<unsigned>(utcNow.tm_hour),
	                    static_cast<unsigned>(utcNow.tm_min), utcNow.tm_sec);
}

const cdownload::string cdownload::DateTime::CSAString() const
{
	std::ostringstream os;
	os << *this;
	return os.str();
}

const std::string cdownload::DateTime::isoExtendedString() const
{
	char buf[EPOCH4_STRING_LEN + 1];
	buf[EPOCH4_STRING_LEN] = 0;
	encodeEPOCH4(milliseconds_, buf);
	return std::string(buf);
}

std::ostream& cdownload::operator<<(std::ostream& os, cdownload::DateTime dt)
{
	// The CDF_EPOCH and CDF_EPOCH16 data types are used to store date and time values referenced
	// from a particular epoch. For CDF that epoch is 01-Jan-0000 00:00:00.000 and
	// 01-Jan-0000 00:00:00.000.000.000.000, respectively. CDF_EPOCH values are the number of
	// milliseconds since the epoch.

	long year; // Year (AD, e.g., 1994)
	long month; //  Month (1-12).
	long day; // Day (1-31).
	long hour; // Hour (0-23).
	long minute; // Minute (0-59).
	long second; //  Second (0-59).
	long msec; // Millisecond (0-999).

	EPOCHbreakdown(dt.milliseconds(), &year, &month, &day, &hour, &minute, &second, &msec);

	const auto fill = os.fill();
	os << std::setfill('0') << std::setw(4) << year
	   << '-' << std::setw(2) << month
	   << '-' << std::setw(2) << day << 'T'
	   << std::setw(2) << hour
	   << ':' << std::setw(2) << minute
	   << ':' << std::setw(2) << second
	   << '.' << std::setw(3) << msec << 'Z'
	   << std::setfill(fill);
	return os;
}

std::istream& cdownload::operator>>(std::istream& is, cdownload::DateTime& dt)
{
	char delimYM, delimMD;
	int year; // Year (AD, e.g., 1994)
	int month; //  Month (1-12).
	int day; // Day (1-31).
	is >> year >> delimYM >> month >> delimMD >> day;
	const char dateDelim = '-';
	if ((delimYM != dateDelim) || (delimMD != dateDelim)) {
		throw std::runtime_error("Can not parse date string");
	}

	auto nextChar = is.peek();
	if (nextChar == ' ') {
		is.get();
	} else {
		if (!std::isdigit(nextChar)) {
			char delimDT;
			is >> delimDT;
			if (delimDT != 'T') {
				throw std::runtime_error("Can not parse date-time delimiter");
			}
		}
	}

	TimeDuration td;
	is >> td;

	const double res = computeEPOCH(year, month, day, 0, 0, 0, 0);
	if (res < 0 ) { // copmuteEPOCH() may return  ILLEGAL_EPOCH_VALUE (-1)
		throw std::runtime_error("Invalid datetime, can not be converted to CDF EPOCH");
	}

	dt = DateTime(res + td.milliseconds());
	return is;
}

cdownload::TimeDuration cdownload::TimeDuration::fromString(const string& td)
{
	std::istringstream is(td);
	TimeDuration res;
	is >> res;
	return res;
}

std::ostream& cdownload::operator<<(std::ostream& os, cdownload::TimeDuration td)
{
	const double SECONDS_IN_HOUR = 3600.;
	const double SECONDS_IN_MINUTE = 60.;
	double duration = td.seconds();
	if (duration < 0) {
		os << '-';
		duration = -duration;
	}
	int hours = std::trunc(duration / SECONDS_IN_HOUR);
	duration = duration - hours * SECONDS_IN_HOUR;
	const auto fill = os.fill();
	os << std::setfill('0');
	os << std::setw(2) << hours << ':';
	int minutes = std::trunc(duration / SECONDS_IN_MINUTE);
	duration = duration - minutes * SECONDS_IN_MINUTE;
	os << std::setw(2) << minutes << ':';
	const auto precision = os.precision();
	os << std::fixed << std::setprecision(3) << duration
	   << std::defaultfloat << std::setprecision(precision) << std::setfill(fill);
	return os;
}

std::istream& cdownload::operator>>(std::istream& is, cdownload::TimeDuration& td)
{
	char delimHM, delimMS, trailing;
	int hour; // Hour (0-23).
	int minute; // Minute (0-59).
	int second; //  Second (0-59).
	int msec = 0; // Millisecond (0-999).
	is >> hour >> delimHM >> minute >> delimMS >> second;
	const char timeDelim = ':';
	if ((delimHM != timeDelim) || (delimMS != timeDelim)) {
		throw std::runtime_error("Can not parse time string");
	}
	// are there milliseconds?
	if (!is.eof()) {
		auto next = is.peek();
		if (next == '.') {
			is.get();
			is >> msec;
		}
	}

	if (!is.eof()) {
		trailing = is.peek();
		if (trailing == 'Z') {
			is.get();
		}
	}

	const double res = computeEPOCH(0, 1, 1, hour, minute, second, msec);
	if (res < 0 ) { // copmuteEPOCH() may return  ILLEGAL_EPOCH_VALUE (-1)
		throw std::runtime_error("Invalid datetime, can not be converted to CDF EPOCH");
	}

	td = TimeDuration(res);
	return is;
}

cdownload::DateTime cdownload::makeDateTime(unsigned int year, unsigned int month, unsigned int day,
                                            unsigned int hours, unsigned int minutes, double seconds)
{
	const double res = computeEPOCH(year, month, day, hours, minutes,
	                                static_cast<long>(seconds), (seconds - static_cast<long>(seconds)) * 1e3);
	if (res < 0 ) { // copmuteEPOCH() may return  ILLEGAL_EPOCH_VALUE (-1)
		throw std::runtime_error("Invalid datetime, can not be converted to CDF EPOCH");
	}
	return {res};
}
