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

#include <algorithm>
#include <cstddef>
#include <cmath>
#include <cstring>
#include <limits>
#include <sstream>
#include <type_traits>

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
#include "dsp/gain.cpp"
#include "dsp/lfo.cpp"
#include "dsp/lfo_envelope_list.cpp"
#include "dsp/macro.cpp"
#include "dsp/math.cpp"
#include "dsp/midi_controller.cpp"
#include "dsp/mixer.cpp"
#include "dsp/oscillator.cpp"
#include "dsp/param.cpp"
#include "dsp/reverb.cpp"
#include "dsp/queue.cpp"
#include "dsp/peak_tracker.cpp"
#include "dsp/side_chain_compressable_effect.cpp"
#include "dsp/signal_producer.cpp"
#include "dsp/wavefolder.cpp"
#include "dsp/wavetable.cpp"

#include "note_stack.cpp"
#include "spscqueue.cpp"
#include "voice.cpp"


namespace JS80P
{

std::vector<bool> Synth::supported_midi_controllers;

bool Synth::supported_midi_controllers_initialized = false;


Synth::ParamIdHashTable Synth::param_id_hash_table;

std::string Synth::param_names_by_id[ParamId::PARAM_ID_COUNT];


Synth::ModeParam::ModeParam(std::string const& name) noexcept
    : ByteParam(name, MODE_MIX_AND_MOD, MODE_SPLIT_AT_C4, MODE_MIX_AND_MOD)
{
}


Synth::Synth(Integer const samples_between_gc) noexcept
    : SignalProducer(
        OUT_CHANNELS,
        7                           /* POLY + MODE + MIX + PM + FM + AM + bus   */
        + 41 * 2                    /* Modulator::Params + Carrier::Params      */
        + POLYPHONY * 2             /* modulators + carriers                    */
        + 1                         /* effects                                  */
        + MACROS * MACRO_PARAMS
        + (Integer)Constants::ENVELOPES * (ENVELOPE_FLOAT_PARAMS + ENVELOPE_DISCRETE_PARAMS)
        + (Integer)Constants::LFOS
    ),
    polyphonic("POLY", ToggleParam::ON),
    mode("MODE"),
    modulator_add_volume(
        "MIX",
        0.0,
        1.0,
        1.0,
        0.0,
        (Envelope* const*)&envelopes_rw
    ),
    phase_modulation_level(
        "PM",
        Constants::PM_MIN,
        Constants::PM_MAX,
        Constants::PM_DEFAULT,
        0.0,
        (Envelope* const*)&envelopes_rw

    ),
    frequency_modulation_level(
        "FM",
        Constants::FM_MIN,
        Constants::FM_MAX,
        Constants::FM_DEFAULT,
        0.0,
        (Envelope* const*)&envelopes_rw

    ),
    amplitude_modulation_level(
        "AM",
        Constants::AM_MIN,
        Constants::AM_MAX,
        Constants::AM_DEFAULT,
        0.0,
        (Envelope* const*)&envelopes_rw
    ),
    modulator_params("M", (Envelope* const*)&envelopes_rw),
    carrier_params("C", (Envelope* const*)&envelopes_rw),
    messages(MESSAGE_QUEUE_SIZE),
    bus(
        OUT_CHANNELS,
        modulators,
        modulator_params,
        carriers,
        carrier_params,
        POLYPHONY,
        modulator_add_volume
    ),
    samples_since_gc(0),
    samples_between_gc(samples_between_gc),
    next_voice(0),
    next_note_id(0),
    previous_note(Midi::NOTE_MAX + 1),
    is_learning(false),
    is_sustaining(false),
    is_polyphonic(true),
    was_polyphonic(true),
    is_dirty_(false),
    effects(
        "E",
        bus,
        biquad_filter_shared_buffers[4],
        biquad_filter_shared_buffers[5]
    ),
    midi_controllers((MidiController* const*)midi_controllers_rw),
    macros((Macro* const*)macros_rw),
    envelopes((Envelope* const*)envelopes_rw),
    lfos((LFO* const*)lfos_rw)
{
    is_mts_esp_connected_.store(false);

    deferred_note_offs.reserve(2 * POLYPHONY);

    initialize_supported_midi_controllers();

    allocate_buffers();

    for (int i = 0; i != (int)ParamId::PARAM_ID_COUNT; ++i) {
        param_ratios[i].store(0.0);
        controller_assignments[i].store(ControllerId::NONE);
        param_names_by_id[i] = "";

        sample_evaluated_float_params[i] = NULL;
        block_evaluated_float_params[i] = NULL;
        byte_params[i] = NULL;
    }

    build_frequency_table();
    register_main_params();
    register_child(bus);
    register_modulator_params();
    register_carrier_params();
    register_child(effects);
    register_effects_params();

    create_envelopes();
    create_lfos();
    create_voices();
    create_midi_controllers();
    create_macros();

    modulator_params.filter_1_freq_log_scale.set_value(ToggleParam::ON);
    modulator_params.filter_2_freq_log_scale.set_value(ToggleParam::ON);
    carrier_params.filter_1_freq_log_scale.set_value(ToggleParam::ON);
    carrier_params.filter_2_freq_log_scale.set_value(ToggleParam::ON);
    effects.filter_1_freq_log_scale.set_value(ToggleParam::ON);
    effects.filter_2_freq_log_scale.set_value(ToggleParam::ON);

    channel_pressure_ctl.change(0.0, 0.0);
    channel_pressure_ctl.clear();

    midi_controllers_rw[Midi::SUSTAIN_PEDAL]->change(0.0, 0.0);
    midi_controllers_rw[Midi::SUSTAIN_PEDAL]->clear();

    osc_1_peak.change(0.0, 0.0);
    osc_1_peak.clear();

    osc_2_peak.change(0.0, 0.0);
    osc_2_peak.clear();

    vol_1_peak.change(0.0, 0.0);
    vol_1_peak.clear();

    vol_2_peak.change(0.0, 0.0);
    vol_2_peak.clear();

    vol_3_peak.change(0.0, 0.0);
    vol_3_peak.clear();

    update_param_states();
}


void Synth::initialize_supported_midi_controllers() noexcept
{
    if (supported_midi_controllers_initialized) {
        return;
    }

    supported_midi_controllers_initialized = true;

    std::vector<bool> supported_midi_controllers(Synth::MIDI_CONTROLLERS, false);

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

    Synth::supported_midi_controllers = supported_midi_controllers;
}


void Synth::build_frequency_table() noexcept
{
    for (Midi::Note note = 0; note != Midi::NOTES; ++note) {
        /*
         * Not using Math::exp() and friends here, for 2 reasons:
         *  1. We go out of their domain here.
         *  2. Since this loop runs only once, we can afford favoring accuracy
         *     over speed.
         */

        Frequency const f_440hz_12tet = (Frequency)(
            std::pow(2.0, ((Frequency)note - 69.0) / 12.0) * 440.0
        );
        Frequency const f_432hz_12tet = (Frequency)(
            std::pow(2.0, ((Frequency)note - 69.0) / 12.0) * 432.0
        );

        per_channel_frequencies[0][note] = f_440hz_12tet;

        frequencies[Modulator::TUNING_440HZ_12TET][note] = f_440hz_12tet;
        frequencies[Modulator::TUNING_432HZ_12TET][note] = f_432hz_12tet;
    }

    for (Midi::Channel channel = 1; channel != Midi::CHANNELS; ++channel) {
        for (Midi::Note note = 0; note != Midi::NOTES; ++note) {
            per_channel_frequencies[channel][note] = per_channel_frequencies[0][note];
        }
    }
}

void Synth::register_main_params() noexcept
{
    register_param_as_child<ToggleParam>(ParamId::POLY, polyphonic);

    register_param_as_child<ModeParam>(ParamId::MODE, mode);

    register_param_as_child<FloatParamS>(ParamId::MIX, modulator_add_volume);
    register_param_as_child<FloatParamS>(ParamId::PM, phase_modulation_level);
    register_param_as_child<FloatParamS>(ParamId::FM, frequency_modulation_level);
    register_param_as_child<FloatParamS>(ParamId::AM, amplitude_modulation_level);
}


void Synth::register_modulator_params() noexcept
{
    register_param_as_child<Modulator::TuningParam>(ParamId::MTUN, modulator_params.tuning);
    register_param_as_child<OscillatorInaccuracyParam>(ParamId::MOIA, modulator_params.oscillator_inaccuracy);
    register_param_as_child<OscillatorInaccuracyParam>(ParamId::MOIS, modulator_params.oscillator_instability);

    register_param_as_child<Modulator::Oscillator_::WaveformParam>(ParamId::MWAV, modulator_params.waveform);
    register_param_as_child<FloatParamS>(ParamId::MAMP, modulator_params.amplitude);
    register_param_as_child<FloatParamB>(ParamId::MVS, modulator_params.velocity_sensitivity);
    register_param_as_child<FloatParamS>(ParamId::MFLD, modulator_params.folding);
    register_param_as_child<FloatParamB>(ParamId::MPRT, modulator_params.portamento_length);
    register_param_as_child<FloatParamB>(ParamId::MPRD, modulator_params.portamento_depth);
    register_param_as_child<FloatParamS>(ParamId::MDTN, modulator_params.detune);
    register_param_as_child<FloatParamS>(ParamId::MFIN, modulator_params.fine_detune);
    register_param_as_child<FloatParamB>(ParamId::MWID, modulator_params.width);
    register_param_as_child<FloatParamS>(ParamId::MPAN, modulator_params.panning);
    register_param_as_child<FloatParamS>(ParamId::MVOL, modulator_params.volume);
    register_param_as_child<FloatParamS>(ParamId::MSUB, modulator_params.subharmonic_amplitude);

    register_param_as_child<FloatParamB>(ParamId::MC1, modulator_params.harmonic_0);
    register_param_as_child<FloatParamB>(ParamId::MC2, modulator_params.harmonic_1);
    register_param_as_child<FloatParamB>(ParamId::MC3, modulator_params.harmonic_2);
    register_param_as_child<FloatParamB>(ParamId::MC4, modulator_params.harmonic_3);
    register_param_as_child<FloatParamB>(ParamId::MC5, modulator_params.harmonic_4);
    register_param_as_child<FloatParamB>(ParamId::MC6, modulator_params.harmonic_5);
    register_param_as_child<FloatParamB>(ParamId::MC7, modulator_params.harmonic_6);
    register_param_as_child<FloatParamB>(ParamId::MC8, modulator_params.harmonic_7);
    register_param_as_child<FloatParamB>(ParamId::MC9, modulator_params.harmonic_8);
    register_param_as_child<FloatParamB>(ParamId::MC10, modulator_params.harmonic_9);

    register_param_as_child<Modulator::Filter1::TypeParam>(
        ParamId::MF1TYP, modulator_params.filter_1_type
    );
    register_param_as_child<ToggleParam>(ParamId::MF1LOG, modulator_params.filter_1_freq_log_scale);
    register_param_as_child<ToggleParam>(ParamId::MF1QLG, modulator_params.filter_1_q_log_scale);
    register_param_as_child<FloatParamS>(ParamId::MF1FRQ, modulator_params.filter_1_frequency);
    register_param_as_child<FloatParamS>(ParamId::MF1Q, modulator_params.filter_1_q);
    register_param_as_child<FloatParamS>(ParamId::MF1G, modulator_params.filter_1_gain);
    register_param_as_child<FloatParamB>(ParamId::MF1FIA, modulator_params.filter_1_freq_inaccuracy);
    register_param_as_child<FloatParamB>(ParamId::MF1QIA, modulator_params.filter_1_q_inaccuracy);

    register_param_as_child<Modulator::Filter2::TypeParam>(
        ParamId::MF2TYP, modulator_params.filter_2_type
    );
    register_param_as_child<ToggleParam>(ParamId::MF2LOG, modulator_params.filter_2_freq_log_scale);
    register_param_as_child<ToggleParam>(ParamId::MF2QLG, modulator_params.filter_2_q_log_scale);
    register_param_as_child<FloatParamS>(ParamId::MF2FRQ, modulator_params.filter_2_frequency);
    register_param_as_child<FloatParamS>(ParamId::MF2Q, modulator_params.filter_2_q);
    register_param_as_child<FloatParamS>(ParamId::MF2G, modulator_params.filter_2_gain);
    register_param_as_child<FloatParamB>(ParamId::MF2FIA, modulator_params.filter_2_freq_inaccuracy);
    register_param_as_child<FloatParamB>(ParamId::MF2QIA, modulator_params.filter_2_q_inaccuracy);
}


void Synth::register_carrier_params() noexcept
{
    register_param_as_child<Carrier::TuningParam>(ParamId::CTUN, carrier_params.tuning);
    register_param_as_child<OscillatorInaccuracyParam>(ParamId::COIA, carrier_params.oscillator_inaccuracy);
    register_param_as_child<OscillatorInaccuracyParam>(ParamId::COIS, carrier_params.oscillator_instability);

    register_param_as_child<Carrier::Oscillator_::WaveformParam>(ParamId::CWAV, carrier_params.waveform);
    register_param_as_child<FloatParamS>(ParamId::CAMP, carrier_params.amplitude);
    register_param_as_child<FloatParamB>(ParamId::CVS, carrier_params.velocity_sensitivity);
    register_param_as_child<FloatParamS>(ParamId::CFLD, carrier_params.folding);
    register_param_as_child<FloatParamB>(ParamId::CPRT, carrier_params.portamento_length);
    register_param_as_child<FloatParamB>(ParamId::CPRD, carrier_params.portamento_depth);
    register_param_as_child<FloatParamS>(ParamId::CDTN, carrier_params.detune);
    register_param_as_child<FloatParamS>(ParamId::CFIN, carrier_params.fine_detune);
    register_param_as_child<FloatParamB>(ParamId::CWID, carrier_params.width);
    register_param_as_child<FloatParamS>(ParamId::CPAN, carrier_params.panning);
    register_param_as_child<FloatParamS>(ParamId::CVOL, carrier_params.volume);
    register_param_as_child<FloatParamS>(ParamId::CDG, carrier_params.distortion);

    register_param_as_child<FloatParamB>(ParamId::CC1, carrier_params.harmonic_0);
    register_param_as_child<FloatParamB>(ParamId::CC2, carrier_params.harmonic_1);
    register_param_as_child<FloatParamB>(ParamId::CC3, carrier_params.harmonic_2);
    register_param_as_child<FloatParamB>(ParamId::CC4, carrier_params.harmonic_3);
    register_param_as_child<FloatParamB>(ParamId::CC5, carrier_params.harmonic_4);
    register_param_as_child<FloatParamB>(ParamId::CC6, carrier_params.harmonic_5);
    register_param_as_child<FloatParamB>(ParamId::CC7, carrier_params.harmonic_6);
    register_param_as_child<FloatParamB>(ParamId::CC8, carrier_params.harmonic_7);
    register_param_as_child<FloatParamB>(ParamId::CC9, carrier_params.harmonic_8);
    register_param_as_child<FloatParamB>(ParamId::CC10, carrier_params.harmonic_9);

    register_param_as_child<Carrier::Filter1::TypeParam>(
        ParamId::CF1TYP, carrier_params.filter_1_type
    );
    register_param_as_child<ToggleParam>(ParamId::CF1LOG, carrier_params.filter_1_freq_log_scale);
    register_param_as_child<ToggleParam>(ParamId::CF1QLG, carrier_params.filter_1_q_log_scale);
    register_param_as_child<FloatParamS>(ParamId::CF1FRQ, carrier_params.filter_1_frequency);
    register_param_as_child<FloatParamS>(ParamId::CF1Q, carrier_params.filter_1_q);
    register_param_as_child<FloatParamS>(ParamId::CF1G, carrier_params.filter_1_gain);
    register_param_as_child<FloatParamB>(ParamId::CF1FIA, carrier_params.filter_1_freq_inaccuracy);
    register_param_as_child<FloatParamB>(ParamId::CF1QIA, carrier_params.filter_1_q_inaccuracy);

    register_param_as_child<Carrier::Filter2::TypeParam>(
        ParamId::CF2TYP, carrier_params.filter_2_type
    );
    register_param_as_child<ToggleParam>(ParamId::CF2LOG, carrier_params.filter_2_freq_log_scale);
    register_param_as_child<ToggleParam>(ParamId::CF2QLG, carrier_params.filter_2_q_log_scale);
    register_param_as_child<FloatParamS>(ParamId::CF2FRQ, carrier_params.filter_2_frequency);
    register_param_as_child<FloatParamS>(ParamId::CF2Q, carrier_params.filter_2_q);
    register_param_as_child<FloatParamS>(ParamId::CF2G, carrier_params.filter_2_gain);
    register_param_as_child<FloatParamB>(ParamId::CF2FIA, carrier_params.filter_2_freq_inaccuracy);
    register_param_as_child<FloatParamB>(ParamId::CF2QIA, carrier_params.filter_2_q_inaccuracy);
}


void Synth::register_effects_params() noexcept
{
    register_param<FloatParamS>(ParamId::EV1V, effects.volume_1_gain);

    register_param<FloatParamS>(ParamId::EOG, effects.overdrive.level);

    register_param<FloatParamS>(ParamId::EDG, effects.distortion.level);

    register_param<Effects::Filter1<Bus>::TypeParam>(ParamId::EF1TYP, effects.filter_1_type);
    register_param<ToggleParam>(ParamId::EF1LOG, effects.filter_1_freq_log_scale);
    register_param<ToggleParam>(ParamId::EF1QLG, effects.filter_1_q_log_scale);
    register_param<FloatParamS>(ParamId::EF1FRQ, effects.filter_1.frequency);
    register_param<FloatParamS>(ParamId::EF1Q, effects.filter_1.q);
    register_param<FloatParamS>(ParamId::EF1G, effects.filter_1.gain);

    register_param<Effects::Filter2<Bus>::TypeParam>(ParamId::EF2TYP, effects.filter_2_type);
    register_param<ToggleParam>(ParamId::EF2LOG, effects.filter_2_freq_log_scale);
    register_param<ToggleParam>(ParamId::EF2QLG, effects.filter_2_q_log_scale);
    register_param<FloatParamS>(ParamId::EF2FRQ, effects.filter_2.frequency);
    register_param<FloatParamS>(ParamId::EF2Q, effects.filter_2.q);
    register_param<FloatParamS>(ParamId::EF2G, effects.filter_2.gain);

    register_param<FloatParamS>(ParamId::EV2V, effects.volume_2_gain);

    register_param<Effects::Chorus<Bus>::TypeParam>(ParamId::ECTYP, effects.chorus.type);
    register_param<FloatParamS>(ParamId::ECDEL, effects.chorus.delay_time);
    register_param<FloatParamS>(ParamId::ECFRQ, effects.chorus.frequency);
    register_param<FloatParamS>(ParamId::ECDPT, effects.chorus.depth);
    register_param<FloatParamS>(ParamId::ECFB, effects.chorus.feedback);
    register_param<FloatParamS>(ParamId::ECDF, effects.chorus.damping_frequency);
    register_param<FloatParamS>(ParamId::ECDG, effects.chorus.damping_gain);
    register_param<FloatParamS>(ParamId::ECWID, effects.chorus.width);
    register_param<FloatParamS>(ParamId::ECHPF, effects.chorus.high_pass_frequency);
    register_param<FloatParamS>(ParamId::ECHPQ, effects.chorus.high_pass_q);
    register_param<FloatParamS>(ParamId::ECWET, effects.chorus.wet);
    register_param<FloatParamS>(ParamId::ECDRY, effects.chorus.dry);
    register_param<ToggleParam>(ParamId::ECSYN, effects.chorus.tempo_sync);
    register_param<ToggleParam>(ParamId::ECLOG, effects.chorus.log_scale_filter_frequencies);
    register_param<ToggleParam>(ParamId::ECLHQ, effects.chorus.log_scale_high_pass_q);
    register_param<ToggleParam>(ParamId::ECLLG, effects.chorus.log_scale_lfo_frequency);

    register_param<FloatParamS>(ParamId::EEDEL, effects.echo.delay_time);
    register_param<FloatParamS>(ParamId::EEINV, effects.echo.input_volume);
    register_param<FloatParamS>(ParamId::EEFB, effects.echo.feedback);
    register_param<FloatParamS>(ParamId::EEDST, effects.echo.distortion_level);
    register_param<FloatParamS>(ParamId::EEDF, effects.echo.damping_frequency);
    register_param<FloatParamS>(ParamId::EEDG, effects.echo.damping_gain);
    register_param<FloatParamS>(ParamId::EEWID, effects.echo.width);
    register_param<FloatParamS>(ParamId::EEHPF, effects.echo.high_pass_frequency);
    register_param<FloatParamS>(ParamId::EEHPQ, effects.echo.high_pass_q);
    register_param<FloatParamB>(ParamId::EECTH, effects.echo.side_chain_compression_threshold);
    register_param<FloatParamB>(ParamId::EECAT, effects.echo.side_chain_compression_attack_time);
    register_param<FloatParamB>(ParamId::EECRL, effects.echo.side_chain_compression_release_time);
    register_param<FloatParamB>(ParamId::EECR, effects.echo.side_chain_compression_ratio);
    register_param<FloatParamS>(ParamId::EEWET, effects.echo.wet);
    register_param<FloatParamS>(ParamId::EEDRY, effects.echo.dry);
    register_param<ToggleParam>(ParamId::EESYN, effects.echo.tempo_sync);
    register_param<ToggleParam>(ParamId::EELOG, effects.echo.log_scale_frequencies);
    register_param<ToggleParam>(ParamId::EELHQ, effects.echo.log_scale_high_pass_q);

    register_param<Effects::Reverb<Bus>::TypeParam>(ParamId::ERTYP, effects.reverb.type);
    register_param<FloatParamS>(ParamId::ERRS, effects.reverb.room_size);
    register_param<FloatParamS>(ParamId::ERDST, effects.reverb.distortion_level);
    register_param<FloatParamS>(ParamId::ERDF, effects.reverb.damping_frequency);
    register_param<FloatParamS>(ParamId::ERDG, effects.reverb.damping_gain);
    register_param<FloatParamS>(ParamId::ERWID, effects.reverb.width);
    register_param<FloatParamS>(ParamId::ERHPF, effects.reverb.high_pass_frequency);
    register_param<FloatParamS>(ParamId::ERHPQ, effects.reverb.high_pass_q);
    register_param<FloatParamB>(ParamId::ERCTH, effects.reverb.side_chain_compression_threshold);
    register_param<FloatParamB>(ParamId::ERCAT, effects.reverb.side_chain_compression_attack_time);
    register_param<FloatParamB>(ParamId::ERCRL, effects.reverb.side_chain_compression_release_time);
    register_param<FloatParamB>(ParamId::ERCR, effects.reverb.side_chain_compression_ratio);
    register_param<FloatParamS>(ParamId::ERWET, effects.reverb.wet);
    register_param<FloatParamS>(ParamId::ERDRY, effects.reverb.dry);
    register_param<ToggleParam>(ParamId::ERLOG, effects.reverb.log_scale_frequencies);
    register_param<ToggleParam>(ParamId::ERLHQ, effects.reverb.log_scale_high_pass_q);

    register_param<FloatParamS>(ParamId::EV3V, effects.volume_3_gain);
}


void Synth::create_voices() noexcept
{
    for (Integer i = 0; i != POLYPHONY; ++i) {
        synced_oscillator_inaccuracies[i] = (
            new OscillatorInaccuracy(calculate_inaccuracy_seed(i))
        );

        modulators[i] = new Modulator(
            frequencies,
            per_channel_frequencies,
            *synced_oscillator_inaccuracies[i],
            calculate_inaccuracy_seed((i + 23) % POLYPHONY),
            modulator_params,
            &biquad_filter_shared_buffers[0],
            &biquad_filter_shared_buffers[1]
        );
        register_child(*modulators[i]);

        carriers[i] = new Carrier(
            frequencies,
            per_channel_frequencies,
            *synced_oscillator_inaccuracies[i],
            calculate_inaccuracy_seed((POLYPHONY - i + 41) % POLYPHONY),
            carrier_params,
            modulators[i]->modulation_out,
            amplitude_modulation_level,
            frequency_modulation_level,
            phase_modulation_level,
            &biquad_filter_shared_buffers[2],
            &biquad_filter_shared_buffers[3]
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


void Synth::create_macros() noexcept
{
    Integer next_id = ParamId::M1IN;

    for (Integer i = 0; i != MACROS; ++i) {
        /*
        Macros used to be called Flexible Controllers, hence the F for
        backward-compatibility.
        */
        Macro* macro = new Macro(std::string("F") + to_string(i + 1));

        macros_rw[i] = macro;

        register_param_as_child<FloatParamB>((ParamId)next_id++, macro->input);
        register_param_as_child<FloatParamB>((ParamId)next_id++, macro->min);
        register_param_as_child<FloatParamB>((ParamId)next_id++, macro->max);
        register_param_as_child<FloatParamB>((ParamId)next_id++, macro->amount);
        register_param_as_child<FloatParamB>((ParamId)next_id++, macro->distortion);
        register_param_as_child<FloatParamB>((ParamId)next_id++, macro->randomness);
    }

    next_id = ParamId::M1DSH;

    for (Integer i = 0; i != MACROS; ++i) {
        register_param_as_child<Macro::DistortionShapeParam>(
            (ParamId)next_id++, macros_rw[i]->distortion_shape
        );
    }
}


void Synth::create_envelopes() noexcept
{
    Integer next_id = ParamId::N1AMT;

    for (Byte i = 0; i != Constants::ENVELOPES; ++i) {
        Envelope* envelope = new Envelope(std::string("N") + to_string((Integer)(i + 1)));
        envelopes_rw[i] = envelope;

        register_param_as_child<FloatParamB>((ParamId)next_id++, envelope->amount);
        register_param_as_child<FloatParamB>((ParamId)next_id++, envelope->initial_value);
        register_param_as_child<FloatParamB>((ParamId)next_id++, envelope->delay_time);
        register_param_as_child<FloatParamB>((ParamId)next_id++, envelope->attack_time);
        register_param_as_child<FloatParamB>((ParamId)next_id++, envelope->peak_value);
        register_param_as_child<FloatParamB>((ParamId)next_id++, envelope->hold_time);
        register_param_as_child<FloatParamB>((ParamId)next_id++, envelope->decay_time);
        register_param_as_child<FloatParamB>((ParamId)next_id++, envelope->sustain_value);
        register_param_as_child<FloatParamB>((ParamId)next_id++, envelope->release_time);
        register_param_as_child<FloatParamB>((ParamId)next_id++, envelope->final_value);
        register_param_as_child<FloatParamB>((ParamId)next_id++, envelope->time_inaccuracy);
        register_param_as_child<FloatParamB>((ParamId)next_id++, envelope->value_inaccuracy);
    }

    register_param_as_child<ByteParam>(ParamId::N1UPD, envelopes_rw[0]->update_mode);
    register_param_as_child<ByteParam>(ParamId::N2UPD, envelopes_rw[1]->update_mode);
    register_param_as_child<ByteParam>(ParamId::N3UPD, envelopes_rw[2]->update_mode);
    register_param_as_child<ByteParam>(ParamId::N4UPD, envelopes_rw[3]->update_mode);
    register_param_as_child<ByteParam>(ParamId::N5UPD, envelopes_rw[4]->update_mode);
    register_param_as_child<ByteParam>(ParamId::N6UPD, envelopes_rw[5]->update_mode);
    register_param_as_child<ByteParam>(ParamId::N7UPD, envelopes_rw[6]->update_mode);
    register_param_as_child<ByteParam>(ParamId::N8UPD, envelopes_rw[7]->update_mode);
    register_param_as_child<ByteParam>(ParamId::N9UPD, envelopes_rw[8]->update_mode);
    register_param_as_child<ByteParam>(ParamId::N10UPD, envelopes_rw[9]->update_mode);
    register_param_as_child<ByteParam>(ParamId::N11UPD, envelopes_rw[10]->update_mode);
    register_param_as_child<ByteParam>(ParamId::N12UPD, envelopes_rw[11]->update_mode);

    register_param_as_child<ToggleParam>(ParamId::N1SYN, envelopes_rw[0]->tempo_sync);
    register_param_as_child<ToggleParam>(ParamId::N2SYN, envelopes_rw[1]->tempo_sync);
    register_param_as_child<ToggleParam>(ParamId::N3SYN, envelopes_rw[2]->tempo_sync);
    register_param_as_child<ToggleParam>(ParamId::N4SYN, envelopes_rw[3]->tempo_sync);
    register_param_as_child<ToggleParam>(ParamId::N5SYN, envelopes_rw[4]->tempo_sync);
    register_param_as_child<ToggleParam>(ParamId::N6SYN, envelopes_rw[5]->tempo_sync);
    register_param_as_child<ToggleParam>(ParamId::N7SYN, envelopes_rw[6]->tempo_sync);
    register_param_as_child<ToggleParam>(ParamId::N8SYN, envelopes_rw[7]->tempo_sync);
    register_param_as_child<ToggleParam>(ParamId::N9SYN, envelopes_rw[8]->tempo_sync);
    register_param_as_child<ToggleParam>(ParamId::N10SYN, envelopes_rw[9]->tempo_sync);
    register_param_as_child<ToggleParam>(ParamId::N11SYN, envelopes_rw[10]->tempo_sync);
    register_param_as_child<ToggleParam>(ParamId::N12SYN, envelopes_rw[11]->tempo_sync);

    register_param_as_child<Envelope::ShapeParam>(ParamId::N1ASH, envelopes_rw[0]->attack_shape);
    register_param_as_child<Envelope::ShapeParam>(ParamId::N2ASH, envelopes_rw[1]->attack_shape);
    register_param_as_child<Envelope::ShapeParam>(ParamId::N3ASH, envelopes_rw[2]->attack_shape);
    register_param_as_child<Envelope::ShapeParam>(ParamId::N4ASH, envelopes_rw[3]->attack_shape);
    register_param_as_child<Envelope::ShapeParam>(ParamId::N5ASH, envelopes_rw[4]->attack_shape);
    register_param_as_child<Envelope::ShapeParam>(ParamId::N6ASH, envelopes_rw[5]->attack_shape);
    register_param_as_child<Envelope::ShapeParam>(ParamId::N7ASH, envelopes_rw[6]->attack_shape);
    register_param_as_child<Envelope::ShapeParam>(ParamId::N8ASH, envelopes_rw[7]->attack_shape);
    register_param_as_child<Envelope::ShapeParam>(ParamId::N9ASH, envelopes_rw[8]->attack_shape);
    register_param_as_child<Envelope::ShapeParam>(ParamId::N10ASH, envelopes_rw[9]->attack_shape);
    register_param_as_child<Envelope::ShapeParam>(ParamId::N11ASH, envelopes_rw[10]->attack_shape);
    register_param_as_child<Envelope::ShapeParam>(ParamId::N12ASH, envelopes_rw[11]->attack_shape);

    register_param_as_child<Envelope::ShapeParam>(ParamId::N1DSH, envelopes_rw[0]->decay_shape);
    register_param_as_child<Envelope::ShapeParam>(ParamId::N2DSH, envelopes_rw[1]->decay_shape);
    register_param_as_child<Envelope::ShapeParam>(ParamId::N3DSH, envelopes_rw[2]->decay_shape);
    register_param_as_child<Envelope::ShapeParam>(ParamId::N4DSH, envelopes_rw[3]->decay_shape);
    register_param_as_child<Envelope::ShapeParam>(ParamId::N5DSH, envelopes_rw[4]->decay_shape);
    register_param_as_child<Envelope::ShapeParam>(ParamId::N6DSH, envelopes_rw[5]->decay_shape);
    register_param_as_child<Envelope::ShapeParam>(ParamId::N7DSH, envelopes_rw[6]->decay_shape);
    register_param_as_child<Envelope::ShapeParam>(ParamId::N8DSH, envelopes_rw[7]->decay_shape);
    register_param_as_child<Envelope::ShapeParam>(ParamId::N9DSH, envelopes_rw[8]->decay_shape);
    register_param_as_child<Envelope::ShapeParam>(ParamId::N10DSH, envelopes_rw[9]->decay_shape);
    register_param_as_child<Envelope::ShapeParam>(ParamId::N11DSH, envelopes_rw[10]->decay_shape);
    register_param_as_child<Envelope::ShapeParam>(ParamId::N12DSH, envelopes_rw[11]->decay_shape);

    register_param_as_child<Envelope::ShapeParam>(ParamId::N1RSH, envelopes_rw[0]->release_shape);
    register_param_as_child<Envelope::ShapeParam>(ParamId::N2RSH, envelopes_rw[1]->release_shape);
    register_param_as_child<Envelope::ShapeParam>(ParamId::N3RSH, envelopes_rw[2]->release_shape);
    register_param_as_child<Envelope::ShapeParam>(ParamId::N4RSH, envelopes_rw[3]->release_shape);
    register_param_as_child<Envelope::ShapeParam>(ParamId::N5RSH, envelopes_rw[4]->release_shape);
    register_param_as_child<Envelope::ShapeParam>(ParamId::N6RSH, envelopes_rw[5]->release_shape);
    register_param_as_child<Envelope::ShapeParam>(ParamId::N7RSH, envelopes_rw[6]->release_shape);
    register_param_as_child<Envelope::ShapeParam>(ParamId::N8RSH, envelopes_rw[7]->release_shape);
    register_param_as_child<Envelope::ShapeParam>(ParamId::N9RSH, envelopes_rw[8]->release_shape);
    register_param_as_child<Envelope::ShapeParam>(ParamId::N10RSH, envelopes_rw[9]->release_shape);
    register_param_as_child<Envelope::ShapeParam>(ParamId::N11RSH, envelopes_rw[10]->release_shape);
    register_param_as_child<Envelope::ShapeParam>(ParamId::N12RSH, envelopes_rw[11]->release_shape);
}


void Synth::create_lfos() noexcept
{
    Integer next_id = ParamId::L1FRQ;

    for (Byte i = 0; i != Constants::LFOS; ++i) {
        LFO* lfo = new LFO(std::string("L") + to_string((Integer)(i + 1)), true);
        lfos_rw[i] = lfo;

        register_child(*lfo);
        register_param<FloatParamS>((ParamId)next_id++, lfo->frequency);
        register_param<FloatParamS>((ParamId)next_id++, lfo->phase);
        register_param<FloatParamS>((ParamId)next_id++, lfo->min);
        register_param<FloatParamS>((ParamId)next_id++, lfo->max);
        register_param<FloatParamS>((ParamId)next_id++, lfo->amount);
        register_param<FloatParamS>((ParamId)next_id++, lfo->distortion);
        register_param<FloatParamS>((ParamId)next_id++, lfo->randomness);
    }

    register_param<LFO::Oscillator_::WaveformParam>(ParamId::L1WAV, lfos_rw[0]->waveform);
    register_param<LFO::Oscillator_::WaveformParam>(ParamId::L2WAV, lfos_rw[1]->waveform);
    register_param<LFO::Oscillator_::WaveformParam>(ParamId::L3WAV, lfos_rw[2]->waveform);
    register_param<LFO::Oscillator_::WaveformParam>(ParamId::L4WAV, lfos_rw[3]->waveform);
    register_param<LFO::Oscillator_::WaveformParam>(ParamId::L5WAV, lfos_rw[4]->waveform);
    register_param<LFO::Oscillator_::WaveformParam>(ParamId::L6WAV, lfos_rw[5]->waveform);
    register_param<LFO::Oscillator_::WaveformParam>(ParamId::L7WAV, lfos_rw[6]->waveform);
    register_param<LFO::Oscillator_::WaveformParam>(ParamId::L8WAV, lfos_rw[7]->waveform);

    register_param<ToggleParam>(ParamId::L1LOG, lfos_rw[0]->freq_log_scale);
    register_param<ToggleParam>(ParamId::L2LOG, lfos_rw[1]->freq_log_scale);
    register_param<ToggleParam>(ParamId::L3LOG, lfos_rw[2]->freq_log_scale);
    register_param<ToggleParam>(ParamId::L4LOG, lfos_rw[3]->freq_log_scale);
    register_param<ToggleParam>(ParamId::L5LOG, lfos_rw[4]->freq_log_scale);
    register_param<ToggleParam>(ParamId::L6LOG, lfos_rw[5]->freq_log_scale);
    register_param<ToggleParam>(ParamId::L7LOG, lfos_rw[6]->freq_log_scale);
    register_param<ToggleParam>(ParamId::L8LOG, lfos_rw[7]->freq_log_scale);

    register_param<ToggleParam>(ParamId::L1SYN, lfos_rw[0]->tempo_sync);
    register_param<ToggleParam>(ParamId::L2SYN, lfos_rw[1]->tempo_sync);
    register_param<ToggleParam>(ParamId::L3SYN, lfos_rw[2]->tempo_sync);
    register_param<ToggleParam>(ParamId::L4SYN, lfos_rw[3]->tempo_sync);
    register_param<ToggleParam>(ParamId::L5SYN, lfos_rw[4]->tempo_sync);
    register_param<ToggleParam>(ParamId::L6SYN, lfos_rw[5]->tempo_sync);
    register_param<ToggleParam>(ParamId::L7SYN, lfos_rw[6]->tempo_sync);
    register_param<ToggleParam>(ParamId::L8SYN, lfos_rw[7]->tempo_sync);

    register_param<ToggleParam>(ParamId::L1CEN, lfos_rw[0]->center);
    register_param<ToggleParam>(ParamId::L2CEN, lfos_rw[1]->center);
    register_param<ToggleParam>(ParamId::L3CEN, lfos_rw[2]->center);
    register_param<ToggleParam>(ParamId::L4CEN, lfos_rw[3]->center);
    register_param<ToggleParam>(ParamId::L5CEN, lfos_rw[4]->center);
    register_param<ToggleParam>(ParamId::L6CEN, lfos_rw[5]->center);
    register_param<ToggleParam>(ParamId::L7CEN, lfos_rw[6]->center);
    register_param<ToggleParam>(ParamId::L8CEN, lfos_rw[7]->center);

    register_param<LFO::AmountEnvelopeParam>(ParamId::L1AEN, lfos_rw[0]->amount_envelope);
    register_param<LFO::AmountEnvelopeParam>(ParamId::L2AEN, lfos_rw[1]->amount_envelope);
    register_param<LFO::AmountEnvelopeParam>(ParamId::L3AEN, lfos_rw[2]->amount_envelope);
    register_param<LFO::AmountEnvelopeParam>(ParamId::L4AEN, lfos_rw[3]->amount_envelope);
    register_param<LFO::AmountEnvelopeParam>(ParamId::L5AEN, lfos_rw[4]->amount_envelope);
    register_param<LFO::AmountEnvelopeParam>(ParamId::L6AEN, lfos_rw[5]->amount_envelope);
    register_param<LFO::AmountEnvelopeParam>(ParamId::L7AEN, lfos_rw[6]->amount_envelope);
    register_param<LFO::AmountEnvelopeParam>(ParamId::L8AEN, lfos_rw[7]->amount_envelope);
}


void Synth::allocate_buffers() noexcept
{
    for (Integer i = 0; i != BIQUAD_FILTER_SHARED_BUFFERS; ++i) {
        BiquadFilterSharedBuffers& shared_buffers = biquad_filter_shared_buffers[i];

        shared_buffers.b0_buffer = new Sample[block_size];
        shared_buffers.b1_buffer = new Sample[block_size];
        shared_buffers.b2_buffer = new Sample[block_size];
        shared_buffers.a1_buffer = new Sample[block_size];
        shared_buffers.a2_buffer = new Sample[block_size];
    }
}


void Synth::free_buffers() noexcept
{
    for (Integer i = 0; i != BIQUAD_FILTER_SHARED_BUFFERS; ++i) {
        BiquadFilterSharedBuffers& shared_buffers = biquad_filter_shared_buffers[i];

        delete[] shared_buffers.b0_buffer;
        delete[] shared_buffers.b1_buffer;
        delete[] shared_buffers.b2_buffer;
        delete[] shared_buffers.a1_buffer;
        delete[] shared_buffers.a2_buffer;

        shared_buffers.b0_buffer = NULL;
        shared_buffers.b1_buffer = NULL;
        shared_buffers.b2_buffer = NULL;
        shared_buffers.a1_buffer = NULL;
        shared_buffers.a2_buffer = NULL;
    }
}


void Synth::reallocate_buffers() noexcept
{
    free_buffers();
    allocate_buffers();
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

    size_t const index = (size_t)param_id;

    if constexpr (std::is_same<ParamClass, FloatParamS>::value) {
        sample_evaluated_float_params[index] = &param;
    } else if constexpr (std::is_same<ParamClass, FloatParamB>::value) {
        block_evaluated_float_params[index] = &param;
    } else if constexpr (std::is_base_of<ByteParam, ParamClass>::value) {
        byte_params[index] = (ByteParam*)&param;
    }
}


Synth::ParamType Synth::find_param_type(ParamId const param_id) const noexcept
{
    size_t const index = (size_t)param_id;

    if (index >= (size_t)ParamId::INVALID_PARAM_ID) {
        return ParamType::INVALID_PARAM_TYPE;
    }

    if (sample_evaluated_float_params[index] != NULL) {
        return ParamType::SAMPLE_EVALUATED_FLOAT;
    }

    if (block_evaluated_float_params[index] != NULL) {
        return ParamType::BLOCK_EVALUATED_FLOAT;
    }

    if (byte_params[index] != NULL) {
        return ParamType::BYTE;
    }

    return ParamType::OTHER;
}


Synth::~Synth()
{
    for (Integer i = 0; i != POLYPHONY; ++i) {
        delete carriers[i];
        delete modulators[i];
        delete synced_oscillator_inaccuracies[i];
    }

    for (Byte i = 0; i != Constants::ENVELOPES; ++i) {
        delete envelopes[i];
    }

    for (Integer i = 0; i != MIDI_CONTROLLERS; ++i) {
        if (midi_controllers_rw[i] != NULL) {
            delete midi_controllers_rw[i];
        }
    }

    for (Byte i = 0; i != Constants::LFOS; ++i) {
        delete lfos_rw[i];
    }

    for (Integer i = 0; i != MACROS; ++i) {
        delete macros_rw[i];
    }

    free_buffers();
}


void Synth::set_sample_rate(Frequency const new_sample_rate) noexcept
{
    SignalProducer::set_sample_rate(new_sample_rate);

    samples_between_gc = std::max((Integer)5000, (Integer)(new_sample_rate * 0.2));
}


void Synth::set_block_size(Integer const new_block_size) noexcept
{
    if (new_block_size == this->block_size) {
        return;
    }

    SignalProducer::set_block_size(new_block_size);

    reallocate_buffers();
}



void Synth::reset() noexcept
{
    SignalProducer::reset();

    next_voice = 0;

    osc_1_peak_tracker.reset();
    osc_2_peak_tracker.reset();
    vol_1_peak_tracker.reset();
    vol_2_peak_tracker.reset();
    vol_3_peak_tracker.reset();
}


bool Synth::is_lock_free() const noexcept
{
    bool is_lock_free = true;

    for (int i = 0; is_lock_free && i != ParamId::PARAM_ID_COUNT; ++i) {
        is_lock_free = (
            param_ratios[i].is_lock_free()
            && controller_assignments[i].is_lock_free()
        );
    }

    return (
        is_lock_free
        && messages.is_lock_free()
        && is_mts_esp_connected_.is_lock_free()
    );
}


bool Synth::is_dirty() const noexcept
{
    return is_dirty_;
}


void Synth::clear_dirty_flag() noexcept
{
    is_dirty_ = false;
}


void Synth::suspend() noexcept
{
    stop_lfos();
    this->reset();
    clear_midi_controllers();
    clear_midi_note_to_voice_assignments();
    clear_sustain();
    note_stack.clear();
}


void Synth::stop_lfos() noexcept
{
    for (Byte i = 0; i != Constants::LFOS; ++i) {
        lfos_rw[i]->stop(0.0);
    }

    effects.chorus.stop_lfos(0.0);
}


void Synth::resume() noexcept
{
    this->reset();
    clear_midi_controllers();
    clear_midi_note_to_voice_assignments();
    start_lfos();
    clear_sustain();
    note_stack.clear();
}


bool Synth::has_mts_esp_tuning() const noexcept
{
    return (
        modulator_params.tuning.get_value() == Modulator::TUNING_MTS_ESP_NOTE_ON
        || carrier_params.tuning.get_value() == Carrier::TUNING_MTS_ESP_NOTE_ON
        || has_continuous_mts_esp_tuning()
    );
}


bool Synth::has_continuous_mts_esp_tuning() const noexcept
{
    return (
        modulator_params.tuning.get_value() == Modulator::TUNING_MTS_ESP_CONTINUOUS
        || carrier_params.tuning.get_value() == Carrier::TUNING_MTS_ESP_CONTINUOUS
    );
}


bool Synth::is_mts_esp_connected() const noexcept
{
    return is_mts_esp_connected_.load();
}


void Synth::mts_esp_connected() noexcept
{
    is_mts_esp_connected_.store(true);
}


void Synth::mts_esp_disconnected() noexcept
{
    is_mts_esp_connected_.store(false);
}


Synth::NoteTunings& Synth::collect_active_notes(Integer& active_notes_count) noexcept
{
    bus.collect_active_notes(active_note_tunings, active_notes_count);

    return active_note_tunings;
}


void Synth::update_note_tuning(NoteTuning const& note_tuning) noexcept
{
    if (note_tuning.is_valid() && note_tuning.frequency > 0.0) {
        per_channel_frequencies[note_tuning.channel][note_tuning.note] = note_tuning.frequency;
    }
}


void Synth::update_note_tunings(NoteTunings const& note_tunings, Integer const count) noexcept
{
    for (Integer i = 0; i != count; ++i) {
        update_note_tuning(note_tunings[i]);
    }
}


void Synth::start_lfos() noexcept
{
    for (Byte i = 0; i != Constants::LFOS; ++i) {
        lfos_rw[i]->start(0.0);
    }

    effects.chorus.start_lfos(0.0);
}


void Synth::note_on(
        Seconds const time_offset,
        Midi::Channel const channel,
        Midi::Note const note,
        Midi::Byte const velocity
) noexcept {
    Number const velocity_float = midi_byte_to_float(velocity);

    note_stack.push(channel, note, velocity_float);

    if (polyphonic.get_value() == ToggleParam::ON) {
        this->triggered_velocity.change(time_offset, velocity_float);
        this->triggered_note.change(time_offset, midi_byte_to_float(note));

        note_on_polyphonic(time_offset, channel, note, velocity_float);
    } else {
        note_on_monophonic(time_offset, channel, note, velocity_float, true);
    }
}


bool Synth::should_sync_oscillator_inaccuracy() const noexcept
{
    return should_sync_oscillator_inaccuracy(modulator_params, carrier_params);
}


bool Synth::should_sync_oscillator_instability() const noexcept
{
    return should_sync_oscillator_instability(modulator_params, carrier_params);
}


bool Synth::should_sync_oscillator_inaccuracy(
        Modulator::Params const& modulator_params,
        Carrier::Params const& carrier_params
) noexcept {
    return modulator_params.oscillator_inaccuracy.get_value() == carrier_params.oscillator_inaccuracy.get_value();
}


bool Synth::should_sync_oscillator_instability(
        Modulator::Params const& modulator_params,
        Carrier::Params const& carrier_params
) noexcept {
    return modulator_params.oscillator_instability.get_value() == carrier_params.oscillator_instability.get_value();
}


void Synth::note_on_polyphonic(
        Seconds const time_offset,
        Midi::Channel const channel,
        Midi::Note const note,
        Number const velocity
) noexcept {
    if (midi_note_to_voice_assignments[channel][note] != INVALID_VOICE) {
        return;
    }

    next_voice = (next_voice + 1) & VOICE_INDEX_MASK;

    for (Integer v = 0; v != POLYPHONY; ++v) {
        if (!(
                modulators[next_voice]->is_off_after(time_offset)
                && carriers[next_voice]->is_off_after(time_offset)
        )) {
            next_voice = (next_voice + 1) & VOICE_INDEX_MASK;
            continue;
        }

        trigger_note_on_voice(next_voice, time_offset, channel, note, velocity);

        break;
    }
}


void Synth::trigger_note_on_voice(
        Integer const voice,
        Seconds const time_offset,
        Midi::Channel const channel,
        Midi::Note const note,
        Number const velocity
) noexcept {
    assign_voice_and_note_id(voice, channel, note);

    Byte const mode = this->mode.get_value();
    bool const should_sync_oscillator_inaccuracy = this->should_sync_oscillator_inaccuracy();

    modulators[voice]->update_inaccuracy(cached_round);
    carriers[voice]->update_inaccuracy(cached_round);

    if (mode == MODE_MIX_AND_MOD) {
        modulators[voice]->note_on(
            time_offset, next_note_id, note, channel, velocity, previous_note, should_sync_oscillator_inaccuracy
        );
        carriers[voice]->note_on(
            time_offset, next_note_id, note, channel, velocity, previous_note, should_sync_oscillator_inaccuracy
        );
    } else {
        if (note < mode + Midi::NOTE_B_2) {
            modulators[voice]->note_on(
                time_offset, next_note_id, note, channel, velocity, previous_note, should_sync_oscillator_inaccuracy
            );
        } else {
            carriers[voice]->note_on(
                time_offset, next_note_id, note, channel, velocity, previous_note, should_sync_oscillator_inaccuracy
            );
        }
    }

    previous_note = note;
}


void Synth::assign_voice_and_note_id(
        Integer const voice,
        Midi::Channel const channel,
        Midi::Note const note
) noexcept {
    if (JS80P_UNLIKELY(previous_note > Midi::NOTE_MAX)) {
        previous_note = note;
    }

    midi_note_to_voice_assignments[channel][note] = voice;
    next_note_id = (next_note_id + 1) & NOTE_ID_MASK;
}


void Synth::note_on_monophonic(
        Seconds const time_offset,
        Midi::Channel const channel,
        Midi::Note const note,
        Number const velocity,
        bool const trigger_if_off
) noexcept {
    this->triggered_velocity.change(time_offset, velocity);
    this->triggered_note.change(time_offset, midi_byte_to_float(note));

    Modulator* const modulator = modulators[0];
    Carrier* const carrier = carriers[0];

    bool const modulator_off = modulator->is_off_after(time_offset);
    bool const carrier_off = carrier->is_off_after(time_offset);

    if (modulator_off && carrier_off) {
        if (trigger_if_off) {
            trigger_note_on_voice(0, time_offset, channel, note, velocity);
        }

        return;
    }

    assign_voice_and_note_id(0, channel, note);

    Byte const mode = this->mode.get_value();
    bool const should_sync_oscillator_inaccuracy = this->should_sync_oscillator_inaccuracy();

    modulator->update_inaccuracy(cached_round);
    carrier->update_inaccuracy(cached_round);

    if (mode == MODE_MIX_AND_MOD) {
        trigger_note_on_voice_monophonic<Modulator>(
            *modulator, modulator_off, time_offset, channel, note, velocity, should_sync_oscillator_inaccuracy
        );
        trigger_note_on_voice_monophonic<Carrier>(
            *carrier, carrier_off, time_offset, channel, note, velocity, should_sync_oscillator_inaccuracy
        );
    } else {
        if (note < mode + Midi::NOTE_B_2) {
            trigger_note_on_voice_monophonic<Modulator>(
                *modulator, modulator_off, time_offset, channel, note, velocity, should_sync_oscillator_inaccuracy
            );
            carrier->cancel_note_smoothly(time_offset);
        } else {
            modulator->cancel_note_smoothly(time_offset);
            trigger_note_on_voice_monophonic<Carrier>(
                *carrier, carrier_off, time_offset, channel, note, velocity, should_sync_oscillator_inaccuracy
            );
        }
    }

    previous_note = note;
}


template<class VoiceClass>
void Synth::trigger_note_on_voice_monophonic(
        VoiceClass& voice,
        bool const is_off,
        Seconds const time_offset,
        Midi::Channel const channel,
        Midi::Note const note,
        Number const velocity,
        bool const should_sync_oscillator_inaccuracy
) noexcept {
    if (is_off) {
        voice.note_on(
            time_offset, next_note_id, note, channel, velocity, previous_note, should_sync_oscillator_inaccuracy
        );
    } else if (voice.is_released()) {
        voice.retrigger(
            time_offset, next_note_id, note, channel, velocity, previous_note, should_sync_oscillator_inaccuracy
        );
    } else {
        voice.glide_to(
            time_offset, next_note_id, note, channel, velocity, previous_note, should_sync_oscillator_inaccuracy
        );
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
    // if (midi_note_to_voice_assignments[channel][note] == INVALID_VOICE) {
        // return;
    // }

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
    Integer const voice = midi_note_to_voice_assignments[channel][note];
    bool const was_note_stack_top = note_stack.is_top(channel, note);

    note_stack.remove(channel, note);

    if (voice == INVALID_VOICE) {
        return;
    }

    midi_note_to_voice_assignments[channel][note] = INVALID_VOICE;

    bool const is_polyphonic = polyphonic.get_value() == ToggleParam::ON;

    Modulator* const modulator = modulators[voice];
    Carrier* const carrier = carriers[voice];

    if (is_sustaining) {
        Integer note_id;

        if (modulator->is_on()) {
            note_id = modulator->get_note_id();
        } else {
            if (!carrier->is_on()) {
                return;
            }

            note_id = carriers[voice]->get_note_id();
        }

        if (is_polyphonic || was_note_stack_top) {
            deferred_note_offs.push_back(
                DeferredNoteOff(note_id, channel, note, velocity, voice)
            );
        }

        return;
    }

    Number const velocity_float = midi_byte_to_float(velocity);

    this->released_velocity.change(time_offset, velocity_float);
    this->released_note.change(time_offset, midi_byte_to_float(note));

    if (!is_polyphonic && was_note_stack_top && !note_stack.is_empty()) {
        Number previous_velocity;
        Midi::Channel previous_channel;
        Midi::Note previous_note;

        note_stack.top(previous_channel, previous_note, previous_velocity);

        note_on_monophonic(time_offset, previous_channel, previous_note, previous_velocity, false);

        return;
    }

    modulator->note_off(time_offset, modulator->get_note_id(), note, velocity_float);
    carrier->note_off(time_offset, carrier->get_note_id(), note, velocity_float);
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
        for (int i = 0; i != ParamId::PARAM_ID_COUNT; ++i) {
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
    bool const is_polyphonic = polyphonic.get_value() == ToggleParam::ON;
    is_sustaining = false;

    if (is_polyphonic || note_stack.is_empty()) {
        for (std::vector<DeferredNoteOff>::const_iterator it = deferred_note_offs.begin(); it != deferred_note_offs.end(); ++it) {
            DeferredNoteOff const& deferred_note_off = *it;
            Integer const voice = deferred_note_off.get_voice();

            if (JS80P_UNLIKELY(voice == INVALID_VOICE)) {
                /* This should never happen, but safety first! */
                JS80P_ASSERT_NOT_REACHED();
                continue;
            }

            Integer const note_id = deferred_note_off.get_note_id();
            Midi::Note const note = deferred_note_off.get_note();
            Number const velocity = midi_byte_to_float(deferred_note_off.get_velocity());

            this->released_velocity.change(time_offset, velocity);
            this->released_note.change(time_offset, midi_byte_to_float(note));

            modulators[voice]->note_off(time_offset, note_id, note, velocity);
            carriers[voice]->note_off(time_offset, note_id, note, velocity);
        }
    } else if (!is_polyphonic) {
        for (std::vector<DeferredNoteOff>::const_iterator it = deferred_note_offs.begin(); it != deferred_note_offs.end(); ++it) {
            DeferredNoteOff const& deferred_note_off = *it;
            Integer const note_id = deferred_note_off.get_note_id();

            bool const modulator_playing = modulators[0]->get_note_id() == note_id;
            bool const carrier_playing = carriers[0]->get_note_id() == note_id;

            if (modulator_playing || carrier_playing) {
                Number velocity;
                Number previous_velocity;
                Midi::Note note;
                Midi::Note previous_note;
                Midi::Channel previous_channel;

                if (modulator_playing) {
                    velocity = modulators[0]->get_velocity();
                    note = modulators[0]->get_note();
                } else {
                    velocity = carriers[0]->get_velocity();
                    note = carriers[0]->get_note();
                }

                this->released_velocity.change(time_offset, velocity);
                this->released_note.change(time_offset, midi_byte_to_float(note));

                note_stack.top(previous_channel, previous_note, previous_velocity);

                note_on_monophonic(time_offset, previous_channel, previous_note, previous_velocity, false);
            }
        }
    }

    deferred_note_offs.clear();
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
    return (
        (controller_id >= ControllerId::ENVELOPE_1 && controller_id <= ControllerId::ENVELOPE_6)
        || (controller_id >= ControllerId::ENVELOPE_7 && controller_id <= ControllerId::ENVELOPE_12)
    );
}


Number Synth::calculate_inaccuracy_seed(Integer const voice) noexcept
{
    constexpr Number scale = 1.0 / (Number)POLYPHONY;

    return OscillatorInaccuracy::calculate_new_inaccuracy(scale * (Number)voice);
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

            Modulator* const modulator = modulators[voice];

            modulator->note_off(time_offset, modulator->get_note_id(), note, 0.0);

            Carrier* const carrier = carriers[voice];

            carrier->note_off(time_offset, carrier->get_note_id(), note, 0.0);
        }
    }
}


void Synth::mono_mode_on(
        Seconds const time_offset,
        Midi::Channel const channel
) noexcept {
    polyphonic.set_value(ToggleParam::OFF);
    handle_refresh_param(ParamId::POLY);
}


void Synth::mono_mode_off(
        Seconds const time_offset,
        Midi::Channel const channel
) noexcept {
    polyphonic.set_value(ToggleParam::ON);
    handle_refresh_param(ParamId::POLY);
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


#ifdef JS80P_ASSERTIONS
void Synth::get_param_id_hash_table_statistics(
        Integer& max_collisions,
        Number& avg_collisions,
        Number& avg_bucket_size
) const noexcept {
    param_id_hash_table.get_statistics(
        max_collisions, avg_collisions, avg_bucket_size
    );
}
#endif


Number Synth::get_param_ratio_atomic(ParamId const param_id) const noexcept
{
    return param_ratios[param_id].load();
}


Number Synth::get_param_default_ratio(ParamId const param_id) const noexcept
{
    size_t const index = (size_t)param_id;
    ParamType const type = find_param_type(param_id);

    switch (type) {
        case ParamType::SAMPLE_EVALUATED_FLOAT:
            return sample_evaluated_float_params[index]->get_default_ratio();

        case ParamType::BLOCK_EVALUATED_FLOAT:
            return block_evaluated_float_params[index]->get_default_ratio();

        case ParamType::BYTE:
            return byte_params[index]->get_default_ratio();

        default:
            JS80P_ASSERT_NOT_REACHED();
            return 0.0;
    }
}


bool Synth::is_toggle_param(ParamId const param_id) const noexcept
{
    return param_id >= ParamId::L1SYN && param_id < ParamId::PARAM_ID_COUNT;
}


Number Synth::get_param_max_value(ParamId const param_id) const noexcept
{
    size_t const index = (size_t)param_id;
    ParamType const type = find_param_type(param_id);

    switch (type) {
        case ParamType::SAMPLE_EVALUATED_FLOAT:
            return sample_evaluated_float_params[index]->get_max_value();

        case ParamType::BLOCK_EVALUATED_FLOAT:
            return block_evaluated_float_params[index]->get_max_value();

        case ParamType::BYTE:
            return byte_params[index]->get_max_value();

        default:
            JS80P_ASSERT_NOT_REACHED();
            return 0.0;
    }
}


Number Synth::float_param_ratio_to_display_value(
        ParamId const param_id,
        Number const ratio
) const noexcept {
    size_t const index = (size_t)param_id;
    ParamType const type = find_param_type(param_id);

    switch (type) {
        case ParamType::SAMPLE_EVALUATED_FLOAT:
            return sample_evaluated_float_params[index]->ratio_to_value(ratio);

        case ParamType::BLOCK_EVALUATED_FLOAT:
            return block_evaluated_float_params[index]->ratio_to_value(ratio);

        default:
            JS80P_ASSERT_NOT_REACHED();
            return 0.0;
    }
}


Byte Synth::byte_param_ratio_to_display_value(
        ParamId const param_id,
        Number const ratio
) const noexcept {
    size_t const index = (size_t)param_id;
    ParamType const type = find_param_type(param_id);

    switch (type) {
        case ParamType::BYTE:
            return byte_params[index]->ratio_to_value(ratio);

        default:
            JS80P_ASSERT_NOT_REACHED();
            return 0;
    }
}


Synth::ControllerId Synth::get_param_controller_id_atomic(
        ParamId const param_id
) const noexcept {
    return (ControllerId)controller_assignments[param_id].load();
}


void Synth::update_param_states() noexcept
{
    for (int i = 0; i != ParamId::PARAM_ID_COUNT; ++i) {
        handle_refresh_param((ParamId)i);
    }
}


Sample const* const* Synth::initialize_rendering(
        Integer const round,
        Integer const sample_count
) noexcept {
    process_messages();

    was_polyphonic = is_polyphonic;
    is_polyphonic = polyphonic.get_value() == ToggleParam::ON;

    if (was_polyphonic && !is_polyphonic) {
        stop_polyphonic_notes();
    }

    samples_since_gc += sample_count;

    if (samples_since_gc > samples_between_gc) {
        garbage_collect_voices();
        samples_since_gc = 0;
    }

    raw_output = SignalProducer::produce< Effects::Effects<Bus> >(
        effects, round, sample_count
    );

    for (int i = 0; i != (int)ParamId::EV3V; ++i) {
        if (sample_evaluated_float_params[i] != NULL) {
            FloatParamS::produce_if_not_constant(
                *sample_evaluated_float_params[i], round, sample_count
            );
        } else if (block_evaluated_float_params[i] != NULL) {
            FloatParamB::produce_if_not_constant(
                *block_evaluated_float_params[i], round, sample_count
            );
        }
    }

    for (Byte i = 0; i != Constants::LFOS; ++i) {
        lfos_rw[i]->skip_round(round, sample_count);
    }

    effects.chorus.skip_round_for_lfos(round, sample_count);

    clear_midi_controllers();

    return NULL;
}


void Synth::stop_polyphonic_notes() noexcept
{
    bool found_note = false;
    Midi::Channel channel = 0;
    Midi::Note note = 0;

    for (Integer voice = 1; voice != POLYPHONY; ++voice) {
        found_note = false;

        Modulator* const modulator = modulators[voice];

        if (modulator->is_on()) {
            note = modulator->get_note();
            channel = modulator->get_channel();
            found_note = true;
            modulator->note_off(0.0, modulator->get_note_id(), note, 0.0);
        }

        Carrier* const carrier = carriers[voice];

        if (carrier->is_on()) {
            note = carrier->get_note();
            channel = carrier->get_channel();
            found_note = true;
            carrier->note_off(0.0, carrier->get_note_id(), note, 0.0);
        }

        if (found_note) {
            midi_note_to_voice_assignments[channel][note] = INVALID_VOICE;
        }
    }
}


void Synth::garbage_collect_voices() noexcept
{
    for (Integer voice = 0; voice != POLYPHONY; ++voice) {
        Midi::Channel channel;
        Midi::Note note;

        Modulator* const modulator = modulators[voice];
        bool const modulator_decayed = modulator->has_decayed_before_note_off();

        if (modulator_decayed) {
            note = modulator->get_note();
            channel = modulator->get_channel();
            modulator->cancel_note();
        }

        Carrier* const carrier = carriers[voice];
        bool const carrier_decayed = carrier->has_decayed_before_note_off();

        if (carrier_decayed) {
            note = carrier->get_note();
            channel = carrier->get_channel();
            carrier->cancel_note();
        }

        if (modulator_decayed && carrier_decayed) {
            Integer const assigned = midi_note_to_voice_assignments[channel][note];

            /*
            The note's key might have been released and triggered again while
            the sustain pedal was engaged. If that's the case, then it is
            assigned to a different voice which was free at the time. If so, we
            don't want to de-assign that other voice.
            */

            if (voice == assigned) {
                midi_note_to_voice_assignments[channel][note] = INVALID_VOICE;
            }
        }
    }
}


void Synth::process_messages() noexcept
{
    SPSCQueue<Message>::SizeType const message_count = messages.length();

    for (SPSCQueue<Message>::SizeType i = 0; i != message_count; ++i) {
        Message message;

        if (messages.pop(message)) {
            process_message(message);
        }
    }
}


void Synth::process_message(
        MessageType const type,
        ParamId const param_id,
        Number const number_param,
        Byte const byte_param
) noexcept {
    Message message(type, param_id, number_param, byte_param);

    process_message(message);
}


void Synth::process_message(Message const& message) noexcept
{
    switch (message.type) {
        case MessageType::SET_PARAM:
            handle_set_param(message.param_id, message.number_param);
            is_dirty_ = true;
            break;

        case MessageType::ASSIGN_CONTROLLER:
            handle_assign_controller(message.param_id, message.byte_param);
            is_dirty_ = true;
            break;

        case MessageType::REFRESH_PARAM:
            handle_refresh_param(message.param_id);
            break;

        case MessageType::CLEAR:
            handle_clear();
            is_dirty_ = true;
            break;

        case MessageType::CLEAR_DIRTY_FLAG:
            is_dirty_ = false;
            break;

        default:
            break;
    }
}


void Synth::handle_set_param(ParamId const param_id, Number const ratio) noexcept
{
    size_t const index = (size_t)param_id;
    ParamType const type = find_param_type(param_id);

    switch (type) {
        case ParamType::SAMPLE_EVALUATED_FLOAT:
            sample_evaluated_float_params[index]->set_ratio(ratio);
            break;

        case ParamType::BLOCK_EVALUATED_FLOAT:
            block_evaluated_float_params[index]->set_ratio(ratio);
            break;

        case ParamType::BYTE:
            byte_params[index]->set_ratio(ratio);
            break;

        default:
            JS80P_ASSERT_NOT_REACHED();
            break;
    }

    handle_refresh_param(param_id);
}


void Synth::handle_assign_controller(
        ParamId const param_id,
        Byte const controller_id
) noexcept {
    ControllerId const ctl_id = (ControllerId)controller_id;
    size_t const index = (size_t)param_id;
    ParamType const type = find_param_type(param_id);
    bool is_assigned = false;

    if (!may_be_controllable(param_id, type)) {
        return;
    }

    switch (type) {
        case ParamType::SAMPLE_EVALUATED_FLOAT:
            is_assigned = assign_controller<FloatParamS>(
                *sample_evaluated_float_params[index], ctl_id
            );
            break;

        case ParamType::BLOCK_EVALUATED_FLOAT:
            is_assigned = assign_controller<FloatParamB>(
                *block_evaluated_float_params[index], ctl_id
            );
            break;

        case ParamType::BYTE:
            is_assigned = assign_controller_to_byte_param(param_id, ctl_id);
            break;

        default:
            JS80P_ASSERT_NOT_REACHED();
            break;
    }

    if (!is_assigned) {
        return;
    }

    controller_assignments[param_id].store(controller_id);

    if ((ControllerId)controller_id == ControllerId::MIDI_LEARN) {
        is_learning = true;
    }
}


bool Synth::may_be_controllable(
        ParamId const param_id,
        ParamType const type
) const noexcept {
    switch (type) {
        case ParamType::OTHER:
        case ParamType::INVALID_PARAM_TYPE:
            return false;

        case ParamType::BYTE:
            /*
            The assign_controller_to_byte_param() method ignores those that are
            not intended to be controlled.
            */
            return true;

        default:
            break;
    }

    switch (param_id) {
        case ParamId::MF1FIA:
        case ParamId::MF1QIA:
        case ParamId::MF2FIA:
        case ParamId::MF2QIA:
        case ParamId::CF1FIA:
        case ParamId::CF1QIA:
        case ParamId::CF2FIA:
        case ParamId::CF2QIA:
        case ParamId::N1TIN:
        case ParamId::N1VIN:
        case ParamId::N2TIN:
        case ParamId::N2VIN:
        case ParamId::N3TIN:
        case ParamId::N3VIN:
        case ParamId::N4TIN:
        case ParamId::N4VIN:
        case ParamId::N5TIN:
        case ParamId::N5VIN:
        case ParamId::N6TIN:
        case ParamId::N6VIN:
        case ParamId::N7TIN:
        case ParamId::N7VIN:
        case ParamId::N8TIN:
        case ParamId::N8VIN:
        case ParamId::N9TIN:
        case ParamId::N9VIN:
        case ParamId::N10TIN:
        case ParamId::N10VIN:
        case ParamId::N11TIN:
        case ParamId::N11VIN:
        case ParamId::N12TIN:
        case ParamId::N12VIN:
            return false;

        default:
            return true;
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

    for (int i = 0; i != ParamId::PARAM_ID_COUNT; ++i) {
        ParamId const param_id = (ParamId)i;

        handle_assign_controller(param_id, no_controller);

        if (param_id != ParamId::MTUN && param_id != ParamId::CTUN) {
            handle_set_param(param_id, get_param_default_ratio(param_id));
        }
    }
}


bool Synth::assign_controller_to_byte_param(
        ParamId const param_id,
        ControllerId const controller_id
) noexcept {
    MidiController* midi_controller = NULL;
    Macro* macro = NULL;
    bool is_assigned = false;
    bool is_special = false;

    switch (controller_id) {
        case NONE: is_special = true; break;

        case PITCH_WHEEL: midi_controller = &pitch_wheel; break;

        case TRIGGERED_NOTE: midi_controller = &triggered_note; break;
        case TRIGGERED_VELOCITY: midi_controller = &triggered_velocity; break;

        case MACRO_1: macro = macros[0]; break;
        case MACRO_2: macro = macros[1]; break;
        case MACRO_3: macro = macros[2]; break;
        case MACRO_4: macro = macros[3]; break;
        case MACRO_5: macro = macros[4]; break;
        case MACRO_6: macro = macros[5]; break;
        case MACRO_7: macro = macros[6]; break;
        case MACRO_8: macro = macros[7]; break;
        case MACRO_9: macro = macros[8]; break;
        case MACRO_10: macro = macros[9]; break;

        case LFO_1:
        case LFO_2:
        case LFO_3:
        case LFO_4:
        case LFO_5:
        case LFO_6:
        case LFO_7:
        case LFO_8:
            break;

        case ENVELOPE_1:
        case ENVELOPE_2:
        case ENVELOPE_3:
        case ENVELOPE_4:
        case ENVELOPE_5:
        case ENVELOPE_6:
            break;

        case CHANNEL_PRESSURE: break;

        case MIDI_LEARN: is_special = true; break;

        case MACRO_11: macro = macros[10]; break;
        case MACRO_12: macro = macros[11]; break;
        case MACRO_13: macro = macros[12]; break;
        case MACRO_14: macro = macros[13]; break;
        case MACRO_15: macro = macros[14]; break;
        case MACRO_16: macro = macros[15]; break;
        case MACRO_17: macro = macros[16]; break;
        case MACRO_18: macro = macros[17]; break;
        case MACRO_19: macro = macros[18]; break;
        case MACRO_20: macro = macros[19]; break;

        case OSC_1_PEAK: midi_controller = &osc_1_peak; break;
        case OSC_2_PEAK: midi_controller = &osc_2_peak; break;
        case VOL_1_PEAK: midi_controller = &vol_1_peak; break;
        case VOL_2_PEAK: midi_controller = &vol_2_peak; break;
        case VOL_3_PEAK: midi_controller = &vol_3_peak; break;

        case ENVELOPE_7:
        case ENVELOPE_8:
        case ENVELOPE_9:
        case ENVELOPE_10:
        case ENVELOPE_11:
        case ENVELOPE_12:
            break;

        case RELEASED_NOTE: midi_controller = &released_note; break;
        case RELEASED_VELOCITY: midi_controller = &released_velocity; break;

        case MACRO_21: macro = macros[20]; break;
        case MACRO_22: macro = macros[21]; break;
        case MACRO_23: macro = macros[22]; break;
        case MACRO_24: macro = macros[23]; break;
        case MACRO_25: macro = macros[24]; break;
        case MACRO_26: macro = macros[25]; break;
        case MACRO_27: macro = macros[26]; break;
        case MACRO_28: macro = macros[27]; break;
        case MACRO_29: macro = macros[28]; break;
        case MACRO_30: macro = macros[29]; break;

        default: {
            if (is_supported_midi_controller(controller_id)) {
                midi_controller = midi_controllers[controller_id];
            }

            break;
        }
    }

    switch (param_id) {
        case ParamId::MODE:
            mode.set_midi_controller(midi_controller);
            mode.set_macro(macro);
            is_assigned = true;
            break;

        case ParamId::MWAV:
            modulator_params.waveform.set_midi_controller(midi_controller);
            modulator_params.waveform.set_macro(macro);
            is_assigned = true;
            break;

        case ParamId::CWAV:
            carrier_params.waveform.set_midi_controller(midi_controller);
            carrier_params.waveform.set_macro(macro);
            is_assigned = true;
            break;

        case ParamId::MF1TYP:
            modulator_params.filter_1_type.set_midi_controller(midi_controller);
            modulator_params.filter_1_type.set_macro(macro);
            is_assigned = true;
            break;

        case ParamId::MF2TYP:
            modulator_params.filter_2_type.set_midi_controller(midi_controller);
            modulator_params.filter_2_type.set_macro(macro);
            is_assigned = true;
            break;

        case ParamId::CF1TYP:
            carrier_params.filter_1_type.set_midi_controller(midi_controller);
            carrier_params.filter_1_type.set_macro(macro);
            is_assigned = true;
            break;

        case ParamId::CF2TYP:
            carrier_params.filter_2_type.set_midi_controller(midi_controller);
            carrier_params.filter_2_type.set_macro(macro);
            is_assigned = true;
            break;

        case ParamId::EF1TYP:
            effects.filter_1_type.set_midi_controller(midi_controller);
            effects.filter_1_type.set_macro(macro);
            is_assigned = true;
            break;

        case ParamId::EF2TYP:
            effects.filter_2_type.set_midi_controller(midi_controller);
            effects.filter_2_type.set_macro(macro);
            is_assigned = true;
            break;

        case ParamId::L1WAV:
            lfos_rw[0]->waveform.set_midi_controller(midi_controller);
            lfos_rw[0]->waveform.set_macro(macro);
            is_assigned = true;
            break;

        case ParamId::L2WAV:
            lfos_rw[1]->waveform.set_midi_controller(midi_controller);
            lfos_rw[1]->waveform.set_macro(macro);
            is_assigned = true;
            break;

        case ParamId::L3WAV:
            lfos_rw[2]->waveform.set_midi_controller(midi_controller);
            lfos_rw[2]->waveform.set_macro(macro);
            is_assigned = true;
            break;

        case ParamId::L4WAV:
            lfos_rw[3]->waveform.set_midi_controller(midi_controller);
            lfos_rw[3]->waveform.set_macro(macro);
            is_assigned = true;
            break;

        case ParamId::L5WAV:
            lfos_rw[4]->waveform.set_midi_controller(midi_controller);
            lfos_rw[4]->waveform.set_macro(macro);
            is_assigned = true;
            break;

        case ParamId::L6WAV:
            lfos_rw[5]->waveform.set_midi_controller(midi_controller);
            lfos_rw[5]->waveform.set_macro(macro);
            is_assigned = true;
            break;

        case ParamId::L7WAV:
            lfos_rw[6]->waveform.set_midi_controller(midi_controller);
            lfos_rw[6]->waveform.set_macro(macro);
            is_assigned = true;
            break;

        case ParamId::L8WAV:
            lfos_rw[7]->waveform.set_midi_controller(midi_controller);
            lfos_rw[7]->waveform.set_macro(macro);
            is_assigned = true;
            break;

        case ParamId::ERTYP:
            effects.reverb.type.set_midi_controller(midi_controller);
            effects.reverb.type.set_macro(macro);
            is_assigned = true;
            break;

        case ParamId::ECTYP:
            effects.chorus.type.set_midi_controller(midi_controller);
            effects.chorus.type.set_macro(macro);
            is_assigned = true;
            break;

        default:
            break;
    }

    return is_assigned && (is_special || midi_controller != NULL || macro != NULL);
}


template<class FloatParamClass>
bool Synth::assign_controller(
        FloatParamClass& param,
        ControllerId const controller_id
) noexcept {
    param.set_midi_controller(NULL);
    param.set_macro(NULL);
    param.set_envelope(NULL);
    param.set_lfo(NULL);

    switch (controller_id) {
        case NONE: return true;

        case PITCH_WHEEL: param.set_midi_controller(&pitch_wheel); return true;

        case TRIGGERED_NOTE: param.set_midi_controller(&triggered_note); return true;
        case TRIGGERED_VELOCITY: param.set_midi_controller(&triggered_velocity); return true;

        case MACRO_1: param.set_macro(macros[0]); return true;
        case MACRO_2: param.set_macro(macros[1]); return true;
        case MACRO_3: param.set_macro(macros[2]); return true;
        case MACRO_4: param.set_macro(macros[3]); return true;
        case MACRO_5: param.set_macro(macros[4]); return true;
        case MACRO_6: param.set_macro(macros[5]); return true;
        case MACRO_7: param.set_macro(macros[6]); return true;
        case MACRO_8: param.set_macro(macros[7]); return true;
        case MACRO_9: param.set_macro(macros[8]); return true;
        case MACRO_10: param.set_macro(macros[9]); return true;

        case LFO_1: param.set_lfo(lfos_rw[0]); return true;
        case LFO_2: param.set_lfo(lfos_rw[1]); return true;
        case LFO_3: param.set_lfo(lfos_rw[2]); return true;
        case LFO_4: param.set_lfo(lfos_rw[3]); return true;
        case LFO_5: param.set_lfo(lfos_rw[4]); return true;
        case LFO_6: param.set_lfo(lfos_rw[5]); return true;
        case LFO_7: param.set_lfo(lfos_rw[6]); return true;
        case LFO_8: param.set_lfo(lfos_rw[7]); return true;

        case ENVELOPE_1: param.set_envelope(envelopes[0]); return true;
        case ENVELOPE_2: param.set_envelope(envelopes[1]); return true;
        case ENVELOPE_3: param.set_envelope(envelopes[2]); return true;
        case ENVELOPE_4: param.set_envelope(envelopes[3]); return true;
        case ENVELOPE_5: param.set_envelope(envelopes[4]); return true;
        case ENVELOPE_6: param.set_envelope(envelopes[5]); return true;

        case CHANNEL_PRESSURE: param.set_midi_controller(&channel_pressure_ctl); return true;

        case MIDI_LEARN: return true;

        case MACRO_11: param.set_macro(macros[10]); return true;
        case MACRO_12: param.set_macro(macros[11]); return true;
        case MACRO_13: param.set_macro(macros[12]); return true;
        case MACRO_14: param.set_macro(macros[13]); return true;
        case MACRO_15: param.set_macro(macros[14]); return true;
        case MACRO_16: param.set_macro(macros[15]); return true;
        case MACRO_17: param.set_macro(macros[16]); return true;
        case MACRO_18: param.set_macro(macros[17]); return true;
        case MACRO_19: param.set_macro(macros[18]); return true;
        case MACRO_20: param.set_macro(macros[19]); return true;

        case OSC_1_PEAK: param.set_midi_controller(&osc_1_peak); return true;
        case OSC_2_PEAK: param.set_midi_controller(&osc_2_peak); return true;
        case VOL_1_PEAK: param.set_midi_controller(&vol_1_peak); return true;
        case VOL_2_PEAK: param.set_midi_controller(&vol_2_peak); return true;
        case VOL_3_PEAK: param.set_midi_controller(&vol_3_peak); return true;

        case ENVELOPE_7: param.set_envelope(envelopes[6]); return true;
        case ENVELOPE_8: param.set_envelope(envelopes[7]); return true;
        case ENVELOPE_9: param.set_envelope(envelopes[8]); return true;
        case ENVELOPE_10: param.set_envelope(envelopes[9]); return true;
        case ENVELOPE_11: param.set_envelope(envelopes[10]); return true;
        case ENVELOPE_12: param.set_envelope(envelopes[11]); return true;

        case RELEASED_NOTE: param.set_midi_controller(&released_note); return true;
        case RELEASED_VELOCITY: param.set_midi_controller(&released_velocity); return true;

        case MACRO_21: param.set_macro(macros[20]); return true;
        case MACRO_22: param.set_macro(macros[21]); return true;
        case MACRO_23: param.set_macro(macros[22]); return true;
        case MACRO_24: param.set_macro(macros[23]); return true;
        case MACRO_25: param.set_macro(macros[24]); return true;
        case MACRO_26: param.set_macro(macros[25]); return true;
        case MACRO_27: param.set_macro(macros[26]); return true;
        case MACRO_28: param.set_macro(macros[27]); return true;
        case MACRO_29: param.set_macro(macros[28]); return true;
        case MACRO_30: param.set_macro(macros[29]); return true;

        default: {
            if (is_supported_midi_controller(controller_id)) {
                param.set_midi_controller(midi_controllers_rw[controller_id]);

                return true;
            }

            return false;
        }
    }
}


Number Synth::get_param_ratio(ParamId const param_id) const noexcept
{
    size_t const index = (size_t)param_id;
    ParamType const type = find_param_type(param_id);

    switch (type) {
        case ParamType::SAMPLE_EVALUATED_FLOAT:
            return sample_evaluated_float_params[index]->get_ratio();

        case ParamType::BLOCK_EVALUATED_FLOAT:
            return block_evaluated_float_params[index]->get_ratio();

        case ParamType::BYTE:
            return byte_params[index]->get_ratio();

        default:
            JS80P_ASSERT_NOT_REACHED();
            return 0.0;
    }
}


void Synth::clear_midi_controllers() noexcept
{
    pitch_wheel.clear();
    triggered_note.clear();
    released_note.clear();
    triggered_velocity.clear();
    released_velocity.clear();
    channel_pressure_ctl.clear();
    osc_1_peak.clear();
    osc_2_peak.clear();
    vol_1_peak.clear();
    vol_2_peak.clear();
    vol_3_peak.clear();

    for (Integer i = 0; i != MIDI_CONTROLLERS; ++i) {
        if (midi_controllers_rw[i] != NULL) {
            midi_controllers_rw[i]->clear();
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
    deferred_note_offs.clear();
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


void Synth::finalize_rendering(
        Integer const round,
        Integer const sample_count
) noexcept {
    Seconds const sampling_period = this->sampling_period;

    Sample peak;
    Integer peak_index;

    if (osc_1_peak.is_assigned()) {
        bus.find_modulators_peak(sample_count, peak, peak_index);
        osc_1_peak_tracker.update(peak, peak_index, sample_count, sampling_period);
        osc_1_peak.change(0.0, std::min(1.0, osc_1_peak_tracker.get_peak()));
    }

    if (osc_2_peak.is_assigned()) {
        bus.find_carriers_peak(sample_count, peak, peak_index);
        osc_2_peak_tracker.update(peak, peak_index, sample_count, sampling_period);
        osc_2_peak.change(0.0, std::min(1.0, osc_2_peak_tracker.get_peak()));
    }

    if (vol_1_peak.is_assigned()) {
        effects.volume_1.find_input_peak(round, sample_count, peak, peak_index);
        vol_1_peak_tracker.update(peak, peak_index, sample_count, sampling_period);
        vol_1_peak.change(0.0, std::min(1.0, vol_1_peak_tracker.get_peak()));
    }

    if (vol_2_peak.is_assigned()) {
        effects.volume_2.find_input_peak(round, sample_count, peak, peak_index);
        vol_2_peak_tracker.update(peak, peak_index, sample_count, sampling_period);
        vol_2_peak.change(0.0, std::min(1.0, vol_2_peak_tracker.get_peak()));
    }

    if (vol_3_peak.is_assigned()) {
        effects.volume_3.find_input_peak(round, sample_count, peak, peak_index);
        vol_3_peak_tracker.update(peak, peak_index, sample_count, sampling_period);
        vol_3_peak.change(0.0, std::min(1.0, vol_3_peak_tracker.get_peak()));
    }
}


std::string const Synth::to_string(Integer const n) const noexcept
{
    std::ostringstream s;

    s << n;

    return s.str();
}


Synth::Message::Message() noexcept
    : type(MessageType::INVALID_MESSAGE_TYPE),
    param_id(ParamId::INVALID_PARAM_ID),
    number_param(0.0),
    byte_param(0)
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


Synth::Bus::Bus(
        Integer const channels,
        Modulator* const* const modulators,
        Modulator::Params const& modulator_params,
        Carrier* const* const carriers,
        Carrier::Params const& carrier_params,
        Integer const polyphony,
        FloatParamS& modulator_add_volume
) noexcept
    : SignalProducer(channels, 0),
    polyphony(polyphony),
    modulators(modulators),
    carriers(carriers),
    modulator_params(modulator_params),
    carrier_params(carrier_params),
    modulator_add_volume(modulator_add_volume),
    modulators_buffer(NULL),
    carriers_buffer(NULL)
{
    allocate_buffers();
}


Synth::Bus::~Bus()
{
    free_buffers();
}


void Synth::Bus::allocate_buffers() noexcept
{
    modulators_buffer = allocate_buffer();
    carriers_buffer = allocate_buffer();
}


void Synth::Bus::free_buffers() noexcept
{
    modulators_buffer = free_buffer(modulators_buffer);
    carriers_buffer = free_buffer(carriers_buffer);
}


void Synth::Bus::set_block_size(Integer const new_block_size) noexcept
{
    if (new_block_size != this->block_size) {
        SignalProducer::set_block_size(new_block_size);

        reallocate_buffers();
    }
}


void Synth::Bus::find_modulators_peak(
        Integer const sample_count,
        Sample& peak,
        Integer& peak_index
) noexcept {
    SignalProducer::find_peak(modulators_buffer, this->channels, sample_count, peak, peak_index);
}


void Synth::Bus::find_carriers_peak(
        Integer const sample_count,
        Sample& peak,
        Integer& peak_index
) noexcept {
    SignalProducer::find_peak(carriers_buffer, this->channels, sample_count, peak, peak_index);
}


void Synth::Bus::collect_active_notes(
        NoteTunings& note_tunings,
        Integer& note_tunings_count
) noexcept {
    Integer i = 0;

    for (Integer v = 0; v != polyphony; ++v) {
        if (!modulators[v]->is_released() && modulators[v]->is_on()) {
            note_tunings[i] = NoteTuning(
                modulators[v]->get_channel(), modulators[v]->get_note()
            );
            ++i;
        } else if (!carriers[v]->is_released() && carriers[v]->is_on()) {
            note_tunings[i] = NoteTuning(
                carriers[v]->get_channel(), carriers[v]->get_note()
            );
            ++i;
        }
    }

    note_tunings_count = i;
}


void Synth::Bus::reallocate_buffers() noexcept
{
    free_buffers();
    allocate_buffers();
}


Sample const* const* Synth::Bus::initialize_rendering(
        Integer const round,
        Integer const sample_count
) noexcept {
    collect_active_voices();

    modulator_add_volume_buffer = FloatParamS::produce_if_not_constant(
        modulator_add_volume, round, sample_count
    );

    render_silence(round, 0, sample_count, modulators_buffer);
    render_silence(round, 0, sample_count, carriers_buffer);
    render_silence(round, 0, sample_count, buffer);

    if (Synth::should_sync_oscillator_inaccuracy(modulator_params, carrier_params)) {
        if (Synth::should_sync_oscillator_instability(modulator_params, carrier_params)) {
            render_voices<Modulator, true, true>(active_modulators, active_modulators_count, modulator_params, round, sample_count);
            render_voices<Carrier, true, true>(active_carriers, active_carriers_count, carrier_params, round, sample_count);
        } else {
            render_voices<Modulator, true, false>(active_modulators, active_modulators_count, modulator_params, round, sample_count);
            render_voices<Carrier, true, false>(active_carriers, active_carriers_count, carrier_params, round, sample_count);
        }
    } else {
        if (Synth::should_sync_oscillator_instability(modulator_params, carrier_params)) {
            render_voices<Modulator, false, true>(active_modulators, active_modulators_count, modulator_params, round, sample_count);
            render_voices<Carrier, false, true>(active_carriers, active_carriers_count, carrier_params, round, sample_count);
        } else {
            render_voices<Modulator, false, false>(active_modulators, active_modulators_count, modulator_params, round, sample_count);
            render_voices<Carrier, false, false>(active_carriers, active_carriers_count, carrier_params, round, sample_count);
        }
    }

    if (active_modulators_count == 0 && active_carriers_count == 0) {
        mark_round_as_silent(round);

        return buffer;
    }

    return NULL;
}


void Synth::Bus::collect_active_voices() noexcept
{
    active_modulators_count = 0;
    active_carriers_count = 0;

    for (Integer v = 0; v != polyphony; ++v) {
        if (modulators[v]->is_on()) {
            active_modulators[active_modulators_count] = modulators[v];
            ++active_modulators_count;
        }

        if (carriers[v]->is_on()) {
            active_carriers[active_carriers_count] = carriers[v];
            ++active_carriers_count;
        }
    }
}


template<class VoiceClass, bool should_sync_oscillator_inaccuracy, bool should_sync_oscillator_instability>
void Synth::Bus::render_voices(
        VoiceClass* (&voices)[POLYPHONY],
        size_t const voices_count,
        typename VoiceClass::Params const& params,
        Integer const round,
        Integer const sample_count
) noexcept {
    if (voices_count > 0) {
        /*
        Rendering oscillators together seems to be more cache-friendly. Cannot group
        modulators and carriers though, because when there is actual modulation,
        then rendering carrier oscillators would trigger rendering the whole signal
        chain of the corresponding modulator.
        */

        if (params.tuning.get_value() == VoiceClass::TUNING_MTS_ESP_CONTINUOUS) {
            for (size_t v = 0; v != voices_count; ++v) {
                voices[v]->template update_note_frequency_for_continuous_mts_esp<should_sync_oscillator_inaccuracy, should_sync_oscillator_instability>(round);
            }
        }

        if (params.oscillator_instability.get_value() != 0) {
            for (size_t v = 0; v != voices_count; ++v) {
                voices[v]->template update_unstable_note_frequency<should_sync_oscillator_instability>(round);
            }
        }

        for (size_t v = 0; v != voices_count; ++v) {
            voices[v]->render_oscillator(round, sample_count);
        }

        for (size_t v = 0; v != voices_count; ++v) {
            SignalProducer::produce<VoiceClass>(*voices[v], round, sample_count);
        }
    }
}


void Synth::Bus::render(
        Integer const round,
        Integer const first_sample_index,
        Integer const last_sample_index,
        Sample** buffer
) noexcept {
    mix_modulators(round, first_sample_index, last_sample_index);
    mix_carriers(round, first_sample_index, last_sample_index);

    for (Integer c = 0; c != channels; ++c) {
        for (Integer i = first_sample_index; i != last_sample_index; ++i) {
            buffer[c][i] = modulators_buffer[c][i] + carriers_buffer[c][i];
        }
    }
}


void Synth::Bus::mix_modulators(
        Integer const round,
        Integer const first_sample_index,
        Integer const last_sample_index
) noexcept {
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

        mix_modulators<true>(
            round,
            first_sample_index,
            last_sample_index,
            modulator_add_volume_value,
            NULL
        );
    } else {
        mix_modulators<false>(
            round,
            first_sample_index,
            last_sample_index,
            1.0,
            modulator_add_volume_buffer
        );
    }
}


template<bool is_additive_volume_constant>
void Synth::Bus::mix_modulators(
        Integer const round,
        Integer const first_sample_index,
        Integer const last_sample_index,
        Sample const add_volume_value,
        Sample const* add_volume_buffer
) noexcept {
    for (size_t v = 0; v != active_modulators_count; ++v) {
        /*
        Rendering was done during Synth::Bus::initialize_rendering(), we're
        just retrieving the cached buffer now.
        */
        Sample const* const* const modulator_output = (
            SignalProducer::produce<Modulator>(*active_modulators[v], round)
        );

        for (Integer c = 0; c != channels; ++c) {
            for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                if constexpr (is_additive_volume_constant) {
                    modulators_buffer[c][i] += add_volume_value * modulator_output[c][i];
                } else {
                    modulators_buffer[c][i] += add_volume_buffer[i] * modulator_output[c][i];
                }
            }
        }
    }
}


void Synth::Bus::mix_carriers(
        Integer const round,
        Integer const first_sample_index,
        Integer const last_sample_index
) noexcept {
    for (size_t v = 0; v != active_carriers_count; ++v) {
        /*
        Rendering was done during Synth::Bus::initialize_rendering(), we're
        just retrieving the cached buffer now.
        */
        Sample const* const* const carrier_output = (
            SignalProducer::produce<Carrier>(*active_carriers[v], round)
        );

        for (Integer c = 0; c != channels; ++c) {
            for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                carriers_buffer[c][i] += carrier_output[c][i];
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

    if ((*root)->param_id == ParamId::INVALID_PARAM_ID) {
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

    return entry == NULL ? ParamId::INVALID_PARAM_ID : entry->param_id;
}


#ifdef JS80P_ASSERTIONS
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

        if (entry->param_id == ParamId::INVALID_PARAM_ID) {
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
#endif


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
    set("", ParamId::INVALID_PARAM_ID);
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


Synth::DeferredNoteOff::DeferredNoteOff()
    : voice(INVALID_VOICE),
    note_id(0),
    channel(0),
    note(0),
    velocity(0)
{
}


Synth::DeferredNoteOff::DeferredNoteOff(
        Integer const note_id,
        Midi::Channel const channel,
        Midi::Note const note,
        Midi::Byte const velocity,
        Integer const voice
) : voice(voice), note_id(note_id), channel(channel), note(note), velocity(velocity)
{
}


Integer Synth::DeferredNoteOff::get_note_id() const noexcept
{
    return note_id;
}


Midi::Channel Synth::DeferredNoteOff::get_channel() const noexcept
{
    return channel;
}


Midi::Note Synth::DeferredNoteOff::get_note() const noexcept
{
    return note;
}


Midi::Byte Synth::DeferredNoteOff::get_velocity() const noexcept
{
    return velocity;
}


Integer Synth::DeferredNoteOff::get_voice() const noexcept
{
    return voice;
}

}
