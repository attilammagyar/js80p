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

#ifndef JS80P__DSP__CHORUS_CPP
#define JS80P__DSP__CHORUS_CPP

#include "dsp/chorus.hpp"

#include "dsp/math.hpp"


namespace JS80P
{

template<class InputSignalProducerClass>
constexpr typename Chorus<InputSignalProducerClass>::Tuning Chorus<InputSignalProducerClass>::TUNINGS[][VOICES];


template<class InputSignalProducerClass>
Chorus<InputSignalProducerClass>::TypeParam::TypeParam(
        std::string const& name
) noexcept
    : ByteParam(name, CHORUS_1, CHORUS_15, CHORUS_1)
{
}

template<class InputSignalProducerClass>
Chorus<InputSignalProducerClass>::Chorus(
        std::string const& name,
        InputSignalProducerClass& input
) : Effect<InputSignalProducerClass>(name, input, 19 + VOICES * 3, &mixer),
    type(name + "TYP"),
    delay_time(
        name + "DEL",
        0.0,
        Constants::CHORUS_DELAY_TIME_MAX,
        Constants::CHORUS_DELAY_TIME_DEFAULT
    ),
    frequency(
        name + "FRQ",
        Constants::CHORUS_LFO_FREQUENCY_MIN,
        Constants::CHORUS_LFO_FREQUENCY_MAX,
        Constants::CHORUS_LFO_FREQUENCY_DEFAULT,
        0.0,
        NULL,
        &log_scale_lfo_frequency,
        Math::log_chorus_lfo_freq_table(),
        Math::LOG_CHORUS_LFO_FREQ_TABLE_MAX_INDEX,
        Math::LOG_CHORUS_LFO_FREQ_TABLE_INDEX_SCALE
    ),
    /*
    The depth parameter will lead the amount parameter of the LFOs which is
    expected to be scaled by 0.5 so that the LFO's oscillation range is not
    greater than 1.0. (The Oscillator oscillates between -1.0 and 1.0.)
    */
    depth(name + "DPT", 0.0, 0.5, 0.15 * 0.5),
    feedback(
        name + "FB",
        Constants::DELAY_FEEDBACK_MIN * FEEDBACK_SCALE_INV,
        Constants::DELAY_FEEDBACK_MAX * FEEDBACK_SCALE_INV,
        Constants::DELAY_FEEDBACK_MIN * FEEDBACK_SCALE_INV
    ),
    damping_frequency(
        name + "DF",
        Constants::BIQUAD_FILTER_FREQUENCY_MIN,
        Constants::BIQUAD_FILTER_FREQUENCY_MAX,
        Constants::BIQUAD_FILTER_FREQUENCY_DEFAULT,
        0.0,
        NULL,
        &log_scale_filter_frequencies,
        Math::log_biquad_filter_freq_table(),
        Math::LOG_BIQUAD_FILTER_FREQ_TABLE_MAX_INDEX,
        Math::LOG_BIQUAD_FILTER_FREQ_TABLE_INDEX_SCALE
    ),
    damping_gain(name + "DG", -36.0, -0.01, -6.0),
    width(name + "WID", -1.0, 1.0, 0.6),
    high_pass_frequency(
        name + "HPF",
        Constants::BIQUAD_FILTER_FREQUENCY_MIN,
        Constants::BIQUAD_FILTER_FREQUENCY_MAX,
        20.0,
        0.0,
        NULL,
        &log_scale_filter_frequencies,
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
    tempo_sync(name + "SYN", ToggleParam::OFF),
    log_scale_filter_frequencies(name + "LOG", ToggleParam::OFF),
    log_scale_high_pass_q(name + "LHQ", ToggleParam::OFF),
    log_scale_lfo_frequency(name + "LLG", ToggleParam::OFF),
    biquad_filter_q(
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
        CombFilter::CHANNELS,
        high_pass_frequency,
        high_pass_q,
        high_pass_filter_gain
    ),
    lfos{
        {name + "LFO1", frequency, delay_time, depth, tempo_sync, 0.0},
        {name + "LFO2", frequency, delay_time, depth, tempo_sync, 0.0},
        {name + "LFO3", frequency, delay_time, depth, tempo_sync, 0.0},
        {name + "LFO4", frequency, delay_time, depth, tempo_sync, 0.0},
        {name + "LFO5", frequency, delay_time, depth, tempo_sync, 0.0},
        {name + "LFO6", frequency, delay_time, depth, tempo_sync, 0.0},
        {name + "LFO7", frequency, delay_time, depth, tempo_sync, 0.0},
    },
    delay_times{
        FloatParamS(name + "DEL1", 0.0, DELAY_TIME_MAX, DELAY_TIME_DEFAULT),
        FloatParamS(name + "DEL2", 0.0, DELAY_TIME_MAX, DELAY_TIME_DEFAULT),
        FloatParamS(name + "DEL3", 0.0, DELAY_TIME_MAX, DELAY_TIME_DEFAULT),
        FloatParamS(name + "DEL4", 0.0, DELAY_TIME_MAX, DELAY_TIME_DEFAULT),
        FloatParamS(name + "DEL5", 0.0, DELAY_TIME_MAX, DELAY_TIME_DEFAULT),
        FloatParamS(name + "DEL6", 0.0, DELAY_TIME_MAX, DELAY_TIME_DEFAULT),
        FloatParamS(name + "DEL7", 0.0, DELAY_TIME_MAX, DELAY_TIME_DEFAULT),
    },
    comb_filters{
        {high_pass_filter, StereoPannedDelayMode::NORMAL, width, delay_times[0], &tempo_sync},
        {high_pass_filter, StereoPannedDelayMode::NORMAL, width, delay_times[1], &tempo_sync},
        {high_pass_filter, StereoPannedDelayMode::NORMAL, width, delay_times[2], &tempo_sync},
        {high_pass_filter, StereoPannedDelayMode::NORMAL, width, delay_times[3], &tempo_sync},
        {high_pass_filter, StereoPannedDelayMode::NORMAL, width, delay_times[4], &tempo_sync},
        {high_pass_filter, StereoPannedDelayMode::NORMAL, width, delay_times[5], &tempo_sync},
        {high_pass_filter, StereoPannedDelayMode::NORMAL, width, delay_times[6], &tempo_sync},
    },
    mixer(CombFilter::CHANNELS),
    high_shelf_filter(
        mixer,
        CombFilter::CHANNELS,
        damping_frequency,
        biquad_filter_q,
        damping_gain,
        NULL,
        0.0,
        NULL,
        NULL,
        &mixer
    ),
    feedback_gain(high_shelf_filter, feedback, NULL, CombFilter::CHANNELS),
    previous_type(255),
    should_start_lfos(true)
{
    this->register_child(delay_time);
    this->register_child(frequency);
    this->register_child(depth);
    this->register_child(feedback);
    this->register_child(damping_frequency);
    this->register_child(damping_gain);
    this->register_child(width);
    this->register_child(high_pass_frequency);
    this->register_child(high_pass_q);
    this->register_child(tempo_sync);
    this->register_child(log_scale_filter_frequencies);
    this->register_child(log_scale_high_pass_q);
    this->register_child(log_scale_lfo_frequency);

    this->register_child(biquad_filter_q);

    this->register_child(high_pass_filter_gain);
    this->register_child(high_pass_filter);

    this->register_child(mixer);

    this->register_child(high_shelf_filter);

    this->register_child(feedback_gain);

    for (size_t i = 0; i != VOICES; ++i) {
        lfos[i].center.set_value(ToggleParam::ON);
        delay_times[i].set_lfo(&lfos[i]);
        comb_filters[i].delay.set_feedback_signal_producer(feedback_gain);

        if (i > 0) {
            comb_filters[i].delay.use_shared_delay_buffer(comb_filters[0].delay);
        }

        mixer.add(comb_filters[i]);

        this->register_child(lfos[i]);
        this->register_child(delay_times[i]);
        this->register_child(comb_filters[i]);
    }
}


template<class InputSignalProducerClass>
void Chorus<InputSignalProducerClass>::start_lfos(Seconds const time_offset) noexcept
{
    should_start_lfos = true;

    for (size_t i = 0; i != VOICES; ++i) {
        lfos[i].start(time_offset);
    }
}


template<class InputSignalProducerClass>
void Chorus<InputSignalProducerClass>::stop_lfos(Seconds const time_offset) noexcept
{
    should_start_lfos = false;

    for (size_t i = 0; i != VOICES; ++i) {
        lfos[i].stop(time_offset);
    }
}


template<class InputSignalProducerClass>
void Chorus<InputSignalProducerClass>::skip_round_for_lfos(
        Integer const round,
        Integer const sample_count
) noexcept {
    for (size_t i = 0; i != VOICES; ++i) {
        lfos[i].skip_round(round, sample_count);
    }
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

    Byte const type = this->type.get_value();

    if (previous_type != type) {
        previous_type = type;

        update_tunings(type);
    }

    chorused = SignalProducer::produce<HighShelfFilter>(high_shelf_filter, round, sample_count);
    SignalProducer::produce< Gain<HighShelfFilter> >(feedback_gain, round, sample_count);

    return NULL;
}


template<class InputSignalProducerClass>
void Chorus<InputSignalProducerClass>::update_tunings(Byte const type) noexcept
{
    Tuning const* const tunings = TUNINGS[type];

    mixer.reset();

    for (size_t i = 0; i != VOICES; ++i) {
        Tuning const& tuning = tunings[i];

        CombFilter& comb_filter = comb_filters[i];
        LFO& lfo = lfos[i];

        comb_filter.reset();
        comb_filter.set_panning_scale(tuning.panning_scale);

        lfo.reset();
        lfo.phase.set_value(tuning.lfo_phase);

        mixer.set_weight(i, tuning.weight);
    }

    if (should_start_lfos) {
        for (size_t i = 0; i != VOICES; ++i) {
            lfos[i].start(0.0);
        }
    }
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
