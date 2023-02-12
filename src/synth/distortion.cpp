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

#ifndef JS80P__SYNTH__DISTORTION_CPP
#define JS80P__SYNTH__DISTORTION_CPP

#include <cmath>

#include "synth/distortion.hpp"

#include "synth/math.hpp"


namespace JS80P
{

template<class InputSignalProducerClass>
Distortion<InputSignalProducerClass>::Distortion(
        std::string const name,
        Number const steepness,
        InputSignalProducerClass& input
) : Filter<InputSignalProducerClass>(input, 1),
    level(name + "G", 0.0, 1.0, 0.0)
{
    initialize_instance(steepness);
}


template<class InputSignalProducerClass>
void Distortion<InputSignalProducerClass>::initialize_instance(
        Number const steepness
) {
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

    Sample const table_size_inv = 1.0 / (Sample)TABLE_SIZE;
    Sample const steepness_inv_double = 2.0 / steepness;

    for (Integer i = 0; i != TABLE_SIZE; ++i) {
        Number const x = INPUT_MAX * ((Sample)i * table_size_inv);
        f_table[i] = std::tanh(steepness * x * 0.5);
        F0_table[i] = (
            x + steepness_inv_double * std::log(std::exp(-steepness * x) + 1.0)
        );
    }
}


template<class InputSignalProducerClass>
Sample const* const* Distortion<InputSignalProducerClass>::initialize_rendering(
        Integer const round,
        Integer const sample_count
) {
    Filter<InputSignalProducerClass>::initialize_rendering(round, sample_count);

    level_buffer = FloatParam::produce_if_not_constant(
        &level, round, sample_count
    );

    if (level_buffer == NULL)
    {
        level_value = level.get_value();
        level.skip_round(round, sample_count);

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
) {
    Integer const channels = this->channels;
    Sample const* const level_buffer = this->level_buffer;
    Sample const* const* const input_buffer = this->input_buffer;

    Sample* previous_input_sample = this->previous_input_sample;
    Sample* F0_previous_input_sample = this->F0_previous_input_sample;

    if (level_buffer == NULL) {
        Sample const distorted_weight = level_value;
        Sample const bypass_weight = 1.0 - distorted_weight;

        for (Integer c = 0; c != channels; ++c) {
            for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                Sample const input_sample = input_buffer[c][i];

                buffer[c][i] = (
                    distorted_weight * distort(
                        input_sample,
                        previous_input_sample[c],
                        F0_previous_input_sample[c]
                    )
                    + bypass_weight * input_sample
                );
            }
        }
    } else {
        for (Integer c = 0; c != channels; ++c) {
            for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                Sample const input_sample = input_buffer[c][i];
                Sample const distorted_weight = level_buffer[i];
                Sample const bypass_weight = 1.0 - distorted_weight;

                buffer[c][i] = (
                    distorted_weight * distort(
                        input_sample,
                        previous_input_sample[c],
                        F0_previous_input_sample[c]
                    )
                    + bypass_weight * input_sample
                );
            }
        }
    }

    this->previous_input_sample = previous_input_sample;
    this->F0_previous_input_sample = F0_previous_input_sample;
}


template<class InputSignalProducerClass>
Sample Distortion<InputSignalProducerClass>::distort(
        Sample const input_sample,
        Sample& previous_input_sample,
        Sample& F0_previous_input_sample
) {
    Sample const delta = input_sample - previous_input_sample;

    if (UNLIKELY(std::fabs(delta) < 0.00000001)) {
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
Sample Distortion<InputSignalProducerClass>::f(Sample const x) const
{
    if (x < 0.0) {
        return -lookup(-x, f_table);
    } else {
        return lookup(x, f_table);
    }
}


template<class InputSignalProducerClass>
Sample Distortion<InputSignalProducerClass>::F0(Sample const x) const
{
    if (x < 0.0) {
        if (x < INPUT_MIN) {
            return -x;
        }

        return lookup(-x, F0_table);
    } else {
        if (x > INPUT_MAX) {
            return x;
        }

        return lookup(x, F0_table);
    }
}


template<class InputSignalProducerClass>
Sample Distortion<InputSignalProducerClass>::lookup(
        Sample const x,
        Sample const* const table
) const {
    Sample const index = x * SCALE;

    Number const after_weight = index - std::floor(index);
    Number const before_weight = 1.0 - after_weight;
    int const before_index = std::min(MAX_INDEX_BEFORE, (int)index);
    int const after_index = before_index + 1;

    return (
        before_weight * table[before_index] + after_weight * table[after_index]
    );
}

}

#endif
