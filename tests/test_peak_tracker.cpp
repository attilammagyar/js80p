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

#include "test.cpp"
#include "utils.cpp"

#include "js80p.hpp"

#include "dsp/peak_tracker.cpp"


using namespace JS80P;


TEST(remembers_latest_peak, {
    PeakTracker peak_tracker;

    assert_eq(0.0, peak_tracker.get_peak(), DOUBLE_DELTA);

    peak_tracker.update(0.5, 64, 128, 0.001);
    assert_eq(0.5, peak_tracker.get_peak(), DOUBLE_DELTA);

    peak_tracker.update(0.9, 64, 128, 0.001);
    assert_eq(0.9, peak_tracker.get_peak(), DOUBLE_DELTA);

    peak_tracker.reset();
    assert_eq(0.0, peak_tracker.get_peak(), DOUBLE_DELTA);
})


TEST(decreasing_peaks_have_some_inertia, {
    constexpr Seconds sampling_period = 0.0002;
    constexpr Integer samples_until_half_ring_down = (
        PeakTracker::RING_DOWN / (2.0 * sampling_period)
    );

    PeakTracker peak_tracker;
    Integer samples_until_next_peak;
    Integer block_size;

    peak_tracker.update(1.0, 0, 1, sampling_period);

    block_size = 10;
    samples_until_next_peak = 0;
    peak_tracker.update(0.5, samples_until_next_peak, block_size, sampling_period);
    assert_eq(1.0, peak_tracker.get_peak(), 0.01);

    samples_until_next_peak = samples_until_half_ring_down - (block_size - samples_until_next_peak);
    block_size = 20;
    peak_tracker.update(0.5, samples_until_next_peak, block_size, sampling_period);
    assert_eq(0.75, peak_tracker.get_peak(), 0.01);

    samples_until_next_peak = samples_until_half_ring_down - (block_size - samples_until_next_peak);
    block_size = 30;
    peak_tracker.update(0.0, samples_until_next_peak, block_size, sampling_period);
    assert_eq(0.375, peak_tracker.get_peak(), 0.01);

    samples_until_next_peak = samples_until_half_ring_down - (block_size - samples_until_next_peak);
    block_size = 16;
    peak_tracker.update(0.0, samples_until_next_peak, block_size, sampling_period);
    assert_eq(0.1875, peak_tracker.get_peak(), 0.01);

    peak_tracker.update(0.123, 99999, 100000, sampling_period);
    assert_eq(0.123, peak_tracker.get_peak(), DOUBLE_DELTA);
})
