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
#include <string>

#include "test.cpp"
#include "utils.cpp"

#include "js80p.hpp"
#include "midi.hpp"

#include "serializer.cpp"
#include "synth.cpp"


using namespace JS80P;


SimpleOscillator::WaveformParam wavetable_cache_waveform("WAV");
SimpleOscillator wavetable_cache(wavetable_cache_waveform);


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


TEST(can_convert_synth_configuration_to_string_and_import_it, {
    Synth synth_1;
    Synth synth_2;
    std::string serialized;
    Number const inv_saw_as_ratio = (
        synth_1.modulator_params.waveform.value_to_ratio(
            SimpleOscillator::INVERSE_SAWTOOTH
        )
    );
    Number const triangle_as_ratio = (
        synth_1.modulator_params.waveform.value_to_ratio(
            SimpleOscillator::TRIANGLE
        )
    );

    synth_1.push_message(
        Synth::MessageType::SET_PARAM, Synth::ParamId::MWAV, inv_saw_as_ratio, 0
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
        Synth::ParamId::CWAV,
        0.0,
        Synth::ControllerId::MODULATION_WHEEL
    );
    synth_1.process_messages();

    synth_2.push_message(
        Synth::MessageType::SET_PARAM, Synth::ParamId::MWAV, triangle_as_ratio, 0
    );
    synth_2.push_message(
        Synth::MessageType::SET_PARAM, Synth::ParamId::PM, 0.42, 0
    );
    synth_2.push_message(
        Synth::MessageType::ASSIGN_CONTROLLER,
        Synth::ParamId::CVOL,
        0.0,
        Synth::ControllerId::VELOCITY
    );
    synth_2.push_message(
        Synth::MessageType::ASSIGN_CONTROLLER,
        Synth::ParamId::CWAV,
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
        synth_2.get_param_ratio_atomic(Synth::ParamId::MWAV),
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
        synth_2.get_param_controller_id_atomic(Synth::ParamId::CWAV)
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
    constexpr Integer spaces_count = Serializer::MAX_SIZE * 2 + 123;
    Synth synth;
    std::string patch("[js80p]\n");
    std::string spaces(" ");
    std::string long_line("");

    for (Integer i = spaces_count; i != 0;)
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
        ToggleParam::ON, synth.modulator_params.filter_1_log_scale.get_value(), DOUBLE_DELTA
    );
    assert_eq(
        1928.2, synth.modulator_params.filter_1_frequency.get_value(), 19.282
    );
    assert_eq(
        ToggleParam::OFF, synth.modulator_params.filter_2_log_scale.get_value(), DOUBLE_DELTA
    );
    assert_eq(
        18000.0, synth.modulator_params.filter_2_frequency.get_value(), 1.0
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
    char param_name[Constants::PARAM_NAME_MAX_LENGTH];
    char suffix[4];
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
    SignalProducer::produce<Synth>(synth, 1);

    synth.control_change(0.0, 0, Midi::MODULATION_WHEEL, 100);
    SignalProducer::produce<Synth>(synth, 2);

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


TEST(synth_message_queue_is_cleared_before_importing_patch_inside_audio_thread, {
    Synth synth;
    std::string const patch = (
        "[js80p]\n"
        "AM = 0.42\n"
    );

    synth.push_message(
        Synth::MessageType::SET_PARAM, Synth::ParamId::AM, 0.123, 0
    );
    Serializer::import_patch_in_audio_thread(synth, patch);
    SignalProducer::produce<Synth>(synth, 1);

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
    SignalProducer::produce<Synth>(synth, 1);

    assert_eq(
        0.42, synth.get_param_ratio_atomic(Synth::ParamId::AM), DOUBLE_DELTA
    );
    assert_eq(
        0.0, synth.get_param_ratio_atomic(Synth::ParamId::FM), DOUBLE_DELTA
    );
})


void assert_trimmed(char const* expected, char const* raw_number)
{
    constexpr size_t buffer_size = 16;

    char buffer[buffer_size];
    int const length = snprintf(buffer, buffer_size, "%s", raw_number);

    Serializer::trim_excess_zeros_from_end_after_snprintf(buffer, length, buffer_size);
    assert_eq(expected, buffer);

    if (strncmp(expected, raw_number, buffer_size) != 0) {
        std::fill_n(buffer, buffer_size, '0');
        int const terminating_zero = snprintf(buffer, buffer_size, "%s", expected);
        buffer[terminating_zero] = '0';
        buffer[buffer_size - 1] = '\x00';
        Serializer::trim_excess_zeros_from_end_after_snprintf(buffer, 12345, buffer_size);
        assert_eq(expected, buffer);
    }

    snprintf(buffer, buffer_size, "000");
    Serializer::trim_excess_zeros_from_end_after_snprintf(buffer, -1, buffer_size);
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


TEST(trailing_zeros_and_none_controllers_and_params_with_default_values_are_omitted_from_serialized_patch, {
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
