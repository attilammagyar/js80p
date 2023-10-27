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
#include "dsp/gain.cpp"
#include "dsp/lfo.cpp"
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

std::vector<bool> Synth::supported_midi_controllers(Synth::MIDI_CONTROLLERS, false);

bool Synth::supported_midi_controllers_initialized = false;


Synth::ParamIdHashTable Synth::param_id_hash_table;

std::string Synth::param_names_by_id[ParamId::MAX_PARAM_ID];


Synth::ModeParam::ModeParam(std::string const name) noexcept
    : Param<Mode, ParamEvaluation::BLOCK>(name, MIX_AND_MOD, SPLIT_AT_C4, MIX_AND_MOD)
{
}


Synth::Synth(Integer const samples_between_gc) noexcept
    : SignalProducer(
        OUT_CHANNELS,
        7                           /* POLY + MODE + MIX + PM + FM + AM + bus   */
        + 31 * 2                    /* Modulator::Params + Carrier::Params      */
        + POLYPHONY * 2             /* modulators + carriers                    */
        + 1                         /* effects                                  */
        + MACROS * 6
        + ENVELOPES * 10
        + LFOS
    ),
    polyphonic("POLY", ToggleParam::ON),
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
    effects("E", bus),
    midi_controllers((MidiController* const*)midi_controllers_rw),
    macros((Macro* const*)macros_rw),
    envelopes((Envelope* const*)envelopes_rw),
    lfos((LFO* const*)lfos_rw)
{
    deferred_note_offs.reserve(2 * POLYPHONY);

    initialize_supported_midi_controllers();

    for (int i = 0; i != 4; ++i) {
        biquad_filter_shared_caches[i] = new BiquadFilterSharedCache();
    }

    for (int i = 0; i != ParamId::MAX_PARAM_ID; ++i) {
        param_ratios[i].store(0.0);
        controller_assignments[i].store(ControllerId::NONE);
        param_names_by_id[i] = "";
    }

    build_frequency_table();
    register_main_params();
    register_child(bus);
    register_modulator_params();
    register_carrier_params();
    register_child(effects);
    register_effects_params();

    create_voices();
    create_midi_controllers();
    create_macros();
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


void Synth::build_frequency_table() noexcept
{
    constexpr Number note_scale = 1.0 / (Number)Midi::NOTES;

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
        Number const note_float = (Number)note * note_scale;
        Number const scale = 0.3 + 0.7 * note_float;

        Number const random_1 = scale * Math::randomize(1.0, note_float);
        Number const random_2 = scale * Math::randomize(1.0, 1.0 - note_float);
        Number const random_3 = scale * Math::randomize(1.0, random_1);
        Number const random_4 = scale * Math::randomize(1.0, random_2);
        Number const random_5 = scale * Math::randomize(1.0, 1.0 - random_1);
        Number const random_6 = scale * Math::randomize(1.0, 1.0 - random_2);
        Number const random_7 = scale * Math::randomize(1.0, random_3);
        Number const random_8 = scale * Math::randomize(1.0, random_4);
        Number const random_9 = scale * Math::randomize(1.0, 1.0 - random_3);
        Number const random_10 = scale * Math::randomize(1.0, 1.0 - random_4);
        Number const random_11 = scale * Math::randomize(1.0, random_5);
        Number const random_12 = scale * Math::randomize(1.0, random_6);

        Number const detune_440_1 = 2.0 * random_1 - 0.3;
        Number const detune_440_2 = 5.0 * random_2 - 1.0;
        Number const detune_440_3 = 15.0 * random_3 - 5.0;
        Number const detune_440_4 = 30.0 * random_4 - 10.0;
        Number const detune_440_5 = 50.0 * random_5 - 15.0;
        Number const detune_440_6 = 50.0 * random_6 - 17.0;

        Number const detune_432_1 = 2.0 * random_7 - 0.3;
        Number const detune_432_2 = 5.0 * random_8 - 1.0;
        Number const detune_432_3 = 15.0 * random_9 - 5.0;
        Number const detune_432_4 = 30.0 * random_10 - 10.0;
        Number const detune_432_5 = 50.0 * random_11 - 15.0;
        Number const detune_432_6 = 50.0 * random_12 - 17.0;

        per_channel_frequencies[0][note] = f_440hz_12tet;

        frequencies[Modulator::TUNING_440HZ_12TET][note] = f_440hz_12tet;
        frequencies[Modulator::TUNING_440HZ_12TET_INACCURATE_1][note] = Math::detune(f_440hz_12tet, detune_440_1);
        frequencies[Modulator::TUNING_440HZ_12TET_INACCURATE_2_SYNCED][note] = Math::detune(f_440hz_12tet, detune_440_2);
        frequencies[Modulator::TUNING_440HZ_12TET_INACCURATE_3][note] = Math::detune(f_440hz_12tet, detune_440_3);
        frequencies[Modulator::TUNING_440HZ_12TET_INACCURATE_4][note] = Math::detune(f_440hz_12tet, detune_440_4);
        frequencies[Modulator::TUNING_440HZ_12TET_INACCURATE_5_SYNCED][note] = Math::detune(f_440hz_12tet, detune_440_5);
        frequencies[Modulator::TUNING_440HZ_12TET_INACCURATE_6][note] = Math::detune(f_440hz_12tet, detune_440_6);

        frequencies[Modulator::TUNING_432HZ_12TET][note] = f_432hz_12tet;
        frequencies[Modulator::TUNING_432HZ_12TET_INACCURATE_1][note] = Math::detune(f_432hz_12tet, detune_432_1);
        frequencies[Modulator::TUNING_432HZ_12TET_INACCURATE_2_SYNCED][note] = Math::detune(f_432hz_12tet, detune_432_2);
        frequencies[Modulator::TUNING_432HZ_12TET_INACCURATE_3][note] = Math::detune(f_432hz_12tet, detune_432_3);
        frequencies[Modulator::TUNING_432HZ_12TET_INACCURATE_4][note] = Math::detune(f_432hz_12tet, detune_432_4);
        frequencies[Modulator::TUNING_432HZ_12TET_INACCURATE_5_SYNCED][note] = Math::detune(f_432hz_12tet, detune_432_5);
        frequencies[Modulator::TUNING_432HZ_12TET_INACCURATE_6][note] = Math::detune(f_432hz_12tet, detune_432_6);
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

    register_param_as_child(ParamId::MODE, mode);

    register_param_as_child<FloatParamS>(ParamId::MIX, modulator_add_volume);
    register_param_as_child<FloatParamS>(ParamId::PM, phase_modulation_level);
    register_param_as_child<FloatParamS>(ParamId::FM, frequency_modulation_level);
    register_param_as_child<FloatParamS>(ParamId::AM, amplitude_modulation_level);
}


void Synth::register_modulator_params() noexcept
{
    register_param_as_child<Modulator::TuningParam>(ParamId::MTUN, modulator_params.tuning);
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
    register_param_as_child<ToggleParam>(ParamId::MF1LOG, modulator_params.filter_1_log_scale);
    register_param_as_child<FloatParamS>(ParamId::MF1FRQ, modulator_params.filter_1_frequency);
    register_param_as_child<FloatParamS>(ParamId::MF1Q, modulator_params.filter_1_q);
    register_param_as_child<FloatParamS>(ParamId::MF1G, modulator_params.filter_1_gain);

    register_param_as_child<Modulator::Filter2::TypeParam>(
        ParamId::MF2TYP, modulator_params.filter_2_type
    );
    register_param_as_child<ToggleParam>(ParamId::MF2LOG, modulator_params.filter_2_log_scale);
    register_param_as_child<FloatParamS>(ParamId::MF2FRQ, modulator_params.filter_2_frequency);
    register_param_as_child<FloatParamS>(ParamId::MF2Q, modulator_params.filter_2_q);
    register_param_as_child<FloatParamS>(ParamId::MF2G, modulator_params.filter_2_gain);
}


void Synth::register_carrier_params() noexcept
{
    register_param_as_child<Carrier::TuningParam>(ParamId::CTUN, carrier_params.tuning);
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
    register_param_as_child<ToggleParam>(ParamId::CF1LOG, carrier_params.filter_1_log_scale);
    register_param_as_child<FloatParamS>(ParamId::CF1FRQ, carrier_params.filter_1_frequency);
    register_param_as_child<FloatParamS>(ParamId::CF1Q, carrier_params.filter_1_q);
    register_param_as_child<FloatParamS>(ParamId::CF1G, carrier_params.filter_1_gain);

    register_param_as_child<Carrier::Filter2::TypeParam>(
        ParamId::CF2TYP, carrier_params.filter_2_type
    );
    register_param_as_child<ToggleParam>(ParamId::CF2LOG, carrier_params.filter_2_log_scale);
    register_param_as_child<FloatParamS>(ParamId::CF2FRQ, carrier_params.filter_2_frequency);
    register_param_as_child<FloatParamS>(ParamId::CF2Q, carrier_params.filter_2_q);
    register_param_as_child<FloatParamS>(ParamId::CF2G, carrier_params.filter_2_gain);
}


void Synth::register_effects_params() noexcept
{
    register_param<FloatParamS>(ParamId::EV1V, effects.volume_1_gain);

    register_param<FloatParamS>(ParamId::EOG, effects.overdrive.level);

    register_param<FloatParamS>(ParamId::EDG, effects.distortion.level);

    register_param<Effects::Filter1<Bus>::TypeParam>(ParamId::EF1TYP, effects.filter_1_type);
    register_param<ToggleParam>(ParamId::EF1LOG, effects.filter_1_log_scale);
    register_param<FloatParamS>(ParamId::EF1FRQ, effects.filter_1.frequency);
    register_param<FloatParamS>(ParamId::EF1Q, effects.filter_1.q);
    register_param<FloatParamS>(ParamId::EF1G, effects.filter_1.gain);

    register_param<Effects::Filter2<Bus>::TypeParam>(ParamId::EF2TYP, effects.filter_2_type);
    register_param<ToggleParam>(ParamId::EF2LOG, effects.filter_2_log_scale);
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
    register_param<FloatParamS>(ParamId::ECWET, effects.chorus.wet);
    register_param<FloatParamS>(ParamId::ECDRY, effects.chorus.dry);
    register_param<ToggleParam>(ParamId::ECSYN, effects.chorus.tempo_sync);
    register_param<ToggleParam>(ParamId::ECLOG, effects.chorus.log_scale_frequencies);

    register_param<FloatParamS>(ParamId::EEDEL, effects.echo.delay_time);
    register_param<FloatParamS>(ParamId::EEFB, effects.echo.feedback);
    register_param<FloatParamS>(ParamId::EEDF, effects.echo.damping_frequency);
    register_param<FloatParamS>(ParamId::EEDG, effects.echo.damping_gain);
    register_param<FloatParamS>(ParamId::EEWID, effects.echo.width);
    register_param<FloatParamS>(ParamId::EEHPF, effects.echo.high_pass_frequency);
    register_param<FloatParamB>(ParamId::EECTH, effects.echo.side_chain_compression_threshold);
    register_param<FloatParamB>(ParamId::EECAT, effects.echo.side_chain_compression_attack_time);
    register_param<FloatParamB>(ParamId::EECRL, effects.echo.side_chain_compression_release_time);
    register_param<FloatParamB>(ParamId::EECR, effects.echo.side_chain_compression_ratio);
    register_param<FloatParamS>(ParamId::EEWET, effects.echo.wet);
    register_param<FloatParamS>(ParamId::EEDRY, effects.echo.dry);
    register_param<ToggleParam>(ParamId::EESYN, effects.echo.tempo_sync);
    register_param<ToggleParam>(ParamId::EELOG, effects.echo.log_scale_frequencies);

    register_param<Effects::Reverb<Bus>::TypeParam>(ParamId::ERTYP, effects.reverb.type);
    register_param<FloatParamS>(ParamId::ERRS, effects.reverb.room_size);
    register_param<FloatParamS>(ParamId::ERDF, effects.reverb.damping_frequency);
    register_param<FloatParamS>(ParamId::ERDG, effects.reverb.damping_gain);
    register_param<FloatParamS>(ParamId::ERWID, effects.reverb.width);
    register_param<FloatParamS>(ParamId::ERHPF, effects.reverb.high_pass_frequency);
    register_param<FloatParamB>(ParamId::ERCTH, effects.reverb.side_chain_compression_threshold);
    register_param<FloatParamB>(ParamId::ERCAT, effects.reverb.side_chain_compression_attack_time);
    register_param<FloatParamB>(ParamId::ERCRL, effects.reverb.side_chain_compression_release_time);
    register_param<FloatParamB>(ParamId::ERCR, effects.reverb.side_chain_compression_ratio);
    register_param<FloatParamS>(ParamId::ERWET, effects.reverb.wet);
    register_param<FloatParamS>(ParamId::ERDRY, effects.reverb.dry);
    register_param<ToggleParam>(ParamId::ERLOG, effects.reverb.log_scale_frequencies);

    register_param<FloatParamS>(ParamId::EV3V, effects.volume_3_gain);
}


void Synth::create_voices() noexcept
{
    for (Integer i = 0; i != POLYPHONY; ++i) {
        synced_inaccuracies[i] = new Inaccuracy(
            calculate_inaccuracy_seed((i + 31) % POLYPHONY)
        );

        modulators[i] = new Modulator(
            frequencies,
            per_channel_frequencies,
            *synced_inaccuracies[i],
            calculate_inaccuracy_seed(i),
            modulator_params,
            biquad_filter_shared_caches[0],
            biquad_filter_shared_caches[1]
        );
        register_child(*modulators[i]);

        carriers[i] = new Carrier(
            frequencies,
            per_channel_frequencies,
            *synced_inaccuracies[i],
            calculate_inaccuracy_seed(POLYPHONY - i),
            carrier_params,
            modulators[i]->modulation_out,
            amplitude_modulation_level,
            frequency_modulation_level,
            phase_modulation_level,
            biquad_filter_shared_caches[2],
            biquad_filter_shared_caches[3]
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
}


void Synth::create_envelopes() noexcept
{
    Integer next_id = ParamId::N1AMT;

    for (Integer i = 0; i != ENVELOPES; ++i) {
        Envelope* envelope = new Envelope(std::string("N") + to_string(i + 1));
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


Synth::~Synth()
{
    for (Integer i = 0; i != POLYPHONY; ++i) {
        delete carriers[i];
        delete modulators[i];
        delete synced_inaccuracies[i];
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


void Synth::set_sample_rate(Frequency const new_sample_rate) noexcept
{
    SignalProducer::set_sample_rate(new_sample_rate);

    samples_between_gc = std::max((Integer)5000, (Integer)(new_sample_rate * 0.2));
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

    for (int i = 0; is_lock_free && i != ParamId::MAX_PARAM_ID; ++i) {
        is_lock_free = (
            param_ratios[i].is_lock_free()
            && controller_assignments[i].is_lock_free()
        );
    }

    return is_lock_free && messages.is_lock_free();
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
    for (Integer i = 0; i != LFOS; ++i) {
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
        || has_realtime_mts_esp_tuning()
    );
}


bool Synth::has_realtime_mts_esp_tuning() const noexcept
{
    return (
        modulator_params.tuning.get_value() == Modulator::TUNING_MTS_ESP_REALTIME
        || carrier_params.tuning.get_value() == Carrier::TUNING_MTS_ESP_REALTIME
    );
}


Synth::NoteTunings const& Synth::collect_active_notes(Integer& active_notes_count) noexcept
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
    for (Integer i = 0; i != LFOS; ++i) {
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
        this->velocity.change(time_offset, velocity_float);
        this->note.change(time_offset, midi_byte_to_float(note));

        note_on_polyphonic(time_offset, channel, note, velocity_float);
    } else {
        note_on_monophonic(time_offset, channel, note, velocity_float, true);
    }
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

    next_voice = (next_voice + 1) & NEXT_VOICE_MASK;

    for (Integer v = 0; v != POLYPHONY; ++v) {
        if (!(
                modulators[next_voice]->is_off_after(time_offset)
                && carriers[next_voice]->is_off_after(time_offset)
        )) {
            next_voice = (next_voice + 1) & NEXT_VOICE_MASK;
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

    Mode const mode = this->mode.get_value();

    if (mode == MIX_AND_MOD) {
        modulators[voice]->note_on(time_offset, next_note_id, note, channel, velocity, previous_note);
        carriers[voice]->note_on(time_offset, next_note_id, note, channel, velocity, previous_note);
    } else {
        if (note < mode + Midi::NOTE_B_2) {
            modulators[voice]->note_on(time_offset, next_note_id, note, channel, velocity, previous_note);
        } else {
            carriers[voice]->note_on(time_offset, next_note_id, note, channel, velocity, previous_note);
        }
    }

    previous_note = note;
}


void Synth::assign_voice_and_note_id(
        Integer const voice,
        Midi::Channel const channel,
        Midi::Note const note
) noexcept {
    if (UNLIKELY(previous_note > Midi::NOTE_MAX)) {
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
    this->velocity.change(time_offset, velocity);
    this->note.change(time_offset, midi_byte_to_float(note));

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

    Mode const mode = this->mode.get_value();

    if (mode == MIX_AND_MOD) {
        trigger_note_on_voice_monophonic<Modulator>(
            *modulator, modulator_off, time_offset, channel, note, velocity
        );
        trigger_note_on_voice_monophonic<Carrier>(
            *carrier, carrier_off, time_offset, channel, note, velocity
        );
    } else {
        if (note < mode + Midi::NOTE_B_2) {
            trigger_note_on_voice_monophonic<Modulator>(
                *modulator, modulator_off, time_offset, channel, note, velocity
            );
            carrier->cancel_note_smoothly(time_offset);
        } else {
            modulator->cancel_note_smoothly(time_offset);
            trigger_note_on_voice_monophonic<Carrier>(
                *carrier, carrier_off, time_offset, channel, note, velocity
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
        Number const velocity
) noexcept {
    if (is_off) {
        voice.note_on(time_offset, next_note_id, note, channel, velocity, previous_note);
    } else if (voice.is_released()) {
        voice.retrigger(time_offset, next_note_id, note, channel, velocity, previous_note);
    } else {
        voice.glide_to(time_offset, next_note_id, note, channel, velocity, previous_note);
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
    Integer const voice = midi_note_to_voice_assignments[channel][note];
    bool const was_note_stack_top = note_stack.is_top(channel, note);

    note_stack.remove(channel, note);

    if (voice == INVALID_VOICE) {
        return;
    }

    midi_note_to_voice_assignments[channel][note] = INVALID_VOICE;

    bool const is_polyphonic = polyphonic.get_value() == ToggleParam::ON;

    Modulator* const modulator = modulators[voice];

    if (is_sustaining) {
        Integer note_id;

        if (modulator->is_on()) {
            note_id = modulator->get_note_id();
        } else {
            Carrier* const carrier = carriers[voice];

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

    if (!is_polyphonic && was_note_stack_top && !note_stack.is_empty()) {
        Number previous_velocity;
        Midi::Channel previous_channel;
        Midi::Note previous_note;

        note_stack.top(previous_channel, previous_note, previous_velocity);

        note_on_monophonic(time_offset, previous_channel, previous_note, previous_velocity, false);

        return;
    }

    Number const velocity_float = midi_byte_to_float(velocity);

    modulator->note_off(time_offset, modulator->get_note_id(), note, velocity_float);

    Carrier* const carrier = carriers[voice];

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
    bool const is_polyphonic = polyphonic.get_value() == ToggleParam::ON;
    is_sustaining = false;

    if (is_polyphonic || note_stack.is_empty()) {
        for (std::vector<DeferredNoteOff>::const_iterator it = deferred_note_offs.begin(); it != deferred_note_offs.end(); ++it) {
            DeferredNoteOff const& deferred_note_off = *it;
            Integer const voice = deferred_note_off.get_voice();

            if (UNLIKELY(voice == INVALID_VOICE)) {
                /* This should never happen, but safety first! */
                continue;
            }

            Integer const note_id = deferred_note_off.get_note_id();
            Midi::Note const note = deferred_note_off.get_note();
            Number const velocity = midi_byte_to_float(deferred_note_off.get_velocity());

            modulators[voice]->note_off(time_offset, note_id, note, velocity);
            carriers[voice]->note_off(time_offset, note_id, note, velocity);
        }
    } else if (!is_polyphonic) {
        for (std::vector<DeferredNoteOff>::const_iterator it = deferred_note_offs.begin(); it != deferred_note_offs.end(); ++it) {
            DeferredNoteOff const& deferred_note_off = *it;
            Integer const note_id = deferred_note_off.get_note_id();

            if (modulators[0]->get_note_id() == note_id || carriers[0]->get_note_id() == note_id) {
                Number previous_velocity;
                Midi::Channel previous_channel;
                Midi::Note previous_note;

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
    return controller_id >= ControllerId::ENVELOPE_1 && controller_id <= ControllerId::ENVELOPE_6;
}


Number Synth::calculate_inaccuracy_seed(Integer const voice) noexcept
{
    constexpr Number scale = 1.0 / (Number)POLYPHONY;

    return Inaccuracy::calculate_new_inaccuracy(scale * (Number)voice);
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
    if (ParamId::M1IN <= param_id && param_id <= ParamId::M20RND) {
        int const offset = (int)param_id - (int)ParamId::M1IN;
        int const macro_idx = offset / MACRO_FLOAT_PARAMS;
        int const param_idx = offset % MACRO_FLOAT_PARAMS;

        switch (param_idx) {
            case 0: return macros_rw[macro_idx]->input.get_default_ratio();
            case 1: return macros_rw[macro_idx]->min.get_default_ratio();
            case 2: return macros_rw[macro_idx]->max.get_default_ratio();
            case 3: return macros_rw[macro_idx]->amount.get_default_ratio();
            case 4: return macros_rw[macro_idx]->distortion.get_default_ratio();
            case 5: return macros_rw[macro_idx]->randomness.get_default_ratio();
            default: return 0.0; /* This should never be reached. */
        }
    } else if (ParamId::N1AMT <= param_id && param_id <= N6FIN) {
        int const offset = (int)param_id - (int)ParamId::N1AMT;
        int const envelope_idx = offset / ENVELOPE_FLOAT_PARAMS;
        int const param_idx = offset % ENVELOPE_FLOAT_PARAMS;

        switch (param_idx) {
            case 0: return envelopes_rw[envelope_idx]->amount.get_default_ratio();
            case 1: return envelopes_rw[envelope_idx]->initial_value.get_default_ratio();
            case 2: return envelopes_rw[envelope_idx]->delay_time.get_default_ratio();
            case 3: return envelopes_rw[envelope_idx]->attack_time.get_default_ratio();
            case 4: return envelopes_rw[envelope_idx]->peak_value.get_default_ratio();
            case 5: return envelopes_rw[envelope_idx]->hold_time.get_default_ratio();
            case 6: return envelopes_rw[envelope_idx]->decay_time.get_default_ratio();
            case 7: return envelopes_rw[envelope_idx]->sustain_value.get_default_ratio();
            case 8: return envelopes_rw[envelope_idx]->release_time.get_default_ratio();
            case 9: return envelopes_rw[envelope_idx]->final_value.get_default_ratio();
            default: return 0.0; /* This should never be reached. */
        }
    } else if (ParamId::L1FRQ <= param_id && param_id <= L8RND) {
        int const offset = (int)param_id - (int)ParamId::L1FRQ;
        int const lfo_idx = offset / LFO_FLOAT_PARAMS;
        int const param_idx = offset % LFO_FLOAT_PARAMS;

        switch (param_idx) {
            case 0: return lfos_rw[lfo_idx]->frequency.get_default_ratio();
            case 1: return lfos_rw[lfo_idx]->phase.get_default_ratio();
            case 2: return lfos_rw[lfo_idx]->min.get_default_ratio();
            case 3: return lfos_rw[lfo_idx]->max.get_default_ratio();
            case 4: return lfos_rw[lfo_idx]->amount.get_default_ratio();
            case 5: return lfos_rw[lfo_idx]->distortion.get_default_ratio();
            case 6: return lfos_rw[lfo_idx]->randomness.get_default_ratio();
            default: return 0.0; /* This should never be reached. */
        }
    }

    switch (param_id) {
        case ParamId::MIX: return modulator_add_volume.get_default_ratio();
        case ParamId::PM: return phase_modulation_level.get_default_ratio();
        case ParamId::FM: return frequency_modulation_level.get_default_ratio();
        case ParamId::AM: return amplitude_modulation_level.get_default_ratio();
        case ParamId::MAMP: return modulator_params.amplitude.get_default_ratio();
        case ParamId::MVS: return modulator_params.velocity_sensitivity.get_default_ratio();
        case ParamId::MFLD: return modulator_params.folding.get_default_ratio();
        case ParamId::MPRT: return modulator_params.portamento_length.get_default_ratio();
        case ParamId::MPRD: return modulator_params.portamento_depth.get_default_ratio();
        case ParamId::MDTN: return modulator_params.detune.get_default_ratio();
        case ParamId::MFIN: return modulator_params.fine_detune.get_default_ratio();
        case ParamId::MWID: return modulator_params.width.get_default_ratio();
        case ParamId::MPAN: return modulator_params.panning.get_default_ratio();
        case ParamId::MVOL: return modulator_params.volume.get_default_ratio();
        case ParamId::MSUB: return modulator_params.subharmonic_amplitude.get_default_ratio();
        case ParamId::MC1: return modulator_params.harmonic_0.get_default_ratio();
        case ParamId::MC2: return modulator_params.harmonic_1.get_default_ratio();
        case ParamId::MC3: return modulator_params.harmonic_2.get_default_ratio();
        case ParamId::MC4: return modulator_params.harmonic_3.get_default_ratio();
        case ParamId::MC5: return modulator_params.harmonic_4.get_default_ratio();
        case ParamId::MC6: return modulator_params.harmonic_5.get_default_ratio();
        case ParamId::MC7: return modulator_params.harmonic_6.get_default_ratio();
        case ParamId::MC8: return modulator_params.harmonic_7.get_default_ratio();
        case ParamId::MC9: return modulator_params.harmonic_8.get_default_ratio();
        case ParamId::MC10: return modulator_params.harmonic_9.get_default_ratio();
        case ParamId::MF1FRQ: return modulator_params.filter_1_frequency.get_default_ratio();
        case ParamId::MF1Q: return modulator_params.filter_1_q.get_default_ratio();
        case ParamId::MF1G: return modulator_params.filter_1_gain.get_default_ratio();
        case ParamId::MF2FRQ: return modulator_params.filter_2_frequency.get_default_ratio();
        case ParamId::MF2Q: return modulator_params.filter_2_q.get_default_ratio();
        case ParamId::MF2G: return modulator_params.filter_2_gain.get_default_ratio();
        case ParamId::CAMP: return carrier_params.amplitude.get_default_ratio();
        case ParamId::CVS: return carrier_params.velocity_sensitivity.get_default_ratio();
        case ParamId::CFLD: return carrier_params.folding.get_default_ratio();
        case ParamId::CPRT: return carrier_params.portamento_length.get_default_ratio();
        case ParamId::CPRD: return carrier_params.portamento_depth.get_default_ratio();
        case ParamId::CDTN: return carrier_params.detune.get_default_ratio();
        case ParamId::CFIN: return carrier_params.fine_detune.get_default_ratio();
        case ParamId::CWID: return carrier_params.width.get_default_ratio();
        case ParamId::CPAN: return carrier_params.panning.get_default_ratio();
        case ParamId::CVOL: return carrier_params.volume.get_default_ratio();
        case ParamId::CDG: return carrier_params.distortion.get_default_ratio();
        case ParamId::CC1: return carrier_params.harmonic_0.get_default_ratio();
        case ParamId::CC2: return carrier_params.harmonic_1.get_default_ratio();
        case ParamId::CC3: return carrier_params.harmonic_2.get_default_ratio();
        case ParamId::CC4: return carrier_params.harmonic_3.get_default_ratio();
        case ParamId::CC5: return carrier_params.harmonic_4.get_default_ratio();
        case ParamId::CC6: return carrier_params.harmonic_5.get_default_ratio();
        case ParamId::CC7: return carrier_params.harmonic_6.get_default_ratio();
        case ParamId::CC8: return carrier_params.harmonic_7.get_default_ratio();
        case ParamId::CC9: return carrier_params.harmonic_8.get_default_ratio();
        case ParamId::CC10: return carrier_params.harmonic_9.get_default_ratio();
        case ParamId::CF1FRQ: return carrier_params.filter_1_frequency.get_default_ratio();
        case ParamId::CF1Q: return carrier_params.filter_1_q.get_default_ratio();
        case ParamId::CF1G: return carrier_params.filter_1_gain.get_default_ratio();
        case ParamId::CF2FRQ: return carrier_params.filter_2_frequency.get_default_ratio();
        case ParamId::CF2Q: return carrier_params.filter_2_q.get_default_ratio();
        case ParamId::CF2G: return carrier_params.filter_2_gain.get_default_ratio();
        case ParamId::EV1V: return effects.volume_1_gain.get_default_ratio();
        case ParamId::EOG: return effects.overdrive.level.get_default_ratio();
        case ParamId::EDG: return effects.distortion.level.get_default_ratio();
        case ParamId::EF1FRQ: return effects.filter_1.frequency.get_default_ratio();
        case ParamId::EF1Q: return effects.filter_1.q.get_default_ratio();
        case ParamId::EF1G: return effects.filter_1.gain.get_default_ratio();
        case ParamId::EF2FRQ: return effects.filter_2.frequency.get_default_ratio();
        case ParamId::EF2Q: return effects.filter_2.q.get_default_ratio();
        case ParamId::EF2G: return effects.filter_2.gain.get_default_ratio();
        case ParamId::EV2V: return effects.volume_2_gain.get_default_ratio();
        case ParamId::ECDEL: return effects.chorus.delay_time.get_default_ratio();
        case ParamId::ECFRQ: return effects.chorus.frequency.get_default_ratio();
        case ParamId::ECDPT: return effects.chorus.depth.get_default_ratio();
        case ParamId::ECFB: return effects.chorus.feedback.get_default_ratio();
        case ParamId::ECDF: return effects.chorus.damping_frequency.get_default_ratio();
        case ParamId::ECDG: return effects.chorus.damping_gain.get_default_ratio();
        case ParamId::ECWID: return effects.chorus.width.get_default_ratio();
        case ParamId::ECHPF: return effects.chorus.high_pass_frequency.get_default_ratio();
        case ParamId::ECWET: return effects.chorus.wet.get_default_ratio();
        case ParamId::ECDRY: return effects.chorus.dry.get_default_ratio();
        case ParamId::EEDEL: return effects.echo.delay_time.get_default_ratio();
        case ParamId::EEFB: return effects.echo.feedback.get_default_ratio();
        case ParamId::EEDF: return effects.echo.damping_frequency.get_default_ratio();
        case ParamId::EEDG: return effects.echo.damping_gain.get_default_ratio();
        case ParamId::EEWID: return effects.echo.width.get_default_ratio();
        case ParamId::EEHPF: return effects.echo.high_pass_frequency.get_default_ratio();
        case ParamId::EECTH: return effects.echo.side_chain_compression_threshold.get_default_ratio();
        case ParamId::EECAT: return effects.echo.side_chain_compression_attack_time.get_default_ratio();
        case ParamId::EECRL: return effects.echo.side_chain_compression_release_time.get_default_ratio();
        case ParamId::EECR: return effects.echo.side_chain_compression_ratio.get_default_ratio();
        case ParamId::EEWET: return effects.echo.wet.get_default_ratio();
        case ParamId::EEDRY: return effects.echo.dry.get_default_ratio();
        case ParamId::ERRS: return effects.reverb.room_size.get_default_ratio();
        case ParamId::ERDF: return effects.reverb.damping_frequency.get_default_ratio();
        case ParamId::ERDG: return effects.reverb.damping_gain.get_default_ratio();
        case ParamId::ERWID: return effects.reverb.width.get_default_ratio();
        case ParamId::ERHPF: return effects.reverb.high_pass_frequency.get_default_ratio();
        case ParamId::ERCTH: return effects.reverb.side_chain_compression_threshold.get_default_ratio();
        case ParamId::ERCAT: return effects.reverb.side_chain_compression_attack_time.get_default_ratio();
        case ParamId::ERCRL: return effects.reverb.side_chain_compression_release_time.get_default_ratio();
        case ParamId::ERCR: return effects.reverb.side_chain_compression_ratio.get_default_ratio();
        case ParamId::ERWET: return effects.reverb.wet.get_default_ratio();
        case ParamId::ERDRY: return effects.reverb.dry.get_default_ratio();
        case ParamId::EV3V: return effects.volume_3_gain.get_default_ratio();
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
        case ParamId::POLY: return polyphonic.get_default_ratio();
        case ParamId::ERTYP: return effects.reverb.type.get_default_ratio();
        case ParamId::ECTYP: return effects.chorus.type.get_default_ratio();
        case ParamId::MTUN: return modulator_params.tuning.get_default_ratio();
        case ParamId::CTUN: return carrier_params.tuning.get_default_ratio();
        default: return 0.0; /* This should never be reached. */
    }
}


bool Synth::is_toggle_param(ParamId const param_id) const noexcept
{
    return param_id >= ParamId::L1SYN && param_id < ParamId::MAX_PARAM_ID;
}


Number Synth::get_param_max_value(ParamId const param_id) const noexcept
{
    if (ParamId::M1IN <= param_id && param_id <= ParamId::M20RND) {
        int const offset = (int)param_id - (int)ParamId::M1IN;
        int const macro_idx = offset / MACRO_FLOAT_PARAMS;
        int const param_idx = offset % MACRO_FLOAT_PARAMS;

        switch (param_idx) {
            case 0: return macros_rw[macro_idx]->input.get_max_value();
            case 1: return macros_rw[macro_idx]->min.get_max_value();
            case 2: return macros_rw[macro_idx]->max.get_max_value();
            case 3: return macros_rw[macro_idx]->amount.get_max_value();
            case 4: return macros_rw[macro_idx]->distortion.get_max_value();
            case 5: return macros_rw[macro_idx]->randomness.get_max_value();
            default: return 0.0; /* This should never be reached. */
        }
    } else if (ParamId::N1AMT <= param_id && param_id <= N6FIN) {
        int const offset = (int)param_id - (int)ParamId::N1AMT;
        int const envelope_idx = offset / ENVELOPE_FLOAT_PARAMS;
        int const param_idx = offset % ENVELOPE_FLOAT_PARAMS;

        switch (param_idx) {
            case 0: return envelopes_rw[envelope_idx]->amount.get_max_value();
            case 1: return envelopes_rw[envelope_idx]->initial_value.get_max_value();
            case 2: return envelopes_rw[envelope_idx]->delay_time.get_max_value();
            case 3: return envelopes_rw[envelope_idx]->attack_time.get_max_value();
            case 4: return envelopes_rw[envelope_idx]->peak_value.get_max_value();
            case 5: return envelopes_rw[envelope_idx]->hold_time.get_max_value();
            case 6: return envelopes_rw[envelope_idx]->decay_time.get_max_value();
            case 7: return envelopes_rw[envelope_idx]->sustain_value.get_max_value();
            case 8: return envelopes_rw[envelope_idx]->release_time.get_max_value();
            case 9: return envelopes_rw[envelope_idx]->final_value.get_max_value();
            default: return 0.0; /* This should never be reached. */
        }
    } else if (ParamId::L1FRQ <= param_id && param_id <= L8RND) {
        int const offset = (int)param_id - (int)ParamId::L1FRQ;
        int const lfo_idx = offset / LFO_FLOAT_PARAMS;
        int const param_idx = offset % LFO_FLOAT_PARAMS;

        switch (param_idx) {
            case 0: return lfos_rw[lfo_idx]->frequency.get_max_value();
            case 1: return lfos_rw[lfo_idx]->phase.get_max_value();
            case 2: return lfos_rw[lfo_idx]->min.get_max_value();
            case 3: return lfos_rw[lfo_idx]->max.get_max_value();
            case 4: return lfos_rw[lfo_idx]->amount.get_max_value();
            case 5: return lfos_rw[lfo_idx]->distortion.get_max_value();
            case 6: return lfos_rw[lfo_idx]->randomness.get_max_value();
            default: return 0.0; /* This should never be reached. */
        }
    }

    switch (param_id) {
        case ParamId::MIX: return modulator_add_volume.get_max_value();
        case ParamId::PM: return phase_modulation_level.get_max_value();
        case ParamId::FM: return frequency_modulation_level.get_max_value();
        case ParamId::AM: return amplitude_modulation_level.get_max_value();
        case ParamId::MAMP: return modulator_params.amplitude.get_max_value();
        case ParamId::MVS: return modulator_params.velocity_sensitivity.get_max_value();
        case ParamId::MFLD: return modulator_params.folding.get_max_value();
        case ParamId::MPRT: return modulator_params.portamento_length.get_max_value();
        case ParamId::MPRD: return modulator_params.portamento_depth.get_max_value();
        case ParamId::MDTN: return modulator_params.detune.get_max_value();
        case ParamId::MFIN: return modulator_params.fine_detune.get_max_value();
        case ParamId::MWID: return modulator_params.width.get_max_value();
        case ParamId::MPAN: return modulator_params.panning.get_max_value();
        case ParamId::MVOL: return modulator_params.volume.get_max_value();
        case ParamId::MSUB: return modulator_params.subharmonic_amplitude.get_max_value();
        case ParamId::MC1: return modulator_params.harmonic_0.get_max_value();
        case ParamId::MC2: return modulator_params.harmonic_1.get_max_value();
        case ParamId::MC3: return modulator_params.harmonic_2.get_max_value();
        case ParamId::MC4: return modulator_params.harmonic_3.get_max_value();
        case ParamId::MC5: return modulator_params.harmonic_4.get_max_value();
        case ParamId::MC6: return modulator_params.harmonic_5.get_max_value();
        case ParamId::MC7: return modulator_params.harmonic_6.get_max_value();
        case ParamId::MC8: return modulator_params.harmonic_7.get_max_value();
        case ParamId::MC9: return modulator_params.harmonic_8.get_max_value();
        case ParamId::MC10: return modulator_params.harmonic_9.get_max_value();
        case ParamId::MF1FRQ: return modulator_params.filter_1_frequency.get_max_value();
        case ParamId::MF1Q: return modulator_params.filter_1_q.get_max_value();
        case ParamId::MF1G: return modulator_params.filter_1_gain.get_max_value();
        case ParamId::MF2FRQ: return modulator_params.filter_2_frequency.get_max_value();
        case ParamId::MF2Q: return modulator_params.filter_2_q.get_max_value();
        case ParamId::MF2G: return modulator_params.filter_2_gain.get_max_value();
        case ParamId::CAMP: return carrier_params.amplitude.get_max_value();
        case ParamId::CVS: return carrier_params.velocity_sensitivity.get_max_value();
        case ParamId::CFLD: return carrier_params.folding.get_max_value();
        case ParamId::CPRT: return carrier_params.portamento_length.get_max_value();
        case ParamId::CPRD: return carrier_params.portamento_depth.get_max_value();
        case ParamId::CDTN: return carrier_params.detune.get_max_value();
        case ParamId::CFIN: return carrier_params.fine_detune.get_max_value();
        case ParamId::CWID: return carrier_params.width.get_max_value();
        case ParamId::CPAN: return carrier_params.panning.get_max_value();
        case ParamId::CVOL: return carrier_params.volume.get_max_value();
        case ParamId::CDG: return carrier_params.distortion.get_max_value();
        case ParamId::CC1: return carrier_params.harmonic_0.get_max_value();
        case ParamId::CC2: return carrier_params.harmonic_1.get_max_value();
        case ParamId::CC3: return carrier_params.harmonic_2.get_max_value();
        case ParamId::CC4: return carrier_params.harmonic_3.get_max_value();
        case ParamId::CC5: return carrier_params.harmonic_4.get_max_value();
        case ParamId::CC6: return carrier_params.harmonic_5.get_max_value();
        case ParamId::CC7: return carrier_params.harmonic_6.get_max_value();
        case ParamId::CC8: return carrier_params.harmonic_7.get_max_value();
        case ParamId::CC9: return carrier_params.harmonic_8.get_max_value();
        case ParamId::CC10: return carrier_params.harmonic_9.get_max_value();
        case ParamId::CF1FRQ: return carrier_params.filter_1_frequency.get_max_value();
        case ParamId::CF1Q: return carrier_params.filter_1_q.get_max_value();
        case ParamId::CF1G: return carrier_params.filter_1_gain.get_max_value();
        case ParamId::CF2FRQ: return carrier_params.filter_2_frequency.get_max_value();
        case ParamId::CF2Q: return carrier_params.filter_2_q.get_max_value();
        case ParamId::CF2G: return carrier_params.filter_2_gain.get_max_value();
        case ParamId::EV1V: return effects.volume_1_gain.get_max_value();
        case ParamId::EOG: return effects.overdrive.level.get_max_value();
        case ParamId::EDG: return effects.distortion.level.get_max_value();
        case ParamId::EF1FRQ: return effects.filter_1.frequency.get_max_value();
        case ParamId::EF1Q: return effects.filter_1.q.get_max_value();
        case ParamId::EF1G: return effects.filter_1.gain.get_max_value();
        case ParamId::EF2FRQ: return effects.filter_2.frequency.get_max_value();
        case ParamId::EF2Q: return effects.filter_2.q.get_max_value();
        case ParamId::EF2G: return effects.filter_2.gain.get_max_value();
        case ParamId::EV2V: return effects.volume_2_gain.get_max_value();
        case ParamId::ECDEL: return effects.chorus.delay_time.get_max_value();
        case ParamId::ECFRQ: return effects.chorus.frequency.get_max_value();
        case ParamId::ECDPT: return effects.chorus.depth.get_max_value();
        case ParamId::ECFB: return effects.chorus.feedback.get_max_value();
        case ParamId::ECDF: return effects.chorus.damping_frequency.get_max_value();
        case ParamId::ECDG: return effects.chorus.damping_gain.get_max_value();
        case ParamId::ECWID: return effects.chorus.width.get_max_value();
        case ParamId::ECHPF: return effects.chorus.high_pass_frequency.get_max_value();
        case ParamId::ECWET: return effects.chorus.wet.get_max_value();
        case ParamId::ECDRY: return effects.chorus.dry.get_max_value();
        case ParamId::EEDEL: return effects.echo.delay_time.get_max_value();
        case ParamId::EEFB: return effects.echo.feedback.get_max_value();
        case ParamId::EEDF: return effects.echo.damping_frequency.get_max_value();
        case ParamId::EEDG: return effects.echo.damping_gain.get_max_value();
        case ParamId::EEWID: return effects.echo.width.get_max_value();
        case ParamId::EEHPF: return effects.echo.high_pass_frequency.get_max_value();
        case ParamId::EECTH: return effects.echo.side_chain_compression_threshold.get_max_value();
        case ParamId::EECAT: return effects.echo.side_chain_compression_attack_time.get_max_value();
        case ParamId::EECRL: return effects.echo.side_chain_compression_release_time.get_max_value();
        case ParamId::EECR: return effects.echo.side_chain_compression_ratio.get_max_value();
        case ParamId::EEWET: return effects.echo.wet.get_max_value();
        case ParamId::EEDRY: return effects.echo.dry.get_max_value();
        case ParamId::ERRS: return effects.reverb.room_size.get_max_value();
        case ParamId::ERDF: return effects.reverb.damping_frequency.get_max_value();
        case ParamId::ERDG: return effects.reverb.damping_gain.get_max_value();
        case ParamId::ERWID: return effects.reverb.width.get_max_value();
        case ParamId::ERHPF: return effects.reverb.high_pass_frequency.get_max_value();
        case ParamId::ERCTH: return effects.reverb.side_chain_compression_threshold.get_max_value();
        case ParamId::ERCAT: return effects.reverb.side_chain_compression_attack_time.get_max_value();
        case ParamId::ERCRL: return effects.reverb.side_chain_compression_release_time.get_max_value();
        case ParamId::ERCR: return effects.reverb.side_chain_compression_ratio.get_max_value();
        case ParamId::ERWET: return effects.reverb.wet.get_max_value();
        case ParamId::ERDRY: return effects.reverb.dry.get_max_value();
        case ParamId::EV3V: return effects.volume_3_gain.get_max_value();
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
        case ParamId::POLY: return polyphonic.get_max_value();
        case ParamId::ERTYP: return effects.reverb.type.get_max_value();
        case ParamId::ECTYP: return effects.chorus.type.get_max_value();
        case ParamId::MTUN: return modulator_params.tuning.get_max_value();
        case ParamId::CTUN: return carrier_params.tuning.get_max_value();
        default: return 0.0; /* This should never be reached. */
    }
}


Number Synth::float_param_ratio_to_display_value(
        ParamId const param_id,
        Number const ratio
) const noexcept {
    if (ParamId::M1IN <= param_id && param_id <= ParamId::M20RND) {
        int const offset = (int)param_id - (int)ParamId::M1IN;
        int const macro_idx = offset / MACRO_FLOAT_PARAMS;
        int const param_idx = offset % MACRO_FLOAT_PARAMS;

        switch (param_idx) {
            case 0: return macros_rw[macro_idx]->input.ratio_to_value(ratio);
            case 1: return macros_rw[macro_idx]->min.ratio_to_value(ratio);
            case 2: return macros_rw[macro_idx]->max.ratio_to_value(ratio);
            case 3: return macros_rw[macro_idx]->amount.ratio_to_value(ratio);
            case 4: return macros_rw[macro_idx]->distortion.ratio_to_value(ratio);
            case 5: return macros_rw[macro_idx]->randomness.ratio_to_value(ratio);
            default: return 0.0; /* This should never be reached. */
        }
    } else if (ParamId::N1AMT <= param_id && param_id <= N6FIN) {
        int const offset = (int)param_id - (int)ParamId::N1AMT;
        int const envelope_idx = offset / ENVELOPE_FLOAT_PARAMS;
        int const param_idx = offset % ENVELOPE_FLOAT_PARAMS;

        switch (param_idx) {
            case 0: return envelopes_rw[envelope_idx]->amount.ratio_to_value(ratio);
            case 1: return envelopes_rw[envelope_idx]->initial_value.ratio_to_value(ratio);
            case 2: return envelopes_rw[envelope_idx]->delay_time.ratio_to_value(ratio);
            case 3: return envelopes_rw[envelope_idx]->attack_time.ratio_to_value(ratio);
            case 4: return envelopes_rw[envelope_idx]->peak_value.ratio_to_value(ratio);
            case 5: return envelopes_rw[envelope_idx]->hold_time.ratio_to_value(ratio);
            case 6: return envelopes_rw[envelope_idx]->decay_time.ratio_to_value(ratio);
            case 7: return envelopes_rw[envelope_idx]->sustain_value.ratio_to_value(ratio);
            case 8: return envelopes_rw[envelope_idx]->release_time.ratio_to_value(ratio);
            case 9: return envelopes_rw[envelope_idx]->final_value.ratio_to_value(ratio);
            default: return 0.0; /* This should never be reached. */
        }
    } else if (ParamId::L1FRQ <= param_id && param_id <= L8RND) {
        int const offset = (int)param_id - (int)ParamId::L1FRQ;
        int const lfo_idx = offset / LFO_FLOAT_PARAMS;
        int const param_idx = offset % LFO_FLOAT_PARAMS;

        switch (param_idx) {
            case 0: return lfos_rw[lfo_idx]->frequency.ratio_to_value(ratio);
            case 1: return lfos_rw[lfo_idx]->phase.ratio_to_value(ratio);
            case 2: return lfos_rw[lfo_idx]->min.ratio_to_value(ratio);
            case 3: return lfos_rw[lfo_idx]->max.ratio_to_value(ratio);
            case 4: return lfos_rw[lfo_idx]->amount.ratio_to_value(ratio);
            case 5: return lfos_rw[lfo_idx]->distortion.ratio_to_value(ratio);
            case 6: return lfos_rw[lfo_idx]->randomness.ratio_to_value(ratio);
            default: return 0.0; /* This should never be reached. */
        }
    }

    switch (param_id) {
        case ParamId::MIX: return modulator_add_volume.ratio_to_value(ratio);
        case ParamId::PM: return phase_modulation_level.ratio_to_value(ratio);
        case ParamId::FM: return frequency_modulation_level.ratio_to_value(ratio);
        case ParamId::AM: return amplitude_modulation_level.ratio_to_value(ratio);
        case ParamId::MAMP: return modulator_params.amplitude.ratio_to_value(ratio);
        case ParamId::MVS: return modulator_params.velocity_sensitivity.ratio_to_value(ratio);
        case ParamId::MFLD: return modulator_params.folding.ratio_to_value(ratio);
        case ParamId::MPRT: return modulator_params.portamento_length.ratio_to_value(ratio);
        case ParamId::MPRD: return modulator_params.portamento_depth.ratio_to_value(ratio);
        case ParamId::MDTN: return modulator_params.detune.ratio_to_value(ratio);
        case ParamId::MFIN: return modulator_params.fine_detune.ratio_to_value(ratio);
        case ParamId::MWID: return modulator_params.width.ratio_to_value(ratio);
        case ParamId::MPAN: return modulator_params.panning.ratio_to_value(ratio);
        case ParamId::MVOL: return modulator_params.volume.ratio_to_value(ratio);
        case ParamId::MSUB: return modulator_params.subharmonic_amplitude.ratio_to_value(ratio);
        case ParamId::MC1: return modulator_params.harmonic_0.ratio_to_value(ratio);
        case ParamId::MC2: return modulator_params.harmonic_1.ratio_to_value(ratio);
        case ParamId::MC3: return modulator_params.harmonic_2.ratio_to_value(ratio);
        case ParamId::MC4: return modulator_params.harmonic_3.ratio_to_value(ratio);
        case ParamId::MC5: return modulator_params.harmonic_4.ratio_to_value(ratio);
        case ParamId::MC6: return modulator_params.harmonic_5.ratio_to_value(ratio);
        case ParamId::MC7: return modulator_params.harmonic_6.ratio_to_value(ratio);
        case ParamId::MC8: return modulator_params.harmonic_7.ratio_to_value(ratio);
        case ParamId::MC9: return modulator_params.harmonic_8.ratio_to_value(ratio);
        case ParamId::MC10: return modulator_params.harmonic_9.ratio_to_value(ratio);
        case ParamId::MF1FRQ: return modulator_params.filter_1_frequency.ratio_to_value(ratio);
        case ParamId::MF1Q: return modulator_params.filter_1_q.ratio_to_value(ratio);
        case ParamId::MF1G: return modulator_params.filter_1_gain.ratio_to_value(ratio);
        case ParamId::MF2FRQ: return modulator_params.filter_2_frequency.ratio_to_value(ratio);
        case ParamId::MF2Q: return modulator_params.filter_2_q.ratio_to_value(ratio);
        case ParamId::MF2G: return modulator_params.filter_2_gain.ratio_to_value(ratio);
        case ParamId::CAMP: return carrier_params.amplitude.ratio_to_value(ratio);
        case ParamId::CVS: return carrier_params.velocity_sensitivity.ratio_to_value(ratio);
        case ParamId::CFLD: return carrier_params.folding.ratio_to_value(ratio);
        case ParamId::CPRT: return carrier_params.portamento_length.ratio_to_value(ratio);
        case ParamId::CPRD: return carrier_params.portamento_depth.ratio_to_value(ratio);
        case ParamId::CDTN: return carrier_params.detune.ratio_to_value(ratio);
        case ParamId::CFIN: return carrier_params.fine_detune.ratio_to_value(ratio);
        case ParamId::CWID: return carrier_params.width.ratio_to_value(ratio);
        case ParamId::CPAN: return carrier_params.panning.ratio_to_value(ratio);
        case ParamId::CVOL: return carrier_params.volume.ratio_to_value(ratio);
        case ParamId::CDG: return carrier_params.distortion.ratio_to_value(ratio);
        case ParamId::CC1: return carrier_params.harmonic_0.ratio_to_value(ratio);
        case ParamId::CC2: return carrier_params.harmonic_1.ratio_to_value(ratio);
        case ParamId::CC3: return carrier_params.harmonic_2.ratio_to_value(ratio);
        case ParamId::CC4: return carrier_params.harmonic_3.ratio_to_value(ratio);
        case ParamId::CC5: return carrier_params.harmonic_4.ratio_to_value(ratio);
        case ParamId::CC6: return carrier_params.harmonic_5.ratio_to_value(ratio);
        case ParamId::CC7: return carrier_params.harmonic_6.ratio_to_value(ratio);
        case ParamId::CC8: return carrier_params.harmonic_7.ratio_to_value(ratio);
        case ParamId::CC9: return carrier_params.harmonic_8.ratio_to_value(ratio);
        case ParamId::CC10: return carrier_params.harmonic_9.ratio_to_value(ratio);
        case ParamId::CF1FRQ: return carrier_params.filter_1_frequency.ratio_to_value(ratio);
        case ParamId::CF1Q: return carrier_params.filter_1_q.ratio_to_value(ratio);
        case ParamId::CF1G: return carrier_params.filter_1_gain.ratio_to_value(ratio);
        case ParamId::CF2FRQ: return carrier_params.filter_2_frequency.ratio_to_value(ratio);
        case ParamId::CF2Q: return carrier_params.filter_2_q.ratio_to_value(ratio);
        case ParamId::CF2G: return carrier_params.filter_2_gain.ratio_to_value(ratio);
        case ParamId::EV1V: return effects.volume_1_gain.ratio_to_value(ratio);
        case ParamId::EOG: return effects.overdrive.level.ratio_to_value(ratio);
        case ParamId::EDG: return effects.distortion.level.ratio_to_value(ratio);
        case ParamId::EF1FRQ: return effects.filter_1.frequency.ratio_to_value(ratio);
        case ParamId::EF1Q: return effects.filter_1.q.ratio_to_value(ratio);
        case ParamId::EF1G: return effects.filter_1.gain.ratio_to_value(ratio);
        case ParamId::EF2FRQ: return effects.filter_2.frequency.ratio_to_value(ratio);
        case ParamId::EF2Q: return effects.filter_2.q.ratio_to_value(ratio);
        case ParamId::EF2G: return effects.filter_2.gain.ratio_to_value(ratio);
        case ParamId::EV2V: return effects.volume_2_gain.ratio_to_value(ratio);
        case ParamId::ECDEL: return effects.chorus.delay_time.ratio_to_value(ratio);
        case ParamId::ECFRQ: return effects.chorus.frequency.ratio_to_value(ratio);
        case ParamId::ECDPT: return effects.chorus.depth.ratio_to_value(ratio);
        case ParamId::ECFB: return effects.chorus.feedback.ratio_to_value(ratio);
        case ParamId::ECDF: return effects.chorus.damping_frequency.ratio_to_value(ratio);
        case ParamId::ECDG: return effects.chorus.damping_gain.ratio_to_value(ratio);
        case ParamId::ECWID: return effects.chorus.width.ratio_to_value(ratio);
        case ParamId::ECHPF: return effects.chorus.high_pass_frequency.ratio_to_value(ratio);
        case ParamId::ECWET: return effects.chorus.wet.ratio_to_value(ratio);
        case ParamId::ECDRY: return effects.chorus.dry.ratio_to_value(ratio);
        case ParamId::EEDEL: return effects.echo.delay_time.ratio_to_value(ratio);
        case ParamId::EEFB: return effects.echo.feedback.ratio_to_value(ratio);
        case ParamId::EEDF: return effects.echo.damping_frequency.ratio_to_value(ratio);
        case ParamId::EEDG: return effects.echo.damping_gain.ratio_to_value(ratio);
        case ParamId::EEWID: return effects.echo.width.ratio_to_value(ratio);
        case ParamId::EEHPF: return effects.echo.high_pass_frequency.ratio_to_value(ratio);
        case ParamId::EECTH: return effects.echo.side_chain_compression_threshold.ratio_to_value(ratio);
        case ParamId::EECAT: return effects.echo.side_chain_compression_attack_time.ratio_to_value(ratio);
        case ParamId::EECRL: return effects.echo.side_chain_compression_release_time.ratio_to_value(ratio);
        case ParamId::EECR: return effects.echo.side_chain_compression_ratio.ratio_to_value(ratio);
        case ParamId::EEWET: return effects.echo.wet.ratio_to_value(ratio);
        case ParamId::EEDRY: return effects.echo.dry.ratio_to_value(ratio);
        case ParamId::ERRS: return effects.reverb.room_size.ratio_to_value(ratio);
        case ParamId::ERDF: return effects.reverb.damping_frequency.ratio_to_value(ratio);
        case ParamId::ERDG: return effects.reverb.damping_gain.ratio_to_value(ratio);
        case ParamId::ERWID: return effects.reverb.width.ratio_to_value(ratio);
        case ParamId::ERHPF: return effects.reverb.high_pass_frequency.ratio_to_value(ratio);
        case ParamId::ERCTH: return effects.reverb.side_chain_compression_threshold.ratio_to_value(ratio);
        case ParamId::ERCAT: return effects.reverb.side_chain_compression_attack_time.ratio_to_value(ratio);
        case ParamId::ERCRL: return effects.reverb.side_chain_compression_release_time.ratio_to_value(ratio);
        case ParamId::ERCR: return effects.reverb.side_chain_compression_ratio.ratio_to_value(ratio);
        case ParamId::ERWET: return effects.reverb.wet.ratio_to_value(ratio);
        case ParamId::ERDRY: return effects.reverb.dry.ratio_to_value(ratio);
        case ParamId::EV3V: return effects.volume_3_gain.ratio_to_value(ratio);
        default: return 0.0; /* This should never be reached. */
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
        case ParamId::POLY: return polyphonic.ratio_to_value(ratio);
        case ParamId::ERTYP: return effects.reverb.type.ratio_to_value(ratio);
        case ParamId::ECTYP: return effects.chorus.type.ratio_to_value(ratio);
        case ParamId::MTUN: return modulator_params.tuning.ratio_to_value(ratio);
        case ParamId::CTUN: return carrier_params.tuning.ratio_to_value(ratio);
        default: return 0; /* This should never be reached. */
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

    FloatParamS::produce_if_not_constant(modulator_add_volume, round, sample_count);

    FloatParamS::produce_if_not_constant(phase_modulation_level, round, sample_count);
    FloatParamS::produce_if_not_constant(frequency_modulation_level, round, sample_count);
    FloatParamS::produce_if_not_constant(amplitude_modulation_level, round, sample_count);

    FloatParamS::produce_if_not_constant(modulator_params.amplitude, round, sample_count);
    FloatParamB::produce_if_not_constant(modulator_params.velocity_sensitivity, round, sample_count);
    FloatParamS::produce_if_not_constant(modulator_params.folding, round, sample_count);
    FloatParamB::produce_if_not_constant(modulator_params.portamento_length, round, sample_count);
    FloatParamB::produce_if_not_constant(modulator_params.portamento_depth, round, sample_count);
    FloatParamS::produce_if_not_constant(modulator_params.detune, round, sample_count);
    FloatParamS::produce_if_not_constant(modulator_params.fine_detune, round, sample_count);
    FloatParamB::produce_if_not_constant(modulator_params.width, round, sample_count);
    FloatParamS::produce_if_not_constant(modulator_params.panning, round, sample_count);
    FloatParamS::produce_if_not_constant(modulator_params.volume, round, sample_count);

    FloatParamB::produce_if_not_constant(modulator_params.harmonic_0, round, sample_count);
    FloatParamB::produce_if_not_constant(modulator_params.harmonic_1, round, sample_count);
    FloatParamB::produce_if_not_constant(modulator_params.harmonic_2, round, sample_count);
    FloatParamB::produce_if_not_constant(modulator_params.harmonic_3, round, sample_count);
    FloatParamB::produce_if_not_constant(modulator_params.harmonic_4, round, sample_count);
    FloatParamB::produce_if_not_constant(modulator_params.harmonic_5, round, sample_count);
    FloatParamB::produce_if_not_constant(modulator_params.harmonic_6, round, sample_count);
    FloatParamB::produce_if_not_constant(modulator_params.harmonic_7, round, sample_count);
    FloatParamB::produce_if_not_constant(modulator_params.harmonic_8, round, sample_count);
    FloatParamB::produce_if_not_constant(modulator_params.harmonic_9, round, sample_count);

    FloatParamS::produce_if_not_constant(modulator_params.filter_1_frequency, round, sample_count);
    FloatParamS::produce_if_not_constant(modulator_params.filter_1_q, round, sample_count);
    FloatParamS::produce_if_not_constant(modulator_params.filter_1_gain, round, sample_count);

    FloatParamS::produce_if_not_constant(modulator_params.filter_2_frequency, round, sample_count);
    FloatParamS::produce_if_not_constant(modulator_params.filter_2_q, round, sample_count);
    FloatParamS::produce_if_not_constant(modulator_params.filter_2_gain, round, sample_count);

    FloatParamS::produce_if_not_constant(carrier_params.amplitude, round, sample_count);
    FloatParamB::produce_if_not_constant(carrier_params.velocity_sensitivity, round, sample_count);
    FloatParamS::produce_if_not_constant(carrier_params.folding, round, sample_count);
    FloatParamB::produce_if_not_constant(carrier_params.portamento_length, round, sample_count);
    FloatParamB::produce_if_not_constant(carrier_params.portamento_depth, round, sample_count);
    FloatParamS::produce_if_not_constant(carrier_params.detune, round, sample_count);
    FloatParamS::produce_if_not_constant(carrier_params.fine_detune, round, sample_count);
    FloatParamB::produce_if_not_constant(carrier_params.width, round, sample_count);
    FloatParamS::produce_if_not_constant(carrier_params.panning, round, sample_count);
    FloatParamS::produce_if_not_constant(carrier_params.volume, round, sample_count);

    FloatParamB::produce_if_not_constant(carrier_params.harmonic_0, round, sample_count);
    FloatParamB::produce_if_not_constant(carrier_params.harmonic_1, round, sample_count);
    FloatParamB::produce_if_not_constant(carrier_params.harmonic_2, round, sample_count);
    FloatParamB::produce_if_not_constant(carrier_params.harmonic_3, round, sample_count);
    FloatParamB::produce_if_not_constant(carrier_params.harmonic_4, round, sample_count);
    FloatParamB::produce_if_not_constant(carrier_params.harmonic_5, round, sample_count);
    FloatParamB::produce_if_not_constant(carrier_params.harmonic_6, round, sample_count);
    FloatParamB::produce_if_not_constant(carrier_params.harmonic_7, round, sample_count);
    FloatParamB::produce_if_not_constant(carrier_params.harmonic_8, round, sample_count);
    FloatParamB::produce_if_not_constant(carrier_params.harmonic_9, round, sample_count);

    FloatParamS::produce_if_not_constant(carrier_params.filter_1_frequency, round, sample_count);
    FloatParamS::produce_if_not_constant(carrier_params.filter_1_q, round, sample_count);
    FloatParamS::produce_if_not_constant(carrier_params.filter_1_gain, round, sample_count);

    FloatParamS::produce_if_not_constant(carrier_params.filter_2_frequency, round, sample_count);
    FloatParamS::produce_if_not_constant(carrier_params.filter_2_q, round, sample_count);
    FloatParamS::produce_if_not_constant(carrier_params.filter_2_gain, round, sample_count);

    FloatParamS::produce_if_not_constant(effects.volume_1_gain, round, sample_count);

    FloatParamS::produce_if_not_constant(effects.overdrive.level, round, sample_count);

    FloatParamS::produce_if_not_constant(effects.distortion.level, round, sample_count);

    FloatParamS::produce_if_not_constant(effects.filter_1.frequency, round, sample_count);
    FloatParamS::produce_if_not_constant(effects.filter_1.q, round, sample_count);
    FloatParamS::produce_if_not_constant(effects.filter_1.gain, round, sample_count);

    FloatParamS::produce_if_not_constant(effects.filter_2.frequency, round, sample_count);
    FloatParamS::produce_if_not_constant(effects.filter_2.q, round, sample_count);
    FloatParamS::produce_if_not_constant(effects.filter_2.gain, round, sample_count);

    FloatParamS::produce_if_not_constant(effects.volume_2_gain, round, sample_count);

    FloatParamS::produce_if_not_constant(effects.chorus.delay_time, round, sample_count);
    FloatParamS::produce_if_not_constant(effects.chorus.frequency, round, sample_count);
    FloatParamS::produce_if_not_constant(effects.chorus.depth, round, sample_count);
    FloatParamS::produce_if_not_constant(effects.chorus.feedback, round, sample_count);
    FloatParamS::produce_if_not_constant(effects.chorus.damping_frequency, round, sample_count);
    FloatParamS::produce_if_not_constant(effects.chorus.damping_gain, round, sample_count);
    FloatParamS::produce_if_not_constant(effects.chorus.width, round, sample_count);
    FloatParamS::produce_if_not_constant(effects.chorus.high_pass_frequency, round, sample_count);
    FloatParamS::produce_if_not_constant(effects.chorus.wet, round, sample_count);
    FloatParamS::produce_if_not_constant(effects.chorus.dry, round, sample_count);

    FloatParamS::produce_if_not_constant(effects.echo.delay_time, round, sample_count);
    FloatParamS::produce_if_not_constant(effects.echo.feedback, round, sample_count);
    FloatParamS::produce_if_not_constant(effects.echo.damping_frequency, round, sample_count);
    FloatParamS::produce_if_not_constant(effects.echo.damping_gain, round, sample_count);
    FloatParamS::produce_if_not_constant(effects.echo.width, round, sample_count);
    FloatParamS::produce_if_not_constant(effects.echo.high_pass_frequency, round, sample_count);
    FloatParamB::produce_if_not_constant(effects.echo.side_chain_compression_threshold, round, sample_count);
    FloatParamB::produce_if_not_constant(effects.echo.side_chain_compression_attack_time, round, sample_count);
    FloatParamB::produce_if_not_constant(effects.echo.side_chain_compression_release_time, round, sample_count);
    FloatParamB::produce_if_not_constant(effects.echo.side_chain_compression_ratio, round, sample_count);
    FloatParamS::produce_if_not_constant(effects.echo.wet, round, sample_count);
    FloatParamS::produce_if_not_constant(effects.echo.dry, round, sample_count);

    FloatParamS::produce_if_not_constant(effects.reverb.room_size, round, sample_count);
    FloatParamS::produce_if_not_constant(effects.reverb.damping_frequency, round, sample_count);
    FloatParamS::produce_if_not_constant(effects.reverb.damping_gain, round, sample_count);
    FloatParamS::produce_if_not_constant(effects.reverb.width, round, sample_count);
    FloatParamS::produce_if_not_constant(effects.reverb.high_pass_frequency, round, sample_count);
    FloatParamB::produce_if_not_constant(effects.reverb.side_chain_compression_threshold, round, sample_count);
    FloatParamB::produce_if_not_constant(effects.reverb.side_chain_compression_attack_time, round, sample_count);
    FloatParamB::produce_if_not_constant(effects.reverb.side_chain_compression_release_time, round, sample_count);
    FloatParamB::produce_if_not_constant(effects.reverb.side_chain_compression_ratio, round, sample_count);
    FloatParamS::produce_if_not_constant(effects.reverb.wet, round, sample_count);
    FloatParamS::produce_if_not_constant(effects.reverb.dry, round, sample_count);

    FloatParamS::produce_if_not_constant(effects.volume_3_gain, round, sample_count);

    for (Integer i = 0; i != LFOS; ++i) {
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
        bool const modulator_decayed = modulator->has_decayed_during_envelope_dahds();

        if (modulator_decayed) {
            note = modulator->get_note();
            channel = modulator->get_channel();
            modulator->cancel_note();
        }

        Carrier* const carrier = carriers[voice];
        bool const carrier_decayed = carrier->has_decayed_during_envelope_dahds();

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

        default:
            break;
    }
}


void Synth::handle_set_param(ParamId const param_id, Number const ratio) noexcept
{
    if (ParamId::M1IN <= param_id && param_id <= ParamId::M20RND) {
        int const offset = (int)param_id - (int)ParamId::M1IN;
        int const macro_idx = offset / MACRO_FLOAT_PARAMS;
        int const param_idx = offset % MACRO_FLOAT_PARAMS;

        switch (param_idx) {
            case 0: macros_rw[macro_idx]->input.set_ratio(ratio); break;
            case 1: macros_rw[macro_idx]->min.set_ratio(ratio); break;
            case 2: macros_rw[macro_idx]->max.set_ratio(ratio); break;
            case 3: macros_rw[macro_idx]->amount.set_ratio(ratio); break;
            case 4: macros_rw[macro_idx]->distortion.set_ratio(ratio); break;
            case 5: macros_rw[macro_idx]->randomness.set_ratio(ratio); break;
            default: break; /* This should never be reached. */
        }
    } else if (ParamId::N1AMT <= param_id && param_id <= N6FIN) {
        int const offset = (int)param_id - (int)ParamId::N1AMT;
        int const envelope_idx = offset / ENVELOPE_FLOAT_PARAMS;
        int const param_idx = offset % ENVELOPE_FLOAT_PARAMS;

        switch (param_idx) {
            case 0: envelopes_rw[envelope_idx]->amount.set_ratio(ratio); break;
            case 1: envelopes_rw[envelope_idx]->initial_value.set_ratio(ratio); break;
            case 2: envelopes_rw[envelope_idx]->delay_time.set_ratio(ratio); break;
            case 3: envelopes_rw[envelope_idx]->attack_time.set_ratio(ratio); break;
            case 4: envelopes_rw[envelope_idx]->peak_value.set_ratio(ratio); break;
            case 5: envelopes_rw[envelope_idx]->hold_time.set_ratio(ratio); break;
            case 6: envelopes_rw[envelope_idx]->decay_time.set_ratio(ratio); break;
            case 7: envelopes_rw[envelope_idx]->sustain_value.set_ratio(ratio); break;
            case 8: envelopes_rw[envelope_idx]->release_time.set_ratio(ratio); break;
            case 9: envelopes_rw[envelope_idx]->final_value.set_ratio(ratio); break;
            default: break; /* This should never be reached. */
        }
    } else if (ParamId::L1FRQ <= param_id && param_id <= L8RND) {
        int const offset = (int)param_id - (int)ParamId::L1FRQ;
        int const lfo_idx = offset / LFO_FLOAT_PARAMS;
        int const param_idx = offset % LFO_FLOAT_PARAMS;

        switch (param_idx) {
            case 0: lfos_rw[lfo_idx]->frequency.set_ratio(ratio); break;
            case 1: lfos_rw[lfo_idx]->phase.set_ratio(ratio); break;
            case 2: lfos_rw[lfo_idx]->min.set_ratio(ratio); break;
            case 3: lfos_rw[lfo_idx]->max.set_ratio(ratio); break;
            case 4: lfos_rw[lfo_idx]->amount.set_ratio(ratio); break;
            case 5: lfos_rw[lfo_idx]->distortion.set_ratio(ratio); break;
            case 6: lfos_rw[lfo_idx]->randomness.set_ratio(ratio); break;
            default: break; /* This should never be reached. */
        }
    } else {
        switch (param_id) {
            case ParamId::MIX: modulator_add_volume.set_ratio(ratio); break;
            case ParamId::PM: phase_modulation_level.set_ratio(ratio); break;
            case ParamId::FM: frequency_modulation_level.set_ratio(ratio); break;
            case ParamId::AM: amplitude_modulation_level.set_ratio(ratio); break;
            case ParamId::MAMP: modulator_params.amplitude.set_ratio(ratio); break;
            case ParamId::MVS: modulator_params.velocity_sensitivity.set_ratio(ratio); break;
            case ParamId::MFLD: modulator_params.folding.set_ratio(ratio); break;
            case ParamId::MPRT: modulator_params.portamento_length.set_ratio(ratio); break;
            case ParamId::MPRD: modulator_params.portamento_depth.set_ratio(ratio); break;
            case ParamId::MDTN: modulator_params.detune.set_ratio(ratio); break;
            case ParamId::MFIN: modulator_params.fine_detune.set_ratio(ratio); break;
            case ParamId::MWID: modulator_params.width.set_ratio(ratio); break;
            case ParamId::MPAN: modulator_params.panning.set_ratio(ratio); break;
            case ParamId::MVOL: modulator_params.volume.set_ratio(ratio); break;
            case ParamId::MSUB: modulator_params.subharmonic_amplitude.set_ratio(ratio); break;
            case ParamId::MC1: modulator_params.harmonic_0.set_ratio(ratio); break;
            case ParamId::MC2: modulator_params.harmonic_1.set_ratio(ratio); break;
            case ParamId::MC3: modulator_params.harmonic_2.set_ratio(ratio); break;
            case ParamId::MC4: modulator_params.harmonic_3.set_ratio(ratio); break;
            case ParamId::MC5: modulator_params.harmonic_4.set_ratio(ratio); break;
            case ParamId::MC6: modulator_params.harmonic_5.set_ratio(ratio); break;
            case ParamId::MC7: modulator_params.harmonic_6.set_ratio(ratio); break;
            case ParamId::MC8: modulator_params.harmonic_7.set_ratio(ratio); break;
            case ParamId::MC9: modulator_params.harmonic_8.set_ratio(ratio); break;
            case ParamId::MC10: modulator_params.harmonic_9.set_ratio(ratio); break;
            case ParamId::MF1FRQ: modulator_params.filter_1_frequency.set_ratio(ratio); break;
            case ParamId::MF1Q: modulator_params.filter_1_q.set_ratio(ratio); break;
            case ParamId::MF1G: modulator_params.filter_1_gain.set_ratio(ratio); break;
            case ParamId::MF2FRQ: modulator_params.filter_2_frequency.set_ratio(ratio); break;
            case ParamId::MF2Q: modulator_params.filter_2_q.set_ratio(ratio); break;
            case ParamId::MF2G: modulator_params.filter_2_gain.set_ratio(ratio); break;
            case ParamId::CAMP: carrier_params.amplitude.set_ratio(ratio); break;
            case ParamId::CVS: carrier_params.velocity_sensitivity.set_ratio(ratio); break;
            case ParamId::CFLD: carrier_params.folding.set_ratio(ratio); break;
            case ParamId::CPRT: carrier_params.portamento_length.set_ratio(ratio); break;
            case ParamId::CPRD: carrier_params.portamento_depth.set_ratio(ratio); break;
            case ParamId::CDTN: carrier_params.detune.set_ratio(ratio); break;
            case ParamId::CFIN: carrier_params.fine_detune.set_ratio(ratio); break;
            case ParamId::CWID: carrier_params.width.set_ratio(ratio); break;
            case ParamId::CPAN: carrier_params.panning.set_ratio(ratio); break;
            case ParamId::CVOL: carrier_params.volume.set_ratio(ratio); break;
            case ParamId::CDG: carrier_params.distortion.set_ratio(ratio); break;
            case ParamId::CC1: carrier_params.harmonic_0.set_ratio(ratio); break;
            case ParamId::CC2: carrier_params.harmonic_1.set_ratio(ratio); break;
            case ParamId::CC3: carrier_params.harmonic_2.set_ratio(ratio); break;
            case ParamId::CC4: carrier_params.harmonic_3.set_ratio(ratio); break;
            case ParamId::CC5: carrier_params.harmonic_4.set_ratio(ratio); break;
            case ParamId::CC6: carrier_params.harmonic_5.set_ratio(ratio); break;
            case ParamId::CC7: carrier_params.harmonic_6.set_ratio(ratio); break;
            case ParamId::CC8: carrier_params.harmonic_7.set_ratio(ratio); break;
            case ParamId::CC9: carrier_params.harmonic_8.set_ratio(ratio); break;
            case ParamId::CC10: carrier_params.harmonic_9.set_ratio(ratio); break;
            case ParamId::CF1FRQ: carrier_params.filter_1_frequency.set_ratio(ratio); break;
            case ParamId::CF1Q: carrier_params.filter_1_q.set_ratio(ratio); break;
            case ParamId::CF1G: carrier_params.filter_1_gain.set_ratio(ratio); break;
            case ParamId::CF2FRQ: carrier_params.filter_2_frequency.set_ratio(ratio); break;
            case ParamId::CF2Q: carrier_params.filter_2_q.set_ratio(ratio); break;
            case ParamId::CF2G: carrier_params.filter_2_gain.set_ratio(ratio); break;
            case ParamId::EV1V: effects.volume_1_gain.set_ratio(ratio); break;
            case ParamId::EOG: effects.overdrive.level.set_ratio(ratio); break;
            case ParamId::EDG: effects.distortion.level.set_ratio(ratio); break;
            case ParamId::EF1FRQ: effects.filter_1.frequency.set_ratio(ratio); break;
            case ParamId::EF1Q: effects.filter_1.q.set_ratio(ratio); break;
            case ParamId::EF1G: effects.filter_1.gain.set_ratio(ratio); break;
            case ParamId::EF2FRQ: effects.filter_2.frequency.set_ratio(ratio); break;
            case ParamId::EF2Q: effects.filter_2.q.set_ratio(ratio); break;
            case ParamId::EF2G: effects.filter_2.gain.set_ratio(ratio); break;
            case ParamId::EV2V: effects.volume_2_gain.set_ratio(ratio); break;
            case ParamId::ECDEL: effects.chorus.delay_time.set_ratio(ratio); break;
            case ParamId::ECFRQ: effects.chorus.frequency.set_ratio(ratio); break;
            case ParamId::ECDPT: effects.chorus.depth.set_ratio(ratio); break;
            case ParamId::ECFB: effects.chorus.feedback.set_ratio(ratio); break;
            case ParamId::ECDF: effects.chorus.damping_frequency.set_ratio(ratio); break;
            case ParamId::ECDG: effects.chorus.damping_gain.set_ratio(ratio); break;
            case ParamId::ECWID: effects.chorus.width.set_ratio(ratio); break;
            case ParamId::ECHPF: effects.chorus.high_pass_frequency.set_ratio(ratio); break;
            case ParamId::ECWET: effects.chorus.wet.set_ratio(ratio); break;
            case ParamId::ECDRY: effects.chorus.dry.set_ratio(ratio); break;
            case ParamId::EEDEL: effects.echo.delay_time.set_ratio(ratio); break;
            case ParamId::EEFB: effects.echo.feedback.set_ratio(ratio); break;
            case ParamId::EEDF: effects.echo.damping_frequency.set_ratio(ratio); break;
            case ParamId::EEDG: effects.echo.damping_gain.set_ratio(ratio); break;
            case ParamId::EEWID: effects.echo.width.set_ratio(ratio); break;
            case ParamId::EEHPF: effects.echo.high_pass_frequency.set_ratio(ratio); break;
            case ParamId::EECTH: effects.echo.side_chain_compression_threshold.set_ratio(ratio); break;
            case ParamId::EECAT: effects.echo.side_chain_compression_attack_time.set_ratio(ratio); break;
            case ParamId::EECRL: effects.echo.side_chain_compression_release_time.set_ratio(ratio); break;
            case ParamId::EECR: effects.echo.side_chain_compression_ratio.set_ratio(ratio); break;
            case ParamId::EEWET: effects.echo.wet.set_ratio(ratio); break;
            case ParamId::EEDRY: effects.echo.dry.set_ratio(ratio); break;
            case ParamId::ERRS: effects.reverb.room_size.set_ratio(ratio); break;
            case ParamId::ERDF: effects.reverb.damping_frequency.set_ratio(ratio); break;
            case ParamId::ERDG: effects.reverb.damping_gain.set_ratio(ratio); break;
            case ParamId::ERWID: effects.reverb.width.set_ratio(ratio); break;
            case ParamId::ERHPF: effects.reverb.high_pass_frequency.set_ratio(ratio); break;
            case ParamId::ERCTH: effects.reverb.side_chain_compression_threshold.set_ratio(ratio); break;
            case ParamId::ERCAT: effects.reverb.side_chain_compression_attack_time.set_ratio(ratio); break;
            case ParamId::ERCRL: effects.reverb.side_chain_compression_release_time.set_ratio(ratio); break;
            case ParamId::ERCR: effects.reverb.side_chain_compression_ratio.set_ratio(ratio); break;
            case ParamId::ERWET: effects.reverb.wet.set_ratio(ratio); break;
            case ParamId::ERDRY: effects.reverb.dry.set_ratio(ratio); break;
            case ParamId::EV3V: effects.volume_3_gain.set_ratio(ratio); break;
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
            case ParamId::POLY: polyphonic.set_ratio(ratio); break;
            case ParamId::ERTYP: effects.reverb.type.set_ratio(ratio); break;
            case ParamId::ECTYP: effects.chorus.type.set_ratio(ratio); break;
            case ParamId::MTUN: modulator_params.tuning.set_ratio(ratio); break;
            case ParamId::CTUN: carrier_params.tuning.set_ratio(ratio); break;
            default: break; /* This should never be reached. */
        }
    }

    handle_refresh_param(param_id);
}


void Synth::handle_assign_controller(
        ParamId const param_id,
        Byte const controller_id
) noexcept {
    ControllerId const ctl_id = (ControllerId)controller_id;
    bool is_assigned = false;

    if (ParamId::M1IN <= param_id && param_id <= ParamId::M20RND) {
        int const offset = (int)param_id - (int)ParamId::M1IN;
        int const macro_idx = offset / MACRO_FLOAT_PARAMS;
        int const param_idx = offset % MACRO_FLOAT_PARAMS;

        switch (param_idx) {
            case 0: is_assigned = assign_controller<FloatParamB>(macros_rw[macro_idx]->input, ctl_id); break;
            case 1: is_assigned = assign_controller<FloatParamB>(macros_rw[macro_idx]->min, ctl_id); break;
            case 2: is_assigned = assign_controller<FloatParamB>(macros_rw[macro_idx]->max, ctl_id); break;
            case 3: is_assigned = assign_controller<FloatParamB>(macros_rw[macro_idx]->amount, ctl_id); break;
            case 4: is_assigned = assign_controller<FloatParamB>(macros_rw[macro_idx]->distortion, ctl_id); break;
            case 5: is_assigned = assign_controller<FloatParamB>(macros_rw[macro_idx]->randomness, ctl_id); break;
            default: break; /* This should never be reached. */
        }
    } else if (ParamId::N1AMT <= param_id && param_id <= N6FIN) {
        int const offset = (int)param_id - (int)ParamId::N1AMT;
        int const envelope_idx = offset / ENVELOPE_FLOAT_PARAMS;
        int const param_idx = offset % ENVELOPE_FLOAT_PARAMS;

        switch (param_idx) {
            case 0: is_assigned = assign_controller<FloatParamB>(envelopes_rw[envelope_idx]->amount, ctl_id); break;
            case 1: is_assigned = assign_controller<FloatParamB>(envelopes_rw[envelope_idx]->initial_value, ctl_id); break;
            case 2: is_assigned = assign_controller<FloatParamB>(envelopes_rw[envelope_idx]->delay_time, ctl_id); break;
            case 3: is_assigned = assign_controller<FloatParamB>(envelopes_rw[envelope_idx]->attack_time, ctl_id); break;
            case 4: is_assigned = assign_controller<FloatParamB>(envelopes_rw[envelope_idx]->peak_value, ctl_id); break;
            case 5: is_assigned = assign_controller<FloatParamB>(envelopes_rw[envelope_idx]->hold_time, ctl_id); break;
            case 6: is_assigned = assign_controller<FloatParamB>(envelopes_rw[envelope_idx]->decay_time, ctl_id); break;
            case 7: is_assigned = assign_controller<FloatParamB>(envelopes_rw[envelope_idx]->sustain_value, ctl_id); break;
            case 8: is_assigned = assign_controller<FloatParamB>(envelopes_rw[envelope_idx]->release_time, ctl_id); break;
            case 9: is_assigned = assign_controller<FloatParamB>(envelopes_rw[envelope_idx]->final_value, ctl_id); break;
            default: break; /* This should never be reached. */
        }
    } else if (ParamId::L1FRQ <= param_id && param_id <= L8RND) {
        int const offset = (int)param_id - (int)ParamId::L1FRQ;
        int const lfo_idx = offset / LFO_FLOAT_PARAMS;
        int const param_idx = offset % LFO_FLOAT_PARAMS;

        switch (param_idx) {
            case 0: is_assigned = assign_controller<FloatParamS>(lfos_rw[lfo_idx]->frequency, ctl_id); break;
            case 1: is_assigned = assign_controller<FloatParamS>(lfos_rw[lfo_idx]->phase, ctl_id); break;
            case 2: is_assigned = assign_controller<FloatParamS>(lfos_rw[lfo_idx]->min, ctl_id); break;
            case 3: is_assigned = assign_controller<FloatParamS>(lfos_rw[lfo_idx]->max, ctl_id); break;
            case 4: is_assigned = assign_controller<FloatParamS>(lfos_rw[lfo_idx]->amount, ctl_id); break;
            case 5: is_assigned = assign_controller<FloatParamS>(lfos_rw[lfo_idx]->distortion, ctl_id); break;
            case 6: is_assigned = assign_controller<FloatParamS>(lfos_rw[lfo_idx]->randomness, ctl_id); break;
            default: break; /* This should never be reached. */
        }
    } else if (param_id < FLOAT_PARAMS) {
        switch (param_id) {
            case ParamId::MIX: is_assigned = assign_controller<FloatParamS>(modulator_add_volume, ctl_id); break;
            case ParamId::PM: is_assigned = assign_controller<FloatParamS>(phase_modulation_level, ctl_id); break;
            case ParamId::FM: is_assigned = assign_controller<FloatParamS>(frequency_modulation_level, ctl_id); break;
            case ParamId::AM: is_assigned = assign_controller<FloatParamS>(amplitude_modulation_level, ctl_id); break;
            case ParamId::MAMP: is_assigned = assign_controller<FloatParamS>(modulator_params.amplitude, ctl_id); break;
            case ParamId::MVS: is_assigned = assign_controller<FloatParamB>(modulator_params.velocity_sensitivity, ctl_id); break;
            case ParamId::MFLD: is_assigned = assign_controller<FloatParamS>(modulator_params.folding, ctl_id); break;
            case ParamId::MPRT: is_assigned = assign_controller<FloatParamB>(modulator_params.portamento_length, ctl_id); break;
            case ParamId::MPRD: is_assigned = assign_controller<FloatParamB>(modulator_params.portamento_depth, ctl_id); break;
            case ParamId::MDTN: is_assigned = assign_controller<FloatParamS>(modulator_params.detune, ctl_id); break;
            case ParamId::MFIN: is_assigned = assign_controller<FloatParamS>(modulator_params.fine_detune, ctl_id); break;
            case ParamId::MWID: is_assigned = assign_controller<FloatParamB>(modulator_params.width, ctl_id); break;
            case ParamId::MPAN: is_assigned = assign_controller<FloatParamS>(modulator_params.panning, ctl_id); break;
            case ParamId::MVOL: is_assigned = assign_controller<FloatParamS>(modulator_params.volume, ctl_id); break;
            case ParamId::MSUB: is_assigned = assign_controller<FloatParamS>(modulator_params.subharmonic_amplitude, ctl_id); break;
            case ParamId::MC1: is_assigned = assign_controller<FloatParamB>(modulator_params.harmonic_0, ctl_id); break;
            case ParamId::MC2: is_assigned = assign_controller<FloatParamB>(modulator_params.harmonic_1, ctl_id); break;
            case ParamId::MC3: is_assigned = assign_controller<FloatParamB>(modulator_params.harmonic_2, ctl_id); break;
            case ParamId::MC4: is_assigned = assign_controller<FloatParamB>(modulator_params.harmonic_3, ctl_id); break;
            case ParamId::MC5: is_assigned = assign_controller<FloatParamB>(modulator_params.harmonic_4, ctl_id); break;
            case ParamId::MC6: is_assigned = assign_controller<FloatParamB>(modulator_params.harmonic_5, ctl_id); break;
            case ParamId::MC7: is_assigned = assign_controller<FloatParamB>(modulator_params.harmonic_6, ctl_id); break;
            case ParamId::MC8: is_assigned = assign_controller<FloatParamB>(modulator_params.harmonic_7, ctl_id); break;
            case ParamId::MC9: is_assigned = assign_controller<FloatParamB>(modulator_params.harmonic_8, ctl_id); break;
            case ParamId::MC10: is_assigned = assign_controller<FloatParamB>(modulator_params.harmonic_9, ctl_id); break;
            case ParamId::MF1FRQ: is_assigned = assign_controller<FloatParamS>(modulator_params.filter_1_frequency, ctl_id); break;
            case ParamId::MF1Q: is_assigned = assign_controller<FloatParamS>(modulator_params.filter_1_q, ctl_id); break;
            case ParamId::MF1G: is_assigned = assign_controller<FloatParamS>(modulator_params.filter_1_gain, ctl_id); break;
            case ParamId::MF2FRQ: is_assigned = assign_controller<FloatParamS>(modulator_params.filter_2_frequency, ctl_id); break;
            case ParamId::MF2Q: is_assigned = assign_controller<FloatParamS>(modulator_params.filter_2_q, ctl_id); break;
            case ParamId::MF2G: is_assigned = assign_controller<FloatParamS>(modulator_params.filter_2_gain, ctl_id); break;
            case ParamId::CAMP: is_assigned = assign_controller<FloatParamS>(carrier_params.amplitude, ctl_id); break;
            case ParamId::CVS: is_assigned = assign_controller<FloatParamB>(carrier_params.velocity_sensitivity, ctl_id); break;
            case ParamId::CFLD: is_assigned = assign_controller<FloatParamS>(carrier_params.folding, ctl_id); break;
            case ParamId::CPRT: is_assigned = assign_controller<FloatParamB>(carrier_params.portamento_length, ctl_id); break;
            case ParamId::CPRD: is_assigned = assign_controller<FloatParamB>(carrier_params.portamento_depth, ctl_id); break;
            case ParamId::CDTN: is_assigned = assign_controller<FloatParamS>(carrier_params.detune, ctl_id); break;
            case ParamId::CFIN: is_assigned = assign_controller<FloatParamS>(carrier_params.fine_detune, ctl_id); break;
            case ParamId::CWID: is_assigned = assign_controller<FloatParamB>(carrier_params.width, ctl_id); break;
            case ParamId::CPAN: is_assigned = assign_controller<FloatParamS>(carrier_params.panning, ctl_id); break;
            case ParamId::CVOL: is_assigned = assign_controller<FloatParamS>(carrier_params.volume, ctl_id); break;
            case ParamId::CDG: is_assigned = assign_controller<FloatParamS>(carrier_params.distortion, ctl_id); break;
            case ParamId::CC1: is_assigned = assign_controller<FloatParamB>(carrier_params.harmonic_0, ctl_id); break;
            case ParamId::CC2: is_assigned = assign_controller<FloatParamB>(carrier_params.harmonic_1, ctl_id); break;
            case ParamId::CC3: is_assigned = assign_controller<FloatParamB>(carrier_params.harmonic_2, ctl_id); break;
            case ParamId::CC4: is_assigned = assign_controller<FloatParamB>(carrier_params.harmonic_3, ctl_id); break;
            case ParamId::CC5: is_assigned = assign_controller<FloatParamB>(carrier_params.harmonic_4, ctl_id); break;
            case ParamId::CC6: is_assigned = assign_controller<FloatParamB>(carrier_params.harmonic_5, ctl_id); break;
            case ParamId::CC7: is_assigned = assign_controller<FloatParamB>(carrier_params.harmonic_6, ctl_id); break;
            case ParamId::CC8: is_assigned = assign_controller<FloatParamB>(carrier_params.harmonic_7, ctl_id); break;
            case ParamId::CC9: is_assigned = assign_controller<FloatParamB>(carrier_params.harmonic_8, ctl_id); break;
            case ParamId::CC10: is_assigned = assign_controller<FloatParamB>(carrier_params.harmonic_9, ctl_id); break;
            case ParamId::CF1FRQ: is_assigned = assign_controller<FloatParamS>(carrier_params.filter_1_frequency, ctl_id); break;
            case ParamId::CF1Q: is_assigned = assign_controller<FloatParamS>(carrier_params.filter_1_q, ctl_id); break;
            case ParamId::CF1G: is_assigned = assign_controller<FloatParamS>(carrier_params.filter_1_gain, ctl_id); break;
            case ParamId::CF2FRQ: is_assigned = assign_controller<FloatParamS>(carrier_params.filter_2_frequency, ctl_id); break;
            case ParamId::CF2Q: is_assigned = assign_controller<FloatParamS>(carrier_params.filter_2_q, ctl_id); break;
            case ParamId::CF2G: is_assigned = assign_controller<FloatParamS>(carrier_params.filter_2_gain, ctl_id); break;
            case ParamId::EV1V: is_assigned = assign_controller<FloatParamS>(effects.volume_1_gain, ctl_id); break;
            case ParamId::EOG: is_assigned = assign_controller<FloatParamS>(effects.overdrive.level, ctl_id); break;
            case ParamId::EDG: is_assigned = assign_controller<FloatParamS>(effects.distortion.level, ctl_id); break;
            case ParamId::EF1FRQ: is_assigned = assign_controller<FloatParamS>(effects.filter_1.frequency, ctl_id); break;
            case ParamId::EF1Q: is_assigned = assign_controller<FloatParamS>(effects.filter_1.q, ctl_id); break;
            case ParamId::EF1G: is_assigned = assign_controller<FloatParamS>(effects.filter_1.gain, ctl_id); break;
            case ParamId::EF2FRQ: is_assigned = assign_controller<FloatParamS>(effects.filter_2.frequency, ctl_id); break;
            case ParamId::EF2Q: is_assigned = assign_controller<FloatParamS>(effects.filter_2.q, ctl_id); break;
            case ParamId::EF2G: is_assigned = assign_controller<FloatParamS>(effects.filter_2.gain, ctl_id); break;
            case ParamId::EV2V: is_assigned = assign_controller<FloatParamS>(effects.volume_2_gain, ctl_id); break;
            case ParamId::ECDEL: is_assigned = assign_controller<FloatParamS>(effects.chorus.delay_time, ctl_id); break;
            case ParamId::ECFRQ: is_assigned = assign_controller<FloatParamS>(effects.chorus.frequency, ctl_id); break;
            case ParamId::ECDPT: is_assigned = assign_controller<FloatParamS>(effects.chorus.depth, ctl_id); break;
            case ParamId::ECFB: is_assigned = assign_controller<FloatParamS>(effects.chorus.feedback, ctl_id); break;
            case ParamId::ECDF: is_assigned = assign_controller<FloatParamS>(effects.chorus.damping_frequency, ctl_id); break;
            case ParamId::ECDG: is_assigned = assign_controller<FloatParamS>(effects.chorus.damping_gain, ctl_id); break;
            case ParamId::ECWID: is_assigned = assign_controller<FloatParamS>(effects.chorus.width, ctl_id); break;
            case ParamId::ECHPF: is_assigned = assign_controller<FloatParamS>(effects.chorus.high_pass_frequency, ctl_id); break;
            case ParamId::ECWET: is_assigned = assign_controller<FloatParamS>(effects.chorus.wet, ctl_id); break;
            case ParamId::ECDRY: is_assigned = assign_controller<FloatParamS>(effects.chorus.dry, ctl_id); break;
            case ParamId::EEDEL: is_assigned = assign_controller<FloatParamS>(effects.echo.delay_time, ctl_id); break;
            case ParamId::EEFB: is_assigned = assign_controller<FloatParamS>(effects.echo.feedback, ctl_id); break;
            case ParamId::EEDF: is_assigned = assign_controller<FloatParamS>(effects.echo.damping_frequency, ctl_id); break;
            case ParamId::EEDG: is_assigned = assign_controller<FloatParamS>(effects.echo.damping_gain, ctl_id); break;
            case ParamId::EEWID: is_assigned = assign_controller<FloatParamS>(effects.echo.width, ctl_id); break;
            case ParamId::EEHPF: is_assigned = assign_controller<FloatParamS>(effects.echo.high_pass_frequency, ctl_id); break;
            case ParamId::EECTH: is_assigned = assign_controller<FloatParamB>(effects.echo.side_chain_compression_threshold, ctl_id); break;
            case ParamId::EECAT: is_assigned = assign_controller<FloatParamB>(effects.echo.side_chain_compression_attack_time, ctl_id); break;
            case ParamId::EECRL: is_assigned = assign_controller<FloatParamB>(effects.echo.side_chain_compression_release_time, ctl_id); break;
            case ParamId::EECR: is_assigned = assign_controller<FloatParamB>(effects.echo.side_chain_compression_ratio, ctl_id); break;
            case ParamId::EEWET: is_assigned = assign_controller<FloatParamS>(effects.echo.wet, ctl_id); break;
            case ParamId::EEDRY: is_assigned = assign_controller<FloatParamS>(effects.echo.dry, ctl_id); break;
            case ParamId::ERRS: is_assigned = assign_controller<FloatParamS>(effects.reverb.room_size, ctl_id); break;
            case ParamId::ERDF: is_assigned = assign_controller<FloatParamS>(effects.reverb.damping_frequency, ctl_id); break;
            case ParamId::ERDG: is_assigned = assign_controller<FloatParamS>(effects.reverb.damping_gain, ctl_id); break;
            case ParamId::ERWID: is_assigned = assign_controller<FloatParamS>(effects.reverb.width, ctl_id); break;
            case ParamId::ERHPF: is_assigned = assign_controller<FloatParamS>(effects.reverb.high_pass_frequency, ctl_id); break;
            case ParamId::ERCTH: is_assigned = assign_controller<FloatParamB>(effects.reverb.side_chain_compression_threshold, ctl_id); break;
            case ParamId::ERCAT: is_assigned = assign_controller<FloatParamB>(effects.reverb.side_chain_compression_attack_time, ctl_id); break;
            case ParamId::ERCRL: is_assigned = assign_controller<FloatParamB>(effects.reverb.side_chain_compression_release_time, ctl_id); break;
            case ParamId::ERCR: is_assigned = assign_controller<FloatParamB>(effects.reverb.side_chain_compression_ratio, ctl_id); break;
            case ParamId::ERWET: is_assigned = assign_controller<FloatParamS>(effects.reverb.wet, ctl_id); break;
            case ParamId::ERDRY: is_assigned = assign_controller<FloatParamS>(effects.reverb.dry, ctl_id); break;
            case ParamId::EV3V: is_assigned = assign_controller<FloatParamS>(effects.volume_3_gain, ctl_id); break;
            default: break;
        }
    } else {
        is_assigned = assign_controller_to_discrete_param(param_id, ctl_id);
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


bool Synth::assign_controller_to_discrete_param(
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
        case NOTE: midi_controller = &note; break;
        case VELOCITY: midi_controller = &velocity; break;

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

        case OSC_1_PEAK: midi_controller = &osc_1_peak; break;
        case OSC_2_PEAK: midi_controller = &osc_2_peak; break;
        case VOL_1_PEAK: midi_controller = &vol_1_peak; break;
        case VOL_2_PEAK: midi_controller = &vol_2_peak; break;
        case VOL_3_PEAK: midi_controller = &vol_3_peak; break;

        case MIDI_LEARN: is_special = true; break;

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
        case NOTE: param.set_midi_controller(&note); return true;
        case VELOCITY: param.set_midi_controller(&velocity); return true;

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

        case OSC_1_PEAK: param.set_midi_controller(&osc_1_peak); return true;
        case OSC_2_PEAK: param.set_midi_controller(&osc_2_peak); return true;
        case VOL_1_PEAK: param.set_midi_controller(&vol_1_peak); return true;
        case VOL_2_PEAK: param.set_midi_controller(&vol_2_peak); return true;
        case VOL_3_PEAK: param.set_midi_controller(&vol_3_peak); return true;

        case MIDI_LEARN: return true;

        default: {
            if (is_supported_midi_controller(controller_id)) {
                param.set_midi_controller(midi_controllers_rw[controller_id]);

                return true;
            }

            break;
        }
    }

    return false;
}


Number Synth::get_param_ratio(ParamId const param_id) const noexcept
{
    if (ParamId::M1IN <= param_id && param_id <= ParamId::M20RND) {
        int const offset = (int)param_id - (int)ParamId::M1IN;
        int const macro_idx = offset / MACRO_FLOAT_PARAMS;
        int const param_idx = offset % MACRO_FLOAT_PARAMS;

        switch (param_idx) {
            case 0: return macros_rw[macro_idx]->input.get_ratio();
            case 1: return macros_rw[macro_idx]->min.get_ratio();
            case 2: return macros_rw[macro_idx]->max.get_ratio();
            case 3: return macros_rw[macro_idx]->amount.get_ratio();
            case 4: return macros_rw[macro_idx]->distortion.get_ratio();
            case 5: return macros_rw[macro_idx]->randomness.get_ratio();
            default: return 0.0; /* This should never be reached. */
        }
    } else if (ParamId::N1AMT <= param_id && param_id <= N6FIN) {
        int const offset = (int)param_id - (int)ParamId::N1AMT;
        int const envelope_idx = offset / ENVELOPE_FLOAT_PARAMS;
        int const param_idx = offset % ENVELOPE_FLOAT_PARAMS;

        switch (param_idx) {
            case 0: return envelopes_rw[envelope_idx]->amount.get_ratio();
            case 1: return envelopes_rw[envelope_idx]->initial_value.get_ratio();
            case 2: return envelopes_rw[envelope_idx]->delay_time.get_ratio();
            case 3: return envelopes_rw[envelope_idx]->attack_time.get_ratio();
            case 4: return envelopes_rw[envelope_idx]->peak_value.get_ratio();
            case 5: return envelopes_rw[envelope_idx]->hold_time.get_ratio();
            case 6: return envelopes_rw[envelope_idx]->decay_time.get_ratio();
            case 7: return envelopes_rw[envelope_idx]->sustain_value.get_ratio();
            case 8: return envelopes_rw[envelope_idx]->release_time.get_ratio();
            case 9: return envelopes_rw[envelope_idx]->final_value.get_ratio();
            default: return 0.0; /* This should never be reached. */
        }
    } else if (ParamId::L1FRQ <= param_id && param_id <= L8RND) {
        int const offset = (int)param_id - (int)ParamId::L1FRQ;
        int const lfo_idx = offset / LFO_FLOAT_PARAMS;
        int const param_idx = offset % LFO_FLOAT_PARAMS;

        switch (param_idx) {
            case 0: return lfos_rw[lfo_idx]->frequency.get_ratio();
            case 1: return lfos_rw[lfo_idx]->phase.get_ratio();
            case 2: return lfos_rw[lfo_idx]->min.get_ratio();
            case 3: return lfos_rw[lfo_idx]->max.get_ratio();
            case 4: return lfos_rw[lfo_idx]->amount.get_ratio();
            case 5: return lfos_rw[lfo_idx]->distortion.get_ratio();
            case 6: return lfos_rw[lfo_idx]->randomness.get_ratio();
            default: return 0.0; /* This should never be reached. */
        }
    }

    switch (param_id) {
        case ParamId::MIX: return modulator_add_volume.get_ratio();
        case ParamId::PM: return phase_modulation_level.get_ratio();
        case ParamId::FM: return frequency_modulation_level.get_ratio();
        case ParamId::AM: return amplitude_modulation_level.get_ratio();
        case ParamId::MAMP: return modulator_params.amplitude.get_ratio();
        case ParamId::MVS: return modulator_params.velocity_sensitivity.get_ratio();
        case ParamId::MFLD: return modulator_params.folding.get_ratio();
        case ParamId::MPRT: return modulator_params.portamento_length.get_ratio();
        case ParamId::MPRD: return modulator_params.portamento_depth.get_ratio();
        case ParamId::MDTN: return modulator_params.detune.get_ratio();
        case ParamId::MFIN: return modulator_params.fine_detune.get_ratio();
        case ParamId::MWID: return modulator_params.width.get_ratio();
        case ParamId::MPAN: return modulator_params.panning.get_ratio();
        case ParamId::MVOL: return modulator_params.volume.get_ratio();
        case ParamId::MSUB: return modulator_params.subharmonic_amplitude.get_ratio();
        case ParamId::MC1: return modulator_params.harmonic_0.get_ratio();
        case ParamId::MC2: return modulator_params.harmonic_1.get_ratio();
        case ParamId::MC3: return modulator_params.harmonic_2.get_ratio();
        case ParamId::MC4: return modulator_params.harmonic_3.get_ratio();
        case ParamId::MC5: return modulator_params.harmonic_4.get_ratio();
        case ParamId::MC6: return modulator_params.harmonic_5.get_ratio();
        case ParamId::MC7: return modulator_params.harmonic_6.get_ratio();
        case ParamId::MC8: return modulator_params.harmonic_7.get_ratio();
        case ParamId::MC9: return modulator_params.harmonic_8.get_ratio();
        case ParamId::MC10: return modulator_params.harmonic_9.get_ratio();
        case ParamId::MF1FRQ: return modulator_params.filter_1_frequency.get_ratio();
        case ParamId::MF1Q: return modulator_params.filter_1_q.get_ratio();
        case ParamId::MF1G: return modulator_params.filter_1_gain.get_ratio();
        case ParamId::MF2FRQ: return modulator_params.filter_2_frequency.get_ratio();
        case ParamId::MF2Q: return modulator_params.filter_2_q.get_ratio();
        case ParamId::MF2G: return modulator_params.filter_2_gain.get_ratio();
        case ParamId::CAMP: return carrier_params.amplitude.get_ratio();
        case ParamId::CVS: return carrier_params.velocity_sensitivity.get_ratio();
        case ParamId::CFLD: return carrier_params.folding.get_ratio();
        case ParamId::CPRT: return carrier_params.portamento_length.get_ratio();
        case ParamId::CPRD: return carrier_params.portamento_depth.get_ratio();
        case ParamId::CDTN: return carrier_params.detune.get_ratio();
        case ParamId::CFIN: return carrier_params.fine_detune.get_ratio();
        case ParamId::CWID: return carrier_params.width.get_ratio();
        case ParamId::CPAN: return carrier_params.panning.get_ratio();
        case ParamId::CVOL: return carrier_params.volume.get_ratio();
        case ParamId::CDG: return carrier_params.distortion.get_ratio();
        case ParamId::CC1: return carrier_params.harmonic_0.get_ratio();
        case ParamId::CC2: return carrier_params.harmonic_1.get_ratio();
        case ParamId::CC3: return carrier_params.harmonic_2.get_ratio();
        case ParamId::CC4: return carrier_params.harmonic_3.get_ratio();
        case ParamId::CC5: return carrier_params.harmonic_4.get_ratio();
        case ParamId::CC6: return carrier_params.harmonic_5.get_ratio();
        case ParamId::CC7: return carrier_params.harmonic_6.get_ratio();
        case ParamId::CC8: return carrier_params.harmonic_7.get_ratio();
        case ParamId::CC9: return carrier_params.harmonic_8.get_ratio();
        case ParamId::CC10: return carrier_params.harmonic_9.get_ratio();
        case ParamId::CF1FRQ: return carrier_params.filter_1_frequency.get_ratio();
        case ParamId::CF1Q: return carrier_params.filter_1_q.get_ratio();
        case ParamId::CF1G: return carrier_params.filter_1_gain.get_ratio();
        case ParamId::CF2FRQ: return carrier_params.filter_2_frequency.get_ratio();
        case ParamId::CF2Q: return carrier_params.filter_2_q.get_ratio();
        case ParamId::CF2G: return carrier_params.filter_2_gain.get_ratio();
        case ParamId::EV1V: return effects.volume_1_gain.get_ratio();
        case ParamId::EOG: return effects.overdrive.level.get_ratio();
        case ParamId::EDG: return effects.distortion.level.get_ratio();
        case ParamId::EF1FRQ: return effects.filter_1.frequency.get_ratio();
        case ParamId::EF1Q: return effects.filter_1.q.get_ratio();
        case ParamId::EF1G: return effects.filter_1.gain.get_ratio();
        case ParamId::EF2FRQ: return effects.filter_2.frequency.get_ratio();
        case ParamId::EF2Q: return effects.filter_2.q.get_ratio();
        case ParamId::EF2G: return effects.filter_2.gain.get_ratio();
        case ParamId::EV2V: return effects.volume_2_gain.get_ratio();
        case ParamId::ECDEL: return effects.chorus.delay_time.get_ratio();
        case ParamId::ECFRQ: return effects.chorus.frequency.get_ratio();
        case ParamId::ECDPT: return effects.chorus.depth.get_ratio();
        case ParamId::ECFB: return effects.chorus.feedback.get_ratio();
        case ParamId::ECDF: return effects.chorus.damping_frequency.get_ratio();
        case ParamId::ECDG: return effects.chorus.damping_gain.get_ratio();
        case ParamId::ECWID: return effects.chorus.width.get_ratio();
        case ParamId::ECHPF: return effects.chorus.high_pass_frequency.get_ratio();
        case ParamId::ECWET: return effects.chorus.wet.get_ratio();
        case ParamId::ECDRY: return effects.chorus.dry.get_ratio();
        case ParamId::EEDEL: return effects.echo.delay_time.get_ratio();
        case ParamId::EEFB: return effects.echo.feedback.get_ratio();
        case ParamId::EEDF: return effects.echo.damping_frequency.get_ratio();
        case ParamId::EEDG: return effects.echo.damping_gain.get_ratio();
        case ParamId::EEWID: return effects.echo.width.get_ratio();
        case ParamId::EEHPF: return effects.echo.high_pass_frequency.get_ratio();
        case ParamId::EECTH: return effects.echo.side_chain_compression_threshold.get_ratio();
        case ParamId::EECAT: return effects.echo.side_chain_compression_attack_time.get_ratio();
        case ParamId::EECRL: return effects.echo.side_chain_compression_release_time.get_ratio();
        case ParamId::EECR: return effects.echo.side_chain_compression_ratio.get_ratio();
        case ParamId::EEWET: return effects.echo.wet.get_ratio();
        case ParamId::EEDRY: return effects.echo.dry.get_ratio();
        case ParamId::ERRS: return effects.reverb.room_size.get_ratio();
        case ParamId::ERDF: return effects.reverb.damping_frequency.get_ratio();
        case ParamId::ERDG: return effects.reverb.damping_gain.get_ratio();
        case ParamId::ERWID: return effects.reverb.width.get_ratio();
        case ParamId::ERHPF: return effects.reverb.high_pass_frequency.get_ratio();
        case ParamId::ERCTH: return effects.reverb.side_chain_compression_threshold.get_ratio();
        case ParamId::ERCAT: return effects.reverb.side_chain_compression_attack_time.get_ratio();
        case ParamId::ERCRL: return effects.reverb.side_chain_compression_release_time.get_ratio();
        case ParamId::ERCR: return effects.reverb.side_chain_compression_ratio.get_ratio();
        case ParamId::ERWET: return effects.reverb.wet.get_ratio();
        case ParamId::ERDRY: return effects.reverb.dry.get_ratio();
        case ParamId::EV3V: return effects.volume_3_gain.get_ratio();
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
        case ParamId::POLY: return polyphonic.get_ratio();
        case ParamId::ERTYP: return effects.reverb.type.get_ratio();
        case ParamId::ECTYP: return effects.chorus.type.get_ratio();
        case ParamId::MTUN: return modulator_params.tuning.get_ratio();
        case ParamId::CTUN: return carrier_params.tuning.get_ratio();
        default: return 0.0; /* This should never be reached. */
    }
}


void Synth::clear_midi_controllers() noexcept
{
    pitch_wheel.clear();
    note.clear();
    velocity.clear();
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
    : type(INVALID),
    param_id(ParamId::MAX_PARAM_ID),
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
                modulators[v]->get_note(), modulators[v]->get_channel()
            );
            ++i;
        } else if (!carriers[v]->is_released() && carriers[v]->is_on()) {
            note_tunings[i] = NoteTuning(
                carriers[v]->get_note(), carriers[v]->get_channel()
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

    render_voices<Modulator>(active_modulators, active_modulators_count, modulator_params.tuning.get_value(), round, sample_count);
    render_voices<Carrier>(active_carriers, active_carriers_count, carrier_params.tuning.get_value(), round, sample_count);

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


template<class VoiceClass>
void Synth::Bus::render_voices(
        VoiceClass* (&voices)[POLYPHONY],
        size_t const voices_count,
        typename VoiceClass::Tuning const tuning,
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

        if (tuning == VoiceClass::TUNING_MTS_ESP_REALTIME) {
            for (size_t v = 0; v != voices_count; ++v) {
                voices[v]->update_note_frequency_for_realtime_mts_esp();
            }
        } else if (VoiceClass::is_tuning_unstable(tuning)) {
            if (VoiceClass::is_tuning_synced_unstable(tuning)) {
                for (size_t v = 0; v != voices_count; ++v) {
                    voices[v]->template update_unstable_note_frequency<true>(round);
                }
            } else {
                for (size_t v = 0; v != voices_count; ++v) {
                    voices[v]->template update_unstable_note_frequency<false>(round);
                }
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
