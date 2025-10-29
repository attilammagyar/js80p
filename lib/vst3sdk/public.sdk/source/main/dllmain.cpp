//-----------------------------------------------------------------------------
// Project     : SDK Core
// Version     : 1.0
//
// Category    : Common Base Classes
// Filename    : public.sdk/source/main/dllmain.cpp
// Created by  : Steinberg, 01/2004
// Description : Windows DLL Entry
//
//-----------------------------------------------------------------------------
// This file is part of a Steinberg SDK. It is subject to the license terms
// in the LICENSE file found in the top-level directory of this distribution
// and at www.steinberg.net/sdklicenses. 
// No part of the SDK, including this file, may be copied, modified, propagated,
// or distributed except according to the terms contained in the LICENSE file.
//-----------------------------------------------------------------------------

#include "pluginterfaces/base/ftypes.h"
#include "pluginterfaces/base/fstrdefs.h"

#include <windows.h>

#if defined(_MSC_VER) && defined(DEVELOPMENT)
#include <crtdbg.h>
#endif

//------------------------------------------------------------------------
HINSTANCE ghInst = nullptr;
void* moduleHandle = nullptr;
#define VST_MAX_PATH 2048
Steinberg::tchar gPath[VST_MAX_PATH] = {0};

//------------------------------------------------------------------------
extern bool InitModule (); ///< must be provided by plug-in: called when the library is loaded
extern bool DeinitModule (); ///< must be provided by plug-in: called when the library is unloaded

//------------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif

static int moduleCounter {0}; // counting for InitDll/ExitDll pairs

//------------------------------------------------------------------------
/** must be called from host right after loading dll,
must be provided by the plug-in!
Note: this could be called more than one time! */
SMTG_EXPORT_SYMBOL bool InitDll ()
{
	if (++moduleCounter == 1)
		return InitModule ();
	return true;
}

//------------------------------------------------------------------------
/** must be called from host right before unloading dll
must be provided by the plug-in!
Note: this could be called more than one time! */
SMTG_EXPORT_SYMBOL bool ExitDll ()
{
	if (--moduleCounter == 0)
		return DeinitModule ();
	if (moduleCounter < 0)
		return false;
	return true;
}
#ifdef __cplusplus
} // extern "C"
#endif

//------------------------------------------------------------------------
BOOL WINAPI DllMain (HINSTANCE hInst, DWORD dwReason, LPVOID /*lpvReserved*/)
{
	if (dwReason == DLL_PROCESS_ATTACH)
	{
#if defined(_MSC_VER) && defined(DEVELOPMENT)
		_CrtSetReportMode (_CRT_WARN, _CRTDBG_MODE_DEBUG);
		_CrtSetReportMode (_CRT_ERROR, _CRTDBG_MODE_DEBUG);
		_CrtSetReportMode (_CRT_ASSERT, _CRTDBG_MODE_DEBUG);
		int flag = _CrtSetDbgFlag (_CRTDBG_REPORT_FLAG);
		_CrtSetDbgFlag (flag | _CRTDBG_LEAK_CHECK_DF);
#endif

		moduleHandle = ghInst = hInst;

		// gets the path of the component
		if (GetModuleFileName (ghInst, Steinberg::wscast (gPath), MAX_PATH) > 0)
		{
			Steinberg::tchar* bkslash = Steinberg::wscast (wcsrchr (Steinberg::wscast (gPath), L'\\'));
			if (bkslash)
				gPath[bkslash - gPath + 1] = 0;
		}
	}

	return TRUE;
}
