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

#ifndef JS80P__DSP__CHORUS_CPP
#define JS80P__DSP__CHORUS_CPP

#include "dsp/chorus.hpp"

#include "dsp/math.hpp"


namespace JS80P
{

template<class InputSignalProducerClass>
Chorus<InputSignalProducerClass>::Chorus(
        std::string const name,
        InputSignalProducerClass& input
) : Effect<InputSignalProducerClass>(name, input, 27),
    delay_time(
        name + "DEL",
        0.0,
        Constants::CHORUS_DELAY_TIME_MAX,
        Constants::CHORUS_DELAY_TIME_DEFAULT
    ),
    frequency(name + "FRQ", 0.001, 20.0, 0.15),
    /*
    The depth parameter will lead the amount parameter of the LFOs which is
    expected to be scaled by 0.5 so that the LFO's oscillation range is not
    greater than 1.0. (The Oscillator oscillates between -1.0 and 1.0.)
    */
    depth(name + "DPT", 0.0, 0.5, 0.15 * 0.5),
    feedback(name + "FB", 0.0, 0.999 * FEEDBACK_SCALE_INV, 0.0),
    damping_frequency(
        name + "DF",
        Constants::BIQUAD_FILTER_FREQUENCY_MIN,
        Constants::BIQUAD_FILTER_FREQUENCY_MAX,
        Constants::BIQUAD_FILTER_FREQUENCY_DEFAULT,
        0.0,
        &log_scale_frequencies,
        Math::log_biquad_filter_freq_table(),
        Math::log_biquad_filter_freq_inv_table(),
        Math::LOG_BIQUAD_FILTER_FREQ_TABLE_MAX_INDEX,
        Math::LOG_BIQUAD_FILTER_FREQ_SCALE,
        Math::LOG_BIQUAD_FILTER_FREQ_INV_SCALE
    ),
    damping_gain(name + "DG", -36.0, -0.01, -6.0),
    width(name + "WID", -1.0, 1.0, 0.6),
    high_pass_frequency(
        name + "HPF",
        Constants::BIQUAD_FILTER_FREQUENCY_MIN,
        Constants::BIQUAD_FILTER_FREQUENCY_MAX,
        20.0,
        0.0,
        &log_scale_frequencies,
        Math::log_biquad_filter_freq_table(),
        Math::log_biquad_filter_freq_inv_table(),
        Math::LOG_BIQUAD_FILTER_FREQ_TABLE_MAX_INDEX,
        Math::LOG_BIQUAD_FILTER_FREQ_SCALE,
        Math::LOG_BIQUAD_FILTER_FREQ_INV_SCALE
    ),
    tempo_sync(name + "SYN", ToggleParam::OFF),
    log_scale_frequencies(name + "LOG", ToggleParam::OFF),
    lfo_1(name, frequency, delay_time, depth, tempo_sync, 0.0 / 3.0),
    lfo_2(name, frequency, delay_time, depth, tempo_sync, 1.0 / 3.0),
    lfo_3(name, frequency, delay_time, depth, tempo_sync, 2.0 / 3.0),
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
    /*
    The delay_time parameter controls the maximum of the centered LFOs which
    control the actual delay time of the delay lines, but for the chorus effect,
    we want to control the midpoint of the oscillation instead of the maximum.
    Thus, the actual delay time range needs to be twice as large as the delay
    time range that we present to the user.
    */
    delay_time_1(
        name + "DEL1",
        0.0,
        Constants::CHORUS_DELAY_TIME_MAX * 2.0,
        Constants::CHORUS_DELAY_TIME_DEFAULT * 2.0
    ),
    delay_time_2(
        name + "DEL2",
        0.0,
        Constants::CHORUS_DELAY_TIME_MAX * 2.0,
        Constants::CHORUS_DELAY_TIME_DEFAULT * 2.0
    ),
    delay_time_3(
        name + "DEL3",
        0.0,
        Constants::CHORUS_DELAY_TIME_MAX * 2.0,
        Constants::CHORUS_DELAY_TIME_DEFAULT * 2.0
    ),
    comb_filter_1(
        high_pass_filter,
        PannedDelayStereoMode::NORMAL,
        width,
        delay_time_1,
        &tempo_sync
    ),
    comb_filter_2(
        high_pass_filter,
        PannedDelayStereoMode::FLIPPED,
        width,
        delay_time_2,
        &tempo_sync
    ),
    comb_filter_3(
        high_pass_filter,
        PannedDelayStereoMode::NORMAL,
        delay_time_3,
        &tempo_sync
    ),
    mixer(input.get_channels()),
    high_shelf_filter_type(""),
    high_shelf_filter(
        mixer,
        high_shelf_filter_type,
        damping_frequency,
        biquad_filter_q,
        damping_gain
    ),
    feedback_gain(high_shelf_filter, feedback)
{
    this->register_child(delay_time);
    this->register_child(frequency);
    this->register_child(depth);
    this->register_child(feedback);
    this->register_child(damping_frequency);
    this->register_child(damping_gain);
    this->register_child(width);
    this->register_child(high_pass_frequency);
    this->register_child(tempo_sync);
    this->register_child(log_scale_frequencies);

    this->register_child(lfo_1);
    this->register_child(lfo_2);
    this->register_child(lfo_3);

    this->register_child(biquad_filter_q);

    this->register_child(high_pass_filter_type);
    this->register_child(high_pass_filter_gain);
    this->register_child(high_pass_filter);

    this->register_child(delay_time_1);
    this->register_child(delay_time_2);
    this->register_child(delay_time_3);

    this->register_child(comb_filter_1);
    this->register_child(comb_filter_2);
    this->register_child(comb_filter_3);

    this->register_child(mixer);

    this->register_child(high_shelf_filter_type);
    this->register_child(high_shelf_filter);

    this->register_child(feedback_gain);

    high_pass_filter_type.set_value(HighPassedInput::HIGH_PASS);

    high_shelf_filter_type.set_value(HighShelfFilter::HIGH_SHELF);

    lfo_1.center.set_value(ToggleParam::ON);
    lfo_2.center.set_value(ToggleParam::ON);
    lfo_3.center.set_value(ToggleParam::ON);

    delay_time_1.set_lfo(&lfo_1);
    delay_time_2.set_lfo(&lfo_2);
    delay_time_3.set_lfo(&lfo_3);

    comb_filter_1.delay.set_feedback_signal_producer(&feedback_gain);
    comb_filter_2.delay.set_feedback_signal_producer(&feedback_gain);
    comb_filter_3.delay.set_feedback_signal_producer(&feedback_gain);

    mixer.add(comb_filter_1);
    mixer.add(comb_filter_2);
    mixer.add(comb_filter_3);
}


template<class InputSignalProducerClass>
Sample const* const* Chorus<InputSignalProducerClass>::initialize_rendering(
        Integer const round,
        Integer const sample_count
) noexcept {
    Sample const* const* const buffer = (
        Effect<InputSignalProducerClass>::initialize_rendering(round, sample_count)
    );

    if (buffer != NULL) {
        return buffer;
    }

    chorused = SignalProducer::produce<HighShelfFilter>(high_shelf_filter, round, sample_count);
    SignalProducer::produce< Gain<HighShelfFilter> >(feedback_gain, round, sample_count);

    return NULL;
}


template<class InputSignalProducerClass>
void Chorus<InputSignalProducerClass>::render(
        Integer const round,
        Integer const first_sample_index,
        Integer const last_sample_index,
        Sample** buffer
) noexcept {
    Integer const channels = this->channels;

    for (Integer c = 0; c != channels; ++c) {
        for (Integer i = first_sample_index; i != last_sample_index; ++i) {
            buffer[c][i] = chorused[c][i];
        }
    }

    Effect<InputSignalProducerClass>::render(
        round, first_sample_index, last_sample_index, buffer
    );
}

}

#endif
