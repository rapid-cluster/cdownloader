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

#ifndef CDOWNLOAD_AVERAGE_HXX
#define CDOWNLOAD_AVERAGE_HXX

#include <cstddef>
#include <vector>

#include <boost/accumulators/accumulators.hpp>

#include <boost/accumulators/statistics/count.hpp>
#include <boost/accumulators/statistics/weighted_mean.hpp>
#include <boost/accumulators/statistics/weighted_variance.hpp>

namespace cdownload {

	/**
	 * @brief Register accumulates and computes average and variance for a single scalar variable
	 *
	 */
	class AveragingRegister {
	public:

		typedef double mean_value_type;
		typedef double stddev_value_type;
		typedef double variance_value_type;
		typedef std::size_t counter_type;

		AveragingRegister();
		void reset();

		void add(double value, double weight = 1.);

		mean_value_type mean() const;
		counter_type count() const;
		variance_value_type variance() const;
		stddev_value_type stdDev() const;
		void scheduleReset();
	private:
		bool resetFlag_;
		using acc_type = boost::accumulators::accumulator_set<double,
			boost::accumulators::features<
				boost::accumulators::tag::weighted_mean,
				boost::accumulators::tag::weighted_variance,
				boost::accumulators::tag::count>,
			double>;
		acc_type acc_;
	};

	/**
	 * @brief Collection of @ref AveragingRegister to work with vector quantities
	 *
	 */
	class AveragedVariable {
	public:
		AveragedVariable(std::size_t numberOfComponents)
			: components_(numberOfComponents) {
		}

		std::size_t size() const {
			return components_.size();
		}

		AveragingRegister& operator[](std::size_t index) {
			return components_[index];
		}
		const AveragingRegister& operator[](std::size_t index) const {
			return components_[index];
		}

		typedef std::vector<AveragingRegister>::iterator iterator;
		typedef std::vector<AveragingRegister>::const_iterator const_iterator;

		iterator begin() {
			return components_.begin();
		}

		const_iterator begin() const {
			return components_.begin();
		}

		iterator end() {
			return components_.end();
		}

		const_iterator end() const {
			return components_.end();
		}

		void reset();
		void scheduleReset();
	private:
		std::vector<AveragingRegister> components_;
	};
}

#endif // CDOWNLOAD_AVERAGE_HXX
