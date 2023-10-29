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

#ifndef JS80P__SERIALIZER_CPP
#define JS80P__SERIALIZER_CPP

#include <algorithm>
#include <cstring>
#include <cctype>
#include <sstream>

#include "serializer.hpp"


namespace JS80P
{

std::string const Serializer::CONTROLLER_SUFFIX = "ctl";

std::string const Serializer::LINE_END = "\r\n";


std::string Serializer::serialize(Synth const& synth) noexcept
{
    constexpr size_t line_size = 128;
    char line[line_size];
    std::string serialized("");

    serialized.reserve(MAX_SIZE);
    serialized += "[";
    serialized += JS80P_SECTION_NAME;
    serialized += "]";
    serialized += LINE_END;

    for (int i = 0; i != Synth::ParamId::PARAM_ID_COUNT; ++i) {
        Synth::ParamId const param_id = (Synth::ParamId)i;
        std::string const param_name = synth.get_param_name(param_id);

        if (param_name.length() > 0) {
            Synth::ControllerId const controller_id = (
                synth.get_param_controller_id_atomic(param_id)
            );

            if (controller_id == Synth::ControllerId::NONE) {
                Number const set_ratio = synth.get_param_ratio_atomic(param_id);
                Number const default_ratio = synth.get_param_default_ratio(param_id);

                if (std::fabs(default_ratio - set_ratio) > 0.000001) {
                    int const length = snprintf(
                        line,
                        line_size,
                        "%s = %.15f",
                        param_name.c_str(),
                        set_ratio
                    );
                    trim_excess_zeros_from_end_after_snprintf(line, length, line_size);
                    serialized += line;
                    serialized += LINE_END;
                }
            } else {
                int const length = snprintf(
                    line,
                    line_size,
                    "%s%s = %.15f",
                    param_name.c_str(),
                    CONTROLLER_SUFFIX.c_str(),
                    controller_id_to_float(controller_id)
                );
                trim_excess_zeros_from_end_after_snprintf(line, length, line_size);
                serialized += line;
                serialized += LINE_END;
            }
        }
    }

    return serialized;
}


void Serializer::trim_excess_zeros_from_end_after_snprintf(
        char* number,
        int const length,
        size_t const max_length
) noexcept {
    if (UNLIKELY(length < 1 || max_length < 1)) {
        return;
    }

    size_t dot_index = max_length;

    for (size_t i = 0; number[i] != '\x00' && i != max_length; ++i) {
        if (number[i] == '.') {
            dot_index = i;

            break;
        }
    }

    if (UNLIKELY(dot_index == max_length)) {
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
void Serializer::import_patch(Synth& synth, std::string const& serialized) noexcept
{
    Lines* lines = parse_lines(serialized);
    process_lines<thread>(synth, lines);

    delete lines;
}


Serializer::Lines* Serializer::parse_lines(std::string const& serialized) noexcept
{
    constexpr Integer max_line_pos = MAX_SIZE - 1;
    Lines* lines = new Lines();
    char* line = new char[MAX_SIZE];
    std::string::const_iterator const end = serialized.end();
    Integer line_pos = 0;
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
void Serializer::process_lines(Synth& synth, Lines* lines) noexcept
{
    typedef std::vector<Synth::Message> Messages;

    Messages messages;
    char section_name[8];
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

    for (Messages::const_iterator it = messages.begin(); it != messages.end(); ++it) {
        if (synth.is_toggle_param(it->param_id)) {
            send_message<thread>(synth, *it);
        }
    }

    for (Messages::const_iterator it = messages.begin(); it != messages.end(); ++it) {
        if (!synth.is_toggle_param(it->param_id)) {
            send_message<thread>(synth, *it);
        }
    }
}


template<Serializer::Thread thread>
void Serializer::send_message(Synth& synth, Synth::Message const& message) noexcept
{
    if constexpr (thread == Thread::AUDIO) {
        synth.process_message(message);
    } else {
        synth.push_message(message);
    }
}


bool Serializer::is_js80p_section_start(char const section_name[8]) noexcept
{
    return strncmp(section_name, JS80P_SECTION_NAME, 8) == 0;
}


bool Serializer::parse_section_name(
        std::string const& line,
        char section_name[8]
) noexcept {
    std::string::const_iterator it = line.begin();
    std::string::const_iterator const end = line.end();
    Integer pos = 0;

    std::fill_n(section_name, 8, '\x00');

    if (skipping_remaining_whitespace_or_comment_reaches_the_end(it, end)) {
        return false;
    }

    if (*it != '[') {
        return false;
    }

    ++it;

    while (it != end && is_inline_whitespace(*it)) {
        ++it;
    }

    while (it != end && is_section_name_char(*it)) {
        if (pos != 6) {
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

    return skipping_remaining_whitespace_or_comment_reaches_the_end(it, end);
}


bool Serializer::parse_line_until_value(
        std::string::const_iterator& it,
        std::string::const_iterator const& end,
        char param_name[Constants::PARAM_NAME_MAX_LENGTH],
        char suffix[4]
) noexcept {
    return (
        !skipping_remaining_whitespace_or_comment_reaches_the_end(it, end)
        && parse_param_name(it, end, param_name)
        && parse_suffix(it, end, suffix)
        && !skipping_remaining_whitespace_or_comment_reaches_the_end(it, end)
        && parse_equal_sign(it, end)
    );
}


void Serializer::process_line(
        std::vector<Synth::Message>& messages,
        Synth& synth,
        std::string const& line
) noexcept {
    std::string::const_iterator it = line.begin();
    std::string::const_iterator const end = line.end();
    Synth::ParamId param_id;
    Number number;
    char param_name[Constants::PARAM_NAME_MAX_LENGTH];
    char suffix[4];
    bool is_controller_assignment;

    if (
            !parse_line_until_value(it, end, param_name, suffix)
            || skipping_remaining_whitespace_or_comment_reaches_the_end(it, end)
            || !parse_number(it, end, number)
            || !skipping_remaining_whitespace_or_comment_reaches_the_end(it, end)
    ) {
        return;
    }

    param_id = synth.get_param_id(param_name);
    is_controller_assignment = CONTROLLER_SUFFIX == suffix;

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


bool Serializer::skipping_remaining_whitespace_or_comment_reaches_the_end(
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
        while (it != end) {
            ++it;
        }

        return true;
    }

    return false;
}


bool Serializer::parse_param_name(
        std::string::const_iterator& it,
        std::string::const_iterator const& end,
        char* param_name
) noexcept {
    constexpr Integer param_name_pos_max = Constants::PARAM_NAME_MAX_LENGTH - 1;
    Integer param_name_pos = 0;

    std::fill_n(param_name, Constants::PARAM_NAME_MAX_LENGTH, '\x00');

    while (is_capital_letter(*it) || is_digit(*it) || is_lowercase_letter(*it)) {
        if (strncmp(&(*it), CONTROLLER_SUFFIX.c_str(), CONTROLLER_SUFFIX.length()) == 0) {
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
        char* suffix
) noexcept {
    Integer suffix_pos = 0;

    if (it == end) {
        return false;
    }

    std::fill_n(suffix, 4, '\x00');

    while (is_lowercase_letter(*it)) {
        suffix[suffix_pos++] = *(it++);

        if (suffix_pos >= 4 || it == end) {
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
    std::string number_text;
    bool has_dot = false;

    while (it != end) {
        if (*it == '.') {
            if (has_dot) {
                return false;
            }

            has_dot = true;
        } else if (!is_digit(*it)) {
            break;
        }

        number_text += *(it++);
    }

    if (number_text.length() == 0) {
        return false;
    }

    Number const parsed_number = std::min(
        1.0, std::max(0.0, to_number(number_text))
    );

    number = parsed_number;

    return true;
}


Number Serializer::to_number(std::string const& text) noexcept
{
    std::istringstream s(text);
    Number n;

    s >> n;

    return n;
}

}

#endif
