/*
 * This file is part of JS80P, a synthesizer plugin.
 * Copyright (C) 2025, 2026  Attila M. Magyar
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

#ifndef JS80P__DSP__NOISE_GENERATOR_CPP
#define JS80P__DSP__NOISE_GENERATOR_CPP

#include <algorithm>
#include <cmath>

#include "dsp/noise_generator.hpp"


namespace JS80P
{

template<class InputSignalProducerClass, class LevelParamClass>
NoiseGenerator<InputSignalProducerClass, LevelParamClass>::NoiseGenerator(
        InputSignalProducerClass& input,
        LevelParamClass& level,
        Frequency const high_pass_frequency,
        Frequency const low_pass_frequency,
        Math::RNG& rng,
        SignalProducer* const buffer_owner,
        Integer const channels
) noexcept
    : Filter<InputSignalProducerClass>(input, 1, channels, buffer_owner),
    high_pass_frequency(high_pass_frequency),
    low_pass_frequency(low_pass_frequency),
    level(level),
    rng(rng)
{
    this->register_child(this->level);

    r_n_m1 = new Sample[this->channels];
    x_n_m1 = new Sample[this->channels];
    y_n_m1 = new Sample[this->channels];

    update_filter_coefficients();
}


template<class InputSignalProducerClass, class LevelParamClass>
NoiseGenerator<InputSignalProducerClass, LevelParamClass>::~NoiseGenerator()
{
    delete[] r_n_m1;
    delete[] x_n_m1;
    delete[] y_n_m1;

    r_n_m1 = NULL;
    x_n_m1 = NULL;
    y_n_m1 = NULL;
}


template<class InputSignalProducerClass, class LevelParamClass>
void NoiseGenerator<InputSignalProducerClass, LevelParamClass>::set_sample_rate(
        Frequency const sample_rate
) noexcept {
    Filter<InputSignalProducerClass>::set_sample_rate(sample_rate);

    update_filter_coefficients();
}


template<class InputSignalProducerClass, class LevelParamClass>
void NoiseGenerator<InputSignalProducerClass, LevelParamClass>::reset() noexcept
{
    Filter<InputSignalProducerClass>::reset();

    clear_filters_state();
}


template<class InputSignalProducerClass, class LevelParamClass>
void NoiseGenerator<InputSignalProducerClass, LevelParamClass>::clear_filters_state() noexcept
{
    for (Integer c = 0; c != this->channels; ++c) {
        r_n_m1[c] = x_n_m1[c] = y_n_m1[c] = 0.0;
    }
}


template<class InputSignalProducerClass, class LevelParamClass>
void NoiseGenerator<InputSignalProducerClass, LevelParamClass>::update_filter_coefficients() noexcept
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

    Frequency const H = std::min(high_pass_frequency, this->sample_rate * 0.0625);
    Frequency const L = std::min(low_pass_frequency, this->sample_rate * 0.3500);
    Sample const PI_2_S = Math::PI_DOUBLE * this->sampling_period;
    Sample const v = PI_2_S * H;
    Sample const t = PI_2_S * L;

    this->a = 1.0 / (v + 1.0);
    this->w1 = t / (t + 1.0);
    this->w2 = 1.0 - this->w1;

    clear_filters_state();
}


template<class InputSignalProducerClass, class LevelParamClass>
Sample const* const* NoiseGenerator<InputSignalProducerClass, LevelParamClass>::initialize_rendering(
        Integer const round,
        Integer const sample_count
) noexcept {
    Sample const* const* const buffer = (
        Filter<InputSignalProducerClass>::initialize_rendering(
            round,
            sample_count
        )
    );

    level_buffer = LevelParamClass::produce_if_not_constant(level, round, sample_count);

    if (level_buffer == NULL && level.get_value() < 0.000001) {
        return buffer;
    }

    return NULL;
}


template<class InputSignalProducerClass, class LevelParamClass>
void NoiseGenerator<InputSignalProducerClass, LevelParamClass>::render(
        Integer const round,
        Integer const first_sample_index,
        Integer const last_sample_index,
        Sample** const buffer
) noexcept {
    if (level_buffer == NULL) {
        render<true>(round, first_sample_index, last_sample_index, buffer);
    } else {
        render<false>(round, first_sample_index, last_sample_index, buffer);
    }
}


template<class InputSignalProducerClass, class LevelParamClass>
template<bool is_level_constant>
void NoiseGenerator<InputSignalProducerClass, LevelParamClass>::render(
        Integer const round,
        Integer const first_sample_index,
        Integer const last_sample_index,
        Sample** const buffer
) noexcept {
    Sample const* const level_buffer = this->level_buffer;
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
            Sample const r_n = rng.random() * 2.0 - 1.0;
            Sample const x_n = a * (x_n_m1 + r_n - r_n_m1);
            Sample const y_n = w1 * x_n + w2 * y_n_m1;

            if constexpr (is_level_constant) {
                out_channel[i] = in_channel[i] + level * y_n;
            } else {
                out_channel[i] = in_channel[i] + level_buffer[i] * y_n;
            }

            r_n_m1 = r_n;
            x_n_m1 = x_n;
            y_n_m1 = y_n;
        }

        this->r_n_m1[c] = r_n_m1;
        this->x_n_m1[c] = x_n_m1;
        this->y_n_m1[c] = y_n_m1;
    }
}

}

#endif
