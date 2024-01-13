/*
 * This file is part of JS80P, a synthesizer plugin.
 * Copyright (C) 2023, 2024  Attila M. Magyar
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

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <ostream>
#include <string>
#include <vector>

#include "js80p.hpp"

#include "bank.hpp"
#include "midi.hpp"
#include "renderer.hpp"
#include "serializer.hpp"
#include "synth.hpp"


using namespace JS80P;


constexpr double SIGNED_24BIT_MAX = 8388607.0;

constexpr size_t BUFFER_SIZE = 8192;
constexpr size_t BLOCK_SIZE = 1024;

constexpr Midi::Byte VELOCITY_DECREASE = 5;

constexpr Frequency SAMPLE_RATE = 44100.0;
constexpr Seconds NOTE_START = 0.1;
constexpr Seconds NOTE_GAP = 1.0;
constexpr Seconds NOTE_END = 35.0;
constexpr Seconds LENGTH = 60.0;
constexpr Integer ROUNDS = (
    (Integer)(LENGTH * SAMPLE_RATE / (Number)BLOCK_SIZE) + 1
);

constexpr int WAV_RIFF_ID = 0x46464952;     /* "RIFF" */
constexpr int WAV_FORMAT_ID = 0x20746d66;   /* "fmt " */
constexpr int WAV_WAVE_ID = 0x45564157;     /* "WAVE" */
constexpr int WAV_DATA_ID = 0x61746164;     /* "data" */

constexpr int WAV_FORMAT_TAG = 1;       /* no compression */
constexpr size_t WAV_CHANNELS = (size_t)Synth::OUT_CHANNELS;
constexpr size_t WAV_BYTES_PER_SAMPLE = 3;
constexpr size_t WAV_BITS_PER_SAMPLE = WAV_BYTES_PER_SAMPLE * 8;
constexpr size_t WAV_BYTES_PER_SEC = (
    WAV_CHANNELS * WAV_BYTES_PER_SAMPLE * (size_t)SAMPLE_RATE
);
constexpr size_t WAV_BLOCK_ALIGN = WAV_CHANNELS * WAV_BYTES_PER_SAMPLE;
constexpr size_t WAV_DATA_SIZE = (
    (size_t)ROUNDS * BLOCK_SIZE * WAV_CHANNELS * WAV_BYTES_PER_SAMPLE
);
constexpr size_t WAV_RIFF_SIZE = 36 + WAV_DATA_SIZE;
constexpr size_t WAV_FORMAT_SIZE = 16;


class WavBuffer
{
    public:
        WavBuffer() : buffer_pos(0) {}

        size_t get_buffer_pos() const
        {
            return buffer_pos;
        }

        char const* get_buffer() const
        {
            return buffer;
        }

        void clear()
        {
            buffer_pos = 0;
        }

        bool append8(char const byte)
        {
            if (JS80P_UNLIKELY(buffer_pos >= BUFFER_SIZE)) {
                return false;
            }

            buffer[buffer_pos++] = byte;

            return true;
        }

        bool append16(uint16_t word)
        {
            return (
                append8(word & 0xff)
                && append8(word >> 8)
            );
        }

        bool append24(uint32_t dword)
        {
            return (
                append8(dword & 0xff)
                && append8((dword >> 8) & 0xff)
                && append8((dword >> 16) & 0xff)
            );
        }

        bool append32(uint32_t dword)
        {
            return (
                append8(dword & 0xff)
                && append8((dword >> 8) & 0xff)
                && append8((dword >> 16) & 0xff)
                && append8((dword >> 24) & 0xff)
            );
        }

    private:
        char buffer[BUFFER_SIZE];
        size_t buffer_pos;
};


void usage(char const* name)
{
    fprintf(stderr, "Usage: valgrind --tool=callgrind %s program velocity out.wav\n", name);
    fprintf(stderr, "\n");
    fprintf(stderr, "    program    preset number (0-%d)\n", (int)Bank::NUMBER_OF_PROGRAMS - 1);
    fprintf(stderr, "    velocity   first note's velocity (0-127)\n");
    fprintf(stderr, "    out.wav    output file\n");
}


uint32_t sample_to_wav(Sample const sample)
{
    if (JS80P_UNLIKELY(sample > 1.0)) {
        return (uint32_t)SIGNED_24BIT_MAX;
    } else if (JS80P_UNLIKELY(sample < -1.0)) {
        return (uint32_t)-SIGNED_24BIT_MAX;
    }

    return (uint32_t)(SIGNED_24BIT_MAX * sample);
}


void write_wav_header(WavBuffer& buffer)
{
    /* RIFF chunk */
    buffer.append32(WAV_RIFF_ID);
    buffer.append32(WAV_RIFF_SIZE);
    buffer.append32(WAV_WAVE_ID);

    /* Format sub-chunk */
    buffer.append32(WAV_FORMAT_ID);
    buffer.append32(WAV_FORMAT_SIZE);
    buffer.append16(WAV_FORMAT_TAG);
    buffer.append16(WAV_CHANNELS);
    buffer.append32((size_t)SAMPLE_RATE);
    buffer.append32(WAV_BYTES_PER_SEC);
    buffer.append16(WAV_BLOCK_ALIGN);
    buffer.append16(WAV_BITS_PER_SAMPLE);

    /* Data sub-chunk */
    buffer.append32(WAV_DATA_ID);
    buffer.append32(WAV_DATA_SIZE);
}


void render_sound(
        size_t const program_index,
        Midi::Byte const initial_velocity,
        std::ofstream& out_file
) {
    Synth synth;
    Bank bank;
    WavBuffer buffer;
    Renderer renderer(synth);
    Sample* samples[Synth::OUT_CHANNELS];
    std::vector<Midi::Note> notes = {
        Midi::NOTE_C_2,
        Midi::NOTE_C_3,
        Midi::NOTE_G_3,
        Midi::NOTE_C_4,
        Midi::NOTE_E_FLAT_4,
        Midi::NOTE_G_4,
        Midi::NOTE_C_5,
        Midi::NOTE_E_FLAT_5,
        Midi::NOTE_G_5,
        Midi::NOTE_B_FLAT_5,
        Midi::NOTE_C_6
    };
    Seconds note_start = NOTE_START;
    Seconds note_end = NOTE_END;
    Midi::Byte mod_wheel = 0;
    Midi::Byte channel_pressure = 0;
    Midi::Byte velocity = initial_velocity;

    for (Integer i = 0; i != Synth::OUT_CHANNELS; ++i) {
        samples[i] = new Sample[BLOCK_SIZE];
    }

    Serializer::import_patch_in_audio_thread(synth, bank[program_index].serialize());

    write_wav_header(buffer);
    out_file.write(buffer.get_buffer(), buffer.get_buffer_pos());
    buffer.clear();

    synth.suspend();
    synth.set_block_size(BLOCK_SIZE);
    synth.set_sample_rate(SAMPLE_RATE);
    synth.resume();
    synth.process_messages();

    for (std::vector<Midi::Note>::const_iterator it = notes.begin(); it != notes.end(); ++it) {
        if (velocity > 0) {
            synth.note_on(note_start, 1, *it, velocity);
            synth.note_off(note_end, 1, *it, velocity);
        }

        note_start += NOTE_GAP;
        note_end += NOTE_GAP;
        velocity = velocity > VELOCITY_DECREASE ? velocity - VELOCITY_DECREASE : 0;
    }

    for (Integer r = 0; r != ROUNDS; ++r) {
        if (JS80P_UNLIKELY((r & 7) == 0)) {
            if (mod_wheel < 127) {
                ++mod_wheel;
                synth.control_change(0.0, 1, Midi::MODULATION_WHEEL, mod_wheel);
            }

            if (mod_wheel > 96 && channel_pressure < 127) {
                ++channel_pressure;
                synth.channel_pressure(0.0, 1, channel_pressure);
            }
        }

        renderer.render<Sample>((Integer)BLOCK_SIZE, samples);

        for (size_t i = 0; i != BLOCK_SIZE; ++i) {
            buffer.append24(sample_to_wav(samples[0][i]));
            buffer.append24(sample_to_wav(samples[1][i]));
        }

        out_file.write(buffer.get_buffer(), buffer.get_buffer_pos());
        buffer.clear();
    }

    for (Integer i = 0; i != Synth::OUT_CHANNELS; ++i) {
        delete[] samples[i];
        samples[i] = NULL;
    }
}


int main(int const argc, char const* argv[])
{
    if (argc < 4) {
        usage(argv[0]);
        return 1;
    }

    int const program_index = atoi(argv[1]);
    int const velocity = atoi(argv[2]);
    std::string const out_file_name(argv[3]);

    if (program_index < 0 || program_index >= (int)Bank::NUMBER_OF_PROGRAMS) {
        fprintf(
            stderr,
            "ERROR: invalid program number, must be between 0 and %d, got: %d (interpreted from \"%s\")\n\n",
            (int)Bank::NUMBER_OF_PROGRAMS - 1,
            program_index,
            argv[1]
        );
        return 2;
    }

    if (velocity < 0 || velocity > 127) {
        fprintf(
            stderr,
            "ERROR: invalid velocity, must be between 0 and 127, got: %d (interpreted from \"%s\")\n\n",
            velocity,
            argv[2]
        );
        return 3;
    }

    if (out_file_name.length() < 1) {
        fprintf(stderr, "ERROR: output file name must not be empty\n");
        return 4;
    }

    std::ofstream out_file(out_file_name, std::ios::out | std::ios::binary);

    if (!out_file.is_open()) {
        char const* error_msg = strerror(errno);

        fprintf(
            stderr,
            "ERROR: unable to open output file \"%s\": errno=%d, error=\"%s\"\n",
            out_file_name.c_str(),
            errno,
            error_msg == NULL ? "<NULL>" : error_msg
        );

        return 5;
    }

    render_sound((size_t)program_index, (Midi::Byte)velocity, out_file);

    return 0;
}

