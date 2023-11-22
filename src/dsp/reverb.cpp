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

#ifndef JS80P__DSP__REVERB_CPP
#define JS80P__DSP__REVERB_CPP

#include "dsp/reverb.hpp"

#include "dsp/math.hpp"


namespace JS80P
{

template<class InputSignalProducerClass>
constexpr typename Reverb<InputSignalProducerClass>::Tuning Reverb<InputSignalProducerClass>::TUNINGS[][COMB_FILTERS];


template<class InputSignalProducerClass>
Reverb<InputSignalProducerClass>::TypeParam::TypeParam(
        std::string const& name
) noexcept
    : Param<Type, ParamEvaluation::BLOCK>(name, REVERB_1, REVERB_10, REVERB_1)
{
}


template<class InputSignalProducerClass>
Reverb<InputSignalProducerClass>::Reverb(
        std::string const& name,
        InputSignalProducerClass& input,
        BiquadFilterSharedBuffers& high_shelf_filter_shared_buffers
) : SideChainCompressableEffect<InputSignalProducerClass>(
        name,
        input,
        12 + COMB_FILTERS,
        &mixer
    ),
    type(name+ "TYP"),
    room_size(name + "RS", 0.0, 0.999, 0.75),
    damping_frequency(
        name + "DF",
        Constants::BIQUAD_FILTER_FREQUENCY_MIN,
        Constants::BIQUAD_FILTER_FREQUENCY_MAX,
        Constants::BIQUAD_FILTER_FREQUENCY_DEFAULT,
        0.0,
        NULL,
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
        NULL,
        &log_scale_frequencies,
        Math::log_biquad_filter_freq_table(),
        Math::LOG_BIQUAD_FILTER_FREQ_TABLE_MAX_INDEX,
        Math::LOG_BIQUAD_FILTER_FREQ_TABLE_INDEX_SCALE
    ),
    log_scale_frequencies(name + "LOG", ToggleParam::OFF),
    mixer(input.get_channels()),
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
    ),
    comb_filters{
        {
            high_pass_filter,
            PannedDelayStereoMode::NORMAL,
            width,
            room_size,
            TUNINGS[0][0].delay_time,
            DELAY_TIME_MAX,
            high_shelf_filter_shared_buffers,
            damping_frequency,
            damping_gain
        },
        {
            high_pass_filter,
            PannedDelayStereoMode::NORMAL,
            width,
            room_size,
            TUNINGS[0][1].delay_time,
            DELAY_TIME_MAX,
            high_shelf_filter_shared_buffers,
            damping_frequency,
            damping_gain
        },
        {
            high_pass_filter,
            PannedDelayStereoMode::NORMAL,
            width,
            room_size,
            TUNINGS[0][2].delay_time,
            DELAY_TIME_MAX,
            high_shelf_filter_shared_buffers,
            damping_frequency,
            damping_gain
        },
        {
            high_pass_filter,
            PannedDelayStereoMode::NORMAL,
            width,
            room_size,
            TUNINGS[0][3].delay_time,
            DELAY_TIME_MAX,
            high_shelf_filter_shared_buffers,
            damping_frequency,
            damping_gain
        },
        {
            high_pass_filter,
            PannedDelayStereoMode::NORMAL,
            width,
            room_size,
            TUNINGS[0][4].delay_time,
            DELAY_TIME_MAX,
            high_shelf_filter_shared_buffers,
            damping_frequency,
            damping_gain
        },
        {
            high_pass_filter,
            PannedDelayStereoMode::NORMAL,
            width,
            room_size,
            TUNINGS[0][5].delay_time,
            DELAY_TIME_MAX,
            high_shelf_filter_shared_buffers,
            damping_frequency,
            damping_gain
        },
        {
            high_pass_filter,
            PannedDelayStereoMode::NORMAL,
            width,
            room_size,
            TUNINGS[0][6].delay_time,
            DELAY_TIME_MAX,
            high_shelf_filter_shared_buffers,
            damping_frequency,
            damping_gain
        },
        {
            high_pass_filter,
            PannedDelayStereoMode::NORMAL,
            width,
            room_size,
            TUNINGS[0][7].delay_time,
            DELAY_TIME_MAX,
            high_shelf_filter_shared_buffers,
            damping_frequency,
            damping_gain
        },
        {
            high_pass_filter,
            PannedDelayStereoMode::NORMAL,
            width,
            room_size,
            TUNINGS[0][8].delay_time,
            DELAY_TIME_MAX,
            high_shelf_filter_shared_buffers,
            damping_frequency,
            damping_gain
        },
        {
            high_pass_filter,
            PannedDelayStereoMode::NORMAL,
            width,
            room_size,
            TUNINGS[0][9].delay_time,
            DELAY_TIME_MAX,
            high_shelf_filter_shared_buffers,
            damping_frequency,
            damping_gain
        },
    },
    previous_type(255)
{
    this->register_child(mixer);

    this->register_child(type);
    this->register_child(room_size);
    this->register_child(damping_frequency);
    this->register_child(damping_gain);
    this->register_child(width);
    this->register_child(high_pass_frequency);
    this->register_child(log_scale_frequencies);

    this->register_child(high_pass_filter_type);
    this->register_child(high_pass_filter_q);
    this->register_child(high_pass_filter_gain);

    this->register_child(high_pass_filter);

    for (size_t i = 0; i != COMB_FILTERS; ++i) {
        comb_filters[i].delay.set_feedback_signal_producer(
            &comb_filters[i].high_shelf_filter
        );

        mixer.add(comb_filters[i]);
        this->register_child(comb_filters[i]);
    }

    high_pass_filter_type.set_value(HighPassedInput::HIGH_PASS);
}


template<class InputSignalProducerClass>
void Reverb<InputSignalProducerClass>::reset() noexcept
{
    SideChainCompressableEffect<InputSignalProducerClass>::reset();

    previous_type = 255;
}


template<class InputSignalProducerClass>
Sample const* const* Reverb<InputSignalProducerClass>::initialize_rendering(
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

    Byte const type = this->type.get_value();

    if (previous_type != type) {
        previous_type = type;

        update_tunings(type);
    }

    mixer.set_output_buffer(this->buffer);
    SignalProducer::produce< Mixer<CombFilter> >(mixer, round, sample_count);

    return NULL;
}


template<class InputSignalProducerClass>
void Reverb<InputSignalProducerClass>::update_tunings(Byte const type) noexcept
{
    Tuning const* const tunings = TUNINGS[type];

    mixer.reset();

    for (size_t i = 0; i != COMB_FILTERS; ++i) {
        Tuning const& tuning = tunings[i];
        CombFilter& comb_filter = comb_filters[i];

        comb_filter.reset();
        comb_filter.delay.time.set_value(tuning.delay_time);
        comb_filter.set_panning_scale(tuning.panning_scale);

        mixer.set_weight(i, tuning.weight);
    }
}

}

#endif
