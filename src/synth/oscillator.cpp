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

#ifndef JS80P__SYNTH__OSCILLATOR_CPP
#define JS80P__SYNTH__OSCILLATOR_CPP

#include <cmath>

#include "synth/oscillator.hpp"

#include "synth/math.hpp"


namespace JS80P
{

template<class ModulatorSignalProducerClass>
FloatParam Oscillator<ModulatorSignalProducerClass>::dummy_param("", 0.0, 0.0, 0.0);


template<class ModulatorSignalProducerClass>
bool Oscillator<ModulatorSignalProducerClass>::Wavetable::is_initialized = false;


template<class ModulatorSignalProducerClass>
Number Oscillator<ModulatorSignalProducerClass>::Wavetable::sines[
    Oscillator<ModulatorSignalProducerClass>::Wavetable::SIZE
] = {0.0};


template<class ModulatorSignalProducerClass>
typename Oscillator<ModulatorSignalProducerClass>::Wavetable*
    Oscillator<ModulatorSignalProducerClass>::sine = NULL;

template<class ModulatorSignalProducerClass>
typename Oscillator<ModulatorSignalProducerClass>::Wavetable*
    Oscillator<ModulatorSignalProducerClass>::sawtooth = NULL;

template<class ModulatorSignalProducerClass>
typename Oscillator<ModulatorSignalProducerClass>::Wavetable*
    Oscillator<ModulatorSignalProducerClass>::inverse_sawtooth = NULL;

template<class ModulatorSignalProducerClass>
typename Oscillator<ModulatorSignalProducerClass>::Wavetable*
    Oscillator<ModulatorSignalProducerClass>::triangle = NULL;

template<class ModulatorSignalProducerClass>
typename Oscillator<ModulatorSignalProducerClass>::Wavetable*
    Oscillator<ModulatorSignalProducerClass>::square = NULL;


template<class ModulatorSignalProducerClass>
Integer Oscillator<ModulatorSignalProducerClass>::instances = 0;


template<class ModulatorSignalProducerClass>
Oscillator<ModulatorSignalProducerClass>::WaveformParam::WaveformParam(
        std::string const name
) : Param<Waveform>(name, SINE, CUSTOM, SAWTOOTH)
{
}


template<class ModulatorSignalProducerClass>
void Oscillator<ModulatorSignalProducerClass>::initialize_wavetables()
{
    Wavetable::initialize();

    if (Oscillator<ModulatorSignalProducerClass>::sine == NULL) {
        Number sine_coefficients[] = {1.0};
        Number sawtooth_coefficients[Wavetable::PARTIALS];
        Number inverse_sawtooth_coefficients[Wavetable::PARTIALS];
        Number triangle_coefficients[Wavetable::PARTIALS];
        Number square_coefficients[Wavetable::PARTIALS];

        for (Integer i = 0; i != Wavetable::PARTIALS; ++i) {
            Number const plus_or_minus_one = ((i & 1) == 1 ? -1.0 : 1.0);
            Number const i_pi = (Number)(i + 1) * Math::PI;
            Number const two_over_i_pi = 2.0 / i_pi;

            sawtooth_coefficients[i] = plus_or_minus_one * two_over_i_pi;
            inverse_sawtooth_coefficients[i] = -sawtooth_coefficients[i];
            triangle_coefficients[i] = (
                8.0 * Math::sin(i_pi / 2.0) / (i_pi * i_pi)
            );
            square_coefficients[i] = (1 + plus_or_minus_one) * two_over_i_pi;
        }

        Oscillator<ModulatorSignalProducerClass>::sine = (
            new Wavetable(sine_coefficients, 1)
        );
        Oscillator<ModulatorSignalProducerClass>::sawtooth = new Wavetable(
            sawtooth_coefficients, Wavetable::PARTIALS
        );
        Oscillator<ModulatorSignalProducerClass>::inverse_sawtooth = new Wavetable(
            inverse_sawtooth_coefficients, Wavetable::PARTIALS
        );
        Oscillator<ModulatorSignalProducerClass>::triangle = new Wavetable(
            triangle_coefficients, Wavetable::PARTIALS
        );
        Oscillator<ModulatorSignalProducerClass>::square = new Wavetable(
            square_coefficients, Wavetable::PARTIALS
        );
    }
}


template<class ModulatorSignalProducerClass>
void Oscillator<ModulatorSignalProducerClass>::free_wavetables()
{
    delete Oscillator<ModulatorSignalProducerClass>::sine;
    delete Oscillator<ModulatorSignalProducerClass>::sawtooth;
    delete Oscillator<ModulatorSignalProducerClass>::inverse_sawtooth;
    delete Oscillator<ModulatorSignalProducerClass>::triangle;
    delete Oscillator<ModulatorSignalProducerClass>::square;

    Oscillator<ModulatorSignalProducerClass>::sine = NULL;
    Oscillator<ModulatorSignalProducerClass>::sawtooth = NULL;
    Oscillator<ModulatorSignalProducerClass>::inverse_sawtooth = NULL;
    Oscillator<ModulatorSignalProducerClass>::triangle = NULL;
    Oscillator<ModulatorSignalProducerClass>::square = NULL;
}


template<class ModulatorSignalProducerClass>
Oscillator<ModulatorSignalProducerClass>::Oscillator(
        WaveformParam& waveform,
        ModulatorSignalProducerClass* modulator,
        FloatParam& amplitude_modulation_level_leader,
        FloatParam& frequency_modulation_level_leader
) : SignalProducer(1, 16),
    waveform(waveform),
    modulated_amplitude(
        modulator,
        amplitude_modulation_level_leader,
        "MA",
        0.0,
        1.0,
        1.0
    ),
    amplitude("", 0.0, 1.0, 1.0),
    frequency(
        modulator,
        frequency_modulation_level_leader,
        "MF",
        FREQUENCY_MIN,
        FREQUENCY_MAX,
        FREQUENCY_DEFAULT
    ),
    detune(
        "", Constants::DETUNE_MIN, Constants::DETUNE_MAX, 0.0
    ),
    fine_detune(
        "", Constants::FINE_DETUNE_MIN, Constants::FINE_DETUNE_MAX, 0.0
    ),
    harmonic_0("", -1.0, 1.0, 0.333),
    harmonic_1("", -1.0, 1.0, 0.333),
    harmonic_2("", -1.0, 1.0, 0.333),
    harmonic_3("", -1.0, 1.0, 0.0),
    harmonic_4("", -1.0, 1.0, 0.0),
    harmonic_5("", -1.0, 1.0, 0.0),
    harmonic_6("", -1.0, 1.0, 0.0),
    harmonic_7("", -1.0, 1.0, 0.0),
    harmonic_8("", -1.0, 1.0, 0.0),
    harmonic_9("", -1.0, 1.0, 0.0)
{
    initialize_instance();
}


template<class ModulatorSignalProducerClass>
void Oscillator<ModulatorSignalProducerClass>::initialize_instance()
{
    Number const custom_waveform[CUSTOM_WAVEFORM_HARMONICS] = {
        0.0, 0.0, 0.0, 0.0, 0.0,
        0.0, 0.0, 0.0, 0.0, 0.0,
    };
    computed_frequency_buffer = NULL;
    computed_amplitude_buffer = NULL;
    start_time_offset = 0.0;
    is_on = false;

    register_child(waveform);
    register_child(modulated_amplitude);
    register_child(amplitude);
    register_child(frequency);
    register_child(detune);
    register_child(fine_detune);

    register_child(harmonic_0);
    register_child(harmonic_1);
    register_child(harmonic_2);
    register_child(harmonic_3);
    register_child(harmonic_4);
    register_child(harmonic_5);
    register_child(harmonic_6);
    register_child(harmonic_7);
    register_child(harmonic_8);
    register_child(harmonic_9);

    custom_waveform_params[0] = &harmonic_0;
    custom_waveform_params[1] = &harmonic_1;
    custom_waveform_params[2] = &harmonic_2;
    custom_waveform_params[3] = &harmonic_3;
    custom_waveform_params[4] = &harmonic_4;
    custom_waveform_params[5] = &harmonic_5;
    custom_waveform_params[6] = &harmonic_6;
    custom_waveform_params[7] = &harmonic_7;
    custom_waveform_params[8] = &harmonic_8;
    custom_waveform_params[9] = &harmonic_9;

    if (++instances == 1) {
        initialize_wavetables();
    }

    wavetables[SINE] = sine;
    wavetables[SAWTOOTH] = sawtooth;
    wavetables[INVERSE_SAWTOOTH] = inverse_sawtooth;
    wavetables[TRIANGLE] = triangle;
    wavetables[SQUARE] = square;

    wavetables[CUSTOM] = new Wavetable(
        custom_waveform, CUSTOM_WAVEFORM_HARMONICS
    );

    allocate_buffers(block_size);

    for (int i = 0; i != CUSTOM_WAVEFORM_HARMONICS; ++i) {
        custom_waveform_change_indices[i] = -1;
    }
}


template<class ModulatorSignalProducerClass>
Oscillator<ModulatorSignalProducerClass>::Oscillator(
        WaveformParam& waveform,
        FloatParam& amplitude_leader,
        FloatParam& detune_leader,
        FloatParam& fine_detune_leader,
        FloatParam& harmonic_0_leader,
        FloatParam& harmonic_1_leader,
        FloatParam& harmonic_2_leader,
        FloatParam& harmonic_3_leader,
        FloatParam& harmonic_4_leader,
        FloatParam& harmonic_5_leader,
        FloatParam& harmonic_6_leader,
        FloatParam& harmonic_7_leader,
        FloatParam& harmonic_8_leader,
        FloatParam& harmonic_9_leader,
        ModulatorSignalProducerClass* modulator,
        FloatParam& amplitude_modulation_level_leader,
        FloatParam& frequency_modulation_level_leader
) : SignalProducer(1, 16),
    waveform(waveform),
    modulated_amplitude(
        modulator,
        amplitude_modulation_level_leader,
        modulator == NULL ? "XX" : "MA2",
        0.0,
        1.0,
        1.0
    ),
    amplitude(amplitude_leader),
    frequency(
        modulator,
        frequency_modulation_level_leader,
        "MF2",
        FREQUENCY_MIN,
        FREQUENCY_MAX,
        FREQUENCY_DEFAULT
    ),
    detune(detune_leader),
    fine_detune(fine_detune_leader),
    harmonic_0(harmonic_0_leader),
    harmonic_1(harmonic_1_leader),
    harmonic_2(harmonic_2_leader),
    harmonic_3(harmonic_3_leader),
    harmonic_4(harmonic_4_leader),
    harmonic_5(harmonic_5_leader),
    harmonic_6(harmonic_6_leader),
    harmonic_7(harmonic_7_leader),
    harmonic_8(harmonic_8_leader),
    harmonic_9(harmonic_9_leader)
{
    initialize_instance();
}


template<class ModulatorSignalProducerClass>
Oscillator<ModulatorSignalProducerClass>::~Oscillator()
{
    delete wavetables[CUSTOM];
    free_buffers();

    // TODO: std::shared_ptr?
    if (--instances == 0) {
        free_wavetables();
    }
}


template<class ModulatorSignalProducerClass>
void Oscillator<ModulatorSignalProducerClass>::allocate_buffers(
        Integer const size
) {
    computed_frequency_buffer = new Frequency[size];
    computed_amplitude_buffer = new Sample[size];
}


template<class ModulatorSignalProducerClass>
void Oscillator<ModulatorSignalProducerClass>::free_buffers()
{
    if (computed_frequency_buffer != NULL) {
        delete[] computed_frequency_buffer;
    }
}


template<class ModulatorSignalProducerClass>
void Oscillator<ModulatorSignalProducerClass>::set_block_size(
        Integer const new_block_size
) {
    if (new_block_size != get_block_size()) {
        free_buffers();
        allocate_buffers(new_block_size);
    }

    SignalProducer::set_block_size(new_block_size);
}


template<class ModulatorSignalProducerClass>
void Oscillator<ModulatorSignalProducerClass>::start(Seconds const time_offset)
{
    schedule(EVT_START, time_offset, 0, 0.0, 0.0);
}


template<class ModulatorSignalProducerClass>
void Oscillator<ModulatorSignalProducerClass>::stop(Seconds const time_offset)
{
    schedule(EVT_STOP, time_offset, 0, 0.0, 0.0);
}


template<class ModulatorSignalProducerClass>
Sample const* const* Oscillator<ModulatorSignalProducerClass>::initialize_rendering(
        Integer const round,
        Integer const sample_count
) {
    Waveform const waveform = this->waveform.get_value();

    if (waveform == CUSTOM) {
        bool has_changed = false;

        for (Integer i = 0; i != CUSTOM_WAVEFORM_HARMONICS; ++i) {
            Integer const param_change_idx = (
                custom_waveform_params[i]->get_change_index()
            );

            if (custom_waveform_change_indices[i] != param_change_idx) {
                custom_waveform_coefficients[i] = (
                    custom_waveform_params[i]->get_value()
                );
                custom_waveform_change_indices[i] = param_change_idx;
                has_changed = true;
            }

            FloatParam::produce_if_not_constant(
                custom_waveform_params[i], round, sample_count
            );
        }

        if (has_changed) {
            wavetables[CUSTOM]->update_coefficients(
                custom_waveform_coefficients, false
            );
        }
    }

    wavetable = wavetables[waveform];
    compute_amplitude_buffer(round, sample_count);
    compute_frequency_buffer(round, sample_count);

    return NULL;
}


template<class ModulatorSignalProducerClass>
void Oscillator<ModulatorSignalProducerClass>::compute_amplitude_buffer(
        Integer const round,
        Integer const sample_count
) {
    Sample const* modulated_amplitude_buffer = (
        FloatParam::produce_if_not_constant<ModulatedFloatParam>(
            &modulated_amplitude, round, sample_count
        )
    );
    Sample const* amplitude_buffer = (
        FloatParam::produce_if_not_constant(&amplitude, round, sample_count)
    );

    if (amplitude_buffer == NULL) {
        if (modulated_amplitude_buffer == NULL) {
            computed_amplitude_is_constant = true;
            computed_amplitude_value = (
                amplitude.get_value() * modulated_amplitude.get_value()
            );
        } else {
            computed_amplitude_is_constant = false;
            Sample const amplitude_value = (Sample)amplitude.get_value();

            for (Integer i = 0; i != sample_count; ++i) {
                computed_amplitude_buffer[i] = (
                    amplitude_value * modulated_amplitude_buffer[i]
                );
            }
        }
    } else {
        computed_amplitude_is_constant = false;

        if (modulated_amplitude_buffer == NULL) {
            Sample const modulated_amplitude_value = (
                (Sample)modulated_amplitude.get_value()
            );

            for (Integer i = 0; i != sample_count; ++i) {
                computed_amplitude_buffer[i] = (
                    amplitude_buffer[i] * modulated_amplitude_value
                );
            }
        } else {
            for (Integer i = 0; i != sample_count; ++i) {
                computed_amplitude_buffer[i] = (
                    amplitude_buffer[i] * modulated_amplitude_buffer[i]
                );
            }
        }
    }
}


template<class ModulatorSignalProducerClass>
void Oscillator<ModulatorSignalProducerClass>::compute_frequency_buffer(
        Integer const round,
        Integer const sample_count
) {
    constexpr Byte NONE = 0;
    constexpr Byte FREQUENCY = 1;
    constexpr Byte DETUNE = 2;
    constexpr Byte FINE_DETUNE = 4;
    constexpr Byte ALL = FREQUENCY | DETUNE | FINE_DETUNE;

    Sample const* const frequency_buffer = (
        FloatParam::produce_if_not_constant<ModulatedFloatParam>(
            &frequency, round, sample_count
        )
    );
    Sample const* const detune_buffer = (
        FloatParam::produce_if_not_constant(&detune, round, sample_count)
    );
    Sample const* const fine_detune_buffer = (
        FloatParam::produce_if_not_constant(&fine_detune, round, sample_count)
    );

    Byte const_param_flags = (
        frequency_buffer == NULL ? FREQUENCY : NONE
    );

    if (detune_buffer == NULL) {
        const_param_flags |= DETUNE;
    }

    if (fine_detune_buffer == NULL) {
        const_param_flags |= FINE_DETUNE;
    }

    switch (const_param_flags) {
        case NONE: {
            computed_frequency_is_constant = false;

            for (Integer i = 0; i != sample_count; ++i) {
                computed_frequency_buffer[i] = compute_frequency(
                    (Number)frequency_buffer[i],
                    (Number)detune_buffer[i],
                    (Number)fine_detune_buffer[i]
                );
            }

            break;
        }

        case FREQUENCY: {
            Number const frequency_value = frequency.get_value();

            computed_frequency_is_constant = false;

            for (Integer i = 0; i != sample_count; ++i) {
                computed_frequency_buffer[i] = compute_frequency(
                    frequency_value,
                    (Number)detune_buffer[i],
                    (Number)fine_detune_buffer[i]
                );
            }

            break;
        }

        case DETUNE: {
            Number const detune_value = detune.get_value();

            computed_frequency_is_constant = false;

            for (Integer i = 0; i != sample_count; ++i) {
                computed_frequency_buffer[i] = compute_frequency(
                    (Number)frequency_buffer[i],
                    detune_value,
                    (Number)fine_detune_buffer[i]
                );
            }

            break;
        }

        case FREQUENCY | DETUNE: {
            Number const frequency_value = frequency.get_value();
            Number const detune_value = detune.get_value();

            computed_frequency_is_constant = false;

            for (Integer i = 0; i != sample_count; ++i) {
                computed_frequency_buffer[i] = compute_frequency(
                    frequency_value,
                    detune_value,
                    (Number)fine_detune_buffer[i]
                );
            }

            break;
        }

        case FINE_DETUNE: {
            Number const fine_detune_value = fine_detune.get_value();

            computed_frequency_is_constant = false;

            for (Integer i = 0; i != sample_count; ++i) {
                computed_frequency_buffer[i] = compute_frequency(
                    (Number)frequency_buffer[i],
                    (Number)detune_buffer[i],
                    fine_detune_value
                );
            }

            break;
        }

        case FREQUENCY | FINE_DETUNE: {
            Number const frequency_value = frequency.get_value();
            Number const fine_detune_value = fine_detune.get_value();

            computed_frequency_is_constant = false;

            for (Integer i = 0; i != sample_count; ++i) {
                computed_frequency_buffer[i] = compute_frequency(
                    frequency_value,
                    (Number)detune_buffer[i],
                    fine_detune_value
                );
            }

            break;
        }

        case DETUNE | FINE_DETUNE: {
            Number const detune_value = detune.get_value();
            Number const fine_detune_value = fine_detune.get_value();

            computed_frequency_is_constant = false;

            for (Integer i = 0; i != sample_count; ++i) {
                computed_frequency_buffer[i] = compute_frequency(
                    (Number)frequency_buffer[i],
                    detune_value,
                    fine_detune_value
                );
            }

            break;
        }

        default:
        case ALL: {
            Number const frequency_value = frequency.get_value();
            Number const detune_value = detune.get_value();
            Number const fine_detune_value = fine_detune.get_value();

            computed_frequency_is_constant = true;
            computed_frequency_value = compute_frequency(
                frequency_value, detune_value, fine_detune_value
            );

            break;
        }
    }
}


template<class ModulatorSignalProducerClass>
Frequency Oscillator<ModulatorSignalProducerClass>::compute_frequency(
        Number const frequency,
        Number const detune,
        Number const fine_detune
) const {
    return Math::detune((Frequency)frequency, detune + fine_detune);
}


template<class ModulatorSignalProducerClass>
void Oscillator<ModulatorSignalProducerClass>::render(
        Integer const round,
        Integer const first_sample_index,
        Integer const last_sample_index,
        Sample** buffer
) {
    if (!is_on) {
        for (Integer i = first_sample_index; i != last_sample_index; ++i) {
            buffer[0][i] = 0.0;
        }

        return;
    }

    if (computed_frequency_is_constant) {
        if (UNLIKELY(is_starting)) {
            is_starting = false;
            Wavetable::reset_state(
                &wavetable_state,
                sampling_period,
                nyquist_frequency,
                computed_frequency_value,
                start_time_offset
            );
        }

        Frequency const computed_frequency_value = this->computed_frequency_value;

        if (computed_amplitude_is_constant) {
            Sample const amplitude_value = computed_amplitude_value;

            for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                buffer[0][i] = amplitude_value * wavetable->lookup(
                    &wavetable_state, computed_frequency_value
                );
            }
        } else {
            Sample const* amplitude_buffer = computed_amplitude_buffer;

            for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                buffer[0][i] = amplitude_buffer[i] * wavetable->lookup(
                    &wavetable_state, computed_frequency_value
                );
            }
        }

    } else {
        if (UNLIKELY(is_starting)) {
            is_starting = false;
            Wavetable::reset_state(
                &wavetable_state,
                sampling_period,
                nyquist_frequency,
                computed_frequency_buffer[first_sample_index],
                start_time_offset
            );
        }

        Frequency const* computed_frequency_buffer = (
            (Frequency const*)this->computed_frequency_buffer
        );

        if (computed_amplitude_is_constant) {
            Sample const amplitude_value = computed_amplitude_value;

            for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                buffer[0][i] = amplitude_value * wavetable->lookup(
                    &wavetable_state, computed_frequency_buffer[i]
                );
            }
        } else {
            Sample const* amplitude_buffer = computed_amplitude_buffer;

            for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                buffer[0][i] = amplitude_buffer[i] * wavetable->lookup(
                    &wavetable_state, computed_frequency_buffer[i]
                );
            }
        }
    }
}


template<class ModulatorSignalProducerClass>
void Oscillator<ModulatorSignalProducerClass>::handle_event(
        Event const& event
) {
    SignalProducer::handle_event(event);

    switch (event.type) {
        case EVT_START:
            handle_start_event(event);
            break;

        case EVT_STOP:
            handle_stop_event(event);
            break;
    }
}


template<class ModulatorSignalProducerClass>
void Oscillator<ModulatorSignalProducerClass>::handle_start_event(
        Event const& event
) {
    is_on = true;
    is_starting = true;
    start_time_offset = current_time - event.time_offset;
}


template<class ModulatorSignalProducerClass>
void Oscillator<ModulatorSignalProducerClass>::handle_stop_event(
        Event const& event
) {
    is_on = false;
}


template<class ModulatorSignalProducerClass>
Oscillator<ModulatorSignalProducerClass>::WavetableState::WavetableState()
{
}


template<class ModulatorSignalProducerClass>
void Oscillator<ModulatorSignalProducerClass>::Wavetable::initialize()
{
    if (is_initialized) {
        return;
    }

    is_initialized = true;

    for (Integer j = 0; j != SIZE; ++j) {
        sines[j] = Math::sin(((Number)j * SIZE_INV) * Math::PI_DOUBLE);
    }
}


template<class ModulatorSignalProducerClass>
void Oscillator<ModulatorSignalProducerClass>::Wavetable::reset_state(
        WavetableState* state,
        Seconds const sampling_period,
        Frequency const nyquist_frequency,
        Frequency const frequency,
        Seconds const start_time_offset
) {
    state->sample_index = (
        SIZE_FLOAT * (Number)start_time_offset * (Number)frequency
    );
    state->scale = SIZE_FLOAT * (Number)sampling_period;
    state->nyquist_frequency = nyquist_frequency;
    state->interpolation_limit = (
        nyquist_frequency * INTERPOLATION_LIMIT_SCALE
    );
}


template<class ModulatorSignalProducerClass>
Oscillator<ModulatorSignalProducerClass>::Wavetable::Wavetable(
        Number const coefficients[],
        Integer const coefficients_length
) : partials(coefficients_length)
{
    samples = new Sample*[partials];

    for (Integer i = 0; i != partials; ++i) {
        samples[i] = new Sample[SIZE];
    }

    update_coefficients(coefficients, true);
}


template<class ModulatorSignalProducerClass>
void Oscillator<ModulatorSignalProducerClass>::Wavetable::update_coefficients(
        Number const coefficients[],
        bool const normalize
) {
    Integer frequency = 1;
    Sample max = 0.0;
    Sample sample;

    /*
    samples[0]: 0 partials above fundamental
    samples[1]: 1 partial above fundamental
    ...
    samples[n]: n partials above fundamental
    */

    for (Integer j = 0; j != SIZE; ++j) {
        sample = std::fabs(
            samples[0][j] = (
                (Sample)(coefficients[0] * sines[(j * frequency) & MASK])
            )
        );

        if (UNLIKELY(normalize && sample > max)) {
            max = sample;
        }
    }

    for (Integer i = 1; i != partials; ++i) {
        ++frequency;

        for (Integer j = 0; j != SIZE; ++j) {
            sample = std::fabs(
                samples[i][j] = (
                    samples[i - 1][j]
                    + (Sample)(coefficients[i] * sines[(j * frequency) & MASK])
                )
            );

            if (UNLIKELY(normalize && sample > max)) {
                max = sample;
            }
        }
    }

    if (UNLIKELY(normalize)) {
        for (Integer i = 0; i != partials; ++i) {
            for (Integer j = 0; j != SIZE; ++j) {
                samples[i][j] /= max;
            }
        }
    }
}


template<class ModulatorSignalProducerClass>
Oscillator<ModulatorSignalProducerClass>::Wavetable::~Wavetable()
{
    for (Integer i = 0; i != partials; ++i) {
        delete[] samples[i];
    }

    delete[] samples;

    samples = NULL;
}


template<class ModulatorSignalProducerClass>
Sample Oscillator<ModulatorSignalProducerClass>::Wavetable::lookup(
        WavetableState* state,
        Frequency const frequency
) const {
    Frequency const abs_frequency = std::fabs(frequency);

    if (UNLIKELY(abs_frequency < 0.0000001)) {
        return 1.0;
    }

    if (UNLIKELY(abs_frequency > state->nyquist_frequency)) {
        return 0.0;
    }

    Number const sample_index = state->sample_index;

    state->sample_index = wrap_around(
        sample_index + state->scale * (Number)frequency
    );

    Integer const partials = Wavetable::partials;

    if (partials == 1) {
        state->needs_table_interpolation = false;
        state->table_indices[0] = 0;

        return interpolate(state, abs_frequency, sample_index);
    }

    Sample const max_partials = (Sample)(state->nyquist_frequency / abs_frequency);
    Integer const more_partials_index = (
        std::max((Integer)0, std::min(partials, (Integer)max_partials) - 1)
    );
    Integer const fewer_partials_index = (
        std::max((Integer)0, more_partials_index - 1)
    );

    state->table_indices[0] = fewer_partials_index;

    if (more_partials_index == fewer_partials_index) {
        state->needs_table_interpolation = false;

        return interpolate(state, abs_frequency, sample_index);
    }

    state->needs_table_interpolation = true;
    state->table_indices[1] = more_partials_index;

    Sample const fewer_partials_weight = max_partials - std::floor(max_partials);
    Sample const more_partials_weight = 1.0 - fewer_partials_weight;

    state->table_weights[0] = fewer_partials_weight;
    state->table_weights[1] = more_partials_weight;

    return interpolate(state, abs_frequency, sample_index);
}


template<class ModulatorSignalProducerClass>
Number Oscillator<ModulatorSignalProducerClass>::Wavetable::wrap_around(
        Number const index
) const {
    return index - std::floor(index * SIZE_INV) * SIZE_FLOAT;
}


template<class ModulatorSignalProducerClass>
Sample Oscillator<ModulatorSignalProducerClass>::Wavetable::interpolate(
        WavetableState const* state,
        Frequency const frequency,
        Number const sample_index
) const {
    if (frequency >= state->interpolation_limit) {
        return interpolate_sample_linear(state, sample_index);
    } else {
        return interpolate_sample_lagrange(state, sample_index);
    }
}


template<class ModulatorSignalProducerClass>
Sample Oscillator<ModulatorSignalProducerClass>::Wavetable::interpolate_sample_linear(
        WavetableState const* state,
        Number const sample_index
) const {
    Sample const sample_2_weight = (
        (Sample)(sample_index - std::floor(sample_index))
    );
    Sample const sample_1_weight = 1.0 - sample_2_weight;
    Integer const sample_1_index = (Integer)sample_index;
    Integer const sample_2_index = (sample_1_index + 1) & MASK;

    Sample const* table_1 = samples[state->table_indices[0]];

    if (!state->needs_table_interpolation) {
        return (
            sample_1_weight * table_1[sample_1_index]
            + sample_2_weight * table_1[sample_2_index]
        );
    }

    Sample const* table_2 = samples[state->table_indices[1]];

    return (
        state->table_weights[0] * (
            sample_1_weight * table_1[sample_1_index]
            + sample_2_weight * table_1[sample_2_index]
        )
        + state->table_weights[1] * (
            sample_1_weight * table_2[sample_1_index]
            + sample_2_weight * table_2[sample_2_index]
        )
    );
}


template<class ModulatorSignalProducerClass>
Sample Oscillator<ModulatorSignalProducerClass>::Wavetable::interpolate_sample_lagrange(
        WavetableState const* state,
        Number const sample_index
) const {
    Integer const sample_1_index = (Integer)sample_index;
    Integer const sample_2_index = (sample_1_index + 1) & MASK;
    Integer const sample_3_index = (sample_2_index + 1) & MASK;

    Sample const* table_1 = samples[state->table_indices[0]];

    // Formula and notation from http://dlmf.nist.gov/3.3#ii

    Sample const f_1_1 = table_1[sample_1_index];
    Sample const f_1_2 = table_1[sample_2_index];
    Sample const f_1_3 = table_1[sample_3_index];

    Sample const t = (Sample)(sample_index - std::floor(sample_index));
    Sample const t_sqr = t * t;

    Sample const a_1 = 0.5 * (t_sqr - t);
    Sample const a_2 = 1.0 - t_sqr;
    Sample const a_3 = 0.5 * (t_sqr + t);

    if (!state->needs_table_interpolation) {
        return a_1 * f_1_1 + a_2 * f_1_2 + a_3 * f_1_3;
    }

    Sample const* table_2 = samples[state->table_indices[1]];

    Sample const f_2_1 = table_2[sample_1_index];
    Sample const f_2_2 = table_2[sample_2_index];
    Sample const f_2_3 = table_2[sample_3_index];

    return (
        state->table_weights[0] * (a_1 * f_1_1 + a_2 * f_1_2 + a_3 * f_1_3)
        + state->table_weights[1] * (a_1 * f_2_1 + a_2 * f_2_2 + a_3 * f_2_3)
    );
}

}

#endif
