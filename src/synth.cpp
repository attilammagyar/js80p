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
#include <sstream>

#include "js80p.hpp"
#include "midi.hpp"

#include "synth.hpp"

#include "synth/biquad_filter.cpp"
#include "synth/delay.cpp"
#include "synth/distortion.cpp"
#include "synth/envelope.cpp"
#include "synth/filter.cpp"
#include "synth/flexible_controller.cpp"
#include "synth/math.cpp"
#include "synth/midi_controller.cpp"
#include "synth/oscillator.cpp"
#include "synth/param.cpp"
#include "synth/queue.cpp"
#include "synth/signal_producer.cpp"
#include "synth/voice.cpp"
#include "synth/wavefolder.cpp"
#include "synth/wavetable.cpp"


namespace JS80P
{

Synth::ParamIdHashTable Synth::param_id_hash_table;
std::string Synth::param_names_by_id[ParamId::MAX_PARAM_ID];


Synth::Synth()
    : SignalProducer(
        OUT_CHANNELS,
        5                           // VOL + ADD + FM + AM + bus
        + 29 * 2                    // Modulator::Params + Carrier::Params
        + POLYPHONY * 2             // modulators + carriers
        + 2                         // overdrive + distortion
        + 2                         // filter_1 + filter_1_type
        + 2                         // filter_2 + filter_2_type
        + FLEXIBLE_CONTROLLERS * 6
        + ENVELOPES * 10
    ),
    volume("VOL", 0.0, 1.0, 0.75),
    modulator_add_volume("ADD", 0.0, 1.0, 1.0),
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
        volume,
        modulator_add_volume
    ),
    overdrive("EO", 3.0, bus),
    distortion("ED", 10.0, overdrive),
    filter_1_type("EF1TYP"),
    filter_2_type("EF2TYP"),
    filter_1("EF1", distortion, filter_1_type),
    filter_2("EF2", filter_1, filter_2_type),
    next_voice(0),
    previous_note(Midi::NOTE_MAX + 1),
    midi_controllers((MidiController* const*)midi_controllers_rw),
    flexible_controllers((FlexibleController* const*)flexible_controllers_rw),
    envelopes((Envelope* const*)envelopes_rw)
{
    for (int i = 0; i != FLOAT_PARAMS; ++i) {
        float_params[i] = NULL;
    }

    for (int i = 0; i != ParamId::MAX_PARAM_ID; ++i) {
        param_ratios[i].store(0.0);
        controller_assignments[i].store(ControllerId::NONE);
        param_names_by_id[i] = "";
    }

    register_float_param_as_child(ParamId::VOL, volume);
    register_float_param_as_child(ParamId::ADD, modulator_add_volume);
    register_float_param_as_child(ParamId::FM, frequency_modulation_level);
    register_float_param_as_child(ParamId::AM, amplitude_modulation_level);
    register_child(bus);

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
    register_float_param_as_child(ParamId::MF1FRQ, modulator_params.filter_1_frequency);
    register_float_param_as_child(ParamId::MF1Q, modulator_params.filter_1_q);
    register_float_param_as_child(ParamId::MF1G, modulator_params.filter_1_gain);

    register_param_as_child<Modulator::Filter2::TypeParam>(
        ParamId::MF2TYP, modulator_params.filter_2_type
    );
    register_float_param_as_child(ParamId::MF2FRQ, modulator_params.filter_2_frequency);
    register_float_param_as_child(ParamId::MF2Q, modulator_params.filter_2_q);
    register_float_param_as_child(ParamId::MF2G, modulator_params.filter_2_gain);

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
    register_float_param_as_child(ParamId::CF1FRQ, carrier_params.filter_1_frequency);
    register_float_param_as_child(ParamId::CF1Q, carrier_params.filter_1_q);
    register_float_param_as_child(ParamId::CF1G, carrier_params.filter_1_gain);

    register_param_as_child<Carrier::Filter2::TypeParam>(
        ParamId::CF2TYP, carrier_params.filter_2_type
    );
    register_float_param_as_child(ParamId::CF2FRQ, carrier_params.filter_2_frequency);
    register_float_param_as_child(ParamId::CF2Q, carrier_params.filter_2_q);
    register_float_param_as_child(ParamId::CF2G, carrier_params.filter_2_gain);

    register_child(overdrive);
    register_float_param(ParamId::EOG, overdrive.level);

    register_child(distortion);
    register_float_param(ParamId::EDG, distortion.level);

    register_child(filter_1);
    register_param_as_child<Filter1::TypeParam>(EF1TYP, filter_1_type);
    register_float_param(ParamId::EF1FRQ, filter_1.frequency);
    register_float_param(ParamId::EF1Q, filter_1.q);
    register_float_param(ParamId::EF1G, filter_1.gain);

    register_child(filter_2);
    register_param_as_child<Filter2::TypeParam>(EF2TYP, filter_2_type);
    register_float_param(ParamId::EF2FRQ, filter_2.frequency);
    register_float_param(ParamId::EF2Q, filter_2.q);
    register_float_param(ParamId::EF2G, filter_2.gain);

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

    for (int i = 0; i != POLYPHONY; ++i) {
        modulators[i] = new Modulator(
            frequencies, Midi::NOTES, modulator_params
        );
        register_child(*modulators[i]);

        carriers[i] = new Carrier(
            frequencies,
            Midi::NOTES,
            carrier_params,
            &modulators[i]->modulation_out,
            amplitude_modulation_level,
            frequency_modulation_level
        );
        register_child(*carriers[i]);
    }

    for (Midi::Channel channel = 0; channel != Midi::CHANNELS; ++channel) {
        for (Midi::Note note = 0; note != Midi::NOTES; ++note) {
            midi_note_to_voice_assignments[channel][note] = -1;
        }
    }

    for (Integer i = 0; i != MIDI_CONTROLLERS; ++i) {
        midi_controllers_rw[i] = NULL;
    }

    midi_controllers_rw[ControllerId::MODULATION_WHEEL] = new MidiController();
    midi_controllers_rw[ControllerId::BREATH] = new MidiController();
    midi_controllers_rw[ControllerId::UNDEFINED_1] = new MidiController();
    midi_controllers_rw[ControllerId::FOOT_PEDAL] = new MidiController();
    midi_controllers_rw[ControllerId::PORTAMENTO_TIME] = new MidiController();
    midi_controllers_rw[ControllerId::VOLUME] = new MidiController();
    midi_controllers_rw[ControllerId::BALANCE] = new MidiController();
    midi_controllers_rw[ControllerId::UNDEFINED_2] = new MidiController();
    midi_controllers_rw[ControllerId::PAN] = new MidiController();
    midi_controllers_rw[ControllerId::EXPRESSION_PEDAL] = new MidiController();
    midi_controllers_rw[ControllerId::FX_CTL_1] = new MidiController();
    midi_controllers_rw[ControllerId::FX_CTL_2] = new MidiController();
    midi_controllers_rw[ControllerId::UNDEFINED_3] = new MidiController();
    midi_controllers_rw[ControllerId::UNDEFINED_4] = new MidiController();
    midi_controllers_rw[ControllerId::GENERAL_1] = new MidiController();
    midi_controllers_rw[ControllerId::GENERAL_2] = new MidiController();
    midi_controllers_rw[ControllerId::GENERAL_3] = new MidiController();
    midi_controllers_rw[ControllerId::GENERAL_4] = new MidiController();
    midi_controllers_rw[ControllerId::UNDEFINED_5] = new MidiController();
    midi_controllers_rw[ControllerId::UNDEFINED_6] = new MidiController();
    midi_controllers_rw[ControllerId::UNDEFINED_7] = new MidiController();
    midi_controllers_rw[ControllerId::UNDEFINED_8] = new MidiController();
    midi_controllers_rw[ControllerId::UNDEFINED_9] = new MidiController();
    midi_controllers_rw[ControllerId::UNDEFINED_10] = new MidiController();
    midi_controllers_rw[ControllerId::UNDEFINED_11] = new MidiController();
    midi_controllers_rw[ControllerId::UNDEFINED_12] = new MidiController();
    midi_controllers_rw[ControllerId::UNDEFINED_13] = new MidiController();
    midi_controllers_rw[ControllerId::UNDEFINED_14] = new MidiController();
    midi_controllers_rw[ControllerId::UNDEFINED_15] = new MidiController();
    midi_controllers_rw[ControllerId::UNDEFINED_16] = new MidiController();
    midi_controllers_rw[ControllerId::PORTAMENTO_AMOUNT] = new MidiController();
    midi_controllers_rw[ControllerId::SOUND_1] = new MidiController();
    midi_controllers_rw[ControllerId::SOUND_2] = new MidiController();
    midi_controllers_rw[ControllerId::SOUND_3] = new MidiController();
    midi_controllers_rw[ControllerId::SOUND_4] = new MidiController();
    midi_controllers_rw[ControllerId::SOUND_5] = new MidiController();
    midi_controllers_rw[ControllerId::SOUND_6] = new MidiController();
    midi_controllers_rw[ControllerId::SOUND_7] = new MidiController();
    midi_controllers_rw[ControllerId::SOUND_8] = new MidiController();
    midi_controllers_rw[ControllerId::SOUND_9] = new MidiController();
    midi_controllers_rw[ControllerId::SOUND_10] = new MidiController();
    midi_controllers_rw[ControllerId::UNDEFINED_17] = new MidiController();
    midi_controllers_rw[ControllerId::UNDEFINED_18] = new MidiController();
    midi_controllers_rw[ControllerId::UNDEFINED_19] = new MidiController();
    midi_controllers_rw[ControllerId::UNDEFINED_20] = new MidiController();
    midi_controllers_rw[ControllerId::UNDEFINED_21] = new MidiController();
    midi_controllers_rw[ControllerId::FX_1] = new MidiController();
    midi_controllers_rw[ControllerId::FX_2] = new MidiController();
    midi_controllers_rw[ControllerId::FX_3] = new MidiController();
    midi_controllers_rw[ControllerId::FX_4] = new MidiController();
    midi_controllers_rw[ControllerId::FX_5] = new MidiController();
    midi_controllers_rw[ControllerId::UNDEFINED_22] = new MidiController();
    midi_controllers_rw[ControllerId::UNDEFINED_23] = new MidiController();
    midi_controllers_rw[ControllerId::UNDEFINED_24] = new MidiController();
    midi_controllers_rw[ControllerId::UNDEFINED_25] = new MidiController();
    midi_controllers_rw[ControllerId::UNDEFINED_26] = new MidiController();
    midi_controllers_rw[ControllerId::UNDEFINED_27] = new MidiController();
    midi_controllers_rw[ControllerId::UNDEFINED_28] = new MidiController();
    midi_controllers_rw[ControllerId::UNDEFINED_29] = new MidiController();
    midi_controllers_rw[ControllerId::UNDEFINED_30] = new MidiController();
    midi_controllers_rw[ControllerId::UNDEFINED_31] = new MidiController();
    midi_controllers_rw[ControllerId::UNDEFINED_32] = new MidiController();
    midi_controllers_rw[ControllerId::UNDEFINED_33] = new MidiController();
    midi_controllers_rw[ControllerId::UNDEFINED_34] = new MidiController();
    midi_controllers_rw[ControllerId::UNDEFINED_35] = new MidiController();
    midi_controllers_rw[ControllerId::UNDEFINED_36] = new MidiController();
    midi_controllers_rw[ControllerId::UNDEFINED_37] = new MidiController();
    midi_controllers_rw[ControllerId::UNDEFINED_38] = new MidiController();
    midi_controllers_rw[ControllerId::UNDEFINED_39] = new MidiController();

    Integer next_id = F1IN;

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

    update_param_states();

    // TODO: remove all these temporary settings

    // modulator_params.waveform.set_value(Modulator::Oscillator_::SINE);
    // carrier_params.waveform.set_value(Carrier::Oscillator_::SINE);
    // modulator_params.amplitude.set_value(1.0);
    // carrier_params.amplitude.set_value(0.0);
    // modulator_params.portamento_length.set_value(3.0);
    // modulator_params.portamento_depth.set_value(-2400.0);
    // modulator_params.detune.set_value(2400.0);
    // // modulator_params.folding.set_value(modulator_params.folding.get_max_value());

    // controller_assignments[ParamId::MFLD].store(ControllerId::ENVELOPE_1);
    // modulator_params.folding.set_envelope(envelopes_rw[0]);

    // envelopes_rw[0]->amount.set_value(1.0);
    // envelopes_rw[0]->initial_value.set_value(0.0);
    // envelopes_rw[0]->delay_time.set_value(0.5);
    // envelopes_rw[0]->attack_time.set_value(3.0);
    // envelopes_rw[0]->peak_value.set_value(1.0);
    // envelopes_rw[0]->hold_time.set_value(1.0);
    // envelopes_rw[0]->decay_time.set_value(3.0);
    // envelopes_rw[0]->sustain_value.set_value(0.0);
    // envelopes_rw[0]->release_time.set_value(0.0);
    // envelopes_rw[0]->final_value.set_value(0.0);



    controller_assignments[ParamId::MAMP].store(ControllerId::ENVELOPE_1);
    controller_assignments[ParamId::CAMP].store(ControllerId::ENVELOPE_1);
    controller_assignments[ParamId::MFIN].store(ControllerId::PITCH_WHEEL);
    controller_assignments[ParamId::CFIN].store(ControllerId::PITCH_WHEEL);
    controller_assignments[ParamId::AM].store(ControllerId::ENVELOPE_2);
    controller_assignments[ParamId::MF1FRQ].store(ControllerId::ENVELOPE_3);
    controller_assignments[ParamId::CF1FRQ].store(ControllerId::ENVELOPE_3);

    modulator_params.amplitude.set_envelope(envelopes_rw[0]);
    carrier_params.amplitude.set_envelope(envelopes_rw[0]);

    modulator_params.fine_detune.set_midi_controller(&pitch_wheel);
    carrier_params.fine_detune.set_midi_controller(&pitch_wheel);

    amplitude_modulation_level.set_envelope(envelopes_rw[1]);
    frequency_modulation_level.set_value(5000.0);
    modulator_params.waveform.set_value(Modulator::Oscillator_::SINE);
    modulator_params.detune.set_value(-1200.0);
    carrier_params.waveform.set_value(Carrier::Oscillator_::SAWTOOTH);

    modulator_params.filter_1_type.set_value(Modulator::Filter1::LOW_PASS);
    modulator_params.filter_1_frequency.set_envelope(envelopes_rw[2]);
    modulator_params.filter_1_q.set_value(0.7);

    modulator_params.filter_2_type.set_value(Modulator::Filter2::LOW_PASS);
    modulator_params.filter_2_frequency.set_value(12000.0);
    modulator_params.filter_2_q.set_value(0.7);

    carrier_params.filter_1_type.set_value(Carrier::Filter1::LOW_PASS);
    carrier_params.filter_1_frequency.set_envelope(envelopes_rw[2]);
    carrier_params.filter_1_q.set_value(0.7);

    carrier_params.filter_2_type.set_value(Carrier::Filter2::LOW_PASS);
    carrier_params.filter_2_frequency.set_value(12000.0);
    carrier_params.filter_2_q.set_value(0.7);

    envelopes_rw[0]->amount.set_value(1.0);
    envelopes_rw[0]->initial_value.set_value(0.0);
    envelopes_rw[0]->delay_time.set_value(0.0);
    envelopes_rw[0]->attack_time.set_value(0.1);
    envelopes_rw[0]->peak_value.set_value(1.0);
    envelopes_rw[0]->hold_time.set_value(0.3);
    envelopes_rw[0]->decay_time.set_value(0.5);
    envelopes_rw[0]->sustain_value.set_value(0.5);
    envelopes_rw[0]->release_time.set_value(1.0);
    envelopes_rw[0]->final_value.set_value(0.0);

    envelopes_rw[1]->amount.set_value(1.0);
    envelopes_rw[1]->initial_value.set_value(0.05);
    envelopes_rw[1]->delay_time.set_value(0.0);
    envelopes_rw[1]->attack_time.set_value(0.5);
    envelopes_rw[1]->peak_value.set_value(1.0);
    envelopes_rw[1]->hold_time.set_value(0.2);
    envelopes_rw[1]->decay_time.set_value(1.5);
    envelopes_rw[1]->sustain_value.set_value(0.5);
    envelopes_rw[1]->release_time.set_value(0.75);
    envelopes_rw[1]->final_value.set_value(0.025);

    envelopes_rw[2]->amount.set_value(1.0);
    envelopes_rw[2]->initial_value.set_value(0.05);
    envelopes_rw[2]->delay_time.set_value(0.0);
    envelopes_rw[2]->attack_time.set_value(0.5);
    envelopes_rw[2]->peak_value.set_value(0.3);
    envelopes_rw[2]->hold_time.set_value(0.2);
    envelopes_rw[2]->decay_time.set_value(1.5);
    envelopes_rw[2]->sustain_value.set_value(0.15);
    envelopes_rw[2]->release_time.set_value(0.75);
    envelopes_rw[2]->final_value.set_value(0.025);

    update_param_states();
}


template<class ParamClass>
void Synth::register_param_as_child(ParamId const param_id, ParamClass& param)
{
    register_child(param);
    register_param<ParamClass>(param_id, param);
}


template<class ParamClass>
void Synth::register_param(ParamId const param_id, ParamClass& param)
{
    std::string const name = param.get_name();

    param_id_hash_table.add(name.c_str(), param_id);
    param_names_by_id[param_id] = name;
}


void Synth::register_float_param_as_child(
        ParamId const param_id,
        FloatParam& float_param
) {
    register_param_as_child<FloatParam>(param_id, float_param);
    float_params[param_id] = &float_param;
}


void Synth::register_float_param(ParamId const param_id, FloatParam& float_param)
{
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


bool Synth::is_lock_free() const
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


void Synth::suspend()
{
}


void Synth::resume()
{
}


void Synth::note_on(
        Seconds const time_offset,
        Midi::Channel const channel,
        Midi::Note const note,
        Number const velocity
) {
    this->note.change(time_offset, (Number)note * NOTE_TO_PARAM_SCALE);
    this->velocity.change(time_offset, velocity);

    if (midi_note_to_voice_assignments[channel][note] != -1) {
        return;
    }

    for (Integer v = 0; v != POLYPHONY; ++v) {
        if (
                modulators[next_voice]->is_off_after(time_offset)
                && carriers[next_voice]->is_off_after(time_offset)
        ) {
            if (UNLIKELY(previous_note > Midi::NOTE_MAX)) {
                previous_note = note;
            }

            midi_note_to_voice_assignments[channel][note] = next_voice;
            modulators[next_voice]->note_on(
                time_offset, note, velocity, previous_note
            );
            carriers[next_voice]->note_on(
                time_offset, note, velocity, previous_note
            );

            previous_note = note;

            break;
        } else {
            next_voice = (next_voice + 1) & NEXT_VOICE_MASK;
        }
    }
}


void Synth::aftertouch(
        Seconds const time_offset,
        Midi::Channel const channel,
        Midi::Note const note,
        Number const pressure
) {
    this->note.change(time_offset, (Number)note * NOTE_TO_PARAM_SCALE);

    if (midi_note_to_voice_assignments[channel][note] == -1) {
        return;
    }

    // Integer const voice = midi_note_to_voice_assignments[channel][note];
}


void Synth::note_off(
        Seconds const time_offset,
        Midi::Channel const channel,
        Midi::Note const note,
        Number const velocity
) {
    if (midi_note_to_voice_assignments[channel][note] == -1) {
        return;
    }

    Integer const voice = midi_note_to_voice_assignments[channel][note];

    midi_note_to_voice_assignments[channel][note] = -1;

    modulators[voice]->note_off(time_offset, note, velocity);
    carriers[voice]->note_off(time_offset, note, velocity);
}


void Synth::control_change(
        Seconds const time_offset,
        Midi::Channel const channel,
        Midi::Controller const controller,
        Number const new_value
) {
    if (controller > MIDI_CONTROLLERS) {
        return;
    }

    MidiController* const midi_controller = midi_controllers_rw[controller];

    if (midi_controller == NULL) {
        return;
    }

    midi_controller->change(time_offset, new_value);
}


void Synth::pitch_wheel_change(
        Seconds const time_offset,
        Midi::Channel const channel,
        Number const new_value
) {
    pitch_wheel.change(time_offset, new_value);
}


void Synth::all_sound_off(
        Seconds const time_offset,
        Midi::Channel const channel
) {
}


void Synth::reset_all_controllers(
        Seconds const time_offset,
        Midi::Channel const channel
) {
}


void Synth::all_notes_off(
        Seconds const time_offset,
        Midi::Channel const channel
) {
}


Sample const* const* Synth::generate_samples(
        Integer const round,
        Integer const sample_count
) {
    return SignalProducer::produce<Synth>(this, round, sample_count);
}


void Synth::push_message(
    MessageType const type,
    ParamId const param_id,
    Number const number_param,
    Byte const byte_param
) {
    Message message(type, param_id, number_param, byte_param);

    messages.push(message);
}


std::string Synth::get_param_name(ParamId const param_id) const
{
    return param_names_by_id[param_id];
}


Synth::ParamId Synth::get_param_id(std::string const name) const
{
    return param_id_hash_table.lookup(name.c_str());
}


void Synth::get_param_id_hash_table_statistics(
        Integer& max_collisions,
        Number& avg_collisions,
        Number& avg_bucket_size
) const {
    param_id_hash_table.get_statistics(
        max_collisions, avg_collisions, avg_bucket_size
    );
}


Number Synth::get_param_ratio_atomic(ParamId const param_id) const
{
    return param_ratios[param_id].load();
}


Number Synth::get_param_default_ratio(ParamId const param_id) const
{
    if (param_id < FLOAT_PARAMS) {
        return float_params[param_id]->get_default_ratio();
    }

    switch (param_id) {
        case ParamId::MODE: return 0.0; // TODO
        case ParamId::MWAV: return modulator_params.waveform.get_default_ratio();
        case ParamId::CWAV: return carrier_params.waveform.get_default_ratio();
        case ParamId::MF1TYP: return modulator_params.filter_1_type.get_default_ratio();
        case ParamId::MF2TYP: return modulator_params.filter_2_type.get_default_ratio();
        case ParamId::CF1TYP: return carrier_params.filter_1_type.get_default_ratio();
        case ParamId::CF2TYP: return carrier_params.filter_2_type.get_default_ratio();
        case ParamId::EF1TYP: return filter_1_type.get_default_ratio();
        case ParamId::EF2TYP: return filter_2_type.get_default_ratio();
        case ParamId::L1WAV: return 0.0; // TODO
        case ParamId::L2WAV: return 0.0; // TODO
        case ParamId::L3WAV: return 0.0; // TODO
        case ParamId::L4WAV: return 0.0; // TODO
        case ParamId::L5WAV: return 0.0; // TODO
        case ParamId::L6WAV: return 0.0; // TODO
        case ParamId::L7WAV: return 0.0; // TODO
        case ParamId::L8WAV: return 0.0; // TODO
        default: return 0.0; // This should never be reached.
    }
}


Number Synth::float_param_ratio_to_display_value(
        ParamId const param_id,
        Number const ratio
) const {
    if (param_id < FLOAT_PARAMS) {
        return float_params[param_id]->ratio_to_value(ratio);
    }

    return 0.0;
}


Byte Synth::int_param_ratio_to_display_value(
        ParamId const param_id,
        Number const ratio
) const {
    switch (param_id) {
        case ParamId::MODE: return 0; // TODO
        case ParamId::MWAV: return modulator_params.waveform.ratio_to_value(ratio);
        case ParamId::CWAV: return carrier_params.waveform.ratio_to_value(ratio);
        case ParamId::MF1TYP: return modulator_params.filter_1_type.ratio_to_value(ratio);
        case ParamId::MF2TYP: return modulator_params.filter_2_type.ratio_to_value(ratio);
        case ParamId::CF1TYP: return carrier_params.filter_1_type.ratio_to_value(ratio);
        case ParamId::CF2TYP: return carrier_params.filter_2_type.ratio_to_value(ratio);
        case ParamId::EF1TYP: return filter_1_type.ratio_to_value(ratio);
        case ParamId::EF2TYP: return filter_2_type.ratio_to_value(ratio);
        case ParamId::L1WAV: return 0; // TODO
        case ParamId::L2WAV: return 0; // TODO
        case ParamId::L3WAV: return 0; // TODO
        case ParamId::L4WAV: return 0; // TODO
        case ParamId::L5WAV: return 0; // TODO
        case ParamId::L6WAV: return 0; // TODO
        case ParamId::L7WAV: return 0; // TODO
        case ParamId::L8WAV: return 0; // TODO
        default: return 0; // This should never be reached.
    }
}


Synth::ControllerId Synth::get_param_controller_id_atomic(ParamId const param_id) const
{
    return (Synth::ControllerId)controller_assignments[param_id].load();
}


void Synth::update_param_states()
{
    for (int i = 0; i != ParamId::MAX_PARAM_ID; ++i) {
        handle_refresh_param((ParamId)i);
    }
}


Sample const* const* Synth::initialize_rendering(
        Integer const round,
        Integer const sample_count
) {
    process_messages();

    Sample const* const* const buffer = SignalProducer::produce<Filter2>(
        &filter_2, round, sample_count
    );

    clear_midi_controllers();

    return buffer;
}


void Synth::process_messages()
{
    size_t const message_count = messages.size();

    for (size_t i = 0; i != message_count; ++i) {
        Message message;

        if (!messages.pop(message)) {
            continue;
        }

        switch (message.type) {
            case SET_PARAM:
                handle_set_param(message.param_id, message.number_param);
                break;

            case ASSIGN_CONTROLLER:
                handle_assign_controller(message.param_id, message.byte_param);
                break;

            case REFRESH_PARAM:
                handle_refresh_param(message.param_id);

            default:
                break;
        }
    }
}


void Synth::handle_set_param(ParamId const param_id, Number const ratio)
{
    if (param_id < FLOAT_PARAMS) {
        // TODO: remove the null check when all params are implemented
        if (float_params[param_id] != NULL) {
            float_params[param_id]->set_ratio(ratio);
        }

    } else {
        switch (param_id) {
            case ParamId::MODE: // TODO
                break;

            case ParamId::MWAV:
                modulator_params.waveform.set_ratio(ratio);
                break;

            case ParamId::CWAV:
                carrier_params.waveform.set_ratio(ratio);
                break;

            case ParamId::MF1TYP:
                modulator_params.filter_1_type.set_ratio(ratio);
                break;

            case ParamId::MF2TYP:
                modulator_params.filter_2_type.set_ratio(ratio);
                break;

            case ParamId::CF1TYP:
                carrier_params.filter_1_type.set_ratio(ratio);
                break;

            case ParamId::CF2TYP:
                carrier_params.filter_2_type.set_ratio(ratio);
                break;

            case ParamId::EF1TYP:
                filter_1_type.set_ratio(ratio);
                break;

            case ParamId::EF2TYP:
                filter_2_type.set_ratio(ratio);
                break;

            case ParamId::L1WAV: // TODO
                break;

            case ParamId::L2WAV: // TODO
                break;

            case ParamId::L3WAV: // TODO
                break;

            case ParamId::L4WAV: // TODO
                break;

            case ParamId::L5WAV: // TODO
                break;

            case ParamId::L6WAV: // TODO
                break;

            case ParamId::L7WAV: // TODO
                break;

            case ParamId::L8WAV: // TODO
                break;

            default:
                // This should never be reached.
                break;
        }
    }

    handle_refresh_param(param_id);
}


void Synth::handle_assign_controller(
        ParamId const param_id,
        Byte const controller_id
) {
    controller_assignments[param_id].store(controller_id);

    if (param_id < FLOAT_PARAMS) {
        assign_controller_to_float_param(param_id, (ControllerId)controller_id);
    } else {
        assign_controller_to_param(param_id, (ControllerId)controller_id);
    }
}


void Synth::handle_refresh_param(ParamId const param_id)
{
    param_ratios[param_id].store(get_param_ratio(param_id));
}


void Synth::assign_controller_to_param(
        ParamId const param_id,
        ControllerId const controller_id
) {
    MidiController* midi_controller = NULL;

    switch (controller_id) {
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

        default: {
            if (controller_id < MIDI_CONTROLLERS) {
                midi_controller = midi_controllers[controller_id];
            }

            break;
        }
    }

    switch (param_id) {
        case ParamId::MODE: // TODO
            break;

        case ParamId::MWAV:
            modulator_params.waveform.set_midi_controller(midi_controller);
            break;

        case ParamId::CWAV:
            carrier_params.waveform.set_midi_controller(midi_controller);
            break;

        case ParamId::MF1TYP:
            modulator_params.filter_1_type.set_midi_controller(midi_controller);
            break;

        case ParamId::MF2TYP:
            modulator_params.filter_2_type.set_midi_controller(midi_controller);
            break;

        case ParamId::CF1TYP:
            carrier_params.filter_1_type.set_midi_controller(midi_controller);
            break;

        case ParamId::CF2TYP:
            carrier_params.filter_2_type.set_midi_controller(midi_controller);
            break;

        case ParamId::EF1TYP:
            filter_1_type.set_midi_controller(midi_controller);
            break;

        case ParamId::EF2TYP:
            filter_2_type.set_midi_controller(midi_controller);
            break;

        case ParamId::L1WAV: break; // TODO
        case ParamId::L2WAV: break; // TODO
        case ParamId::L3WAV: break; // TODO
        case ParamId::L4WAV: break; // TODO
        case ParamId::L5WAV: break; // TODO
        case ParamId::L6WAV: break; // TODO
        case ParamId::L7WAV: break; // TODO
        case ParamId::L8WAV: break; // TODO

        default: break; // This should never be reached.
    }
}


void Synth::assign_controller_to_float_param(
        ParamId const param_id,
        ControllerId const controller_id
) {
    FloatParam* param = float_params[param_id];

    // TODO: remove the null check when all params are implemented
    if (param == NULL) {
        return;
    }

    param->set_midi_controller(NULL);
    param->set_flexible_controller(NULL);
    param->set_envelope(NULL);

    switch (controller_id) {
        case PITCH_WHEEL:
            param->set_midi_controller(&pitch_wheel);
            break;

        case NOTE:
            param->set_midi_controller(&note);
            break;

        case VELOCITY:
            param->set_midi_controller(&velocity);
            break;

        case FLEXIBLE_CONTROLLER_1:
            param->set_flexible_controller(flexible_controllers[0]);
            break;

        case FLEXIBLE_CONTROLLER_2:
            param->set_flexible_controller(flexible_controllers[1]);
            break;

        case FLEXIBLE_CONTROLLER_3:
            param->set_flexible_controller(flexible_controllers[2]);
            break;

        case FLEXIBLE_CONTROLLER_4:
            param->set_flexible_controller(flexible_controllers[3]);
            break;

        case FLEXIBLE_CONTROLLER_5:
            param->set_flexible_controller(flexible_controllers[4]);
            break;

        case FLEXIBLE_CONTROLLER_6:
            param->set_flexible_controller(flexible_controllers[5]);
            break;

        case FLEXIBLE_CONTROLLER_7:
            param->set_flexible_controller(flexible_controllers[6]);
            break;

        case FLEXIBLE_CONTROLLER_8:
            param->set_flexible_controller(flexible_controllers[7]);
            break;

        case FLEXIBLE_CONTROLLER_9:
            param->set_flexible_controller(flexible_controllers[8]);
            break;

        case FLEXIBLE_CONTROLLER_10:
            param->set_flexible_controller(flexible_controllers[9]);
            break;

        case LFO_1: break;
        case LFO_2: break;
        case LFO_3: break;
        case LFO_4: break;
        case LFO_5: break;
        case LFO_6: break;
        case LFO_7: break;
        case LFO_8: break;

        case ENVELOPE_1: param->set_envelope(envelopes[0]); break;
        case ENVELOPE_2: param->set_envelope(envelopes[1]); break;
        case ENVELOPE_3: param->set_envelope(envelopes[2]); break;
        case ENVELOPE_4: param->set_envelope(envelopes[3]); break;
        case ENVELOPE_5: param->set_envelope(envelopes[4]); break;
        case ENVELOPE_6: param->set_envelope(envelopes[5]); break;

        default: {
            if (controller_id < MIDI_CONTROLLERS) {
                param->set_midi_controller(midi_controllers[controller_id]);
            }

            break;
        }
    }
}


Number Synth::get_param_ratio(ParamId const param_id) const
{
    if (param_id < FLOAT_PARAMS) {
        // TODO: remove the null check when all params are implemented
        if (float_params[param_id] != NULL) {
            return float_params[param_id]->get_ratio();
        }
    }

    switch (param_id) {
        case ParamId::MODE: return 0.0; // TODO
        case ParamId::MWAV: return modulator_params.waveform.get_ratio();
        case ParamId::CWAV: return carrier_params.waveform.get_ratio();
        case ParamId::MF1TYP: return modulator_params.filter_1_type.get_ratio();
        case ParamId::MF2TYP: return modulator_params.filter_2_type.get_ratio();
        case ParamId::CF1TYP: return carrier_params.filter_1_type.get_ratio();
        case ParamId::CF2TYP: return carrier_params.filter_2_type.get_ratio();
        case ParamId::EF1TYP: return filter_1_type.get_ratio();
        case ParamId::EF2TYP: return filter_2_type.get_ratio();
        case ParamId::L1WAV: return 0.0; // TODO
        case ParamId::L2WAV: return 0.0; // TODO
        case ParamId::L3WAV: return 0.0; // TODO
        case ParamId::L4WAV: return 0.0; // TODO
        case ParamId::L5WAV: return 0.0; // TODO
        case ParamId::L6WAV: return 0.0; // TODO
        case ParamId::L7WAV: return 0.0; // TODO
        case ParamId::L8WAV: return 0.0; // TODO
        default: return 0.0; // This should never be reached.
    }
}


void Synth::clear_midi_controllers()
{
    pitch_wheel.clear();
    note.clear();
    velocity.clear();

    for (Integer i = 0; i != MIDI_CONTROLLERS; ++i) {
        if (midi_controllers[i] != NULL) {
            midi_controllers[i]->clear();
        }
    }
}


void Synth::render(
        Integer const round,
        Integer const first_sample_index,
        Integer const last_sample_index,
        Sample** buffer
) {
}


std::string const Synth::to_string(Integer const n) const
{
    std::ostringstream s;

    s << n;

    return s.str();
}


Synth::Message::Message()
    : type(INVALID),
    param_id(ParamId::MAX_PARAM_ID),
    number_param(0.0),
    byte_param(0)
{
}


Synth::Message::Message(Message const& message)
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
) : type(type),
    param_id(param_id),
    number_param(number_param),
    byte_param(byte_param)
{
}


Synth::Message& Synth::Message::operator=(Message const& message)
{
    if (this != &message) {
        type = message.type;
        param_id = message.param_id;
        number_param = message.number_param;
        byte_param = message.byte_param;
    }

    return *this;
}


Synth::SingleProducerSingleConsumerMessageQueue::SingleProducerSingleConsumerMessageQueue()
    : next_push(0),
    next_pop(0)
{
}


bool Synth::SingleProducerSingleConsumerMessageQueue::is_lock_free() const
{
    return next_push.is_lock_free() && next_pop.is_lock_free();
}


bool Synth::SingleProducerSingleConsumerMessageQueue::push(
        Synth::Message const& message
) {
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


bool Synth::SingleProducerSingleConsumerMessageQueue::pop(Message& message)
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


size_t Synth::SingleProducerSingleConsumerMessageQueue::size() const
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
) const {
    return (index + 1) & SIZE_MASK;
}


Synth::Bus::Bus(
        Integer const channels,
        Modulator* const* const modulators,
        Carrier* const* const carriers,
        Integer const polyphony,
        FloatParam& volume,
        FloatParam& modulator_add_volume
) : SignalProducer(channels, 0),
    polyphony(polyphony),
    modulators(modulators),
    carriers(carriers),
    volume(volume),
    modulator_add_volume(modulator_add_volume),
    modulators_on(POLYPHONY),
    carriers_on(POLYPHONY)
{
}


Sample const* const* Synth::Bus::initialize_rendering(
        Integer const round,
        Integer const sample_count
) {
    is_silent = true;

    for (Integer v = 0; v != polyphony; ++v) {
        modulators_on[v] = modulators[v]->is_on();

        if (modulators_on[v]) {
            is_silent = false;
            SignalProducer::produce<Modulator>(
                modulators[v], round, sample_count
            );
        }

        carriers_on[v] = carriers[v]->is_on();

        if (carriers_on[v]) {
            is_silent = false;
            SignalProducer::produce<Carrier>(carriers[v], round, sample_count);
        }
    }

    if (is_silent) {
        return NULL;
    }

    volume_buffer = (
        FloatParam::produce_if_not_constant(&volume, round, sample_count)
    );

    modulator_add_volume_buffer = FloatParam::produce_if_not_constant(
        &modulator_add_volume, round, sample_count
    );

    return NULL;
}


void Synth::Bus::render(
        Integer const round,
        Integer const first_sample_index,
        Integer const last_sample_index,
        Sample** buffer
) {
    render_silence(round, first_sample_index, last_sample_index, buffer);

    if (is_silent) {
        return;
    }

    mix_modulators(round, first_sample_index, last_sample_index, buffer);
    mix_carriers(round, first_sample_index, last_sample_index, buffer);
    apply_volume(round, first_sample_index, last_sample_index, buffer);
}


void Synth::Bus::mix_modulators(
        Integer const round,
        Integer const first_sample_index,
        Integer const last_sample_index,
        Sample** buffer
) const {
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
                SignalProducer::produce<Modulator>(modulators[v], round)
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
                SignalProducer::produce<Modulator>(modulators[v], round)
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
) const {
    for (Integer v = 0; v != polyphony; ++v) {
        if (!carriers_on[v]) {
            continue;
        }

        Sample const* const* const carrier_output = (
            SignalProducer::produce<Carrier>(carriers[v], round)
        );

        for (Integer c = 0; c != channels; ++c) {
            for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                buffer[c][i] += carrier_output[c][i];
            }
        }
    }
}


void Synth::Bus::apply_volume(
        Integer const round,
        Integer const first_sample_index,
        Integer const last_sample_index,
        Sample** buffer
) const {
    Sample const* const volume_buffer = this->volume_buffer;

    if (LIKELY(volume_buffer == NULL)) {
        Sample const volume_value = (Sample)volume.get_value();

        for (Integer c = 0; c != channels; ++c) {
            for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                buffer[c][i] *= volume_value;
            }
        }

    } else {
        for (Integer c = 0; c != channels; ++c) {
            for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                buffer[c][i] *= volume_buffer[i];
            }
        }
    }
}


Synth::ParamIdHashTable::ParamIdHashTable()
{
}


Synth::ParamIdHashTable::~ParamIdHashTable()
{
}


void Synth::ParamIdHashTable::add(char const* name, ParamId const param_id)
{
    Entry* root;
    Entry* parent;
    Entry* entry;

    lookup(name, &root, &parent, &entry);

    if (entry != NULL) {
        return;
    }

    if (parent != NULL) {
        parent->next = new Entry(name, param_id);

        return;
    }

    root->set(name, param_id);
}


void Synth::ParamIdHashTable::lookup(
        char const* name,
        Entry** root,
        Entry** parent,
        Entry** entry
) {
    Integer const hash = this->hash(name);
    *root = &entries[hash];

    *parent = NULL;

    if ((*root)->param_id == MAX_PARAM_ID) {
        *entry = NULL;

        return;
    }

    *entry = *root;

    while (strncmp((*entry)->name, name, Entry::NAME_SIZE) != 0) {
        *parent = *entry;
        *entry = (*entry)->next;

        if (*entry == NULL) {
            break;
        }
    }
}


Synth::ParamId Synth::ParamIdHashTable::lookup(char const* name)
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
) const {
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
Integer Synth::ParamIdHashTable::hash(char const* name)
{
    /*
    We only care about the 36 characters which are used in param names: capital
    letters and numbers.
    */
    constexpr Integer alphabet_size = 36;
    constexpr char letter_offset = 'A' - 10;
    constexpr char number_offset = '0';

    if (*name == '\x00') {
        return 0;
    }

    Integer i;
    Integer hash = 0;

    for (i = -1; *name != '\x00'; ++name) {
        char c = *name;

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


Synth::ParamIdHashTable::Entry::Entry() : next(NULL)
{
    set("", MAX_PARAM_ID);
}


Synth::ParamIdHashTable::Entry::Entry(const char* name, ParamId const param_id)
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
) {
    std::fill_n(this->name, NAME_SIZE, '\x00');
    strncpy(this->name, name, NAME_MAX_INDEX);
    this->param_id = param_id;
}

}
