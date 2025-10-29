//-----------------------------------------------------------------------------
// Flags       : clang-format SMTGSequencer
// Project     : VST SDK
//
// Category    : Validator
// Filename    : public.sdk/source/vst/testsuite/processing/processformat.cpp
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

#include "public.sdk/source/vst/testsuite/processing/processformat.h"

//------------------------------------------------------------------------
namespace Steinberg {
namespace Vst {

//------------------------------------------------------------------------
// ProcessFormatTest
//------------------------------------------------------------------------
ProcessFormatTest::ProcessFormatTest (ITestPlugProvider* plugProvider, ProcessSampleSize sampl)
: ProcessTest (plugProvider, sampl)
{
}

//------------------------------------------------------------------------
bool PLUGIN_API ProcessFormatTest::run (ITestResult* testResult)
{
	if (!vstPlug || !testResult || !audioEffect)
		return false;

	if (!canProcessSampleSize (testResult))
		return true;

	printTestHeader (testResult);

	int32 numFails = 0;
	const int32 numRates = 12;
	SampleRate sampleRateFormats[numRates] = {22050.,    32000.,    44100.,    48000.,
	                                          88200.,    96000.,    192000.,   384000.,
	                                          1234.5678, 12345.678, 123456.78, 1234567.8};

	tresult result = vstPlug->setActive (false);
	if (result != kResultOk)
	{
		addErrorMessage (testResult, STR ("IComponent::setActive (false) failed."));
		return false;
	}

	addMessage (testResult, STR ("***Tested Sample Rates***"));

	for (int32 i = 0; i < numRates; ++i)
	{
		processSetup.sampleRate = sampleRateFormats[i];
		result = audioEffect->setupProcessing (processSetup);
		if (result == kResultOk)
		{
			result = vstPlug->setActive (true);
			if (result != kResultOk)
			{
				addErrorMessage (testResult, STR ("IComponent::setActive (true) failed."));
				return false;
			}

			audioEffect->setProcessing (true);
			result = audioEffect->process (processData);
			audioEffect->setProcessing (false);

			if (result == kResultOk)
			{
				addMessage (testResult,
				            printf (" %10.10G Hz - processed successfully!", sampleRateFormats[i]));
			}
			else
			{
				numFails++;
				addErrorMessage (testResult,
				                 printf (" %10.10G Hz - failed to process!", sampleRateFormats[i]));
			}

			result = vstPlug->setActive (false);
			if (result != kResultOk)
			{
				addErrorMessage (testResult, STR ("IComponent::setActive (false) failed."));
				return false;
			}
		}
		else if (sampleRateFormats[i] > 0.)
		{
			addErrorMessage (
			    testResult,
			    printf ("IAudioProcessor::setupProcessing (..) failed for samplerate %.3f Hz! ",
			            sampleRateFormats[i]));
			// return false;
		}
	}

	result = vstPlug->setActive (true);
	if (result != kResultOk)
		return false;

	return true;
}

//------------------------------------------------------------------------
} // Vst
} // Steinberg
