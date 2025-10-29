//-----------------------------------------------------------------------------
// Flags       : clang-format SMTGSequencer
// Project     : VST SDK
//
// Category    : Validator
// Filename    : public.sdk/source/vst/testsuite/general/suspendresume.cpp
// Created by  : Steinberg, 04/2005
// Description : VST Test Suite
//
//-----------------------------------------------------------------------------
// This file is part of a Steinberg SDK. It is subject to the license terms
// in the LICENSE file found in the top-level directory of this distribution
// and at www.steinberg.net/sdklicenses.
// No part of the SDK, including this file, may be copied, modified, propagated,
// or distributed except according to the terms contained in the LICENSE file.
//-----------------------------------------------------------------------------

#include "public.sdk/source/vst/testsuite/general/suspendresume.h"

//------------------------------------------------------------------------
namespace Steinberg {
namespace Vst {

//------------------------------------------------------------------------
// SuspendResumeTest
//------------------------------------------------------------------------
SuspendResumeTest::SuspendResumeTest (ITestPlugProvider* plugProvider, ProcessSampleSize sampl)
: TestEnh (plugProvider, sampl)
{
}

//------------------------------------------------------------------------
bool PLUGIN_API SuspendResumeTest::run (ITestResult* testResult)
{
	if (!testResult || !vstPlug)
		return false;

	printTestHeader (testResult);

	for (int32 i = 0; i < 3; ++i)
	{
		if (audioEffect)
		{
			if (audioEffect->canProcessSampleSize (kSample32) == kResultOk)
				processSetup.symbolicSampleSize = kSample32;
			else if (audioEffect->canProcessSampleSize (kSample64) == kResultOk)
				processSetup.symbolicSampleSize = kSample64;
			else
			{
				addErrorMessage (testResult,
				                 STR ("No appropriate symbolic sample size supported!"));
				return false;
			}

			if (audioEffect->setupProcessing (processSetup) != kResultOk)
			{
				addErrorMessage (testResult, STR ("Process setup failed!"));
				return false;
			}
		}
		tresult result = vstPlug->setActive (true);
		if (result != kResultOk)
			return false;

		result = vstPlug->setActive (false);
		if (result != kResultOk)
			return false;
	}
	return true;
}

//------------------------------------------------------------------------
} // Vst
} // Steinberg
