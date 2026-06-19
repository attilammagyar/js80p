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

#ifndef JS80P__RANDOM_PATCH_CPP
#define JS80P__RANDOM_PATCH_CPP

#include <algorithm>
#include <cmath>
#include <cstddef>

#include "random_patch.hpp"

#include "dsp/chorus.hpp"
#include "dsp/distortion.hpp"
#include "dsp/echo.hpp"
#include "dsp/oscillator.hpp"
#include "dsp/reverb.hpp"


namespace JS80P
{

/*
Fixed controller allocation:

 * LFO 1, 2, 4: volume effects
 * LFO 3: PM/FM/AM
 * LFO 5, 6: PWM
 * LFO 7: vibrato
 * LFO 8: either filter frequency (oscillator or effects) or delay time
 * Envelope 11: dummy for making LFO 1 and friends polyphonic
 * Envelope 12: dummy for making LFO 7 polyphonic
*/

RandomPatchGenerator::RandomPatchGenerator(
        Synth& synth,
        Integer const random_seed
) : synth(synth),
    rng((unsigned int)(random_seed > 0 ? random_seed : -random_seed))
{
}


void RandomPatchGenerator::generate() noexcept
{
    clear_synth_settings();

    initialize();
    set_up_flags();
    set_up_note_handling();
    set_up_portamento();
    set_up_amplitude();
    set_up_modulation();
    set_up_waveform();
    set_up_inaccuracy();
    set_up_detune();

    if (rng.random() < 0.5) {
        set_up_pitch_wheel_without_vibrato();
    } else {
        set_up_vibrato();
    }

    set_up_waveshaper();
    set_up_filter();
    set_up_volume();
    set_up_panning();

    set_up_fx_distortion();
    set_up_fx_filter();
    set_up_fx_tape();
    set_up_fx_chorus();
    set_up_fx_echo();
    set_up_fx_reverb();

    refresh_all_params();
}


void RandomPatchGenerator::set_param_ratio(
        Synth::ParamId const param_id,
        Number const ratio
) noexcept {
    synth.process_message(Synth::MessageType::SET_PARAM, param_id, ratio, 0);
}


void RandomPatchGenerator::set_param_ratio_if(
        bool const condition,
        Synth::ParamId const param_id,
        Number const ratio
) noexcept {
    if (condition) {
        set_param_ratio(param_id, ratio);
    }
}


void RandomPatchGenerator::assign_controller(
        Synth::ParamId const param_id,
        Synth::ControllerId const controller_id
) noexcept {
    synth.process_message(
        Synth::MessageType::ASSIGN_CONTROLLER,
        param_id,
        0.0,
        (Byte)controller_id
    );
}


void RandomPatchGenerator::assign_controller_if(
        bool const condition,
        Synth::ParamId const param_id,
        Synth::ControllerId const controller_id
) noexcept {
    if (condition) {
        assign_controller(param_id, controller_id);
    }
}


Synth::ControllerId RandomPatchGenerator::pick_random_midi_controller(
    Number const mod_wheel_probability,
    Synth::ControllerId const fallback_controller_id
) noexcept {
    if (mod_wheel_budget == 0 && channel_pressure_budget == 0) {
        return fallback_controller_id;
    }

    if (mod_wheel_budget == 0) {
        --channel_pressure_budget;

        return Synth::ControllerId::CHANNEL_PRESSURE;
    }

    if (channel_pressure_budget == 0) {
        --mod_wheel_budget;

        return Synth::ControllerId::MODULATION_WHEEL;
    }

    if (rng.random() < mod_wheel_probability) {
        --mod_wheel_budget;

        return Synth::ControllerId::MODULATION_WHEEL;
    }

    --channel_pressure_budget;

    return Synth::ControllerId::CHANNEL_PRESSURE;
}


Number RandomPatchGenerator::random_normal_pow(
        Number const min,
        Number const max,
        Number const pow
) noexcept {
    Number const raw_number = std::pow(rng.random_normal(), pow);

    return min + raw_number * (max - min);
}


void RandomPatchGenerator::clear_synth_settings() noexcept
{
    synth.process_message(
        Synth::MessageType::CLEAR, Synth::ParamId::INVALID_PARAM_ID, 0.0, 0
    );

    set_param_ratio(Synth::ParamId::MF1LOG, 1.0);
    set_param_ratio(Synth::ParamId::MF2LOG, 1.0);
    set_param_ratio(Synth::ParamId::CF1LOG, 1.0);
    set_param_ratio(Synth::ParamId::CF2LOG, 1.0);
    set_param_ratio(Synth::ParamId::EF1LOG, 1.0);
    set_param_ratio(Synth::ParamId::EF2LOG, 1.0);
    set_param_ratio(Synth::ParamId::ECLOG, 1.0);
    set_param_ratio(Synth::ParamId::EELOG, 1.0);
    set_param_ratio(Synth::ParamId::ERLOG, 1.0);
}


void RandomPatchGenerator::initialize() noexcept
{
    available_macros = {
        {
            Synth::ControllerId::MACRO_1,
            Synth::ParamId::M1IN,
            synth.macros[0]
        },
        {
            Synth::ControllerId::MACRO_2,
            Synth::ParamId::M2IN,
            synth.macros[1]
        },
        {
            Synth::ControllerId::MACRO_3,
            Synth::ParamId::M3IN,
            synth.macros[2]
        },
        {
            Synth::ControllerId::MACRO_4,
            Synth::ParamId::M4IN,
            synth.macros[3]
        },
        {
            Synth::ControllerId::MACRO_5,
            Synth::ParamId::M5IN,
            synth.macros[4]
        },
        {
            Synth::ControllerId::MACRO_6,
            Synth::ParamId::M6IN,
            synth.macros[5]
        },
        {
            Synth::ControllerId::MACRO_7,
            Synth::ParamId::M7IN,
            synth.macros[6]
        },
        {
            Synth::ControllerId::MACRO_8,
            Synth::ParamId::M8IN,
            synth.macros[7]
        },
        {
            Synth::ControllerId::MACRO_9,
            Synth::ParamId::M9IN,
            synth.macros[8]
        },
        {
            Synth::ControllerId::MACRO_10,
            Synth::ParamId::M10IN,
            synth.macros[9]
        },
        {
            Synth::ControllerId::MACRO_11,
            Synth::ParamId::M11IN,
            synth.macros[10]
        },
        {
            Synth::ControllerId::MACRO_12,
            Synth::ParamId::M12IN,
            synth.macros[11]
        },
        {
            Synth::ControllerId::MACRO_13,
            Synth::ParamId::M13IN,
            synth.macros[12]
        },
        {
            Synth::ControllerId::MACRO_14,
            Synth::ParamId::M14IN,
            synth.macros[13]
        },
        {
            Synth::ControllerId::MACRO_15,
            Synth::ParamId::M15IN,
            synth.macros[14]
        },
        {
            Synth::ControllerId::MACRO_16,
            Synth::ParamId::M16IN,
            synth.macros[15]
        },
        {
            Synth::ControllerId::MACRO_17,
            Synth::ParamId::M17IN,
            synth.macros[16]
        },
        {
            Synth::ControllerId::MACRO_18,
            Synth::ParamId::M18IN,
            synth.macros[17]
        },
        {
            Synth::ControllerId::MACRO_19,
            Synth::ParamId::M19IN,
            synth.macros[18]
        },
        {
            Synth::ControllerId::MACRO_20,
            Synth::ParamId::M20IN,
            synth.macros[19]
        },
        {
            Synth::ControllerId::MACRO_21,
            Synth::ParamId::M21IN,
            synth.macros[20]
        },
        {
            Synth::ControllerId::MACRO_22,
            Synth::ParamId::M22IN,
            synth.macros[21]
        },
        {
            Synth::ControllerId::MACRO_23,
            Synth::ParamId::M23IN,
            synth.macros[22]
        },
        {
            Synth::ControllerId::MACRO_24,
            Synth::ParamId::M24IN,
            synth.macros[23]
        },
        {
            Synth::ControllerId::MACRO_25,
            Synth::ParamId::M25IN,
            synth.macros[24]
        },
        {
            Synth::ControllerId::MACRO_26,
            Synth::ParamId::M26IN,
            synth.macros[25]
        },
        {
            Synth::ControllerId::MACRO_27,
            Synth::ParamId::M27IN,
            synth.macros[26]
        },
        {
            Synth::ControllerId::MACRO_28,
            Synth::ParamId::M28IN,
            synth.macros[27]
        },
        {
            Synth::ControllerId::MACRO_29,
            Synth::ParamId::M29IN,
            synth.macros[28]
        },
        {
            Synth::ControllerId::MACRO_30,
            Synth::ParamId::M30IN,
            synth.macros[29]
        },
    };

    available_envelopes = {
        {
            Synth::ControllerId::ENVELOPE_1,
            Synth::ParamId::N1SCL,
            synth.envelopes[0]
        },
        {
            Synth::ControllerId::ENVELOPE_2,
            Synth::ParamId::N2SCL,
            synth.envelopes[1]
        },
        {
            Synth::ControllerId::ENVELOPE_3,
            Synth::ParamId::N3SCL,
            synth.envelopes[2]
        },
        {
            Synth::ControllerId::ENVELOPE_4,
            Synth::ParamId::N4SCL,
            synth.envelopes[3]
        },
        {
            Synth::ControllerId::ENVELOPE_5,
            Synth::ParamId::N5SCL,
            synth.envelopes[4]
        },
        {
            Synth::ControllerId::ENVELOPE_6,
            Synth::ParamId::N6SCL,
            synth.envelopes[5]
        },
        {
            Synth::ControllerId::ENVELOPE_7,
            Synth::ParamId::N7SCL,
            synth.envelopes[6]
        },
        {
            Synth::ControllerId::ENVELOPE_8,
            Synth::ParamId::N8SCL,
            synth.envelopes[7]
        },
        {
            Synth::ControllerId::ENVELOPE_9,
            Synth::ParamId::N9SCL,
            synth.envelopes[8]
        },
        {
            Synth::ControllerId::ENVELOPE_10,
            Synth::ParamId::N10SCL,
            synth.envelopes[9]
        },
    };

    next_macro = available_macros.begin();
    next_envelope = available_envelopes.begin();

    no_filter_probability = 0.1;

    mod_wheel_budget = 2;
    channel_pressure_budget = 2;
}


void RandomPatchGenerator::set_up_flags() noexcept
{
    is_monophonic = rng.random() < 0.2;
    is_pluck = !is_monophonic && rng.random() < 0.25;
    is_slow_attack = !is_pluck && rng.random() < 0.3;
    is_decaying = is_pluck || rng.random() < 0.25;

    is_osc_1_active = rng.random() < 0.7;
    is_osc_2_active = !is_osc_1_active || rng.random() < 0.7;

    is_mix = is_pm = is_fm = is_am = is_stereo = false;
    is_lfo_8_used = false;

    can_use_reverse_echo = is_pluck;

    if (is_osc_1_active && is_osc_2_active) {
        Number const choice = rng.random();

        if (choice < 0.3) {
            is_mix = true;
            is_stereo = rng.random() < 0.6;
        } else if (choice < 0.6) {
            is_pm = true;
        } else if (choice < 0.9) {
            is_fm = true;
        } else {
            is_am = true;
        }
    } else {
        is_mix = true;
    }
}


void RandomPatchGenerator::set_up_note_handling() noexcept
{
    if (is_monophonic) {
        synth.note_handling.set_value(Synth::NOTE_HANDLING_MONO);
    } else if (is_decaying) {
        synth.note_handling.set_value(Synth::NOTE_HANDLING_POLY);
    } else {
        synth.note_handling.set_value(Synth::NOTE_HANDLING_POLY_RETRIGGER);
    }
}


void RandomPatchGenerator::set_up_portamento() noexcept
{
    if (rng.random() < 0.5) {
        return;
    }

    if (is_monophonic) {
        /* Random portamento time. */
        Number const portamento_length_ratio = rng.random(0.005, 0.06);

        set_param_ratio_if(
            is_osc_1_active, Synth::ParamId::MPRT, portamento_length_ratio
        );
        set_param_ratio_if(
            is_osc_2_active, Synth::ParamId::CPRT, portamento_length_ratio
        );

        if (rng.random() < 0.7) {
            /*
            Negligible portamento depth turns off glide from previous note while
            maintaining it between legato notes.
            */
            set_param_ratio_if(is_osc_1_active, Synth::ParamId::MPRD, 0.4999);
            set_param_ratio_if(is_osc_2_active, Synth::ParamId::CPRD, 0.4999);
        }

        return;
    }

    if (is_pluck && rng.random() < 0.5) {
        /*
        Percussive transient via a note-dependent very short downward glide.
        */

        MacroDescriptor const& macro_descriptor = *next_macro++;

        Synth::ControllerId const macro_ctl_id = std::get<0>(macro_descriptor);
        Synth::ParamId const macro_input_param_id = std::get<1>(
            macro_descriptor
        );
        Macro& macro = *std::get<2>(macro_descriptor);

        assign_controller(
            macro_input_param_id, Synth::ControllerId::TRIGGERED_NOTE
        );
        macro.min.set_value(0.0035);
        macro.max.set_value(0.0010);
        macro.distortion.set_value(0.15);

        if (is_osc_1_active && is_mix) {
            assign_controller(Synth::ParamId::MPRT, macro_ctl_id);
            set_param_ratio(Synth::ParamId::MPRD, 1.0);
        }

        if (is_osc_2_active) {
            assign_controller(Synth::ParamId::CPRT, macro_ctl_id);
            set_param_ratio(Synth::ParamId::CPRD, 1.0);
        }

        return;
    }

    /* Random portamento time. */

    Number const portamento_length_ratio = (
        is_pluck
            ? random_normal_pow(0.03, 0.12, 3.0)
            : random_normal_pow(0.05, 0.20, 2.3)
    );

    set_param_ratio_if(
        is_osc_1_active, Synth::ParamId::MPRT, portamento_length_ratio
    );
    set_param_ratio_if(
        is_osc_2_active, Synth::ParamId::CPRT, portamento_length_ratio
    );

    if (rng.random() < 0.7) {
        /* Random portamento depth. */

        Number const portamento_depth_st = rng.random_choice<Number, 10>(
            {3.0, 6.0, 7.0, 9.0, 12.0, 15.0, 18.0, 19.0, 21.0, 24.0}
        );
        Number const portamento_depth_sign = rng.random_choice<Number, 2>(
            {-1.0, 1.0}
        );
        Number const portamento_depth_cents = (
            100.0 * portamento_depth_sign * portamento_depth_st
        );
        Number const portamento_depth_ratio = (
            synth.modulator_params.portamento_depth.value_to_ratio(
                portamento_depth_cents
            )
        );

        set_param_ratio_if(
            is_osc_1_active, Synth::ParamId::MPRD, portamento_depth_ratio
        );
        set_param_ratio_if(
            is_osc_2_active, Synth::ParamId::CPRD, portamento_depth_ratio
        );
    }
}


void RandomPatchGenerator::set_up_amplitude() noexcept
{
    set_param_ratio_if(!is_mix, Synth::ParamId::MIX, 0.0);
    set_param_ratio_if(!is_osc_1_active, Synth::ParamId::MAMP, 0.0);
    set_param_ratio_if(!is_osc_2_active, Synth::ParamId::CAMP, 0.0);

    EnvelopeDescriptors::const_iterator envelope_descriptor = next_envelope++;
    Synth::ControllerId envelope_ctl_id = std::get<0>(*envelope_descriptor);

    randomize_amp_envelope(*envelope_descriptor, 1.0);

    EnvelopeDescriptors::const_iterator noise_envelope_descriptor = (
        available_envelopes.end()
    );
    Synth::ControllerId noise_envelope_ctl_id = (
        Synth::ControllerId::INVALID_CONTROLLER_ID
    );
    bool const is_noisy_pluck = is_pluck && rng.random() < 0.3;

    if (is_noisy_pluck) {
        /*
        Emphasize transients via a short burst of noise. Assignments will be
        handled later.
        */

        noise_envelope_descriptor = next_envelope++;
        noise_envelope_ctl_id = std::get<0>(*noise_envelope_descriptor);
        Envelope& noise_envelope = *std::get<2>(*noise_envelope_descriptor);

        noise_envelope.attack_shape.set_value(Envelope::SHAPE_SHARP_SMOOTH);
        noise_envelope.decay_shape.set_value(Envelope::SHAPE_SHARP_SMOOTH);
        noise_envelope.release_shape.set_value(Envelope::SHAPE_SHARP_SMOOTH);

        noise_envelope.scale.set_value(rng.random(0.100, 0.500));
        noise_envelope.attack_time.set_value(rng.random(0.005, 0.010));
        noise_envelope.hold_time.set_value(0.000);
        noise_envelope.decay_time.set_value(rng.random(0.010, 0.050));
        noise_envelope.sustain_value.set_value(0.0);
        noise_envelope.release_time.set_value(rng.random(0.010, 0.050));
    }

    if (is_stereo) {
        assign_controller(Synth::ParamId::MAMP, envelope_ctl_id);
        assign_controller(Synth::ParamId::CAMP, envelope_ctl_id);

        if (is_noisy_pluck) {
            assign_controller(Synth::ParamId::MN, noise_envelope_ctl_id);
            assign_controller(Synth::ParamId::CN, noise_envelope_ctl_id);
        }

        return;
    }

    if (is_osc_1_active) {
        /*
        Assign amplitude envelopes to Oscillator 1 and its noise generator.
        */

        assign_controller(Synth::ParamId::MAMP, envelope_ctl_id);
        assign_controller_if(
            is_noisy_pluck, Synth::ParamId::MN, noise_envelope_ctl_id
        );

        if (rng.random() < 0.5) {
            /* Turn on the sub-harmonic oscillator. */

            if (rng.random() < 0.7) {
                /* Use a different envelope for the sub-harmonic oscillator. */

                envelope_descriptor = next_envelope++;
                envelope_ctl_id = std::get<0>(*envelope_descriptor);

                randomize_amp_envelope(
                    *envelope_descriptor, rng.random(0.2, 1.0)
                );
            }

            assign_controller(Synth::ParamId::MSUB, envelope_ctl_id);
        }

        if (is_osc_2_active) {
            /* Use a different amplitude envelope for Oscillator 2. */

            envelope_descriptor = next_envelope++;
            envelope_ctl_id = std::get<0>(*envelope_descriptor);

            randomize_amp_envelope(*envelope_descriptor, 1.0);
        }
    }

    if (is_osc_2_active) {
        /*
        Assign amplitude envelopes to Oscillator 2 and its noise generator.
        */

        assign_controller(Synth::ParamId::CAMP, envelope_ctl_id);
        assign_controller_if(
            is_noisy_pluck, Synth::ParamId::CN, noise_envelope_ctl_id
        );
    }
}


void RandomPatchGenerator::randomize_amp_envelope(
        EnvelopeDescriptor const& envelope_descriptor,
        Number const peak_value
) noexcept {
    Envelope& envelope = *std::get<2>(envelope_descriptor);

    if (rng.random() < 0.7) {
        randomize_envelope_shape(envelope);
    }

    if (is_pluck) {
        envelope.attack_time.set_value(rng.random(0.003, 0.015));
        envelope.hold_time.set_value(rng.random(0.000, 0.300));
        envelope.release_time.set_value(rng.random(0.020, 1.000));
    } else if (is_slow_attack) {
        envelope.attack_time.set_value(rng.random(0.350, 3.000));
        envelope.hold_time.set_value(rng.random(0.000, 6.000));
        envelope.release_time.set_value(rng.random(0.150, 1.500));
    } else {
        envelope.attack_time.set_value(rng.random(0.010, 0.900));
        envelope.hold_time.set_value(rng.random(0.000, 1.500));
        envelope.release_time.set_value(rng.random(0.050, 0.700));
    }

    if (is_decaying) {
        envelope.sustain_value.set_value(0.0);

        if (rng.random() < 0.5) {
            envelope.decay_time.set_value(rng.random(0.300, 15.000));
        } else {
            /* Use slower decay for lower notes. */

            Synth::ParamId const envelope_decay_time_param_id = (
                (Synth::ParamId)((int)std::get<1>(envelope_descriptor) + 6)
            );

            MacroDescriptor const& macro_descriptor = *next_macro++;

            Synth::ControllerId const macro_ctl_id = std::get<0>(
                macro_descriptor
            );
            Synth::ParamId const macro_input_param_id = std::get<1>(
                macro_descriptor
            );
            Macro& macro = *std::get<2>(macro_descriptor);

            assign_controller(
                macro_input_param_id, Synth::ControllerId::TRIGGERED_NOTE
            );
            macro.min.set_value(rng.random(0.150, 1.000));
            macro.max.set_value(rng.random(0.001, 0.100));
            macro.distortion.set_value(0.15);

            assign_controller(envelope_decay_time_param_id, macro_ctl_id);
        }

        return;
    }

    envelope.peak_value.set_value(peak_value);
    envelope.sustain_value.set_value(rng.random(0.20, 1.00));
    envelope.decay_time.set_value(rng.random(0.500, 15.000));
}


void RandomPatchGenerator::randomize_envelope_shape(Envelope& envelope) noexcept
{
    if (rng.random() < 0.8) {
        std::array<EnvelopeShape, 5> const shapes = {
            Envelope::SHAPE_SMOOTH_SMOOTH,
            Envelope::SHAPE_SMOOTH_SHARP,
            Envelope::SHAPE_SHARP_SMOOTH,
            Envelope::SHAPE_SHARP_SHARP,
            Envelope::SHAPE_LINEAR,
        };

        randomize_envelope_shape(envelope, shapes);

        return;
    }

    std::array<EnvelopeShape, 13> const shapes = {
        Envelope::SHAPE_SMOOTH_SMOOTH,
        Envelope::SHAPE_SMOOTH_SMOOTH_STEEP,
        Envelope::SHAPE_SMOOTH_SMOOTH_STEEPER,
        Envelope::SHAPE_SMOOTH_SHARP,
        Envelope::SHAPE_SMOOTH_SHARP_STEEP,
        Envelope::SHAPE_SMOOTH_SHARP_STEEPER,
        Envelope::SHAPE_SHARP_SMOOTH,
        Envelope::SHAPE_SHARP_SMOOTH_STEEP,
        Envelope::SHAPE_SHARP_SMOOTH_STEEPER,
        Envelope::SHAPE_SHARP_SHARP,
        Envelope::SHAPE_SHARP_SHARP_STEEP,
        Envelope::SHAPE_SHARP_SHARP_STEEPER,
        Envelope::SHAPE_LINEAR,
    };

    randomize_envelope_shape(envelope, shapes);
}


template<std::size_t N>
void RandomPatchGenerator::randomize_envelope_shape(
        Envelope& envelope,
        std::array<EnvelopeShape, N> const& shapes
) noexcept {
    envelope.attack_shape.set_value(rng.random_choice<EnvelopeShape>(shapes));
    envelope.decay_shape.set_value(rng.random_choice<EnvelopeShape>(shapes));
    envelope.release_shape.set_value(rng.random_choice<EnvelopeShape>(shapes));
}


void RandomPatchGenerator::set_up_modulation() noexcept
{
    if (is_mix) {
        return;
    }

    Synth::ParamId const mod_param_id = (
        is_pm
            ? Synth::ParamId::PM
            : is_fm ? Synth::ParamId::FM : Synth::ParamId::AM
    );

    Number const choice = rng.random();

    if (choice < 0.2) {
        /*
        MIDI-controlled modulation level, or a fixed level if no controllers are
        available.
        */

        Synth::ControllerId const mod_level_ctl_id = (
            pick_random_midi_controller(0.7)
        );

        if (mod_level_ctl_id == Synth::ControllerId::INVALID_CONTROLLER_ID) {
            set_param_ratio(mod_param_id, rng.random(0.2, 1.0));

            return;
        }

        MacroDescriptor const& macro_descriptor = *next_macro++;

        Synth::ControllerId const macro_ctl_id = std::get<0>(macro_descriptor);
        Synth::ParamId const macro_input_param_id = std::get<1>(
            macro_descriptor
        );
        Macro& macro = *std::get<2>(macro_descriptor);

        macro.min.set_value(rng.random(0.00, 0.15));
        macro.max.set_value(rng.random(0.25, 1.00));

        assign_controller(mod_param_id, macro_ctl_id);
        assign_controller(macro_input_param_id, mod_level_ctl_id);

        return;
    }

    if (choice < 0.5) {
        /* LFO-controlled modulation level. */

        LFO& lfo = *synth.lfos[2];

        lfo.waveform.set_value(
            rng.random_choice<Byte, 3>(
                {
                    SimpleOscillator::SINE,
                    SimpleOscillator::TRIANGLE,
                    SimpleOscillator::SOFT_TRIANGLE,
                }
            )
        );
        lfo.frequency.set_value(rng.random(0.05, 0.35));
        lfo.min.set_value(rng.random(0.00, 0.15));
        lfo.max.set_value(rng.random(0.25, 1.00));

        if (rng.random() < 0.3) {
            lfo.distortion.set_value(rng.random(0.05, 0.30));
        }

        assign_controller(mod_param_id, Synth::ControllerId::LFO_3);

        return;
    }

    /* Envelope-generator-controlled modulation level. */

    EnvelopeDescriptor const& envelope_descriptor = *next_envelope++;
    Synth::ControllerId envelope_ctl_id = std::get<0>(envelope_descriptor);
    Envelope& envelope = *std::get<2>(envelope_descriptor);

    randomize_envelope_shape(envelope);

    envelope.initial_value.set_ratio(rng.random());

    if (rng.random() < 0.5) {
        envelope.delay_time.set_ratio(rng.random(0.0, 0.2));
    }

    envelope.attack_time.set_ratio(rng.random());
    envelope.peak_value.set_ratio(rng.random());
    envelope.hold_time.set_ratio(rng.random());
    envelope.decay_time.set_ratio(rng.random());
    envelope.sustain_value.set_ratio(rng.random());
    envelope.release_time.set_ratio(rng.random(0.1, 1.0));
    envelope.final_value.set_ratio(rng.random());

    assign_controller(mod_param_id, envelope_ctl_id);
}


void RandomPatchGenerator::set_up_waveform() noexcept
{
    Harmonics osc_1_harmonics;
    Harmonics osc_2_harmonics;
    Byte osc_1_waveform = SimpleOscillator::SINE;
    Byte osc_2_waveform = SimpleOscillator::SINE;
    bool is_osc_1_waveform_pulse = false;
    bool is_osc_2_waveform_pulse = false;
    bool need_osc_2_high_pass = false;

    osc_1_harmonics.fill(0.0);
    osc_2_harmonics.fill(0.0);

    if (is_stereo) {
        randomize_waveform(
            !is_mix, osc_1_waveform, osc_1_harmonics, is_osc_1_waveform_pulse
        );
        osc_2_waveform = osc_1_waveform;
        osc_2_harmonics = osc_1_harmonics;
        is_osc_2_waveform_pulse = is_osc_1_waveform_pulse;
    } else {
        if (is_osc_1_active) {
            randomize_waveform(
                !is_mix,
                osc_1_waveform,
                osc_1_harmonics,
                is_osc_1_waveform_pulse
            );
        }

        if (is_osc_2_active) {
            randomize_waveform(
                false, osc_2_waveform, osc_2_harmonics, is_osc_2_waveform_pulse
            );
        }
    }

    /*
    When the two oscillators are mixed, avoid using each other's inverted
    waveforms.
    */

    if (
            (is_osc_1_active && is_osc_2_active && is_mix)
            && (
                (
                    osc_1_waveform == SimpleOscillator::SAWTOOTH
                    && osc_2_waveform == SimpleOscillator::INVERSE_SAWTOOTH
                )
                || (
                    osc_1_waveform == SimpleOscillator::SOFT_SAWTOOTH
                    && osc_2_waveform == SimpleOscillator::SOFT_INVERSE_SAWTOOTH
                )
                || (
                    osc_1_waveform == SimpleOscillator::INVERSE_SAWTOOTH
                    && osc_2_waveform == SimpleOscillator::SAWTOOTH
                )
                || (
                    osc_1_waveform == SimpleOscillator::SOFT_INVERSE_SAWTOOTH
                    && osc_2_waveform == SimpleOscillator::SOFT_SAWTOOTH
                )
            )
    ) {
        osc_2_waveform = osc_1_waveform;
    }

    /* Apply the waveforms. */

    if (is_osc_1_active) {
        synth.modulator_params.waveform.set_value(osc_1_waveform);

        if (osc_1_waveform == SimpleOscillator::CUSTOM) {
            synth.modulator_params.harmonic_0.set_value(osc_1_harmonics[0]);
            synth.modulator_params.harmonic_1.set_value(osc_1_harmonics[1]);
            synth.modulator_params.harmonic_2.set_value(osc_1_harmonics[2]);
            synth.modulator_params.harmonic_3.set_value(osc_1_harmonics[3]);
            synth.modulator_params.harmonic_4.set_value(osc_1_harmonics[4]);
            synth.modulator_params.harmonic_5.set_value(osc_1_harmonics[5]);
            synth.modulator_params.harmonic_6.set_value(osc_1_harmonics[6]);
            synth.modulator_params.harmonic_7.set_value(osc_1_harmonics[7]);
            synth.modulator_params.harmonic_8.set_value(osc_1_harmonics[8]);
            synth.modulator_params.harmonic_9.set_value(osc_1_harmonics[9]);

            no_filter_probability += 0.15;
        } else if (osc_1_waveform == SimpleOscillator::SINE) {
            no_filter_probability += 0.25;
        }
    }

    if (is_osc_2_active) {
        synth.carrier_params.waveform.set_value(osc_2_waveform);

        if (osc_2_waveform == SimpleOscillator::CUSTOM) {
            synth.carrier_params.harmonic_0.set_value(osc_2_harmonics[0]);
            synth.carrier_params.harmonic_1.set_value(osc_2_harmonics[1]);
            synth.carrier_params.harmonic_2.set_value(osc_2_harmonics[2]);
            synth.carrier_params.harmonic_3.set_value(osc_2_harmonics[3]);
            synth.carrier_params.harmonic_4.set_value(osc_2_harmonics[4]);
            synth.carrier_params.harmonic_5.set_value(osc_2_harmonics[5]);
            synth.carrier_params.harmonic_6.set_value(osc_2_harmonics[6]);
            synth.carrier_params.harmonic_7.set_value(osc_2_harmonics[7]);
            synth.carrier_params.harmonic_8.set_value(osc_2_harmonics[8]);
            synth.carrier_params.harmonic_9.set_value(osc_2_harmonics[9]);

            no_filter_probability += 0.15;
        } else if (osc_2_waveform == SimpleOscillator::SINE) {
            no_filter_probability += 0.25;
        }
    }

    /*
    Oscillator 1 (and in case of a stereo patch, oscillator 2) pulse width.
    */

    if (is_osc_1_active && is_osc_1_waveform_pulse) {
        Synth::ControllerId controller_id = (
            Synth::ControllerId::INVALID_CONTROLLER_ID
        );

        if (rng.random() < 0.5) {
            controller_id = Synth::ControllerId::LFO_5;
            randomize_pwm_lfo(*synth.lfos[4]);
        } else {
            EnvelopeDescriptor const& envelope_descriptor = *next_envelope++;
            controller_id = std::get<0>(envelope_descriptor);
            randomize_pwm_envelope(*std::get<2>(envelope_descriptor));
        }

        if (rng.random() < 0.5) {
            synth.modulator_params.filter_1_type.set_value(
                SimpleBiquadFilter::HIGH_PASS
            );
            synth.modulator_params.filter_1_frequency.set_value(10.0);
            synth.modulator_params.filter_1_q.set_value(0.0);

            need_osc_2_high_pass = is_stereo;
        }

        assign_controller(Synth::ParamId::MPW, controller_id);

        assign_controller_if(is_stereo, Synth::ParamId::CPW, controller_id);
    }

    /* Oscillator 2 pulse width, if not already set up. */

    if (is_osc_2_active && is_osc_2_waveform_pulse && !is_stereo) {
        if (rng.random() < 0.5) {
            randomize_pwm_lfo(*synth.lfos[5]);
            assign_controller(Synth::ParamId::CPW, Synth::ControllerId::LFO_6);
        } else {
            EnvelopeDescriptor const& envelope_descriptor = *next_envelope++;
            randomize_pwm_envelope(*std::get<2>(envelope_descriptor));
            assign_controller(
                Synth::ParamId::CPW, std::get<0>(envelope_descriptor)
            );
        }

        need_osc_2_high_pass = need_osc_2_high_pass || rng.random() < 0.5;
    }

    if (need_osc_2_high_pass) {
        synth.carrier_params.filter_1_type.set_value(
            SimpleBiquadFilter::HIGH_PASS
        );
        synth.carrier_params.filter_1_frequency.set_value(10.0);
        synth.carrier_params.filter_1_q.set_value(0.0);
    }
}


void RandomPatchGenerator::randomize_waveform(
        bool const soft_only,
        Byte& waveform,
        Harmonics& harmonics,
        bool& is_pulse
) noexcept {
    std::array<Byte, (size_t)SimpleOscillator::WAVEFORMS> const waveforms{
        SimpleOscillator::SINE,
        SimpleOscillator::SAWTOOTH,
        SimpleOscillator::SOFT_SAWTOOTH,
        SimpleOscillator::INVERSE_SAWTOOTH,
        SimpleOscillator::SOFT_INVERSE_SAWTOOTH,
        SimpleOscillator::TRIANGLE,
        SimpleOscillator::SOFT_TRIANGLE,
        SimpleOscillator::SQUARE,
        SimpleOscillator::SOFT_SQUARE,
        SimpleOscillator::PULSE,
        SimpleOscillator::SOFT_PULSE,
        SimpleOscillator::BIPOLAR_PULSE,
        SimpleOscillator::SOFT_BIPOLAR_PULSE,
    };
    std::array<Byte, 7> const soft_waveforms{
        SimpleOscillator::SINE,
        SimpleOscillator::SOFT_SAWTOOTH,
        SimpleOscillator::SOFT_INVERSE_SAWTOOTH,
        SimpleOscillator::SOFT_TRIANGLE,
        SimpleOscillator::SOFT_SQUARE,
        SimpleOscillator::SOFT_PULSE,
        SimpleOscillator::SOFT_BIPOLAR_PULSE,
    };

    if (rng.random() < 0.2) {
        /*
        Custom waveform with some harmonics set to 0, others to random values.
        Higher harmonics get smaller amplitudes. Overall sum is normalized to 1.
        */

        Integer mask = (Integer)std::floor(rng.random(3, 1024)) | 1;
        Number scale = 1.0;
        Number norm;
        Harmonics::iterator it = harmonics.begin();
        waveform = SimpleOscillator::CUSTOM;
        is_pulse = false;

        *it = rng.random(0.5, 1.0);
        norm = *it;
        ++it;

        for (; it != harmonics.end(); ++it) {
            *it = (mask & 1) == 1 ? rng.random(-1.0 * scale, 1.0 * scale) : 0.0;
            norm += std::fabs(*it);
            scale *= rng.random(0.7, 0.99);
            mask >>= 1;
        }

        for (it = harmonics.begin(); it != harmonics.end(); ++it) {
            *it /= norm;
        }

        return;
    }

    /* Random waveform. */

    if (soft_only) {
        waveform = rng.random_choice<Byte>(soft_waveforms);
        is_pulse = (
            waveform == SimpleOscillator::SOFT_PULSE
            || waveform == SimpleOscillator::SOFT_BIPOLAR_PULSE
        );
    } else {
        waveform = rng.random_choice<Byte>(waveforms);
        is_pulse = (
            waveform == SimpleOscillator::PULSE
            || waveform == SimpleOscillator::SOFT_PULSE
            || waveform == SimpleOscillator::BIPOLAR_PULSE
            || waveform == SimpleOscillator::SOFT_BIPOLAR_PULSE
        );
    }
}


void RandomPatchGenerator::randomize_pwm_lfo(LFO& lfo) noexcept
{
    randomize_lfo_waveform(lfo);
    lfo.frequency.set_value(rng.random(0.03, 0.50));
    lfo.min.set_value(rng.random(0.03, 0.20));
    lfo.max.set_value(rng.random(0.30, 0.97));

    if (rng.random() < 0.5) {
        lfo.distortion.set_value(rng.random(0.1, 0.5));
    }
}


void RandomPatchGenerator::randomize_lfo_waveform(LFO& lfo) noexcept
{
    lfo.waveform.set_value(
        rng.random_choice<Byte, 6>(
            {
                SimpleOscillator::SINE,
                SimpleOscillator::SOFT_SAWTOOTH,
                SimpleOscillator::SOFT_INVERSE_SAWTOOTH,
                SimpleOscillator::TRIANGLE,
                SimpleOscillator::SOFT_TRIANGLE,
                SimpleOscillator::SOFT_SQUARE,
            }
        )
    );
}


void RandomPatchGenerator::randomize_pwm_envelope(Envelope& envelope) noexcept
{
    randomize_envelope_shape(envelope);
    envelope.initial_value.set_ratio(rng.random(0.03, 0.97));

    if (rng.random() < 0.5) {
        envelope.delay_time.set_ratio(rng.random(0.0, 0.5));
    }

    envelope.attack_time.set_ratio(rng.random());
    envelope.peak_value.set_ratio(rng.random(0.03, 0.97));
    envelope.hold_time.set_ratio(rng.random(0.0, 0.1));
    envelope.decay_time.set_ratio(rng.random());
    envelope.sustain_value.set_ratio(rng.random(0.03, 0.97));
    envelope.release_time.set_ratio(rng.random(0.1, 1.0));
    envelope.final_value.set_ratio(rng.random(0.03, 0.97));
}


void RandomPatchGenerator::set_up_inaccuracy() noexcept
{
    if (is_stereo) {
        /*
        Use instability and inaccuracy to slightly detune the two oscillators.
        */

        synth.modulator_params.oscillator_inaccuracy.set_ratio(
            rng.random(0.0, 0.55)
        );
        synth.modulator_params.oscillator_instability.set_ratio(
            rng.random(0.0, 0.55)
        );
        synth.carrier_params.oscillator_inaccuracy.set_ratio(
            rng.random(0.0, 0.55)
        );
        synth.carrier_params.oscillator_instability.set_ratio(
            rng.random(0.0, 0.55)
        );

        return;
    }

    if (rng.random() > 0.3) {
        return;
    }

    /* Random inaccuracy and instability. */

    if (is_osc_1_active) {
        synth.modulator_params.oscillator_inaccuracy.set_ratio(
            rng.random(0.0, 0.3)
        );
        synth.modulator_params.oscillator_instability.set_ratio(
            rng.random(0.0, 0.3)
        );
    }

    if (is_osc_2_active) {
        synth.carrier_params.oscillator_inaccuracy.set_ratio(
            rng.random(0.0, 0.3)
        );
        synth.carrier_params.oscillator_instability.set_ratio(
            rng.random(0.0, 0.3)
        );
    }
}


void RandomPatchGenerator::set_up_detune() noexcept
{
    if (is_pm || is_fm || is_am) {
        /* Random, mostly consonant detune. */

        synth.modulator_params.detune.set_value(
            100.0 * rng.random_choice<Number, 25>(
                {
                    -24.0, -21.0, -19.0, -17.0, -15.0,
                    -12.0, -9.0, -7.0, -6.0, -5.0, -3.0, -2.0,
                    0.0,
                    2.0, 3.0, 5.0, 6.0, 7.0, 9.0, 12.0,
                    15.0, 17.0, 19.0, 21.0, 24.0,
                }
            )
        );

        return;
    }

    if (
            is_mix
            && is_osc_1_active
            && is_osc_2_active
            && !is_stereo
            && rng.random() < 0.5
    ) {
        /* Octave detune. */

        Number const detune = 100.0 * rng.random_choice<Number, 5>(
            {-24.0, -12.0, 0.0, 12.0, 24.0}
        );

        if (rng.random() < 0.5) {
            synth.modulator_params.detune.set_value(detune);
        } else {
            synth.carrier_params.detune.set_value(detune);
        }
    }
}


void RandomPatchGenerator::set_up_pitch_wheel_without_vibrato() noexcept
{
    Number const range = pick_random_pitch_wheel_range();

    if (Math::is_close(range, 1200.0)) {
        /* Full octave bends, simply assign the pitch wheel directly. */

        assign_controller_if(
            is_osc_1_active,
            Synth::ParamId::MFIN,
            Synth::ControllerId::PITCH_WHEEL
        );
        assign_controller_if(
            is_osc_2_active,
            Synth::ParamId::CFIN,
            Synth::ControllerId::PITCH_WHEEL
        );

        return;
    }

    MacroDescriptor const& macro_descriptor = *next_macro++;

    Synth::ControllerId const macro_ctl_id = std::get<0>(macro_descriptor);
    Synth::ParamId const macro_input_param_id = std::get<1>(macro_descriptor);
    Macro& macro = *std::get<2>(macro_descriptor);

    bool need_fine_detune_x4 = false;

    if (range < 0.000001) {
        /* Bend 3 octaves down, 1 octave up. */

        need_fine_detune_x4 = true;

        macro.midpoint.set_value(0.75);
        macro.min.set_value(0.125);
        macro.max.set_value(0.625);
    } else {
        /* Bend the chosen number of cents in both directions. */

        need_fine_detune_x4 = range >= 1200.0;

        Number const fine_detune_range = need_fine_detune_x4 ? 9600.0 : 2400.0;
        Number const delta = range / fine_detune_range;

        macro.min.set_value(0.5 - delta);
        macro.max.set_value(0.5 + delta);
    }

    if (need_fine_detune_x4) {
        set_param_ratio_if(is_osc_1_active, Synth::ParamId::MFX4, 1.0);
        set_param_ratio_if(is_osc_2_active, Synth::ParamId::CFX4, 1.0);
    }

    assign_controller(macro_input_param_id, Synth::ControllerId::PITCH_WHEEL);

    assign_controller_if(is_osc_1_active, Synth::ParamId::MFIN, macro_ctl_id);
    assign_controller_if(is_osc_2_active, Synth::ParamId::CFIN, macro_ctl_id);
}


Number RandomPatchGenerator::pick_random_pitch_wheel_range() noexcept
{
    return rng.random_choice<Number, 9>(
        {0.0, 0.0, 200.0, 200.0, 700.0, 700.0, 700.0, 1200.0, 2400.0}
    );
}


void RandomPatchGenerator::set_up_vibrato() noexcept
{
    Synth::ControllerId strength_ctl_id = pick_random_midi_controller(0.5);

    LFO& lfo = *synth.lfos[6];

    lfo.tempo_sync.set_value(ToggleParam::ON);
    lfo.center.set_value(ToggleParam::ON);
    lfo.amplitude_envelope.set_value(11);

    if (
            strength_ctl_id == Synth::ControllerId::INVALID_CONTROLLER_ID
            || rng.random() < 0.5
    ) {
        /* Fixed speed. */

        lfo.frequency.set_value(
            rng.random_choice<Number, 5>({0.5, 1.0, 1.5, 2.0, 3.0})
        );
    } else {
        /* Speed will increase with strength. */

        MacroDescriptor const& macro_descriptor = *next_macro++;

        Synth::ControllerId const macro_ctl_id = std::get<0>(macro_descriptor);
        Synth::ParamId const macro_input_param_id = std::get<1>(
            macro_descriptor
        );
        Macro& macro = *std::get<2>(macro_descriptor);

        macro.min.set_value(0.0497);
        macro.max.set_value(0.1497);

        assign_controller(macro_input_param_id, strength_ctl_id);
        assign_controller(Synth::ParamId::L7FRQ, macro_ctl_id);
    }

    Envelope& envelope = *synth.envelopes[11];

    envelope.update_mode.set_value(Envelope::UPDATE_MODE_DYNAMIC);
    envelope.initial_value.set_value(1.0);
    envelope.peak_value.set_value(1.0);
    envelope.sustain_value.set_value(1.0);
    envelope.final_value.set_value(1.0);

    if (strength_ctl_id != Synth::ControllerId::INVALID_CONTROLLER_ID) {
        assign_controller(Synth::ParamId::N12SCL, strength_ctl_id);
    } else {
        lfo.amplitude.set_value(rng.random(0.05, 0.2));
    }

    MacroDescriptor const& min_macro_descriptor = *next_macro++;
    MacroDescriptor const& max_macro_descriptor = *next_macro++;

    Synth::ControllerId const min_macro_ctl_id = std::get<0>(
        min_macro_descriptor
    );
    Synth::ParamId const min_macro_input_param_id = std::get<1>(
        min_macro_descriptor
    );
    Macro& min_macro = *std::get<2>(min_macro_descriptor);

    Synth::ControllerId const max_macro_ctl_id = std::get<0>(
        max_macro_descriptor
    );
    Synth::ParamId const max_macro_input_param_id = std::get<1>(
        max_macro_descriptor
    );
    Macro& max_macro = *std::get<2>(max_macro_descriptor);

    Number const pitch_wheel_range = pick_random_pitch_wheel_range();
    bool need_fine_detune_x4 = false;

    if (pitch_wheel_range < 0.000001) {
        /* Pitch wheel will bend 3 octaves down, 1 octave up. */

        need_fine_detune_x4 = true;

        min_macro.midpoint.set_value(0.75);
        min_macro.min.set_value(0.115);
        min_macro.max.set_value(0.615);

        max_macro.midpoint.set_value(0.75);
        max_macro.min.set_value(0.135);
        max_macro.max.set_value(0.635);
    } else {
        /*
        Pitch wheel will bend the chosen number of cents in both directions.
        */

        need_fine_detune_x4 = pitch_wheel_range >= 1200.0;

        Number const lfo_delta = need_fine_detune_x4 ? 0.01 : 0.03;
        Number const fine_detune_range = need_fine_detune_x4 ? 9600.0 : 2400.0;
        Number const macro_delta = pitch_wheel_range / fine_detune_range;

        min_macro.min.set_value(0.5 - macro_delta - lfo_delta);
        min_macro.max.set_value(0.5 + macro_delta - lfo_delta);

        max_macro.min.set_value(0.5 - macro_delta + lfo_delta);
        max_macro.max.set_value(0.5 + macro_delta + lfo_delta);
    }

    if (need_fine_detune_x4) {
        set_param_ratio_if(is_osc_1_active, Synth::ParamId::MFX4, 1.0);
        set_param_ratio_if(is_osc_2_active, Synth::ParamId::CFX4, 1.0);
    }

    assign_controller(
        min_macro_input_param_id, Synth::ControllerId::PITCH_WHEEL
    );
    assign_controller(
        max_macro_input_param_id, Synth::ControllerId::PITCH_WHEEL
    );
    assign_controller(Synth::ParamId::L7MIN, min_macro_ctl_id);
    assign_controller(Synth::ParamId::L7MAX, max_macro_ctl_id);

    assign_controller_if(
        is_osc_1_active, Synth::ParamId::MFIN, Synth::ControllerId::LFO_7
    );
    assign_controller_if(
        is_osc_2_active, Synth::ParamId::CFIN, Synth::ControllerId::LFO_7
    );
}


void RandomPatchGenerator::set_up_waveshaper() noexcept
{
    if ((is_pm || is_fm) && rng.random() < 0.7) {
        return;
    }

    if (is_osc_2_active && !is_stereo && !is_pm && !is_fm) {
        /* Oscillator 2 distortion: MIDI-controlled or fixed random value. */

        Synth::ControllerId dist_ctl_id;
        Number dist_level;

        randomize_waveshaper(0.5, dist_level, dist_ctl_id);

        if (dist_ctl_id != Synth::ControllerId::INVALID_CONTROLLER_ID) {
            synth.carrier_params.distortion_type.set_value(
                pick_random_distortion_type()
            );
            assign_controller(Synth::ParamId::CDL, dist_ctl_id);
        } else {
            synth.carrier_params.distortion.set_value(dist_level);
        }
    }

    /* Oscillator 1 and 2 folding: MIDI-controlled or fixed random value. */

    Synth::ControllerId fold_ctl_id;
    Number fold_level;

    randomize_waveshaper(0.5, fold_level, fold_ctl_id);

    if (fold_ctl_id != Synth::ControllerId::INVALID_CONTROLLER_ID) {
        assign_controller_if(
            is_osc_1_active && is_mix, Synth::ParamId::MFLD, fold_ctl_id
        );
        assign_controller_if(
            is_osc_2_active, Synth::ParamId::CFLD, fold_ctl_id
        );
    } else {
        set_param_ratio_if(
            is_osc_1_active && is_mix, Synth::ParamId::MFLD, fold_level
        );
        set_param_ratio_if(is_osc_2_active, Synth::ParamId::CFLD, fold_level);
    }
}


void RandomPatchGenerator::randomize_waveshaper(
        Number const no_op_probability,
        Number& level,
        Synth::ControllerId& ctl_id
) noexcept {
    level = 0.0;
    ctl_id = Synth::ControllerId::INVALID_CONTROLLER_ID;

    if (rng.random() < no_op_probability) {
        return;
    }

    Number const choice = rng.random();

    if (choice < 0.2) {
        /* Fixed random level. */

        level = rng.random(0.1, 1.0);

        return;
    }

    if (choice < 0.7) {
        /* MIDI-controlled waveshaping level. */

        Synth::ControllerId const macro_input_ctl_id = (
            pick_random_midi_controller(
                0.5, Synth::ControllerId::TRIGGERED_VELOCITY
            )
        );
        MacroDescriptor const& macro_descriptor = *next_macro++;

        Synth::ControllerId const macro_ctl_id = std::get<0>(macro_descriptor);
        Synth::ParamId const macro_input_param_id = std::get<1>(
            macro_descriptor
        );
        Macro& macro = *std::get<2>(macro_descriptor);

        Number const min = rng.random(0.05, 0.15);

        macro.min.set_value(min);
        macro.max.set_value(rng.random(min + 0.2, 1.0));

        assign_controller(macro_input_param_id, macro_input_ctl_id);

        ctl_id = macro_ctl_id;

        return;
    }

    /* Envelope-generator-controlled waveshaping level. */

    EnvelopeDescriptor const& envelope_descriptor = *next_envelope++;
    Synth::ControllerId envelope_ctl_id = std::get<0>(envelope_descriptor);
    Envelope& envelope = *std::get<2>(envelope_descriptor);

    randomize_envelope_shape(envelope);
    envelope.initial_value.set_ratio(rng.random(0.1, 1.0));

    if (rng.random() < 0.5) {
        envelope.delay_time.set_ratio(rng.random(0.0, 0.2));
    }

    envelope.attack_time.set_ratio(rng.random());
    envelope.peak_value.set_ratio(rng.random(0.2, 1.0));
    envelope.hold_time.set_ratio(rng.random(0.0, 0.1));
    envelope.decay_time.set_ratio(rng.random());
    envelope.sustain_value.set_ratio(rng.random(0.1, 1.0));
    envelope.release_time.set_ratio(rng.random(0.1, 1.0));
    envelope.final_value.set_ratio(rng.random(0.1, 1.0));

    if (rng.random() < 0.3) {
        Synth::ParamId const scale_param_id = std::get<1>(envelope_descriptor);
        assign_controller(
            scale_param_id, Synth::ControllerId::TRIGGERED_VELOCITY
        );
    }

    ctl_id = envelope_ctl_id;
}


Byte RandomPatchGenerator::pick_random_distortion_type() noexcept
{
    return rng.random_choice<Byte, 14>(
        {
            Distortion::TYPE_TANH_3,
            Distortion::TYPE_TANH_5,
            Distortion::TYPE_TANH_10,
            Distortion::TYPE_SIN,
            Distortion::TYPE_SQRT,
            Distortion::TYPE_CBRT,
            Distortion::TYPE_HARMONIC_13,
            Distortion::TYPE_HARMONIC_15,
            Distortion::TYPE_HARMONIC_135,
            Distortion::TYPE_HARMONIC_SQR,
            Distortion::TYPE_HARMONIC_TRI,
            Distortion::TYPE_BIT_CRUSH_1,
            Distortion::TYPE_BIT_CRUSH_2,
            Distortion::TYPE_BIT_CRUSH_3,
        }
    );
}


void RandomPatchGenerator::set_up_filter() noexcept
{
    Number q;
    Synth::ControllerId freq_ctl_id;

    randomize_filter(q, freq_ctl_id);

    if (
            is_osc_1_active
            && is_mix
            && freq_ctl_id != Synth::ControllerId::INVALID_CONTROLLER_ID
    ) {
        synth.modulator_params.filter_2_q.set_value(q);
        assign_controller(Synth::ParamId::MF2FRQ, freq_ctl_id);

        if (is_osc_2_active && !is_stereo) {
            /* Use different settings for Oscillator 2. */

            randomize_filter(q, freq_ctl_id);
        }
    }

    if (
            is_osc_2_active
            && freq_ctl_id != Synth::ControllerId::INVALID_CONTROLLER_ID
    ) {
        synth.carrier_params.filter_2_q.set_value(q);
        assign_controller(Synth::ParamId::CF2FRQ, freq_ctl_id);
    }
}


void RandomPatchGenerator::randomize_filter(
        Number& q,
        Synth::ControllerId& freq_ctl_id
) noexcept {
    q = Constants::BIQUAD_FILTER_Q_DEFAULT;
    freq_ctl_id = Synth::ControllerId::INVALID_CONTROLLER_ID;

    if (rng.random() < no_filter_probability) {
        return;
    }

    if (rng.random() < 0.7) {
        q = rng.random(0.0, 15.0);
    } else {
        q = 0.0;
    }

    Number const choice = rng.random();

    if (!is_lfo_8_used && choice < 0.2) {
        /* LFO 8 will control the filter. */

        LFO& lfo = *synth.lfos[7];

        is_lfo_8_used = true;

        randomize_lfo_waveform(lfo);
        lfo.frequency.set_value(rng.random(0.02, 0.80));
        lfo.min.set_value(rng.random(0.60, 0.72));
        lfo.max.set_value(rng.random(0.77, 0.99));

        if (rng.random() < 0.5) {
            lfo.distortion.set_value(rng.random(0.1, 0.5));
        }

        freq_ctl_id = Synth::ControllerId::LFO_8;

        return;
    }

    if (choice < 0.5) {
        /* MIDI-controlled filter. */

        MacroDescriptor const& macro_descriptor = *next_macro++;

        Synth::ControllerId const macro_ctl_id = std::get<0>(macro_descriptor);
        Synth::ParamId const macro_input_param_id = std::get<1>(
            macro_descriptor
        );
        Macro& macro = *std::get<2>(macro_descriptor);
        Number const min_value = rng.random(0.60, 0.75);

        macro.min.set_value(min_value);
        macro.max.set_value(rng.random(min_value + 0.1, 1.0));

        if (rng.random() < 0.5) {
            macro.distortion.set_value(rng.random(0.0, 0.3));
        }

        assign_controller(
            macro_input_param_id,
            pick_random_midi_controller(
                0.5, Synth::ControllerId::TRIGGERED_VELOCITY
            )
        );

        freq_ctl_id = macro_ctl_id;

        return;
    }

    /* Envelope-generator-controlled filter. */

    EnvelopeDescriptor const& envelope_descriptor = *next_envelope++;
    Synth::ControllerId envelope_ctl_id = std::get<0>(envelope_descriptor);
    Envelope& envelope = *std::get<2>(envelope_descriptor);

    randomize_envelope_shape(envelope);

    if (rng.random() < 0.5) {
        envelope.delay_time.set_ratio(rng.random(0.0, 0.2));
    }

    envelope.attack_time.set_ratio(rng.random());
    envelope.hold_time.set_ratio(rng.random(0.0, 0.3));
    envelope.decay_time.set_ratio(rng.random());
    envelope.release_time.set_ratio(rng.random(0.1, 1.0));

    freq_ctl_id = envelope_ctl_id;

    if (choice < 0.7) {
        /* Random levels. */

        envelope.initial_value.set_ratio(rng.random(0.6, 1.0));
        envelope.peak_value.set_ratio(rng.random(0.6, 1.0));
        envelope.sustain_value.set_ratio(rng.random(0.6, 1.0));
        envelope.final_value.set_ratio(rng.random(0.6, 1.0));

        return;
    }

    /*
    Key tracking: envelope levels will react to the triggered note and its
    velocity in various ways.
    */

    Synth::ParamId const env_scale_param_id = std::get<1>(envelope_descriptor);

    Synth::ParamId const env_init_param_id = (
        (Synth::ParamId)((int)env_scale_param_id + 1)
    );
    Synth::ParamId const env_peak_param_id = (
        (Synth::ParamId)((int)env_scale_param_id + 4)
    );
    Synth::ParamId const env_sustain_param_id = (
        (Synth::ParamId)((int)env_scale_param_id + 7)
    );
    Synth::ParamId const env_final_param_id = (
        (Synth::ParamId)((int)env_scale_param_id + 9)
    );

    MacroDescriptor const& init_macro_descriptor = *next_macro++;
    MacroDescriptor const& peak_macro_descriptor = *next_macro++;
    MacroDescriptor const& sustain_macro_descriptor = *next_macro++;

    Synth::ControllerId const init_macro_ctl_id = std::get<0>(
        init_macro_descriptor
    );
    Synth::ControllerId const peak_macro_ctl_id = std::get<0>(
        peak_macro_descriptor
    );
    Synth::ControllerId const sustain_macro_ctl_id = std::get<0>(
        sustain_macro_descriptor
    );

    Synth::ParamId const init_macro_input_param_id = std::get<1>(
        init_macro_descriptor
    );
    Synth::ParamId const peak_macro_input_param_id = std::get<1>(
        peak_macro_descriptor
    );
    Synth::ParamId const sustain_macro_input_param_id = std::get<1>(
        sustain_macro_descriptor
    );

    Synth::ParamId const init_macro_scale_param_id = (
        (Synth::ParamId)((int)init_macro_input_param_id + 3)
    );
    Synth::ParamId const peak_macro_scale_param_id = (
        (Synth::ParamId)((int)peak_macro_input_param_id + 3)
    );
    Synth::ParamId const sustain_macro_scale_param_id = (
        (Synth::ParamId)((int)sustain_macro_input_param_id + 3)
    );

    Macro& init_macro = *std::get<2>(init_macro_descriptor);
    Macro& peak_macro = *std::get<2>(peak_macro_descriptor);
    Macro& sustain_macro = *std::get<2>(sustain_macro_descriptor);

    init_macro.min.set_value(rng.random(0.6, 1.0));
    init_macro.max.set_value(rng.random(0.6, 1.0));

    if (rng.random() < 0.5) {
        init_macro.distortion.set_value(rng.random(0.0, 0.3));
    }

    peak_macro.min.set_value(rng.random(0.6, 1.0));
    peak_macro.max.set_value(rng.random(0.6, 1.0));

    if (rng.random() < 0.5) {
        peak_macro.distortion.set_value(rng.random(0.0, 0.3));
    }

    sustain_macro.min.set_value(rng.random(0.6, 1.0));
    sustain_macro.max.set_value(rng.random(0.6, 1.0));

    if (rng.random() < 0.5) {
        sustain_macro.distortion.set_value(rng.random(0.0, 0.3));
    }

    assign_controller(
        init_macro_input_param_id, Synth::ControllerId::TRIGGERED_NOTE
    );
    assign_controller(
        peak_macro_input_param_id, Synth::ControllerId::TRIGGERED_NOTE
    );
    assign_controller(
        sustain_macro_input_param_id, Synth::ControllerId::TRIGGERED_NOTE
    );

    assign_controller(
        init_macro_scale_param_id, Synth::ControllerId::TRIGGERED_VELOCITY
    );
    assign_controller(
        peak_macro_scale_param_id, Synth::ControllerId::TRIGGERED_VELOCITY
    );
    assign_controller(
        sustain_macro_scale_param_id, Synth::ControllerId::TRIGGERED_VELOCITY
    );

    assign_controller(env_init_param_id, init_macro_ctl_id);
    assign_controller(env_peak_param_id, peak_macro_ctl_id);
    assign_controller(env_sustain_param_id, sustain_macro_ctl_id);
    assign_controller(env_final_param_id, init_macro_ctl_id);
}


void RandomPatchGenerator::set_up_volume() noexcept
{
    if (rng.random() < 0.5) {
        /* Randomize velocity sensitivity. */

        Number const vel_sens = rng.random_normal();

        set_param_ratio_if(is_osc_1_active, Synth::ParamId::MVS, vel_sens);
        set_param_ratio_if(is_osc_2_active, Synth::ParamId::CVS, vel_sens);
    }

    /*
    When modulating, the volume of oscillator 1 will be a fixed random value.
    Then, unless it's a decaying sound, the sounding oscillator(s) may randomly
    get a rhythmic volume or tremolo effect (which may be triggered via MIDI).
    */

    Number volume;
    Synth::ControllerId ctl_id;

    randomize_volume(volume, ctl_id);

    if (is_osc_1_active) {
        if (is_pm || is_fm || is_am) {
            synth.modulator_params.volume.set_value(rng.random(0.2, 1.0));
        } else {
            if (ctl_id != Synth::ControllerId::INVALID_CONTROLLER_ID) {
                assign_controller(Synth::ParamId::MVOL, ctl_id);
            } else {
                synth.modulator_params.volume.set_value(volume);
            }
        }
    }

    if (is_osc_2_active) {
        if (ctl_id != Synth::ControllerId::INVALID_CONTROLLER_ID) {
            assign_controller(Synth::ParamId::CVOL, ctl_id);
        } else {
            synth.carrier_params.volume.set_value(volume);
        }
    }
}


void RandomPatchGenerator::randomize_volume(
        Number& volume,
        Synth::ControllerId& ctl_id
) noexcept {
    volume = is_mix ? 0.33 : 0.50;
    ctl_id = Synth::ControllerId::INVALID_CONTROLLER_ID;

    if (is_decaying) {
        return;
    }

    Number const choice = rng.random();

    if (choice < 0.5) {
        return;
    }

    ctl_id = Synth::ControllerId::LFO_1;

    can_use_reverse_echo = true;

    LFO& lfo_1 = *synth.lfos[0];

    lfo_1.tempo_sync.set_value(ToggleParam::ON);
    lfo_1.amplitude_envelope.set_value(10);

    Envelope& envelope = *synth.envelopes[10];

    envelope.update_mode.set_value(Envelope::UPDATE_MODE_DYNAMIC);
    envelope.initial_value.set_value(1.0);
    envelope.peak_value.set_value(1.0);
    envelope.sustain_value.set_value(1.0);
    envelope.final_value.set_value(1.0);

    if (rng.random() < 0.7) {
        Synth::ControllerId const env_scale_ctl_id = (
            pick_random_midi_controller(0.5)
        );

        assign_controller_if(
            env_scale_ctl_id != Synth::ControllerId::INVALID_CONTROLLER_ID,
            Synth::ParamId::N11SCL,
            env_scale_ctl_id
        );
    }

    lfo_1.phase.set_value(180.0 / 360.0);
    lfo_1.max.set_value(0.0);

    if (choice < 0.75) {
        /*
        Use a basic waveform as a periodically repeating envelope generator.
        */

        lfo_1.waveform.set_value(
            rng.random() < 0.3
                ?  rng.random_choice<Byte, 3>(
                    {
                        SimpleOscillator::SINE,
                        SimpleOscillator::INVERSE_SAWTOOTH,
                        SimpleOscillator::SOFT_INVERSE_SAWTOOTH,
                    }
                )
                : rng.random_choice<Byte, 2>(
                    {
                        SimpleOscillator::SAWTOOTH,
                        SimpleOscillator::SOFT_SAWTOOTH,
                    }
                )
        );

        lfo_1.frequency.set_value(
            rng.random_choice<Number, 5>({1.0, 4.0 / 3.0, 1.5, 2.0, 3.0})
        );
        lfo_1.min.set_value(volume);

        if (rng.random() < 0.5) {
            lfo_1.distortion.set_value(rng.random(0.1, 0.5));
        }

        return;
    }

    /*
    Combine pulse LFOs to create a tremolo effect with random rhythm patterns.
    */

    lfo_1.waveform.set_value(SimpleOscillator::SOFT_PULSE);
    lfo_1.frequency.set_value(
        rng.random_choice<Number, 9>(
            {1.0, 4.0 / 3.0, 1.5, 2.0, 3.0, 3.5, 4.0, 5.0, 6.0}
        )
    );

    LFO& lfo_2 = *synth.lfos[1];
    LFO& lfo_4 = *synth.lfos[3];

    lfo_2.tempo_sync.set_value(ToggleParam::ON);
    lfo_2.amplitude_envelope.set_value(10);
    lfo_2.waveform.set_value(SimpleOscillator::SOFT_SQUARE);
    lfo_2.frequency.set_value(
        rng.random_choice<Number, 7>({1.0, 4.0 / 3.0, 1.5, 2.0, 2.5, 3.0, 3.5})
    );
    lfo_2.phase.set_value(210.0 / 360.0);
    lfo_2.min.set_value(0.0);
    lfo_2.max.set_value(0.5);

    if (rng.random() < 0.3) {
        lfo_2.distortion.set_value(rng.random(0.1, 1.0));
    }

    lfo_4.tempo_sync.set_value(ToggleParam::ON);
    lfo_4.amplitude_envelope.set_value(10);
    lfo_4.waveform.set_value(SimpleOscillator::SOFT_SQUARE);
    lfo_4.frequency.set_value(1.0);
    lfo_4.phase.set_value(210.0 / 360.0);
    lfo_4.min.set_value(volume * rng.random(0.70, 0.95));
    lfo_4.max.set_value(volume);

    if (rng.random() < 0.3) {
        lfo_4.distortion.set_value(rng.random(0.1, 1.0));
    }

    assign_controller(Synth::ParamId::L1PW, Synth::ControllerId::LFO_2);
    assign_controller(Synth::ParamId::L1MIN, Synth::ControllerId::LFO_4);
}


void RandomPatchGenerator::set_up_panning() noexcept
{
    if (rng.random() < 0.5) {
        /* Randomize instrument width. */

        Number const width = rng.random_normal();

        set_param_ratio_if(
            is_mix && is_osc_1_active, Synth::ParamId::MWID, width
        );
        set_param_ratio_if(is_osc_2_active, Synth::ParamId::CWID, width);
    }

    if (is_stereo) {
        /* Separate the two oscillators randomly in the stereo field. */

        Number const delta = rng.random_normal(0.1, 0.5);
        Number const flip = rng.random_choice<Number, 2>({1.0, -1.0});

        set_param_ratio(Synth::ParamId::MPAN, 0.5 + flip * delta);
        set_param_ratio(Synth::ParamId::CPAN, 0.5 - flip * delta);
    }
}


void RandomPatchGenerator::set_up_fx_distortion() noexcept
{
    if (rng.random() > 0.3) {
        return;
    }

    Synth::ControllerId const ctl_id = pick_random_midi_controller(0.5);

    if (ctl_id != Synth::ControllerId::INVALID_CONTROLLER_ID) {
        assign_controller(Synth::ParamId::ED1L, ctl_id);
    } else {
        synth.effects.distortion_1.level.set_value(rng.random_normal());
    }

    if (rng.random() < 0.5) {
        Number const volume = 1.0 + rng.random(0.2, 0.7);

        synth.effects.volume_1_gain.set_value(volume);
        synth.effects.volume_3_gain.set_value(std::max(0.5, 1.0 / volume));
    }

    synth.effects.distortion_1_type.set_value(pick_random_distortion_type());
}


void RandomPatchGenerator::set_up_fx_filter() noexcept
{
    if (is_lfo_8_used || rng.random() >= 0.2) {
        return;
    }

    /*
    Use a filter as a phaser-like effect.
    */

    LFO& lfo_8 = *synth.lfos[7];

    lfo_8.waveform.set_value(
        rng.random_choice<Byte, 9>(
            {
                SimpleOscillator::SINE,
                SimpleOscillator::SINE,
                SimpleOscillator::SINE,
                SimpleOscillator::SAWTOOTH,
                SimpleOscillator::SOFT_SAWTOOTH,
                SimpleOscillator::INVERSE_SAWTOOTH,
                SimpleOscillator::SOFT_INVERSE_SAWTOOTH,
                SimpleOscillator::TRIANGLE,
                SimpleOscillator::SOFT_TRIANGLE,
            }
        )
    );

    lfo_8.frequency.set_value(random_normal_pow(0.01, 0.50, 2.3));
    lfo_8.min.set_value(rng.random(0.59, 0.70));
    lfo_8.max.set_value(rng.random(0.83, 0.93));

    if (rng.random() < 0.5) {
        lfo_8.distortion.set_value(rng.random_normal(0.05, 0.30));
    }

    if (rng.random() < 0.5) {
        synth.effects.filter_1_type.set_value(
            BiquadFilter<SignalProducer>::NOTCH
        );
    } else {
        synth.effects.filter_1_type.set_value(
            BiquadFilter<SignalProducer>::PEAKING
        );

        synth.effects.filter_1.gain.set_value(
            rng.random() < 0.5 ? rng.random(-15.0, -6.0) : rng.random(3.0, 9.0)
        );
    }

    synth.effects.filter_1.q.set_value(random_normal_pow(0.2, 3.5, 2.0));

    is_lfo_8_used = true;

    assign_controller(Synth::ParamId::EF1FRQ, Synth::ControllerId::LFO_8);
}


void RandomPatchGenerator::set_up_fx_tape() noexcept
{
    if (rng.random() >= 0.2) {
        return;
    }

    if (rng.random() < 0.5) {
        synth.effects.tape_at_end.set_value(ToggleParam::ON);
    }

    if (rng.random() < 0.3) {
        synth.effects.tape_params.wnf_speed.set_ratio(
            rng.random_normal(0.2, 0.7)
        );
        synth.effects.tape_params.wnf_amp.set_ratio(
            rng.random_normal(0.1, 0.5)
        );
        synth.effects.tape_params.stereo_wnf.set_ratio(
            rng.random_normal(0.0, 0.3)
        );
    }

    synth.effects.tape_params.distortion_level.set_ratio(rng.random(0.3, 1.0));
    synth.effects.tape_params.color.set_ratio(rng.random_normal(0.3, 0.7));
}


void RandomPatchGenerator::set_up_fx_chorus() noexcept
{
    if (rng.random() >= 0.3) {
        return;
    }

    synth.effects.chorus.high_pass_q.set_value(0.0);

    if (rng.random() < 0.7) {
        synth.effects.chorus.high_pass_frequency.set_value(
            rng.random(50.0, 250.0)
        );
    }

    synth.effects.chorus.type.set_value(
        rng.random_choice<Byte, 15>(
            {
                Chorus<SignalProducer>::CHORUS_1,
                Chorus<SignalProducer>::CHORUS_2,
                Chorus<SignalProducer>::CHORUS_3,
                Chorus<SignalProducer>::CHORUS_4,
                Chorus<SignalProducer>::CHORUS_5,
                Chorus<SignalProducer>::CHORUS_6,
                Chorus<SignalProducer>::CHORUS_7,
                Chorus<SignalProducer>::CHORUS_8,
                Chorus<SignalProducer>::CHORUS_9,
                Chorus<SignalProducer>::CHORUS_10,
                Chorus<SignalProducer>::CHORUS_11,
                Chorus<SignalProducer>::CHORUS_12,
                Chorus<SignalProducer>::CHORUS_13,
                Chorus<SignalProducer>::CHORUS_14,
                Chorus<SignalProducer>::CHORUS_15,
            }
        )
    );

    synth.effects.chorus.delay_time.set_value(rng.random_normal(0.01, 0.06));
    synth.effects.chorus.frequency.set_value(rng.random_normal(0.05, 0.25));
    synth.effects.chorus.depth.set_value(rng.random_normal(0.1, 0.6));

    if (rng.random() < 0.3) {
        synth.effects.chorus.damping_frequency.set_ratio(rng.random(0.7, 0.9));
        synth.effects.chorus.damping_gain.set_value(rng.random(-15.0, -3.0));
    }

    if (rng.random() < 0.3) {
        synth.effects.chorus.feedback.set_ratio(rng.random(0.1, 0.3));
    }

    synth.effects.chorus.width.set_ratio(rng.random());

    Number const wet = rng.random(0.1, 0.5);

    synth.effects.chorus.wet.set_value(wet);
    synth.effects.chorus.dry.set_value(1.0 - wet);
}


void RandomPatchGenerator::set_up_fx_echo() noexcept
{
    if (rng.random() >= 0.2) {
        return;
    }

    Number volume_scale = 1.0;
    bool emulate_tape = rng.random() < 0.3;

    synth.effects.echo.high_pass_q.set_value(0.0);

    if (rng.random() < 0.7) {
        synth.effects.echo.high_pass_frequency.set_value(
            rng.random(50.0, 250.0)
        );
    }

    if (emulate_tape || rng.random() < 0.5) {
        synth.effects.echo.damping_frequency.set_ratio(rng.random(0.7, 0.9));
        synth.effects.echo.damping_gain.set_value(rng.random(-15.0, -3.0));
    }

    synth.effects.echo.feedback.set_ratio(rng.random_normal(0.5, 0.9));

    if (emulate_tape && !is_lfo_8_used) {
        /* Tape-like wow or flutter. */

        LFO& lfo_8 = *synth.lfos[7];

        lfo_8.waveform.set_value(SimpleOscillator::SINE);
        lfo_8.frequency.set_value(random_normal_pow(0.01, 0.50, 1.8));
        lfo_8.min.set_value(0.1618);
        lfo_8.max.set_value(0.1716);
        lfo_8.amplitude.set_ratio(rng.random(0.5, 1.0));

        if (rng.random() < 0.5) {
            lfo_8.distortion.set_value(rng.random_normal(0.05, 0.30));
        }

        lfo_8.randomness.set_value(rng.random_normal(0.003, 0.015));
        lfo_8.center.set_value(ToggleParam::ON);

        is_lfo_8_used = true;

        assign_controller(Synth::ParamId::EEDEL, Synth::ControllerId::LFO_8);
    } else {
        /*
        Randomize delay time but keep it sensible for tempo synchronization.
        */

        synth.effects.echo.delay_time.set_value(
            rng.random_choice<Number, 10>(
                {0.25, 0.25, 0.33, 0.50, 0.50, 0.50, 0.50, 0.50, 0.75, 1.0}
            )
        );
    }

    if (emulate_tape || rng.random() < 0.5) {
        Number const input_volume = 1.0 + rng.random();

        volume_scale = 1.0 / input_volume;

        synth.effects.echo.input_volume.set_value(input_volume);
        synth.effects.echo.distortion_level.set_value(rng.random(0.05, 0.30));
    }

    synth.effects.echo.tempo_sync.set_value(ToggleParam::ON);

    if (can_use_reverse_echo && rng.random() < 0.5) {
        if (rng.random() < 0.5) {
            synth.effects.echo.reversed_1.set_value(ToggleParam::ON);
        }

        if (rng.random() < 0.5) {
            synth.effects.echo.reversed_2.set_value(ToggleParam::ON);
        }
    }

    if (rng.random() < 0.7) {
        synth.effects.echo.side_chain_compression_threshold.set_value(
            rng.random_choice<Number, 3>({-24.0, -21.0, -18.0})
        );
        synth.effects.echo.side_chain_compression_ratio.set_value(
            rng.random(1.2, 3.0)
        );
    }

    synth.effects.echo.width.set_ratio(rng.random());

    Number const wet = rng.random_normal(0.1, 0.5);

    synth.effects.echo.wet.set_value(wet * volume_scale);
    synth.effects.echo.dry.set_value(1.0 - wet);
}


void RandomPatchGenerator::set_up_fx_reverb() noexcept
{
    if (rng.random() >= 0.2) {
        return;
    }

    synth.effects.reverb.type.set_value(
        rng.random_choice<Byte, 10>(
            {
                Reverb<SignalProducer>::REVERB_1,
                Reverb<SignalProducer>::REVERB_2,
                Reverb<SignalProducer>::REVERB_3,
                Reverb<SignalProducer>::REVERB_4,
                Reverb<SignalProducer>::REVERB_5,
                Reverb<SignalProducer>::REVERB_6,
                Reverb<SignalProducer>::REVERB_7,
                Reverb<SignalProducer>::REVERB_8,
                Reverb<SignalProducer>::REVERB_9,
            }
        )
    );

    synth.effects.reverb.high_pass_q.set_value(0.0);

    if (rng.random() < 0.7) {
        synth.effects.reverb.high_pass_frequency.set_value(
            rng.random(50.0, 250.0)
        );
    }

    if (rng.random() < 0.5) {
        synth.effects.reverb.damping_frequency.set_ratio(rng.random(0.7, 0.9));
        synth.effects.reverb.damping_gain.set_value(rng.random(-15.0, -3.0));
    }

    synth.effects.reverb.room_reflectivity.set_ratio(
        rng.random_normal(0.70, 0.98)
    );

    if (rng.random() < 0.2) {
        /* The pitch wheel will trigger pitch changes in reverb tails. */
        MacroDescriptor const& macro_descriptor = *next_macro++;

        Synth::ControllerId const macro_ctl_id = std::get<0>(macro_descriptor);
        Synth::ParamId const macro_input_param_id = std::get<1>(
            macro_descriptor
        );
        Macro& macro = *std::get<2>(macro_descriptor);

        macro.midpoint.set_value(2.0 / 3.0);

        if (rng.random() < 0.5) {
            macro.min.set_value(2.0 / 3.0);
            macro.max.set_value(1.0 / 6.0);
        } else {
            macro.min.set_value(1.00);
            macro.max.set_value(0.25);
        }

        assign_controller(
            macro_input_param_id, Synth::ControllerId::PITCH_WHEEL
        );
        assign_controller(Synth::ParamId::ERRS, macro_ctl_id);
    } else {
        synth.effects.reverb.room_size.set_ratio(rng.random(0.2, 0.7));
    }

    if (rng.random() < 0.5) {
        synth.effects.reverb.distortion_level.set_value(rng.random(0.05, 0.20));
    }

    if (rng.random() < 0.7) {
        synth.effects.reverb.side_chain_compression_threshold.set_value(
            rng.random_choice<Number, 3>({-24.0, -21.0, -18.0})
        );
        synth.effects.reverb.side_chain_compression_ratio.set_value(
            rng.random(1.2, 3.0)
        );
    }

    synth.effects.reverb.width.set_ratio(rng.random());

    Number const wet = rng.random_normal(0.01, 0.15);

    synth.effects.reverb.wet.set_value(wet);
    synth.effects.reverb.dry.set_value(1.0 - wet);
}


void RandomPatchGenerator::refresh_all_params() const noexcept
{
    for (size_t i = 0; i != (size_t)Synth::ParamId::PARAM_ID_COUNT; ++i) {
        synth.process_message(
            Synth::MessageType::REFRESH_PARAM, (Synth::ParamId)i, 0.0, 0
        );
    }
}

}

#endif
