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

#ifndef JS80P__DSP__OSCILLATOR_CPP
#define JS80P__DSP__OSCILLATOR_CPP

#include <cmath>

#include "dsp/oscillator.hpp"

#include "dsp/math.hpp"


namespace JS80P
{

template<class ModulatorSignalProducerClass, bool is_lfo>
FloatParamB Oscillator<ModulatorSignalProducerClass, is_lfo>::dummy_param("", 0.0, 1.0, 0.0);

template<class ModulatorSignalProducerClass, bool is_lfo>
ToggleParam Oscillator<ModulatorSignalProducerClass, is_lfo>::dummy_toggle("", ToggleParam::OFF);


template<class ModulatorSignalProducerClass, bool is_lfo>
Oscillator<ModulatorSignalProducerClass, is_lfo>::WaveformParam::WaveformParam(
        std::string const& name,
        Byte const max_value
) noexcept : ByteParam(name, SINE, max_value, SINE)
{
}


template<class ModulatorSignalProducerClass, bool is_lfo>
Oscillator<ModulatorSignalProducerClass, is_lfo>::Oscillator(
        WaveformParam& waveform
) noexcept
    : SignalProducer(1, NUMBER_OF_CHILDREN, NUMBER_OF_EVENTS),
    waveform(waveform),
    modulated_amplitude(
        "MA",
        0.0,
        1.0,
        1.0
    ),
    amplitude("A", 0.0, 1.0, 1.0),
    subharmonic_amplitude("SA", 0.0, 1.0, 0.0),
    frequency(
        "MF",
        FREQUENCY_MIN,
        FREQUENCY_MAX,
        FREQUENCY_DEFAULT
    ),
    phase(
        "MP",
        0.0,
        1.0,
        0.0
    ),
    detune(
        "D",
        Constants::DETUNE_MIN,
        Constants::DETUNE_MAX,
        Constants::DETUNE_DEFAULT
    ),
    fine_detune(
        "FD",
        Constants::FINE_DETUNE_MIN,
        Constants::FINE_DETUNE_MAX,
        Constants::FINE_DETUNE_DEFAULT
    ),
    fine_detune_x4(dummy_toggle),
    harmonic_0(dummy_param),
    harmonic_1(dummy_param),
    harmonic_2(dummy_param),
    harmonic_3(dummy_param),
    harmonic_4(dummy_param),
    harmonic_5(dummy_param),
    harmonic_6(dummy_param),
    harmonic_7(dummy_param),
    harmonic_8(dummy_param),
    harmonic_9(dummy_param),
    tempo_sync(dummy_toggle),
    center(dummy_toggle)
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
    subharmonic_amplitude_is_constant = true;
    subharmonic_amplitude_buffer = NULL;
    subharmonic_amplitude_value = 0.0;

    register_child(waveform);
    register_child(modulated_amplitude);
    register_child(amplitude);
    register_child(subharmonic_amplitude);
    register_child(frequency);
    register_child(phase);
    register_child(detune);
    register_child(fine_detune);

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
        ModulatorSignalProducerClass& modulator,
        FloatParamS& amplitude_modulation_level_leader,
        FloatParamS& frequency_modulation_level_leader,
        FloatParamS& phase_modulation_level_leader
) noexcept
    : SignalProducer(1, NUMBER_OF_CHILDREN, NUMBER_OF_EVENTS),
    waveform(waveform),
    modulated_amplitude(
        modulator,
        amplitude_modulation_level_leader,
        "MA",
        0.0,
        1.0,
        1.0
    ),
    amplitude("A", 0.0, 1.0, 1.0),
    subharmonic_amplitude("SA", 0.0, 1.0, 0.0),
    frequency(
        modulator,
        frequency_modulation_level_leader,
        "MF",
        FREQUENCY_MIN,
        FREQUENCY_MAX,
        FREQUENCY_DEFAULT
    ),
    phase(modulator, phase_modulation_level_leader, "MP", 0.0, 1.0, 0.0),
    detune(
        "D",
        Constants::DETUNE_MIN,
        Constants::DETUNE_MAX,
        Constants::DETUNE_DEFAULT
    ),
    fine_detune(
        "FD",
        Constants::FINE_DETUNE_MIN,
        Constants::FINE_DETUNE_MAX,
        Constants::FINE_DETUNE_DEFAULT
    ),
    fine_detune_x4(dummy_toggle),
    harmonic_0(dummy_param),
    harmonic_1(dummy_param),
    harmonic_2(dummy_param),
    harmonic_3(dummy_param),
    harmonic_4(dummy_param),
    harmonic_5(dummy_param),
    harmonic_6(dummy_param),
    harmonic_7(dummy_param),
    harmonic_8(dummy_param),
    harmonic_9(dummy_param),
    tempo_sync(dummy_toggle),
    center(dummy_toggle)
{
    initialize_instance();
}


template<class ModulatorSignalProducerClass, bool is_lfo>
Oscillator<ModulatorSignalProducerClass, is_lfo>::Oscillator(
        WaveformParam& waveform,
        FloatParamS& amplitude_leader,
        FloatParamS& frequency_leader,
        FloatParamS& phase_leader,
        ToggleParam& tempo_sync,
        ToggleParam& center
) noexcept
    : SignalProducer(1, NUMBER_OF_CHILDREN, NUMBER_OF_EVENTS),
    waveform(waveform),
    modulated_amplitude("MA", 0.0, 1.0, 1.0),
    amplitude(amplitude_leader),
    subharmonic_amplitude("SA", 0.0, 1.0, 0.0),
    frequency(frequency_leader),
    phase(phase_leader),
    detune(
        "D",
        Constants::DETUNE_MIN,
        Constants::DETUNE_MAX,
        Constants::DETUNE_DEFAULT
    ),
    fine_detune(
        "FD",
        Constants::FINE_DETUNE_MIN,
        Constants::FINE_DETUNE_MAX,
        Constants::FINE_DETUNE_DEFAULT
    ),
    fine_detune_x4(dummy_toggle),
    harmonic_0(dummy_param),
    harmonic_1(dummy_param),
    harmonic_2(dummy_param),
    harmonic_3(dummy_param),
    harmonic_4(dummy_param),
    harmonic_5(dummy_param),
    harmonic_6(dummy_param),
    harmonic_7(dummy_param),
    harmonic_8(dummy_param),
    harmonic_9(dummy_param),
    tempo_sync(tempo_sync),
    center(center)
{
    initialize_instance();
}


template<class ModulatorSignalProducerClass, bool is_lfo>
Oscillator<ModulatorSignalProducerClass, is_lfo>::Oscillator(
        WaveformParam& waveform,
        FloatParamS& amplitude_leader,
        FloatParamS& subharmonic_leader,
        FloatParamS& detune_leader,
        FloatParamS& fine_detune_leader,
        ToggleParam& fine_detune_x4_leader,
        FloatParamB& harmonic_0_leader,
        FloatParamB& harmonic_1_leader,
        FloatParamB& harmonic_2_leader,
        FloatParamB& harmonic_3_leader,
        FloatParamB& harmonic_4_leader,
        FloatParamB& harmonic_5_leader,
        FloatParamB& harmonic_6_leader,
        FloatParamB& harmonic_7_leader,
        FloatParamB& harmonic_8_leader,
        FloatParamB& harmonic_9_leader,
        Byte const& voice_status
) noexcept
    : SignalProducer(1, NUMBER_OF_CHILDREN, NUMBER_OF_EVENTS),
    waveform(waveform),
    modulated_amplitude("MA2", 0.0, 1.0, 1.0),
    amplitude(amplitude_leader, voice_status),
    subharmonic_amplitude(subharmonic_leader, voice_status),
    frequency("MF2", FREQUENCY_MIN, FREQUENCY_MAX, FREQUENCY_DEFAULT),
    phase("MP2", 0.0, 1.0, 0.0),
    detune(detune_leader, voice_status),
    fine_detune(fine_detune_leader, voice_status),
    fine_detune_x4(fine_detune_x4_leader),
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
Oscillator<ModulatorSignalProducerClass, is_lfo>::Oscillator(
        WaveformParam& waveform,
        FloatParamS& amplitude_leader,
        FloatParamS& detune_leader,
        FloatParamS& fine_detune_leader,
        ToggleParam& fine_detune_x4_leader,
        FloatParamB& harmonic_0_leader,
        FloatParamB& harmonic_1_leader,
        FloatParamB& harmonic_2_leader,
        FloatParamB& harmonic_3_leader,
        FloatParamB& harmonic_4_leader,
        FloatParamB& harmonic_5_leader,
        FloatParamB& harmonic_6_leader,
        FloatParamB& harmonic_7_leader,
        FloatParamB& harmonic_8_leader,
        FloatParamB& harmonic_9_leader,
        Byte const& voice_status,
        ModulatorSignalProducerClass& modulator,
        FloatParamS& amplitude_modulation_level_leader,
        FloatParamS& frequency_modulation_level_leader,
        FloatParamS& phase_modulation_level_leader
) noexcept
    : SignalProducer(1, NUMBER_OF_CHILDREN, NUMBER_OF_EVENTS),
    waveform(waveform),
    modulated_amplitude(
        modulator,
        amplitude_modulation_level_leader,
        voice_status,
        "MA2",
        0.0,
        1.0,
        1.0
    ),
    amplitude(amplitude_leader, voice_status),
    subharmonic_amplitude("", 0.0, 1.0, 0.0),
    frequency(
        modulator,
        frequency_modulation_level_leader,
        voice_status,
        "MF2",
        FREQUENCY_MIN,
        FREQUENCY_MAX,
        FREQUENCY_DEFAULT
    ),
    phase(
        modulator,
        phase_modulation_level_leader,
        voice_status,
        "MP2",
        0.0,
        1.0,
        0.0
    ),
    detune(detune_leader),
    fine_detune(fine_detune_leader, voice_status),
    fine_detune_x4(fine_detune_x4_leader),
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
    subharmonic_amplitude.skip_round(round, sample_count);
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

    if (JS80P_UNLIKELY(is_starting)) {
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
void Oscillator<ModulatorSignalProducerClass, is_lfo>::produce_for_lfo_with_envelope(
        WavetableState& wavetable_state,
        Integer const round,
        Integer const sample_count,
        Integer const first_sample_index,
        Integer const last_sample_index,
        Sample* const buffer,
        Sample const* const amplitude_buffer,
        Sample const* const frequency_buffer,
        Sample const* const phase_buffer
) noexcept {
    wavetable = wavetables[waveform.get_value()];

    compute_amplitude_buffer(
        amplitude_buffer, round, sample_count, first_sample_index, last_sample_index
    );
    compute_frequency_buffer(
        frequency_buffer, round, sample_count, first_sample_index, last_sample_index
    );
    compute_phase_buffer(
        phase_buffer, round, sample_count, first_sample_index, last_sample_index
    );

    apply_toggle_params(bpm);

    bool const was_on = this->is_on_;
    bool const was_starting = this->is_starting;

    this->is_on_ = true;

    render<false>(
        wavetable_state, round, first_sample_index, last_sample_index, buffer
    );

    this->is_on_ = was_on;
    this->is_starting = was_starting;
}


template<class ModulatorSignalProducerClass, bool is_lfo>
Sample const* const* Oscillator<ModulatorSignalProducerClass, is_lfo>::initialize_rendering(
        Integer const round,
        Integer const sample_count
) noexcept {
    Byte const waveform = this->waveform.get_value();

    if constexpr (is_lfo) {
        apply_toggle_params(bpm);
    } else if (waveform == CUSTOM) {
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

            FloatParamB::produce_if_not_constant(
                *custom_waveform_params[i], round, sample_count
            );
        }

        if (has_changed) {
            custom_waveform->update_coefficients(custom_waveform_coefficients);
        }
    }

    wavetable = wavetables[waveform];

    Sample const* const amplitude_buffer = (
        FloatParamS::produce_if_not_constant(amplitude, round, sample_count)
    );
    Sample const* const frequency_buffer = (
        FloatParamS::produce_if_not_constant<ModulatedFloatParam>(
            frequency, round, sample_count
        )
    );
    Sample const* const phase_buffer = (
        FloatParamS::produce_if_not_constant<ModulatedFloatParam>(
            phase, round, sample_count
        )
    );

    compute_amplitude_buffer(amplitude_buffer, round, sample_count, 0, sample_count);
    compute_frequency_buffer(frequency_buffer, round, sample_count, 0, sample_count);
    compute_phase_buffer(phase_buffer, round, sample_count, 0, sample_count);

    return NULL;
}


template<class ModulatorSignalProducerClass, bool is_lfo>
void Oscillator<ModulatorSignalProducerClass, is_lfo>::apply_toggle_params(
        Number const bpm
) noexcept {
    frequency_scale = (
        tempo_sync.get_value() == ToggleParam::ON
            ? bpm * TEMPO_SYNC_FREQUENCY_SCALE
            : 1.0
    );

    sample_offset_scale = center.get_value() == ToggleParam::ON ? 0.0 : 1.0;
}


template<class ModulatorSignalProducerClass, bool is_lfo>
void Oscillator<ModulatorSignalProducerClass, is_lfo>::compute_amplitude_buffer(
        Sample const* const amplitude_buffer,
        Integer const round,
        Integer const sample_count,
        Integer const first_sample_index,
        Integer const last_sample_index
) noexcept {
    if constexpr (HAS_SUBHARMONIC) {
        subharmonic_amplitude_buffer = FloatParamS::produce_if_not_constant(
            subharmonic_amplitude, round, sample_count
        );

        subharmonic_amplitude_is_constant = subharmonic_amplitude_buffer == NULL;

        if (subharmonic_amplitude_is_constant) {
            subharmonic_amplitude_value = subharmonic_amplitude.get_value();
        }
    }

    Sample const* const modulated_amplitude_buffer = (
        FloatParamS::produce_if_not_constant<ModulatedFloatParam>(
            modulated_amplitude, round, sample_count
        )
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

            for (Integer i = first_sample_index; i != last_sample_index; ++i) {
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

            for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                computed_amplitude_buffer[i] = (
                    amplitude_buffer[i] * modulated_amplitude_value
                );
            }
        } else {
            for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                computed_amplitude_buffer[i] = (
                    amplitude_buffer[i] * modulated_amplitude_buffer[i]
                );
            }
        }
    }
}


template<class ModulatorSignalProducerClass, bool is_lfo>
void Oscillator<ModulatorSignalProducerClass, is_lfo>::compute_frequency_buffer(
        Sample const* const frequency_buffer,
        Integer const round,
        Integer const sample_count,
        Integer const first_sample_index,
        Integer const last_sample_index
) noexcept {
    Sample const* const detune_buffer = (
        FloatParamS::produce_if_not_constant(detune, round, sample_count)
    );
    Sample const* const fine_detune_buffer = (
        FloatParamS::produce_if_not_constant(fine_detune, round, sample_count)
    );

    if constexpr (is_lfo) {
        if (frequency_buffer == NULL) {
            Number const frequency_value = frequency.get_value();
            Number const detune_value = detune.get_value();
            Number const fine_detune_value = fine_detune.get_value();

            computed_frequency_is_constant = true;
            computed_frequency_value = compute_frequency(
                frequency_value, detune_value, fine_detune_value
            );
        } else {
            Number const detune_value = detune.get_value();
            Number const fine_detune_value = fine_detune.get_value();

            computed_frequency_is_constant = false;

            for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                computed_frequency_buffer[i] = compute_frequency(
                    (Number)frequency_buffer[i],
                    detune_value,
                    fine_detune_value
                );
            }
        }
    } else {
        constexpr Byte NONE = 0;
        constexpr Byte FREQUENCY = 1;
        constexpr Byte DETUNE = 2;
        constexpr Byte FINE_DETUNE = 4;
        constexpr Byte ALL = FREQUENCY | DETUNE | FINE_DETUNE;

        Number const fine_detune_scale = (
            fine_detune_x4.get_value() == ToggleParam::ON ? 4.0 : 1.0
        );

        Byte const const_param_flags = (
            (frequency_buffer == NULL ? FREQUENCY : NONE)
            | (detune_buffer == NULL ? DETUNE : NONE)
            | (fine_detune_buffer == NULL ? FINE_DETUNE : NONE)
        );

        switch (const_param_flags) {
            case NONE: {
                computed_frequency_is_constant = false;

                for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                    computed_frequency_buffer[i] = compute_frequency(
                        (Number)frequency_buffer[i],
                        (Number)detune_buffer[i],
                        fine_detune_scale * (Number)fine_detune_buffer[i]
                    );
                }

                break;
            }

            case FREQUENCY: {
                Number const frequency_value = frequency.get_value();

                computed_frequency_is_constant = false;

                for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                    computed_frequency_buffer[i] = compute_frequency(
                        frequency_value,
                        (Number)detune_buffer[i],
                        fine_detune_scale * (Number)fine_detune_buffer[i]
                    );
                }

                break;
            }

            case DETUNE: {
                Number const detune_value = detune.get_value();

                computed_frequency_is_constant = false;

                for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                    computed_frequency_buffer[i] = compute_frequency(
                        (Number)frequency_buffer[i],
                        detune_value,
                        fine_detune_scale * (Number)fine_detune_buffer[i]
                    );
                }

                break;
            }

            case FREQUENCY | DETUNE: {
                Number const frequency_value = frequency.get_value();
                Number const detune_value = detune.get_value();

                computed_frequency_is_constant = false;

                for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                    computed_frequency_buffer[i] = compute_frequency(
                        frequency_value,
                        detune_value,
                        fine_detune_scale * (Number)fine_detune_buffer[i]
                    );
                }

                break;
            }

            case FINE_DETUNE: {
                Number const fine_detune_value = (
                    fine_detune_scale * fine_detune.get_value()
                );

                computed_frequency_is_constant = false;

                for (Integer i = first_sample_index; i != last_sample_index; ++i) {
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
                Number const fine_detune_value = (
                    fine_detune_scale * fine_detune.get_value()
                );

                computed_frequency_is_constant = false;

                for (Integer i = first_sample_index; i != last_sample_index; ++i) {
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
                Number const fine_detune_value = (
                    fine_detune_scale * fine_detune.get_value()
                );

                computed_frequency_is_constant = false;

                for (Integer i = first_sample_index; i != last_sample_index; ++i) {
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
                Number const fine_detune_value = (
                    fine_detune_scale * fine_detune.get_value()
                );

                computed_frequency_is_constant = true;
                computed_frequency_value = compute_frequency(
                    frequency_value, detune_value, fine_detune_value
                );

                break;
            }
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
        Sample const* const phase_buffer,
        Integer const round,
        Integer const sample_count,
        Integer const first_sample_index,
        Integer const last_sample_index
) noexcept {
    phase_is_constant = phase_buffer == NULL;

    if (phase_is_constant) {
        phase_value = Wavetable::scale_phase_offset(phase.get_value());
    } else {
        for (Integer i = first_sample_index; i != last_sample_index; ++i) {
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
        render_silence(round, first_sample_index, last_sample_index, buffer);

        return;
    }

    if constexpr (HAS_SUBHARMONIC) {
        if (subharmonic_amplitude_is_constant && subharmonic_amplitude_value < 0.000001) {
            render<false>(
                wavetable_state, round, first_sample_index, last_sample_index, buffer[0]
            );
        } else {
            render<true>(
                wavetable_state, round, first_sample_index, last_sample_index, buffer[0]
            );
        }
    } else {
        render<false>(
            wavetable_state, round, first_sample_index, last_sample_index, buffer[0]
        );
    }
}


template<class ModulatorSignalProducerClass, bool is_lfo>
template<bool has_subharmonic>
void Oscillator<ModulatorSignalProducerClass, is_lfo>::render(
        WavetableState& wavetable_state,
        Integer const round,
        Integer const first_sample_index,
        Integer const last_sample_index,
        Sample* buffer
) noexcept {
    if (computed_frequency_is_constant) {
        Wavetable::Interpolation const interpolation = (
            wavetable->select_interpolation(
                frequency_scale * computed_frequency_value, nyquist_frequency
            )
        );

        if (JS80P_UNLIKELY(wavetable->has_single_partial())) {
            switch (interpolation) {
                case Wavetable::Interpolation::LINEAR_ONLY:
                    render_with_constant_frequency<Wavetable::Interpolation::LINEAR_ONLY, true, has_subharmonic>(
                        wavetable_state, round, first_sample_index, last_sample_index, buffer
                    );
                    break;

                case Wavetable::Interpolation::LAGRANGE_ONLY:
                    render_with_constant_frequency<Wavetable::Interpolation::LAGRANGE_ONLY, true, has_subharmonic>(
                        wavetable_state, round, first_sample_index, last_sample_index, buffer
                    );
                    break;

                default:
                    render_with_constant_frequency<Wavetable::Interpolation::DYNAMIC, true, has_subharmonic>(
                        wavetable_state, round, first_sample_index, last_sample_index, buffer
                    );
                    break;
            }
        } else {
            switch (interpolation) {
                case Wavetable::Interpolation::LINEAR_ONLY:
                    render_with_constant_frequency<Wavetable::Interpolation::LINEAR_ONLY, false, has_subharmonic>(
                        wavetable_state, round, first_sample_index, last_sample_index, buffer
                    );
                    break;

                case Wavetable::Interpolation::LAGRANGE_ONLY:
                    render_with_constant_frequency<Wavetable::Interpolation::LAGRANGE_ONLY, false, has_subharmonic>(
                        wavetable_state, round, first_sample_index, last_sample_index, buffer
                    );
                    break;

                default:
                    render_with_constant_frequency<Wavetable::Interpolation::DYNAMIC, false, has_subharmonic>(
                        wavetable_state, round, first_sample_index, last_sample_index, buffer
                    );
                    break;
            }
        }
    } else if (JS80P_UNLIKELY(wavetable->has_single_partial())) {
        render_with_changing_frequency<true, has_subharmonic>(
            wavetable_state, round, first_sample_index, last_sample_index, buffer
        );
    } else {
        render_with_changing_frequency<false, has_subharmonic>(
            wavetable_state, round, first_sample_index, last_sample_index, buffer
        );
    }
}


template<class ModulatorSignalProducerClass, bool is_lfo>
template<Wavetable::Interpolation interpolation, bool single_partial, bool has_subharmonic>
void Oscillator<ModulatorSignalProducerClass, is_lfo>::render_with_constant_frequency(
        WavetableState& wavetable_state,
        Integer const round,
        Integer const first_sample_index,
        Integer const last_sample_index,
        Sample* buffer
) noexcept {
    if (JS80P_UNLIKELY(is_starting)) {
        initialize_first_round(computed_frequency_value);
    }

    if constexpr (has_subharmonic) {
        if (computed_amplitude_is_constant) {
            if (phase_is_constant) {
                Sample const phase = phase_value;

                if (subharmonic_amplitude_is_constant) {
                    Sample const subharmonic_amplitude = subharmonic_amplitude_value;

                    for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                        buffer[i] = render_sample<single_partial, true, interpolation>(
                            wavetable_state,
                            computed_amplitude_value,
                            computed_frequency_value,
                            phase,
                            subharmonic_amplitude
                        );
                    }
                } else {
                    Sample const* const subharmonic_amplitude_buffer = this->subharmonic_amplitude_buffer;

                    for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                        buffer[i] = render_sample<single_partial, true, interpolation>(
                            wavetable_state,
                            computed_amplitude_value,
                            computed_frequency_value,
                            phase,
                            subharmonic_amplitude_buffer[i]
                        );
                    }
                }
            } else {
                if (subharmonic_amplitude_is_constant) {
                    Sample const subharmonic_amplitude = subharmonic_amplitude_value;

                    for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                        buffer[i] = render_sample<single_partial, true, interpolation>(
                            wavetable_state,
                            computed_amplitude_value,
                            computed_frequency_value,
                            phase_buffer[i],
                            subharmonic_amplitude
                        );
                    }
                } else {
                    Sample const* const subharmonic_amplitude_buffer = this->subharmonic_amplitude_buffer;

                    for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                        buffer[i] = render_sample<single_partial, true, interpolation>(
                            wavetable_state,
                            computed_amplitude_value,
                            computed_frequency_value,
                            phase_buffer[i],
                            subharmonic_amplitude_buffer[i]
                        );
                    }
                }
            }
        } else {
            if (phase_is_constant) {
                if (subharmonic_amplitude_is_constant) {
                    Sample const subharmonic_amplitude = subharmonic_amplitude_value;

                    for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                        buffer[i] = render_sample<single_partial, true, interpolation>(
                            wavetable_state,
                            computed_amplitude_buffer[i],
                            computed_frequency_value,
                            phase_value,
                            subharmonic_amplitude
                        );
                    }
                } else {
                    Sample const* const subharmonic_amplitude_buffer = this->subharmonic_amplitude_buffer;

                    for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                        buffer[i] = render_sample<single_partial, true, interpolation>(
                            wavetable_state,
                            computed_amplitude_buffer[i],
                            computed_frequency_value,
                            phase_value,
                            subharmonic_amplitude_buffer[i]
                        );
                    }
                }
            } else {
                if (subharmonic_amplitude_is_constant) {
                    Sample const subharmonic_amplitude = subharmonic_amplitude_value;

                    for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                        buffer[i] = render_sample<single_partial, true, interpolation>(
                            wavetable_state,
                            computed_amplitude_buffer[i],
                            computed_frequency_value,
                            phase_buffer[i],
                            subharmonic_amplitude
                        );
                    }
                } else {
                    Sample const* const subharmonic_amplitude_buffer = this->subharmonic_amplitude_buffer;

                    for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                        buffer[i] = render_sample<single_partial, true, interpolation>(
                            wavetable_state,
                            computed_amplitude_buffer[i],
                            computed_frequency_value,
                            phase_buffer[i],
                            subharmonic_amplitude_buffer[i]
                        );
                    }
                }
            }
        }
    } else {
        if (computed_amplitude_is_constant) {
            if (phase_is_constant) {
                Sample const phase = phase_value;

                for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                    buffer[i] = render_sample<single_partial, false, interpolation>(
                        wavetable_state,
                        computed_amplitude_value,
                        computed_frequency_value,
                        phase
                    );
                }
            } else {
                for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                    buffer[i] = render_sample<single_partial, false, interpolation>(
                        wavetable_state,
                        computed_amplitude_value,
                        computed_frequency_value,
                        phase_buffer[i]
                    );
                }
            }
        } else {
            if (phase_is_constant) {
                for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                    buffer[i] = render_sample<single_partial, false, interpolation>(
                        wavetable_state,
                        computed_amplitude_buffer[i],
                        computed_frequency_value,
                        phase_value
                    );
                }
            } else {
                for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                    buffer[i] = render_sample<single_partial, false, interpolation>(
                        wavetable_state,
                        computed_amplitude_buffer[i],
                        computed_frequency_value,
                        phase_buffer[i]
                    );
                }
            }
        }
    }
}


template<class ModulatorSignalProducerClass, bool is_lfo>
template<bool single_partial, bool has_subharmonic>
void Oscillator<ModulatorSignalProducerClass, is_lfo>::render_with_changing_frequency(
        WavetableState& wavetable_state,
        Integer const round,
        Integer const first_sample_index,
        Integer const last_sample_index,
        Sample* buffer
) noexcept {
    if (JS80P_UNLIKELY(is_starting)) {
        initialize_first_round(computed_frequency_buffer[first_sample_index]);
    }

    Frequency const* const computed_frequency_buffer = (
        (Frequency const*)this->computed_frequency_buffer
    );
    Sample const* const phase_buffer = this->phase_buffer;

    if constexpr (has_subharmonic) {
        if (computed_amplitude_is_constant) {
            Sample const amplitude_value = computed_amplitude_value;

            if (phase_is_constant) {
                Sample const phase = phase_value;

                if (subharmonic_amplitude_is_constant) {
                    Sample const subharmonic_amplitude = subharmonic_amplitude_value;

                    for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                        buffer[i] = render_sample<single_partial, true>(
                            wavetable_state,
                            amplitude_value,
                            computed_frequency_buffer[i],
                            phase,
                            subharmonic_amplitude
                        );
                    }
                } else {
                    Sample const* const subharmonic_amplitude_buffer = this->subharmonic_amplitude_buffer;

                    for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                        buffer[i] = render_sample<single_partial, true>(
                            wavetable_state,
                            amplitude_value,
                            computed_frequency_buffer[i],
                            phase,
                            subharmonic_amplitude_buffer[i]
                        );
                    }
                }
            } else {
                if (subharmonic_amplitude_is_constant) {
                    Sample const subharmonic_amplitude = subharmonic_amplitude_value;

                    for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                        buffer[i] = render_sample<single_partial, true>(
                            wavetable_state,
                            amplitude_value,
                            computed_frequency_buffer[i],
                            phase_buffer[i],
                            subharmonic_amplitude
                        );
                    }
                } else {
                    Sample const* const subharmonic_amplitude_buffer = this->subharmonic_amplitude_buffer;

                    for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                        buffer[i] = render_sample<single_partial, true>(
                            wavetable_state,
                            amplitude_value,
                            computed_frequency_buffer[i],
                            phase_buffer[i],
                            subharmonic_amplitude_buffer[i]
                        );
                    }
                }
            }
        } else {
            if (phase_is_constant) {
                if (subharmonic_amplitude_is_constant) {
                    Sample const subharmonic_amplitude = subharmonic_amplitude_value;

                    for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                        buffer[i] = render_sample<single_partial, true>(
                            wavetable_state,
                            computed_amplitude_buffer[i],
                            computed_frequency_buffer[i],
                            phase_value,
                            subharmonic_amplitude
                        );
                    }
                } else {
                    Sample const* const subharmonic_amplitude_buffer = this->subharmonic_amplitude_buffer;

                    for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                        buffer[i] = render_sample<single_partial, true>(
                            wavetable_state,
                            computed_amplitude_buffer[i],
                            computed_frequency_buffer[i],
                            phase_value,
                            subharmonic_amplitude_buffer[i]
                        );
                    }
                }
            } else {
                if (subharmonic_amplitude_is_constant) {
                    Sample const subharmonic_amplitude = subharmonic_amplitude_value;

                    for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                        buffer[i] = render_sample<single_partial, true>(
                            wavetable_state,
                            computed_amplitude_buffer[i],
                            computed_frequency_buffer[i],
                            phase_buffer[i],
                            subharmonic_amplitude
                        );
                    }
                } else {
                    Sample const* const subharmonic_amplitude_buffer = this->subharmonic_amplitude_buffer;

                    for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                        buffer[i] = render_sample<single_partial, true>(
                            wavetable_state,
                            computed_amplitude_buffer[i],
                            computed_frequency_buffer[i],
                            phase_buffer[i],
                            subharmonic_amplitude_buffer[i]
                        );
                    }
                }
            }
        }
    } else {
        if (computed_amplitude_is_constant) {
            Sample const amplitude_value = computed_amplitude_value;

            if (phase_is_constant) {
                Sample const phase = phase_value;

                for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                    buffer[i] = render_sample<single_partial, false>(
                        wavetable_state,
                        amplitude_value,
                        computed_frequency_buffer[i],
                        phase
                    );
                }
            } else {
                for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                    buffer[i] = render_sample<single_partial, false>(
                        wavetable_state,
                        amplitude_value,
                        computed_frequency_buffer[i],
                        phase_buffer[i]
                    );
                }
            }
        } else {
            if (phase_is_constant) {
                for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                    buffer[i] = render_sample<single_partial, false>(
                        wavetable_state,
                        computed_amplitude_buffer[i],
                        computed_frequency_buffer[i],
                        phase_value
                    );
                }
            } else {
                for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                    buffer[i] = render_sample<single_partial, false>(
                        wavetable_state,
                        computed_amplitude_buffer[i],
                        computed_frequency_buffer[i],
                        phase_buffer[i]
                    );
                }
            }
        }
    }
}


template<class ModulatorSignalProducerClass, bool is_lfo>
template<bool single_partial, bool has_subharmonic, Wavetable::Interpolation interpolation>
Sample Oscillator<ModulatorSignalProducerClass, is_lfo>::render_sample(
        WavetableState& wavetable_state,
        Sample const amplitude,
        Sample const frequency,
        Sample const phase,
        Sample const subharmonic_amplitude
) noexcept {
    Sample sample;
    Sample subharmonic_sample;

    if constexpr (is_lfo) {
        wavetable->lookup<interpolation, single_partial, false>(
            wavetable_state, frequency * frequency_scale, phase, sample, subharmonic_sample
        );

        return amplitude * (sample_offset_scale + sample);
    } else if constexpr (has_subharmonic) {
        wavetable->lookup<interpolation, single_partial, true>(
            wavetable_state, frequency, phase, sample, subharmonic_sample
        );

        return amplitude * sample + subharmonic_amplitude * subharmonic_sample;
    } else {
        wavetable->lookup<interpolation, single_partial, false>(
            wavetable_state, frequency, phase, sample, subharmonic_sample
        );

        return amplitude * sample;
    }
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
