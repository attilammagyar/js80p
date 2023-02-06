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

#ifndef JS80P__SYNTH_HPP
#define JS80P__SYNTH_HPP

#include <atomic>
#include <string>
#include <vector>

#include "js80p.hpp"
#include "midi.hpp"

#include "synth/envelope.hpp"
#include "synth/biquad_filter.hpp"
#include "synth/filter.hpp"
#include "synth/math.hpp"
#include "synth/midi_controller.hpp"
#include "synth/oscillator.hpp"
#include "synth/param.hpp"
#include "synth/queue.hpp"
#include "synth/signal_producer.hpp"
#include "synth/voice.hpp"


namespace JS80P
{

class Synth : public Midi::EventHandler, public SignalProducer
{
    public:
        typedef Voice<SignalProducer> Modulator;
        typedef Voice<Modulator::ModulationOut> Carrier;

        static constexpr Integer OUT_CHANNELS = Carrier::CHANNELS;

        static constexpr Integer ENVELOPES = 6;
        static constexpr Integer MIDI_CONTROLLERS = 120;
        static constexpr Integer SHAPED_CONTROLLERS = 8;
        static constexpr Integer LFOS = 8;

        enum MessageType {
            SET_PARAM = 1,
            ASSIGN_CONTROLLER = 2,
            REFRESH_PARAM = 3,
            INVALID,
        };

        enum ParamId {
            MODE = 0,       // Mode
            MWAV = 1,       // Modulator Waveform
            CWAV = 2,       // Carrier Waveform
            MF1TYP = 3,     // Modulator Filter 1 Type
            MF2TYP = 4,     // Modulator Filter 2 Type
            CF1TYP = 5,     // Carrier Filter 1 Type
            CF2TYP = 6,     // Carrier Filter 2 Type

            EF1TYP = 7,     // Effects Filter 1 Type
            EF2TYP = 8,     // Effects Filter 2 Type

            L1WAV = 9,      // LFO 1 waveform
            L2WAV = 10,     // LFO 2 waveform
            L3WAV = 11,     // LFO 3 waveform
            L4WAV = 12,     // LFO 4 waveform
            L5WAV = 13,     // LFO 5 waveform
            L6WAV = 14,     // LFO 6 waveform
            L7WAV = 15,     // LFO 7 waveform
            L8WAV = 16,     // LFO 8 waveform

            VOL = 17,       // Volume

            ADD = 18,       // Modulator Additive Volume
            FM = 19,        // Frequency Modulation
            AM = 20,        // Amplitude Modulation

            MAMP = 21,      // Modulator Amplitude
            MVS = 22,       // Modulator Velocity Sensitivity
            MFLD = 23,      // Modulator Folding
            MPRT = 24,      // Modulator Portamento Length
            MPRD = 25,      // Modulator Portamento Depth
            MDTN = 26,      // Modulator Detune
            MFIN = 27,      // Modulator Fine Detune
            MWID = 28,      // Modulator Width
            MPAN = 29,      // Modulator Pan
            MVOL = 30,      // Modulator Volume

            MC1 = 31,       // Modulator Custom Waveform 1st Harmonic
            MC2 = 32,       // Modulator Custom Waveform 2nd Harmonic
            MC3 = 33,       // Modulator Custom Waveform 3rd Harmonic
            MC4 = 34,       // Modulator Custom Waveform 4th Harmonic
            MC5 = 35,       // Modulator Custom Waveform 5th Harmonic
            MC6 = 36,       // Modulator Custom Waveform 6th Harmonic
            MC7 = 37,       // Modulator Custom Waveform 7th Harmonic
            MC8 = 38,       // Modulator Custom Waveform 8th Harmonic
            MC9 = 39,       // Modulator Custom Waveform 9th Harmonic
            MC10 = 40,      // Modulator Custom Waveform 10th Harmonic

            MF1FRQ = 41,    // Modulator Filter 1 Frequency
            MF1Q = 42,      // Modulator Filter 1 Q Factor
            MF1G = 43,      // Modulator Filter 1 Gain

            MF2FRQ = 44,    // Modulator Filter 2 Frequency
            MF2Q = 45,      // Modulator Filter 2 Q Factor
            MF2G = 46,      // Modulator Filter 2 Gain

            CAMP = 47,      // Carrier Amplitude
            CVS = 48,       // Carrier Velocity Sensitivity
            CFLD = 49,      // Carrier Folding
            CPRT = 50,      // Carrier Portamento
            CPRD = 51,      // Carrier Portamento Depth
            CDTN = 52,      // Carrier Detune
            CFIN = 53,      // Carrier Fine Detune
            CWID = 54,      // Carrier Width
            CPAN = 55,      // Carrier Pan
            CVOL = 56,      // Carrier Volume

            CC1 = 57,       // Carrier Custom Waveform 1st Harmonic
            CC2 = 58,       // Carrier Custom Waveform 2nd Harmonic
            CC3 = 59,       // Carrier Custom Waveform 3rd Harmonic
            CC4 = 60,       // Carrier Custom Waveform 4th Harmonic
            CC5 = 61,       // Carrier Custom Waveform 5th Harmonic
            CC6 = 62,       // Carrier Custom Waveform 6th Harmonic
            CC7 = 63,       // Carrier Custom Waveform 7th Harmonic
            CC8 = 64,       // Carrier Custom Waveform 8th Harmonic
            CC9 = 65,       // Carrier Custom Waveform 9th Harmonic
            CC10 = 66,      // Carrier Custom Waveform 10th Harmonic

            CF1FRQ = 67,    // Carrier Filter 1 Frequency
            CF1Q = 68,      // Carrier Filter 1 Q Factor
            CF1G = 69,      // Carrier Filter 1 Gain

            CF2FRQ = 70,    // Carrier Filter 2 Frequency
            CF2Q = 71,      // Carrier Filter 2 Q Factor
            CF2G = 72,      // Carrier Filter 2 Gain

            EOG = 73,       // Effects Overdrive Gain

            EDG = 74,       // Effects Distortaion Gain

            EF1FRQ = 75,    // Effects Filter 1 Frequency
            EF1Q = 76,      // Effects Filter 1 Q Factor
            EF1G = 77,      // Effects Filter 1 Gain

            EF2FRQ = 78,    // Effects Filter 2 Frequency
            EF2Q = 79,      // Effects Filter 2 Q Factor
            EF2G = 80,      // Effects Filter 2 Gain

            EEDEL = 81,     // Effects Echo Delay
            EEFB = 82,      // Effects Echo Feedback
            EEDF = 83,      // Effects Echo Dampening Frequency
            EEDG = 84,      // Effects Echo Dampening Gain
            EEWID = 85,     // Effects Echo Stereo Width
            EEHPF = 86,     // Effects Echo Highpass Frequency
            EEWET = 87,     // Effects Echo Wet Volume
            EEDRY = 88,     // Effects Echo Dry Volume

            ERRS = 89,      // Effects Reverb Room Size
            ERDF = 90,      // Effects Reverb Dampening Frequency
            ERDG = 91,      // Effects Reverb Dampening Gain
            ERWID = 92,     // Effects Reverb Stereo Width
            ERHPF = 93,     // Effects Reverb Highpass Frequency
            ERWET = 94,     // Effects Reverb Wet Volume
            ERDRY = 95,     // Effects Reverb Dry Volume

            E1AMT = 96,     // Envelope 1 Amount
            E1INI = 97,     // Envelope 1 Initial Level
            E1DEL = 98,     // Envelope 1 Delay Time
            E1ATK = 99,     // Envelope 1 Attack Time
            E1PK = 100,     // Envelope 1 Peak Level
            E1HLD = 101,    // Envelope 1 Hold Time
            E1DEC = 102,    // Envelope 1 Decay Time
            E1SUS = 103,    // Envelope 1 Sustain Level
            E1REL = 104,    // Envelope 1 Release Time
            E1FIN = 105,    // Envelope 1 Final Level

            E2AMT = 106,    // Envelope 2 Amount
            E2INI = 107,    // Envelope 2 Initial Level
            E2DEL = 108,    // Envelope 2 Delay Time
            E2ATK = 109,    // Envelope 2 Attack Time
            E2PK = 110,     // Envelope 2 Peak Level
            E2HLD = 111,    // Envelope 2 Hold Time
            E2DEC = 112,    // Envelope 2 Decay Time
            E2SUS = 113,    // Envelope 2 Sustain Level
            E2REL = 114,    // Envelope 2 Release Time
            E2FIN = 115,    // Envelope 2 Final Level

            E3AMT = 116,    // Envelope 3 Amount
            E3INI = 117,    // Envelope 3 Initial Level
            E3DEL = 118,    // Envelope 3 Delay Time
            E3ATK = 119,    // Envelope 3 Attack Time
            E3PK = 120,     // Envelope 3 Peak Level
            E3HLD = 121,    // Envelope 3 Hold Time
            E3DEC = 122,    // Envelope 3 Decay Time
            E3SUS = 123,    // Envelope 3 Sustain Level
            E3REL = 124,    // Envelope 3 Release Time
            E3FIN = 125,    // Envelope 3 Final Level

            E4AMT = 126,    // Envelope 4 Amount
            E4INI = 127,    // Envelope 4 Initial Level
            E4DEL = 128,    // Envelope 4 Delay Time
            E4ATK = 129,    // Envelope 4 Attack Time
            E4PK = 130,     // Envelope 4 Peak Level
            E4HLD = 131,    // Envelope 4 Hold Time
            E4DEC = 132,    // Envelope 4 Decay Time
            E4SUS = 133,    // Envelope 4 Sustain Level
            E4REL = 134,    // Envelope 4 Release Time
            E4FIN = 135,    // Envelope 4 Final Level

            E5AMT = 136,    // Envelope 5 Amount
            E5INI = 137,    // Envelope 5 Initial Level
            E5DEL = 138,    // Envelope 5 Delay Time
            E5ATK = 139,    // Envelope 5 Attack Time
            E5PK = 140,     // Envelope 5 Peak Level
            E5HLD = 141,    // Envelope 5 Hold Time
            E5DEC = 142,    // Envelope 5 Decay Time
            E5SUS = 143,    // Envelope 5 Sustain Level
            E5REL = 144,    // Envelope 5 Release Time
            E5FIN = 145,    // Envelope 5 Final Level

            E6AMT = 146,    // Envelope 6 Amount
            E6INI = 147,    // Envelope 6 Initial Level
            E6DEL = 148,    // Envelope 6 Delay Time
            E6ATK = 149,    // Envelope 6 Attack Time
            E6PK = 150,     // Envelope 6 Peak Level
            E6HLD = 151,    // Envelope 6 Hold Time
            E6DEC = 152,    // Envelope 6 Decay Time
            E6SUS = 153,    // Envelope 6 Sustain Level
            E6REL = 154,    // Envelope 6 Release Time
            E6FIN = 155,    // Envelope 6 Final Level

            // TODO: generate the rest
            MAX_PARAM_ID = 156,
        };

        enum ControllerId {
            NONE = 0,                   // None
            MODULATION_WHEEL = 1,       // Modulation Wheel (CC 1)
            BREATH = 2,                 // Breath (CC 2)
            UNDEFINED_1 = 3,            // Undefined (CC 3)
            FOOT_PEDAL = 4,             // Foot Pedal (CC 4)
            PORTAMENTO_TIME = 5,        // Portamento Time (CC 5)
            VOLUME = 7,                 // Volume (CC 7)
            BALANCE = 8,                // Balance (CC 8)
            UNDEFINED_2 = 9,            // Undefined (CC 9)
            PAN = 10,                   // Pan (CC 10)
            EXPRESSION_PEDAL = 11,      // Expression Pedal (CC 11)
            FX_CTL_1 = 12,              // Effect Control 1 (CC 12)
            FX_CTL_2 = 13,              // Effect Control 2 (CC 13)
            UNDEFINED_3 = 14,           // Undefined (CC 14)
            UNDEFINED_4 = 15,           // Undefined (CC 15)
            GENERAL_1 = 16,             // General (CC 16)
            GENERAL_2 = 17,             // General (CC 17)
            GENERAL_3 = 18,             // General (CC 18)
            GENERAL_4 = 19,             // General (CC 19)
            UNDEFINED_5 = 20,           // Undefined (CC 20)
            UNDEFINED_6 = 21,           // Undefined (CC 21)
            UNDEFINED_7 = 22,           // Undefined (CC 22)
            UNDEFINED_8 = 23,           // Undefined (CC 23)
            UNDEFINED_9 = 24,           // Undefined (CC 24)
            UNDEFINED_10 = 25,          // Undefined (CC 25)
            UNDEFINED_11 = 26,          // Undefined (CC 26)
            UNDEFINED_12 = 27,          // Undefined (CC 27)
            UNDEFINED_13 = 28,          // Undefined (CC 28)
            UNDEFINED_14 = 29,          // Undefined (CC 29)
            UNDEFINED_15 = 30,          // Undefined (CC 30)
            UNDEFINED_16 = 31,          // Undefined (CC 31)
            PORTAMENTO_AMOUNT = 84,     // Portamento (CC 84)
            SOUND_1 = 70,               // Sound 1 (CC 70)
            SOUND_2 = 71,               // Sound 2 (CC 71)
            SOUND_3 = 72,               // Sound 3 (CC 72)
            SOUND_4 = 73,               // Sound 4 (CC 73)
            SOUND_5 = 74,               // Sound 5 (CC 74)
            SOUND_6 = 75,               // Sound 6 (CC 75)
            SOUND_7 = 76,               // Sound 7 (CC 76)
            SOUND_8 = 77,               // Sound 8 (CC 77)
            SOUND_9 = 78,               // Sound 9 (CC 78)
            SOUND_10 = 79,              // Sound 10 (CC 79)
            UNDEFINED_17 = 85,          // Undefined (CC 85)
            UNDEFINED_18 = 86,          // Undefined (CC 86)
            UNDEFINED_19 = 87,          // Undefined (CC 87)
            UNDEFINED_20 = 89,          // Undefined (CC 89)
            UNDEFINED_21 = 90,          // Undefined (CC 90)
            FX_1 = 91,                  // Effect 1 (CC 91)
            FX_2 = 92,                  // Effect 2 (CC 92)
            FX_3 = 93,                  // Effect 3 (CC 93)
            FX_4 = 94,                  // Effect 4 (CC 94)
            FX_5 = 95,                  // Effect 5 (CC 95)
            UNDEFINED_22 = 102,         // Undefined (CC 102)
            UNDEFINED_23 = 103,         // Undefined (CC 103)
            UNDEFINED_24 = 104,         // Undefined (CC 104)
            UNDEFINED_25 = 105,         // Undefined (CC 105)
            UNDEFINED_26 = 106,         // Undefined (CC 106)
            UNDEFINED_27 = 107,         // Undefined (CC 107)
            UNDEFINED_28 = 108,         // Undefined (CC 108)
            UNDEFINED_29 = 109,         // Undefined (CC 109)
            UNDEFINED_30 = 110,         // Undefined (CC 110)
            UNDEFINED_31 = 111,         // Undefined (CC 111)
            UNDEFINED_32 = 112,         // Undefined (CC 112)
            UNDEFINED_33 = 113,         // Undefined (CC 113)
            UNDEFINED_34 = 114,         // Undefined (CC 114)
            UNDEFINED_35 = 115,         // Undefined (CC 115)
            UNDEFINED_36 = 116,         // Undefined (CC 116)
            UNDEFINED_37 = 117,         // Undefined (CC 117)
            UNDEFINED_38 = 118,         // Undefined (CC 118)
            UNDEFINED_39 = 119,         // Undefined (CC 119)
            PITCH_WHEEL = 128,          // Pitch Wheel
            NOTE = 129,                 // Note
            VELOCITY = 130,             // Velocity
            SHAPED_CONTROLLER_1 = 131,  // Shaped Controller 1
            SHAPED_CONTROLLER_2 = 132,  // Shaped Controller 2
            SHAPED_CONTROLLER_3 = 133,  // Shaped Controller 3
            SHAPED_CONTROLLER_4 = 134,  // Shaped Controller 4
            SHAPED_CONTROLLER_5 = 135,  // Shaped Controller 5
            SHAPED_CONTROLLER_6 = 136,  // Shaped Controller 6
            SHAPED_CONTROLLER_7 = 137,  // Shaped Controller 7
            SHAPED_CONTROLLER_8 = 138,  // Shaped Controller 8
            SHAPED_CONTROLLER_9 = 139,  // Shaped Controller 9
            SHAPED_CONTROLLER_10 = 140, // Shaped Controller 10
            LFO_1 = 141,                // LFO 1
            LFO_2 = 142,                // LFO 2
            LFO_3 = 143,                // LFO 3
            LFO_4 = 144,                // LFO 4
            LFO_5 = 145,                // LFO 5
            LFO_6 = 146,                // LFO 6
            LFO_7 = 147,                // LFO 7
            LFO_8 = 148,                // LFO 8
            ENVELOPE_1 = 149,           // Envelope 1
            ENVELOPE_2 = 150,           // Envelope 2
            ENVELOPE_3 = 151,           // Envelope 3
            ENVELOPE_4 = 152,           // Envelope 4
            ENVELOPE_5 = 153,           // Envelope 5
            ENVELOPE_6 = 154,           // Envelope 6

            MAX_CONTROLLER_ID = 155,
        };

        Synth();
        virtual ~Synth() override;

        bool is_lock_free() const;

        void suspend();
        void resume();

        Sample const* const* generate_samples(
            Integer const round, Integer const sample_count
        );

        void push_message(
            MessageType const message,
            ParamId const param_id,
            Number const number_param,
            Byte const byte_param
        );

        Number get_param_ratio_atomic(ParamId const param_id) const;
        Number get_param_default_ratio(ParamId const param_id) const;
        Number float_param_ratio_to_display_value(
            ParamId const param_id,
            Number const ratio
        ) const;
        Byte int_param_ratio_to_display_value(
            ParamId const param_id,
            Number const ratio
        ) const;
        ControllerId get_param_controller_id(ParamId const param_id) const;

        void note_on(
            Seconds const time_offset,
            Midi::Channel const channel,
            Midi::Note const note,
            Number const velocity
        );

        void note_off(
            Seconds const time_offset,
            Midi::Channel const channel,
            Midi::Note const note,
            Number const velocity
        );

        void control_change(
            Seconds const time_offset,
            Midi::Channel const channel,
            Midi::Controller const controller,
            Number const new_value
        );

        void pitch_wheel_change(
            Seconds const time_offset,
            Midi::Channel const channel,
            Number const new_value
        );

        void all_sound_off(
            Seconds const time_offset, Midi::Channel const channel
        );

        void reset_all_controllers(
            Seconds const time_offset, Midi::Channel const channel
        );

        void all_notes_off(
            Seconds const time_offset, Midi::Channel const channel
        );

        Sample const* const* initialize_rendering(
            Integer const round,
            Integer const sample_count
        );

        void render(
            Integer const round,
            Integer const first_sample_index,
            Integer const last_sample_index,
            Sample** buffer
        );

        // TODO: operating mode: mix&mod (same as add + AM + FM in JS-80) vs. splits

        FloatParam volume;
        FloatParam modulator_add_volume;
        FloatParam frequency_modulation_level;
        FloatParam amplitude_modulation_level;

        Modulator::Params modulator_params;
        Carrier::Params carrier_params;

        MidiController pitch_wheel;
        MidiController note;
        MidiController velocity;

    protected:
        Frequency frequencies[Midi::NOTES];

    private:
        class Message
        {
            public:
                Message();
                Message(Message const& message);
                Message(
                    MessageType const type,
                    ParamId const param_id,
                    Number const number_param,
                    Byte const byte_param
                );

                Message& operator=(Message const& message);

                MessageType type;
                ParamId param_id;
                Number number_param;
                Byte byte_param;
        };

        // See Timur Doumler [ACCU 2017]: Lock-free programming with modern C++
        // https://www.youtube.com/watch?v=qdrp6k4rcP4
        class SingleProducerSingleConsumerMessageQueue
        {
            public:
                SingleProducerSingleConsumerMessageQueue();

                SingleProducerSingleConsumerMessageQueue(
                    SingleProducerSingleConsumerMessageQueue const& queue
                ) = delete;

                bool is_lock_free() const;
                bool push(Message const& message);
                bool pop(Message& message);
                size_t size() const;

            private:
                static constexpr size_t SIZE = 0x1000; // must be power of 2
                static constexpr size_t SIZE_MASK = SIZE - 1;

                size_t advance(size_t const index) const;

                Message messages[SIZE];

                std::atomic<size_t> next_push;
                std::atomic<size_t> next_pop;
        };

        class Bus : public SignalProducer
        {
            public:
                Bus(
                    Integer const channels,
                    Modulator* const* const modulators,
                    Carrier* const* const carriers,
                    Integer const polyphony,
                    FloatParam& volume,
                    FloatParam& modulator_add_volume
                );

                Sample const* const* initialize_rendering(
                    Integer const round,
                    Integer const sample_count
                );

                void render(
                    Integer const round,
                    Integer const first_sample_index,
                    Integer const last_sample_index,
                    Sample** buffer
                );

            private:
                void mix_modulators(
                    Integer const round,
                    Integer const first_sample_index,
                    Integer const last_sample_index,
                    Sample** buffer
                ) const;

                void mix_carriers(
                    Integer const round,
                    Integer const first_sample_index,
                    Integer const last_sample_index,
                    Sample** buffer
                ) const;

                void apply_volume(
                    Integer const round,
                    Integer const first_sample_index,
                    Integer const last_sample_index,
                    Sample** buffer
                ) const;

                Integer const polyphony;
                Synth::Modulator* const* const modulators;
                Synth::Carrier* const* const carriers;
                FloatParam& volume;
                FloatParam& modulator_add_volume;
                Sample const* volume_buffer;
                Sample const* modulator_add_volume_buffer;
                std::vector<bool> modulators_on;
                std::vector<bool> carriers_on;
                bool is_silent;
        };

        static constexpr Integer NEXT_VOICE_MASK = 0x0f;
        static constexpr Integer POLYPHONY = NEXT_VOICE_MASK + 1;

        static constexpr Integer SPECIAL_PARAMS = 17;
        static constexpr Integer FLOAT_PARAMS_MAX = (
            ParamId::MAX_PARAM_ID - SPECIAL_PARAMS
        );

        static constexpr Number NOTE_TO_PARAM_SCALE = 1.0 / 127.0;

        void register_float_param(
            ParamId const param_id,
            FloatParam& float_param
        );

        void process_messages();
        void handle_set_param(
            ParamId const param_id,
            Number const ratio
        );
        void handle_assign_controller(
            ParamId const param_id,
            Byte const controller_id
        );
        void handle_refresh_param(ParamId const param_id);

        Number get_param_ratio(ParamId const param_id) const;

        void clear_midi_controllers();

        void update_param_states();

        std::string const to_string(Integer const) const;

        SingleProducerSingleConsumerMessageQueue messages;
        Bus bus;
        FloatParam* float_params[FLOAT_PARAMS_MAX];
        std::atomic<Number> param_ratios[ParamId::MAX_PARAM_ID];
        std::atomic<Byte> controller_assignments[ParamId::MAX_PARAM_ID];
        Envelope* envelopes_rw[ENVELOPES];
        MidiController* midi_controllers_rw[MIDI_CONTROLLERS];
        Integer midi_note_to_voice_assignments[Midi::CHANNELS][Midi::NOTES];
        Modulator* modulators[POLYPHONY];
        Carrier* carriers[POLYPHONY];
        Integer next_voice;
        Midi::Note previous_note;

    public:
        Envelope* const* const envelopes;
        MidiController* const* const midi_controllers;
};

}

#endif
