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

#ifndef JS80P__DSP__WAVEFOLDER_CPP
#define JS80P__DSP__WAVEFOLDER_CPP

#include <cmath>

#include "dsp/wavefolder.hpp"

#include "dsp/math.hpp"


namespace JS80P
{

template<class InputSignalProducerClass>
bool Wavefolder<InputSignalProducerClass>::is_initialized = false;


// template<class InputSignalProducerClass>
// Sample Wavefolder<InputSignalProducerClass>::f_table[Wavefolder<InputSignalProducerClass>::TABLE_SIZE] = {};


template<class InputSignalProducerClass>
Sample Wavefolder<InputSignalProducerClass>::F0_table[Wavefolder<InputSignalProducerClass>::TABLE_SIZE] = {};


template<class InputSignalProducerClass>
void Wavefolder<InputSignalProducerClass>::initialize_class() noexcept
{
    if (is_initialized) {
        return;
    }

    is_initialized = true;

    constexpr Number scale = WAVE_LENGTH * TABLE_SIZE_FLOAT_INV;
    constexpr Number table_size_half = TABLE_SIZE_FLOAT / 2.0;

    for (int i = 0; i != TABLE_SIZE; ++i) {
        Number const x = ((Number)i - table_size_half) * scale;
        // f_table[i] = S0 * std::sin(S1 * x) - S2 * std::sin(S3 * x) + S4 * std::sin(S5 * x);
        F0_table[i] = -S6 * std::cos(S1 * x) + S7 * std::cos(S3 * x) - S8 * std::cos(S5 * x);
    }
}


template<class InputSignalProducerClass>
Wavefolder<InputSignalProducerClass>::Wavefolder(
        InputSignalProducerClass& input
) noexcept
    : Filter<InputSignalProducerClass>(input, 1),
    folding(
        "FLD", Constants::FOLD_MIN, Constants::FOLD_MAX, Constants::FOLD_DEFAULT
    )
{
    initialize_instance();
}


template<class InputSignalProducerClass>
void Wavefolder<InputSignalProducerClass>::initialize_instance() noexcept
{
    initialize_class();

    this->register_child(folding);

    if (this->channels > 0) {
        previous_input_sample = new Sample[this->channels];
        F0_previous_input_sample = new Sample[this->channels];
        previous_output_sample = new Sample[this->channels];

        for (Integer c = 0; c != this->channels; ++c) {
            previous_input_sample[c] = 0.0;
            F0_previous_input_sample[c] = F0(0.0);
            previous_output_sample[c] = 0.0;
        }
    } else {
        previous_input_sample = NULL;
        F0_previous_input_sample = NULL;
        previous_output_sample = NULL;
    }
}


template<class InputSignalProducerClass>
Wavefolder<InputSignalProducerClass>::Wavefolder(
        InputSignalProducerClass& input,
        FloatParamS& folding_leader
) noexcept
    : Filter<InputSignalProducerClass>(input, 1),
    folding(folding_leader)
{
    initialize_instance();
}


template<class InputSignalProducerClass>
Wavefolder<InputSignalProducerClass>::~Wavefolder()
{
    if (previous_input_sample != NULL) {
        delete[] previous_input_sample;
        delete[] F0_previous_input_sample;
        delete[] previous_output_sample;

        previous_input_sample = NULL;
        F0_previous_input_sample = NULL;
        previous_output_sample = NULL;
    }
}


template<class InputSignalProducerClass>
void Wavefolder<InputSignalProducerClass>::reset() noexcept
{
    Filter<InputSignalProducerClass>::reset();

    for (Integer c = 0; c != this->channels; ++c) {
        previous_input_sample[c] = 0.0;
        F0_previous_input_sample[c] = F0(0.0);
        previous_output_sample[c] = 0.0;
    }
}


template<class InputSignalProducerClass>
Sample const* const* Wavefolder<InputSignalProducerClass>::initialize_rendering(
        Integer const round,
        Integer const sample_count
) noexcept {
    Filter<InputSignalProducerClass>::initialize_rendering(round, sample_count);

    folding_buffer = FloatParamS::produce_if_not_constant(
        folding, round, sample_count
    );

    if (this->input.is_silent(round, sample_count)) {
        return this->input_was_silent(round);
    }

    if (folding_buffer == NULL)
    {
        folding_value = folding.get_value();

        if (folding_value < 0.000001) {
            return this->input_buffer;
        }
    }

    return NULL;
}


template<class InputSignalProducerClass>
void Wavefolder<InputSignalProducerClass>::render(
        Integer const round,
        Integer const first_sample_index,
        Integer const last_sample_index,
        Sample** buffer
) noexcept {
    Integer const channels = this->channels;
    Sample const* const folding_buffer = this->folding_buffer;
    Sample const* const* const input_buffer = this->input_buffer;

    Sample* previous_input_sample = this->previous_input_sample;
    Sample* F0_previous_input_sample = this->F0_previous_input_sample;

    if (folding_buffer == NULL) {
        if (folding_value <= Constants::FOLD_TRANSITION) {
            Sample const folded_weight = folding_value * TRANSITION_INV;

            for (Integer c = 0; c != channels; ++c) {
                for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                    Sample const input_sample = input_buffer[c][i];

                    buffer[c][i] = Math::combine(
                        folded_weight,
                        fold(
                            1.0,
                            input_sample,
                            previous_input_sample[c],
                            F0_previous_input_sample[c],
                            previous_output_sample[c]
                        ),
                        input_sample
                    );
                }
            }
        } else {
            Sample const folding = folding_value + TRANSITION_DELTA;

            for (Integer c = 0; c != channels; ++c) {
                for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                    Sample const input_sample = input_buffer[c][i];

                    buffer[c][i] = fold(
                        folding,
                        input_sample,
                        previous_input_sample[c],
                        F0_previous_input_sample[c],
                        previous_output_sample[c]
                    );
                }
            }
        }
    } else {
        for (Integer c = 0; c != channels; ++c) {
            for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                Sample const input_sample = input_buffer[c][i];
                Sample const folding_raw = folding_buffer[i];

                if (folding_raw <= Constants::FOLD_TRANSITION) {
                    Sample const folded_weight = folding_raw * TRANSITION_INV;

                    buffer[c][i] = Math::combine(
                        folded_weight,
                        fold(
                            1.0,
                            input_sample,
                            previous_input_sample[c],
                            F0_previous_input_sample[c],
                            previous_output_sample[c]
                        ),
                        input_sample
                    );
                } else {
                    Sample const folding = folding_raw + TRANSITION_DELTA;

                    buffer[c][i] = fold(
                        folding,
                        input_sample,
                        previous_input_sample[c],
                        F0_previous_input_sample[c],
                        previous_output_sample[c]
                    );
                }
            }
        }
    }
}


template<class InputSignalProducerClass>
Sample Wavefolder<InputSignalProducerClass>::fold(
        Sample const folding,
        Sample const input_sample,
        Sample& previous_input_sample,
        Sample& F0_previous_input_sample,
        Sample& previous_output_sample
) noexcept {
    Sample const folding_times_input_sample = folding * input_sample;
    Sample const delta = folding_times_input_sample - previous_input_sample;

    if (UNLIKELY(std::fabs(delta) < 0.000001)) {
        /*
        We're supposed to calculate f for the average of the two samples here,
        but the numerical approximation of our f(x) via its antiderivative
        F0(x) has quite a noticable error near the zeros of the derivative of
        f(x), and when two very close input samples fall into those regions,
        then using f would produce audible discontinuities. So instead, we
        pretend that we encountered the exact same sample value again, which,
        when folded, should produce the same output sample as last time.
        */

        return previous_output_sample;
    }

    Sample const F0_input_sample = F0(folding_times_input_sample);
    Sample const ret = (F0_input_sample - F0_previous_input_sample) / delta;

    previous_input_sample = folding_times_input_sample;
    F0_previous_input_sample = F0_input_sample;
    previous_output_sample = ret;

    return ret;
}


// template<class InputSignalProducerClass>
// Sample Wavefolder<InputSignalProducerClass>::f(Sample const x) const noexcept
// {
    // return Math::lookup_periodic_2(f_table, TABLE_SIZE, TABLE_MASK, TABLE_SCALE * x + TABLE_OFFSET);
// }


template<class InputSignalProducerClass>
Sample Wavefolder<InputSignalProducerClass>::F0(Sample const x) const noexcept
{
    return Math::lookup_periodic_2(F0_table, TABLE_SIZE, TABLE_MASK, TABLE_SCALE * x + TABLE_OFFSET);
}

}

#endif
