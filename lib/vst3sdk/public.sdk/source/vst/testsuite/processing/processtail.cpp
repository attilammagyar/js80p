//-----------------------------------------------------------------------------
// Flags       : clang-format SMTGSequencer
// Project     : VST SDK
//
// Category    : Validator
// Filename    : public.sdk/source/vst/testsuite/processing/processtail.cpp
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

#include "public.sdk/source/vst/testsuite/processing/processtail.h"
#include <cmath>

//------------------------------------------------------------------------
namespace Steinberg {
namespace Vst {

//------------------------------------------------------------------------
// ProcessTailTest
//------------------------------------------------------------------------
ProcessTailTest::ProcessTailTest (ITestPlugProvider* plugProvider, ProcessSampleSize sampl)
: ProcessTest (plugProvider, sampl)
, mTailSamples (0)
, mInTail (0)
, dataPtrFloat (nullptr)
, dataPtrDouble (nullptr)
, mInSilenceInput (false)
, mDontTest (false) {FUNKNOWN_CTOR}

//------------------------------------------------------------------------
ProcessTailTest::~ProcessTailTest ()
{
	if (dataPtrFloat)
	{
		delete[] dataPtrFloat;
		dataPtrFloat = nullptr;
	}
	if (dataPtrDouble)
	{
		delete[] dataPtrDouble;
		dataPtrDouble = nullptr;
	}
}

//------------------------------------------------------------------------
bool PLUGIN_API ProcessTailTest::setup ()
{
	bool result = ProcessTest::setup ();
	if (result)
	{
		mTailSamples = audioEffect->getTailSamples ();

		StringResult subCat;
		plugProvider->getSubCategories (subCat);
		if (subCat.get ().find ("Generator") != std::string::npos ||
		    subCat.get ().find ("Instrument") != std::string::npos)
		{
			mDontTest = true;
		}
	}

	return result;
}

//------------------------------------------------------------------------
bool ProcessTailTest::preProcess (ITestResult* /*testResult*/)
{
	if (!mInSilenceInput)
	{
		if (processSetup.symbolicSampleSize == kSample32)
		{
			if (!dataPtrFloat)
				dataPtrFloat = new float[processData.numSamples];
			float* ptr = dataPtrFloat;
			for (int32 i = 0; i < processData.numSamples; ++i)
				ptr[i] = (float)(2 * rand () / 32767.0 - 1);
		}
		else
		{
			if (!dataPtrDouble)
				dataPtrDouble = new double[processData.numSamples];
			double* ptr = (double*)dataPtrDouble;
			for (int32 i = 0; i < processData.numSamples; ++i)
				ptr[i] = (double)(2 * rand () / 32767.0 - 1);
		}
		for (int32 i = 0; i < processData.numOutputs; ++i)
		{
			for (int32 c = 0; c < processData.outputs->numChannels; ++c)
			{
				if (processSetup.symbolicSampleSize == kSample32)
					memset (processData.outputs->channelBuffers32[c], 0,
					        processData.numSamples * sizeof (float));
				else
					memset (processData.outputs->channelBuffers64[c], 0,
					        processData.numSamples * sizeof (double));
			}
		}
		for (int32 i = 0; i < processData.numInputs; ++i)
		{
			for (int32 c = 0; c < processData.inputs->numChannels; ++c)
			{
				if (processSetup.symbolicSampleSize == kSample32)
					memcpy (processData.inputs->channelBuffers32[c], dataPtrFloat,
					        processData.numSamples * sizeof (float));
				else
					memcpy (processData.inputs->channelBuffers64[c], dataPtrDouble,
					        processData.numSamples * sizeof (double));
			}
		}
	}
	else
	{
		// process with silent buffers
		for (int32 i = 0; i < processData.numOutputs; ++i)
		{
			for (int32 c = 0; c < processData.outputs->numChannels; ++c)
			{
				if (processSetup.symbolicSampleSize == kSample32)
					memset (processData.outputs->channelBuffers32[c], 0,
					        processData.numSamples * sizeof (float));
				else
					memset (processData.outputs->channelBuffers64[c], 0,
					        processData.numSamples * sizeof (double));
			}
		}
		for (int32 i = 0; i < processData.numInputs; ++i)
		{
			for (int32 c = 0; c < processData.inputs->numChannels; ++c)
			{
				if (processSetup.symbolicSampleSize == kSample32)
					memset (processData.inputs->channelBuffers32[c], 0,
					        processData.numSamples * sizeof (float));
				else
					memset (processData.inputs->channelBuffers64[c], 0,
					        processData.numSamples * sizeof (double));
			}
		}
	}

	return true;
}

//------------------------------------------------------------------------
bool ProcessTailTest::postProcess (ITestResult* testResult)
{
	if (mInSilenceInput)
	{
		// should be silence
		if (mTailSamples < mInTail + processData.numSamples)
		{
			int32 start = mTailSamples > mInTail ? mTailSamples - mInTail : 0;
			int32 end = processData.numSamples;

			for (int32 i = 0; i < processData.numOutputs; ++i)
			{
				for (int32 c = 0; c < processData.outputs->numChannels; ++c)
				{
					if (processSetup.symbolicSampleSize == kSample32)
					{
						for (int32 s = start; s < end; ++s)
						{
							if (fabsf (processData.outputs->channelBuffers32[c][s]) >= 1e-7)
							{
								addErrorMessage (
								    testResult,
								    printf (
								        "IAudioProcessor::process (..) generates non silent output for silent input for tail above %d samples.",
								        mTailSamples));
								return false;
							}
						}
					}
					else
					{
						for (int32 s = start; s < end; ++s)
						{
							if (fabs (processData.outputs->channelBuffers64[c][s]) >= 1e-7)
							{
								addErrorMessage (
								    testResult,
								    printf (
								        "IAudioProcessor::process (..) generates non silent output for silent input for tail above %d samples.",
								        mTailSamples));
								return false;
							}
						}
					}
				}
			}
		}
		mInTail += processData.numSamples;
	}
	return true;
}

//------------------------------------------------------------------------
bool PLUGIN_API ProcessTailTest::run (ITestResult* testResult)
{
	if (!testResult || !audioEffect)
		return false;

	if (processSetup.symbolicSampleSize != processData.symbolicSampleSize)
		return false;
	if (!canProcessSampleSize (testResult))
		return true;

	if (mDontTest)
		return true;

	addMessage (testResult,
	            printf ("===%s == Tail=%d ======================", getName (), mTailSamples));

	audioEffect->setProcessing (true);

	// process with signal (noise) and silence
	for (int32 i = 0; i < 20 * TestDefaults::instance ().numAudioBlocksToProcess; ++i)
	{
		mInSilenceInput = i > 10;

		if (!preProcess (testResult))
			return false;
		tresult result = audioEffect->process (processData);
		if (result != kResultOk)
		{
			addErrorMessage (testResult, STR ("IAudioProcessor::process (..) failed."));

			audioEffect->setProcessing (false);
			return false;
		}
		if (!postProcess (testResult))
		{
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
