//-----------------------------------------------------------------------------
// Project     : VST SDK
//
// Category    : Examples
// Filename    : public.sdk/source/common/threadchecker_win32.cpp
// Created by  : Steinberg, 01/2019
// Description : win32 thread checker
//
//-----------------------------------------------------------------------------
// This file is part of a Steinberg SDK. It is subject to the license terms
// in the LICENSE file found in the top-level directory of this distribution
// and at www.steinberg.net/sdklicenses.
// No part of the SDK, including this file, may be copied, modified, propagated,
// or distributed except according to the terms contained in the LICENSE file.
//-----------------------------------------------------------------------------

#include "threadchecker.h"

#if SMTG_OS_WINDOWS
#include <windows.h>

//------------------------------------------------------------------------
namespace Steinberg {
namespace Vst {

//------------------------------------------------------------------------
class Win32ThreadChecker : public ThreadChecker
{
public:
	bool test (const char* failmessage = nullptr, bool exit = false) override
	{
		if (threadID == GetCurrentThreadId ())
			return true;
		if (failmessage)
			OutputDebugStringA (failmessage);
		if (exit)
			std::terminate ();
		return false;
	}

	DWORD threadID {GetCurrentThreadId ()};
};

//------------------------------------------------------------------------
std::unique_ptr<ThreadChecker> ThreadChecker::create ()
{
	return std::unique_ptr<ThreadChecker> (new Win32ThreadChecker);
}

//------------------------------------------------------------------------
} // namespace Vst
} // namespace Steinberg

#endif // SMTG_OS_WINDOWS
