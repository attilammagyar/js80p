//-----------------------------------------------------------------------------
// Flags       : clang-format SMTGSequencer
// Project     : VST SDK
//
// Category    : Validator
// Filename    : public.sdk/source/vst/testsuite/processing/silenceprocessing.cpp
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

#include "public.sdk/source/vst/testsuite/processing/silenceprocessing.h"
#include <cmath>

//------------------------------------------------------------------------
namespace Steinberg {
namespace Vst {

//------------------------------------------------------------------------
// SilenceProcessingTest
//------------------------------------------------------------------------
SilenceProcessingTest::SilenceProcessingTest (ITestPlugProvider* plugProvider,
                                              ProcessSampleSize sampl)
: ProcessTest (plugProvider, sampl)
{
}

//------------------------------------------------------------------------
bool SilenceProcessingTest::isBufferSilent (void* buffer, int32 numSamples, ProcessSampleSize sampl)
{
	if (sampl == kSample32)
	{
		const float kSilenceThreshold = 0.000132184039f;

		float* floatBuffer = (float*)buffer;
		while (numSamples--)
		{
			if (fabsf (*floatBuffer) > kSilenceThreshold)
				return false;
			floatBuffer++;
		}
	}
	else if (sampl == kSample64)
	{
		const double kSilenceThreshold = 0.000132184039;

		double* floatBuffer = (double*)buffer;
		while (numSamples--)
		{
			if (fabs (*floatBuffer) > kSilenceThreshold)
				return false;
			floatBuffer++;
		}
	}
	return true;
}

//------------------------------------------------------------------------
bool PLUGIN_API SilenceProcessingTest::run (ITestResult* testResult)
{
	if (!vstPlug || !testResult || !audioEffect)
		return false;

	if (!canProcessSampleSize (testResult))
		return true;

	printTestHeader (testResult);

	if (processData.inputs != nullptr)
	{
		// process 20s before checking flags
		int32 numPasses = int32 (20 * processSetup.sampleRate / processData.numSamples + 0.5);

		audioEffect->setProcessing (true);
		for (int32 pass = 0; pass < numPasses; pass++)
		{
			for (int32 busIndex = 0; busIndex < processData.numInputs; busIndex++)
			{
				processData.inputs[busIndex].silenceFlags = 0;
				for (int32 channelIndex = 0;
				     channelIndex < processData.inputs[busIndex].numChannels; channelIndex++)
				{
					processData.inputs[busIndex].silenceFlags |= (uint64)1 << (uint64)channelIndex;
					if (processData.symbolicSampleSize == kSample32)
						memset (processData.inputs[busIndex].channelBuffers32[channelIndex], 0,
						        sizeof (float) * processData.numSamples);
					else if (processData.symbolicSampleSize == kSample64)
						memset (processData.inputs[busIndex].channelBuffers32[channelIndex], 0,
						        sizeof (double) * processData.numSamples);
				}
			}

			for (int32 busIndex = 0; busIndex < processData.numOutputs; busIndex++)
			{
				if (processData.numInputs > busIndex)
					processData.outputs[busIndex].silenceFlags =
					    processData.inputs[busIndex].silenceFlags;
				else
				{
					processData.outputs[busIndex].silenceFlags = 0;
					for (int32 channelIndex = 0;
					     channelIndex < processData.outputs[busIndex].numChannels; channelIndex++)
						processData.outputs[busIndex].silenceFlags |= (uint64)1
						                                              << (uint64)channelIndex;
				}
			}

			tresult result = audioEffect->process (processData);
			if (result != kResultOk)
			{
				addErrorMessage (testResult, printf ("%s", "The component failed to process!"));

				audioEffect->setProcessing (false);
				return false;
			}
		}

		for (int32 busIndex = 0; busIndex < processData.numOutputs; busIndex++)
		{
			for (int32 channelIndex = 0; channelIndex < processData.outputs[busIndex].numChannels;
			     channelIndex++)
			{
				bool channelShouldBeSilent = (processData.outputs[busIndex].silenceFlags &
				                              (uint64)1 << (uint64)channelIndex) != 0;
				bool channelIsSilent =
				    isBufferSilent (processData.outputs[busIndex].channelBuffers32[channelIndex],
				                    processData.numSamples, processData.symbolicSampleSize);
				if (channelShouldBeSilent != channelIsSilent)
				{
					constexpr auto silentText = STR (
					    "The component reported a wrong silent flag for its output buffer! : output is silent but silenceFlags not set !");
					constexpr auto nonSilentText = STR (
					    "The component reported a wrong silent flag for its output buffer! : silenceFlags is set to silence but output is not silent");
					addMessage (testResult, channelIsSilent ? silentText : nonSilentText);
					break;
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
