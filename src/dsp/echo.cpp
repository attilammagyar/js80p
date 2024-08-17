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

#ifndef JS80P__DSP__ECHO_CPP
#define JS80P__DSP__ECHO_CPP

#include "dsp/echo.hpp"

#include "dsp/math.hpp"


namespace JS80P
{

template<class InputSignalProducerClass>
Echo<InputSignalProducerClass>::Echo(
        std::string const& name,
        InputSignalProducerClass& input,
        BiquadFilterSharedBuffers& high_shelf_filter_shared_buffers
) : SideChainCompressableEffect<InputSignalProducerClass>(
        name, input, 18, &comb_filter_2
    ),
    delay_time(
        name + "DEL",
        Constants::DELAY_TIME_MIN,
        Constants::DELAY_TIME_MAX,
        Constants::DELAY_TIME_DEFAULT
    ),
    input_volume(name + "INV", 0.0, 2.0, 1.0),
    feedback(
        name + "FB",
        Constants::DELAY_FEEDBACK_MIN,
        Constants::DELAY_FEEDBACK_MAX,
        Constants::DELAY_FEEDBACK_DEFAULT
    ),
    damping_frequency(
        name + "DF",
        Constants::BIQUAD_FILTER_FREQUENCY_MIN,
        Constants::BIQUAD_FILTER_FREQUENCY_MAX,
        Constants::BIQUAD_FILTER_FREQUENCY_DEFAULT,
        0.0,
        NULL,
        &log_scale_frequencies,
        Math::log_biquad_filter_freq_table(),
        Math::LOG_BIQUAD_FILTER_FREQ_TABLE_MAX_INDEX,
        Math::LOG_BIQUAD_FILTER_FREQ_TABLE_INDEX_SCALE
    ),
    damping_gain(name + "DG", -36.0, -0.01, -6.0),
    width(name + "WID", -1.0, 1.0, 0.0),
    high_pass_frequency(
        name + "HPF",
        Constants::BIQUAD_FILTER_FREQUENCY_MIN,
        Constants::BIQUAD_FILTER_FREQUENCY_MAX,
        20.0,
        0.0,
        NULL,
        &log_scale_frequencies,
        Math::log_biquad_filter_freq_table(),
        Math::LOG_BIQUAD_FILTER_FREQ_TABLE_MAX_INDEX,
        Math::LOG_BIQUAD_FILTER_FREQ_TABLE_INDEX_SCALE
    ),
    high_pass_q(
        name + "HPQ",
        Constants::BIQUAD_FILTER_Q_MIN,
        Constants::BIQUAD_FILTER_Q_MAX,
        Constants::BIQUAD_FILTER_Q_DEFAULT,
        0.0,
        NULL,
        &log_scale_high_pass_q,
        Math::log_biquad_filter_q_table(),
        Math::LOG_BIQUAD_FILTER_Q_TABLE_MAX_INDEX,
        Math::LOG_BIQUAD_FILTER_Q_TABLE_INDEX_SCALE,
        Math::LOG_BIQUAD_FILTER_Q_VALUE_OFFSET
    ),
    distortion_level(name + "DST", 0.0, 1.0, 0.0),
    tempo_sync(name + "SYN", ToggleParam::OFF),
    log_scale_frequencies(name + "LOG", ToggleParam::OFF),
    log_scale_high_pass_q(name + "LHQ", ToggleParam::OFF),
    distortion_type("", Distortion::TYPE_DELAY_FEEDBACK),
    high_pass_filter_gain(
        "",
        Constants::BIQUAD_FILTER_GAIN_MIN,
        Constants::BIQUAD_FILTER_GAIN_MAX,
        0.0
    ),
    gain(input, input_volume),
    high_pass_filter(
        gain,
        high_pass_frequency,
        high_pass_q,
        high_pass_filter_gain,
        NULL,
        0.0,
        NULL,
        NULL,
        &gain
    ),
    comb_filter_1(
        high_pass_filter,
        PannedDelayStereoMode::NORMAL,
        width,
        feedback,
        delay_time,
        high_shelf_filter_shared_buffers,
        damping_frequency,
        damping_gain,
        distortion_level,
        distortion_type,
        &tempo_sync
    ),
    comb_filter_2(
        comb_filter_1.high_shelf_filter,
        PannedDelayStereoMode::FLIPPED,
        width,
        feedback,
        delay_time,
        high_shelf_filter_shared_buffers,
        damping_frequency,
        damping_gain,
        distortion_level,
        distortion_type,
        &tempo_sync
    ),
    comb_filter_1_buffer(NULL),
    comb_filter_2_buffer(NULL)
{
    this->register_child(delay_time);
    this->register_child(input_volume);
    this->register_child(feedback);
    this->register_child(damping_frequency);
    this->register_child(damping_gain);
    this->register_child(width);
    this->register_child(high_pass_frequency);
    this->register_child(high_pass_q);
    this->register_child(distortion_level);
    this->register_child(tempo_sync);
    this->register_child(log_scale_frequencies);
    this->register_child(log_scale_high_pass_q);

    this->register_child(distortion_type);

    this->register_child(high_pass_filter_gain);

    this->register_child(gain);
    this->register_child(high_pass_filter);
    this->register_child(comb_filter_1);
    this->register_child(comb_filter_2);

    comb_filter_1.delay.set_feedback_signal_producer(comb_filter_2.high_shelf_filter);
}


template<class InputSignalProducerClass>
Sample const* const* Echo<InputSignalProducerClass>::initialize_rendering(
        Integer const round,
        Integer const sample_count
) noexcept {
    Sample const* const* const buffer = (
        SideChainCompressableEffect<InputSignalProducerClass>::initialize_rendering(
            round, sample_count
        )
    );

    if (buffer != NULL) {
        return buffer;
    }

    comb_filter_1_buffer = SignalProducer::produce<CombFilter1>(
        comb_filter_1, round, sample_count
    );
    comb_filter_2_buffer = SignalProducer::produce<CombFilter2>(
        comb_filter_2, round, sample_count
    );

    return NULL;
}


template<class InputSignalProducerClass>
void Echo<InputSignalProducerClass>::render(
        Integer const round,
        Integer const first_sample_index,
        Integer const last_sample_index,
        Sample** buffer
) noexcept {
    Integer const channels = this->channels;

    for (Integer c = 0; c != channels; ++c) {
        for (Integer i = first_sample_index; i != last_sample_index; ++i) {
            buffer[c][i] = comb_filter_1_buffer[c][i] + comb_filter_2_buffer[c][i];
        }
    }

    SideChainCompressableEffect<InputSignalProducerClass>::render(
        round, first_sample_index, last_sample_index, buffer
    );
}

}

#endif
