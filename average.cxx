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

#include "average.hxx"

#include <cmath>

namespace ba = boost::accumulators;

cdownload::AveragingRegister::AveragingRegister()
{
	reset();
}

void cdownload::AveragingRegister::reset()
{
	acc_ = acc_type();
	resetFlag_ = false;
}

void cdownload::AveragingRegister::add(double value, double aweight)
{
	if (resetFlag_) {
		reset(); // will reset the flag too
	}
	acc_(value, ba::weight = aweight);
}

cdownload::AveragingRegister::mean_value_type cdownload::AveragingRegister::mean() const
{
	return ba::extract::weighted_mean(acc_);
}

cdownload::AveragingRegister::counter_type cdownload::AveragingRegister::count() const
{
	return ba::extract::count(acc_);
}

cdownload::AveragingRegister::variance_value_type cdownload::AveragingRegister::variance() const
{
	return ba::extract::weighted_variance(acc_);
}

cdownload::AveragingRegister::stddev_value_type cdownload::AveragingRegister::stdDev() const
{
	return std::sqrt(variance());
}

void cdownload::AveragingRegister::scheduleReset()
{
	resetFlag_ = true;
}

void cdownload::AveragedVariable::reset()
{
	for (AveragingRegister& ar: components_) {
		ar.reset();
	}
}

void cdownload::AveragedVariable::scheduleReset()
{
	for (AveragingRegister& ar: components_) {
		ar.scheduleReset();
	}
}
