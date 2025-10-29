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

#ifndef JS80P__MTSESP_HPP
#define JS80P__MTSESP_HPP

#include <mtsesp/Client/libMTSClient.h>

#include "js80p.hpp"
#include "midi.hpp"
#include "synth.hpp"


namespace JS80P
{

class MtsEsp
{
    public:
        explicit MtsEsp(Synth& synth) noexcept : synth(synth)
        {
            client = MTS_RegisterClient();
        }

        ~MtsEsp()
        {
            MTS_DeregisterClient(client);
        }

        void update_connection_status()
        {
            if (!synth.has_mts_esp_tuning()) {
                return;
            }

            if (MTS_HasMaster(client)) {
                synth.mts_esp_connected();
            } else {
                synth.mts_esp_disconnected();
            }
        }

        void update_note_tuning(Midi::Channel const channel, Midi::Note const note)
        {
            if (!synth.has_mts_esp_tuning()) {
                return;
            }

            Synth::NoteTuning tuning(channel, note);

            update_frequency(tuning);
            synth.update_note_tuning(tuning);
        }

        void update_active_notes_tuning()
        {
            if (!synth.has_continuous_mts_esp_tuning()) {
                return;
            }

            Integer count;
            Synth::NoteTunings& tunings = synth.collect_active_notes(count);

            for (Integer i = 0; i != count; ++i) {
                update_frequency(tunings[i]);
            }

            synth.update_note_tunings(tunings, count);
        }

    private:
        void update_frequency(Synth::NoteTuning& tuning) noexcept
        {
            tuning.frequency = MTS_NoteToFrequency(
                client, (char)tuning.note, (char)tuning.channel
            );
        }

        Synth& synth;
        MTSClient* client;
};

}

#endif
