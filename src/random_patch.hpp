/*
 * This file is part of JS80P, a synthesizer plugin.
 * Copyright (C) 2026  Attila M. Magyar
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

#ifndef JS80P__RANDOM_PATCH_HPP
#define JS80P__RANDOM_PATCH_HPP

#include <array>
#include <cstddef>
#include <vector>
#include <tuple>

#include "js80p.hpp"
#include "synth.hpp"

#include "dsp/envelope.hpp"
#include "dsp/lfo.hpp"
#include "dsp/macro.hpp"
#include "dsp/math.hpp"


namespace JS80P
{

class Synth;

class RandomPatchGenerator
{
    public:
        RandomPatchGenerator(
            Synth& synth,
            Integer const random_seed,
            bool const has_cc_74,
            bool const has_channel_pressure
        );

        void generate() noexcept;

    private:
        typedef std::tuple<
            Synth::ControllerId,
            Synth::ParamId,
            Macro*
        > MacroDescriptor;

        typedef std::tuple<
            Synth::ControllerId,
            Synth::ParamId,
            LFO*
        > LFODescriptor;

        typedef std::tuple<
            Synth::ControllerId,
            Synth::ParamId,
            Envelope*
        > EnvelopeDescriptor;

        typedef std::vector<Synth::ControllerId> MidiControllers;
        typedef std::vector<MacroDescriptor> MacroDescriptors;
        typedef std::vector<LFODescriptor> LFODescriptors;
        typedef std::vector<EnvelopeDescriptor> EnvelopeDescriptors;

        typedef std::array<Number, 10> Harmonics;

        void set_param_ratio(
            Synth::ParamId const param_id,
            Number const ratio
        ) noexcept;

        void set_param_ratio_if(
            bool const condition,
            Synth::ParamId const param_id,
            Number const ratio
        ) noexcept;

        void assign_controller(
            Synth::ParamId const param_id,
            Synth::ControllerId const controller_id
        ) noexcept;

        void assign_controller_if(
            bool const condition,
            Synth::ParamId const param_id,
            Synth::ControllerId const controller_id
        ) noexcept;

        Synth::ControllerId pick_random_midi_controller(
            Number const mod_wheel_probability,
            Synth::ControllerId const fallback_controller_id = (
                Synth::ControllerId::INVALID_CONTROLLER_ID
            )
        ) noexcept;

        Number pick_random_pitch_wheel_range() noexcept;

        Number random_normal_pow(
            Number const min,
            Number const max,
            Number const pow
        ) noexcept;

        void clear_synth_settings() noexcept;
        void initialize() noexcept;
        void set_up_flags() noexcept;
        void set_up_note_handling() noexcept;
        void set_up_portamento() noexcept;
        void set_up_amplitude() noexcept;
        void set_up_modulation() noexcept;
        void set_up_waveform() noexcept;
        void set_up_inaccuracy() noexcept;
        void set_up_detune() noexcept;
        void set_up_pitch_wheel_without_vibrato() noexcept;
        void set_up_vibrato() noexcept;
        void set_up_waveshaper() noexcept;
        void set_up_filter() noexcept;
        void set_up_volume() noexcept;
        void set_up_panning() noexcept;
        void set_up_fx_distortion() noexcept;
        void set_up_fx_filter() noexcept;
        void set_up_fx_tape() noexcept;
        void set_up_fx_chorus() noexcept;
        void set_up_fx_echo() noexcept;
        void set_up_fx_reverb() noexcept;

        void randomize_amp_envelope(
            EnvelopeDescriptor const& envelope_descriptor,
            Number const peak_value
        ) noexcept;

        void randomize_envelope_shape(Envelope& envelope) noexcept;
        void randomize_pwm_lfo(LFO& lfo) noexcept;
        void randomize_lfo_waveform(LFO& lfo) noexcept;
        void randomize_pwm_envelope(Envelope& envelope) noexcept;

        template<std::size_t N>
        void randomize_envelope_shape(
            Envelope& envelope,
            std::array<EnvelopeShape, N> const& shapes
        ) noexcept;

        void randomize_waveform(
            bool const soft_only,
            Byte& waveform,
            Harmonics& harmonics,
            bool& is_pulse
        ) noexcept;

        void randomize_waveshaper(
            Number const no_op_probability,
            Number& level,
            Synth::ControllerId& ctl_id
        ) noexcept;

        void randomize_filter(
            Number& q,
            Synth::ControllerId& freq_ctl_id
        ) noexcept;

        void randomize_volume(
            Number& volume,
            Synth::ControllerId& ctl_id
        ) noexcept;

        Byte pick_random_distortion_type() noexcept;

        void refresh_all_params() const noexcept;

        bool const has_cc_74:1;
        bool const has_channel_pressure:1;

        Synth& synth;
        Math::RNG rng;
        MidiControllers available_midi_controllers;
        MacroDescriptors available_macros;
        EnvelopeDescriptors available_envelopes;
        MacroDescriptors::const_iterator next_macro;
        EnvelopeDescriptors::const_iterator next_envelope;
        Number no_filter_probability;
        Integer mod_wheel_budget;

        bool is_monophonic:1;
        bool is_pluck:1;
        bool is_slow_attack:1;
        bool is_decaying:1;
        bool is_osc_1_active:1;
        bool is_osc_2_active:1;
        bool is_mix:1;
        bool is_pm:1;
        bool is_fm:1;
        bool is_am:1;
        bool is_stereo:1;
        bool is_lfo_8_used:1;
        bool can_use_reverse_echo:1;
};

}

#endif
