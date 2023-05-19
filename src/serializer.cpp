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
#include <sstream>

#include "serializer.hpp"


namespace JS80P
{

std::string Serializer::serialize(Synth const* synth) noexcept
{
    constexpr size_t line_size = 128;
    char line[line_size];
    std::string serialized("[js80p]\r\n");

    for (int i = 0; i != Synth::ParamId::MAX_PARAM_ID; ++i) {
        Synth::ParamId const param_id = (Synth::ParamId)i;
        std::string const param_name = synth->get_param_name(param_id);

        if (param_name.length() > 0) {
            snprintf(
                line,
                line_size,
                "%s = %.15f\r\n",
                param_name.c_str(),
                synth->get_param_ratio_atomic(param_id)
            );
            serialized += line;

            snprintf(
                line,
                line_size,
                "%sctl = %.15f\r\n",
                param_name.c_str(),
                controller_id_to_float(
                    synth->get_param_controller_id_atomic(param_id)
                )
            );
            serialized += line;
        }
    }

    return serialized;
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


void Serializer::import(Synth* synth, std::string const& serialized) noexcept
{
    reset_all_params_to_default(synth);
    std::vector<std::string>* lines = parse_lines(serialized);
    process_lines(synth, lines);

    delete lines;
}


void Serializer::reset_all_params_to_default(Synth* synth) noexcept
{
    for (int i = 0; i != Synth::ParamId::MAX_PARAM_ID; ++i) {
        Synth::ParamId const param_id = (Synth::ParamId)i;

        synth->push_message(
            Synth::MessageType::SET_PARAM,
            param_id,
            synth->get_param_default_ratio(param_id),
            0
        );
        synth->push_message(
            Synth::MessageType::ASSIGN_CONTROLLER,
            param_id,
            0,
            (Byte)Synth::ControllerId::NONE
        );
    }
}


std::vector<std::string>* Serializer::parse_lines(
        std::string const& serialized
) noexcept {
    constexpr Integer max_line_pos = MAX_SIZE - 1;
    std::vector<std::string>* lines = new std::vector<std::string>();
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


void Serializer::process_lines(Synth* synth, std::vector<std::string>* lines) noexcept
{
    typedef std::vector<Synth::Message> Messages;

    Messages messages;
    char section_name[7];
    bool inside_js80p_section = false;

    messages.reserve(600);

    for (std::vector<std::string>::iterator it = lines->begin(); it != lines->end(); ++it) {
        std::string line = *it;

        if (parse_section_name(line, section_name)) {
            inside_js80p_section = false;

            if (strncmp(section_name, "js80p", 7) == 0) {
                inside_js80p_section = true;
                continue;
            }
        } else if (inside_js80p_section) {
            process_line(messages, synth, line);
        }
    }

    for (Messages::const_iterator it = messages.begin(); it != messages.end(); ++it) {
        if (it->param_id > Synth::ParamId::L1SYN) {
            synth->push_message(*it);
        }
    }

    for (Messages::const_iterator it = messages.begin(); it != messages.end(); ++it) {
        if (it->param_id <= Synth::ParamId::L1SYN) {
            synth->push_message(*it);
        }
    }
}


bool Serializer::parse_section_name(
        std::string const line,
        char* section_name
) noexcept {
    std::string::const_iterator it = line.begin();
    std::string::const_iterator const end = line.end();
    Integer pos = 0;

    std::fill_n(section_name, 7, '\x00');

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


void Serializer::process_line(
        std::vector<Synth::Message>& messages,
        Synth* synth,
        std::string const line
) noexcept {
    std::string::const_iterator it = line.begin();
    std::string::const_iterator const end = line.end();
    Synth::ParamId param_id;
    Number number;
    char param_name[Constants::PARAM_NAME_MAX_LENGTH];
    char suffix[4];
    bool is_controller_assignment;

    if (
            skipping_remaining_whitespace_or_comment_reaches_the_end(it, end)
            || !parse_param_name(it, end, &param_name[0])
            || !parse_suffix(it, end, &suffix[0])
            || skipping_remaining_whitespace_or_comment_reaches_the_end(it, end)
            || !parse_equal_sign(it, end)
            || skipping_remaining_whitespace_or_comment_reaches_the_end(it, end)
            || !parse_number(it, end, number)
            || !skipping_remaining_whitespace_or_comment_reaches_the_end(it, end)
    ) {
        return;
    }

    param_id = synth->get_param_id(param_name);
    is_controller_assignment = strncmp(suffix, "ctl", 4) == 0;

    if (
            param_id == Synth::ParamId::MAX_PARAM_ID
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
        std::string::const_iterator const end
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
        std::string::const_iterator const end,
        char* param_name
) noexcept {
    constexpr Integer param_name_pos_max = Constants::PARAM_NAME_MAX_LENGTH - 1;
    Integer param_name_pos = 0;

    std::fill_n(param_name, Constants::PARAM_NAME_MAX_LENGTH, '\x00');

    while (is_capital_letter(*it) || is_digit(*it)) {
        param_name[param_name_pos++] = *(it++);

        if (param_name_pos == param_name_pos_max || it == end) {
            return false;
        }
    }

    return param_name_pos > 0;
}


bool Serializer::parse_suffix(
        std::string::const_iterator& it,
        std::string::const_iterator const end,
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
        std::string::const_iterator const end
) noexcept {
    if (*it != '=') {
        return false;
    }

    ++it;

    return it != end;
}


bool Serializer::parse_number(
        std::string::const_iterator& it,
        std::string::const_iterator const end,
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


Number Serializer::to_number(std::string const text) noexcept
{
    std::istringstream s(text);
    Number n;

    s >> n;

    return n;
}

}

#endif
