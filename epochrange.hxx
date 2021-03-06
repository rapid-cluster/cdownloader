/*
 * cdownload lib: downloads, unpacks, and reads data from the Cluster CSA arhive
 * Copyright (C) 2016-2017 Eugene Shalygin <eugene.shalygin@gmail.com>
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

#ifndef CDOWNLOAD_EPOCHRANGE_HXX
#define CDOWNLOAD_EPOCHRANGE_HXX

#include <cassert>

namespace cdownload {
	class EpochRange {
	public:
		using EpochType = double;
		EpochRange(EpochType mid, EpochType halfWidth)
			: midEpoch_(mid)
			, halfWidth_(halfWidth) {
		}

		static EpochRange fromRange(EpochType begin, EpochType end) {
			assert(end >= begin);
			return EpochRange((begin+end)/2, (end - begin)/2);
		}

		EpochType begin() const {
			return midEpoch_ - halfWidth_;
		}

		EpochType end() const {
			return midEpoch_ + halfWidth_;
		}

		EpochType mid() const {
			return midEpoch_;
		}

		EpochType halfWidth() const {
			return halfWidth_;
		}

		EpochType width() const {
			return halfWidth_ * 2;
		}

		bool contains(EpochType epoch) const {
			return (epoch >= midEpoch_ - halfWidth_) &&
				(epoch <= midEpoch_ + halfWidth_);
		}

	private:
		EpochType midEpoch_;
		EpochType halfWidth_;
	};
}

#endif // CDOWNLOAD_EPOCHRANGE_HXX
