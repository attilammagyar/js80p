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

#ifndef JS80P__SERIALIZER_CPP
#define JS80P__SERIALIZER_CPP

#include <algorithm>
#include <cmath>
#include <cctype>
#include <cstring>
#include <ios>
#include <locale>
#include <sstream>

#include "serializer.hpp"


namespace JS80P
{

std::string const Serializer::CONTROLLER_SUFFIX = "ctl";

std::string const Serializer::LINE_END = "\r\n";


std::string Serializer::serialize(Synth const& synth) noexcept
{
    constexpr size_t line_size = 127;
    char line[line_size + 1];
    std::string serialized("");
    std::ostringstream oss;
    int length;

    serialized.reserve(MAX_SIZE);
    serialized += "[";
    serialized += JS80P_SECTION_NAME;
    serialized += "]";
    serialized += LINE_END;

    oss.imbue(std::locale::classic());
    oss.setf(std::ios_base::fixed, std::ios_base::floatfield);
    oss.precision(15);

    for (int i = 0; i != Synth::ParamId::PARAM_ID_COUNT; ++i) {
        Synth::ParamId const param_id = (Synth::ParamId)i;
        std::string const param_name = synth.get_param_name(param_id);

        if (param_name.length() < 1) {
            continue;
        }

        Synth::ControllerId const controller_id = (
            synth.get_param_controller_id_atomic(param_id)
        );

        if (controller_id == Synth::ControllerId::NONE) {
            Number const set_ratio = synth.get_param_ratio_atomic(param_id);
            Number const default_ratio = (
                synth.get_param_default_ratio(param_id)
            );

            if (std::fabs(default_ratio - set_ratio) <= 0.000001) {
                continue;
            }

            oss << set_ratio;
            length = snprintf(
                line,
                line_size,
                "%s = %s",
                param_name.c_str(),
                oss.str().c_str()
            );
        } else {
            oss << controller_id_to_float(controller_id);
            length = snprintf(
                line,
                line_size,
                "%s%s = %s",
                param_name.c_str(),
                CONTROLLER_SUFFIX.c_str(),
                oss.str().c_str()
            );
        }

        trim_excess_zeros_from_end(line, length, line_size);
        line[line_size] = '\x00';
        serialized += line;
        serialized += LINE_END;

        oss.str("");
        oss.clear();
    }

    return serialized;
}


void Serializer::trim_excess_zeros_from_end(
        char* const number,
        int const length,
        size_t const max_length
) noexcept {
    if (JS80P_UNLIKELY(length < 1 || max_length < 1)) {
        return;
    }

    size_t dot_index = max_length;

    for (size_t i = 0; number[i] != '\x00' && i != max_length; ++i) {
        if (number[i] == '.') {
            dot_index = i;

            break;
        }
    }

    if (JS80P_UNLIKELY(dot_index == max_length)) {
        return;
    }

    size_t last_zero_index = max_length;

    for (size_t i = dot_index; number[i] != '\x00' && i != max_length; ++i) {
        if (number[i] != '0') {
            last_zero_index = max_length;
        } else if (last_zero_index == max_length) {
            last_zero_index = i;
        }
    }

    if (last_zero_index == max_length) {
        return;
    }

    ++last_zero_index;

    if (last_zero_index < max_length) {
        number[last_zero_index] = '\x00';
    }
}


Number Serializer::controller_id_to_float(
        Synth::ControllerId const controller_id
) noexcept {
    return (Number)controller_id * CONTROLLER_ID_TO_FLOAT_SCALE;
}


Synth::ControllerId Serializer::float_to_controller_id(
    Number const controller_id
) noexcept {
    return (Synth::ControllerId)std::round(
        FLOAT_TO_CONTROLLER_ID_SCALE * controller_id
    );
}


void Serializer::import_patch_in_gui_thread(
        Synth& synth,
        std::string const& serialized
) noexcept {
    import_patch<Thread::GUI>(synth, serialized);
}


void Serializer::import_patch_in_audio_thread(
        Synth& synth,
        std::string const& serialized
) noexcept {
    synth.process_messages();
    import_patch<Thread::AUDIO>(synth, serialized);
}


template<Serializer::Thread thread>
void Serializer::import_patch(
        Synth& synth,
        std::string const& serialized
) noexcept {
    Lines const* const lines = parse_lines(serialized);
    process_lines<thread>(synth, lines);

    delete lines;
}


Serializer::Lines* Serializer::parse_lines(
        std::string const& serialized
) noexcept {
    constexpr size_t max_line_pos = MAX_SIZE - 1;
    Lines* const lines = new Lines();
    char* const line = new char[MAX_SIZE];
    std::string::const_iterator const end = serialized.end();
    size_t line_pos = 0;
    bool truncating = false;

    for (std::string::const_iterator it = serialized.begin(); it != end; ++it) {
        char const c = *it;

        if (is_line_break(c)) {
            if (line_pos != 0) {
                lines->push_back(std::string(line, line_pos));
            }

            line_pos = 0;
            truncating = false;

            continue;
        }

        if (truncating) {
            continue;
        }

        line[line_pos++] = c;

        if (line_pos == max_line_pos) {
            lines->push_back(std::string(line, line_pos));
            line_pos = 0;
            truncating = true;
        }
    }

    if (line_pos != 0) {
        lines->push_back(std::string(line, line_pos));
    }

    delete[] line;

    return lines;
}


bool Serializer::is_digit(char const c) noexcept
{
    return '0' <= c && c <= '9';
}


bool Serializer::is_capital_letter(char const c) noexcept
{
    return 'A' <= c && c <= 'Z';
}


bool Serializer::is_lowercase_letter(char const c) noexcept
{
    return 'a' <= c && c <= 'z';
}


bool Serializer::is_line_break(char const c) noexcept
{
    return c == '\n' || c == '\r';
}


bool Serializer::is_inline_whitespace(char const c) noexcept
{
    return c == ' ' || c == '\t';
}


bool Serializer::is_comment_leader(char const c) noexcept
{
    return c == ';';
}


bool Serializer::is_section_name_char(char const c) noexcept
{
    return is_digit(c) || is_capital_letter(c) || is_lowercase_letter(c);
}


template<Serializer::Thread thread>
void Serializer::process_lines(Synth& synth, Lines const* const lines) noexcept
{
    typedef std::vector<Synth::Message> Messages;

    Messages messages;
    SectionName section_name;
    bool inside_js80p_section = false;

    messages.reserve(800);

    for (Lines::const_iterator it = lines->begin(); it != lines->end(); ++it) {
        std::string line = *it;

        if (parse_section_name(line, section_name)) {
            inside_js80p_section = false;

            if (is_js80p_section_start(section_name)) {
                inside_js80p_section = true;
                continue;
            }
        } else if (inside_js80p_section) {
            process_line(messages, synth, line);
        }
    }

    send_message<thread>(
        synth,
        Synth::Message(
            Synth::MessageType::CLEAR, Synth::ParamId::INVALID_PARAM_ID, 0.0, 0
        )
    );

    /*
    Load discrete parameters first because they may affect how float param
    ratios are to be interpreted (especially the log-scale toggles).
    */

    Messages::const_iterator it;

    for (it = messages.begin(); it != messages.end(); ++it) {
        if (synth.is_discrete_param(it->param_id)) {
            send_message<thread>(synth, *it);
        }
    }

    for (it = messages.begin(); it != messages.end(); ++it) {
        if (!synth.is_discrete_param(it->param_id)) {
            send_message<thread>(synth, *it);
        }
    }
}


template<Serializer::Thread thread>
void Serializer::send_message(
        Synth& synth,
        Synth::Message const& message
) noexcept {
    if constexpr (thread == Thread::AUDIO) {
        synth.process_message(message);
    } else {
        synth.push_message(message);
    }
}


bool Serializer::is_js80p_section_start(
        SectionName const& section_name
) noexcept {
    return (
        strncmp(section_name, JS80P_SECTION_NAME, SECTION_NAME_MAX_LENGTH) == 0
    );
}


bool Serializer::parse_section_name(
        std::string const& line,
        SectionName& section_name
) noexcept {
    std::string::const_iterator it = line.begin();
    std::string::const_iterator const end = line.end();
    size_t pos = 0;

    std::fill_n(section_name, SECTION_NAME_MAX_LENGTH, '\x00');

    if (skipping_whitespace_or_comment_reaches_the_end(it, end)) {
        return false;
    }

    if (*it != '[') {
        return false;
    }

    ++it;

    while (it != end && is_inline_whitespace(*it)) {
        ++it;
    }

    /* strlen() is not constexpr on some platforms. */
    size_t const section_name_pos_limit = strlen(JS80P_SECTION_NAME) + 1;

    while (it != end && is_section_name_char(*it)) {
        if (pos != section_name_pos_limit) {
            section_name[pos++] = *it;
        }

        ++it;
    }

    while (it != end && is_inline_whitespace(*it)) {
        ++it;
    }

    if (it == end || *it != ']') {
        return false;
    }

    ++it;

    return skipping_whitespace_or_comment_reaches_the_end(it, end);
}


bool Serializer::parse_line_until_value(
        std::string::const_iterator& it,
        std::string::const_iterator const& end,
        ParamName& param_name,
        Suffix& suffix
) noexcept {
    return (
        !skipping_whitespace_or_comment_reaches_the_end(it, end)
        && parse_param_name(it, end, param_name)
        && parse_suffix(it, end, suffix)
        && !skipping_whitespace_or_comment_reaches_the_end(it, end)
        && parse_equal_sign(it, end)
    );
}


void Serializer::process_line(
        std::vector<Synth::Message>& messages,
        Synth const& synth,
        std::string const& line
) noexcept {
    std::string::const_iterator it = line.begin();
    std::string::const_iterator const end = line.end();
    Synth::ParamId param_id;
    Number number;
    ParamName param_name;
    Suffix suffix;
    bool is_controller_assignment;

    if (
            !parse_line_until_value(it, end, param_name, suffix)
            || skipping_whitespace_or_comment_reaches_the_end(it, end)
            || !parse_number(it, end, number)
            || !skipping_whitespace_or_comment_reaches_the_end(
                it, end
            )
    ) {
        return;
    }

    is_controller_assignment = CONTROLLER_SUFFIX == suffix;

    upgrade_line(synth, param_name, number, is_controller_assignment);

    param_id = synth.get_param_id(param_name);

    if (
            param_id == Synth::ParamId::INVALID_PARAM_ID
            || (suffix[0] != '\x00' && !is_controller_assignment)
    ) {
        return;
    }

    if (is_controller_assignment) {
        messages.push_back(
            Synth::Message(
                Synth::MessageType::ASSIGN_CONTROLLER,
                param_id,
                0.0,
                (Byte)float_to_controller_id(number)
            )
        );
    } else {
        messages.push_back(
            Synth::Message(Synth::MessageType::SET_PARAM, param_id, number, 0)
        );
    }
}


void Serializer::upgrade_line(
        Synth const& synth,
        ParamName& param_name,
        Number& number,
        bool const is_controller_assignment
) noexcept {
    if (
            JS80P_UNLIKELY(
                strncmp(param_name, "POLY", PARAM_NAME_MAX_LENGTH) == 0
            )
    ) {
        strncpy(param_name, "NH", 3);
        number = upgrade_old_note_handling_param(synth, number);
    } else if (
            JS80P_UNLIKELY(param_name[0] == 'N' && is_digit(param_name[1]))
    ) {
        if (
                JS80P_UNLIKELY(
                    strncmp(
                        &param_name[2], "DYN", PARAM_NAME_MAX_LENGTH - 2
                    ) == 0
                )
        ) {
            strncpy(&param_name[2], "UPD", 4);
            number = upgrade_old_envelope_update_mode(synth, number);
        } else if (
                JS80P_UNLIKELY(
                    is_digit(param_name[2])
                    && strncmp(
                        &param_name[3], "DYN", PARAM_NAME_MAX_LENGTH - 3
                    ) == 0
                )
        ) {
            strncpy(&param_name[3], "UPD", 4);
            number = upgrade_old_envelope_update_mode(synth, number);
        }
    } else if (
            JS80P_UNLIKELY(
                strncmp(param_name, "MWAV", PARAM_NAME_MAX_LENGTH) == 0
                || strncmp(param_name, "CWAV", PARAM_NAME_MAX_LENGTH) == 0
            )
    ) {
        strncpy(&param_name[1], "WFM", 4);

        if (!is_controller_assignment) {
            number = upgrade_old_voice_oscillator_waveform(synth, number);
        }
    } else if (
            JS80P_UNLIKELY(
                strncmp(param_name, "L1WAV", PARAM_NAME_MAX_LENGTH) == 0
                || strncmp(param_name, "L3WAV", PARAM_NAME_MAX_LENGTH) == 0
                || strncmp(param_name, "L5WAV", PARAM_NAME_MAX_LENGTH) == 0
                || strncmp(param_name, "L7WAV", PARAM_NAME_MAX_LENGTH) == 0
            )
    ) {
        strncpy(&param_name[2], "WFM", 4);

        if (!is_controller_assignment) {
            number = upgrade_old_lfo_waveform(synth, number);
        }
    } else if (
            JS80P_UNLIKELY(
                strncmp(param_name, "CDT", PARAM_NAME_MAX_LENGTH) == 0
            )
    ) {
        strncpy(param_name, "CDTP", 5);
        number = upgrade_old_distortion_type(
            synth, number, Synth::ParamId::CDTYP
        );
    } else if (
            JS80P_UNLIKELY(
                strncmp(param_name, "EOT", PARAM_NAME_MAX_LENGTH) == 0
            )
    ) {
        strncpy(param_name, "EOTP", 5);
        number = upgrade_old_distortion_type(
            synth, number, Synth::ParamId::ED1TYP
        );
    } else if (
            JS80P_UNLIKELY(
                strncmp(param_name, "EDT", PARAM_NAME_MAX_LENGTH) == 0
            )
    ) {
        strncpy(param_name, "EDTP", 5);
        number = upgrade_old_distortion_type(
            synth, number, Synth::ParamId::ED2TYP
        );
    } else if (
            JS80P_UNLIKELY(
                strncmp(param_name, "ETSTYP", PARAM_NAME_MAX_LENGTH) == 0
            )
    ) {
        strncpy(param_name, "ETDTP", 6);
        number = upgrade_old_distortion_type(
            synth, number, Synth::ParamId::ETSTYP
        );
    }
}


Number Serializer::upgrade_old_note_handling_param(
        Synth const& synth,
        Number const old_value
) noexcept {
    constexpr Byte NEW_VALUES[] = {
        Synth::NOTE_HANDLING_MONO,
        Synth::NOTE_HANDLING_MONO_HOLD,
        Synth::NOTE_HANDLING_POLY_HOLD,
        Synth::NOTE_HANDLING_POLY,
    };

    Byte const old_value_byte = std::round(old_value * 3.0);

    if (JS80P_UNLIKELY(old_value_byte > 3)) {
        return synth.get_param_default_ratio(Synth::ParamId::NH);
    }

    return synth.discrete_param_value_to_ratio(
        Synth::ParamId::NH,
        NEW_VALUES[old_value_byte]
    );
}


Number Serializer::upgrade_old_envelope_update_mode(
        Synth const& synth,
        Number const old_value
) noexcept {
    constexpr Byte NEW_VALUES[] = {
        Envelope::UPDATE_MODE_STATIC,
        Envelope::UPDATE_MODE_END,
        Envelope::UPDATE_MODE_DYNAMIC,
    };

    Byte const old_value_byte = std::round(old_value * 2.0);

    if (JS80P_UNLIKELY(old_value_byte > 2)) {
        return synth.get_param_default_ratio(Synth::ParamId::N1UPD);
    }

    return synth.discrete_param_value_to_ratio(
        Synth::ParamId::N1UPD,
        NEW_VALUES[old_value_byte]
    );
}


Number Serializer::upgrade_old_voice_oscillator_waveform(
        Synth const& synth,
        Number const old_value
) noexcept {
    constexpr Byte NEW_VALUES[] = {
        SimpleOscillator::SINE,
        SimpleOscillator::SAWTOOTH,
        SimpleOscillator::SOFT_SAWTOOTH,
        SimpleOscillator::INVERSE_SAWTOOTH,
        SimpleOscillator::SOFT_INVERSE_SAWTOOTH,
        SimpleOscillator::TRIANGLE,
        SimpleOscillator::SOFT_TRIANGLE,
        SimpleOscillator::SQUARE,
        SimpleOscillator::SOFT_SQUARE,
        SimpleOscillator::CUSTOM,
    };

    Byte const old_value_byte = std::round(old_value * 9.0);

    if (JS80P_UNLIKELY(old_value_byte > 9)) {
        return synth.get_param_default_ratio(Synth::ParamId::MWFM);
    }

    return synth.discrete_param_value_to_ratio(
        Synth::ParamId::MWFM,
        NEW_VALUES[old_value_byte]
    );
}


Number Serializer::upgrade_old_lfo_waveform(
        Synth const& synth,
        Number const old_value
) noexcept {
    constexpr Byte NEW_VALUES[] = {
        SimpleOscillator::SINE,
        SimpleOscillator::SAWTOOTH,
        SimpleOscillator::SOFT_SAWTOOTH,
        SimpleOscillator::INVERSE_SAWTOOTH,
        SimpleOscillator::SOFT_INVERSE_SAWTOOTH,
        SimpleOscillator::TRIANGLE,
        SimpleOscillator::SOFT_TRIANGLE,
        SimpleOscillator::SQUARE,
        SimpleOscillator::SOFT_SQUARE,
    };

    Byte const old_value_byte = std::round(old_value * 8.0);

    if (JS80P_UNLIKELY(old_value_byte > 8)) {
        return synth.get_param_default_ratio(Synth::ParamId::L1WAV);
    }

    return synth.discrete_param_value_to_ratio(
        Synth::ParamId::L1WAV,
        NEW_VALUES[old_value_byte]
    );
}


Number Serializer::upgrade_old_distortion_type(
        Synth const& synth,
        Number const old_value,
        Synth::ParamId const param_id
) noexcept {
    constexpr Byte NEW_VALUES[] = {
        Distortion::TYPE_TANH_3,
        Distortion::TYPE_TANH_5,
        Distortion::TYPE_TANH_10,
        Distortion::TYPE_HARMONIC_13,
        Distortion::TYPE_HARMONIC_15,
        Distortion::TYPE_HARMONIC_135,
        Distortion::TYPE_HARMONIC_SQR,
        Distortion::TYPE_HARMONIC_TRI,
        Distortion::TYPE_BIT_CRUSH_1,
        Distortion::TYPE_BIT_CRUSH_2,
        Distortion::TYPE_BIT_CRUSH_3,
        Distortion::TYPE_BIT_CRUSH_4,
        Distortion::TYPE_BIT_CRUSH_4_6,
        Distortion::TYPE_BIT_CRUSH_5,
        Distortion::TYPE_BIT_CRUSH_5_6,
        Distortion::TYPE_BIT_CRUSH_6,
        Distortion::TYPE_BIT_CRUSH_6_6,
        Distortion::TYPE_BIT_CRUSH_7,
        Distortion::TYPE_BIT_CRUSH_7_6,
        Distortion::TYPE_BIT_CRUSH_8,
        Distortion::TYPE_BIT_CRUSH_8_6,
        Distortion::TYPE_BIT_CRUSH_9,
        Distortion::TYPE_DELAY_FEEDBACK,
    };

    Byte const old_value_byte = std::round(old_value * 22.0);

    if (JS80P_UNLIKELY(old_value_byte > 22)) {
        return synth.get_param_default_ratio(param_id);
    }

    return synth.discrete_param_value_to_ratio(
        param_id, NEW_VALUES[old_value_byte]
    );
}


bool Serializer::skipping_whitespace_or_comment_reaches_the_end(
        std::string::const_iterator& it,
        std::string::const_iterator const& end
) noexcept {
    if (it == end) {
        return true;
    }

    while (is_inline_whitespace(*it)) {
        ++it;

        if (it == end) {
            return true;
        }
    }

    if (is_comment_leader(*it)) {
        it = end;

        return true;
    }

    return false;
}


bool Serializer::parse_param_name(
        std::string::const_iterator& it,
        std::string::const_iterator const& end,
        ParamName& param_name
) noexcept {
    constexpr size_t param_name_pos_max = PARAM_NAME_MAX_LENGTH - 1;
    size_t param_name_pos = 0;

    std::fill_n(param_name, PARAM_NAME_MAX_LENGTH, '\x00');

    while (
            is_capital_letter(*it) || is_digit(*it) || is_lowercase_letter(*it)
    ) {
        if (
                strncmp(
                    &(*it),
                    CONTROLLER_SUFFIX.c_str(),
                    CONTROLLER_SUFFIX.length()
                ) == 0
        ) {
            break;
        }

        param_name[param_name_pos++] = toupper(*(it++));

        if (param_name_pos == param_name_pos_max || it == end) {
            return false;
        }
    }

    return param_name_pos > 0;
}


bool Serializer::parse_suffix(
        std::string::const_iterator& it,
        std::string::const_iterator const& end,
        Suffix& suffix
) noexcept {
    size_t suffix_pos = 0;

    if (it == end) {
        return false;
    }

    std::fill_n(suffix, SUFFIX_MAX_LENGTH, '\x00');

    while (is_lowercase_letter(*it)) {
        suffix[suffix_pos++] = *(it++);

        if (suffix_pos >= SUFFIX_MAX_LENGTH || it == end) {
            return false;
        }
    }

    return true;
}


bool Serializer::parse_equal_sign(
        std::string::const_iterator& it,
        std::string::const_iterator const& end
) noexcept {
    if (*it != '=') {
        return false;
    }

    ++it;

    return true;
}


bool Serializer::parse_number(
        std::string::const_iterator& it,
        std::string::const_iterator const& end,
        Number& number
) noexcept {
    std::string number_text("");
    bool has_dot = false;

    while (it != end) {
        /*
        Decimal separator was locale-dependent before v4.1.1 - let's try loading
        those broken serializations as well.
        */
        bool const is_comma = *it == ',';

        if (is_comma || *it == '.') {
            if (has_dot) {
                return false;
            }

            has_dot = true;
        } else if (!is_digit(*it)) {
            break;
        }

        number_text += is_comma ? '.' : *it;
        ++it;
    }

    if (number_text.length() == 0) {
        return false;
    }

    number = std::clamp(to_number(number_text), 0.0, 1.0);

    return true;
}


Number Serializer::to_number(std::string const& text) noexcept
{
    std::istringstream s(text);
    Number n;

    s.imbue(std::locale::classic());
    s >> n;

    return n;
}

}

#endif
