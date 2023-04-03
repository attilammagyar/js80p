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
    Serializer serializer;
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
        Synth::ControllerId::FLEXIBLE_CONTROLLER_1
    );
    synth_2.process_messages();

    serialized = serializer.serialize(&synth_1);
    serializer.import(&synth_2, serialized);
    synth_2.process_messages();

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
    Serializer serializer;
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

    serializer.import(&synth, patch);
    synth.process_messages();

    assert_eq(
        0.42, synth.get_param_ratio_atomic(Synth::ParamId::PM), DOUBLE_DELTA
    );
    assert_eq(
        0.43, synth.get_param_ratio_atomic(Synth::ParamId::CVOL), DOUBLE_DELTA
    );
})


TEST(importing_a_patch_ignores_invalid_lines_and_unknown_sections, {
    Synth synth;
    Serializer serializer;
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

    serializer.import(&synth, patch);
    synth.process_messages();

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
    Serializer serializer;
    std::string const patch = (
        "[js80p]\n"
        "PM = 0.42"
    );

    serializer.import(&synth, patch);
    synth.process_messages();

    assert_eq(
        0.42, synth.get_param_ratio_atomic(Synth::ParamId::PM), DOUBLE_DELTA
    );
})


TEST(imported_values_are_clamped, {
    Synth synth;
    Serializer serializer;
    std::string const patch = (
        "[js80p]\n"
        "PM = 2.1\n"
    );

    serializer.import(&synth, patch);
    synth.process_messages();

    assert_eq(
        1.0, synth.get_param_ratio_atomic(Synth::ParamId::PM), DOUBLE_DELTA
    );
})


TEST(extremely_long_lines_may_be_truncated, {
    constexpr Integer spaces_count = Serializer::MAX_SIZE * 2 + 123;
    Synth synth;
    Serializer serializer;
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
    serializer.import(&synth, patch);
    synth.process_messages();

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
