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

#include <cerrno>
#include <cstring>
#include <istream>
#include <iostream>
#include <fstream>
#include <ostream>

#include "js80p.hpp"
#include "serializer.hpp"
#include "synth.hpp"


bool read_patch(char const* file_path, std::string& result)
{
    std::ifstream patch_file(file_path, std::ios::in | std::ios::binary);

    if (!patch_file.is_open()) {
        return false;
    }

    char* buffer = new char[JS80P::Serializer::MAX_SIZE];

    std::fill_n(buffer, JS80P::Serializer::MAX_SIZE, '\x00');
    patch_file.read(buffer, JS80P::Serializer::MAX_SIZE);

    result = buffer;

    delete[] buffer;

    return true;
}


bool write_patch(char const* file_path, std::string const& patch)
{
    std::ofstream patch_file(file_path, std::ios::out | std::ios::binary);

    if (!patch_file.is_open()) {
        return false;
    }

    patch_file.write(patch.c_str(), patch.length());

    return true;
}


int error(char const* message, char const* file_path)
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
    std::cerr << "Upgrading " << patch_file << std::endl;

    std::string patch;

    if (!read_patch(patch_file, patch)) {
        return error("Error reading patch file", patch_file);
    }

    JS80P::Synth synth;
    JS80P::Serializer::import(synth, patch);

    synth.process_messages();

    if (!write_patch(patch_file, JS80P::Serializer::serialize(synth))) {
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
