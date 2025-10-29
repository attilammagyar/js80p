//-----------------------------------------------------------------------------
// Project     : VST SDK
//
// Category    : Examples
// Filename    : public.sdk/source/common/threadchecker_linux.cpp
// Created by  : Steinberg, 01/2019
// Description : linux thread checker
//
//-----------------------------------------------------------------------------
// This file is part of a Steinberg SDK. It is subject to the license terms
// in the LICENSE file found in the top-level directory of this distribution
// and at www.steinberg.net/sdklicenses.
// No part of the SDK, including this file, may be copied, modified, propagated,
// or distributed except according to the terms contained in the LICENSE file.
//-----------------------------------------------------------------------------

#include "threadchecker.h"

#if SMTG_OS_LINUX

#include <cstdio> 
#include <pthread.h>

//------------------------------------------------------------------------
namespace Steinberg {
namespace Vst {

//------------------------------------------------------------------------
class LinuxThreadChecker : public ThreadChecker
{
public:
	bool test (const char* failmessage = nullptr, bool exit = false) override
	{
		if (threadID == pthread_self ())
			return true;
		if (failmessage)
			fprintf (stderr, "%s", failmessage);
		if (exit)
			std::terminate ();
		return false;
	}

	pthread_t threadID {pthread_self ()};
};

//------------------------------------------------------------------------
std::unique_ptr<ThreadChecker> ThreadChecker::create ()
{
	return std::unique_ptr<ThreadChecker> (new LinuxThreadChecker);
}

//------------------------------------------------------------------------
} // namespace Vst
} // namespace Steinberg

#endif // SMTG_OS_LINUX
