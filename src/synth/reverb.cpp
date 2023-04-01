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

#ifndef JS80P__SYNTH__REVERB_CPP
#define JS80P__SYNTH__REVERB_CPP

#include "synth/reverb.hpp"


namespace JS80P
{

template<class InputSignalProducerClass>
constexpr Seconds Reverb<InputSignalProducerClass>::TUNINGS[COMB_FILTERS];


template<class InputSignalProducerClass>
Reverb<InputSignalProducerClass>::Reverb(
        std::string const name,
        InputSignalProducerClass& input
) : Effect<InputSignalProducerClass>(name, input, 9 + COMB_FILTERS),
    room_size(name + "RS", 0.0, 0.999, 0.75),
    damping_frequency(
        name + "DF",
        Constants::BIQUAD_FILTER_FREQUENCY_MIN,
        Constants::BIQUAD_FILTER_FREQUENCY_MAX,
        Constants::BIQUAD_FILTER_FREQUENCY_DEFAULT
    ),
    damping_gain(name + "DG", -36.0, -0.01, -6.0),
    width(name + "WID", -1.0, 1.0, 0.0),
    high_pass_frequency(
        name + "HPF",
        Constants::BIQUAD_FILTER_FREQUENCY_MIN,
        Constants::BIQUAD_FILTER_FREQUENCY_MAX,
        20.0
    ),
    high_pass_filter_type(""),
    high_pass_filter_q(
        "",
        Constants::BIQUAD_FILTER_Q_MIN,
        Constants::BIQUAD_FILTER_Q_MAX,
        Constants::BIQUAD_FILTER_Q_DEFAULT
    ),
    high_pass_filter_gain(
        "",
        Constants::BIQUAD_FILTER_GAIN_MIN,
        Constants::BIQUAD_FILTER_GAIN_MAX,
        0.0
    ),
    high_pass_filter(
        input,
        high_pass_filter_type,
        high_pass_frequency,
        high_pass_filter_q,
        high_pass_filter_gain
    )
{
    this->register_child(room_size);
    this->register_child(damping_frequency);
    this->register_child(damping_gain);
    this->register_child(width);
    this->register_child(high_pass_frequency);

    this->register_child(high_pass_filter_type);
    this->register_child(high_pass_filter_q);
    this->register_child(high_pass_filter_gain);

    this->register_child(high_pass_filter);

    for (Integer i = 0; i != COMB_FILTERS; ++i) {
        comb_filters[i] = new HighPassCombFilter(
            high_pass_filter,
            i & 1
                ? HighPassCombFilter::StereoMode::FLIPPED
                : HighPassCombFilter::StereoMode::NORMAL,
            width,
            room_size,
            TUNINGS[i],
            damping_frequency,
            damping_gain
        );
        comb_filters[i]->delay.set_feedback_signal_producer(
            &comb_filters[i]->high_shelf_filter
        );

        this->register_child(*comb_filters[i]);
        comb_filter_buffers[i] = NULL;
    }

    high_pass_filter_type.set_value(HighPassInput::HIGH_PASS);
}


template<class InputSignalProducerClass>
Reverb<InputSignalProducerClass>::~Reverb()
{
    for (Integer i = 0; i != COMB_FILTERS; ++i) {
        delete comb_filters[i];
        comb_filters[i] = NULL;
    }
}


template<class InputSignalProducerClass>
Sample const* const* Reverb<InputSignalProducerClass>::initialize_rendering(
        Integer const round,
        Integer const sample_count
) noexcept {
    Sample const* const* const buffer = (
        Effect<InputSignalProducerClass>::initialize_rendering(round, sample_count)
    );

    if (buffer != NULL) {
        return buffer;
    }

    for (Integer i = 0; i != COMB_FILTERS; ++i) {
        comb_filter_buffers[i] = SignalProducer::produce<HighPassCombFilter>(
            comb_filters[i], round, sample_count
        );
    }

    return NULL;
}


template<class InputSignalProducerClass>
void Reverb<InputSignalProducerClass>::render(
        Integer const round,
        Integer const first_sample_index,
        Integer const last_sample_index,
        Sample** buffer
) noexcept {
    Integer const channels = this->channels;

    for (Integer c = 0; c != channels; ++c) {
        for (Integer i = first_sample_index; i != last_sample_index; ++i) {
            buffer[c][i] = 0.0;

            for (Integer j = 0; j != COMB_FILTERS; ++j) {
                buffer[c][i] += comb_filter_buffers[j][c][i];
            }
        }
    }

    Effect<InputSignalProducerClass>::render(
        round, first_sample_index, last_sample_index, buffer
    );
}

}

#endif
