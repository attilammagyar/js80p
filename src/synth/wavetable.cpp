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

#ifndef JS80P__SYNTH__WAVETABLE_CPP
#define JS80P__SYNTH__WAVETABLE_CPP

#include <cmath>

#include "synth/wavetable.hpp"

#include "synth/math.hpp"


namespace JS80P
{

bool Wavetable::is_initialized = false;


Number Wavetable::sines[Wavetable::SIZE] = {0.0};


WavetableState::WavetableState()
{
}


void Wavetable::initialize()
{
    if (is_initialized) {
        return;
    }

    is_initialized = true;

    for (Integer j = 0; j != SIZE; ++j) {
        sines[j] = std::sin(((Number)j * SIZE_INV) * Math::PI_DOUBLE);
    }
}


void Wavetable::reset_state(
        WavetableState& state,
        Seconds const sampling_period,
        Frequency const nyquist_frequency,
        Frequency const frequency,
        Seconds const start_time_offset
) {
    state.sample_index = (
        SIZE_FLOAT * (Number)start_time_offset * (Number)frequency
    );
    state.scale = SIZE_FLOAT * (Number)sampling_period;
    state.nyquist_frequency = nyquist_frequency;
    state.interpolation_limit = nyquist_frequency * INTERPOLATION_LIMIT_SCALE;
}


Wavetable::Wavetable(
        Number const coefficients[],
        Integer const coefficients_length
) : partials(coefficients_length)
{
    samples = new Sample*[partials];

    for (Integer i = 0; i != partials; ++i) {
        samples[i] = new Sample[SIZE];
    }

    update_coefficients(coefficients);
    normalize();
}


void Wavetable::update_coefficients(Number const coefficients[])
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
            (Sample)(coefficients[0] * sines[(j * frequency) & MASK])
        );
    }

    for (Integer i = 1, prev_i = 0; i != partials; ++i, ++prev_i) {
        ++frequency;

        for (Integer j = 0; j != SIZE; ++j) {
            samples[i][j] = (
                samples[prev_i][j]
                + (Sample)(coefficients[i] * sines[(j * frequency) & MASK])
            );
        }
    }
}


void Wavetable::normalize()
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


Wavetable::~Wavetable()
{
    for (Integer i = 0; i != partials; ++i) {
        delete[] samples[i];
    }

    delete[] samples;

    samples = NULL;
}


Sample Wavetable::lookup(WavetableState* state, Frequency const frequency) const
{
    Frequency const abs_frequency = std::fabs(frequency);

    if (UNLIKELY(abs_frequency < 0.0000001)) {
        return 1.0;
    }

    if (UNLIKELY(abs_frequency > state->nyquist_frequency)) {
        return 0.0;
    }

    Number const sample_index = state->sample_index;

    state->sample_index = wrap_around(
        sample_index + state->scale * (Number)frequency
    );

    Integer const partials = this->partials;

    if (partials == 1) {
        state->needs_table_interpolation = false;
        state->table_indices[0] = 0;

        return interpolate(state, abs_frequency, sample_index);
    }

    Sample const max_partials = (
        (Sample)(state->nyquist_frequency / abs_frequency)
    );
    Integer const more_partials_index = (
        std::max((Integer)0, std::min(partials, (Integer)max_partials) - 1)
    );
    Integer const fewer_partials_index = (
        std::max((Integer)0, more_partials_index - 1)
    );

    state->table_indices[0] = fewer_partials_index;

    if (more_partials_index == fewer_partials_index) {
        state->needs_table_interpolation = false;

        return interpolate(state, abs_frequency, sample_index);
    }

    state->needs_table_interpolation = true;
    state->table_indices[1] = more_partials_index;
    state->fewer_partials_weight = max_partials - std::floor(max_partials);

    return interpolate(state, abs_frequency, sample_index);
}


Number Wavetable::wrap_around(Number const index) const
{
    return index - std::floor(index * SIZE_INV) * SIZE_FLOAT;
}


Sample Wavetable::interpolate(
        WavetableState const* state,
        Frequency const frequency,
        Number const sample_index
) const {
    if (frequency >= state->interpolation_limit) {
        return interpolate_sample_linear(state, sample_index);
    } else {
        return interpolate_sample_lagrange(state, sample_index);
    }
}


Sample Wavetable::interpolate_sample_linear(
        WavetableState const* state,
        Number const sample_index
) const {
    /*
    Not using Math::lookup_periodic() here, because we don't want to calculate
    the weight twice when interpolation between the two tables (fewer and more
    partials) is needed.
    */
    Sample const sample_2_weight = (
        (Sample)(sample_index - std::floor(sample_index))
    );
    Integer const sample_1_index = (Integer)sample_index;
    Integer const sample_2_index = (sample_1_index + 1) & MASK;

    Sample const* table_1 = samples[state->table_indices[0]];

    if (!state->needs_table_interpolation) {
        return Math::combine(
            sample_2_weight, table_1[sample_2_index], table_1[sample_1_index]
        );
    }

    Sample const* table_2 = samples[state->table_indices[1]];

    return Math::combine(
        state->fewer_partials_weight,
        Math::combine(
            sample_2_weight, table_1[sample_2_index], table_1[sample_1_index]
        ),
        Math::combine(
            sample_2_weight, table_2[sample_2_index], table_2[sample_1_index]
        )
    );
}


Sample Wavetable::interpolate_sample_lagrange(
        WavetableState const* state,
        Number const sample_index
) const {
    Integer const sample_1_index = (Integer)sample_index;
    Integer const sample_2_index = (sample_1_index + 1) & MASK;
    Integer const sample_3_index = (sample_2_index + 1) & MASK;

    Sample const* table_1 = samples[state->table_indices[0]];

    // Formula and notation from http://dlmf.nist.gov/3.3#ii

    Sample const f_1_1 = table_1[sample_1_index];
    Sample const f_1_2 = table_1[sample_2_index];
    Sample const f_1_3 = table_1[sample_3_index];

    Sample const t = (Sample)(sample_index - std::floor(sample_index));
    Sample const t_sqr = t * t;

    Sample const a_1 = 0.5 * (t_sqr - t);
    Sample const a_2 = 1.0 - t_sqr;
    Sample const a_3 = 0.5 * (t_sqr + t);

    if (!state->needs_table_interpolation) {
        return a_1 * f_1_1 + a_2 * f_1_2 + a_3 * f_1_3;
    }

    Sample const* table_2 = samples[state->table_indices[1]];

    Sample const f_2_1 = table_2[sample_1_index];
    Sample const f_2_2 = table_2[sample_2_index];
    Sample const f_2_3 = table_2[sample_3_index];

    return Math::combine(
        state->fewer_partials_weight,
        a_1 * f_1_1 + a_2 * f_1_2 + a_3 * f_1_3,
        a_1 * f_2_1 + a_2 * f_2_2 + a_3 * f_2_3
    );
}


StandardWaveforms const StandardWaveforms::standard_waveforms;


Wavetable const* StandardWaveforms::sine()
{
    return standard_waveforms.sine_wt;
}


Wavetable const* StandardWaveforms::sawtooth()
{
    return standard_waveforms.sawtooth_wt;
}


Wavetable const* StandardWaveforms::soft_sawtooth()
{
    return standard_waveforms.soft_sawtooth_wt;
}


Wavetable const* StandardWaveforms::inverse_sawtooth()
{
    return standard_waveforms.inverse_sawtooth_wt;
}


Wavetable const* StandardWaveforms::soft_inverse_sawtooth()
{
    return standard_waveforms.soft_inverse_sawtooth_wt;
}


Wavetable const* StandardWaveforms::triangle()
{
    return standard_waveforms.triangle_wt;
}


Wavetable const* StandardWaveforms::soft_triangle()
{
    return standard_waveforms.soft_triangle_wt;
}


Wavetable const* StandardWaveforms::square()
{
    return standard_waveforms.square_wt;
}


Wavetable const* StandardWaveforms::soft_square()
{
    return standard_waveforms.soft_square_wt;
}


StandardWaveforms::StandardWaveforms()
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
        square_coefficients[i] = (1 + plus_or_minus_one) * two_over_i_pi;
    }

    for (Integer i = 0; i != Wavetable::SOFT_PARTIALS; ++i) {
        Number const softener = 2.0 / (Number)(i + 2.0);
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
