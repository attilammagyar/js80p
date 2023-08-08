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

#ifndef JS80P__DSP__OSCILLATOR_CPP
#define JS80P__DSP__OSCILLATOR_CPP

#include <cmath>

#include "dsp/oscillator.hpp"

#include "dsp/math.hpp"


namespace JS80P
{

template<class ModulatorSignalProducerClass, bool is_lfo>
FloatParam Oscillator<ModulatorSignalProducerClass, is_lfo>::dummy_param("", 0.0, 0.0, 0.0);

template<class ModulatorSignalProducerClass, bool is_lfo>
ToggleParam Oscillator<ModulatorSignalProducerClass, is_lfo>::dummy_toggle("", ToggleParam::OFF);


template<class ModulatorSignalProducerClass, bool is_lfo>
Oscillator<ModulatorSignalProducerClass, is_lfo>::WaveformParam::WaveformParam(
        std::string const name,
        Waveform const max_value
) noexcept : Param<Waveform>(name, SINE, max_value, SINE)
{
}


template<class ModulatorSignalProducerClass, bool is_lfo>
Oscillator<ModulatorSignalProducerClass, is_lfo>::Oscillator(
        WaveformParam& waveform,
        ModulatorSignalProducerClass* modulator,
        FloatParam& amplitude_modulation_level_leader,
        FloatParam& frequency_modulation_level_leader,
        FloatParam& phase_modulation_level_leader,
        ToggleParam& tempo_sync,
        ToggleParam& center
) noexcept
    : SignalProducer(1, NUMBER_OF_CHILDREN),
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
    phase(
        modulator,
        phase_modulation_level_leader,
        "MP",
        0.0,
        1.0,
        0.0
    ),
    detune(
        "",
        Constants::DETUNE_MIN,
        Constants::DETUNE_MAX,
        Constants::DETUNE_DEFAULT
    ),
    fine_detune(
        "",
        Constants::FINE_DETUNE_MIN,
        Constants::FINE_DETUNE_MAX,
        Constants::FINE_DETUNE_DEFAULT
    ),
    harmonic_0("", -1.0, 1.0, 0.0),
    harmonic_1("", -1.0, 1.0, 0.0),
    harmonic_2("", -1.0, 1.0, 0.0),
    harmonic_3("", -1.0, 1.0, 0.0),
    harmonic_4("", -1.0, 1.0, 0.0),
    harmonic_5("", -1.0, 1.0, 0.0),
    harmonic_6("", -1.0, 1.0, 0.0),
    harmonic_7("", -1.0, 1.0, 0.0),
    harmonic_8("", -1.0, 1.0, 0.0),
    harmonic_9("", -1.0, 1.0, 0.0),
    tempo_sync(tempo_sync),
    center(center)
{
    initialize_instance();
}


template<class ModulatorSignalProducerClass, bool is_lfo>
void Oscillator<ModulatorSignalProducerClass, is_lfo>::initialize_instance() noexcept
{
    Number const custom_waveform_coefficients[CUSTOM_WAVEFORM_HARMONICS] = {
        0.0, 0.0, 0.0, 0.0, 0.0,
        0.0, 0.0, 0.0, 0.0, 0.0,
    };
    computed_frequency_buffer = NULL;
    computed_amplitude_buffer = NULL;
    phase_buffer = NULL;
    start_time_offset = 0.0;
    frequency_scale = 1.0;
    is_on_ = false;
    is_starting = false;

    register_child(waveform);
    register_child(modulated_amplitude);
    register_child(amplitude);
    register_child(frequency);
    register_child(phase);
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

    wavetables[SINE] = StandardWaveforms::sine();
    wavetables[SAWTOOTH] = StandardWaveforms::sawtooth();
    wavetables[SOFT_SAWTOOTH] = StandardWaveforms::soft_sawtooth();
    wavetables[INVERSE_SAWTOOTH] = StandardWaveforms::inverse_sawtooth();
    wavetables[SOFT_INVERSE_SAWTOOTH] = StandardWaveforms::soft_inverse_sawtooth();
    wavetables[TRIANGLE] = StandardWaveforms::triangle();
    wavetables[SOFT_TRIANGLE] = StandardWaveforms::soft_triangle();
    wavetables[SQUARE] = StandardWaveforms::square();
    wavetables[SOFT_SQUARE] = StandardWaveforms::soft_square();
    wavetables[CUSTOM] = custom_waveform;

    allocate_buffers(block_size);
}


template<class ModulatorSignalProducerClass, bool is_lfo>
Oscillator<ModulatorSignalProducerClass, is_lfo>::Oscillator(
        WaveformParam& waveform,
        FloatParam& amplitude_leader,
        FloatParam& frequency_leader,
        FloatParam& phase_leader,
        ToggleParam& tempo_sync,
        ToggleParam& center
) noexcept
    : SignalProducer(1, NUMBER_OF_CHILDREN),
    waveform(waveform),
    modulated_amplitude(NULL, dummy_param, "X", 0.0, 1.0, 1.0),
    amplitude(amplitude_leader),
    frequency(frequency_leader),
    phase(phase_leader),
    detune(
        "",
        Constants::DETUNE_MIN,
        Constants::DETUNE_MAX,
        Constants::DETUNE_DEFAULT
    ),
    fine_detune(
        "",
        Constants::FINE_DETUNE_MIN,
        Constants::FINE_DETUNE_MAX,
        Constants::FINE_DETUNE_DEFAULT
    ),
    harmonic_0("", -1.0, 1.0, 0.0),
    harmonic_1("", -1.0, 1.0, 0.0),
    harmonic_2("", -1.0, 1.0, 0.0),
    harmonic_3("", -1.0, 1.0, 0.0),
    harmonic_4("", -1.0, 1.0, 0.0),
    harmonic_5("", -1.0, 1.0, 0.0),
    harmonic_6("", -1.0, 1.0, 0.0),
    harmonic_7("", -1.0, 1.0, 0.0),
    harmonic_8("", -1.0, 1.0, 0.0),
    harmonic_9("", -1.0, 1.0, 0.0),
    tempo_sync(tempo_sync),
    center(center)
{
    initialize_instance();
}


template<class ModulatorSignalProducerClass, bool is_lfo>
Oscillator<ModulatorSignalProducerClass, is_lfo>::Oscillator(
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
        FloatParam& frequency_modulation_level_leader,
        FloatParam& phase_modulation_level_leader
) noexcept
    : SignalProducer(1, NUMBER_OF_CHILDREN),
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
    phase(
        modulator,
        phase_modulation_level_leader,
        "MP2",
        0.0,
        1.0,
        0.0
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
    harmonic_9(harmonic_9_leader),
    tempo_sync(dummy_toggle),
    center(dummy_toggle)
{
    initialize_instance();
}


template<class ModulatorSignalProducerClass, bool is_lfo>
Oscillator<ModulatorSignalProducerClass, is_lfo>::~Oscillator()
{
    delete custom_waveform;
    custom_waveform = NULL;
    wavetables[CUSTOM] = NULL;
    free_buffers();
}


template<class ModulatorSignalProducerClass, bool is_lfo>
void Oscillator<ModulatorSignalProducerClass, is_lfo>::allocate_buffers(
        Integer const size
) noexcept {
    computed_frequency_buffer = new Frequency[size];
    computed_amplitude_buffer = new Sample[size];
    phase_buffer = new Sample[size];
}


template<class ModulatorSignalProducerClass, bool is_lfo>
void Oscillator<ModulatorSignalProducerClass, is_lfo>::free_buffers() noexcept
{
    if (computed_frequency_buffer != NULL) {
        delete[] computed_frequency_buffer;
        delete[] computed_amplitude_buffer;
        delete[] phase_buffer;

        computed_frequency_buffer = NULL;
        computed_amplitude_buffer = NULL;
        phase_buffer = NULL;
    }
}


template<class ModulatorSignalProducerClass, bool is_lfo>
void Oscillator<ModulatorSignalProducerClass, is_lfo>::set_block_size(
        Integer const new_block_size
) noexcept {
    if (new_block_size != get_block_size()) {
        free_buffers();
        allocate_buffers(new_block_size);
    }

    SignalProducer::set_block_size(new_block_size);
}


template<class ModulatorSignalProducerClass, bool is_lfo>
void Oscillator<ModulatorSignalProducerClass, is_lfo>::reset() noexcept
{
    SignalProducer::reset();

    is_on_ = false;
    is_starting = false;
    start_time_offset = 0.0;
}


template<class ModulatorSignalProducerClass, bool is_lfo>
void Oscillator<ModulatorSignalProducerClass, is_lfo>::start(
        Seconds const time_offset
) noexcept {
    schedule(EVT_START, time_offset, 0, 0.0, 0.0);
}


template<class ModulatorSignalProducerClass, bool is_lfo>
void Oscillator<ModulatorSignalProducerClass, is_lfo>::stop(
        Seconds const time_offset
) noexcept {
    schedule(EVT_STOP, time_offset, 0, 0.0, 0.0);
}


template<class ModulatorSignalProducerClass, bool is_lfo>
bool Oscillator<ModulatorSignalProducerClass, is_lfo>::is_on() const noexcept
{
    return is_on_;
}


template<class ModulatorSignalProducerClass, bool is_lfo>
void Oscillator<ModulatorSignalProducerClass, is_lfo>::skip_round(
        Integer const round,
        Integer const sample_count
) noexcept {
    if (cached_round == round) {
        return;
    }

    cached_round = round;
    cached_buffer = buffer;

    modulated_amplitude.skip_round(round, sample_count);
    amplitude.skip_round(round, sample_count);
    frequency.skip_round(round, sample_count);
    phase.skip_round(round, sample_count);
    detune.skip_round(round, sample_count);
    fine_detune.skip_round(round, sample_count);

    harmonic_0.skip_round(round, sample_count);
    harmonic_1.skip_round(round, sample_count);
    harmonic_2.skip_round(round, sample_count);
    harmonic_3.skip_round(round, sample_count);
    harmonic_4.skip_round(round, sample_count);
    harmonic_5.skip_round(round, sample_count);
    harmonic_6.skip_round(round, sample_count);
    harmonic_7.skip_round(round, sample_count);
    harmonic_8.skip_round(round, sample_count);
    harmonic_9.skip_round(round, sample_count);

    for (Integer i = 0; i != sample_count; ++i) {
        buffer[0][i] = 0.0;
    }

    if (UNLIKELY(is_starting)) {
        initialize_first_round(frequency.get_value());
    }
}


template<class ModulatorSignalProducerClass, bool is_lfo>
void Oscillator<ModulatorSignalProducerClass, is_lfo>::initialize_first_round(
        Frequency const frequency
) noexcept {
    is_starting = false;
    Wavetable::reset_state(
        wavetable_state,
        sampling_period,
        nyquist_frequency,
        frequency,
        start_time_offset
    );
}


template<class ModulatorSignalProducerClass, bool is_lfo>
Sample const* const* Oscillator<ModulatorSignalProducerClass, is_lfo>::initialize_rendering(
        Integer const round,
        Integer const sample_count
) noexcept {
    apply_toggle_params<is_lfo>(bpm);

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
                *custom_waveform_params[i], round, sample_count
            );
        }

        if (has_changed) {
            custom_waveform->update_coefficients(custom_waveform_coefficients);
        }
    }

    wavetable = wavetables[waveform];
    compute_amplitude_buffer(round, sample_count);
    compute_frequency_buffer(round, sample_count);
    compute_phase_buffer(round, sample_count);

    return NULL;
}


template<class ModulatorSignalProducerClass, bool is_lfo>
template<bool is_lfo_>
void Oscillator<ModulatorSignalProducerClass, is_lfo>::apply_toggle_params(
        typename std::enable_if<is_lfo_, Number const>::type bpm
) noexcept {
    frequency_scale = (
        tempo_sync.get_value() == ToggleParam::ON
            ? bpm * TEMPO_SYNC_FREQUENCY_SCALE
            : 1.0
    );

    sample_offset_scale = center.get_value() == ToggleParam::ON ? 0.0 : 1.0;
}


template<class ModulatorSignalProducerClass, bool is_lfo>
template<bool is_lfo_>
void Oscillator<ModulatorSignalProducerClass, is_lfo>::apply_toggle_params(
        typename std::enable_if<!is_lfo_, Number const>::type bpm
) noexcept {
}


template<class ModulatorSignalProducerClass, bool is_lfo>
void Oscillator<ModulatorSignalProducerClass, is_lfo>::compute_amplitude_buffer(
        Integer const round,
        Integer const sample_count
) noexcept {
    Sample const* modulated_amplitude_buffer = (
        FloatParam::produce_if_not_constant<ModulatedFloatParam>(
            modulated_amplitude, round, sample_count
        )
    );
    Sample const* amplitude_buffer = (
        FloatParam::produce_if_not_constant(amplitude, round, sample_count)
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


template<class ModulatorSignalProducerClass, bool is_lfo>
void Oscillator<ModulatorSignalProducerClass, is_lfo>::compute_frequency_buffer(
        Integer const round,
        Integer const sample_count
) noexcept {
    constexpr Byte NONE = 0;
    constexpr Byte FREQUENCY = 1;
    constexpr Byte DETUNE = 2;
    constexpr Byte FINE_DETUNE = 4;
    constexpr Byte ALL = FREQUENCY | DETUNE | FINE_DETUNE;

    Sample const* const frequency_buffer = (
        FloatParam::produce_if_not_constant<ModulatedFloatParam>(
            frequency, round, sample_count
        )
    );
    Sample const* const detune_buffer = (
        FloatParam::produce_if_not_constant(detune, round, sample_count)
    );
    Sample const* const fine_detune_buffer = (
        FloatParam::produce_if_not_constant(fine_detune, round, sample_count)
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


template<class ModulatorSignalProducerClass, bool is_lfo>
Frequency Oscillator<ModulatorSignalProducerClass, is_lfo>::compute_frequency(
        Number const frequency,
        Number const detune,
        Number const fine_detune
) const noexcept {
    return Math::detune((Frequency)frequency, detune + fine_detune);
}


template<class ModulatorSignalProducerClass, bool is_lfo>
void Oscillator<ModulatorSignalProducerClass, is_lfo>::compute_phase_buffer(
        Integer const round,
        Integer const sample_count
) noexcept {
    Sample const* const phase_buffer = FloatParam::produce_if_not_constant<ModulatedFloatParam>(
        phase, round, sample_count
    );
    phase_is_constant = phase_buffer == NULL;

    if (phase_is_constant) {
        phase_value = Wavetable::scale_phase_offset(phase.get_value());
    } else {
        for (Integer i = 0; i != sample_count; ++i) {
            this->phase_buffer[i] = Wavetable::scale_phase_offset(phase_buffer[i]);
        }
    }
}


template<class ModulatorSignalProducerClass, bool is_lfo>
void Oscillator<ModulatorSignalProducerClass, is_lfo>::render(
        Integer const round,
        Integer const first_sample_index,
        Integer const last_sample_index,
        Sample** buffer
) noexcept {
    if (!is_on_) {
        for (Integer i = first_sample_index; i != last_sample_index; ++i) {
            buffer[0][i] = 0.0;
        }

        return;
    }

    if (computed_frequency_is_constant) {
        render_with_constant_frequency(
            round, first_sample_index, last_sample_index, buffer
        );
    } else {
        render_with_changing_frequency(
            round, first_sample_index, last_sample_index, buffer
        );
    }
}


template<class ModulatorSignalProducerClass, bool is_lfo>
void Oscillator<ModulatorSignalProducerClass, is_lfo>::render_with_constant_frequency(
        Integer const round,
        Integer const first_sample_index,
        Integer const last_sample_index,
        Sample** buffer
) noexcept {
    if (UNLIKELY(is_starting)) {
        initialize_first_round(computed_frequency_value);
    }

    if (computed_amplitude_is_constant) {
        if (phase_is_constant) {
            Sample const phase = phase_value;

            for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                buffer[0][i] = render_sample<is_lfo>(
                    computed_amplitude_value, computed_frequency_value, phase
                );
            }
        } else {
            for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                buffer[0][i] = render_sample<is_lfo>(
                    computed_amplitude_value, computed_frequency_value, phase_buffer[i]
                );
            }
        }
    } else {
        if (phase_is_constant) {
            for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                buffer[0][i] = render_sample<is_lfo>(
                    computed_amplitude_buffer[i], computed_frequency_value, phase_value
                );
            }
        } else {
            for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                buffer[0][i] = render_sample<is_lfo>(
                    computed_amplitude_buffer[i], computed_frequency_value, phase_buffer[i]
                );
            }
        }
    }
}


template<class ModulatorSignalProducerClass, bool is_lfo>
void Oscillator<ModulatorSignalProducerClass, is_lfo>::render_with_changing_frequency(
        Integer const round,
        Integer const first_sample_index,
        Integer const last_sample_index,
        Sample** buffer
) noexcept {
    if (UNLIKELY(is_starting)) {
        initialize_first_round(computed_frequency_buffer[first_sample_index]);
    }

    Frequency const* const computed_frequency_buffer = (
        (Frequency const*)this->computed_frequency_buffer
    );
    Sample const* const phase_buffer = this->phase_buffer;

    if (computed_amplitude_is_constant) {
        Sample const amplitude_value = computed_amplitude_value;

        if (phase_is_constant) {
            Sample const phase = phase_value;

            for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                buffer[0][i] = render_sample<is_lfo>(
                    amplitude_value, computed_frequency_buffer[i], phase
                );
            }
        } else {
            for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                buffer[0][i] = render_sample<is_lfo>(
                    amplitude_value, computed_frequency_buffer[i], phase_buffer[i]
                );
            }
        }
    } else {
        if (phase_is_constant) {
            for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                buffer[0][i] = render_sample<is_lfo>(
                    computed_amplitude_buffer[i], computed_frequency_buffer[i], phase_value
                );
            }
        } else {
            for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                buffer[0][i] = render_sample<is_lfo>(
                    computed_amplitude_buffer[i], computed_frequency_buffer[i], phase_buffer[i]
                );
            }
        }
    }
}


template<class ModulatorSignalProducerClass, bool is_lfo>
template<bool is_lfo_>
Sample Oscillator<ModulatorSignalProducerClass, is_lfo>::render_sample(
        typename std::enable_if<is_lfo_, Sample const>::type amplitude,
        typename std::enable_if<is_lfo_, Sample const>::type frequency,
        typename std::enable_if<is_lfo_, Sample const>::type phase
) noexcept {
    /*
    We could set a bool flag in apply_toggle_params() to indicate if the offset
    has to be added, so no multiplication would be necessary, but in practice,
    there doesn't seem to be a significant performance difference between the
    two approaches.
    */

    return amplitude * sample_offset_scale + amplitude * wavetable->lookup(
        wavetable_state, frequency * frequency_scale, phase
    );
}


template<class ModulatorSignalProducerClass, bool is_lfo>
template<bool is_lfo_>
Sample Oscillator<ModulatorSignalProducerClass, is_lfo>::render_sample(
        typename std::enable_if<!is_lfo_, Sample const>::type amplitude,
        typename std::enable_if<!is_lfo_, Sample const>::type frequency,
        typename std::enable_if<!is_lfo_, Sample const>::type phase
) noexcept {
    return amplitude * wavetable->lookup(wavetable_state, frequency, phase);
}


template<class ModulatorSignalProducerClass, bool is_lfo>
void Oscillator<ModulatorSignalProducerClass, is_lfo>::handle_event(
        Event const& event
) noexcept {
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


template<class ModulatorSignalProducerClass, bool is_lfo>
void Oscillator<ModulatorSignalProducerClass, is_lfo>::handle_start_event(
        Event const& event
) noexcept {
    if (is_on_) {
        return;
    }

    is_on_ = true;
    is_starting = true;
    start_time_offset = current_time - event.time_offset;
}


template<class ModulatorSignalProducerClass, bool is_lfo>
void Oscillator<ModulatorSignalProducerClass, is_lfo>::handle_stop_event(
        Event const& event
) noexcept {
    is_on_ = false;
}

}

#endif
