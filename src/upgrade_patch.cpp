/*
 * This file is part of JS80P, a synthesizer plugin.
 * Copyright (C) 2023, 2024, 2025  Attila M. Magyar
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

#include <cerrno>
#include <cstring>
#include <istream>
#include <iostream>
#include <fstream>
#include <ostream>

#include "js80p.hpp"
#include "serializer.hpp"
#include "synth.hpp"


bool read_patch(char const* const file_path, std::string& result)
{
    std::ifstream patch_file(file_path, std::ios::in | std::ios::binary);

    if (!patch_file.is_open()) {
        return false;
    }

    char* const buffer = new char[JS80P::Serializer::MAX_SIZE];

    std::fill_n(buffer, JS80P::Serializer::MAX_SIZE, '\x00');
    patch_file.read(buffer, JS80P::Serializer::MAX_SIZE);

    result = buffer;

    delete[] buffer;

    return true;
}


bool is_whole_line_comment_or_white_space(std::string const& line)
{
    std::string::const_iterator it = line.begin();

    return JS80P::Serializer::skipping_remaining_whitespace_or_comment_reaches_the_end(
        it, line.end()
    );
}


void collect_comments(std::string const& patch, JS80P::Serializer::Lines& comments)
{
    JS80P::Serializer::Lines* const lines = JS80P::Serializer::parse_lines(patch);

    for (JS80P::Serializer::Lines::const_iterator it = lines->begin(); it != lines->end(); ++it) {
        std::string const& line = *it;

        if (is_whole_line_comment_or_white_space(line)) {
            comments.push_back(line);
        }
    }
}


bool write_patch(
        char const* const file_path,
        std::string const& patch,
        JS80P::Serializer::Lines const& comments
) {
    std::ofstream patch_file(file_path, std::ios::out | std::ios::binary);

    if (!patch_file.is_open()) {
        return false;
    }

    std::string const line_end = JS80P::Serializer::LINE_END;

    for (JS80P::Serializer::Lines::const_iterator it = comments.begin(); it != comments.end(); ++it) {
        std::string const& comment = *it;
        std::cout << "c: " << comment << std::endl;

        patch_file.write(comment.c_str(), comment.length());
        patch_file.write(line_end.c_str(), line_end.length());
    }

    patch_file.write(patch.c_str(), patch.length());

    return true;
}


int error(char const* const message, char const* const file_path)
{
    std::cerr
        << "ERROR: "
        << message
        << std::endl
        << "  File: " << file_path << std::endl
        << "  Errno: " << errno << std::endl
        << "  Message: " << std::strerror(errno) << std::endl;

    return 1;
}


int upgrade_patch(char const* const patch_file)
{
    std::cout << "Upgrading " << patch_file << std::endl;

    std::string patch;

    if (!read_patch(patch_file, patch)) {
        return error("Error reading patch file", patch_file);
    }

    JS80P::Synth synth;
    JS80P::Serializer::Lines comments;

    JS80P::Serializer::import_patch_in_audio_thread(synth, patch);

    collect_comments(patch, comments);

    if (!write_patch(patch_file, JS80P::Serializer::serialize(synth), comments)) {
        return error("Error writing patch file", patch_file);
    }

    return 0;
}


int main(int argc, char const* argv[])
{
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " patch_file.js80p" << std::endl;

        return 1;
    }

    return upgrade_patch(argv[1]);
}
