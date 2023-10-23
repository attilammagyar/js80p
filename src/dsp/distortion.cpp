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

#ifndef JS80P__DSP__DISTORTION_CPP
#define JS80P__DSP__DISTORTION_CPP

#include <cmath>

#include "dsp/distortion.hpp"

#include "dsp/math.hpp"


namespace JS80P { namespace Distortion
{

Tables tables;


Tables::Tables()
{
    fill_tables(Type::SOFT, 3.0);
    fill_tables(Type::HEAVY, 10.0);
}


void Tables::fill_tables(Type const type, Number const steepness) noexcept
{
    Sample const steepness_inv_double = 2.0 / steepness;

    Table& f_table = f_tables[(int)type];
    Table& F0_table = F0_tables[(int)type];

    for (Integer i = 0; i != SIZE; ++i) {
        Number const x = INPUT_MAX * ((Sample)i * SIZE_INV);
        f_table[i] = std::tanh(steepness * x * 0.5);
        F0_table[i] = (
            x + steepness_inv_double * std::log(std::exp(-steepness * x) + 1.0)
        );
    }
}


Table const& Tables::get_f_table(Type const type) const noexcept
{
    return f_tables[type];
}


Table const& Tables::get_F0_table(Type const type) const noexcept
{
    return F0_tables[type];
}


template<class InputSignalProducerClass>
Distortion<InputSignalProducerClass>::Distortion(
        std::string const name,
        Type const type,
        InputSignalProducerClass& input
) noexcept
    : Filter<InputSignalProducerClass>(input, 1),
    level(name + "G", 0.0, 1.0, 0.0),
    f_table(tables.get_f_table(type)),
    F0_table(tables.get_F0_table(type))
{
    initialize_instance();
}


template<class InputSignalProducerClass>
Distortion<InputSignalProducerClass>::Distortion(
        std::string const name,
        Type const type,
        InputSignalProducerClass& input,
        FloatParamS& level_leader
) noexcept
    : Filter<InputSignalProducerClass>(input, 1),
    level(level_leader),
    f_table(tables.get_f_table(type)),
    F0_table(tables.get_F0_table(type))
{
    initialize_instance();
}


template<class InputSignalProducerClass>
void Distortion<InputSignalProducerClass>::initialize_instance() noexcept
{
    this->register_child(level);

    if (this->channels > 0) {
        previous_input_sample = new Sample[this->channels];
        F0_previous_input_sample = new Sample[this->channels];

        for (Integer c = 0; c != this->channels; ++c) {
            previous_input_sample[c] = 0.0;
            F0_previous_input_sample[c] = F0(0.0);
        }
    } else {
        previous_input_sample = NULL;
        F0_previous_input_sample = NULL;
    }
}


template<class InputSignalProducerClass>
Distortion<InputSignalProducerClass>::~Distortion()
{
    if (previous_input_sample != NULL) {
        delete[] previous_input_sample;
        delete[] F0_previous_input_sample;
    }

    previous_input_sample = NULL;
    F0_previous_input_sample = NULL;
}


template<class InputSignalProducerClass>
void Distortion<InputSignalProducerClass>::reset() noexcept
{
    Filter<InputSignalProducerClass>::reset();

    for (Integer c = 0; c != this->channels; ++c) {
        previous_input_sample[c] = 0.0;
        F0_previous_input_sample[c] = F0(0.0);
    }
}


template<class InputSignalProducerClass>
Sample const* const* Distortion<InputSignalProducerClass>::initialize_rendering(
        Integer const round,
        Integer const sample_count
) noexcept {
    Filter<InputSignalProducerClass>::initialize_rendering(round, sample_count);

    level_buffer = FloatParamS::produce_if_not_constant(level, round, sample_count);

    if (this->input.is_silent(round, sample_count)) {
        return this->input_was_silent(round);
    }

    if (level_buffer == NULL)
    {
        level_value = level.get_value();

        if (level_value < 0.000001) {
            return this->input_buffer;
        }
    }

    return NULL;
}


template<class InputSignalProducerClass>
void Distortion<InputSignalProducerClass>::render(
        Integer const round,
        Integer const first_sample_index,
        Integer const last_sample_index,
        Sample** buffer
) noexcept {
    Integer const channels = this->channels;
    Sample const* const level_buffer = this->level_buffer;
    Sample const* const* const input_buffer = this->input_buffer;

    Sample* previous_input_sample = this->previous_input_sample;
    Sample* F0_previous_input_sample = this->F0_previous_input_sample;

    if (level_buffer == NULL) {
        for (Integer c = 0; c != channels; ++c) {
            for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                Sample const input_sample = input_buffer[c][i];

                buffer[c][i] = Math::combine(
                    level_value,
                    distort(
                        input_sample,
                        previous_input_sample[c],
                        F0_previous_input_sample[c]
                    ),
                    input_sample
                );
            }
        }
    } else {
        for (Integer c = 0; c != channels; ++c) {
            for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                Sample const input_sample = input_buffer[c][i];

                buffer[c][i] = Math::combine(
                    level_buffer[i],
                    distort(
                        input_sample,
                        previous_input_sample[c],
                        F0_previous_input_sample[c]
                    ),
                    input_sample
                );
            }
        }
    }
}


template<class InputSignalProducerClass>
Sample Distortion<InputSignalProducerClass>::distort(
        Sample const input_sample,
        Sample& previous_input_sample,
        Sample& F0_previous_input_sample
) noexcept {
    Sample const delta = input_sample - previous_input_sample;

    if (UNLIKELY(Math::is_abs_small(delta, 0.00000001))) {
        previous_input_sample = input_sample;
        F0_previous_input_sample = F0(input_sample);

        /*
        We're supposed to calculate the average of the current and the previous
        input sample here, but since we only do this when their difference is
        very small or zero, we can probably get away with just using one of
        them.
        */
        return f(input_sample);
    }

    Sample const F0_input_sample = F0(input_sample);
    Sample const ret = (F0_input_sample - F0_previous_input_sample) / delta;

    previous_input_sample = input_sample;
    F0_previous_input_sample = F0_input_sample;

    return ret;
}


template<class InputSignalProducerClass>
Sample Distortion<InputSignalProducerClass>::f(Sample const x) const noexcept
{
    if (x < 0.0) {
        return -lookup(f_table, -x);
    } else {
        return lookup(f_table, x);
    }
}


template<class InputSignalProducerClass>
Sample Distortion<InputSignalProducerClass>::F0(Sample const x) const noexcept
{
    if (x < 0.0) {
        if (x < INPUT_MIN) {
            return -x;
        }

        return lookup(F0_table, -x);
    } else {
        if (x > INPUT_MAX) {
            return x;
        }

        return lookup(F0_table, x);
    }
}


template<class InputSignalProducerClass>
Sample Distortion<InputSignalProducerClass>::lookup(
        Table const& table,
        Sample const x
) const noexcept {
    return Math::lookup(&(table[0]), MAX_INDEX, x * SCALE);
}

} }

#endif
