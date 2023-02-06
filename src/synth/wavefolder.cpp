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

#ifndef JS80P__SYNTH__WAVEFOLDER_CPP
#define JS80P__SYNTH__WAVEFOLDER_CPP

#include <cmath>

#include "wavefolder.hpp"

#include "math.hpp"


namespace JS80P
{

template<class InputSignalProducerClass>
Wavefolder<InputSignalProducerClass>::Wavefolder(
        InputSignalProducerClass& input,
        FloatParam& folding_leader
) : Filter<InputSignalProducerClass>(input, 1),
    folding(folding_leader)
{
    this->register_child(folding);

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
Sample const* const* Wavefolder<InputSignalProducerClass>::initialize_rendering(
        Integer const round,
        Integer const sample_count
) {
    Filter<InputSignalProducerClass>::initialize_rendering(round, sample_count);

    folding_buffer = FloatParam::produce_if_not_constant(
        &folding, round, sample_count
    );

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
) {
    Integer const channels = this->channels;
    Sample const* const folding_buffer = this->folding_buffer;
    Sample const* const* const input_buffer = this->input_buffer;

    Sample* previous_input_sample = this->previous_input_sample;
    Sample* F0_previous_input_sample = this->F0_previous_input_sample;

    if (folding_buffer == NULL) {
        if (folding_value < Constants::FOLD_TRANSITION) {
            Sample const folded_weight = folding_value * TRANSITION_INV;
            Sample const bypass_weight = 1.0 - folded_weight;

            for (Integer c = 0; c != channels; ++c) {
                for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                    Sample const input_sample = input_buffer[c][i];

                    buffer[c][i] = (
                        folded_weight * fold(
                            1.0,
                            input_sample,
                            previous_input_sample[c],
                            F0_previous_input_sample[c]
                        )
                        + bypass_weight * input_sample
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
                        F0_previous_input_sample[c]
                    );
                }
            }
        }
    } else {
        for (Integer c = 0; c != channels; ++c) {
            for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                Sample const input_sample = input_buffer[c][i];
                Sample const folding_raw = folding_buffer[i];

                if (folding_raw < Constants::FOLD_TRANSITION) {
                    Sample const folded_weight = folding_raw * TRANSITION_INV;
                    Sample const bypass_weight = 1.0 - folded_weight;

                    buffer[c][i] = (
                        folded_weight * fold(
                            1.0,
                            input_sample,
                            previous_input_sample[c],
                            F0_previous_input_sample[c]
                        )
                        + bypass_weight * input_sample
                    );
                } else {
                    Sample const folding = folding_raw + TRANSITION_DELTA;

                    buffer[c][i] = fold(
                        folding,
                        input_sample,
                        previous_input_sample[c],
                        F0_previous_input_sample[c]
                    );
                }
            }
        }
    }

    this->previous_input_sample = previous_input_sample;
    this->F0_previous_input_sample = F0_previous_input_sample;
}


template<class InputSignalProducerClass>
Sample Wavefolder<InputSignalProducerClass>::fold(
        Sample const folding,
        Sample const input_sample,
        Sample& previous_input_sample,
        Sample& F0_previous_input_sample
) {
    Sample const folding_times_input_sample = folding * input_sample;
    Sample const delta = folding_times_input_sample - previous_input_sample;

    if (UNLIKELY(std::fabs(delta) < 0.00000001)) {
        // We're supposed to calculate the average of the current and the
        // previous input sample here, but since we only do this when their
        // difference very small or zero, we can probably get away with just
        // using one of them.
        Sample const ret = f(folding_times_input_sample);

        previous_input_sample = folding_times_input_sample;
        F0_previous_input_sample = F0(folding_times_input_sample);

        return ret;
    }

    Sample const F0_input_sample = F0(folding_times_input_sample);
    Sample const ret = (F0_input_sample - F0_previous_input_sample) / delta;

    previous_input_sample = folding_times_input_sample;
    F0_previous_input_sample = F0_input_sample;

    return ret;
}


template<class InputSignalProducerClass>
Sample Wavefolder<InputSignalProducerClass>::f(Sample const x) const
{
    return (
        S0 * Math::sin(S1 * x + TRIG_OFFSET)
        - S2 * Math::sin(S3 * x + TRIG_OFFSET)
        + S4 * Math::sin(S5 * x + TRIG_OFFSET)
    );
}


template<class InputSignalProducerClass>
Sample Wavefolder<InputSignalProducerClass>::F0(Sample const x) const
{
    return (
        -S6 * Math::cos(S1 * x + TRIG_OFFSET)
        + S7 * Math::cos(S3 * x + TRIG_OFFSET)
        - S8 * Math::cos(S5 * x + TRIG_OFFSET)
    );
}

}

#endif
