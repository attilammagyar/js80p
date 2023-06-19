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

#ifndef JS80P__DSP__WAVETABLE_HPP
#define JS80P__DSP__WAVETABLE_HPP

#include <string>

#include "js80p.hpp"


namespace JS80P
{

class WavetableState
{
    public:
        WavetableState() noexcept;

        Number scale;
        Number sample_index;
        Number fewer_partials_weight;
        Frequency nyquist_frequency;
        Frequency interpolation_limit;
        Integer table_indices[2];
        bool needs_table_interpolation;
};


class Wavetable
{
    /*
    https://www.music.mcgill.ca/~gary/307/week4/wavetables.html
    https://www.music.mcgill.ca/~gary/307/week5/node12.html
    */

    public:
        /*
        The Nyquist limit for 48 kHz sampling rate is 24 kHz, which
        can represent up to 384 partials of a 62.5 Hz sawtooth wave.
        So with 384 partials, we only start to loose high frequencies
        for notes below B1.
        */
        static constexpr Integer PARTIALS = 384;
        static constexpr Integer SOFT_PARTIALS = PARTIALS / 2;

        static void initialize() noexcept;

        static void reset_state(
            WavetableState& state,
            Seconds const sampling_period,
            Frequency const nyquist_frequency,
            Frequency const frequency,
            Seconds const time_offset
        ) noexcept;

        static Number scale_phase_offset(Number const phase_offset) noexcept;

        Wavetable(
            Number const coefficients[],
            Integer const coefficients_length
        ) noexcept;

        ~Wavetable();

        Sample lookup(
            WavetableState& state,
            Frequency const frequency,
            Number const phase_offset
        ) const noexcept;

        void update_coefficients(Number const coefficients[]) noexcept;
        void normalize() noexcept;

    private:
        /*
        24 Hz at 48 kHz sampling rate has a wavelength of 2000 samples,
        so 2048 samples per waveform with linear interpolation should be
        good enough for most of the audible spectrum.

        Better interpolation is needed though when frequency is
        significantly lower than sample_rate / SIZE.
        */
        static constexpr Integer SIZE = 0x0800;
        static constexpr Integer MASK = 0x07ff;

        static constexpr Number SIZE_FLOAT = (Number)SIZE;
        static constexpr Number SIZE_INV = 1.0 / SIZE_FLOAT;
        static constexpr Frequency INTERPOLATION_LIMIT_SCALE = (
            1.0 / (2.0 * (Frequency)SIZE_FLOAT)
        );

        static Number sines[SIZE];
        static bool is_initialized;

        Number wrap_around(Number const index) const noexcept;

        Sample interpolate(
            WavetableState const& state,
            Frequency const frequency,
            Number const sample_index
        ) const noexcept;

        Sample interpolate_sample_linear(
            WavetableState const& state,
            Number const sample_index
        ) const noexcept;

        Sample interpolate_sample_lagrange(
            WavetableState const& state,
            Number const sample_index
        ) const noexcept;

        Integer const partials;

        Sample** samples;
};


class StandardWaveforms
{
    public:
        static Wavetable const* sine() noexcept;
        static Wavetable const* sawtooth() noexcept;
        static Wavetable const* soft_sawtooth() noexcept;
        static Wavetable const* inverse_sawtooth() noexcept;
        static Wavetable const* soft_inverse_sawtooth() noexcept;
        static Wavetable const* triangle() noexcept;
        static Wavetable const* soft_triangle() noexcept;
        static Wavetable const* square() noexcept;
        static Wavetable const* soft_square() noexcept;

        StandardWaveforms() noexcept;
        ~StandardWaveforms();

    private:
        static StandardWaveforms const standard_waveforms;

        Wavetable const* sine_wt;
        Wavetable const* sawtooth_wt;
        Wavetable const* soft_sawtooth_wt;
        Wavetable const* inverse_sawtooth_wt;
        Wavetable const* soft_inverse_sawtooth_wt;
        Wavetable const* triangle_wt;
        Wavetable const* soft_triangle_wt;
        Wavetable const* square_wt;
        Wavetable const* soft_square_wt;
};

}

#endif
