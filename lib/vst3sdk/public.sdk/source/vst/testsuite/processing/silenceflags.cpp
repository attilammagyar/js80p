//-----------------------------------------------------------------------------
// Flags       : clang-format SMTGSequencer
// Project     : VST SDK
//
// Category    : Validator
// Filename    : public.sdk/source/vst/testsuite/processing/silenceflags.cpp
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

#include "public.sdk/source/vst/testsuite/processing/silenceflags.h"

//------------------------------------------------------------------------
namespace Steinberg {
namespace Vst {

//------------------------------------------------------------------------
// SilenceFlagsTest
//------------------------------------------------------------------------
SilenceFlagsTest::SilenceFlagsTest (ITestPlugProvider* plugProvider, ProcessSampleSize sampl)
: ProcessTest (plugProvider, sampl)
{
}

//------------------------------------------------------------------------
bool PLUGIN_API SilenceFlagsTest::run (ITestResult* testResult)
{
	if (!vstPlug || !testResult || !audioEffect)
		return false;

	if (!canProcessSampleSize (testResult))
		return true;

	printTestHeader (testResult);

	if (processData.inputs != nullptr)
	{
		audioEffect->setProcessing (true);

		for (int32 inputsIndex = 0; inputsIndex < processData.numInputs; inputsIndex++)
		{
			int32 numSilenceFlagsCombinations =
			    (1 << processData.inputs[inputsIndex].numChannels) - 1;
			for (int32 flagCombination = 0; flagCombination <= numSilenceFlagsCombinations;
			     flagCombination++)
			{
				processData.inputs[inputsIndex].silenceFlags = flagCombination;
				tresult result = audioEffect->process (processData);
				if (result != kResultOk)
				{
					addErrorMessage (
					    testResult,
					    printf (
					        "The component failed to process bus %i with silence flag combination %x!",
					        inputsIndex, flagCombination));
					audioEffect->setProcessing (false);
					return false;
				}
			}
		}
	}
	else if (processData.numInputs > 0)
	{
		addErrorMessage (testResult,
		                 STR ("ProcessData::inputs are 0 but ProcessData::numInputs are nonzero."));
		return false;
	}

	audioEffect->setProcessing (false);
	return true;
}

//------------------------------------------------------------------------
} // Vst
} // Steinberg
