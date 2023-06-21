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

#ifndef JS80P__DSP__PHASER_CPP
#define JS80P__DSP__PHASER_CPP

#include "dsp/phaser.hpp"

#include "dsp/math.hpp"


namespace JS80P
{

template<class InputSignalProducerClass>
Phaser<InputSignalProducerClass>::Phaser(
        std::string const name,
        InputSignalProducerClass& input
) : Effect<InputSignalProducerClass>(name, input, 22),
    tempo_sync(name + "SYN", ToggleParam::OFF),
    phaser_filter_log_scale(name + "LOG", ToggleParam::ON),
    lfo_frequency(name + "FRQ", 0.001, 20.0, 0.35),
    // TODO: waveform? feedback? more stages?
    phaser_filter_frequency(name + "CNTR", 0.0, 0.5, 0.35),
    phaser_filter_q(
        name + "Q",
        Constants::BIQUAD_FILTER_Q_MIN,
        Constants::BIQUAD_FILTER_Q_MAX,
        Constants::BIQUAD_FILTER_Q_DEFAULT
    ),
    depth(name + "DPT", 0.0, 1.0, 0.45),
    damping_frequency(
        name + "DF",
        Constants::BIQUAD_FILTER_FREQUENCY_MIN,
        Constants::BIQUAD_FILTER_FREQUENCY_MAX,
        Constants::BIQUAD_FILTER_FREQUENCY_DEFAULT
    ),
    damping_gain(name + "DG", -36.0, -0.01, -6.0),
    phase_difference(name + "PHS", 0.0, 1.0, 0.0),
    high_pass_frequency(
        name + "HPF",
        Constants::BIQUAD_FILTER_FREQUENCY_MIN,
        Constants::BIQUAD_FILTER_FREQUENCY_MAX,
        20.0
    ),
    lfo_1(name + "LFO1", lfo_frequency, phaser_filter_frequency, depth, 0.0, tempo_sync),
    lfo_2(name + "LFO2", lfo_frequency, phaser_filter_frequency, depth, phase_difference, tempo_sync),
    biquad_filter_q(
        "",
        Constants::BIQUAD_FILTER_Q_MIN,
        Constants::BIQUAD_FILTER_Q_MAX,
        Constants::BIQUAD_FILTER_Q_DEFAULT
    ),
    high_pass_filter_type(""),
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
        biquad_filter_q,
        high_pass_filter_gain
    ),
    phaser_filter_type(""),
    phaser_filter_1(
        name + "F1",
        high_pass_filter,
        phaser_filter_type,
        phaser_filter_q,
        phaser_filter_log_scale,
        0
    ),
    phaser_filter_2(
        name + "F2",
        high_pass_filter,
        phaser_filter_type,
        phaser_filter_q,
        phaser_filter_log_scale,
        1
    ),
    mixer(input.get_channels()),
    high_shelf_filter_type(""),
    high_shelf_filter(
        mixer,
        high_shelf_filter_type,
        damping_frequency,
        biquad_filter_q,
        damping_gain
    )
{
    this->register_child(tempo_sync);
    this->register_child(phaser_filter_log_scale);

    this->register_child(lfo_frequency);
    this->register_child(phaser_filter_frequency);
    this->register_child(phaser_filter_q);
    this->register_child(depth);
    this->register_child(damping_frequency);
    this->register_child(damping_gain);
    this->register_child(phase_difference);
    this->register_child(high_pass_frequency);

    this->register_child(lfo_1);
    this->register_child(lfo_2);

    this->register_child(biquad_filter_q);

    this->register_child(high_pass_filter_type);
    this->register_child(high_pass_filter_gain);
    this->register_child(high_pass_filter);

    this->register_child(phaser_filter_type);

    this->register_child(phaser_filter_1);
    this->register_child(phaser_filter_2);

    this->register_child(mixer);

    this->register_child(high_shelf_filter_type);
    this->register_child(high_shelf_filter);

    high_pass_filter_type.set_value(HighPassedInput::HIGH_PASS);

    high_shelf_filter_type.set_value(HighShelfFilter::HIGH_SHELF);

    lfo_1.center.set_value(ToggleParam::ON);
    lfo_2.center.set_value(ToggleParam::ON);

    phaser_filter_1.frequency.set_lfo(&lfo_1);
    phaser_filter_2.frequency.set_lfo(&lfo_2);

    mixer.add(phaser_filter_1);
    mixer.add(phaser_filter_2);
}


template<class InputSignalProducerClass>
Sample const* const* Phaser<InputSignalProducerClass>::initialize_rendering(
        Integer const round,
        Integer const sample_count
) noexcept {
    Sample const* const* const buffer = (
        Effect<InputSignalProducerClass>::initialize_rendering(round, sample_count)
    );

    if (buffer != NULL) {
        return buffer;
    }

    phaser_filtered = SignalProducer::produce<HighShelfFilter>(
        high_shelf_filter, round, sample_count
    );

    return NULL;
}


template<class InputSignalProducerClass>
void Phaser<InputSignalProducerClass>::render(
        Integer const round,
        Integer const first_sample_index,
        Integer const last_sample_index,
        Sample** buffer
) noexcept {
    Integer const channels = this->channels;

    for (Integer c = 0; c != channels; ++c) {
        for (Integer i = first_sample_index; i != last_sample_index; ++i) {
            buffer[c][i] = phaser_filtered[c][i];
        }
    }

    Effect<InputSignalProducerClass>::render(
        round, first_sample_index, last_sample_index, buffer
    );
}

}

#endif
