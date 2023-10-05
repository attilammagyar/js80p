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

#ifndef JS80P__VOICE_CPP
#define JS80P__VOICE_CPP

#include "dsp/math.hpp"

#include "voice.hpp"


namespace JS80P
{

template<class ModulatorSignalProducerClass>
Voice<ModulatorSignalProducerClass>::Dummy::Dummy()
{
}


template<class ModulatorSignalProducerClass>
Voice<ModulatorSignalProducerClass>::Dummy::Dummy(
        std::string const a,
        Number const b,
        Number const c,
        Number const d
) {
}


template<class ModulatorSignalProducerClass>
Voice<ModulatorSignalProducerClass>::Params::Params(std::string const name) noexcept
    : waveform(name + "WAV"),
    amplitude(name + "AMP", 0.0, 1.0, 0.75),
    velocity_sensitivity(name + "VS", 0.0, 2.0, 1.0),
    folding(
        name + "FLD",
        Constants::FOLD_MIN,
        Constants::FOLD_MAX,
        Constants::FOLD_DEFAULT
    ),
    portamento_length(name + "PRT", 0.0, 3.0, 0.0),
    portamento_depth(name + "PRD", -2400.0, 2400.0, 0.0),
    detune(
        name + "DTN",
        Constants::DETUNE_MIN,
        Constants::DETUNE_MAX,
        Constants::DETUNE_DEFAULT,
        100.0
    ),
    fine_detune(
        name + "FIN",
        Constants::FINE_DETUNE_MIN,
        Constants::FINE_DETUNE_MAX,
        Constants::FINE_DETUNE_DEFAULT
    ),
    width(name + "WID", -1.0, 1.0, 0.0),
    panning(name + "PAN", -1.0, 1.0, 0.0),
    volume(name + "VOL", 0.0, 1.0, 0.33),

    harmonic_0(name + "C1", -1.0, 1.0, 0.0),
    harmonic_1(name + "C2", -1.0, 1.0, 0.0),
    harmonic_2(name + "C3", -1.0, 1.0, 0.0),
    harmonic_3(name + "C4", -1.0, 1.0, 0.0),
    harmonic_4(name + "C5", -1.0, 1.0, 0.0),
    harmonic_5(name + "C6", -1.0, 1.0, 0.0),
    harmonic_6(name + "C7", -1.0, 1.0, 0.0),
    harmonic_7(name + "C8", -1.0, 1.0, 0.0),
    harmonic_8(name + "C9", -1.0, 1.0, 0.0),
    harmonic_9(name + "C10", -1.0, 1.0, 0.0),

    filter_1_type(name + "F1TYP"),
    filter_1_log_scale(name + "F1LOG", ToggleParam::OFF),
    filter_1_frequency(
        name + "F1FRQ",
        Constants::BIQUAD_FILTER_FREQUENCY_MIN,
        Constants::BIQUAD_FILTER_FREQUENCY_MAX,
        Constants::BIQUAD_FILTER_FREQUENCY_DEFAULT,
        0.0,
        &filter_1_log_scale,
        Math::log_biquad_filter_freq_table(),
        Math::LOG_BIQUAD_FILTER_FREQ_TABLE_MAX_INDEX,
        Math::LOG_BIQUAD_FILTER_FREQ_SCALE
    ),
    filter_1_q(
        name + "F1Q",
        Constants::BIQUAD_FILTER_Q_MIN,
        Constants::BIQUAD_FILTER_Q_MAX,
        Constants::BIQUAD_FILTER_Q_DEFAULT
    ),
    filter_1_gain(
        name + "F1G",
        Constants::BIQUAD_FILTER_GAIN_MIN,
        Constants::BIQUAD_FILTER_GAIN_MAX,
        Constants::BIQUAD_FILTER_GAIN_DEFAULT
    ),

    filter_2_type(name + "F2TYP"),
    filter_2_log_scale(name + "F2LOG", ToggleParam::OFF),
    filter_2_frequency(
        name + "F2FRQ",
        Constants::BIQUAD_FILTER_FREQUENCY_MIN,
        Constants::BIQUAD_FILTER_FREQUENCY_MAX,
        Constants::BIQUAD_FILTER_FREQUENCY_DEFAULT,
        0.0,
        &filter_2_log_scale,
        Math::log_biquad_filter_freq_table(),
        Math::LOG_BIQUAD_FILTER_FREQ_TABLE_MAX_INDEX,
        Math::LOG_BIQUAD_FILTER_FREQ_SCALE
    ),
    filter_2_q(
        name + "F2Q",
        Constants::BIQUAD_FILTER_Q_MIN,
        Constants::BIQUAD_FILTER_Q_MAX,
        Constants::BIQUAD_FILTER_Q_DEFAULT
    ),
    filter_2_gain(
        name + "F2G",
        Constants::BIQUAD_FILTER_GAIN_MIN,
        Constants::BIQUAD_FILTER_GAIN_MAX,
        Constants::BIQUAD_FILTER_GAIN_DEFAULT
    ),

    distortion(name + "DG", 0.0, 1.0, 0.0)
{
}


template<class ModulatorSignalProducerClass>
Voice<ModulatorSignalProducerClass>::VolumeApplier::VolumeApplier(
        Filter2& input,
        FloatParamS& velocity,
        FloatParamS& volume
) noexcept
    : Filter<Filter2>(input),
    volume(volume),
    velocity(velocity)
{
}


template<class ModulatorSignalProducerClass>
Sample const* const* Voice<ModulatorSignalProducerClass>::VolumeApplier::initialize_rendering(
        Integer const round,
        Integer const sample_count
) noexcept {
    Filter<Filter2>::initialize_rendering(round, sample_count);

    volume_buffer = FloatParamS::produce_if_not_constant<FloatParamS>(
        volume, round, sample_count
    );

    if (volume_buffer == NULL) {
        volume_value = (Sample)volume.get_value();
    }

    velocity_buffer = FloatParamS::produce_if_not_constant<FloatParamS>(
        velocity, round, sample_count
    );

    if (velocity_buffer == NULL) {
        velocity_value = (Sample)velocity.get_value();
    }

    return NULL;
}


template<class ModulatorSignalProducerClass>
void Voice<ModulatorSignalProducerClass>::VolumeApplier::render(
        Integer const round,
        Integer const first_sample_index,
        Integer const last_sample_index,
        Sample** buffer
) noexcept {
    Integer const channels = this->channels;
    Sample const* volume_buffer = this->volume_buffer;
    Sample const* velocity_buffer = this->velocity_buffer;

    if (volume_buffer == NULL) {
        Sample const volume_value = this->volume_value;

        if (LIKELY(velocity_buffer == NULL)) {
            Sample const velocity_value = this->velocity_value;

            for (Integer c = 0; c != channels; ++c) {
                Sample const* const input = this->input_buffer[c];

                for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                    buffer[c][i] = velocity_value * volume_value * input[i];
                }
            }
        } else {
            for (Integer c = 0; c != channels; ++c) {
                Sample const* const input = this->input_buffer[c];

                for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                    buffer[c][i] = velocity_buffer[i] * volume_value * input[i];
                }
            }
        }
    } else if (LIKELY(velocity_buffer == NULL)) {
        Sample const velocity_value = this->velocity_value;

        for (Integer c = 0; c != channels; ++c) {
            Sample const* const input = this->input_buffer[c];

            for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                buffer[c][i] = velocity_value * volume_buffer[i] * input[i];
            }
        }
    } else {
        for (Integer c = 0; c != channels; ++c) {
            Sample const* const input = this->input_buffer[c];

            for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                buffer[c][i] = velocity_buffer[i] * volume_buffer[i] * input[i];
            }
        }
    }
}


template<class ModulatorSignalProducerClass>
Voice<ModulatorSignalProducerClass>::Voice(
        Frequency const* frequencies,
        Midi::Note const notes,
        Params& param_leaders,
        BiquadFilterSharedCache* filter_1_shared_cache,
        BiquadFilterSharedCache* filter_2_shared_cache
) noexcept
    : SignalProducer(CHANNELS, 12),
    notes(notes),
    param_leaders(param_leaders),
    oscillator(
        param_leaders.waveform,
        param_leaders.amplitude,
        param_leaders.detune,
        param_leaders.fine_detune,
        param_leaders.harmonic_0,
        param_leaders.harmonic_1,
        param_leaders.harmonic_2,
        param_leaders.harmonic_3,
        param_leaders.harmonic_4,
        param_leaders.harmonic_5,
        param_leaders.harmonic_6,
        param_leaders.harmonic_7,
        param_leaders.harmonic_8,
        param_leaders.harmonic_9
    ),
    filter_1(
        oscillator,
        param_leaders.filter_1_type,
        param_leaders.filter_1_frequency,
        param_leaders.filter_1_q,
        param_leaders.filter_1_gain,
        filter_1_shared_cache
    ),
    wavefolder(filter_1, param_leaders.folding),
    filter_2(
        wavefolder,
        param_leaders.filter_2_type,
        param_leaders.filter_2_frequency,
        param_leaders.filter_2_q,
        param_leaders.filter_2_gain,
        filter_2_shared_cache
    ),
    velocity_sensitivity(param_leaders.velocity_sensitivity),
    note_velocity("NV", 0.0, 1.0, 1.0),
    note_panning("NP", -1.0, 1.0, 0.0),
    portamento_length(param_leaders.portamento_length),
    portamento_depth(param_leaders.portamento_depth),
    panning(param_leaders.panning),
    volume(param_leaders.volume),
    volume_applier(filter_2, note_velocity, volume),
    modulation_out((ModulationOut&)volume_applier)
{
    initialize_instance(frequencies);
}


template<class ModulatorSignalProducerClass>
Voice<ModulatorSignalProducerClass>::Voice(
        Frequency const* frequencies,
        Midi::Note const notes,
        Params& param_leaders,
        ModulatorSignalProducerClass& modulator,
        FloatParamS& amplitude_modulation_level_leader,
        FloatParamS& frequency_modulation_level_leader,
        FloatParamS& phase_modulation_level_leader,
        BiquadFilterSharedCache* filter_1_shared_cache,
        BiquadFilterSharedCache* filter_2_shared_cache
) noexcept
    : SignalProducer(CHANNELS, 12),
    notes(notes),
    param_leaders(param_leaders),
    oscillator(
        param_leaders.waveform,
        param_leaders.amplitude,
        param_leaders.detune,
        param_leaders.fine_detune,
        param_leaders.harmonic_0,
        param_leaders.harmonic_1,
        param_leaders.harmonic_2,
        param_leaders.harmonic_3,
        param_leaders.harmonic_4,
        param_leaders.harmonic_5,
        param_leaders.harmonic_6,
        param_leaders.harmonic_7,
        param_leaders.harmonic_8,
        param_leaders.harmonic_9,
        &modulator,
        amplitude_modulation_level_leader,
        frequency_modulation_level_leader,
        phase_modulation_level_leader
    ),
    filter_1(
        oscillator,
        param_leaders.filter_1_type,
        param_leaders.filter_1_frequency,
        param_leaders.filter_1_q,
        param_leaders.filter_1_gain,
        filter_1_shared_cache
    ),
    wavefolder(filter_1, param_leaders.folding),
    distortion(
        "DIST",
        Distortion::Type::MEDIUM,
        wavefolder,
        param_leaders.distortion
    ),
    filter_2(
        distortion,
        param_leaders.filter_2_type,
        param_leaders.filter_2_frequency,
        param_leaders.filter_2_q,
        param_leaders.filter_2_gain,
        filter_2_shared_cache
    ),
    velocity_sensitivity(param_leaders.velocity_sensitivity),
    note_velocity("NV", 0.0, 1.0, 1.0),
    note_panning("NP", -1.0, 1.0, 0.0),
    portamento_length(param_leaders.portamento_length),
    portamento_depth(param_leaders.portamento_depth),
    panning(param_leaders.panning),
    volume(param_leaders.volume),
    volume_applier(filter_2, note_velocity, volume),
    modulation_out((ModulationOut&)volume_applier)
{
    initialize_instance(frequencies);
}


template<class ModulatorSignalProducerClass>
void Voice<ModulatorSignalProducerClass>::initialize_instance(
        Frequency const* frequencies
) noexcept {
    this->frequencies = frequencies;

    state = State::OFF;
    note_id = 0;
    note = 0;
    channel = 0;

    register_child(velocity_sensitivity);
    register_child(note_velocity);
    register_child(note_panning);
    register_child(portamento_length);
    register_child(portamento_depth);
    register_child(panning);
    register_child(volume);

    register_child(oscillator);
    register_child(filter_1);
    register_child(wavefolder);

    if constexpr (IS_CARRIER) {
        register_child(distortion);
    }

    register_child(filter_2);
    register_child(volume_applier);
}


template<class ModulatorSignalProducerClass>
void Voice<ModulatorSignalProducerClass>::reset() noexcept
{
    SignalProducer::reset();

    state = OFF;
    note_id = 0;
    note = 0;
    channel = 0;
}


template<class ModulatorSignalProducerClass>
bool Voice<ModulatorSignalProducerClass>::is_on() const noexcept
{
    return !is_off_after(current_time);
}


template<class ModulatorSignalProducerClass>
bool Voice<ModulatorSignalProducerClass>::is_off_after(
        Seconds const time_offset
) const noexcept {
    return is_released() && !oscillator.has_events_after(time_offset);
}


template<class ModulatorSignalProducerClass>
bool Voice<ModulatorSignalProducerClass>::is_released() const noexcept
{
    return state == OFF;
}


template<class ModulatorSignalProducerClass>
void Voice<ModulatorSignalProducerClass>::note_on(
        Seconds const time_offset,
        Integer const note_id,
        Midi::Note const note,
        Midi::Channel const channel,
        Number const velocity,
        Midi::Note const previous_note
) noexcept {
    if (state == ON || note >= notes) {
        return;
    }

    state = ON;

    save_note_info(note_id, note, channel);

    note_velocity.cancel_events_at(time_offset);
    note_velocity.schedule_value(time_offset, calculate_note_velocity(velocity));

    note_panning.cancel_events_at(time_offset);
    note_panning.schedule_value(time_offset, calculate_note_panning(note));

    oscillator.cancel_events_at(time_offset);

    wavefolder.folding.start_envelope(time_offset);

    panning.start_envelope(time_offset);
    volume.start_envelope(time_offset);

    set_up_oscillator_frequency(time_offset, note, previous_note);

    /*
    Though we never assign an envelope to some Oscillator parameters, their
    modulation level parameter might have one (through the leader).
    */
    oscillator.modulated_amplitude.start_envelope(time_offset);
    oscillator.amplitude.start_envelope(time_offset);
    oscillator.frequency.start_envelope(time_offset);
    oscillator.phase.start_envelope(time_offset);

    oscillator.fine_detune.start_envelope(time_offset);

    filter_1.frequency.start_envelope(time_offset);
    filter_1.q.start_envelope(time_offset);
    filter_1.gain.start_envelope(time_offset);

    filter_2.frequency.start_envelope(time_offset);
    filter_2.q.start_envelope(time_offset);
    filter_2.gain.start_envelope(time_offset);

    oscillator.start(time_offset);
}


template<class ModulatorSignalProducerClass>
void Voice<ModulatorSignalProducerClass>::save_note_info(
        Integer const note_id,
        Midi::Note const note,
        Midi::Channel const channel
) noexcept {
    this->note_id = note_id;
    this->note = note;
    this->channel = channel;
}


template<class ModulatorSignalProducerClass>
Number Voice<ModulatorSignalProducerClass>::calculate_note_velocity(
        Number const raw_velocity
) const noexcept {
    Number const sensitivity = velocity_sensitivity.get_value();

    if (sensitivity <= 1.0) {
        return 1.0 - sensitivity + sensitivity * raw_velocity;
    }

    Number const oversensitivity = sensitivity - 1.0;
    Number const velocity_sqr = raw_velocity * raw_velocity;

    return (
        raw_velocity
        + oversensitivity * (velocity_sqr * velocity_sqr - raw_velocity)
    );
}


template<class ModulatorSignalProducerClass>
Number Voice<ModulatorSignalProducerClass>::calculate_note_panning(
        Midi::Note const note
) const noexcept {
    /* note_panning = 2.0 * (note / 127.0) - 1.0; */

    return std::min(
        1.0,
        std::max(
            -1.0,
            NOTE_PANNING_SCALE * (
                note + param_leaders.detune.get_value() * Constants::DETUNE_SCALE
            ) - 1.0
        )
    ) * param_leaders.width.get_value();
}


template<class ModulatorSignalProducerClass>
void Voice<ModulatorSignalProducerClass>::set_up_oscillator_frequency(
        Seconds const time_offset,
        Midi::Note const note,
        Midi::Note const previous_note
) noexcept {
    Number const portamento_length = this->portamento_length.get_value();
    Frequency const note_frequency = frequencies[note];

    oscillator.frequency.cancel_events_at(time_offset);

    if (portamento_length <= sampling_period) {
        oscillator.frequency.schedule_value(time_offset, (Number)note_frequency);
        return;
    }

    Number const portamento_depth = this->portamento_depth.get_value();
    Frequency const start_frequency = (
        std::fabs(portamento_depth) < 0.01
            ? frequencies[previous_note]
            : Math::detune(note_frequency, portamento_depth)
    );

    oscillator.frequency.schedule_value(time_offset, (Number)start_frequency);
    oscillator.frequency.schedule_linear_ramp(
        portamento_length, (Number)note_frequency
    );
}


template<class ModulatorSignalProducerClass>
void Voice<ModulatorSignalProducerClass>::retrigger(
        Seconds const time_offset,
        Integer const note_id,
        Midi::Note const note,
        Midi::Channel const channel,
        Number const velocity,
        Midi::Note const previous_note
) noexcept {
    if (note >= notes) {
        return;
    }

    cancel_note_smoothly(time_offset);
    note_on(
        time_offset + SMOOTH_NOTE_CANCELLATION_DURATION,
        note_id,
        note,
        channel,
        velocity,
        previous_note
    );
}


template<class ModulatorSignalProducerClass>
void Voice<ModulatorSignalProducerClass>::glide_to(
        Seconds const time_offset,
        Integer const note_id,
        Midi::Note const note,
        Midi::Channel const channel,
        Number const velocity,
        Midi::Note const previous_note
) noexcept {
    if (note >= notes) {
        return;
    }

    Number const portamento_length = this->portamento_length.get_value();

    if (portamento_length <= 0.000001) {
        retrigger(time_offset, note_id, note, channel, velocity, previous_note);

        return;
    }

    save_note_info(note_id, note, channel);

    wavefolder.folding.update_envelope(time_offset);

    panning.update_envelope(time_offset);
    volume.update_envelope(time_offset);

    /*
    Though we never assign an envelope to some Oscillator parameters, their
    modulation level parameter might have one (through the leader).
    */
    oscillator.modulated_amplitude.update_envelope(time_offset);
    oscillator.amplitude.update_envelope(time_offset);
    oscillator.frequency.update_envelope(time_offset);
    oscillator.phase.update_envelope(time_offset);

    oscillator.fine_detune.update_envelope(time_offset);

    filter_1.frequency.update_envelope(time_offset);
    filter_1.q.update_envelope(time_offset);
    filter_1.gain.update_envelope(time_offset);

    filter_2.frequency.update_envelope(time_offset);
    filter_2.q.update_envelope(time_offset);
    filter_2.gain.update_envelope(time_offset);

    note_velocity.cancel_events_at(time_offset);
    note_panning.cancel_events_at(time_offset);

    oscillator.frequency.cancel_events_at(time_offset);

    note_velocity.schedule_linear_ramp(portamento_length, calculate_note_velocity(velocity));
    note_panning.schedule_linear_ramp(portamento_length, calculate_note_panning(note));
    oscillator.frequency.schedule_linear_ramp(portamento_length, frequencies[note]);
}


template<class ModulatorSignalProducerClass>
void Voice<ModulatorSignalProducerClass>::note_off(
        Seconds const time_offset,
        Integer const note_id,
        Midi::Note const note,
        Number const velocity
) noexcept {
    if (state != ON || note_id != this->note_id || note != this->note) {
        return;
    }

    /*
    Though we never assign an envelope to some Oscillator parameters, their
    modulation level parameter might have one (through the leader).
    */
    oscillator.modulated_amplitude.end_envelope(time_offset);
    oscillator.frequency.end_envelope(time_offset);
    oscillator.phase.end_envelope(time_offset);

    Seconds off_after = time_offset + std::max(
        oscillator.amplitude.end_envelope(time_offset),
        volume.end_envelope(time_offset)
    );

    oscillator.cancel_events_at(off_after);
    oscillator.stop(off_after);

    state = OFF;

    wavefolder.folding.end_envelope(time_offset);

    panning.end_envelope(time_offset);

    oscillator.fine_detune.end_envelope(time_offset);

    filter_1.frequency.end_envelope(time_offset);
    filter_1.q.end_envelope(time_offset);
    filter_1.gain.end_envelope(time_offset);

    filter_2.frequency.end_envelope(time_offset);
    filter_2.q.end_envelope(time_offset);
    filter_2.gain.end_envelope(time_offset);
}


template<class ModulatorSignalProducerClass>
void Voice<ModulatorSignalProducerClass>::cancel_note() noexcept
{
    if (state != ON) {
        return;
    }

    note_id = 0;
    note = 0;
    channel = 0;

    state = OFF;

    oscillator.amplitude.cancel_events();
    volume.cancel_events();

    oscillator.cancel_events();
    oscillator.stop(0.0);

    wavefolder.folding.cancel_events();

    panning.cancel_events();

    oscillator.modulated_amplitude.cancel_events();
    oscillator.frequency.cancel_events();
    oscillator.phase.cancel_events();
    oscillator.fine_detune.cancel_events();

    filter_1.frequency.cancel_events();
    filter_1.q.cancel_events();
    filter_1.gain.cancel_events();

    filter_2.frequency.cancel_events();
    filter_2.q.cancel_events();
    filter_2.gain.cancel_events();
}


template<class ModulatorSignalProducerClass>
void Voice<ModulatorSignalProducerClass>::cancel_note_smoothly(
        Seconds const time_offset
) noexcept {
    state = OFF;

    wavefolder.folding.cancel_envelope(time_offset, SMOOTH_NOTE_CANCELLATION_DURATION);

    panning.cancel_envelope(time_offset, SMOOTH_NOTE_CANCELLATION_DURATION);
    volume.cancel_envelope(time_offset, SMOOTH_NOTE_CANCELLATION_DURATION);

    /*
    Though we never assign an envelope to some Oscillator parameters, their
    modulation level parameter might have one (through the leader).
    */
    oscillator.modulated_amplitude.cancel_envelope(time_offset, SMOOTH_NOTE_CANCELLATION_DURATION);
    oscillator.amplitude.cancel_envelope(time_offset, SMOOTH_NOTE_CANCELLATION_DURATION);
    oscillator.frequency.cancel_envelope(time_offset, SMOOTH_NOTE_CANCELLATION_DURATION);
    oscillator.phase.cancel_envelope(time_offset, SMOOTH_NOTE_CANCELLATION_DURATION);

    oscillator.stop(time_offset + SMOOTH_NOTE_CANCELLATION_DURATION);

    oscillator.fine_detune.cancel_envelope(time_offset, SMOOTH_NOTE_CANCELLATION_DURATION);

    filter_1.frequency.cancel_envelope(time_offset, SMOOTH_NOTE_CANCELLATION_DURATION);
    filter_1.q.cancel_envelope(time_offset, SMOOTH_NOTE_CANCELLATION_DURATION);
    filter_1.gain.cancel_envelope(time_offset, SMOOTH_NOTE_CANCELLATION_DURATION);

    filter_2.frequency.cancel_envelope(time_offset, SMOOTH_NOTE_CANCELLATION_DURATION);
    filter_2.q.cancel_envelope(time_offset, SMOOTH_NOTE_CANCELLATION_DURATION);
    filter_2.gain.cancel_envelope(time_offset, SMOOTH_NOTE_CANCELLATION_DURATION);
}


template<class ModulatorSignalProducerClass>
bool Voice<ModulatorSignalProducerClass>::has_decayed_during_envelope_dahds() const noexcept
{
    return (
        state == ON
        && (has_decayed(volume) || has_decayed(oscillator.amplitude))
    );
}


template<class ModulatorSignalProducerClass>
bool Voice<ModulatorSignalProducerClass>::has_decayed(
        FloatParamS const& param
) const noexcept {
    constexpr Number threshold = 0.000001;

    Envelope* envelope = param.get_envelope();

    if (envelope == NULL) {
        return false;
    }

    return (
        !param.has_events_after(0.0)
        && param.get_value() < threshold
        && envelope->final_value.get_value() < threshold
    );
}


template<class ModulatorSignalProducerClass>
Integer Voice<ModulatorSignalProducerClass>::get_note_id() const noexcept
{
    return note_id;
}


template<class ModulatorSignalProducerClass>
Midi::Note Voice<ModulatorSignalProducerClass>::get_note() const noexcept
{
    return note;
}


template<class ModulatorSignalProducerClass>
Midi::Note Voice<ModulatorSignalProducerClass>::get_channel() const noexcept
{
    return channel;
}


template<class ModulatorSignalProducerClass>
Sample const* const* Voice<ModulatorSignalProducerClass>::initialize_rendering(
        Integer const round,
        Integer const sample_count
) noexcept {
    volume_applier_buffer = SignalProducer::produce<VolumeApplier>(
        volume_applier, round, sample_count
    )[0];

    panning_buffer = FloatParamS::produce_if_not_constant<FloatParamS>(
        panning, round, sample_count
    );

    if (panning_buffer == NULL) {
        panning_value = panning.get_value();
    }

    note_panning_buffer = FloatParamS::produce_if_not_constant<FloatParamS>(
        note_panning, round, sample_count
    );

    if (note_panning_buffer == NULL) {
        note_panning_value = note_panning.get_value();
    }

    return NULL;
}


template<class ModulatorSignalProducerClass>
void Voice<ModulatorSignalProducerClass>::render(
        Integer const round,
        Integer const first_sample_index,
        Integer const last_sample_index,
        Sample** buffer
) noexcept {
    /* https://www.w3.org/TR/webaudio/#stereopanner-algorithm */

    Sample const* const panning_buffer = this->panning_buffer;
    Sample const* const note_panning_buffer = this->note_panning_buffer;

    if (LIKELY(note_panning_buffer == NULL)) {
        if (panning_buffer == NULL) {
            Number const panning = std::min(
                1.0, std::max(panning_value + note_panning_value, -1.0)
            );
            Number const x = (panning + 1.0) * Math::PI_QUARTER;

            Sample left_gain;
            Sample right_gain;

            Math::sincos(x, right_gain, left_gain);

            for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                buffer[0][i] = left_gain * volume_applier_buffer[i];
                buffer[1][i] = right_gain * volume_applier_buffer[i];
            }
        } else {
            for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                Number const panning = std::min(
                    1.0, std::max(panning_buffer[i] + note_panning_value, -1.0)
                );
                Number const x = (panning + 1.0) * Math::PI_QUARTER;

                Sample left_gain;
                Sample right_gain;

                Math::sincos(x, right_gain, left_gain);

                buffer[0][i] = left_gain * volume_applier_buffer[i];
                buffer[1][i] = right_gain * volume_applier_buffer[i];
            }
        }
    } else if (panning_buffer == NULL) {
        for (Integer i = first_sample_index; i != last_sample_index; ++i) {
            Number const panning = std::min(
                1.0, std::max(panning_value + note_panning_buffer[i], -1.0)
            );
            Number const x = (panning + 1.0) * Math::PI_QUARTER;

            Sample left_gain;
            Sample right_gain;

            Math::sincos(x, right_gain, left_gain);

            buffer[0][i] = left_gain * volume_applier_buffer[i];
            buffer[1][i] = right_gain * volume_applier_buffer[i];
        }
    } else {
        for (Integer i = first_sample_index; i != last_sample_index; ++i) {
            Number const panning = std::min(
                1.0, std::max(panning_buffer[i] + note_panning_buffer[i], -1.0)
            );
            Number const x = (panning + 1.0) * Math::PI_QUARTER;

            Sample left_gain;
            Sample right_gain;

            Math::sincos(x, right_gain, left_gain);

            buffer[0][i] = left_gain * volume_applier_buffer[i];
            buffer[1][i] = right_gain * volume_applier_buffer[i];
        }
    }
}

}

#endif
