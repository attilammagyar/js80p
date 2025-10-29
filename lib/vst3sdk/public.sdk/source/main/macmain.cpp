//-----------------------------------------------------------------------------
// Project     : SDK Core
// Version     : 1.0
//
// Category    : Common Base Classes
// Filename    : public.sdk/source/main/macmain.cpp
// Created by  : Steinberg, 01/2004
// Description : Mac OS X Bundle Entry
//
//-----------------------------------------------------------------------------
// This file is part of a Steinberg SDK. It is subject to the license terms
// in the LICENSE file found in the top-level directory of this distribution
// and at www.steinberg.net/sdklicenses.
// No part of the SDK, including this file, may be copied, modified, propagated,
// or distributed except according to the terms contained in the LICENSE file.
//-----------------------------------------------------------------------------

#include "pluginterfaces/base/fplatform.h"

#ifndef __CF_USE_FRAMEWORK_INCLUDES__
#define __CF_USE_FRAMEWORK_INCLUDES__ 1
#endif

#include <CoreFoundation/CoreFoundation.h>

//------------------------------------------------------------------------
CFBundleRef ghInst = nullptr;
int bundleRefCounter = 0; // counting for bundleEntry/bundleExit pairs
void* moduleHandle = nullptr;
#define VST_MAX_PATH 2048
char gPath[VST_MAX_PATH] = {0};

//------------------------------------------------------------------------
bool InitModule (); ///< must be provided by plug-in: called when the library is loaded
bool DeinitModule (); ///< must be provided by plug-in: called when the library is unloaded

//------------------------------------------------------------------------
extern "C" {
/** bundleEntry and bundleExit must be provided by the plug-in! */
SMTG_EXPORT_SYMBOL bool bundleEntry (CFBundleRef);
SMTG_EXPORT_SYMBOL bool bundleExit (void);
}

#include <vector>

std::vector<CFBundleRef> gBundleRefs;

//------------------------------------------------------------------------
/** must be called from host right after loading bundle
Note: this could be called more than one time! */
bool bundleEntry (CFBundleRef ref)
{
	if (ref)
	{
		bundleRefCounter++;
		CFRetain (ref);

		// hold all bundle refs until plug-in is fully uninitialized
		gBundleRefs.push_back (ref);

		if (!moduleHandle)
		{
			ghInst = ref;
			moduleHandle = ref;

			// obtain the bundle path
			CFURLRef tempURL = CFBundleCopyBundleURL (ref);
			CFURLGetFileSystemRepresentation (tempURL, true, reinterpret_cast<UInt8*> (gPath), VST_MAX_PATH);
			CFRelease (tempURL);
		}

		if (bundleRefCounter == 1)
			return InitModule ();
	}
	return true;
}

//------------------------------------------------------------------------
/** must be called from host right before unloading bundle
Note: this could be called more than one time! */
bool bundleExit (void)
{
	if (--bundleRefCounter == 0)
	{
		DeinitModule ();

		// release the CFBundleRef's once all bundleExit clients called in
		// there is no way to identify the proper CFBundleRef of the bundleExit call
		for (size_t i = 0; i < gBundleRefs.size (); i++)
			CFRelease (gBundleRefs[i]);
		gBundleRefs.clear ();
	}
	else if (bundleRefCounter < 0)
		return false;
	return true;
}
