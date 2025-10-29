//-----------------------------------------------------------------------------
// Flags       : clang-format SMTGSequencer
// Project     : VST SDK
//
// Category    : Validator
// Filename    : public.sdk/source/vst/testsuite/processing/variableblocksize.cpp
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

#include "public.sdk/source/vst/testsuite/processing/variableblocksize.h"

//------------------------------------------------------------------------
namespace Steinberg {
namespace Vst {

//------------------------------------------------------------------------
// VariableBlockSizeTest
//------------------------------------------------------------------------
VariableBlockSizeTest::VariableBlockSizeTest (ITestPlugProvider* plugProvider,
                                              ProcessSampleSize sampl)
: ProcessTest (plugProvider, sampl)
{
}

//------------------------------------------------------------------------
bool PLUGIN_API VariableBlockSizeTest::run (ITestResult* testResult)
{
	if (!vstPlug || !testResult || !audioEffect)
		return false;

	if (!canProcessSampleSize (testResult))
		return true;

	printTestHeader (testResult);

	audioEffect->setProcessing (true);

	for (int32 i = 0; i <= TestDefaults::instance ().numIterations; ++i)
	{
		int32 sampleFrames = rand () % processSetup.maxSamplesPerBlock;
		processData.numSamples = sampleFrames;
		if (i == 0)
			processData.numSamples = 0;
#if defined(TOUGHTESTS) && TOUGHTESTS
		else if (i == 1)
			processData.numSamples = -50000;
		else if (i == 2)
			processData.numSamples = processSetup.maxSamplesPerBlock * 2;
#endif // TOUGHTESTS

		tresult result = audioEffect->process (processData);
		if ((result != kResultOk)
#if defined(TOUGHTESTS) && TOUGHTESTS
		    && (i > 1)
#else
		    && (i > 0)
#endif // TOUGHTESTS
		        )
		{
			addErrorMessage (
			    testResult,
			    printf ("The component failed to process an audioblock of size %i", sampleFrames));
			audioEffect->setProcessing (false);
			return false;
		}
	}

	audioEffect->setProcessing (false);
	return true;
}

//------------------------------------------------------------------------
} // Vst
} // Steinberg
