/*
 * This file is part of JS80P, a synthesizer plugin.
 * Copyright (C) 2025  Attila M. Magyar
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

#ifndef JS80P__DSP__TAPE_CPP
#define JS80P__DSP__TAPE_CPP

#include <algorithm>
#include <cmath>

#include "dsp/tape.hpp"

#include "dsp/math.hpp"


namespace JS80P
{

TapeParams::TapeParams(
        std::string const& name,
        ToggleParam& bypass_toggle
) noexcept
    : stop_start(name + "STP", 0.0, DELAY_TIME_MAX / 2.0, 0.0),
    wnf_amp(wnf_amp_macro.input),
    wnf_speed(wnf_speed_macro.input),
    distortion_level(name + "DST", 0.0, 1.0, 0.0),
    color(color_macro.input),
    hiss_level(name + "HSS", 0.0, 0.125, 0.0),
    stereo_separation(name + "STR", 0.0, 0.5, 0.0),
    distortion_type(name + "DTYP", Distortion::TYPE_TANH_5),
    bypass_toggle(bypass_toggle),
    volume(name + "VOL", 0.0, 1.0, 1.0),
    delay_time_lfo(name + "LD"),
    wow_lfo(name + "LW"),
    flutter_lfo(name + "LF"),
    delay_channel_lfo_1(name + "DCL1", stereo_separation),
    delay_channel_lfo_2(name + "DCL2", stereo_separation),
    wnf_amp_macro(name + "A", 0.0),
    wnf_amp_sharp_smooth_macro(name + "WFAHM"),
    wnf_amp_smooth_sharp_macro(name + "WFAMH"),
    wnf_speed_macro(name + "S", 0.5),
    wnf_speed_delay_time_lfo_macro(name + "WFSLD"),
    wnf_speed_wow_lfo_macro(name + "WFSLW"),
    wnf_speed_flutter_lfo_macro(name + "WFSLF"),
    delay_channel_lfo_1_frequency_macro(name + "ST1"),
    delay_channel_lfo_2_frequency_macro(name + "ST2"),
    color_macro(name + "C", 0.5),
    high_shelf_filter_frequency_macro(name + "HSF"),
    high_shelf_filter_gain_macro(name + "HSG"),
    offset_below_midpoint(name + "OB"),
    offset_above_midpoint(name + "OA"),
    distance_from_midpoint(name + "D"),
    low_pass_filter_frequency_macro(name + "LPF"),
    low_shelf_filter_gain_macro(name + "LSG"),
    state(State::TAPE_STATE_INIT)
{
    wnf_amp_smooth_sharp_macro.distortion.set_value(0.3);
    wnf_amp_smooth_sharp_macro.distortion_shape.set_value(
        Macro::DIST_SHAPE_SMOOTH_SHARP
    );
    wnf_amp_smooth_sharp_macro.input.set_macro(&wnf_amp_macro);

    wnf_amp_sharp_smooth_macro.distortion.set_value(0.2);
    wnf_amp_sharp_smooth_macro.distortion_shape.set_value(
        Macro::DIST_SHAPE_SHARP_SMOOTH
    );
    wnf_amp_sharp_smooth_macro.input.set_macro(&wnf_amp_macro);

    wnf_speed_delay_time_lfo_macro.min.set_value(0.37);
    wnf_speed_delay_time_lfo_macro.max.set_value(0.90);
    wnf_speed_delay_time_lfo_macro.input.set_macro(&wnf_speed_macro);

    wnf_speed_wow_lfo_macro.min.set_value(0.0);
    wnf_speed_wow_lfo_macro.max.set_value(0.6);
    wnf_speed_wow_lfo_macro.input.set_macro(&wnf_speed_macro);

    wnf_speed_flutter_lfo_macro.min.set_value(0.72);
    wnf_speed_flutter_lfo_macro.max.set_value(0.93);
    wnf_speed_flutter_lfo_macro.input.set_macro(&wnf_speed_macro);

    delay_channel_lfo_1_frequency_macro.min.set_value(0.0212);
    delay_channel_lfo_1_frequency_macro.max.set_value(0.1612);
    delay_channel_lfo_1_frequency_macro.distortion.set_value(0.8);
    delay_channel_lfo_1_frequency_macro.distortion_shape.set_value(
        Macro::DIST_SHAPE_SMOOTH_SHARP
    );
    delay_channel_lfo_1_frequency_macro.input.set_macro(&wnf_speed_macro);

    delay_channel_lfo_2_frequency_macro.min.set_value(0.0325);
    delay_channel_lfo_2_frequency_macro.max.set_value(0.1725);
    delay_channel_lfo_2_frequency_macro.distortion.set_value(0.8);
    delay_channel_lfo_2_frequency_macro.distortion_shape.set_value(
        Macro::DIST_SHAPE_SMOOTH_SHARP
    );
    delay_channel_lfo_2_frequency_macro.input.set_macro(&wnf_speed_macro);

    delay_time_lfo.freq_log_scale.set_value(ToggleParam::ON);
    delay_time_lfo.phase.set_lfo(&wow_lfo);
    delay_time_lfo.max.set_value(DELAY_TIME_LFO_RANGE);
    delay_time_lfo.frequency.set_macro(&wnf_speed_delay_time_lfo_macro);
    delay_time_lfo.amplitude.set_macro(&wnf_amp_smooth_sharp_macro);

    wow_lfo.freq_log_scale.set_value(ToggleParam::ON);
    wow_lfo.phase.set_lfo(&flutter_lfo);
    wow_lfo.max.set_value(0.5);
    wow_lfo.frequency.set_macro(&wnf_speed_wow_lfo_macro);
    wow_lfo.amplitude.set_macro(&wnf_amp_sharp_smooth_macro);

    flutter_lfo.freq_log_scale.set_value(ToggleParam::ON);
    flutter_lfo.max.set_value(0.5);
    flutter_lfo.frequency.set_macro(&wnf_speed_flutter_lfo_macro);
    flutter_lfo.amplitude.set_macro(&wnf_amp_sharp_smooth_macro);

    delay_channel_lfo_1.phase.set_value(0.3);
    delay_channel_lfo_1.distortion.set_value(0.15);
    delay_channel_lfo_1.waveform.set_value(LFO::Oscillator_::SOFT_TRIANGLE);
    delay_channel_lfo_1.frequency.set_macro(&delay_channel_lfo_1_frequency_macro);

    delay_channel_lfo_2.distortion.set_value(0.05);
    delay_channel_lfo_2.frequency.set_macro(&delay_channel_lfo_2_frequency_macro);

    high_shelf_filter_frequency_macro.midpoint.set_value(0.82);
    high_shelf_filter_frequency_macro.min.set_value(0.015);
    high_shelf_filter_frequency_macro.max.set_value(0.30);
    high_shelf_filter_frequency_macro.distortion.set_value(0.3);
    high_shelf_filter_frequency_macro.distortion_shape.set_value(
        Macro::DIST_SHAPE_SMOOTH_SHARP
    );
    high_shelf_filter_frequency_macro.input.set_macro(&color_macro);

    constexpr Number filter_gain_min = Constants::BIQUAD_FILTER_GAIN_MIN;
    constexpr Number filter_gain_max = Constants::BIQUAD_FILTER_GAIN_MAX;
    constexpr Number filter_gain_range = filter_gain_max - filter_gain_min;

    high_shelf_filter_gain_macro.midpoint.set_value(0.672);
    high_shelf_filter_gain_macro.min.set_value(
        (-30.0 - filter_gain_min) / filter_gain_range
    );
    high_shelf_filter_gain_macro.max.set_value(
        (9.0 - filter_gain_min) / filter_gain_range
    );
    high_shelf_filter_gain_macro.distortion.set_value(0.3);
    high_shelf_filter_gain_macro.distortion_shape.set_value(
        Macro::DIST_SHAPE_SMOOTH_SMOOTH
    );
    high_shelf_filter_gain_macro.input.set_macro(&color_macro);

    offset_below_midpoint.midpoint.set_value(1.0);
    offset_below_midpoint.min.set_value(1.0);
    offset_below_midpoint.max.set_value(0.0);
    offset_below_midpoint.input.set_macro(&color_macro);

    offset_above_midpoint.midpoint.set_value(0.0);
    offset_above_midpoint.input.set_macro(&color_macro);

    distance_from_midpoint.input.set_macro(&offset_below_midpoint);
    distance_from_midpoint.min.set_macro(&offset_above_midpoint);

    low_pass_filter_frequency_macro.min.set_value(1.0);
    low_pass_filter_frequency_macro.max.set_value(0.59);
    low_pass_filter_frequency_macro.distortion.set_value(0.5);
    low_pass_filter_frequency_macro.distortion_shape.set_value(
        Macro::DIST_SHAPE_SHARP_SMOOTH
    );
    low_pass_filter_frequency_macro.input.set_macro(&distance_from_midpoint);

    low_shelf_filter_gain_macro.min.set_value(
        (0.0 - filter_gain_min) / filter_gain_range
    );
    low_shelf_filter_gain_macro.max.set_value(
        (3.5 - filter_gain_min) / filter_gain_range
    );
    low_shelf_filter_gain_macro.distortion.set_value(0.3);
    low_shelf_filter_gain_macro.distortion_shape.set_value(
        Macro::DIST_SHAPE_SMOOTH_SHARP
    );
    low_shelf_filter_gain_macro.input.set_macro(&distance_from_midpoint);

    size_t i = 0;

    signal_producers[i++] = &stop_start;
    signal_producers[i++] = &distortion_level;
    signal_producers[i++] = &hiss_level;
    signal_producers[i++] = &stereo_separation;
    signal_producers[i++] = &distortion_type;
    signal_producers[i++] = &volume;
    signal_producers[i++] = &delay_time_lfo;
    signal_producers[i++] = &wow_lfo;
    signal_producers[i++] = &flutter_lfo;
    signal_producers[i++] = &delay_channel_lfo_1;
    signal_producers[i++] = &delay_channel_lfo_2;

    store_signal_producers_from_macro(wnf_amp_macro, i);
    store_signal_producers_from_macro(wnf_amp_sharp_smooth_macro, i);
    store_signal_producers_from_macro(wnf_amp_smooth_sharp_macro, i);
    store_signal_producers_from_macro(wnf_speed_macro, i);
    store_signal_producers_from_macro(wnf_speed_delay_time_lfo_macro, i);
    store_signal_producers_from_macro(wnf_speed_wow_lfo_macro, i);
    store_signal_producers_from_macro(wnf_speed_flutter_lfo_macro, i);
    store_signal_producers_from_macro(delay_channel_lfo_1_frequency_macro, i);
    store_signal_producers_from_macro(delay_channel_lfo_2_frequency_macro, i);
    store_signal_producers_from_macro(color_macro, i);
    store_signal_producers_from_macro(high_shelf_filter_frequency_macro, i);
    store_signal_producers_from_macro(high_shelf_filter_gain_macro, i);
    store_signal_producers_from_macro(offset_below_midpoint, i);
    store_signal_producers_from_macro(offset_above_midpoint, i);
    store_signal_producers_from_macro(distance_from_midpoint, i);
    store_signal_producers_from_macro(low_pass_filter_frequency_macro, i);
    store_signal_producers_from_macro(low_shelf_filter_gain_macro, i);

    JS80P_ASSERT(i == (size_t)SIGNAL_PRODUCERS);
}


void TapeParams::store_signal_producers_from_macro(Macro& macro, size_t& i) noexcept
{
    signal_producers[i++] = &macro.midpoint;
    signal_producers[i++] = &macro.input;
    signal_producers[i++] = &macro.min;
    signal_producers[i++] = &macro.max;
    signal_producers[i++] = &macro.scale;
    signal_producers[i++] = &macro.distortion;
    signal_producers[i++] = &macro.randomness;
    signal_producers[i++] = &macro.distortion_shape;
}


SignalProducer* TapeParams::get_signal_producer(size_t const n) const noexcept
{
    return n < SIGNAL_PRODUCERS ? signal_producers[n] : NULL;
}


void TapeParams::start_lfos(Seconds const time_offset) noexcept
{
    delay_time_lfo.start(time_offset);
    wow_lfo.start(time_offset);
    flutter_lfo.start(time_offset);
    delay_channel_lfo_1.start(time_offset);
    delay_channel_lfo_2.start(time_offset);
}


void TapeParams::stop_lfos(Seconds const time_offset) noexcept
{
    delay_time_lfo.stop(time_offset);
    wow_lfo.stop(time_offset);
    flutter_lfo.stop(time_offset);
    delay_channel_lfo_1.stop(time_offset);
    delay_channel_lfo_2.stop(time_offset);
}


void TapeParams::skip_round_for_lfos(
        Integer const round,
        Integer const sample_count
) noexcept {
    delay_time_lfo.skip_round(round, sample_count);
    wow_lfo.skip_round(round, sample_count);
    flutter_lfo.skip_round(round, sample_count);
    delay_channel_lfo_1.skip_round(round, sample_count);
    delay_channel_lfo_2.skip_round(round, sample_count);
}


template<class InputSignalProducerClass, Byte required_bypass_toggle_value>
template<class HissInputSignalProducerClass>
Tape<
        InputSignalProducerClass,
        required_bypass_toggle_value
>::HissGenerator<
        HissInputSignalProducerClass
>::HissGenerator(
        HissInputSignalProducerClass& input,
        FloatParamB& level
) noexcept
    : Filter<HissInputSignalProducerClass>(input),
    level(level),
    rng(0x1c99)
{
    r_n_m1 = new Sample[this->channels];
    x_n_m1 = new Sample[this->channels];
    y_n_m1 = new Sample[this->channels];

    update_filter_coefficients();
}


template<class InputSignalProducerClass, Byte required_bypass_toggle_value>
template<class HissInputSignalProducerClass>
Tape<
        InputSignalProducerClass,
        required_bypass_toggle_value
>::HissGenerator<
        HissInputSignalProducerClass
>::~HissGenerator()
{
    delete[] r_n_m1;
    delete[] x_n_m1;
    delete[] y_n_m1;

    r_n_m1 = NULL;
    x_n_m1 = NULL;
    y_n_m1 = NULL;
}


template<class InputSignalProducerClass, Byte required_bypass_toggle_value>
template<class HissInputSignalProducerClass>
void Tape<
        InputSignalProducerClass,
        required_bypass_toggle_value
>::HissGenerator<
        HissInputSignalProducerClass
>::set_sample_rate(Frequency const sample_rate) noexcept
{
    Filter<HissInputSignalProducerClass>::set_sample_rate(sample_rate);

    update_filter_coefficients();
}


template<class InputSignalProducerClass, Byte required_bypass_toggle_value>
template<class HissInputSignalProducerClass>
void Tape<
        InputSignalProducerClass,
        required_bypass_toggle_value
>::HissGenerator<
        HissInputSignalProducerClass
>::reset() noexcept
{
    Filter<HissInputSignalProducerClass>::reset();

    rng.reset();

    for (Integer c = 0; c != this->channels; ++c) {
        r_n_m1[c] = x_n_m1[c] = y_n_m1[c] = 0.0;
    }
}


template<class InputSignalProducerClass, Byte required_bypass_toggle_value>
template<class HissInputSignalProducerClass>
void Tape<
        InputSignalProducerClass,
        required_bypass_toggle_value
>::HissGenerator<
        HissInputSignalProducerClass
>::update_filter_coefficients() noexcept
{
    /*
    Simple low-pass and high-pass filters. See:
     - https://en.wikipedia.org/wiki/Low-pass_filter#Discrete-time_realization
     - https://en.wikipedia.org/wiki/High-pass_filter#Discrete-time_realization

    The two filters are combined using the following notation:

     - High-pass:

           S := sampling period length
           H := high-pass cut-off frequency
           r[n] := n-th raw sample (random noise)

           v := 2 * pi * S * H
           a := 1 / (v + 1)
           x[n] := a * (x[n - 1] + r[n] - r[n - 1])

     - Low-pass:

           L := low-pass cut-off frequency
           t := 2 * pi * S * L
           w1 := t / (t + 1)
           w2 := 1 - w1
           y[n] := w1 * x[n] + (1 - w2) * y[n - 1]

    */

    Frequency const H = std::min(30.0, this->sample_rate * 0.0625);
    Frequency const L = std::min(600.0, this->sample_rate * 0.125);
    Sample const PI_2_S = Math::PI_DOUBLE * this->sampling_period;
    Sample const v = PI_2_S * H;
    Sample const t = PI_2_S * L;

    this->a = 1.0 / (v + 1.0);
    this->w1 = t / (t + 1.0);
    this->w2 = 1.0 - this->w1;
}


template<class InputSignalProducerClass, Byte required_bypass_toggle_value>
template<class HissInputSignalProducerClass>
Sample const* const* Tape<
        InputSignalProducerClass,
        required_bypass_toggle_value
>::HissGenerator<
        HissInputSignalProducerClass
>::initialize_rendering(
        Integer const round,
        Integer const sample_count
) noexcept {
    Sample const* const* buffer = (
        Filter<HissInputSignalProducerClass>::initialize_rendering(
            round,
            sample_count
        )
    );

    if (level.get_value() < 0.000001) {
        return buffer;
    }

    return NULL;
}


template<class InputSignalProducerClass, Byte required_bypass_toggle_value>
template<class HissInputSignalProducerClass>
void Tape<
        InputSignalProducerClass,
        required_bypass_toggle_value
>::HissGenerator<
        HissInputSignalProducerClass
>::render(
        Integer const round,
        Integer const first_sample_index,
        Integer const last_sample_index,
        Sample** buffer
) noexcept {
    Sample const level = this->level.get_value();
    Sample const a = this->a;
    Sample const w1 = this->w1;
    Sample const w2 = this->w2;

    for (Integer c = 0; c != this->channels; ++c) {
        Sample* const out_channel = buffer[c];
        Sample const* const in_channel = this->input_buffer[c];
        Sample r_n_m1 = this->r_n_m1[c];
        Sample x_n_m1 = this->x_n_m1[c];
        Sample y_n_m1 = this->y_n_m1[c];

        for (Integer i = first_sample_index; i != last_sample_index; ++i) {
            Sample const r_n = rng.random();
            Sample const x_n = a * (x_n_m1 + r_n - r_n_m1);
            Sample const y_n = w1 * x_n + w2 * y_n_m1;

            out_channel[i] = in_channel[i] + level * y_n;

            r_n_m1 = r_n;
            x_n_m1 = x_n;
            y_n_m1 = y_n;
        }

        this->r_n_m1[c] = r_n_m1;
        this->x_n_m1[c] = x_n_m1;
        this->y_n_m1[c] = y_n_m1;
    }
}


template<class InputSignalProducerClass, Byte required_bypass_toggle_value>
Tape<InputSignalProducerClass, required_bypass_toggle_value>::Tape(
        std::string const& name,
        TapeParams& params,
        InputSignalProducerClass& input
) noexcept
    : Filter<InputSignalProducerClass>(input, 6, 0, &delay),
    params(params),
    distortion(
        name + "DIST",
        params.distortion_type,
        input,
        params.distortion_level,
        input.get_buffer_owner()
    ),
    low_shelf_filter(
        name + "LS",
        distortion,
        distortion.get_buffer_owner()
    ),
    hiss_generator(low_shelf_filter, params.hiss_level),
    high_shelf_filter(
        name + "HS",
        hiss_generator,
        hiss_generator.get_buffer_owner()
    ),
    low_pass_filter(
        name + "LP",
        high_shelf_filter,
        high_shelf_filter.get_buffer_owner()
    ),
    delay(low_pass_filter, NULL, TapeParams::DELAY_TIME_MAX),
    transition_duration(0.0),
    previous_bypass_toggle_value(params.bypass_toggle.get_value()),
    needs_ff_rescheduling(true)
{
    this->register_child(distortion);
    this->register_child(low_shelf_filter);
    this->register_child(hiss_generator);
    this->register_child(high_shelf_filter);
    this->register_child(low_pass_filter);
    this->register_child(delay);

    low_shelf_filter.type.set_value(HighShelfFilter::LOW_SHELF);
    low_shelf_filter.q.set_value(0.0);
    low_shelf_filter.gain.set_macro(&params.low_shelf_filter_gain_macro);
    low_shelf_filter.frequency.set_value(110.0);

    high_shelf_filter.type.set_value(HighShelfFilter::HIGH_SHELF);
    high_shelf_filter.q.set_value(0.0);
    high_shelf_filter.gain.set_macro(&params.high_shelf_filter_gain_macro);
    high_shelf_filter.frequency.set_macro(
        &params.high_shelf_filter_frequency_macro
    );

    low_pass_filter.type.set_value(LowPassFilter::LOW_PASS);
    low_pass_filter.q.set_value(0.0);
    low_pass_filter.frequency.set_macro(&params.low_pass_filter_frequency_macro);

    delay.time.set_lfo(&params.delay_time_lfo);
    delay.gain.set_value(1.0);
    delay.set_channel_lfo(0, params.delay_channel_lfo_1, 0.0037);
    delay.set_channel_lfo(1, params.delay_channel_lfo_2, 0.0043);
}


template<class InputSignalProducerClass, Byte required_bypass_toggle_value>
void Tape<
        InputSignalProducerClass,
        required_bypass_toggle_value
>::reset() noexcept
{
    Filter<InputSignalProducerClass>::reset();

    transition_duration = 0.0;
    needs_ff_rescheduling = true;

    params.state = TapeParams::State::TAPE_STATE_INIT;

    params.volume.cancel_events();
    params.volume.set_value(1.0);

    params.delay_time_lfo.min.cancel_events();
    params.delay_time_lfo.min.set_value(0.0);

    params.delay_time_lfo.max.cancel_events();
    params.delay_time_lfo.max.set_value(TapeParams::DELAY_TIME_LFO_RANGE);
}


template<class InputSignalProducerClass, Byte required_bypass_toggle_value>
Sample const* const* Tape<
        InputSignalProducerClass,
        required_bypass_toggle_value
>::initialize_rendering(
        Integer const round,
        Integer const sample_count
) noexcept {
    Sample const* const* input_buffer = (
        Filter<InputSignalProducerClass>::initialize_rendering(
            round,
            sample_count
        )
    );
    Byte const toggle = params.bypass_toggle.get_value();

    if (JS80P_UNLIKELY(toggle != previous_bypass_toggle_value)) {
        this->reset();

        previous_bypass_toggle_value = toggle;
    }

    if (toggle != required_bypass_toggle_value) {
        return input_buffer;
    }

    Sample const* const* result = NULL;

    switch (params.state) {
        case TapeParams::State::TAPE_STATE_INIT:
            result = initialize_init_rendering(round, sample_count);
            break;

        case TapeParams::State::TAPE_STATE_NORMAL:
            result = initialize_normal_rendering(round, sample_count);
            break;

        case TapeParams::State::TAPE_STATE_STOPPING:
            result = initialize_stopping_rendering(round, sample_count);
            break;

        case TapeParams::State::TAPE_STATE_STOPPED:
            result = initialize_stopped_rendering(round, sample_count);
            break;

        case TapeParams::State::TAPE_STATE_STARTABLE:
            result = initialize_startable_rendering(round, sample_count);
            break;

        case TapeParams::State::TAPE_STATE_STARTING:
            result = initialize_starting_rendering(round, sample_count);
            break;

        case TapeParams::State::TAPE_STATE_STARTED:
            result = initialize_started_rendering(round, sample_count);
            break;

        case TapeParams::State::TAPE_STATE_FF_STARTABLE:
            result = initialize_ff_startable_rendering(round, sample_count);
            break;

        case TapeParams::State::TAPE_STATE_FF_STARTING:
            result = initialize_ff_starting_rendering(round, sample_count);
            break;

        case TapeParams::State::TAPE_STATE_FF_STARTED:
            result = initialize_ff_started_rendering(round, sample_count);
            break;

        default:
            return input_buffer;
    }

    if (result != NULL) {
        return result;
    }

    volume_buffer = FloatParamS::produce_if_not_constant(
        params.volume, round, sample_count
    );

    delay_output = SignalProducer::produce<Delay_>(delay, round, sample_count);

    return result;
}


template<class InputSignalProducerClass, Byte required_bypass_toggle_value>
Sample const* const* Tape<
        InputSignalProducerClass,
        required_bypass_toggle_value
>::initialize_init_rendering(
        Integer const round,
        Integer const sample_count
) noexcept {
    if (params.stop_start.get_value() < 0.000001) {
        transition_duration = 0.0;
        params.state = TapeParams::State::TAPE_STATE_NORMAL;
    }

    if (is_bypassable()) {
        return this->input_buffer;
    }

    return NULL;
}


template<class InputSignalProducerClass, Byte required_bypass_toggle_value>
Sample const* const* Tape<
        InputSignalProducerClass,
        required_bypass_toggle_value
>::initialize_normal_rendering(
        Integer const round,
        Integer const sample_count
) noexcept {
    Seconds const new_transition_duration = (
        (Seconds)params.stop_start.get_value()
    );

    if (JS80P_LIKELY(new_transition_duration < 0.000001)) {
        if (is_bypassable()) {
            return this->input_buffer;
        }

        return NULL;
    }

    schedule_stop(std::max(STOP_TIME_MIN, new_transition_duration));

    return NULL;
}


template<class InputSignalProducerClass, Byte required_bypass_toggle_value>
bool Tape<
        InputSignalProducerClass,
        required_bypass_toggle_value
>::is_bypassable() const noexcept {
    return (
        params.wnf_amp.get_value() < 0.000001
        && params.distortion_level.get_value() < 0.000001
        && Math::is_close(params.color.get_value(), 0.5, 0.005)
        && params.hiss_level.get_value() < 0.000001
        && params.stereo_separation.get_value() < 0.000001
    );
}


template<class InputSignalProducerClass, Byte required_bypass_toggle_value>
Sample const* const* Tape<
        InputSignalProducerClass,
        required_bypass_toggle_value
>::initialize_stopping_rendering(
        Integer const round,
        Integer const sample_count
) noexcept {
    Number const new_stop_start_value = params.stop_start.get_value();
    Seconds const new_transition_duration = std::max(
        STOP_TIME_MIN, (Seconds)new_stop_start_value
    );

    if (Math::is_close(new_transition_duration, transition_duration)) {
        if (params.volume.get_value() < 0.000001) {
            params.volume.set_value(0.0);
            params.state = TapeParams::State::TAPE_STATE_STOPPED;
        }
    } else if (new_transition_duration < transition_duration) {
        schedule_fast_forward_start(0.1);
        needs_ff_rescheduling = false;
    } else {
        schedule_stop(new_transition_duration);
    }

    return NULL;
}


template<class InputSignalProducerClass, Byte required_bypass_toggle_value>
Sample const* const* Tape<
        InputSignalProducerClass,
        required_bypass_toggle_value
>::initialize_stopped_rendering(
        Integer const round,
        Integer const sample_count
) noexcept {
    Number const new_stop_start_value = params.stop_start.get_value();

    if (new_stop_start_value < 0.000001) {
        transition_duration = 0.0;
        params.state = TapeParams::State::TAPE_STATE_FF_STARTABLE;
    } else if (new_stop_start_value > transition_duration) {
        params.state = TapeParams::State::TAPE_STATE_STARTABLE;
    } else {
        transition_duration = new_stop_start_value;
    }

    return NULL;
}


template<class InputSignalProducerClass, Byte required_bypass_toggle_value>
Sample const* const* Tape<
        InputSignalProducerClass,
        required_bypass_toggle_value
>::initialize_startable_rendering(
        Integer const round,
        Integer const sample_count
) noexcept {
    schedule_start();

    return NULL;
}


template<class InputSignalProducerClass, Byte required_bypass_toggle_value>
Sample const* const* Tape<
        InputSignalProducerClass,
        required_bypass_toggle_value
>::initialize_starting_rendering(
        Integer const round,
        Integer const sample_count
) noexcept {
    if (params.volume.get_value() >= 0.999999) {
        transition_duration = 0.0;

        params.volume.set_value(1.0);
        params.state = TapeParams::State::TAPE_STATE_STARTED;
    }

    return NULL;
}


template<class InputSignalProducerClass, Byte required_bypass_toggle_value>
Sample const* const* Tape<
        InputSignalProducerClass,
        required_bypass_toggle_value
>::initialize_started_rendering(
        Integer const round,
        Integer const sample_count
) noexcept {
    Seconds const new_transition_duration = (
        (Seconds)params.stop_start.get_value()
    );

    if (new_transition_duration < 0.000001) {
        transition_duration = 0.0;
        params.state = TapeParams::State::TAPE_STATE_NORMAL;
    }

    if (is_bypassable()) {
        return this->input_buffer;
    }

    return NULL;
}


template<class InputSignalProducerClass, Byte required_bypass_toggle_value>
Sample const* const* Tape<
        InputSignalProducerClass,
        required_bypass_toggle_value
>::initialize_ff_startable_rendering(
        Integer const round,
        Integer const sample_count
) noexcept {
    Number const new_stop_start_value = params.stop_start.get_value();

    if (new_stop_start_value > 0.000001) {
        schedule_fast_forward_start(
            std::max(START_TIME_MIN, (Seconds)new_stop_start_value)
        );
    }

    needs_ff_rescheduling = true;

    return NULL;
}


template<class InputSignalProducerClass, Byte required_bypass_toggle_value>
Sample const* const* Tape<
        InputSignalProducerClass,
        required_bypass_toggle_value
>::initialize_ff_starting_rendering(
        Integer const round,
        Integer const sample_count
) noexcept {
    Seconds const new_transition_duration = std::max(
        START_TIME_MIN, (Seconds)params.stop_start.get_value()
    );

    if (
            needs_ff_rescheduling
            && !Math::is_close(new_transition_duration, transition_duration)
    ) {
        schedule_fast_forward_start(new_transition_duration);
    } else if (params.volume.get_value() >= 0.999999) {
        transition_duration = 0.0;

        params.volume.set_value(1.0);
        params.state = TapeParams::State::TAPE_STATE_FF_STARTED;
    }

    return NULL;
}


template<class InputSignalProducerClass, Byte required_bypass_toggle_value>
Sample const* const* Tape<
        InputSignalProducerClass,
        required_bypass_toggle_value
>::initialize_ff_started_rendering(
        Integer const round,
        Integer const sample_count
) noexcept {
    Seconds const new_transition_duration = (
        (Seconds)params.stop_start.get_value()
    );

    if (new_transition_duration < 0.000001) {
        transition_duration = 0.0;
        params.state = TapeParams::State::TAPE_STATE_NORMAL;
    }

    if (is_bypassable()) {
        return this->input_buffer;
    }

    return NULL;
}


template<class InputSignalProducerClass, Byte required_bypass_toggle_value>
void Tape<InputSignalProducerClass, required_bypass_toggle_value>::schedule_stop(
        Seconds const duration
) noexcept {
    constexpr Number delay_time_min_max = 1.0 - TapeParams::DELAY_TIME_LFO_RANGE;

    transition_duration = duration;
    params.state = TapeParams::State::TAPE_STATE_STOPPING;

    Number const delay_time_increase_as_ratio = (
        delay.time.value_to_ratio(duration * 0.5)
    );

    Number const delay_time_lfo_min_target = std::min(
        delay_time_min_max,
        params.delay_time_lfo.min.get_value() + delay_time_increase_as_ratio
    );
    Number const delay_time_lfo_max_target = std::min(
        1.0, delay_time_lfo_min_target + TapeParams::DELAY_TIME_LFO_RANGE
    );

    params.delay_time_lfo.min.cancel_events_at(STOP_START_DELAY);
    params.delay_time_lfo.min.schedule_curved_ramp(
        duration,
        delay_time_lfo_min_target,
        Math::EnvelopeShape::ENV_SHAPE_SMOOTH_SHARP
    );

    params.delay_time_lfo.max.cancel_events_at(STOP_START_DELAY);
    params.delay_time_lfo.max.schedule_curved_ramp(
        duration,
        delay_time_lfo_max_target,
        Math::EnvelopeShape::ENV_SHAPE_SMOOTH_SHARP
    );

    params.volume.cancel_events_at(STOP_START_DELAY);
    params.volume.schedule_curved_ramp(
        duration,
        0.0,
        Math::EnvelopeShape::ENV_SHAPE_SMOOTH_SHARP_STEEP
    );
}


template<class InputSignalProducerClass, Byte required_bypass_toggle_value>
void Tape<
        InputSignalProducerClass,
        required_bypass_toggle_value
>::schedule_start() noexcept {
    transition_duration = 0.0;

    params.state = TapeParams::State::TAPE_STATE_STARTING;

    params.delay_time_lfo.min.cancel_events_at(STOP_START_DELAY);
    params.delay_time_lfo.min.schedule_value(STOP_START_DELAY, 0.0);

    params.delay_time_lfo.max.cancel_events_at(STOP_START_DELAY);
    params.delay_time_lfo.max.schedule_value(
        STOP_START_DELAY,
        TapeParams::DELAY_TIME_LFO_RANGE
    );

    params.volume.cancel_events_at(STOP_START_DELAY);
    params.volume.schedule_curved_ramp(
        START_TIME_MIN,
        1.0,
        Math::EnvelopeShape::ENV_SHAPE_SMOOTH_SMOOTH
    );
}


template<class InputSignalProducerClass, Byte required_bypass_toggle_value>
void Tape<
        InputSignalProducerClass,
        required_bypass_toggle_value
>::schedule_fast_forward_start(
        Seconds const duration
) noexcept {
    transition_duration = duration;

    params.state = TapeParams::State::TAPE_STATE_FF_STARTING;

    params.delay_time_lfo.min.cancel_events_at(STOP_START_DELAY);
    params.delay_time_lfo.min.schedule_curved_ramp(
        duration,
        0.0,
        Math::EnvelopeShape::ENV_SHAPE_SHARP_SMOOTH
    );

    params.delay_time_lfo.max.cancel_events_at(STOP_START_DELAY);
    params.delay_time_lfo.max.schedule_curved_ramp(
        duration,
        TapeParams::DELAY_TIME_LFO_RANGE,
        Math::EnvelopeShape::ENV_SHAPE_SHARP_SMOOTH
    );

    params.volume.cancel_events_at(STOP_START_DELAY);
    params.volume.schedule_curved_ramp(
        duration,
        1.0,
        Math::EnvelopeShape::ENV_SHAPE_SHARP_SMOOTH
    );
}


template<class InputSignalProducerClass, Byte required_bypass_toggle_value>
void Tape<InputSignalProducerClass, required_bypass_toggle_value>::render(
        Integer const round,
        Integer const first_sample_index,
        Integer const last_sample_index,
        Sample** buffer
) noexcept {
    if (params.state == TapeParams::State::TAPE_STATE_STOPPED) {
        this->render_silence(
            round, first_sample_index, last_sample_index, buffer
        );

        return;
    }

    if (volume_buffer == NULL) {
        Sample const volume_level = params.volume.get_value();

        for (Integer c = 0; c != this->channels; ++c) {
            Sample* const channel = buffer[c];

            for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                /*
                Conveniently, our buffer owner is the delay, so the buffer
                already contains its rendered signal, we just have to apply
                our volume.
                */
                channel[i] *= volume_level;
            }
        }
    } else {
        for (Integer c = 0; c != this->channels; ++c) {
            Sample* const channel = buffer[c];

            for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                /*
                Conveniently, our buffer owner is the delay, so the buffer
                already contains its rendered signal, we just have to apply
                our volume.
                */
                channel[i] *= volume_buffer[i];
            }
        }
    }
}


template<class InputSignalProducerClass, Byte required_bypass_toggle_value>
Sample Tape<
        InputSignalProducerClass,
        required_bypass_toggle_value
>::distort_volume(
        Sample const volume_level
) noexcept {
    return Math::apply_envelope_shape(
        Math::EnvelopeShape::ENV_SHAPE_SMOOTH_SHARP_STEEPER,
        volume_level
    );
}

}

#endif
