//-----------------------------------------------------------------------------
// Flags       : clang-format SMTGSequencer
// Project     : VST SDK
//
// Category    : Validator
// Filename    : public.sdk/source/vst/testsuite/processing/processthreaded.cpp
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

#include "public.sdk/source/vst/testsuite/processing/processthreaded.h"
#include <thread>

//------------------------------------------------------------------------
namespace Steinberg {
namespace Vst {

//------------------------------------------------------------------------
// ProcessTest
//------------------------------------------------------------------------
ProcessThreadTest::ProcessThreadTest (ITestPlugProvider* plugProvider, ProcessSampleSize sampl)
: ProcessTest (plugProvider, sampl)
{
}

//------------------------------------------------------------------------
ProcessThreadTest::~ProcessThreadTest ()
{
}

//------------------------------------------------------------------------
bool ProcessThreadTest::run (ITestResult* testResult)
{
	constexpr auto NUM_ITERATIONS = 9999;

	if (!vstPlug || !testResult || !audioEffect)
		return false;
	if (!canProcessSampleSize (testResult))
		return true;

	printTestHeader (testResult);

	bool result = false;
	std::thread processThread ([&] () {
		result = true;
		audioEffect->setProcessing (true);
		for (auto i = 0; i < NUM_ITERATIONS; i++)
		{
			tresult tr = audioEffect->process (processData);
			if (tr != kResultTrue)
			{
				result = false;
				break;
			}
		}
		audioEffect->setProcessing (false);
	});

	processThread.join ();

	if (!result)
		testResult->addErrorMessage (STR ("Processing failed."));
	return result;
}

//------------------------------------------------------------------------
} // Vst
} // Steinberg
