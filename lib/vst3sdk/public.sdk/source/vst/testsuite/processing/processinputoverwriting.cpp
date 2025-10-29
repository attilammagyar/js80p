//-----------------------------------------------------------------------------
// Flags       : clang-format SMTGSequencer
// Project     : VST SDK
//
// Category    : Validator
// Filename    : public.sdk/source/vst/testsuite/processing/processinputoverwriting.cpp
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

#include "public.sdk/source/vst/testsuite/processing/processinputoverwriting.h"

//------------------------------------------------------------------------
namespace Steinberg {
namespace Vst {

//------------------------------------------------------------------------
// ProcessInputOverwritingTest
//------------------------------------------------------------------------
ProcessInputOverwritingTest::ProcessInputOverwritingTest (ITestPlugProvider* plugProvider,
                                                          ProcessSampleSize sampl)
: ProcessTest (plugProvider, sampl)
{
}

//------------------------------------------------------------------------
bool ProcessInputOverwritingTest::run (ITestResult* testResult)
{
	if (!testResult || !vstPlug)
		return false;

	printTestHeader (testResult);

	bool ret = ProcessTest::run (testResult);
	return ret;
}

//------------------------------------------------------------------------
bool ProcessInputOverwritingTest::preProcess (ITestResult* /*testResult*/)
{
	int32 min = processData.numInputs < processData.numOutputs ? processData.numInputs :
	                                                             processData.numOutputs;
	noNeedtoProcess = true;

	for (int32 i = 0; i < min; i++)
	{
		if (!noNeedtoProcess)
			break;

		int32 minChannel = processData.inputs[i].numChannels < processData.outputs[i].numChannels ?
		                       processData.inputs[i].numChannels :
		                       processData.outputs[i].numChannels;

		auto ptrIn = processData.inputs[i].channelBuffers32;
		auto ptrOut = processData.outputs[i].channelBuffers32;
		for (int32 j = 0; j < minChannel; j++)
		{
			if (ptrIn[j] != ptrOut[j])
			{
				noNeedtoProcess = false;
				break;
			}
		}
	}
	if (noNeedtoProcess)
		return true;

	for (int32 i = 0; i < processData.numInputs; i++)
	{
		if (processSetup.symbolicSampleSize == kSample32)
		{
			auto ptr = processData.inputs[i].channelBuffers32;
			if (ptr)
			{
				float inc = 1.f / (processData.numSamples - 1);
				for (int32 c = 0; c < processData.inputs[i].numChannels; c++)
				{
					auto chaBuf = ptr[c];
					for (int32 j = 0; j < processData.numSamples; j++)
					{
						*chaBuf = inc * j;
						chaBuf++;
					}
				}
			}
		}
		else if (processSetup.symbolicSampleSize == kSample64)
		{
			auto ptr = processData.inputs[i].channelBuffers64;
			if (ptr)
			{
				double inc = 1.0 / (processData.numSamples - 1);
				for (int32 c = 0; c < processData.inputs[i].numChannels; c++)
				{
					auto chaBuf = ptr[c];
					for (int32 j = 0; j < processData.numSamples; j++)
					{
						*chaBuf = inc * j;
						chaBuf++;
					}
				}
			}
		}
	}
	return true;
}

//------------------------------------------------------------------------
bool ProcessInputOverwritingTest::postProcess (ITestResult* testResult)
{
	if (noNeedtoProcess)
		return true;

	for (int32 i = 0; i < processData.numInputs; i++)
	{
		if (processSetup.symbolicSampleSize == kSample32)
		{
			auto ptr = processData.inputs[i].channelBuffers32;
			if (ptr)
			{
				float inc = 1.f / (processData.numSamples - 1);
				for (int32 c = 0; c < processData.inputs[i].numChannels; c++)
				{
					auto chaBuf = ptr[c];
					for (int32 j = 0; j < processData.numSamples; j++)
					{
						if (*chaBuf != inc * j)
						{
							addErrorMessage (
							    testResult,
							    STR (
							        "IAudioProcessor::process overwrites input buffer (..with kSample32..)!"));
							return false;
						}
						chaBuf++;
					}
				}
			}
		}
		else if (processSetup.symbolicSampleSize == kSample64)
		{
			auto ptr = processData.inputs[i].channelBuffers64;
			if (ptr)
			{
				double inc = 1.0 / (processData.numSamples - 1);
				for (int32 c = 0; c < processData.inputs[i].numChannels; c++)
				{
					auto chaBuf = ptr[c];
					for (int32 j = 0; j < processData.numSamples; j++)
					{
						if (*chaBuf != inc * j)
						{
							addErrorMessage (
							    testResult,
							    STR (
							        "IAudioProcessor::process overwrites input buffer (..with kSample64..)!"));
							return false;
						}
						chaBuf++;
					}
				}
			}
		}
	}
	return true;
}

//------------------------------------------------------------------------
} // Vst
} // Steinberg
