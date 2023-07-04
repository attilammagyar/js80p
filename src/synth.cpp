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

#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <sstream>

#include "js80p.hpp"
#include "midi.hpp"

#include "synth.hpp"

#include "dsp/biquad_filter.cpp"
#include "dsp/chorus.cpp"
#include "dsp/delay.cpp"
#include "dsp/distortion.cpp"
#include "dsp/echo.cpp"
#include "dsp/effect.cpp"
#include "dsp/effects.cpp"
#include "dsp/envelope.cpp"
#include "dsp/filter.cpp"
#include "dsp/flexible_controller.cpp"
#include "dsp/gain.cpp"
#include "dsp/lfo.cpp"
#include "dsp/math.cpp"
#include "dsp/midi_controller.cpp"
#include "dsp/mixer.cpp"
#include "dsp/oscillator.cpp"
#include "dsp/param.cpp"
#include "dsp/reverb.cpp"
#include "dsp/queue.cpp"
#include "dsp/signal_producer.cpp"
#include "dsp/voice.cpp"
#include "dsp/wavefolder.cpp"
#include "dsp/wavetable.cpp"


namespace JS80P
{

std::vector<bool> Synth::supported_midi_controllers(Synth::MIDI_CONTROLLERS, false);

bool Synth::supported_midi_controllers_initialized = false;


Synth::ParamIdHashTable Synth::param_id_hash_table;

std::string Synth::param_names_by_id[ParamId::MAX_PARAM_ID];


Synth::ModeParam::ModeParam(std::string const name) noexcept
    : Param<Mode>(name, MIX_AND_MOD, SPLIT_AT_C4, MIX_AND_MOD)
{
}


Synth::Synth() noexcept
    : SignalProducer(
        OUT_CHANNELS,
        6                           // MODE + MIX + PM + FM + AM + bus
        + 31 * 2                    // Modulator::Params + Carrier::Params
        + POLYPHONY * 2             // modulators + carriers
        + 1                         // effects
        + FLEXIBLE_CONTROLLERS * 6
        + ENVELOPES * 10
        + LFOS
    ),
    mode("MODE"),
    modulator_add_volume("MIX", 0.0, 1.0, 1.0),
    phase_modulation_level(
        "PM", Constants::PM_MIN, Constants::PM_MAX, Constants::PM_DEFAULT
    ),
    frequency_modulation_level(
        "FM", Constants::FM_MIN, Constants::FM_MAX, Constants::FM_DEFAULT
    ),
    amplitude_modulation_level(
        "AM", Constants::AM_MIN, Constants::AM_MAX, Constants::AM_DEFAULT
    ),
    modulator_params("M"),
    carrier_params("C"),
    bus(
        OUT_CHANNELS,
        modulators,
        carriers,
        POLYPHONY,
        modulator_add_volume
    ),
    effects("E", bus),
    next_voice(0),
    previous_note(Midi::NOTE_MAX + 1),
    is_learning(false),
    is_sustaining(false),
    midi_controllers((MidiController* const*)midi_controllers_rw),
    flexible_controllers((FlexibleController* const*)flexible_controllers_rw),
    envelopes((Envelope* const*)envelopes_rw),
    lfos((LFO* const*)lfos_rw)
{
    delayed_note_offs.reserve(POLYPHONY);

    initialize_supported_midi_controllers();

    for (int i = 0; i != 4; ++i) {
        biquad_filter_shared_caches[i] = new BiquadFilterSharedCache();
    }

    for (int i = 0; i != FLOAT_PARAMS; ++i) {
        float_params[i] = NULL;
    }

    for (int i = 0; i != ParamId::MAX_PARAM_ID; ++i) {
        param_ratios[i].store(0.0);
        controller_assignments[i].store(ControllerId::NONE);
        param_names_by_id[i] = "";
    }

    for (Midi::Note note = 0; note != Midi::NOTES; ++note) {
        /*
         * Not using Math::exp() and friends here, for 2 reasons:
         *  1. We go out of their domain here.
         *  2. Since this loop runs only once, we can afford favoring accuracy
         *     over speed.
         */
        frequencies[note] = (
            (Frequency)std::pow(2.0, ((Frequency)note - 69.0) / 12.0) * 440.0
        );
    }

    register_main_params();
    register_child(bus);
    register_modulator_params();
    register_carrier_params();
    register_child(effects);
    register_effects_params();

    create_voices();
    create_midi_controllers();
    create_flexible_controllers();
    create_envelopes();
    create_lfos();

    modulator_params.filter_1_log_scale.set_value(ToggleParam::ON);
    modulator_params.filter_2_log_scale.set_value(ToggleParam::ON);
    carrier_params.filter_1_log_scale.set_value(ToggleParam::ON);
    carrier_params.filter_2_log_scale.set_value(ToggleParam::ON);
    effects.filter_1_log_scale.set_value(ToggleParam::ON);
    effects.filter_2_log_scale.set_value(ToggleParam::ON);

    channel_pressure_ctl.change(0.0, 0.0);
    channel_pressure_ctl.clear();

    update_param_states();
}


void Synth::initialize_supported_midi_controllers() noexcept
{
    if (supported_midi_controllers_initialized) {
        return;
    }

    supported_midi_controllers_initialized = true;

    supported_midi_controllers[ControllerId::MODULATION_WHEEL] = true;
    supported_midi_controllers[ControllerId::BREATH] = true;
    supported_midi_controllers[ControllerId::UNDEFINED_1] = true;
    supported_midi_controllers[ControllerId::FOOT_PEDAL] = true;
    supported_midi_controllers[ControllerId::PORTAMENTO_TIME] = true;
    supported_midi_controllers[ControllerId::VOLUME] = true;
    supported_midi_controllers[ControllerId::BALANCE] = true;
    supported_midi_controllers[ControllerId::UNDEFINED_2] = true;
    supported_midi_controllers[ControllerId::PAN] = true;
    supported_midi_controllers[ControllerId::EXPRESSION_PEDAL] = true;
    supported_midi_controllers[ControllerId::FX_CTL_1] = true;
    supported_midi_controllers[ControllerId::FX_CTL_2] = true;
    supported_midi_controllers[ControllerId::UNDEFINED_3] = true;
    supported_midi_controllers[ControllerId::UNDEFINED_4] = true;
    supported_midi_controllers[ControllerId::GENERAL_1] = true;
    supported_midi_controllers[ControllerId::GENERAL_2] = true;
    supported_midi_controllers[ControllerId::GENERAL_3] = true;
    supported_midi_controllers[ControllerId::GENERAL_4] = true;
    supported_midi_controllers[ControllerId::UNDEFINED_5] = true;
    supported_midi_controllers[ControllerId::UNDEFINED_6] = true;
    supported_midi_controllers[ControllerId::UNDEFINED_7] = true;
    supported_midi_controllers[ControllerId::UNDEFINED_8] = true;
    supported_midi_controllers[ControllerId::UNDEFINED_9] = true;
    supported_midi_controllers[ControllerId::UNDEFINED_10] = true;
    supported_midi_controllers[ControllerId::UNDEFINED_11] = true;
    supported_midi_controllers[ControllerId::UNDEFINED_12] = true;
    supported_midi_controllers[ControllerId::UNDEFINED_13] = true;
    supported_midi_controllers[ControllerId::UNDEFINED_14] = true;
    supported_midi_controllers[ControllerId::UNDEFINED_15] = true;
    supported_midi_controllers[ControllerId::UNDEFINED_16] = true;
    supported_midi_controllers[ControllerId::SUSTAIN_PEDAL] = true;
    supported_midi_controllers[ControllerId::SOUND_1] = true;
    supported_midi_controllers[ControllerId::SOUND_2] = true;
    supported_midi_controllers[ControllerId::SOUND_3] = true;
    supported_midi_controllers[ControllerId::SOUND_4] = true;
    supported_midi_controllers[ControllerId::SOUND_5] = true;
    supported_midi_controllers[ControllerId::SOUND_6] = true;
    supported_midi_controllers[ControllerId::SOUND_7] = true;
    supported_midi_controllers[ControllerId::SOUND_8] = true;
    supported_midi_controllers[ControllerId::SOUND_9] = true;
    supported_midi_controllers[ControllerId::SOUND_10] = true;
    supported_midi_controllers[ControllerId::UNDEFINED_17] = true;
    supported_midi_controllers[ControllerId::UNDEFINED_18] = true;
    supported_midi_controllers[ControllerId::UNDEFINED_19] = true;
    supported_midi_controllers[ControllerId::UNDEFINED_20] = true;
    supported_midi_controllers[ControllerId::UNDEFINED_21] = true;
    supported_midi_controllers[ControllerId::FX_1] = true;
    supported_midi_controllers[ControllerId::FX_2] = true;
    supported_midi_controllers[ControllerId::FX_3] = true;
    supported_midi_controllers[ControllerId::FX_4] = true;
    supported_midi_controllers[ControllerId::FX_5] = true;
    supported_midi_controllers[ControllerId::UNDEFINED_22] = true;
    supported_midi_controllers[ControllerId::UNDEFINED_23] = true;
    supported_midi_controllers[ControllerId::UNDEFINED_24] = true;
    supported_midi_controllers[ControllerId::UNDEFINED_25] = true;
    supported_midi_controllers[ControllerId::UNDEFINED_26] = true;
    supported_midi_controllers[ControllerId::UNDEFINED_27] = true;
    supported_midi_controllers[ControllerId::UNDEFINED_28] = true;
    supported_midi_controllers[ControllerId::UNDEFINED_29] = true;
    supported_midi_controllers[ControllerId::UNDEFINED_30] = true;
    supported_midi_controllers[ControllerId::UNDEFINED_31] = true;
    supported_midi_controllers[ControllerId::UNDEFINED_32] = true;
    supported_midi_controllers[ControllerId::UNDEFINED_33] = true;
    supported_midi_controllers[ControllerId::UNDEFINED_34] = true;
    supported_midi_controllers[ControllerId::UNDEFINED_35] = true;
    supported_midi_controllers[ControllerId::UNDEFINED_36] = true;
    supported_midi_controllers[ControllerId::UNDEFINED_37] = true;
    supported_midi_controllers[ControllerId::UNDEFINED_38] = true;
    supported_midi_controllers[ControllerId::UNDEFINED_39] = true;
}


void Synth::register_main_params() noexcept
{
    register_param_as_child(ParamId::MODE, mode);

    register_float_param_as_child(ParamId::MIX, modulator_add_volume);
    register_float_param_as_child(ParamId::PM, phase_modulation_level);
    register_float_param_as_child(ParamId::FM, frequency_modulation_level);
    register_float_param_as_child(ParamId::AM, amplitude_modulation_level);
}


void Synth::register_modulator_params() noexcept
{
    register_param_as_child<Modulator::Oscillator_::WaveformParam>(
        ParamId::MWAV, modulator_params.waveform
    );
    register_float_param_as_child(ParamId::MAMP, modulator_params.amplitude);
    register_float_param_as_child(ParamId::MVS, modulator_params.velocity_sensitivity);
    register_float_param_as_child(ParamId::MFLD, modulator_params.folding);
    register_float_param_as_child(ParamId::MPRT, modulator_params.portamento_length);
    register_float_param_as_child(ParamId::MPRD, modulator_params.portamento_depth);
    register_float_param_as_child(ParamId::MDTN, modulator_params.detune);
    register_float_param_as_child(ParamId::MFIN, modulator_params.fine_detune);
    register_float_param_as_child(ParamId::MWID, modulator_params.width);
    register_float_param_as_child(ParamId::MPAN, modulator_params.panning);
    register_float_param_as_child(ParamId::MVOL, modulator_params.volume);

    register_float_param_as_child(ParamId::MC1, modulator_params.harmonic_0);
    register_float_param_as_child(ParamId::MC2, modulator_params.harmonic_1);
    register_float_param_as_child(ParamId::MC3, modulator_params.harmonic_2);
    register_float_param_as_child(ParamId::MC4, modulator_params.harmonic_3);
    register_float_param_as_child(ParamId::MC5, modulator_params.harmonic_4);
    register_float_param_as_child(ParamId::MC6, modulator_params.harmonic_5);
    register_float_param_as_child(ParamId::MC7, modulator_params.harmonic_6);
    register_float_param_as_child(ParamId::MC8, modulator_params.harmonic_7);
    register_float_param_as_child(ParamId::MC9, modulator_params.harmonic_8);
    register_float_param_as_child(ParamId::MC10, modulator_params.harmonic_9);

    register_param_as_child<Modulator::Filter1::TypeParam>(
        ParamId::MF1TYP, modulator_params.filter_1_type
    );
    register_param_as_child<ToggleParam>(ParamId::MF1LOG, modulator_params.filter_1_log_scale);
    register_float_param_as_child(ParamId::MF1FRQ, modulator_params.filter_1_frequency);
    register_float_param_as_child(ParamId::MF1Q, modulator_params.filter_1_q);
    register_float_param_as_child(ParamId::MF1G, modulator_params.filter_1_gain);

    register_param_as_child<Modulator::Filter2::TypeParam>(
        ParamId::MF2TYP, modulator_params.filter_2_type
    );
    register_param_as_child<ToggleParam>(ParamId::MF2LOG, modulator_params.filter_2_log_scale);
    register_float_param_as_child(ParamId::MF2FRQ, modulator_params.filter_2_frequency);
    register_float_param_as_child(ParamId::MF2Q, modulator_params.filter_2_q);
    register_float_param_as_child(ParamId::MF2G, modulator_params.filter_2_gain);
}


void Synth::register_carrier_params() noexcept
{
    register_param_as_child<Carrier::Oscillator_::WaveformParam>(
        ParamId::CWAV, carrier_params.waveform
    );
    register_float_param_as_child(ParamId::CAMP, carrier_params.amplitude);
    register_float_param_as_child(ParamId::CVS, carrier_params.velocity_sensitivity);
    register_float_param_as_child(ParamId::CFLD, carrier_params.folding);
    register_float_param_as_child(ParamId::CPRT, carrier_params.portamento_length);
    register_float_param_as_child(ParamId::CPRD, carrier_params.portamento_depth);
    register_float_param_as_child(ParamId::CDTN, carrier_params.detune);
    register_float_param_as_child(ParamId::CFIN, carrier_params.fine_detune);
    register_float_param_as_child(ParamId::CWID, carrier_params.width);
    register_float_param_as_child(ParamId::CPAN, carrier_params.panning);
    register_float_param_as_child(ParamId::CVOL, carrier_params.volume);

    register_float_param_as_child(ParamId::CC1, carrier_params.harmonic_0);
    register_float_param_as_child(ParamId::CC2, carrier_params.harmonic_1);
    register_float_param_as_child(ParamId::CC3, carrier_params.harmonic_2);
    register_float_param_as_child(ParamId::CC4, carrier_params.harmonic_3);
    register_float_param_as_child(ParamId::CC5, carrier_params.harmonic_4);
    register_float_param_as_child(ParamId::CC6, carrier_params.harmonic_5);
    register_float_param_as_child(ParamId::CC7, carrier_params.harmonic_6);
    register_float_param_as_child(ParamId::CC8, carrier_params.harmonic_7);
    register_float_param_as_child(ParamId::CC9, carrier_params.harmonic_8);
    register_float_param_as_child(ParamId::CC10, carrier_params.harmonic_9);

    register_param_as_child<Carrier::Filter1::TypeParam>(
        ParamId::CF1TYP, carrier_params.filter_1_type
    );
    register_param_as_child<ToggleParam>(ParamId::CF1LOG, carrier_params.filter_1_log_scale);
    register_float_param_as_child(ParamId::CF1FRQ, carrier_params.filter_1_frequency);
    register_float_param_as_child(ParamId::CF1Q, carrier_params.filter_1_q);
    register_float_param_as_child(ParamId::CF1G, carrier_params.filter_1_gain);

    register_param_as_child<Carrier::Filter2::TypeParam>(
        ParamId::CF2TYP, carrier_params.filter_2_type
    );
    register_param_as_child<ToggleParam>(ParamId::CF2LOG, carrier_params.filter_2_log_scale);
    register_float_param_as_child(ParamId::CF2FRQ, carrier_params.filter_2_frequency);
    register_float_param_as_child(ParamId::CF2Q, carrier_params.filter_2_q);
    register_float_param_as_child(ParamId::CF2G, carrier_params.filter_2_gain);
}


void Synth::register_effects_params() noexcept
{
    register_float_param(ParamId::EOG, effects.overdrive.level);

    register_float_param(ParamId::EDG, effects.distortion.level);

    register_param<Effects::Filter1<Bus>::TypeParam>(ParamId::EF1TYP, effects.filter_1_type);
    register_param<ToggleParam>(ParamId::EF1LOG, effects.filter_1_log_scale);
    register_float_param(ParamId::EF1FRQ, effects.filter_1.frequency);
    register_float_param(ParamId::EF1Q, effects.filter_1.q);
    register_float_param(ParamId::EF1G, effects.filter_1.gain);

    register_param<Effects::Filter2<Bus>::TypeParam>(ParamId::EF2TYP, effects.filter_2_type);
    register_param<ToggleParam>(ParamId::EF2LOG, effects.filter_2_log_scale);
    register_float_param(ParamId::EF2FRQ, effects.filter_2.frequency);
    register_float_param(ParamId::EF2Q, effects.filter_2.q);
    register_float_param(ParamId::EF2G, effects.filter_2.gain);

    register_float_param(ParamId::ECDEL, effects.chorus.delay_time);
    register_float_param(ParamId::ECFRQ, effects.chorus.frequency);
    register_float_param(ParamId::ECDPT, effects.chorus.depth);
    register_float_param(ParamId::ECFB, effects.chorus.feedback);
    register_float_param(ParamId::ECDF, effects.chorus.damping_frequency);
    register_float_param(ParamId::ECDG, effects.chorus.damping_gain);
    register_float_param(ParamId::ECWID, effects.chorus.width);
    register_float_param(ParamId::ECHPF, effects.chorus.high_pass_frequency);
    register_float_param(ParamId::ECWET, effects.chorus.wet);
    register_float_param(ParamId::ECDRY, effects.chorus.dry);
    register_param<ToggleParam>(ParamId::ECSYN, effects.chorus.tempo_sync);
    register_param<ToggleParam>(ParamId::ECLOG, effects.chorus.log_scale_frequencies);

    register_float_param(ParamId::EEDEL, effects.echo.delay_time);
    register_float_param(ParamId::EEFB, effects.echo.feedback);
    register_float_param(ParamId::EEDF, effects.echo.damping_frequency);
    register_float_param(ParamId::EEDG, effects.echo.damping_gain);
    register_float_param(ParamId::EEWID, effects.echo.width);
    register_float_param(ParamId::EEHPF, effects.echo.high_pass_frequency);
    register_float_param(ParamId::EEWET, effects.echo.wet);
    register_float_param(ParamId::EEDRY, effects.echo.dry);
    register_param<ToggleParam>(ParamId::EESYN, effects.echo.tempo_sync);
    register_param<ToggleParam>(ParamId::EELOG, effects.echo.log_scale_frequencies);

    register_float_param(ParamId::ERRS, effects.reverb.room_size);
    register_float_param(ParamId::ERDF, effects.reverb.damping_frequency);
    register_float_param(ParamId::ERDG, effects.reverb.damping_gain);
    register_float_param(ParamId::ERWID, effects.reverb.width);
    register_float_param(ParamId::ERHPF, effects.reverb.high_pass_frequency);
    register_float_param(ParamId::ERWET, effects.reverb.wet);
    register_float_param(ParamId::ERDRY, effects.reverb.dry);
    register_param<ToggleParam>(ParamId::ERLOG, effects.reverb.log_scale_frequencies);
}


void Synth::create_voices() noexcept
{
    for (Integer i = 0; i != POLYPHONY; ++i) {
        modulators[i] = new Modulator(
            frequencies,
            Midi::NOTES,
            modulator_params,
            biquad_filter_shared_caches[0],
            biquad_filter_shared_caches[1]
        );
        register_child(*modulators[i]);

        carriers[i] = new Carrier(
            frequencies,
            Midi::NOTES,
            carrier_params,
            biquad_filter_shared_caches[2],
            biquad_filter_shared_caches[3],
            &modulators[i]->modulation_out,
            amplitude_modulation_level,
            frequency_modulation_level,
            phase_modulation_level
        );
        register_child(*carriers[i]);
    }

    clear_midi_note_to_voice_assignments();
}


void Synth::create_midi_controllers() noexcept
{
    for (Integer i = 0; i != MIDI_CONTROLLERS; ++i) {
        midi_controllers_rw[i] = (
            is_supported_midi_controller((ControllerId)i)
                ? new MidiController()
                : NULL
        );
    }
}


void Synth::create_flexible_controllers() noexcept
{
    Integer next_id = ParamId::F1IN;

    for (Integer i = 0; i != FLEXIBLE_CONTROLLERS; ++i) {
        FlexibleController* flexible_controller = (
            new FlexibleController(std::string("F") + to_string(i + 1))
        );
        flexible_controllers_rw[i] = flexible_controller;

        register_float_param_as_child((ParamId)next_id++, flexible_controller->input);
        register_float_param_as_child((ParamId)next_id++, flexible_controller->min);
        register_float_param_as_child((ParamId)next_id++, flexible_controller->max);
        register_float_param_as_child((ParamId)next_id++, flexible_controller->amount);
        register_float_param_as_child((ParamId)next_id++, flexible_controller->distortion);
        register_float_param_as_child((ParamId)next_id++, flexible_controller->randomness);
    }
}


void Synth::create_envelopes() noexcept
{
    Integer next_id = ParamId::N1AMT;

    for (Integer i = 0; i != ENVELOPES; ++i) {
        Envelope* envelope = new Envelope(std::string("N") + to_string(i + 1));
        envelopes_rw[i] = envelope;

        register_float_param_as_child((ParamId)next_id++, envelope->amount);
        register_float_param_as_child((ParamId)next_id++, envelope->initial_value);
        register_float_param_as_child((ParamId)next_id++, envelope->delay_time);
        register_float_param_as_child((ParamId)next_id++, envelope->attack_time);
        register_float_param_as_child((ParamId)next_id++, envelope->peak_value);
        register_float_param_as_child((ParamId)next_id++, envelope->hold_time);
        register_float_param_as_child((ParamId)next_id++, envelope->decay_time);
        register_float_param_as_child((ParamId)next_id++, envelope->sustain_value);
        register_float_param_as_child((ParamId)next_id++, envelope->release_time);
        register_float_param_as_child((ParamId)next_id++, envelope->final_value);
    }

    register_param_as_child<ToggleParam>(ParamId::N1DYN, envelopes_rw[0]->dynamic);
    register_param_as_child<ToggleParam>(ParamId::N2DYN, envelopes_rw[1]->dynamic);
    register_param_as_child<ToggleParam>(ParamId::N3DYN, envelopes_rw[2]->dynamic);
    register_param_as_child<ToggleParam>(ParamId::N4DYN, envelopes_rw[3]->dynamic);
    register_param_as_child<ToggleParam>(ParamId::N5DYN, envelopes_rw[4]->dynamic);
    register_param_as_child<ToggleParam>(ParamId::N6DYN, envelopes_rw[5]->dynamic);
}


void Synth::create_lfos() noexcept
{
    Integer next_id = ParamId::L1FRQ;

    for (Integer i = 0; i != LFOS; ++i) {
        LFO* lfo = new LFO(std::string("L") + to_string(i + 1));
        lfos_rw[i] = lfo;

        register_child(*lfo);
        register_float_param((ParamId)next_id++, lfo->frequency);
        register_float_param((ParamId)next_id++, lfo->phase);
        register_float_param((ParamId)next_id++, lfo->min);
        register_float_param((ParamId)next_id++, lfo->max);
        register_float_param((ParamId)next_id++, lfo->amount);
        register_float_param((ParamId)next_id++, lfo->distortion);
        register_float_param((ParamId)next_id++, lfo->randomness);
    }

    register_param<LFO::Oscillator_::WaveformParam>(ParamId::L1WAV, lfos_rw[0]->waveform);
    register_param<LFO::Oscillator_::WaveformParam>(ParamId::L2WAV, lfos_rw[1]->waveform);
    register_param<LFO::Oscillator_::WaveformParam>(ParamId::L3WAV, lfos_rw[2]->waveform);
    register_param<LFO::Oscillator_::WaveformParam>(ParamId::L4WAV, lfos_rw[3]->waveform);
    register_param<LFO::Oscillator_::WaveformParam>(ParamId::L5WAV, lfos_rw[4]->waveform);
    register_param<LFO::Oscillator_::WaveformParam>(ParamId::L6WAV, lfos_rw[5]->waveform);
    register_param<LFO::Oscillator_::WaveformParam>(ParamId::L7WAV, lfos_rw[6]->waveform);
    register_param<LFO::Oscillator_::WaveformParam>(ParamId::L8WAV, lfos_rw[7]->waveform);

    register_param<ToggleParam>(ParamId::L1CEN, lfos_rw[0]->center);
    register_param<ToggleParam>(ParamId::L2CEN, lfos_rw[1]->center);
    register_param<ToggleParam>(ParamId::L3CEN, lfos_rw[2]->center);
    register_param<ToggleParam>(ParamId::L4CEN, lfos_rw[3]->center);
    register_param<ToggleParam>(ParamId::L5CEN, lfos_rw[4]->center);
    register_param<ToggleParam>(ParamId::L6CEN, lfos_rw[5]->center);
    register_param<ToggleParam>(ParamId::L7CEN, lfos_rw[6]->center);
    register_param<ToggleParam>(ParamId::L8CEN, lfos_rw[7]->center);

    register_param<ToggleParam>(ParamId::L1SYN, lfos_rw[0]->tempo_sync);
    register_param<ToggleParam>(ParamId::L2SYN, lfos_rw[1]->tempo_sync);
    register_param<ToggleParam>(ParamId::L3SYN, lfos_rw[2]->tempo_sync);
    register_param<ToggleParam>(ParamId::L4SYN, lfos_rw[3]->tempo_sync);
    register_param<ToggleParam>(ParamId::L5SYN, lfos_rw[4]->tempo_sync);
    register_param<ToggleParam>(ParamId::L6SYN, lfos_rw[5]->tempo_sync);
    register_param<ToggleParam>(ParamId::L7SYN, lfos_rw[6]->tempo_sync);
    register_param<ToggleParam>(ParamId::L8SYN, lfos_rw[7]->tempo_sync);
}


template<class ParamClass>
void Synth::register_param_as_child(
        ParamId const param_id,
        ParamClass& param
) noexcept {
    register_child(param);
    register_param<ParamClass>(param_id, param);
}


template<class ParamClass>
void Synth::register_param(ParamId const param_id, ParamClass& param) noexcept
{
    std::string const& name = param.get_name();

    param_id_hash_table.add(name, param_id);
    param_names_by_id[param_id] = name;
}


void Synth::register_float_param_as_child(
        ParamId const param_id,
        FloatParam& float_param
) noexcept {
    register_param_as_child<FloatParam>(param_id, float_param);
    float_params[param_id] = &float_param;
}


void Synth::register_float_param(
        ParamId const param_id,
        FloatParam& float_param
) noexcept {
    float_params[param_id] = &float_param;
    register_param<FloatParam>(param_id, float_param);
}


Synth::~Synth()
{
    for (Integer i = 0; i != POLYPHONY; ++i) {
        delete carriers[i];
        delete modulators[i];
    }

    for (Integer i = 0; i != ENVELOPES; ++i) {
        delete envelopes[i];
    }

    for (Integer i = 0; i != MIDI_CONTROLLERS; ++i) {
        if (midi_controllers_rw[i] != NULL) {
            delete midi_controllers_rw[i];
        }
    }
}


bool Synth::is_lock_free() const noexcept
{
    bool is_lock_free = true;

    for (int i = 0; is_lock_free && i != ParamId::MAX_PARAM_ID; ++i) {
        is_lock_free = (
            param_ratios[i].is_lock_free()
            && controller_assignments[i].is_lock_free()
        );
    }

    return is_lock_free && messages.is_lock_free();
}


void Synth::suspend() noexcept
{
    stop_lfos();
    this->reset();
    clear_midi_controllers();
    clear_midi_note_to_voice_assignments();
    clear_sustain();
}


void Synth::stop_lfos() noexcept
{
    for (Integer i = 0; i != LFOS; ++i) {
        lfos_rw[i]->stop(0.0);
    }

    effects.chorus.lfo_1.stop(0.0);
    effects.chorus.lfo_2.stop(0.0);
    effects.chorus.lfo_3.stop(0.0);
}


void Synth::resume() noexcept
{
    this->reset();
    clear_midi_controllers();
    clear_midi_note_to_voice_assignments();
    start_lfos();
    clear_sustain();
}


void Synth::start_lfos() noexcept
{
    for (Integer i = 0; i != LFOS; ++i) {
        lfos_rw[i]->start(0.0);
    }

    effects.chorus.lfo_1.start(0.0);
    effects.chorus.lfo_2.start(0.0);
    effects.chorus.lfo_3.start(0.0);
}


void Synth::note_on(
        Seconds const time_offset,
        Midi::Channel const channel,
        Midi::Note const note,
        Midi::Byte const velocity
) noexcept {
    Number const velocity_float = midi_byte_to_float(velocity);

    this->velocity.change(time_offset, velocity_float);
    this->note.change(time_offset, midi_byte_to_float(note));

    if (midi_note_to_voice_assignments[channel][note] != INVALID_VOICE) {
        return;
    }

    for (Integer v = 0; v != POLYPHONY; ++v) {
        if (!(
                modulators[next_voice]->is_off_after(time_offset)
                && carriers[next_voice]->is_off_after(time_offset)
        )) {
            next_voice = (next_voice + 1) & NEXT_VOICE_MASK;
            continue;
        }

        if (UNLIKELY(previous_note > Midi::NOTE_MAX)) {
            previous_note = note;
        }

        midi_note_to_voice_assignments[channel][note] = next_voice;

        Mode const mode = this->mode.get_value();

        if (mode == MIX_AND_MOD) {
            modulators[next_voice]->note_on(time_offset, note, channel, velocity_float, previous_note);
            carriers[next_voice]->note_on(time_offset, note, channel, velocity_float, previous_note);
        } else {
            if (note < mode + Midi::NOTE_B_2) {
                modulators[next_voice]->note_on(time_offset, note, channel, velocity_float, previous_note);
            } else {
                carriers[next_voice]->note_on(time_offset, note, channel, velocity_float, previous_note);
            }
        }

        previous_note = note;

        break;
    }
}


Number Synth::midi_byte_to_float(Midi::Byte const midi_byte) const noexcept
{
    return (Number)midi_byte * MIDI_BYTE_SCALE;
}


Number Synth::midi_word_to_float(Midi::Word const midi_word) const noexcept
{
    return (Number)midi_word * MIDI_WORD_SCALE;
}


void Synth::aftertouch(
        Seconds const time_offset,
        Midi::Channel const channel,
        Midi::Note const note,
        Midi::Byte const pressure
) noexcept {
    this->note.change(time_offset, midi_byte_to_float(note));

    if (midi_note_to_voice_assignments[channel][note] == INVALID_VOICE) {
        return;
    }

    // Integer const voice = midi_note_to_voice_assignments[channel][note];
}


void Synth::channel_pressure(
        Seconds const time_offset,
        Midi::Channel const channel,
        Midi::Byte const pressure
) noexcept {
    if (
            is_repeated_midi_controller_message(
                ControllerId::CHANNEL_PRESSURE, time_offset, channel, pressure
            )
    ) {
        return;
    }

    channel_pressure_ctl.change(time_offset, midi_byte_to_float(pressure));
}


bool Synth::is_repeated_midi_controller_message(
        ControllerId const controller_id,
        Seconds const time_offset,
        Midi::Channel const channel,
        Midi::Word const value
) noexcept {
    /*
    By default, FL Studio 21 sends multiple clones of the same pitch bend event
    separately on all channels, but it's enough for JS80P to handle only one of
    those.
    */
    MidiControllerMessage const message(time_offset, value);

    if (previous_controller_message[controller_id] == message) {
        return true;
    }

    previous_controller_message[controller_id] = message;

    return false;
}


void Synth::note_off(
        Seconds const time_offset,
        Midi::Channel const channel,
        Midi::Note const note,
        Midi::Byte const velocity
) noexcept {
    if (midi_note_to_voice_assignments[channel][note] == INVALID_VOICE) {
        return;
    }

    Integer const voice = midi_note_to_voice_assignments[channel][note];

    midi_note_to_voice_assignments[channel][note] = INVALID_VOICE;

    if (UNLIKELY(is_sustaining)) {
        DelayedNoteOff const delayed_note_off(channel, note, velocity, voice);

        delayed_note_offs.push_back(delayed_note_off);
    } else {
        Number const velocity_float = midi_byte_to_float(velocity);

        modulators[voice]->note_off(time_offset, note, velocity_float);
        carriers[voice]->note_off(time_offset, note, velocity_float);
    }
}


void Synth::control_change(
        Seconds const time_offset,
        Midi::Channel const channel,
        Midi::Controller const controller,
        Midi::Byte const new_value
) noexcept {
    if (!is_supported_midi_controller(controller)) {
        return;
    }

    if (
            is_repeated_midi_controller_message(
                (ControllerId)controller, time_offset, channel, new_value
            )
    ) {
        return;
    }

    if (is_learning) {
        for (int i = 0; i != ParamId::MAX_PARAM_ID; ++i) {
            if (controller_assignments[i].load() == ControllerId::MIDI_LEARN) {
                handle_assign_controller((ParamId)i, controller);
            }
        }

        is_learning = false;
    }

    midi_controllers_rw[controller]->change(time_offset, midi_byte_to_float(new_value));

    if (controller == Midi::SUSTAIN_PEDAL) {
        if (new_value < 64) {
            sustain_off(time_offset);
        } else {
            sustain_on(time_offset);
        }
    }
}


void Synth::sustain_on(Seconds const time_offset) noexcept
{
    is_sustaining = true;
}


void Synth::sustain_off(Seconds const time_offset) noexcept
{
    is_sustaining = false;

    for (std::vector<DelayedNoteOff>::const_iterator it = delayed_note_offs.begin(); it != delayed_note_offs.end(); ++it) {
        DelayedNoteOff const& delayed_note_off = *it;
        Integer const voice = delayed_note_off.get_voice();

        if (voice != INVALID_VOICE) {
            Midi::Note const note = delayed_note_off.get_note();
            Number const velocity = midi_byte_to_float(delayed_note_off.get_velocity());

            modulators[voice]->note_off(time_offset, note, velocity);
            carriers[voice]->note_off(time_offset, note, velocity);
        }
    }

    delayed_note_offs.clear();
}


bool Synth::is_supported_midi_controller(
        Midi::Controller const controller
) noexcept {
    if ((Integer)controller >= MIDI_CONTROLLERS) {
        return false;
    }

    return supported_midi_controllers[controller];
}


bool Synth::is_controller_polyphonic(ControllerId const controller_id) noexcept
{
    return controller_id >= ControllerId::ENVELOPE_1 && controller_id <= ControllerId::ENVELOPE_6;
}


void Synth::pitch_wheel_change(
        Seconds const time_offset,
        Midi::Channel const channel,
        Midi::Word const new_value
) noexcept {
    if (
            is_repeated_midi_controller_message(
                ControllerId::PITCH_WHEEL, time_offset, channel, new_value
            )
    ) {
        return;
    }

    pitch_wheel.change(time_offset, midi_word_to_float(new_value));
}


void Synth::all_sound_off(
        Seconds const time_offset,
        Midi::Channel const channel
) noexcept {
    suspend();
    resume();
}


void Synth::reset_all_controllers(
        Seconds const time_offset,
        Midi::Channel const channel
) noexcept {
}


void Synth::all_notes_off(
        Seconds const time_offset,
        Midi::Channel const channel
) noexcept {
    for (Midi::Channel channel_ = 0; channel_ != Midi::CHANNELS; ++channel_) {
        for (Midi::Note note = 0; note != Midi::NOTES; ++note) {
            Integer const voice = midi_note_to_voice_assignments[channel_][note];

            if (voice == INVALID_VOICE) {
                continue;
            }

            midi_note_to_voice_assignments[channel_][note] = INVALID_VOICE;

            modulators[voice]->note_off(time_offset, note, 0.0);
            carriers[voice]->note_off(time_offset, note, 0.0);
        }
    }
}


Sample const* const* Synth::generate_samples(
        Integer const round,
        Integer const sample_count
) noexcept {
    return SignalProducer::produce<Synth>(*this, round, sample_count);
}


void Synth::push_message(
        MessageType const type,
        ParamId const param_id,
        Number const number_param,
        Byte const byte_param
) noexcept {
    Message message(type, param_id, number_param, byte_param);

    push_message(message);
}


void Synth::push_message(Message const& message) noexcept
{
    messages.push(message);
}


std::string const& Synth::get_param_name(ParamId const param_id) const noexcept
{
    return param_names_by_id[param_id];
}


Synth::ParamId Synth::get_param_id(std::string const& name) const noexcept
{
    return param_id_hash_table.lookup(name);
}


void Synth::get_param_id_hash_table_statistics(
        Integer& max_collisions,
        Number& avg_collisions,
        Number& avg_bucket_size
) const noexcept {
    param_id_hash_table.get_statistics(
        max_collisions, avg_collisions, avg_bucket_size
    );
}


Number Synth::get_param_ratio_atomic(ParamId const param_id) const noexcept
{
    return param_ratios[param_id].load();
}


Number Synth::get_param_default_ratio(ParamId const param_id) const noexcept
{
    if (param_id < FLOAT_PARAMS) {
        return float_params[param_id]->get_default_ratio();
    }

    switch (param_id) {
        case ParamId::MODE: return mode.get_default_ratio();
        case ParamId::MWAV: return modulator_params.waveform.get_default_ratio();
        case ParamId::CWAV: return carrier_params.waveform.get_default_ratio();
        case ParamId::MF1TYP: return modulator_params.filter_1_type.get_default_ratio();
        case ParamId::MF2TYP: return modulator_params.filter_2_type.get_default_ratio();
        case ParamId::CF1TYP: return carrier_params.filter_1_type.get_default_ratio();
        case ParamId::CF2TYP: return carrier_params.filter_2_type.get_default_ratio();
        case ParamId::EF1TYP: return effects.filter_1_type.get_default_ratio();
        case ParamId::EF2TYP: return effects.filter_2_type.get_default_ratio();
        case ParamId::L1WAV: return lfos_rw[0]->waveform.get_default_ratio();
        case ParamId::L2WAV: return lfos_rw[1]->waveform.get_default_ratio();
        case ParamId::L3WAV: return lfos_rw[2]->waveform.get_default_ratio();
        case ParamId::L4WAV: return lfos_rw[3]->waveform.get_default_ratio();
        case ParamId::L5WAV: return lfos_rw[4]->waveform.get_default_ratio();
        case ParamId::L6WAV: return lfos_rw[5]->waveform.get_default_ratio();
        case ParamId::L7WAV: return lfos_rw[6]->waveform.get_default_ratio();
        case ParamId::L8WAV: return lfos_rw[7]->waveform.get_default_ratio();
        case ParamId::L1CEN: return lfos_rw[0]->center.get_default_ratio();
        case ParamId::L2CEN: return lfos_rw[1]->center.get_default_ratio();
        case ParamId::L3CEN: return lfos_rw[2]->center.get_default_ratio();
        case ParamId::L4CEN: return lfos_rw[3]->center.get_default_ratio();
        case ParamId::L5CEN: return lfos_rw[4]->center.get_default_ratio();
        case ParamId::L6CEN: return lfos_rw[5]->center.get_default_ratio();
        case ParamId::L7CEN: return lfos_rw[6]->center.get_default_ratio();
        case ParamId::L8CEN: return lfos_rw[7]->center.get_default_ratio();
        case ParamId::L1SYN: return lfos_rw[0]->tempo_sync.get_default_ratio();
        case ParamId::L2SYN: return lfos_rw[1]->tempo_sync.get_default_ratio();
        case ParamId::L3SYN: return lfos_rw[2]->tempo_sync.get_default_ratio();
        case ParamId::L4SYN: return lfos_rw[3]->tempo_sync.get_default_ratio();
        case ParamId::L5SYN: return lfos_rw[4]->tempo_sync.get_default_ratio();
        case ParamId::L6SYN: return lfos_rw[5]->tempo_sync.get_default_ratio();
        case ParamId::L7SYN: return lfos_rw[6]->tempo_sync.get_default_ratio();
        case ParamId::L8SYN: return lfos_rw[7]->tempo_sync.get_default_ratio();
        case ParamId::ECSYN: return effects.chorus.tempo_sync.get_default_ratio();
        case ParamId::EESYN: return effects.echo.tempo_sync.get_default_ratio();
        case ParamId::MF1LOG: return modulator_params.filter_1_log_scale.get_default_ratio();
        case ParamId::MF2LOG: return modulator_params.filter_2_log_scale.get_default_ratio();
        case ParamId::CF1LOG: return carrier_params.filter_1_log_scale.get_default_ratio();
        case ParamId::CF2LOG: return carrier_params.filter_2_log_scale.get_default_ratio();
        case ParamId::EF1LOG: return effects.filter_1_log_scale.get_default_ratio();
        case ParamId::EF2LOG: return effects.filter_2_log_scale.get_default_ratio();
        case ParamId::ECLOG: return effects.chorus.log_scale_frequencies.get_default_ratio();
        case ParamId::EELOG: return effects.echo.log_scale_frequencies.get_default_ratio();
        case ParamId::ERLOG: return effects.reverb.log_scale_frequencies.get_default_ratio();
        case ParamId::N1DYN: return envelopes_rw[0]->dynamic.get_default_ratio();
        case ParamId::N2DYN: return envelopes_rw[1]->dynamic.get_default_ratio();
        case ParamId::N3DYN: return envelopes_rw[2]->dynamic.get_default_ratio();
        case ParamId::N4DYN: return envelopes_rw[3]->dynamic.get_default_ratio();
        case ParamId::N5DYN: return envelopes_rw[4]->dynamic.get_default_ratio();
        case ParamId::N6DYN: return envelopes_rw[5]->dynamic.get_default_ratio();
        default: return 0.0; // This should neacver be reached.
    }
}


bool Synth::is_toggle_param(ParamId const param_id) const noexcept
{
    return param_id >= ParamId::L1SYN && param_id < ParamId::MAX_PARAM_ID;
}


Number Synth::get_param_max_value(ParamId const param_id) const noexcept
{
    if (param_id < FLOAT_PARAMS) {
        return float_params[param_id]->get_max_value();
    }

    switch (param_id) {
        case ParamId::MODE: return mode.get_max_value();
        case ParamId::MWAV: return modulator_params.waveform.get_max_value();
        case ParamId::CWAV: return carrier_params.waveform.get_max_value();
        case ParamId::MF1TYP: return modulator_params.filter_1_type.get_max_value();
        case ParamId::MF2TYP: return modulator_params.filter_2_type.get_max_value();
        case ParamId::CF1TYP: return carrier_params.filter_1_type.get_max_value();
        case ParamId::CF2TYP: return carrier_params.filter_2_type.get_max_value();
        case ParamId::EF1TYP: return effects.filter_1_type.get_max_value();
        case ParamId::EF2TYP: return effects.filter_2_type.get_max_value();
        case ParamId::L1WAV: return lfos_rw[0]->waveform.get_max_value();
        case ParamId::L2WAV: return lfos_rw[1]->waveform.get_max_value();
        case ParamId::L3WAV: return lfos_rw[2]->waveform.get_max_value();
        case ParamId::L4WAV: return lfos_rw[3]->waveform.get_max_value();
        case ParamId::L5WAV: return lfos_rw[4]->waveform.get_max_value();
        case ParamId::L6WAV: return lfos_rw[5]->waveform.get_max_value();
        case ParamId::L7WAV: return lfos_rw[6]->waveform.get_max_value();
        case ParamId::L8WAV: return lfos_rw[7]->waveform.get_max_value();
        case ParamId::L1CEN: return lfos_rw[0]->center.get_max_value();
        case ParamId::L2CEN: return lfos_rw[1]->center.get_max_value();
        case ParamId::L3CEN: return lfos_rw[2]->center.get_max_value();
        case ParamId::L4CEN: return lfos_rw[3]->center.get_max_value();
        case ParamId::L5CEN: return lfos_rw[4]->center.get_max_value();
        case ParamId::L6CEN: return lfos_rw[5]->center.get_max_value();
        case ParamId::L7CEN: return lfos_rw[6]->center.get_max_value();
        case ParamId::L8CEN: return lfos_rw[7]->center.get_max_value();
        case ParamId::L1SYN: return lfos_rw[0]->tempo_sync.get_max_value();
        case ParamId::L2SYN: return lfos_rw[1]->tempo_sync.get_max_value();
        case ParamId::L3SYN: return lfos_rw[2]->tempo_sync.get_max_value();
        case ParamId::L4SYN: return lfos_rw[3]->tempo_sync.get_max_value();
        case ParamId::L5SYN: return lfos_rw[4]->tempo_sync.get_max_value();
        case ParamId::L6SYN: return lfos_rw[5]->tempo_sync.get_max_value();
        case ParamId::L7SYN: return lfos_rw[6]->tempo_sync.get_max_value();
        case ParamId::L8SYN: return lfos_rw[7]->tempo_sync.get_max_value();
        case ParamId::ECSYN: return effects.chorus.tempo_sync.get_max_value();
        case ParamId::EESYN: return effects.echo.tempo_sync.get_max_value();
        case ParamId::MF1LOG: return modulator_params.filter_1_log_scale.get_max_value();
        case ParamId::MF2LOG: return modulator_params.filter_2_log_scale.get_max_value();
        case ParamId::CF1LOG: return carrier_params.filter_1_log_scale.get_max_value();
        case ParamId::CF2LOG: return carrier_params.filter_2_log_scale.get_max_value();
        case ParamId::EF1LOG: return effects.filter_1_log_scale.get_max_value();
        case ParamId::EF2LOG: return effects.filter_2_log_scale.get_max_value();
        case ParamId::ECLOG: return effects.chorus.log_scale_frequencies.get_max_value();
        case ParamId::EELOG: return effects.echo.log_scale_frequencies.get_max_value();
        case ParamId::ERLOG: return effects.reverb.log_scale_frequencies.get_max_value();
        case ParamId::N1DYN: return envelopes_rw[0]->dynamic.get_max_value();
        case ParamId::N2DYN: return envelopes_rw[1]->dynamic.get_max_value();
        case ParamId::N3DYN: return envelopes_rw[2]->dynamic.get_max_value();
        case ParamId::N4DYN: return envelopes_rw[3]->dynamic.get_max_value();
        case ParamId::N5DYN: return envelopes_rw[4]->dynamic.get_max_value();
        case ParamId::N6DYN: return envelopes_rw[5]->dynamic.get_max_value();
        default: return 0.0; // This should neacver be reached.
    }
}


Number Synth::float_param_ratio_to_display_value(
        ParamId const param_id,
        Number const ratio
) const noexcept {
    if (param_id < FLOAT_PARAMS) {
        return float_params[param_id]->ratio_to_value(ratio);
    }

    return 0.0;
}


Byte Synth::int_param_ratio_to_display_value(
        ParamId const param_id,
        Number const ratio
) const noexcept {
    switch (param_id) {
        case ParamId::MODE: return mode.ratio_to_value(ratio);
        case ParamId::MWAV: return modulator_params.waveform.ratio_to_value(ratio);
        case ParamId::CWAV: return carrier_params.waveform.ratio_to_value(ratio);
        case ParamId::MF1TYP: return modulator_params.filter_1_type.ratio_to_value(ratio);
        case ParamId::MF2TYP: return modulator_params.filter_2_type.ratio_to_value(ratio);
        case ParamId::CF1TYP: return carrier_params.filter_1_type.ratio_to_value(ratio);
        case ParamId::CF2TYP: return carrier_params.filter_2_type.ratio_to_value(ratio);
        case ParamId::EF1TYP: return effects.filter_1_type.ratio_to_value(ratio);
        case ParamId::EF2TYP: return effects.filter_2_type.ratio_to_value(ratio);
        case ParamId::L1WAV: return lfos_rw[0]->waveform.ratio_to_value(ratio);
        case ParamId::L2WAV: return lfos_rw[1]->waveform.ratio_to_value(ratio);
        case ParamId::L3WAV: return lfos_rw[2]->waveform.ratio_to_value(ratio);
        case ParamId::L4WAV: return lfos_rw[3]->waveform.ratio_to_value(ratio);
        case ParamId::L5WAV: return lfos_rw[4]->waveform.ratio_to_value(ratio);
        case ParamId::L6WAV: return lfos_rw[5]->waveform.ratio_to_value(ratio);
        case ParamId::L7WAV: return lfos_rw[6]->waveform.ratio_to_value(ratio);
        case ParamId::L8WAV: return lfos_rw[7]->waveform.ratio_to_value(ratio);
        case ParamId::L1CEN: return lfos_rw[0]->center.ratio_to_value(ratio);
        case ParamId::L2CEN: return lfos_rw[1]->center.ratio_to_value(ratio);
        case ParamId::L3CEN: return lfos_rw[2]->center.ratio_to_value(ratio);
        case ParamId::L4CEN: return lfos_rw[3]->center.ratio_to_value(ratio);
        case ParamId::L5CEN: return lfos_rw[4]->center.ratio_to_value(ratio);
        case ParamId::L6CEN: return lfos_rw[5]->center.ratio_to_value(ratio);
        case ParamId::L7CEN: return lfos_rw[6]->center.ratio_to_value(ratio);
        case ParamId::L8CEN: return lfos_rw[7]->center.ratio_to_value(ratio);
        case ParamId::L1SYN: return lfos_rw[0]->tempo_sync.ratio_to_value(ratio);
        case ParamId::L2SYN: return lfos_rw[1]->tempo_sync.ratio_to_value(ratio);
        case ParamId::L3SYN: return lfos_rw[2]->tempo_sync.ratio_to_value(ratio);
        case ParamId::L4SYN: return lfos_rw[3]->tempo_sync.ratio_to_value(ratio);
        case ParamId::L5SYN: return lfos_rw[4]->tempo_sync.ratio_to_value(ratio);
        case ParamId::L6SYN: return lfos_rw[5]->tempo_sync.ratio_to_value(ratio);
        case ParamId::L7SYN: return lfos_rw[6]->tempo_sync.ratio_to_value(ratio);
        case ParamId::L8SYN: return lfos_rw[7]->tempo_sync.ratio_to_value(ratio);
        case ParamId::ECSYN: return effects.chorus.tempo_sync.ratio_to_value(ratio);
        case ParamId::EESYN: return effects.echo.tempo_sync.ratio_to_value(ratio);
        case ParamId::MF1LOG: return modulator_params.filter_1_log_scale.ratio_to_value(ratio);
        case ParamId::MF2LOG: return modulator_params.filter_2_log_scale.ratio_to_value(ratio);
        case ParamId::CF1LOG: return carrier_params.filter_1_log_scale.ratio_to_value(ratio);
        case ParamId::CF2LOG: return carrier_params.filter_2_log_scale.ratio_to_value(ratio);
        case ParamId::EF1LOG: return effects.filter_1_log_scale.ratio_to_value(ratio);
        case ParamId::EF2LOG: return effects.filter_2_log_scale.ratio_to_value(ratio);
        case ParamId::ECLOG: return effects.chorus.log_scale_frequencies.ratio_to_value(ratio);
        case ParamId::EELOG: return effects.echo.log_scale_frequencies.ratio_to_value(ratio);
        case ParamId::ERLOG: return effects.reverb.log_scale_frequencies.ratio_to_value(ratio);
        case ParamId::N1DYN: return envelopes_rw[0]->dynamic.ratio_to_value(ratio);
        case ParamId::N2DYN: return envelopes_rw[1]->dynamic.ratio_to_value(ratio);
        case ParamId::N3DYN: return envelopes_rw[2]->dynamic.ratio_to_value(ratio);
        case ParamId::N4DYN: return envelopes_rw[3]->dynamic.ratio_to_value(ratio);
        case ParamId::N5DYN: return envelopes_rw[4]->dynamic.ratio_to_value(ratio);
        case ParamId::N6DYN: return envelopes_rw[5]->dynamic.ratio_to_value(ratio);
        default: return 0; // This should never be reached.
    }
}


Synth::ControllerId Synth::get_param_controller_id_atomic(
        ParamId const param_id
) const noexcept {
    return (ControllerId)controller_assignments[param_id].load();
}


void Synth::update_param_states() noexcept
{
    for (int i = 0; i != ParamId::MAX_PARAM_ID; ++i) {
        handle_refresh_param((ParamId)i);
    }
}


Sample const* const* Synth::initialize_rendering(
        Integer const round,
        Integer const sample_count
) noexcept {
    process_messages();
    garbage_collect_voices();

    raw_output = SignalProducer::produce< Effects::Effects<Bus> >(
        effects, round, sample_count
    );

    for (int i = 0; i != FLOAT_PARAMS; ++i) {
        FloatParam::produce_if_not_constant(*float_params[i], round, sample_count);
    }

    for (Integer i = 0; i != LFOS; ++i) {
        lfos_rw[i]->skip_round(round, sample_count);
    }

    effects.chorus.lfo_1.skip_round(round, sample_count);
    effects.chorus.lfo_2.skip_round(round, sample_count);
    effects.chorus.lfo_3.skip_round(round, sample_count);

    clear_midi_controllers();

    return NULL;
}


void Synth::garbage_collect_voices() noexcept
{
    for (Integer v = 0; v != POLYPHONY; ++v) {
        Midi::Channel channel;
        Midi::Note note;

        Modulator* const modulator = modulators[v];
        bool const modulator_decayed = modulator->has_decayed_during_envelope_dahds();

        if (modulator_decayed) {
            note = modulator->get_note();
            channel = modulator->get_channel();
            modulator->reset();
        }

        Carrier* const carrier = carriers[v];
        bool const carrier_decayed = carrier->has_decayed_during_envelope_dahds();

        if (carrier_decayed) {
            note = carrier->get_note();
            channel = carrier->get_channel();
            carrier->reset();
        }

        if (modulator_decayed && carrier_decayed) {
            midi_note_to_voice_assignments[channel][note] = INVALID_VOICE;
        }
    }
}


void Synth::process_messages() noexcept
{
    size_t const message_count = messages.size();

    for (size_t i = 0; i != message_count; ++i) {
        Message message;

        if (!messages.pop(message)) {
            continue;
        }

        switch (message.type) {
            case MessageType::SET_PARAM:
                handle_set_param(message.param_id, message.number_param);
                break;

            case MessageType::ASSIGN_CONTROLLER:
                handle_assign_controller(message.param_id, message.byte_param);
                break;

            case MessageType::REFRESH_PARAM:
                handle_refresh_param(message.param_id);
                break;

            case MessageType::CLEAR:
                handle_clear();
                break;

            default:
                break;
        }
    }
}


void Synth::handle_set_param(ParamId const param_id, Number const ratio) noexcept
{
    if (param_id < FLOAT_PARAMS) {
        float_params[param_id]->set_ratio(ratio);
    } else {
        switch (param_id) {
            case ParamId::MODE: mode.set_ratio(ratio); break;
            case ParamId::MWAV: modulator_params.waveform.set_ratio(ratio); break;
            case ParamId::CWAV: carrier_params.waveform.set_ratio(ratio); break;
            case ParamId::MF1TYP: modulator_params.filter_1_type.set_ratio(ratio); break;
            case ParamId::MF2TYP: modulator_params.filter_2_type.set_ratio(ratio); break;
            case ParamId::CF1TYP: carrier_params.filter_1_type.set_ratio(ratio); break;
            case ParamId::CF2TYP: carrier_params.filter_2_type.set_ratio(ratio); break;
            case ParamId::EF1TYP: effects.filter_1_type.set_ratio(ratio); break;
            case ParamId::EF2TYP: effects.filter_2_type.set_ratio(ratio); break;
            case ParamId::L1WAV: lfos_rw[0]->waveform.set_ratio(ratio); break;
            case ParamId::L2WAV: lfos_rw[1]->waveform.set_ratio(ratio); break;
            case ParamId::L3WAV: lfos_rw[2]->waveform.set_ratio(ratio); break;
            case ParamId::L4WAV: lfos_rw[3]->waveform.set_ratio(ratio); break;
            case ParamId::L5WAV: lfos_rw[4]->waveform.set_ratio(ratio); break;
            case ParamId::L6WAV: lfos_rw[5]->waveform.set_ratio(ratio); break;
            case ParamId::L7WAV: lfos_rw[6]->waveform.set_ratio(ratio); break;
            case ParamId::L8WAV: lfos_rw[7]->waveform.set_ratio(ratio); break;
            case ParamId::L1SYN: lfos_rw[0]->tempo_sync.set_ratio(ratio); break;
            case ParamId::L2SYN: lfos_rw[1]->tempo_sync.set_ratio(ratio); break;
            case ParamId::L3SYN: lfos_rw[2]->tempo_sync.set_ratio(ratio); break;
            case ParamId::L4SYN: lfos_rw[3]->tempo_sync.set_ratio(ratio); break;
            case ParamId::L5SYN: lfos_rw[4]->tempo_sync.set_ratio(ratio); break;
            case ParamId::L6SYN: lfos_rw[5]->tempo_sync.set_ratio(ratio); break;
            case ParamId::L7SYN: lfos_rw[6]->tempo_sync.set_ratio(ratio); break;
            case ParamId::L8SYN: lfos_rw[7]->tempo_sync.set_ratio(ratio); break;
            case ParamId::L1CEN: lfos_rw[0]->center.set_ratio(ratio); break;
            case ParamId::L2CEN: lfos_rw[1]->center.set_ratio(ratio); break;
            case ParamId::L3CEN: lfos_rw[2]->center.set_ratio(ratio); break;
            case ParamId::L4CEN: lfos_rw[3]->center.set_ratio(ratio); break;
            case ParamId::L5CEN: lfos_rw[4]->center.set_ratio(ratio); break;
            case ParamId::L6CEN: lfos_rw[5]->center.set_ratio(ratio); break;
            case ParamId::L7CEN: lfos_rw[6]->center.set_ratio(ratio); break;
            case ParamId::L8CEN: lfos_rw[7]->center.set_ratio(ratio); break;
            case ParamId::ECSYN: effects.chorus.tempo_sync.set_ratio(ratio); break;
            case ParamId::EESYN: effects.echo.tempo_sync.set_ratio(ratio); break;
            case ParamId::MF1LOG: modulator_params.filter_1_log_scale.set_ratio(ratio); break;
            case ParamId::MF2LOG: modulator_params.filter_2_log_scale.set_ratio(ratio); break;
            case ParamId::CF1LOG: carrier_params.filter_1_log_scale.set_ratio(ratio); break;
            case ParamId::CF2LOG: carrier_params.filter_2_log_scale.set_ratio(ratio); break;
            case ParamId::EF1LOG: effects.filter_1_log_scale.set_ratio(ratio); break;
            case ParamId::EF2LOG: effects.filter_2_log_scale.set_ratio(ratio); break;
            case ParamId::ECLOG: effects.chorus.log_scale_frequencies.set_ratio(ratio); break;
            case ParamId::EELOG: effects.echo.log_scale_frequencies.set_ratio(ratio); break;
            case ParamId::ERLOG: effects.reverb.log_scale_frequencies.set_ratio(ratio); break;
            case ParamId::N1DYN: envelopes_rw[0]->dynamic.set_ratio(ratio); break;
            case ParamId::N2DYN: envelopes_rw[1]->dynamic.set_ratio(ratio); break;
            case ParamId::N3DYN: envelopes_rw[2]->dynamic.set_ratio(ratio); break;
            case ParamId::N4DYN: envelopes_rw[3]->dynamic.set_ratio(ratio); break;
            case ParamId::N5DYN: envelopes_rw[4]->dynamic.set_ratio(ratio); break;
            case ParamId::N6DYN: envelopes_rw[5]->dynamic.set_ratio(ratio); break;
            default: break; // This should never be reached.
        }
    }

    handle_refresh_param(param_id);
}


void Synth::handle_assign_controller(
        ParamId const param_id,
        Byte const controller_id
) noexcept {
    bool is_assigned = false;

    if (param_id < FLOAT_PARAMS) {
        is_assigned = assign_controller_to_float_param(
            param_id, (ControllerId)controller_id
        );
    } else {
        is_assigned = assign_controller_to_param(param_id, (ControllerId)controller_id);
    }

    if (!is_assigned) {
        return;
    }

    controller_assignments[param_id].store(controller_id);

    if ((ControllerId)controller_id == ControllerId::MIDI_LEARN) {
        is_learning = true;
    }
}


void Synth::handle_refresh_param(ParamId const param_id) noexcept
{
    param_ratios[param_id].store(get_param_ratio(param_id));
}


void Synth::handle_clear() noexcept
{
    constexpr Byte no_controller = (Byte)ControllerId::NONE;

    reset();
    start_lfos();

    clear_midi_note_to_voice_assignments();
    clear_sustain();

    for (int i = 0; i != ParamId::MAX_PARAM_ID; ++i) {
        ParamId const param_id = (ParamId)i;

        handle_assign_controller(param_id, no_controller);
        handle_set_param(param_id, get_param_default_ratio(param_id));
    }
}


bool Synth::assign_controller_to_param(
        ParamId const param_id,
        ControllerId const controller_id
) noexcept {
    MidiController* midi_controller = NULL;
    bool is_assigned = false;
    bool is_special = false;

    switch (controller_id) {
        case NONE: is_special = true; break;

        case PITCH_WHEEL: midi_controller = &pitch_wheel; break;
        case NOTE: midi_controller = &note; break;
        case VELOCITY: midi_controller = &velocity; break;

        case FLEXIBLE_CONTROLLER_1:
        case FLEXIBLE_CONTROLLER_2:
        case FLEXIBLE_CONTROLLER_3:
        case FLEXIBLE_CONTROLLER_4:
        case FLEXIBLE_CONTROLLER_5:
        case FLEXIBLE_CONTROLLER_6:
        case FLEXIBLE_CONTROLLER_7:
        case FLEXIBLE_CONTROLLER_8:
        case FLEXIBLE_CONTROLLER_9:
        case FLEXIBLE_CONTROLLER_10:
        case FLEXIBLE_CONTROLLER_11:
        case FLEXIBLE_CONTROLLER_12:
        case FLEXIBLE_CONTROLLER_13:
        case FLEXIBLE_CONTROLLER_14:
        case FLEXIBLE_CONTROLLER_15:
        case FLEXIBLE_CONTROLLER_16:
        case FLEXIBLE_CONTROLLER_17:
        case FLEXIBLE_CONTROLLER_18:
        case FLEXIBLE_CONTROLLER_19:
        case LFO_1:
        case LFO_2:
        case LFO_3:
        case LFO_4:
        case LFO_5:
        case LFO_6:
        case LFO_7:
        case LFO_8:
        case ENVELOPE_1:
        case ENVELOPE_2:
        case ENVELOPE_3:
        case ENVELOPE_4:
        case ENVELOPE_5:
        case ENVELOPE_6:
            break;

        case CHANNEL_PRESSURE: break;

        case MIDI_LEARN: is_special = true; break;

        default: {
            if (is_supported_midi_controller(controller_id)) {
                midi_controller = midi_controllers[controller_id];
            }

            break;
        }
    }

    switch (param_id) {
        case ParamId::MODE: mode.set_midi_controller(midi_controller); is_assigned = true; break;
        case ParamId::MWAV: modulator_params.waveform.set_midi_controller(midi_controller); is_assigned = true; break;
        case ParamId::CWAV: carrier_params.waveform.set_midi_controller(midi_controller); is_assigned = true; break;
        case ParamId::MF1TYP: modulator_params.filter_1_type.set_midi_controller(midi_controller); is_assigned = true; break;
        case ParamId::MF2TYP: modulator_params.filter_2_type.set_midi_controller(midi_controller); is_assigned = true; break;
        case ParamId::CF1TYP: carrier_params.filter_1_type.set_midi_controller(midi_controller); is_assigned = true; break;
        case ParamId::CF2TYP: carrier_params.filter_2_type.set_midi_controller(midi_controller); is_assigned = true; break;
        case ParamId::EF1TYP: effects.filter_1_type.set_midi_controller(midi_controller); is_assigned = true; break;
        case ParamId::EF2TYP: effects.filter_2_type.set_midi_controller(midi_controller); is_assigned = true; break;
        case ParamId::L1WAV: lfos_rw[0]->waveform.set_midi_controller(midi_controller); is_assigned = true; break;
        case ParamId::L2WAV: lfos_rw[1]->waveform.set_midi_controller(midi_controller); is_assigned = true; break;
        case ParamId::L3WAV: lfos_rw[2]->waveform.set_midi_controller(midi_controller); is_assigned = true; break;
        case ParamId::L4WAV: lfos_rw[3]->waveform.set_midi_controller(midi_controller); is_assigned = true; break;
        case ParamId::L5WAV: lfos_rw[4]->waveform.set_midi_controller(midi_controller); is_assigned = true; break;
        case ParamId::L6WAV: lfos_rw[5]->waveform.set_midi_controller(midi_controller); is_assigned = true; break;
        case ParamId::L7WAV: lfos_rw[6]->waveform.set_midi_controller(midi_controller); is_assigned = true; break;
        case ParamId::L8WAV: lfos_rw[7]->waveform.set_midi_controller(midi_controller); is_assigned = true; break;
        default: break;
    }

    return is_assigned && (is_special || midi_controller != NULL);
}


bool Synth::assign_controller_to_float_param(
        ParamId const param_id,
        ControllerId const controller_id
) noexcept {
    FloatParam* param = float_params[param_id];

    param->set_midi_controller(NULL);
    param->set_flexible_controller(NULL);
    param->set_envelope(NULL);
    param->set_lfo(NULL);

    switch (controller_id) {
        case NONE: return true;

        case PITCH_WHEEL: param->set_midi_controller(&pitch_wheel); return true;
        case NOTE: param->set_midi_controller(&note); return true;
        case VELOCITY: param->set_midi_controller(&velocity); return true;

        case FLEXIBLE_CONTROLLER_1: param->set_flexible_controller(flexible_controllers[0]); return true;
        case FLEXIBLE_CONTROLLER_2: param->set_flexible_controller(flexible_controllers[1]); return true;
        case FLEXIBLE_CONTROLLER_3: param->set_flexible_controller(flexible_controllers[2]); return true;
        case FLEXIBLE_CONTROLLER_4: param->set_flexible_controller(flexible_controllers[3]); return true;
        case FLEXIBLE_CONTROLLER_5: param->set_flexible_controller(flexible_controllers[4]); return true;
        case FLEXIBLE_CONTROLLER_6: param->set_flexible_controller(flexible_controllers[5]); return true;
        case FLEXIBLE_CONTROLLER_7: param->set_flexible_controller(flexible_controllers[6]); return true;
        case FLEXIBLE_CONTROLLER_8: param->set_flexible_controller(flexible_controllers[7]); return true;
        case FLEXIBLE_CONTROLLER_9: param->set_flexible_controller(flexible_controllers[8]); return true;
        case FLEXIBLE_CONTROLLER_10: param->set_flexible_controller(flexible_controllers[9]); return true;
        case FLEXIBLE_CONTROLLER_11: param->set_flexible_controller(flexible_controllers[10]); return true;
        case FLEXIBLE_CONTROLLER_12: param->set_flexible_controller(flexible_controllers[11]); return true;
        case FLEXIBLE_CONTROLLER_13: param->set_flexible_controller(flexible_controllers[12]); return true;
        case FLEXIBLE_CONTROLLER_14: param->set_flexible_controller(flexible_controllers[13]); return true;
        case FLEXIBLE_CONTROLLER_15: param->set_flexible_controller(flexible_controllers[14]); return true;
        case FLEXIBLE_CONTROLLER_16: param->set_flexible_controller(flexible_controllers[15]); return true;
        case FLEXIBLE_CONTROLLER_17: param->set_flexible_controller(flexible_controllers[16]); return true;
        case FLEXIBLE_CONTROLLER_18: param->set_flexible_controller(flexible_controllers[17]); return true;
        case FLEXIBLE_CONTROLLER_19: param->set_flexible_controller(flexible_controllers[18]); return true;
        case FLEXIBLE_CONTROLLER_20: param->set_flexible_controller(flexible_controllers[19]); return true;

        case LFO_1: param->set_lfo(lfos_rw[0]); return true;
        case LFO_2: param->set_lfo(lfos_rw[1]); return true;
        case LFO_3: param->set_lfo(lfos_rw[2]); return true;
        case LFO_4: param->set_lfo(lfos_rw[3]); return true;
        case LFO_5: param->set_lfo(lfos_rw[4]); return true;
        case LFO_6: param->set_lfo(lfos_rw[5]); return true;
        case LFO_7: param->set_lfo(lfos_rw[6]); return true;
        case LFO_8: param->set_lfo(lfos_rw[7]); return true;

        case ENVELOPE_1: param->set_envelope(envelopes[0]); return true;
        case ENVELOPE_2: param->set_envelope(envelopes[1]); return true;
        case ENVELOPE_3: param->set_envelope(envelopes[2]); return true;
        case ENVELOPE_4: param->set_envelope(envelopes[3]); return true;
        case ENVELOPE_5: param->set_envelope(envelopes[4]); return true;
        case ENVELOPE_6: param->set_envelope(envelopes[5]); return true;

        case CHANNEL_PRESSURE: param->set_midi_controller(&channel_pressure_ctl); return true;

        case MIDI_LEARN: return true;

        default: {
            if (is_supported_midi_controller(controller_id)) {
                param->set_midi_controller(midi_controllers[controller_id]);

                return true;
            }

            break;
        }
    }

    return false;
}


Number Synth::get_param_ratio(ParamId const param_id) const noexcept
{
    if (param_id < FLOAT_PARAMS) {
        return float_params[param_id]->get_ratio();
    }

    switch (param_id) {
        case ParamId::MODE: return mode.get_ratio();
        case ParamId::MWAV: return modulator_params.waveform.get_ratio();
        case ParamId::CWAV: return carrier_params.waveform.get_ratio();
        case ParamId::MF1TYP: return modulator_params.filter_1_type.get_ratio();
        case ParamId::MF2TYP: return modulator_params.filter_2_type.get_ratio();
        case ParamId::CF1TYP: return carrier_params.filter_1_type.get_ratio();
        case ParamId::CF2TYP: return carrier_params.filter_2_type.get_ratio();
        case ParamId::EF1TYP: return effects.filter_1_type.get_ratio();
        case ParamId::EF2TYP: return effects.filter_2_type.get_ratio();
        case ParamId::L1WAV: return lfos_rw[0]->waveform.get_ratio();
        case ParamId::L2WAV: return lfos_rw[1]->waveform.get_ratio();
        case ParamId::L3WAV: return lfos_rw[2]->waveform.get_ratio();
        case ParamId::L4WAV: return lfos_rw[3]->waveform.get_ratio();
        case ParamId::L5WAV: return lfos_rw[4]->waveform.get_ratio();
        case ParamId::L6WAV: return lfos_rw[5]->waveform.get_ratio();
        case ParamId::L7WAV: return lfos_rw[6]->waveform.get_ratio();
        case ParamId::L8WAV: return lfos_rw[7]->waveform.get_ratio();
        case ParamId::L1CEN: return lfos_rw[0]->center.get_ratio();
        case ParamId::L2CEN: return lfos_rw[1]->center.get_ratio();
        case ParamId::L3CEN: return lfos_rw[2]->center.get_ratio();
        case ParamId::L4CEN: return lfos_rw[3]->center.get_ratio();
        case ParamId::L5CEN: return lfos_rw[4]->center.get_ratio();
        case ParamId::L6CEN: return lfos_rw[5]->center.get_ratio();
        case ParamId::L7CEN: return lfos_rw[6]->center.get_ratio();
        case ParamId::L8CEN: return lfos_rw[7]->center.get_ratio();
        case ParamId::L1SYN: return lfos_rw[0]->tempo_sync.get_ratio();
        case ParamId::L2SYN: return lfos_rw[1]->tempo_sync.get_ratio();
        case ParamId::L3SYN: return lfos_rw[2]->tempo_sync.get_ratio();
        case ParamId::L4SYN: return lfos_rw[3]->tempo_sync.get_ratio();
        case ParamId::L5SYN: return lfos_rw[4]->tempo_sync.get_ratio();
        case ParamId::L6SYN: return lfos_rw[5]->tempo_sync.get_ratio();
        case ParamId::L7SYN: return lfos_rw[6]->tempo_sync.get_ratio();
        case ParamId::L8SYN: return lfos_rw[7]->tempo_sync.get_ratio();
        case ParamId::ECSYN: return effects.chorus.tempo_sync.get_ratio();
        case ParamId::EESYN: return effects.echo.tempo_sync.get_ratio();
        case ParamId::MF1LOG: return modulator_params.filter_1_log_scale.get_ratio();
        case ParamId::MF2LOG: return modulator_params.filter_2_log_scale.get_ratio();
        case ParamId::CF1LOG: return carrier_params.filter_1_log_scale.get_ratio();
        case ParamId::CF2LOG: return carrier_params.filter_2_log_scale.get_ratio();
        case ParamId::EF1LOG: return effects.filter_1_log_scale.get_ratio();
        case ParamId::EF2LOG: return effects.filter_2_log_scale.get_ratio();
        case ParamId::ECLOG: return effects.chorus.log_scale_frequencies.get_ratio();
        case ParamId::EELOG: return effects.echo.log_scale_frequencies.get_ratio();
        case ParamId::ERLOG: return effects.reverb.log_scale_frequencies.get_ratio();
        case ParamId::N1DYN: return envelopes_rw[0]->dynamic.get_ratio();
        case ParamId::N2DYN: return envelopes_rw[1]->dynamic.get_ratio();
        case ParamId::N3DYN: return envelopes_rw[2]->dynamic.get_ratio();
        case ParamId::N4DYN: return envelopes_rw[3]->dynamic.get_ratio();
        case ParamId::N5DYN: return envelopes_rw[4]->dynamic.get_ratio();
        case ParamId::N6DYN: return envelopes_rw[5]->dynamic.get_ratio();
        default: return 0.0; // This should never be reached.
    }
}


void Synth::clear_midi_controllers() noexcept
{
    pitch_wheel.clear();
    note.clear();
    velocity.clear();
    channel_pressure_ctl.clear();

    for (Integer i = 0; i != MIDI_CONTROLLERS; ++i) {
        if (midi_controllers[i] != NULL) {
            midi_controllers[i]->clear();
        }
    }
}


void Synth::clear_midi_note_to_voice_assignments() noexcept
{
    for (Midi::Channel channel = 0; channel != Midi::CHANNELS; ++channel) {
        for (Midi::Note note = 0; note != Midi::NOTES; ++note) {
            midi_note_to_voice_assignments[channel][note] = INVALID_VOICE;
        }
    }
}


void Synth::clear_sustain() noexcept
{
    is_sustaining = false;
    delayed_note_offs.clear();
}


void Synth::render(
        Integer const round,
        Integer const first_sample_index,
        Integer const last_sample_index,
        Sample** buffer
) noexcept {
    for (Integer c = 0; c != channels; ++c) {
        Sample* const out = buffer[c];
        Sample const* const raw = raw_output[c];

        for (Integer i = first_sample_index; i != last_sample_index; ++i) {
            /*
            Normal synth configurations should stay below 0 dB; if we happen to
            produce samples way outside the [-1, +1] range, it most probably
            means filter misconfiguration, which is likely to blow up even
            further. Some plugin hosts are known to automatically turn off
            plugins which produce extremely loud signals, so in order to avoid
            that happening to us, we are forcing digital clipping here at
            around 9 dB.
            */
            out[i] = std::min(2.8, std::max(-2.8, raw[i]));
        }
    }
}


std::string const Synth::to_string(Integer const n) const noexcept
{
    std::ostringstream s;

    s << n;

    return s.str();
}


Synth::Message::Message() noexcept
    : type(INVALID),
    param_id(ParamId::MAX_PARAM_ID),
    number_param(0.0),
    byte_param(0)
{
}


Synth::Message::Message(Message const& message) noexcept
    : type(message.type),
    param_id(message.param_id),
    number_param(message.number_param),
    byte_param(message.byte_param)
{
}


Synth::Message::Message(
        MessageType const type,
        ParamId const param_id,
        Number const number_param,
        Byte const byte_param
) noexcept
    : type(type),
    param_id(param_id),
    number_param(number_param),
    byte_param(byte_param)
{
}


Synth::Message& Synth::Message::operator=(Message const& message) noexcept
{
    if (this != &message) {
        type = message.type;
        param_id = message.param_id;
        number_param = message.number_param;
        byte_param = message.byte_param;
    }

    return *this;
}


Synth::Message& Synth::Message::operator=(Message const&& message) noexcept
{
    if (this != &message) {
        type = message.type;
        param_id = message.param_id;
        number_param = message.number_param;
        byte_param = message.byte_param;
    }

    return *this;
}


Synth::SingleProducerSingleConsumerMessageQueue::SingleProducerSingleConsumerMessageQueue() noexcept
    : next_push(0),
    next_pop(0)
{
}


bool Synth::SingleProducerSingleConsumerMessageQueue::is_lock_free() const noexcept
{
    return next_push.is_lock_free() && next_pop.is_lock_free();
}


bool Synth::SingleProducerSingleConsumerMessageQueue::push(
        Synth::Message const& message
) noexcept {
    size_t const old_next_push = next_push.load();
    size_t const next_pop = this->next_pop.load();
    size_t const new_next_push = advance(old_next_push);

    if (next_pop == new_next_push) {
        return false;
    }

    messages[old_next_push] = message;
    next_push.store(new_next_push);

    return true;
}


bool Synth::SingleProducerSingleConsumerMessageQueue::pop(Message& message) noexcept
{
    size_t const next_pop = this->next_pop.load();
    size_t const next_push = this->next_push.load();

    if (next_push == next_pop) {
        return false;
    }

    message = std::move(messages[next_pop]);

    this->next_pop.store(advance(next_pop));

    return true;
}


size_t Synth::SingleProducerSingleConsumerMessageQueue::size() const noexcept
{
    size_t const next_pop = this->next_pop.load();
    size_t const next_push = this->next_push.load();

    if (next_push < next_pop) {
        return SIZE + next_push - next_pop;
    } else {
        return next_push - next_pop;
    }
}


size_t Synth::SingleProducerSingleConsumerMessageQueue::advance(
        size_t const index
) const noexcept {
    return (index + 1) & SIZE_MASK;
}


Synth::Bus::Bus(
        Integer const channels,
        Modulator* const* const modulators,
        Carrier* const* const carriers,
        Integer const polyphony,
        FloatParam& modulator_add_volume
) noexcept
    : SignalProducer(channels, 0),
    polyphony(polyphony),
    modulators(modulators),
    carriers(carriers),
    modulator_add_volume(modulator_add_volume),
    modulators_on(POLYPHONY),
    carriers_on(POLYPHONY)
{
}


Sample const* const* Synth::Bus::initialize_rendering(
        Integer const round,
        Integer const sample_count
) noexcept {
    is_silent = true;

    for (Integer v = 0; v != polyphony; ++v) {
        modulators_on[v] = modulators[v]->is_on();

        if (modulators_on[v]) {
            is_silent = false;
            SignalProducer::produce<Modulator>(
                *modulators[v], round, sample_count
            );
        }

        carriers_on[v] = carriers[v]->is_on();

        if (carriers_on[v]) {
            is_silent = false;
            SignalProducer::produce<Carrier>(*carriers[v], round, sample_count);
        }
    }

    if (is_silent) {
        return NULL;
    }

    modulator_add_volume_buffer = FloatParam::produce_if_not_constant(
        modulator_add_volume, round, sample_count
    );

    return NULL;
}


void Synth::Bus::render(
        Integer const round,
        Integer const first_sample_index,
        Integer const last_sample_index,
        Sample** buffer
) noexcept {
    render_silence(round, first_sample_index, last_sample_index, buffer);

    if (is_silent) {
        return;
    }

    mix_modulators(round, first_sample_index, last_sample_index, buffer);
    mix_carriers(round, first_sample_index, last_sample_index, buffer);
}


void Synth::Bus::mix_modulators(
        Integer const round,
        Integer const first_sample_index,
        Integer const last_sample_index,
        Sample** buffer
) const noexcept {
    Sample const* const modulator_add_volume_buffer = (
        this->modulator_add_volume_buffer
    );

    if (modulator_add_volume_buffer == NULL) {
        Sample const modulator_add_volume_value = (
            modulator_add_volume.get_value()
        );

        if (modulator_add_volume_value <= 0.000001) {
            return;
        }

        for (Integer v = 0; v != polyphony; ++v) {
            if (!modulators_on[v]) {
                continue;
            }

            Sample const* const* const modulator_output = (
                SignalProducer::produce<Modulator>(*modulators[v], round)
            );

            for (Integer c = 0; c != channels; ++c) {
                for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                    buffer[c][i] += (
                        modulator_add_volume_value * modulator_output[c][i]
                    );
                }
            }
        }

    } else {
        for (Integer v = 0; v != polyphony; ++v) {
            if (!modulators_on[v]) {
                continue;
            }

            Sample const* const* const modulator_output = (
                SignalProducer::produce<Modulator>(*modulators[v], round)
            );

            for (Integer c = 0; c != channels; ++c) {
                for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                    buffer[c][i] += (
                        modulator_add_volume_buffer[i] * modulator_output[c][i]
                    );
                }
            }
        }
    }
}


void Synth::Bus::mix_carriers(
        Integer const round,
        Integer const first_sample_index,
        Integer const last_sample_index,
        Sample** buffer
) const noexcept {
    for (Integer v = 0; v != polyphony; ++v) {
        if (!carriers_on[v]) {
            continue;
        }

        Sample const* const* const carrier_output = (
            SignalProducer::produce<Carrier>(*carriers[v], round)
        );

        for (Integer c = 0; c != channels; ++c) {
            for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                buffer[c][i] += carrier_output[c][i];
            }
        }
    }
}


Synth::ParamIdHashTable::ParamIdHashTable() noexcept
{
}


Synth::ParamIdHashTable::~ParamIdHashTable() noexcept
{
}


void Synth::ParamIdHashTable::add(std::string const& name, ParamId const param_id) noexcept
{
    Entry* root;
    Entry* parent;
    Entry* entry;

    lookup(name, &root, &parent, &entry);

    if (entry != NULL) {
        return;
    }

    if (parent != NULL) {
        parent->next = new Entry(name.c_str(), param_id);

        return;
    }

    root->set(name.c_str(), param_id);
}


void Synth::ParamIdHashTable::lookup(
        std::string const& name,
        Entry** root,
        Entry** parent,
        Entry** entry
) noexcept {
    char const* const name_ptr = name.c_str();
    Integer const hash = this->hash(name);
    *root = &entries[hash];

    *parent = NULL;

    if ((*root)->param_id == MAX_PARAM_ID) {
        *entry = NULL;

        return;
    }

    *entry = *root;

    while (strncmp((*entry)->name, name_ptr, Entry::NAME_SIZE) != 0) {
        *parent = *entry;
        *entry = (*entry)->next;

        if (*entry == NULL) {
            break;
        }
    }
}


Synth::ParamId Synth::ParamIdHashTable::lookup(std::string const& name) noexcept
{
    Entry* root;
    Entry* parent;
    Entry* entry;

    lookup(name, &root, &parent, &entry);

    return entry == NULL ? ParamId::MAX_PARAM_ID : entry->param_id;
}


void Synth::ParamIdHashTable::get_statistics(
        Integer& max_collisions,
        Number& avg_collisions,
        Number& avg_bucket_size
) const noexcept {
    Integer collisions_sum = 0;
    Integer collisions_count = 0;
    Integer bucket_size_sum = 0;
    Integer bucket_count = 0;

    max_collisions = 0;

    for (Integer i = 0; i != ENTRIES; ++i) {
        Entry const* entry = &entries[i];

        if (entry->param_id == ParamId::MAX_PARAM_ID) {
            continue;
        }

        Integer collisions = 1;
        ++bucket_count;
        ++bucket_size_sum;
        entry = entry->next;

        while (entry != NULL) {
            ++collisions;
            ++bucket_size_sum;
            entry = entry->next;
        }

        if (collisions > 1) {
            collisions_sum += collisions;
            ++collisions_count;

            if (collisions > max_collisions) {
                max_collisions = collisions;
            }
        }
    }

    avg_collisions = (double)collisions_sum / (double)collisions_count;
    avg_bucket_size = (double)bucket_size_sum / (double)bucket_count;
}


/*
Inspiration from https://orlp.net/blog/worlds-smallest-hash-table/
*/
Integer Synth::ParamIdHashTable::hash(std::string const& name) noexcept
{
    /*
    We only care about the 36 characters which are used in param names: capital
    letters and numbers.
    */
    constexpr Integer alphabet_size = 36;
    constexpr char letter_offset = 'A' - 10;
    constexpr char number_offset = '0';

    char const* name_ptr = name.c_str();

    if (*name_ptr == '\x00') {
        return 0;
    }

    Integer i;
    Integer hash = 0;

    for (i = -1; *name_ptr != '\x00'; ++name_ptr) {
        char c = *name_ptr;

        if (c >= letter_offset) {
            c -= letter_offset;
        } else {
            c -= number_offset;
        }

        hash = hash * alphabet_size + c;

        if (++i == 4) {
            break;
        }
    }

    hash = (hash << 3) + i;

    if (hash < 0) {
        hash = -hash;
    }

    hash = (hash * MULTIPLIER >> SHIFT) & MASK;

    return hash;
}


Synth::ParamIdHashTable::Entry::Entry() noexcept : next(NULL)
{
    set("", MAX_PARAM_ID);
}


Synth::ParamIdHashTable::Entry::Entry(
        const char* name,
        ParamId const param_id
) noexcept
    : next(NULL)
{
    set(name, param_id);
}


Synth::ParamIdHashTable::Entry::~Entry()
{
    if (next != NULL) {
        delete next;

        next = NULL;
    }
}


void Synth::ParamIdHashTable::Entry::set(
        const char* name,
        ParamId const param_id
) noexcept {
    std::fill_n(this->name, NAME_SIZE, '\x00');
    strncpy(this->name, name, NAME_MAX_INDEX);
    this->param_id = param_id;
}


Synth::MidiControllerMessage::MidiControllerMessage() : time_offset(-INFINITY), value(0)
{
}


Synth::MidiControllerMessage::MidiControllerMessage(
        Seconds const time_offset,
        Midi::Word const value
) : time_offset(time_offset),
    value(value)
{
}


bool Synth::MidiControllerMessage::operator==(
        MidiControllerMessage const& message
) const noexcept {
    return value == message.value && time_offset == message.time_offset;
}


Synth::MidiControllerMessage& Synth::MidiControllerMessage::operator=(
        MidiControllerMessage const& message
) noexcept {
    if (this != &message) {
        time_offset = message.time_offset;
        value = message.value;
    }

    return *this;
}


Synth::MidiControllerMessage& Synth::MidiControllerMessage::operator=(
        MidiControllerMessage const&& message
) noexcept {
    if (this != &message) {
        time_offset = message.time_offset;
        value = message.value;
    }

    return *this;
}


Synth::DelayedNoteOff::DelayedNoteOff()
    : voice(INVALID_VOICE),
    channel(0),
    note(0),
    velocity(0)
{
}


Synth::DelayedNoteOff::DelayedNoteOff(DelayedNoteOff const& delayed_note_off)
    : voice(delayed_note_off.voice),
    channel(delayed_note_off.channel),
    note(delayed_note_off.note),
    velocity(delayed_note_off.velocity)
{
}


Synth::DelayedNoteOff::DelayedNoteOff(DelayedNoteOff const&& delayed_note_off)
    : voice(delayed_note_off.voice),
    channel(delayed_note_off.channel),
    note(delayed_note_off.note),
    velocity(delayed_note_off.velocity)
{
}


Synth::DelayedNoteOff::DelayedNoteOff(
        Midi::Channel const channel,
        Midi::Note const note,
        Midi::Byte const velocity,
        Integer const voice
) : voice(voice), channel(channel), note(note), velocity(velocity)
{
}


Synth::DelayedNoteOff& Synth::DelayedNoteOff::operator=(
        DelayedNoteOff const& delayed_note_off
) noexcept {
    if (this != &delayed_note_off) {
        voice = delayed_note_off.voice;
        channel = delayed_note_off.channel;
        note = delayed_note_off.note;
        velocity = delayed_note_off.velocity;
    }

    return *this;
}


Synth::DelayedNoteOff& Synth::DelayedNoteOff::operator=(
        DelayedNoteOff const&& delayed_note_off
) noexcept {
    if (this != &delayed_note_off) {
        voice = delayed_note_off.voice;
        channel = delayed_note_off.channel;
        note = delayed_note_off.note;
        velocity = delayed_note_off.velocity;
    }

    return *this;
}


Midi::Channel Synth::DelayedNoteOff::get_channel() const noexcept
{
    return channel;
}


Midi::Note Synth::DelayedNoteOff::get_note() const noexcept
{
    return note;
}


Midi::Byte Synth::DelayedNoteOff::get_velocity() const noexcept
{
    return velocity;
}


Integer Synth::DelayedNoteOff::get_voice() const noexcept
{
    return voice;
}

}
