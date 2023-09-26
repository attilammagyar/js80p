/*
 * This file is part of JS80P, a synthesizer plugin.
 * Copyright (C) 2023  Attila M. Magyar
 *
 * JS80P is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * JS80P is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef JS80P__DSP__PEAK_TRACKER_HPP
#define JS80P__DSP__PEAK_TRACKER_HPP

#include "js80p.hpp"


namespace JS80P
{

class PeakTracker
{
    public:
        static constexpr Seconds RING_DOWN = 0.015;

        PeakTracker();

        void update(
            Sample const peak,
            Integer const peak_index,
            Integer const sample_count,
            Seconds const sampling_period
        ) noexcept;

        Sample get_peak() const noexcept;

        void reset() noexcept;

    private:
        static constexpr Seconds RING_DOWN_INV = 1.0 / RING_DOWN;

        Sample peak;
        Integer samples_since_previous_peak;
};

}

#endif
