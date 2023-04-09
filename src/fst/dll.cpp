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

#include <windows.h>

#include "fst/plugin.hpp"
#include "gui/gui.hpp"


static HINSTANCE dll_instance = NULL;


extern "C" BOOL WINAPI DllMain(
        HINSTANCE hinstDLL,
        DWORD fdwReason,
        LPVOID lpvReserved
) {
    dll_instance = hinstDLL;

    return TRUE;
}


extern "C" __declspec(dllexport) AEffect* VSTPluginMain(audioMasterCallback host_callback)
{
    return JS80P::FstPlugin::create_instance(
        host_callback, (JS80P::GUI::PlatformData)dll_instance
    );
}
