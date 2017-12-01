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

#ifndef CDOWNLOAD_CSATIME_HXX
#define CDOWNLOAD_CSATIME_HXX

#include "commonDefinitions.hxx"
#include <cassert>
#include <iosfwd>
#include <string>

namespace cdownload {

	class TimeDuration {
	public:
		TimeDuration(double milliseconds = 0.)
			: milliseconds_{milliseconds}
		{
			updateStringRep();
		}

		TimeDuration(int hours, int minutes, int seconds, int msec = 0)
			: TimeDuration{(hours* 3600. + minutes* 60. + seconds)*1e3 + msec}
		{
		}

		static TimeDuration fromString(const string& td);

		double seconds() const
		{
			return milliseconds_/1e3;
		}

		double milliseconds() const {
			return milliseconds_;
		}

		TimeDuration& operator*=(double k)
		{
			milliseconds_ *= k;
			updateStringRep();
			return *this;
		}

		TimeDuration& operator/=(double k)
		{
			milliseconds_ /= k;
			updateStringRep();
			return *this;
		}

		TimeDuration& operator+=(TimeDuration v)
		{
			milliseconds_ += v.milliseconds_;
			updateStringRep();
			return *this;
		}

		TimeDuration& operator-=(TimeDuration v)
		{
			milliseconds_ -= v.milliseconds_;
			updateStringRep();
			return *this;
		}

	private:
		double milliseconds_;
#ifndef NDEBUG
		void updateStringRep();
#else
		void updateStringRep(){}
#endif
#ifndef NDEBUG
		std::string str_;
#endif
	};

	inline bool operator<(TimeDuration left, TimeDuration right)
	{
		return left.milliseconds() < right.milliseconds();
	}

	inline bool operator<=(TimeDuration left, TimeDuration right)
	{
		return left.milliseconds() <= right.milliseconds();
	}

	inline bool operator>(TimeDuration left, TimeDuration right)
	{
		return left.milliseconds() > right.milliseconds();
	}

	inline bool operator>=(TimeDuration left, TimeDuration right)
	{
		return left.milliseconds() >= right.milliseconds();
	}

	inline TimeDuration operator*(TimeDuration d, double k)
	{
		return {d.milliseconds()* k};
	}

	inline TimeDuration operator/(TimeDuration d, double k)
	{
		return {d.milliseconds() / k};
	}

	std::ostream& operator<<(std::ostream& os, TimeDuration td);
	std::istream& operator>>(std::istream& is, TimeDuration& td);


	class DateTime {
	public:
		DateTime(double milliseconds = 0.)
			: milliseconds_{milliseconds}
		{
			assert(milliseconds_ >= 0);
			updateStringRep();
		}

		static DateTime fromString(const string& dt);
		static DateTime utcNow();

		const std::string CSAString() const;
		const std::string isoExtendedString() const;

		double seconds() const
		{
			return milliseconds_/1e3;
		}

		double milliseconds() const
		{
			return milliseconds_;
		}

		DateTime& operator+=(TimeDuration td)
		{
			milliseconds_ += td.milliseconds();
			updateStringRep();
			return *this;
		}

		struct Components {
			long year;
			long month;
			long day;
			long hour;
			long minute;
			long second;
			long msec;
		};

		Components breakdown() const;

	private:
#ifndef NDEBUG
		void updateStringRep();
#else
		void updateStringRep(){}
#endif
		double milliseconds_;
#ifndef NDEBUG
		std::string str_;
#endif
	};

	inline bool operator<(DateTime left, DateTime right)
	{
		return left.milliseconds() < right.milliseconds();
	}

	inline bool operator<=(DateTime left, DateTime right)
	{
		return left.milliseconds() <= right.milliseconds();
	}

	inline bool operator>(DateTime left, DateTime right)
	{
		return left.milliseconds() > right.milliseconds();
	}

	inline bool operator>=(DateTime left, DateTime right)
	{
		return left.milliseconds() >= right.milliseconds();
	}

	inline DateTime operator+(DateTime dt, TimeDuration td)
	{
		return DateTime(dt.milliseconds() + td.milliseconds());
	}

	inline DateTime operator-(DateTime dt, TimeDuration td)
	{
		return DateTime(dt.milliseconds() - td.milliseconds());
	}

	inline TimeDuration operator-(DateTime left, DateTime right)
	{
		return TimeDuration(left.milliseconds() - right.milliseconds());
	}

	std::ostream& operator<<(std::ostream& os, DateTime dt);
	std::istream& operator>>(std::istream& is, DateTime& dt);

	DateTime makeDateTime(unsigned year, unsigned month, unsigned day,
	                      unsigned hours = 0, unsigned minutes = 0, double seconds = 0.);
}

#endif // CDOWNLOAD_CSATIME_HXX
