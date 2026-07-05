/*
 * This file is part of JS80P, a synthesizer plugin.
 * Copyright (C) 2023, 2024, 2025, 2026  Attila M. Magyar
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
#include <clocale>
#include <cstddef>
#include <cstdio>
#include <string>

#include "test.cpp"
#include "utils.hpp"

#include "js80p.hpp"
#include "midi.hpp"

#include "dsp/lfo.hpp"
#include "dsp/math.hpp"

#include "serializer.hpp"
#include "synth.hpp"


using namespace JS80P;


LFO wavetable_cache("CACHE");


void assert_in(std::string const& needle, std::string const& haystack)
{
    std::string::size_type pos = haystack.find(needle);

    assert_neq(
        (int)pos,
        (int)std::string::npos,
        "needle=\"%s\", haystack=\"%s\"",
        needle.c_str(),
        haystack.c_str()
    );
}


void set_synth_discrete_param_value(
        Synth& synth,
        Synth::ParamId const param_id,
        Byte const value
) {
    synth.process_message(
        Synth::MessageType::SET_PARAM,
        param_id,
        synth.discrete_param_value_to_ratio(param_id, value),
        0
    );
}


TEST(can_convert_synth_configuration_to_string_and_import_it, {
    Synth synth_1;
    Synth synth_2;
    std::string serialized;
    Number const inv_saw_as_ratio = (
        synth_1.discrete_param_value_to_ratio(
            Synth::ParamId::MWFM, Modulator::Oscillator_::INVERSE_SAWTOOTH
        )
    );
    Number const triangle_as_ratio = (
        synth_1.discrete_param_value_to_ratio(
            Synth::ParamId::MWFM, Modulator::Oscillator_::TRIANGLE
        )
    );

    synth_1.push_message(
        Synth::MessageType::SET_PARAM, Synth::ParamId::MWFM, inv_saw_as_ratio, 0
    );
    synth_1.push_message(
        Synth::MessageType::SET_PARAM, Synth::ParamId::PM, 0.123, 0
    );
    synth_1.push_message(
        Synth::MessageType::ASSIGN_CONTROLLER,
        Synth::ParamId::CVOL,
        0.0,
        Synth::ControllerId::ENVELOPE_3
    );
    synth_1.push_message(
        Synth::MessageType::ASSIGN_CONTROLLER,
        Synth::ParamId::CWFM,
        0.0,
        Synth::ControllerId::MODULATION_WHEEL
    );
    synth_1.process_messages();

    synth_2.push_message(
        Synth::MessageType::SET_PARAM,
        Synth::ParamId::MWFM,
        triangle_as_ratio,
        0
    );
    synth_2.push_message(
        Synth::MessageType::SET_PARAM, Synth::ParamId::PM, 0.42, 0
    );
    synth_2.push_message(
        Synth::MessageType::ASSIGN_CONTROLLER,
        Synth::ParamId::CVOL,
        0.0,
        Synth::ControllerId::TRIGGERED_VELOCITY
    );
    synth_2.push_message(
        Synth::MessageType::ASSIGN_CONTROLLER,
        Synth::ParamId::CWFM,
        0.0,
        Synth::ControllerId::PITCH_WHEEL
    );
    synth_2.push_message(
        Synth::MessageType::ASSIGN_CONTROLLER,
        Synth::ParamId::MVOL,
        0.0,
        Synth::ControllerId::MACRO_1
    );
    synth_2.process_messages();

    serialized = Serializer::serialize(synth_1);
    Serializer::import_patch_in_audio_thread(synth_2, serialized);

    assert_in("\r\nPM = 0.123", serialized);

    assert_eq(
        inv_saw_as_ratio,
        synth_2.get_param_ratio_atomic(Synth::ParamId::MWFM),
        DOUBLE_DELTA
    );
    assert_eq(
        0.123, synth_2.get_param_ratio_atomic(Synth::ParamId::PM), DOUBLE_DELTA
    );
    assert_eq(
        Synth::ControllerId::ENVELOPE_3,
        synth_2.get_param_controller_id_atomic(Synth::ParamId::CVOL)
    );
    assert_eq(
        Synth::ControllerId::MODULATION_WHEEL,
        synth_2.get_param_controller_id_atomic(Synth::ParamId::CWFM)
    );
    assert_eq(
        Synth::ControllerId::NONE,
        synth_2.get_param_controller_id_atomic(Synth::ParamId::MVOL)
    );
})


TEST(importing_a_patch_ignores_comments_and_whitespace_and_unknown_sections, {
    Synth synth;
    std::string const patch = (
        "  [  \t   js80p   \t   ]    ; comment\n"
        "; PM = 0.99\n"
        "  ; PM = 0.98\n"
        "   \t   PM    = \t    0.42        \n"
        "CVOL = 0.43 ; some comment\n"
        "\n"
        "[unknown]\n"
        "PM = 0.123\n"
    );

    Serializer::import_patch_in_audio_thread(synth, patch);

    assert_eq(
        0.42, synth.get_param_ratio_atomic(Synth::ParamId::PM), DOUBLE_DELTA
    );
    assert_eq(
        0.43, synth.get_param_ratio_atomic(Synth::ParamId::CVOL), DOUBLE_DELTA
    );
})


TEST(importing_a_patch_ignores_invalid_lines_and_unknown_sections, {
    Synth synth;
    std::string const patch = (
        "AM = 0.99\n"
        "[js80p]\n"
        "PM = 0.42\n"
        "MVOL = 1\n"
        "CVOL = .6\n"
        "= 0.98\n"
        "PM 0.97\n"
        "PM =\n"
        "PM = a\n"
        "PM = 0.96  a   \n"
        "PMx = 0.95\n"
        "PM = 0.94a   \n"
        "PM = 0.93  a   \n"
        "PM = -0.92\n"
        "PM = 0..91\n"
        "PM = ..90\n"
        "\n"
        "[js08p]]\n"
        "[js08p]x\n"
        "\n"
        "FM = 0.\n"
        "MIX = 0.123\n"
        "[js80px]\n"
        "PM = 0.89\n"
        "MVOL = 0.88\n"
        "CVOL = 0.87\n"
        "FM = 0.86\n"
        "MIX = 0.85\n"
        "[js80paaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa]\n"
        "PM = 0.84\n"
        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA = 0.83\n"
    );

    Serializer::import_patch_in_audio_thread(synth, patch);

    assert_eq(
        0.42, synth.get_param_ratio_atomic(Synth::ParamId::PM), DOUBLE_DELTA
    );
    assert_eq(
        1.00, synth.get_param_ratio_atomic(Synth::ParamId::MVOL), DOUBLE_DELTA
    );
    assert_eq(
        0.60, synth.get_param_ratio_atomic(Synth::ParamId::CVOL), DOUBLE_DELTA
    );
    assert_eq(
        0.0, synth.get_param_ratio_atomic(Synth::ParamId::FM), DOUBLE_DELTA
    );
    assert_eq(
        0.123, synth.get_param_ratio_atomic(Synth::ParamId::MIX), DOUBLE_DELTA
    );
    assert_eq(
        synth.get_param_default_ratio(Synth::ParamId::AM),
        synth.get_param_ratio_atomic(Synth::ParamId::AM),
        DOUBLE_DELTA
    );
    assert_neq(
        0.99,
        synth.get_param_ratio_atomic(Synth::ParamId::AM),
        DOUBLE_DELTA
    );
})


TEST(importing_a_patch_does_not_require_terminating_new_line, {
    Synth synth;
    std::string const patch = (
        "[js80p]\n"
        "PM = 0.42"
    );

    Serializer::import_patch_in_audio_thread(synth, patch);

    assert_eq(
        0.42, synth.get_param_ratio_atomic(Synth::ParamId::PM), DOUBLE_DELTA
    );
})


TEST(imported_values_are_clamped, {
    Synth synth;
    std::string const patch = (
        "[js80p]\n"
        "PM = 2.1\n"
    );

    Serializer::import_patch_in_audio_thread(synth, patch);

    assert_eq(
        1.0, synth.get_param_ratio_atomic(Synth::ParamId::PM), DOUBLE_DELTA
    );
})


TEST(extremely_long_lines_may_be_truncated, {
    constexpr size_t spaces_count = Serializer::MAX_SIZE * 2 + 123;
    Synth synth;
    std::string patch("[js80p]\n");
    std::string spaces(" ");
    std::string long_line("");

    for (size_t i = spaces_count; i != 0;)
    {
        if ((i & 1) != 0) {
            long_line += spaces;
        }

        spaces += spaces;
        i >>= 1;
    }

    assert_eq((int)spaces_count, (int)long_line.size());

    patch += long_line;
    patch += (
        "MVOL = 0.42\n"
        "CVOL = 0.123\n"
    );
    Serializer::import_patch_in_audio_thread(synth, patch);

    assert_eq(
        synth.get_param_default_ratio(Synth::ParamId::PM),
        synth.get_param_ratio_atomic(Synth::ParamId::PM),
        DOUBLE_DELTA
    );
    assert_eq(
        synth.get_param_default_ratio(Synth::ParamId::MVOL),
        synth.get_param_ratio_atomic(Synth::ParamId::MVOL),
        DOUBLE_DELTA
    );
    assert_eq(
        0.123, synth.get_param_ratio_atomic(Synth::ParamId::CVOL), DOUBLE_DELTA
    );
})


TEST(toggle_params_are_loaded_before_other_params, {
    Synth synth;
    std::string const patch = (
        "[js80p]\n"
        "MF1FRQ = 0.75\n"
        "MF1LOG = 1\n"
        "MF2LOG = 0\n"
        "MF2FRQ = 0.75\n"
    );

    Serializer::import_patch_in_audio_thread(synth, patch);

    assert_eq(
        ToggleParam::ON, (Byte)synth.get_param_value(Synth::ParamId::MF1LOG)
    );
    assert_eq(
        1928.2, synth.get_param_value(Synth::ParamId::MF1FRQ), 19.282
    );
    assert_eq(
        ToggleParam::OFF, (Byte)synth.get_param_value(Synth::ParamId::MF2LOG)
    );
    assert_eq(
        18000.0, synth.get_param_value(Synth::ParamId::MF2FRQ), 1.0
    );
})


TEST(param_names_are_parsed_case_insensitively_and_converted_to_upper_case, {
    Synth synth;
    std::string const patch = (
        "[js80p]\n"
        "cVol = 0.5\n"
        "cVolctl = 0.123\n"
    );
    std::string const line_with_ctl = "cVolctl = 0.1";
    std::string const line_without_ctl = "cVol = 0.1";
    Serializer::ParamName param_name;
    Serializer::Suffix suffix;
    std::string::const_iterator line_with_ctl_it = line_with_ctl.begin();
    std::string::const_iterator line_without_ctl_it = line_without_ctl.begin();

    Serializer::import_patch_in_audio_thread(synth, patch);

    assert_eq(
        0.5, synth.get_param_ratio_atomic(Synth::ParamId::CVOL), DOUBLE_DELTA
    );

    Serializer::parse_line_until_value(
        line_with_ctl_it, line_with_ctl.end(), param_name, suffix
    );
    assert_eq("CVOL", param_name);
    assert_eq("ctl", suffix);

    param_name[0] = 'x';
    suffix[0] = 'x';

    Serializer::parse_line_until_value(
        line_without_ctl_it, line_without_ctl.end(), param_name, suffix
    );
    assert_eq("CVOL", param_name);
    assert_eq("", suffix);
})


TEST(params_which_are_missing_from_the_patch_are_cleared_and_reset_to_default, {
    Synth synth;
    std::string const patch = (
        "[js80p]\n"
        "AM = 0.42\n"
    );

    synth.push_message(
        Synth::MessageType::SET_PARAM, Synth::ParamId::AM, 0.123, 0
    );
    synth.push_message(
        Synth::MessageType::ASSIGN_CONTROLLER,
        Synth::ParamId::AM,
        0.0,
        Synth::ControllerId::MODULATION_WHEEL
    );
    synth.push_message(
        Synth::MessageType::SET_PARAM, Synth::ParamId::PM, 0.123, 0
    );
    synth.push_message(
        Synth::MessageType::ASSIGN_CONTROLLER,
        Synth::ParamId::PM,
        0.0,
        Synth::ControllerId::MODULATION_WHEEL
    );
    synth.process_messages();

    synth.control_change(0.0, 0, Midi::MODULATION_WHEEL, 100);
    synth.process_messages();

    Serializer::import_patch_in_audio_thread(synth, patch);

    assert_eq(
        0.42, synth.get_param_ratio_atomic(Synth::ParamId::AM), DOUBLE_DELTA
    );
    assert_eq(
        Synth::ControllerId::NONE,
        synth.get_param_controller_id_atomic(Synth::ParamId::CVOL)
    );
    assert_eq(
        0.00, synth.get_param_ratio_atomic(Synth::ParamId::PM), DOUBLE_DELTA
    );
})


TEST(synth_message_queue_is_cleared_before_importing_patch_in_audio_thread, {
    Synth synth;
    std::string const patch = (
        "[js80p]\n"
        "AM = 0.42\n"
    );

    synth.push_message(
        Synth::MessageType::SET_PARAM, Synth::ParamId::AM, 0.123, 0
    );
    Serializer::import_patch_in_audio_thread(synth, patch);
    synth.process_messages();

    assert_eq(
        0.42, synth.get_param_ratio_atomic(Synth::ParamId::AM), DOUBLE_DELTA
    );
})


TEST(can_import_patch_inside_the_gui_thread, {
    Synth synth;
    std::string const patch = (
        "[js80p]\n"
        "AM = 0.42\n"
    );

    synth.push_message(
        Synth::MessageType::SET_PARAM, Synth::ParamId::FM, 0.123, 0
    );
    Serializer::import_patch_in_gui_thread(synth, patch);
    synth.process_messages();

    assert_eq(
        0.42, synth.get_param_ratio_atomic(Synth::ParamId::AM), DOUBLE_DELTA
    );
    assert_eq(
        0.0, synth.get_param_ratio_atomic(Synth::ParamId::FM), DOUBLE_DELTA
    );
})


void assert_trimmed(char const* const expected, char const* const raw_number)
{
    constexpr size_t buffer_size = 16;

    char buffer[buffer_size];
    int const length = snprintf(buffer, buffer_size, "%s", raw_number);

    Serializer::trim_excess_zeros_from_end(buffer, length, buffer_size);
    assert_eq(expected, buffer);

    if (strncmp(expected, raw_number, buffer_size) != 0) {
        std::fill_n(buffer, buffer_size, '0');
        int const terminating_zero = snprintf(
            buffer, buffer_size, "%s", expected
        );
        buffer[terminating_zero] = '0';
        buffer[buffer_size - 1] = '\x00';
        Serializer::trim_excess_zeros_from_end(buffer, 12345, buffer_size);
        assert_eq(expected, buffer);
    }

    snprintf(buffer, buffer_size, "000");
    Serializer::trim_excess_zeros_from_end(buffer, -1, buffer_size);
    assert_eq("000", buffer);
}


TEST(trimming_zeros_from_end_of_numbers, {
    assert_trimmed("", "");
    assert_trimmed("0", "0");
    assert_trimmed("1", "1");
    assert_trimmed("10", "10");
    assert_trimmed("100", "100");
    assert_trimmed("1000", "1000");
    assert_trimmed("0.0", "0.0");
    assert_trimmed("0.1", "0.1");
    assert_trimmed("0.10", "0.10");
    assert_trimmed("0.12", "0.12");
    assert_trimmed("0.120", "0.120");
    assert_trimmed("0.0", "0.00");
    assert_trimmed("0.0", "0.00000");
    assert_trimmed("0.120", "0.1200");
    assert_trimmed("0.120", "0.120000");
    assert_trimmed("0.120", "0.1200000000000");
    assert_trimmed("0.1234567890123", "0.1234567890123");
})


TEST(trailing_zeros_and_none_ctls_and_params_with_default_values_are_omitted, {
    Synth synth;
    std::string patch = "";

    patch += "[js80p]";
    patch += Serializer::LINE_END;
    patch += "FM = 0.50";
    patch += Serializer::LINE_END;

    synth.push_message(
        Synth::MessageType::CLEAR, Synth::ParamId::INVALID_PARAM_ID, 0.0, 0
    );
    synth.push_message(
        Synth::MessageType::SET_PARAM, Synth::ParamId::FM, 0.5, 0
    );
    synth.process_messages();

    assert_eq(patch, Serializer::serialize(synth));
})


TEST(when_a_param_has_a_controller_then_its_own_value_is_omitted, {
    Synth synth;
    std::string patch = "";

    patch += "[js80p]";
    patch += Serializer::LINE_END;
    patch += "FMctl = 0.50";
    patch += Serializer::LINE_END;

    synth.push_message(
        Synth::MessageType::CLEAR, Synth::ParamId::INVALID_PARAM_ID, 0.0, 0
    );
    synth.push_message(
        Synth::MessageType::ASSIGN_CONTROLLER,
        Synth::ParamId::FM,
        0.0,
        Synth::ControllerId::PITCH_WHEEL
    );
    synth.process_messages();

    assert_eq(patch, Serializer::serialize(synth));
})


TEST(current_tuning_is_kept_unless_the_patch_contains_a_tuning, {
    constexpr size_t serialized_modulator_tuning_length = 64;

    Synth synth;
    char serialized_modulator_tuning[serialized_modulator_tuning_length];
    std::string patch = "";

    snprintf(
        serialized_modulator_tuning,
        serialized_modulator_tuning_length,
        "MTUN = %f",
        (double)synth.discrete_param_value_to_ratio(
            Synth::ParamId::MTUN, Modulator::TUNING_432HZ_12TET
        )
    );

    patch += "[js80p]";
    patch += Serializer::LINE_END;
    patch += serialized_modulator_tuning;
    patch += Serializer::LINE_END;

    set_synth_discrete_param_value(
        synth, Synth::ParamId::MTUN, Modulator::TUNING_MTS_ESP_NOTE_ON
    );
    set_synth_discrete_param_value(
        synth, Synth::ParamId::CTUN, Modulator::TUNING_MTS_ESP_NOTE_ON
    );

    Serializer::import_patch_in_audio_thread(synth, patch);

    assert_eq(
        (int)Modulator::TUNING_432HZ_12TET,
        (int)synth.get_param_value(Synth::ParamId::MTUN)
    );
    assert_eq(
        (int)Carrier::TUNING_MTS_ESP_NOTE_ON,
        (int)synth.get_param_value(Synth::ParamId::CTUN)
    );
})


TEST(mpe_settings_are_kept_unless_the_patch_contains_mpe_settings, {
    constexpr size_t serialized_mpe_settings_length = 64;

    Synth synth;
    char serialized_mpe_settings[serialized_mpe_settings_length];
    std::string patch = "";

    snprintf(
        serialized_mpe_settings,
        serialized_mpe_settings_length,
        "MPE = %f",
        (double)synth.discrete_param_value_to_ratio(
            Synth::ParamId::MPEST, Synth::MPE_U10
        )
    );

    patch += "[js80p]";
    patch += Serializer::LINE_END;
    patch += serialized_mpe_settings;
    patch += Serializer::LINE_END;

    set_synth_discrete_param_value(
        synth, Synth::ParamId::MPEST, Synth::MPE_L15
    );

    Serializer::import_patch_in_audio_thread(synth, patch);

    assert_eq(
        (int)Synth::MPE_U10, (int)synth.get_param_value(Synth::ParamId::MPEST)
    );

    patch = "";

    Serializer::import_patch_in_audio_thread(synth, patch);

    assert_eq(
        (int)Synth::MPE_U10, (int)synth.get_param_value(Synth::ParamId::MPEST)
    );
})


void assert_value_upgrade(
        std::string const& old_serialized_param,
        Synth::ParamId const param_id,
        Byte const expected_value
) {
    Synth synth;
    std::string patch = "";

    patch += "[js80p]";
    patch += Serializer::LINE_END;
    patch += old_serialized_param;
    patch += Serializer::LINE_END;

    Serializer::import_patch_in_audio_thread(synth, patch);

    assert_eq(
        expected_value,
        synth.get_param_value(param_id),
        "old_serialized_param=\"%s\"",
        old_serialized_param.c_str()
    );
    assert_eq(
        (int)Synth::ControllerId::NONE,
        (int)synth.get_param_controller_id_atomic(param_id),
        "old_serialized_param=\"%s\"",
        old_serialized_param.c_str()
    );
}


void assert_controller_upgrade(
        std::string const& old_serialized_param,
        Synth::ParamId const param_id,
        Synth::ControllerId const expected_controller
) {
    Synth synth;
    std::string patch = "";

    patch += "[js80p]";
    patch += Serializer::LINE_END;
    patch += old_serialized_param;
    patch += Serializer::LINE_END;

    Serializer::import_patch_in_audio_thread(synth, patch);

    assert_eq(
        (int)expected_controller,
        (int)synth.get_param_controller_id_atomic(param_id),
        "old_serialized_param=\"%s\"",
        old_serialized_param.c_str()
    );
}


TEST(old_note_handling_parameter_is_upgraded, {
    assert_value_upgrade(
        "POLY = 0.0", Synth::ParamId::NH, Synth::NOTE_HANDLING_MONO
    );
    assert_value_upgrade(
        "POLY = 0.333333333333333",
        Synth::ParamId::NH,
        Synth::NOTE_HANDLING_MONO_HOLD
    );
    assert_value_upgrade(
        "POLY = 0.666666666666667",
        Synth::ParamId::NH,
        Synth::NOTE_HANDLING_POLY_HOLD
    );
    assert_value_upgrade(
        "POLY = 1.0", Synth::ParamId::NH, Synth::NOTE_HANDLING_POLY
    );
})


TEST(old_envelope_update_mode_parameter_is_upgraded, {
    assert_value_upgrade(
        "N1DYN = 0.0", Synth::ParamId::N1UPD, Envelope::UPDATE_MODE_STATIC
    );
    assert_value_upgrade(
        "N10DYN = 0.50", Synth::ParamId::N10UPD, Envelope::UPDATE_MODE_END
    );
    assert_value_upgrade(
        "N12DYN = 1.0", Synth::ParamId::N12UPD, Envelope::UPDATE_MODE_DYNAMIC
    );
})


TEST(old_voice_oscillator_waveform_parameters_are_upgraded, {
    assert_value_upgrade(
        "MWAV = 0.0", Synth::ParamId::MWFM, SimpleOscillator::SINE
    );
    assert_value_upgrade(
        "CWAV = 0.111111111111111",
        Synth::ParamId::CWFM,
        SimpleOscillator::SAWTOOTH
    );
    assert_value_upgrade(
        "MWAV = 0.222222222222222",
        Synth::ParamId::MWFM,
        SimpleOscillator::SOFT_SAWTOOTH
    );
    assert_value_upgrade(
        "CWAV = 0.333333333333333",
        Synth::ParamId::CWFM,
        SimpleOscillator::INVERSE_SAWTOOTH
    );
    assert_value_upgrade(
        "MWAV = 0.444444444444444",
        Synth::ParamId::MWFM,
        SimpleOscillator::SOFT_INVERSE_SAWTOOTH
    );
    assert_value_upgrade(
        "CWAV = 0.555555555555556",
        Synth::ParamId::CWFM,
        SimpleOscillator::TRIANGLE
    );
    assert_value_upgrade(
        "MWAV = 0.666666666666667",
        Synth::ParamId::MWFM,
        SimpleOscillator::SOFT_TRIANGLE
    );
    assert_value_upgrade(
        "CWAV = 0.777777777777778",
        Synth::ParamId::CWFM,
        SimpleOscillator::SQUARE
    );
    assert_value_upgrade(
        "MWAV = 0.888888888888889",
        Synth::ParamId::MWFM,
        SimpleOscillator::SOFT_SQUARE
    );
    assert_value_upgrade(
        "CWAV = 1.0", Synth::ParamId::CWFM, SimpleOscillator::CUSTOM
    );

    assert_controller_upgrade(
        "MWAVctl = 0.06250",
        Synth::ParamId::MWFM,
        Synth::ControllerId::GENERAL_1
    );
    assert_controller_upgrade(
        "CWAVctl = 0.06250",
        Synth::ParamId::CWFM,
        Synth::ControllerId::GENERAL_1
    );
})


TEST(old_lfo_waveform_parameters_are_upgraded, {
    assert_value_upgrade(
        "L1WAV = 0.0", Synth::ParamId::L1WAV, SimpleOscillator::SINE
    );
    assert_value_upgrade(
        "L2WAV = 0.0", Synth::ParamId::L2WAV, SimpleOscillator::SINE
    );
    assert_value_upgrade(
        "L3WAV = 0.1250", Synth::ParamId::L3WAV, SimpleOscillator::SAWTOOTH
    );
    assert_value_upgrade(
        "L4WAV = 0.1250", Synth::ParamId::L4WAV, SimpleOscillator::SAWTOOTH
    );
    assert_value_upgrade(
        "L5WAV = 0.250", Synth::ParamId::L5WAV, SimpleOscillator::SOFT_SAWTOOTH
    );
    assert_value_upgrade(
        "L6WAV = 0.250", Synth::ParamId::L6WAV, SimpleOscillator::SOFT_SAWTOOTH
    );
    assert_value_upgrade(
        "L7WAV = 0.3750",
        Synth::ParamId::L7WAV,
        SimpleOscillator::INVERSE_SAWTOOTH
    );
    assert_value_upgrade(
        "L8WAV = 0.3750",
        Synth::ParamId::L8WAV,
        SimpleOscillator::INVERSE_SAWTOOTH
    );
    assert_value_upgrade(
        "L1WAV = 0.50",
        Synth::ParamId::L1WAV,
        SimpleOscillator::SOFT_INVERSE_SAWTOOTH
    );
    assert_value_upgrade(
        "L2WAV = 0.50",
        Synth::ParamId::L2WAV,
        SimpleOscillator::SOFT_INVERSE_SAWTOOTH
    );
    assert_value_upgrade(
        "L3WAV = 0.6250", Synth::ParamId::L3WAV, SimpleOscillator::TRIANGLE
    );
    assert_value_upgrade(
        "L4WAV = 0.6250", Synth::ParamId::L4WAV, SimpleOscillator::TRIANGLE
    );
    assert_value_upgrade(
        "L5WAV = 0.750", Synth::ParamId::L5WAV, SimpleOscillator::SOFT_TRIANGLE
    );
    assert_value_upgrade(
        "L6WAV = 0.750", Synth::ParamId::L6WAV, SimpleOscillator::SOFT_TRIANGLE
    );
    assert_value_upgrade(
        "L7WAV = 0.8750", Synth::ParamId::L7WAV, SimpleOscillator::SQUARE
    );
    assert_value_upgrade(
        "L8WAV = 0.8750", Synth::ParamId::L8WAV, SimpleOscillator::SQUARE
    );
    assert_value_upgrade(
        "L1WAV = 1.0", Synth::ParamId::L1WAV, SimpleOscillator::SOFT_SQUARE
    );
    assert_value_upgrade(
        "L2WAV = 1.0", Synth::ParamId::L2WAV, SimpleOscillator::SOFT_SQUARE
    );

    assert_controller_upgrade(
        "L1WAVctl = 0.06250",
        Synth::ParamId::L1WAV,
        Synth::ControllerId::GENERAL_1
    );
    assert_controller_upgrade(
        "L2WAVctl = 0.06250",
        Synth::ParamId::L2WAV,
        Synth::ControllerId::GENERAL_1
    );
})


TEST(old_distortion_types_are_upgraded, {
    assert_value_upgrade(
        "CDT = 0.0", Synth::ParamId::CDTYP, Distortion::TYPE_TANH_3
    );
    assert_value_upgrade(
        "EOT = 0.090909090909091",
        Synth::ParamId::ED1TYP,
        Distortion::TYPE_TANH_10
    );
    assert_value_upgrade(
        "EDT = 0.045454545454545",
        Synth::ParamId::ED2TYP,
        Distortion::TYPE_TANH_5
    );
    assert_value_upgrade(
        "ETSTYP = 0.136363636363636",
        Synth::ParamId::ETSTYP,
        Distortion::TYPE_HARMONIC_13
    );
    assert_value_upgrade(
        "CDT = 0.181818181818182",
        Synth::ParamId::CDTYP,
        Distortion::TYPE_HARMONIC_15
    );
    assert_value_upgrade(
        "EOT = 0.227272727272727",
        Synth::ParamId::ED1TYP,
        Distortion::TYPE_HARMONIC_135
    );
    assert_value_upgrade(
        "EDT = 0.272727272727273",
        Synth::ParamId::ED2TYP,
        Distortion::TYPE_HARMONIC_SQR
    );
    assert_value_upgrade(
        "ETSTYP = 0.318181818181818",
        Synth::ParamId::ETSTYP,
        Distortion::TYPE_HARMONIC_TRI
    );
    assert_value_upgrade(
        "CDT = 0.363636363636364",
        Synth::ParamId::CDTYP,
        Distortion::TYPE_BIT_CRUSH_1
    );
    assert_value_upgrade(
        "EOT = 0.409090909090909",
        Synth::ParamId::ED1TYP,
        Distortion::TYPE_BIT_CRUSH_2
    );
    assert_value_upgrade(
        "EDT = 0.454545454545455",
        Synth::ParamId::ED2TYP,
        Distortion::TYPE_BIT_CRUSH_3
    );
    assert_value_upgrade(
        "ETSTYP = 0.50", Synth::ParamId::ETSTYP, Distortion::TYPE_BIT_CRUSH_4
    );
    assert_value_upgrade(
        "CDT = 0.545454545454545",
        Synth::ParamId::CDTYP,
        Distortion::TYPE_BIT_CRUSH_4_6
    );
    assert_value_upgrade(
        "EOT = 0.590909090909091",
        Synth::ParamId::ED1TYP,
        Distortion::TYPE_BIT_CRUSH_5
    );
    assert_value_upgrade(
        "EDT = 0.636363636363636",
        Synth::ParamId::ED2TYP,
        Distortion::TYPE_BIT_CRUSH_5_6
    );
    assert_value_upgrade(
        "ETSTYP = 0.681818181818182",
        Synth::ParamId::ETSTYP,
        Distortion::TYPE_BIT_CRUSH_6
    );
    assert_value_upgrade(
        "CDT = 0.727272727272727",
        Synth::ParamId::CDTYP,
        Distortion::TYPE_BIT_CRUSH_6_6
    );
    assert_value_upgrade(
        "EOT = 0.772727272727273",
        Synth::ParamId::ED1TYP,
        Distortion::TYPE_BIT_CRUSH_7
    );
    assert_value_upgrade(
        "EDT = 0.818181818181818",
        Synth::ParamId::ED2TYP,
        Distortion::TYPE_BIT_CRUSH_7_6
    );
    assert_value_upgrade(
        "ETSTYP = 0.863636363636364",
        Synth::ParamId::ETSTYP,
        Distortion::TYPE_BIT_CRUSH_8
    );
    assert_value_upgrade(
        "CDT = 0.909090909090909",
        Synth::ParamId::CDTYP,
        Distortion::TYPE_BIT_CRUSH_8_6
    );
    assert_value_upgrade(
        "EOT = 0.954545454545455",
        Synth::ParamId::ED1TYP,
        Distortion::TYPE_BIT_CRUSH_9
    );
    assert_value_upgrade(
        "EDT = 1.0", Synth::ParamId::ED2TYP, Distortion::TYPE_DELAY_FEEDBACK
    );
})


TEST(serialization_is_independent_of_locale, {
    constexpr char const* decimal_comma_locales[] = {
        "da_DK.UTF-8",
        "da_DK.utf8",
        "da_DK",
        "da-DK.UTF-8",
        "da-DK",
        "Danish_Denmark.1252",
        "Danish_Denmark.UTF8",
        "Danish_Denmark",
        "de_DE.UTF-8",
        "de_DE.utf8",
        "de_DE",
        "de-DE.UTF-8",
        "de-DE",
        "en_DK.UTF-8",
        "en_DK.utf8",
        "en_DK",
        "en-DK.UTF-8",
        "en-DK",
        "fr_FR.UTF-8",
        "fr_FR.utf8",
        "fr_FR",
        "fr-FR.UTF-8",
        "fr-FR",
        "French_France.1252",
        "French_France.UTF8",
        "French_France",
        "German_Germany.1252",
        "German_Germany.UTF8",
        "German_Germany",
        "hu_HU.UTF-8",
        "hu_HU.utf8",
        "hu_HU",
        "hu-HU.UTF-8",
        "hu-HU",
        "Hungarian_Hungary.1250",
        "Hungarian_Hungary",
        NULL,
    };
    Synth synth_1;
    Synth synth_2;
    std::string serialized;
    char const* locale = NULL;

    for (
            size_t i = 0;
            decimal_comma_locales[i] != NULL && locale == NULL;
            ++i
    ) {
        locale = setlocale(LC_NUMERIC, decimal_comma_locales[i]);
    }

    assert_neq(
        (void const*)locale,
        NULL,
        "Unable to find a decimal comma using locale for testing"
    );

    synth_1.push_message(
        Synth::MessageType::SET_PARAM, Synth::ParamId::PM, 0.123, 0
    );
    synth_1.push_message(
        Synth::MessageType::ASSIGN_CONTROLLER,
        Synth::ParamId::CVOL,
        0.0,
        Synth::ControllerId::ENVELOPE_3
    );
    synth_1.process_messages();

    synth_2.push_message(
        Synth::MessageType::SET_PARAM, Synth::ParamId::PM, 0.42, 0
    );
    synth_2.push_message(
        Synth::MessageType::ASSIGN_CONTROLLER,
        Synth::ParamId::CWFM,
        0.0,
        Synth::ControllerId::PITCH_WHEEL
    );
    synth_2.process_messages();

    serialized = Serializer::serialize(synth_1);
    Serializer::import_patch_in_audio_thread(synth_2, serialized);

    assert_in("\r\nPM = 0.123", serialized + "; locale=\"" + locale + "\"");

    assert_eq(
        0.123,
        synth_2.get_param_ratio_atomic(Synth::ParamId::PM),
        DOUBLE_DELTA,
        "locale=\"%s\"",
        locale
    );
    assert_eq(
        Synth::ControllerId::ENVELOPE_3,
        synth_2.get_param_controller_id_atomic(Synth::ParamId::CVOL),
        "locale=\"%s\"",
        locale
    );
    assert_eq(
        Synth::ControllerId::NONE,
        synth_2.get_param_controller_id_atomic(Synth::ParamId::CWFM),
        "locale=\"%s\"",
        locale
    );
})


TEST(can_load_buggy_locale_dependent_serialization, {
    Synth synth;

    synth.push_message(
        Synth::MessageType::SET_PARAM, Synth::ParamId::PM, 0.42, 0
    );
    synth.push_message(
        Synth::MessageType::ASSIGN_CONTROLLER,
        Synth::ParamId::CWFM,
        0.0,
        Synth::ControllerId::PITCH_WHEEL
    );
    synth.process_messages();

    Serializer::import_patch_in_audio_thread(
        synth,
        (
            "[js80p]\n"
            "PM = 0,42.123\n"           /* ignored due to being invalid */
            "PM = 0,42,123\n"           /* ignored due to being invalid */
            "PM = 0,1230\n"
            "PM = 0,42 123\n"           /* ignored due to being invalid */
            "CVOLctl = 0,589.43750\n"   /* ignored due to being invalid */
            "CVOLctl = 0,589,43750\n"   /* ignored due to being invalid */
            "CVOLctl = 0,589843750\n"
            "CVOLctl = 0,589 43750\n"   /* ignored due to being invalid */
        )
    );

    assert_eq(
        0.123, synth.get_param_ratio_atomic(Synth::ParamId::PM), DOUBLE_DELTA
    );
    assert_eq(
        Synth::ControllerId::ENVELOPE_3,
        synth.get_param_controller_id_atomic(Synth::ParamId::CVOL)
    );
    assert_eq(
        Synth::ControllerId::NONE,
        synth.get_param_controller_id_atomic(Synth::ParamId::CWFM)
    );
})


TEST(fuzzing, {
    Synth synth_1;
    Synth synth_2;
    std::string serialized;

    for (int i = 0; i != 20; ++i) {
        synth_1.process_message(
            Synth::MessageType::RANDOMIZE,
            Synth::ParamId::INVALID_PARAM_ID,
            0.0,
            0
        );
        serialized = Serializer::serialize(synth_1);
        Serializer::import_patch_in_audio_thread(synth_2, serialized);

        for (int j = 0; j != Synth::ParamId::PARAM_ID_COUNT; ++j) {
            Synth::ParamId const param_id = (Synth::ParamId)j;
            Synth::ControllerId const synth_1_controller_id = (
                synth_1.get_param_controller_id_atomic(param_id)
            );

            if (synth_1_controller_id == Synth::ControllerId::NONE) {
                assert_eq(
                    synth_1.get_param_ratio_atomic(param_id),
                    synth_2.get_param_ratio_atomic(param_id),
                    DOUBLE_DELTA,
                    "i=%d, param_id=%d, synth_1=\"%s\", synth_2=\"%s\"",
                    i,
                    (int)param_id,
                    serialized.c_str(),
                    Serializer::serialize(synth_2).c_str()
                );
            } else {
                assert_eq(
                    (int)synth_1_controller_id,
                    (int)synth_2.get_param_controller_id_atomic(param_id),
                    "i=%d, param_id=%d, synth_1=\"%s\", synth_2=\"%s\"",
                    i,
                    (int)param_id,
                    serialized.c_str(),
                    Serializer::serialize(synth_2).c_str()
                );
            }
        }
    }
})
