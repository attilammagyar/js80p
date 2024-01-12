/*
 * This file is part of JS80P, a synthesizer plugin.
 * Copyright (C) 2023, 2024  Attila M. Magyar
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

#ifndef JS80P__DSP__PEAK_TRACKER_CPP
#define JS80P__DSP__PEAK_TRACKER_CPP

#include "dsp/peak_tracker.hpp"


namespace JS80P
{

PeakTracker::PeakTracker()
{
    reset();
}


void PeakTracker::reset() noexcept
{
    peak = 0.0;
    samples_since_previous_peak = 0;
}


Sample PeakTracker::get_peak() const noexcept
{
    return peak;
}


void PeakTracker::update(
        Sample const peak,
        Integer const peak_index,
        Integer const sample_count,
        Seconds const sampling_period
) noexcept {
    if (peak < this->peak) {
        Integer const samples_since_previous_peak = (
            this->samples_since_previous_peak + peak_index
        );
        Seconds const seconds_since_previous_peak = (
            sampling_period * (Seconds)samples_since_previous_peak
        );

        if (seconds_since_previous_peak >= RING_DOWN) {
            this->peak = peak;
        } else {
            Number const prev_peak_weight = (
                (RING_DOWN - seconds_since_previous_peak) * RING_DOWN_INV
            );

            this->peak = prev_peak_weight * (this->peak - peak) + peak;
        }
    } else {
        this->peak = peak;
    }

    this->samples_since_previous_peak = sample_count - peak_index;
}

}

#endif
