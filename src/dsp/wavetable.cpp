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

#ifndef JS80P__DSP__WAVETABLE_CPP
#define JS80P__DSP__WAVETABLE_CPP

#include <cmath>

#include "dsp/wavetable.hpp"

#include "dsp/math.hpp"


namespace JS80P
{

bool Wavetable::is_initialized = false;


Number Wavetable::subharmonic[Wavetable::SIZE] = {0.0};

Number Wavetable::sines[Wavetable::SIZE] = {0.0};


WavetableState::WavetableState() noexcept
    : scale(1.0),
    sample_index(0.0),
    fewer_partials_weight(1.0),
    nyquist_frequency(22100.0),
    interpolation_limit(10.0),
    table_indices{0, 0}
{
}


void Wavetable::initialize() noexcept
{
    if (is_initialized) {
        return;
    }

    is_initialized = true;

    for (Integer j = 0; j != SIZE; ++j) {
        subharmonic[j] = std::sin(((Number)j * SIZE_INV) * Math::PI_DOUBLE);
        sines[j] = std::sin(((Number)j * PERIOD_SIZE_INV) * Math::PI_DOUBLE);
    }
}


void Wavetable::reset_state(
        WavetableState& state,
        Seconds const sampling_period,
        Frequency const nyquist_frequency,
        Frequency const frequency,
        Seconds const start_time_offset
) noexcept {
    state.sample_index = (
        PERIOD_SIZE_FLOAT * (Number)start_time_offset * (Number)frequency
    );
    state.scale = PERIOD_SIZE_FLOAT * (Number)sampling_period;
    state.nyquist_frequency = nyquist_frequency;
    state.interpolation_limit = nyquist_frequency * INTERPOLATION_LIMIT_SCALE;
}


Number Wavetable::scale_phase_offset(Number const phase_offset) noexcept
{
    return phase_offset * PERIOD_SIZE_FLOAT;
}


Wavetable::Wavetable(
        Number const coefficients[],
        Integer const coefficients_length
) noexcept : partials(coefficients_length)
{
    samples = new Sample*[partials];

    for (Integer i = 0; i != partials; ++i) {
        samples[i] = new Sample[SIZE];
    }

    update_coefficients(coefficients);
    normalize();
}


void Wavetable::update_coefficients(Number const coefficients[]) noexcept
{
    Integer frequency = 1;

    /*
    samples[0]: 0 partials above fundamental
    samples[1]: 1 partial above fundamental
    ...
    samples[n]: n partials above fundamental
    */

    for (Integer j = 0; j != SIZE; ++j) {
        samples[0][j] = (
            (Sample)(coefficients[0] * sines[(j * frequency) & TABLE_INDEX_MASK])
        );
    }

    for (Integer i = 1, prev_i = 0; i != partials; ++i, ++prev_i) {
        ++frequency;

        for (Integer j = 0; j != SIZE; ++j) {
            samples[i][j] = (
                samples[prev_i][j]
                + (Sample)(coefficients[i] * sines[(j * frequency) & TABLE_INDEX_MASK])
            );
        }
    }
}


void Wavetable::normalize() noexcept
{
    Sample max = 0.0;

    for (Integer i = 0; i != partials; ++i) {
        for (Integer j = 0; j != SIZE; ++j) {
            Sample const sample = std::fabs(samples[i][j]);

            if (sample > max) {
                max = sample;
            }
        }
    }

    for (Integer i = 0; i != partials; ++i) {
        for (Integer j = 0; j != SIZE; ++j) {
            samples[i][j] /= max;
        }
    }
}


Wavetable::Interpolation Wavetable::select_interpolation(
        Frequency const frequency,
        Frequency const nyquist_frequency
) const noexcept {
    return (
        std::fabs(frequency) >= nyquist_frequency * INTERPOLATION_LIMIT_SCALE
            ? Interpolation::LINEAR_ONLY
            : Interpolation::LAGRANGE_ONLY
    );
}


bool Wavetable::has_single_partial() const noexcept
{
    return partials == 1;
}


Wavetable::~Wavetable()
{
    for (Integer i = 0; i != partials; ++i) {
        delete[] samples[i];
    }

    delete[] samples;

    samples = NULL;
}


template<Wavetable::Interpolation interpolation, bool single_partial, bool with_subharmonic>
void Wavetable::lookup(
        WavetableState& state,
        Frequency const frequency,
        Number const phase_offset,
        Sample& sample,
        Sample& subharmonic_sample
) const noexcept {
    Frequency const abs_frequency = std::fabs(frequency);

    if (JS80P_UNLIKELY(abs_frequency < 0.0000001)) {
        sample = 1.0;

        return;
    }

    if (JS80P_UNLIKELY(abs_frequency > state.nyquist_frequency)) {
        sample = 0.0;

        return;
    }

    Number const sample_index = state.sample_index;

    state.sample_index += state.scale * (Number)frequency;

    if constexpr (single_partial) {
        state.table_indices[0] = 0;

        interpolate<interpolation, false, with_subharmonic>(
            state, abs_frequency, sample_index + phase_offset, sample, subharmonic_sample
        );
    } else {
        Sample const max_partials = (
            (Sample)(state.nyquist_frequency / abs_frequency)
        );
        Integer const more_partials_index = (
            std::max((Integer)0, std::min(this->partials, (Integer)max_partials) - 1)
        );
        Integer const fewer_partials_index = (
            std::max((Integer)0, more_partials_index - 1)
        );

        state.table_indices[0] = fewer_partials_index;

        if (more_partials_index == fewer_partials_index) {
            interpolate<interpolation, false, with_subharmonic>(
                state, abs_frequency, sample_index + phase_offset, sample, subharmonic_sample
            );

            return;
        }

        state.table_indices[1] = more_partials_index;
        state.fewer_partials_weight = max_partials - std::floor(max_partials);

        interpolate<interpolation, true, with_subharmonic>(
            state, abs_frequency, sample_index + phase_offset, sample, subharmonic_sample
        );
    }
}


template<Wavetable::Interpolation interpolation, bool table_interpolation, bool with_subharmonic>
void Wavetable::interpolate(
        WavetableState const& state,
        Frequency const frequency,
        Number const sample_index,
        Sample& sample,
        Sample& subharmonic_sample
) const noexcept {
    if constexpr (interpolation == Interpolation::LINEAR_ONLY) {
        interpolate_sample_linear<table_interpolation, with_subharmonic>(
            state, sample_index, sample, subharmonic_sample
        );
    } else if constexpr (interpolation == Interpolation::LAGRANGE_ONLY) {
        interpolate_sample_lagrange<table_interpolation, with_subharmonic>(
            state, sample_index, sample, subharmonic_sample
        );
    } else {
        if (JS80P_LIKELY(frequency >= state.interpolation_limit)) {
            interpolate_sample_linear<table_interpolation, with_subharmonic>(
                state, sample_index, sample, subharmonic_sample
            );
        } else {
            interpolate_sample_lagrange<table_interpolation, with_subharmonic>(
                state, sample_index, sample, subharmonic_sample
            );
        }
    }
}


template<bool table_interpolation, bool with_subharmonic>
void Wavetable::interpolate_sample_linear(
        WavetableState const& state,
        Number const sample_index,
        Sample& sample,
        Sample& subharmonic_sample
) const noexcept {
    /*
    Not using Math::lookup_periodic() here, because we don't want to calculate
    the weight twice when interpolation between the two tables (fewer and more
    partials) is needed.
    */
    Sample const sample_2_weight = (
        (Sample)(sample_index - std::floor(sample_index))
    );
    Integer const mask = get_index_mask<with_subharmonic>();
    Integer const sample_1_index = (Integer)sample_index & mask;
    Integer const sample_2_index = (sample_1_index + 1) & mask;

    Sample const* const table_1 = samples[state.table_indices[0]];

    if constexpr (table_interpolation) {
        Sample const* const table_2 = samples[state.table_indices[1]];

        sample = Math::combine(
            state.fewer_partials_weight,
            Math::combine(
                sample_2_weight, table_1[sample_2_index], table_1[sample_1_index]
            ),
            Math::combine(
                sample_2_weight, table_2[sample_2_index], table_2[sample_1_index]
            )
        );
    } else {
        sample = Math::combine(
            sample_2_weight, table_1[sample_2_index], table_1[sample_1_index]
        );
    }

    if constexpr (with_subharmonic) {
        subharmonic_sample = Math::combine(
            sample_2_weight, subharmonic[sample_2_index], subharmonic[sample_1_index]
        );
    }
}


template<bool table_interpolation, bool with_subharmonic>
void Wavetable::interpolate_sample_lagrange(
        WavetableState const& state,
        Number const sample_index,
        Sample& sample,
        Sample& subharmonic_sample
) const noexcept {
    Integer const mask = get_index_mask<with_subharmonic>();
    Integer const sample_1_index = (Integer)sample_index & mask;
    Integer const sample_2_index = (sample_1_index + 1) & mask;
    Integer const sample_3_index = (sample_1_index + 2) & mask;

    Sample const* const table_1 = samples[state.table_indices[0]];

    /* Formula and notation from http://dlmf.nist.gov/3.3#ii */

    Sample const f_1_1 = table_1[sample_1_index];
    Sample const f_1_2 = table_1[sample_2_index];
    Sample const f_1_3 = table_1[sample_3_index];

    Sample const t = (Sample)(sample_index - std::floor(sample_index));
    Sample const t_sqr = t * t;

    Sample const a_1 = 0.5 * (t_sqr - t);
    Sample const a_2 = 1.0 - t_sqr;
    Sample const a_3 = 0.5 * (t_sqr + t);

    if constexpr (table_interpolation) {
        Sample const* const table_2 = samples[state.table_indices[1]];

        Sample const f_2_1 = table_2[sample_1_index];
        Sample const f_2_2 = table_2[sample_2_index];
        Sample const f_2_3 = table_2[sample_3_index];

        sample = Math::combine(
            state.fewer_partials_weight,
            a_1 * f_1_1 + a_2 * f_1_2 + a_3 * f_1_3,
            a_1 * f_2_1 + a_2 * f_2_2 + a_3 * f_2_3
        );
    } else {
        sample = a_1 * f_1_1 + a_2 * f_1_2 + a_3 * f_1_3;
    }

    if constexpr (with_subharmonic) {
        Sample const sh_f_1_1 = subharmonic[sample_1_index];
        Sample const sh_f_1_2 = subharmonic[sample_2_index];
        Sample const sh_f_1_3 = subharmonic[sample_3_index];

        subharmonic_sample = a_1 * sh_f_1_1 + a_2 * sh_f_1_2 + a_3 * sh_f_1_3;
    }
}


template<bool with_subharmonic>
constexpr Integer Wavetable::get_index_mask() noexcept
{
    if constexpr (with_subharmonic) {
        return TABLE_INDEX_MASK;
    } else {
        /*
        The lookup table for the fundamental contains 2 periods of the wave, and
        we have another table with the same size holding a single period of the
        subharmonic. This way we can calculate both waves in a single
        interpolation step.

        However, when we don't need the subharmonic, we can restrict our lookup
        to only the first half of the table for the fundamental, leaving more
        room in CPU caches for other data.
        */
        return PERIOD_INDEX_MASK;
    }
}


StandardWaveforms const StandardWaveforms::standard_waveforms;


Wavetable const* StandardWaveforms::sine() noexcept
{
    return standard_waveforms.sine_wt;
}


Wavetable const* StandardWaveforms::sawtooth() noexcept
{
    return standard_waveforms.sawtooth_wt;
}


Wavetable const* StandardWaveforms::soft_sawtooth() noexcept
{
    return standard_waveforms.soft_sawtooth_wt;
}


Wavetable const* StandardWaveforms::inverse_sawtooth() noexcept
{
    return standard_waveforms.inverse_sawtooth_wt;
}


Wavetable const* StandardWaveforms::soft_inverse_sawtooth() noexcept
{
    return standard_waveforms.soft_inverse_sawtooth_wt;
}


Wavetable const* StandardWaveforms::triangle() noexcept
{
    return standard_waveforms.triangle_wt;
}


Wavetable const* StandardWaveforms::soft_triangle() noexcept
{
    return standard_waveforms.soft_triangle_wt;
}


Wavetable const* StandardWaveforms::square() noexcept
{
    return standard_waveforms.square_wt;
}


Wavetable const* StandardWaveforms::soft_square() noexcept
{
    return standard_waveforms.soft_square_wt;
}


StandardWaveforms::StandardWaveforms() noexcept
{
    Wavetable::initialize();

    Number sine_coefficients[] = {1.0};
    Number sawtooth_coefficients[Wavetable::PARTIALS];
    Number soft_sawtooth_coefficients[Wavetable::SOFT_PARTIALS];
    Number inverse_sawtooth_coefficients[Wavetable::PARTIALS];
    Number soft_inverse_sawtooth_coefficients[Wavetable::SOFT_PARTIALS];
    Number triangle_coefficients[Wavetable::PARTIALS];
    Number soft_triangle_coefficients[Wavetable::SOFT_PARTIALS];
    Number square_coefficients[Wavetable::PARTIALS];
    Number soft_square_coefficients[Wavetable::SOFT_PARTIALS];

    for (Integer i = 0; i != Wavetable::PARTIALS; ++i) {
        Number const plus_or_minus_one = ((i & 1) == 1 ? -1.0 : 1.0);
        Number const i_pi = (Number)(i + 1) * Math::PI;
        Number const two_over_i_pi = 2.0 / i_pi;

        sawtooth_coefficients[i] = plus_or_minus_one * two_over_i_pi;
        inverse_sawtooth_coefficients[i] = -sawtooth_coefficients[i];
        triangle_coefficients[i] = (
            8.0 * std::sin(i_pi / 2.0) / (i_pi * i_pi)
        );
        square_coefficients[i] = (1.0 + plus_or_minus_one) * two_over_i_pi;
    }

    for (Integer i = 0; i != Wavetable::SOFT_PARTIALS; ++i) {
        Number const softener = 5.0 / (Number)(i + 5.0);
        soft_sawtooth_coefficients[i] = softener * sawtooth_coefficients[i];
        soft_inverse_sawtooth_coefficients[i] = -soft_sawtooth_coefficients[i];
        soft_triangle_coefficients[i] = softener * triangle_coefficients[i];
        soft_square_coefficients[i] = softener * square_coefficients[i];
    }

    sine_wt = new Wavetable(sine_coefficients, 1);
    sawtooth_wt = new Wavetable(sawtooth_coefficients, Wavetable::PARTIALS);
    soft_sawtooth_wt = new Wavetable(
        soft_sawtooth_coefficients, Wavetable::SOFT_PARTIALS
    );
    inverse_sawtooth_wt = new Wavetable(
        inverse_sawtooth_coefficients, Wavetable::PARTIALS
    );
    soft_inverse_sawtooth_wt = new Wavetable(
        soft_inverse_sawtooth_coefficients, Wavetable::SOFT_PARTIALS
    );
    triangle_wt = new Wavetable(triangle_coefficients, Wavetable::PARTIALS);
    soft_triangle_wt = new Wavetable(
        soft_triangle_coefficients, Wavetable::SOFT_PARTIALS
    );
    square_wt = new Wavetable(square_coefficients, Wavetable::PARTIALS);
    soft_square_wt = new Wavetable(
        soft_square_coefficients, Wavetable::SOFT_PARTIALS
    );
}


StandardWaveforms::~StandardWaveforms()
{
    delete sine_wt;
    delete sawtooth_wt;
    delete soft_sawtooth_wt;
    delete inverse_sawtooth_wt;
    delete soft_inverse_sawtooth_wt;
    delete triangle_wt;
    delete soft_triangle_wt;
    delete square_wt;
    delete soft_square_wt;

    sine_wt = NULL;
    sawtooth_wt = NULL;
    soft_sawtooth_wt = NULL;
    inverse_sawtooth_wt = NULL;
    soft_inverse_sawtooth_wt = NULL;
    triangle_wt = NULL;
    soft_triangle_wt = NULL;
    square_wt = NULL;
    soft_square_wt = NULL;
}

}

#endif
