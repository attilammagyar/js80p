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
Oscillator<ModulatorSignalProducerClass>::WaveformParam::WaveformParam(
        std::string const name
) : Param<Waveform>(name, SINE, CUSTOM, SAWTOOTH)
{
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
    Number const custom_waveform_coefficients[CUSTOM_WAVEFORM_HARMONICS] = {
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

    for (int i = 0; i != CUSTOM_WAVEFORM_HARMONICS; ++i) {
        custom_waveform_change_indices[i] = -1;
    }

    custom_waveform = new Wavetable(
        custom_waveform_coefficients, CUSTOM_WAVEFORM_HARMONICS
    );

    wavetables[SINE] = StandardWavetables::sine();
    wavetables[SAWTOOTH] = StandardWavetables::sawtooth();
    wavetables[INVERSE_SAWTOOTH] = StandardWavetables::inverse_sawtooth();
    wavetables[TRIANGLE] = StandardWavetables::triangle();
    wavetables[SQUARE] = StandardWavetables::square();
    wavetables[CUSTOM] = custom_waveform;

    allocate_buffers(block_size);
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
            custom_waveform->update_coefficients(
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
                wavetable_state,
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
                wavetable_state,
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

}

#endif
