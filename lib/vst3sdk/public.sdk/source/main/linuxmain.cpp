//-----------------------------------------------------------------------------
// Project     : SDK Core
// Version     : 1.0
//
// Category    : Common Base Classes
// Filename    : public.sdk/source/main/linuxmain.cpp
// Created by  : Steinberg, 03/2017
// Description : Linux Component Entry
//
//-----------------------------------------------------------------------------
// This file is part of a Steinberg SDK. It is subject to the license terms
// in the LICENSE file found in the top-level directory of this distribution
// and at www.steinberg.net/sdklicenses.
// No part of the SDK, including this file, may be copied, modified, propagated,
// or distributed except according to the terms contained in the LICENSE file.
//-----------------------------------------------------------------------------

#include "pluginterfaces/base/fplatform.h"

void* moduleHandle = nullptr;

//------------------------------------------------------------------------
bool InitModule ();		///< must be provided by plug-in: called when the library is loaded
bool DeinitModule ();	///< must be provided by plug-in: called when the library is unloaded

//------------------------------------------------------------------------
extern "C"
{
	/** must be provided by the plug-in! */
	SMTG_EXPORT_SYMBOL bool ModuleEntry (void*);
	SMTG_EXPORT_SYMBOL bool ModuleExit (void);
}

static int moduleCounter {0}; // counting for ModuleEntry/ModuleExit pairs

//------------------------------------------------------------------------
/** must be called from host right after loading dll
Note: this could be called more than one time! */
bool ModuleEntry (void* sharedLibraryHandle)
{
	if (++moduleCounter == 1)
	{
		moduleHandle = sharedLibraryHandle;
		return InitModule ();
	}
	return true;
}

//------------------------------------------------------------------------
/** must be called from host right before unloading dll
Note: this could be called more than one time! */
bool ModuleExit (void)
{
	if (--moduleCounter == 0)
	{
		moduleHandle = nullptr;
		return DeinitModule ();
	}
	else if (moduleCounter < 0)
		return false;
	return true;
}
