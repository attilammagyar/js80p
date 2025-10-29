//-----------------------------------------------------------------------------
// Project     : SDK Core
//
// Category    : Common Base Classes
// Filename    : public.sdk/source/main/moduleinit.h
// Created by  : Steinberg, 11/2020
// Description : Module Initializers/Terminators
//
//-----------------------------------------------------------------------------
// This file is part of a Steinberg SDK. It is subject to the license terms
// in the LICENSE file found in the top-level directory of this distribution
// and at www.steinberg.net/sdklicenses. 
// No part of the SDK, including this file, may be copied, modified, propagated,
// or distributed except according to the terms contained in the LICENSE file.
//-----------------------------------------------------------------------------

#pragma once

#include "pluginterfaces/base/ftypes.h"
#include <algorithm>
#include <functional>
#include <limits>
#include <numeric>

//------------------------------------------------------------------------
/** A replacement for InitModule and DeinitModule
 *
 *	If you link this file the InitModule and DeinitModule functions are 
 *	implemented and you can use this to register functions that will be 
 *	called when the module is loaded and before the module is unloaded.
 *
 *	Use this for one time initializers or cleanup functions.
 *	For example: if you depend on a 3rd party library that needs
 *	initialization before you can use it you can write an initializer like this:
 *
 *	static ModuleInitializer InitMyExternalLib ([] () { MyExternalLib::init (); });
 *
 *	Or you have a lazy create wavetable you need to free the allocated memory later:
 *
 *	static ModuleTerminator FreeWaveTableMemory ([] () { MyWaveTable::free (); });
 */

//------------------------------------------------------------------------
#if SMTG_OS_WINDOWS
using HINSTANCE = struct HINSTANCE__*;
namespace Steinberg { using PlatformModuleHandle = HINSTANCE; }
//------------------------------------------------------------------------
#elif SMTG_OS_OSX || SMTG_OS_IOS
typedef struct __CFBundle* CFBundleRef;
namespace Steinberg { using PlatformModuleHandle = CFBundleRef; }
//------------------------------------------------------------------------
#elif SMTG_OS_LINUX
namespace Steinberg { using PlatformModuleHandle = void*; }
#endif

//------------------------------------------------------------------------
namespace Steinberg {

using ModuleInitFunction = std::function<void ()>;
using ModuleInitPriority = uint32;
static constexpr ModuleInitPriority DefaultModulePriority =
    std::numeric_limits<ModuleInitPriority>::max () / 2;

//------------------------------------------------------------------------
struct ModuleInitializer
{
	/**
	 *	Register a function which is called when the module is loaded
	 *	@param func function to call
	 *	@param prio priority
	 */
	ModuleInitializer (ModuleInitFunction&& func,
	                   ModuleInitPriority prio = DefaultModulePriority);
};

//------------------------------------------------------------------------
struct ModuleTerminator
{
	/**
	 *	Register a function which is called when the module is unloaded
	 *	@param func function to call
	 *	@param prio priority
	 */
	ModuleTerminator (ModuleInitFunction&& func,
	                  ModuleInitPriority prio = DefaultModulePriority);
};

//------------------------------------------------------------------------
PlatformModuleHandle getPlatformModuleHandle ();

//------------------------------------------------------------------------
} // Steinberg

