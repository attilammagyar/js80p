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

#include <ostream>
#include <string>

#include <vst3sdk/public.sdk/source/vst/moduleinfo/moduleinfo.h>
#include <vst3sdk/public.sdk/source/vst/moduleinfo/moduleinfocreator.h>
#include <vst3sdk/public.sdk/source/vst/moduleinfo/moduleinfocreator.cpp>

#include <vst3sdk/public.sdk/source/vst/hosting/module.cpp>
#include <vst3sdk/public.sdk/source/vst/hosting/module.h>

// #if SMTG_OS_LINUX
#include <vst3sdk/public.sdk/source/vst/hosting/module_linux.cpp>
#include "plugin/vst3/so.cpp"
// #elif SMTG_OS_WINDOWS
// #include <vst3sdk/public.sdk/source/vst/hosting/module_win32.cpp>
// #include "plugin/vst3/dll.cpp"
// #else
// #error "Unsupported OS, currently  can be compiled only for Linux and Windows. (Or did something go wrong with the SMTG_OS_LINUX and SMTG_OS_WINDOWS macros?)"
// #endif


using namespace Steinberg;


int main(int const argc, char const* argv[])
{
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " vst3_plugin_path" << std::endl;

        return 1;
    }

    std::string const path(argv[1]);
    std::string error;

    std::shared_ptr<VST3::Hosting::Module> module = (
        VST3::Hosting::Module::create(path, error)
    );

    if (!module) {
        std::cerr << "ERROR: " << error << std::endl;

        return 2;
    }

    ModuleInfo module_info = ModuleInfoLib::createModuleInfo(*module, false);
    ModuleInfoLib::outputJson(module_info, std::cout);

    return 0;
}
