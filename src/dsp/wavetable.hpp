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

#ifndef JS80P__DSP__WAVETABLE_HPP
#define JS80P__DSP__WAVETABLE_HPP

#include <string>
#include <type_traits>

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
};


class Wavetable
{
    /*
    https://www.music.mcgill.ca/~gary/307/week4/wavetables.html
    https://www.music.mcgill.ca/~gary/307/week5/node12.html
    */

    public:
        enum Interpolation {
            DYNAMIC = 0,
            LINEAR_ONLY = 1,
            LAGRANGE_ONLY = 2,
        };

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

        Wavetable(Wavetable const& wavetable) = delete;
        Wavetable(Wavetable&& wavetable) = delete;

        Wavetable& operator=(Wavetable const& wavetable) = delete;
        Wavetable& operator=(Wavetable&& wavetable) = delete;

        template<
            Interpolation interpolation = Interpolation::DYNAMIC,
            bool single_partial = false,
            bool with_subharmonic = false
        >
        void lookup(
            WavetableState& state,
            Frequency const frequency,
            Number const phase_offset,
            Sample& sample,
            Sample& subharmonic_sample
        ) const noexcept;

        void update_coefficients(Number const coefficients[]) noexcept;
        void normalize() noexcept;

        Interpolation select_interpolation(
            Frequency const frequency,
            Frequency const nyquist_frequency
        ) const noexcept;

        bool has_single_partial() const noexcept;

    private:
        /*
        24 Hz at 48 kHz sampling rate has a wavelength of 2000 samples,
        so 2048 samples per waveform with linear interpolation should be
        good enough for most of the audible spectrum.

        Better interpolation is needed though when frequency is
        significantly lower than sample_rate / PERIOD_SIZE.

        In order to be able to interpolate both the fundamental and the first
        subharmonic (when needed) in a single step, the table size is doubled
        so it holds 2 periods of the fundamental.
        */
        static constexpr Integer PERIOD_SIZE = 0x0800;
        static constexpr Integer PERIOD_INDEX_MASK = PERIOD_SIZE - 1;
        static constexpr Integer SIZE = PERIOD_SIZE * 2;
        static constexpr Integer TABLE_INDEX_MASK = SIZE - 1;

        static constexpr Number PERIOD_SIZE_FLOAT = (Number)PERIOD_SIZE;
        static constexpr Number PERIOD_SIZE_INV = 1.0 / PERIOD_SIZE_FLOAT;

        static constexpr Number SIZE_FLOAT = (Number)SIZE;
        static constexpr Number SIZE_INV = 1.0 / SIZE_FLOAT;

        static constexpr Frequency INTERPOLATION_LIMIT_SCALE = (
            1.0 / (2.0 * (Frequency)PERIOD_SIZE_FLOAT)
        );

        static Number subharmonic[SIZE];
        static Number sines[SIZE];
        static bool is_initialized;

        template<bool with_subharmonic>
        static constexpr Integer get_index_mask() noexcept;

        template<
            Interpolation interpolation,
            bool table_interpolation,
            bool with_subharmonic
        >
        void interpolate(
            WavetableState const& state,
            Frequency const frequency,
            Number const sample_index,
            Sample& sample,
            Sample& subharmonic_sample
        ) const noexcept;

        template<bool table_interpolation, bool with_subharmonic>
        void interpolate_sample_linear(
            WavetableState const& state,
            Number const sample_index,
            Sample& sample,
            Sample& subharmonic_sample
        ) const noexcept;

        template<bool table_interpolation, bool with_subharmonic>
        void interpolate_sample_lagrange(
            WavetableState const& state,
            Number const sample_index,
            Sample& sample,
            Sample& subharmonic_sample
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

        StandardWaveforms(
            StandardWaveforms const& standard_waveforms
        ) = delete;

        StandardWaveforms(
            StandardWaveforms&& standard_waveforms
        ) = delete;

        StandardWaveforms& operator=(
            StandardWaveforms const& standard_waveforms
        ) = delete;

        StandardWaveforms& operator=(
            StandardWaveforms&& standard_waveforms
        ) = delete;

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
