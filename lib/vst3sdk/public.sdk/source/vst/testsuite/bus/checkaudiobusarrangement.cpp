//-----------------------------------------------------------------------------
// Flags       : clang-format SMTGSequencer
// Project     : VST SDK
//
// Category    : Validator
// Filename    : public.sdk/source/vst/testsuite/bus/checkaudiobusarrangement.cpp
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

#include "public.sdk/source/vst/testsuite/bus/checkaudiobusarrangement.h"
#include "pluginterfaces/base/funknownimpl.h"

//------------------------------------------------------------------------
namespace Steinberg {
namespace Vst {

//------------------------------------------------------------------------
// CheckAudioBusArrangementTest
//------------------------------------------------------------------------
CheckAudioBusArrangementTest::CheckAudioBusArrangementTest (ITestPlugProvider* plugProvider)
: TestBase (plugProvider)
{
}

//------------------------------------------------------------------------
bool CheckAudioBusArrangementTest::run (ITestResult* testResult)
{
	if (!testResult || !vstPlug)
		return false;

	printTestHeader (testResult);

	int32 numInputs = vstPlug->getBusCount (kAudio, kInput);
	int32 numOutputs = vstPlug->getBusCount (kAudio, kOutput);
	int32 arrangementMismatchs = 0;

	if (auto audioEffect = U::cast<IAudioProcessor> (vstPlug))
	{
		for (int32 i = 0; i < numInputs + numOutputs; ++i)
		{
			BusDirection dir = i < numInputs ? kInput : kOutput;
			int32 busIndex = dir == kInput ? i : i - numInputs;

			addMessage (testResult, printf ("   Check %s Audio Bus Arrangement (%d)",
			                                dir == kInput ? "Input" : "Output", busIndex));

			BusInfo busInfo = {};
			if (vstPlug->getBusInfo (kAudio, dir, busIndex, busInfo) == kResultTrue)
			{
				SpeakerArrangement arrangement;
				if (audioEffect->getBusArrangement (dir, busIndex, arrangement) == kResultTrue)
				{
					if (busInfo.channelCount != SpeakerArr::getChannelCount (arrangement))
					{
						arrangementMismatchs++;
						addErrorMessage (testResult, STR ("channelCount is inconsistent!"));
					}
				}
				else
				{
					addErrorMessage (testResult,
					                 STR ("IAudioProcessor::getBusArrangement (..) failed!"));
					return false;
				}
			}
			else
			{
				addErrorMessage (testResult, STR ("IComponent::getBusInfo (..) failed!"));
				return false;
			}
		}
	}
	return (arrangementMismatchs == 0);
}

//------------------------------------------------------------------------
} // Vst
} // Steinberg
