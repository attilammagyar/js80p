//-----------------------------------------------------------------------------
// Flags       : clang-format SMTGSequencer
// Project     : VST SDK
//
// Category    : Validator
// Filename    : public.sdk/source/vst/testsuite/bus/busactivation.cpp
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

#include "public.sdk/source/vst/testsuite/bus/busactivation.h"

//------------------------------------------------------------------------
namespace Steinberg {
namespace Vst {

//------------------------------------------------------------------------
// BusActivationTest
//------------------------------------------------------------------------
BusActivationTest::BusActivationTest (ITestPlugProvider* plugProvider) : TestBase (plugProvider)
{
}

//------------------------------------------------------------------------
bool PLUGIN_API BusActivationTest::run (ITestResult* testResult)
{
	if (!testResult || !vstPlug)
		return false;

	printTestHeader (testResult);

	int32 numTotalBusses = 0;
	int32 numFailedActivations = 0;

	for (MediaType type = kAudio; type < kNumMediaTypes; type++)
	{
		int32 numInputs = vstPlug->getBusCount (type, kInput);
		int32 numOutputs = vstPlug->getBusCount (type, kOutput);

		numTotalBusses += (numInputs + numOutputs);

		for (int32 i = 0; i < numInputs + numOutputs; ++i)
		{
			BusDirection busDirection = i < numInputs ? kInput : kOutput;
			int32 busIndex = busDirection == kInput ? i : i - numInputs;

			BusInfo busInfo = {};
			if (vstPlug->getBusInfo (type, busDirection, busIndex, busInfo) != kResultTrue)
			{
				addErrorMessage (testResult, STR ("IComponent::getBusInfo (..) failed."));
				return false;
			}

			addMessage (testResult, printf ("   Bus Activation: %s %s Bus (%d) (%s)",
			                                busDirection == kInput ? "Input" : "Output",
			                                type == kAudio ? "Audio" : "Event", busIndex,
			                                busInfo.busType == kMain ? "kMain" : "kAux"));

			if ((busInfo.flags & BusInfo::kDefaultActive) == false)
			{
				if (vstPlug->activateBus (type, busDirection, busIndex, true) != kResultOk)
					numFailedActivations++;
				if (vstPlug->activateBus (type, busDirection, busIndex, false) != kResultOk)
					numFailedActivations++;
			}
			else if ((busInfo.flags & BusInfo::kDefaultActive) == true)
			{
				if (vstPlug->activateBus (type, busDirection, busIndex, false) != kResultOk)
					numFailedActivations++;
				if (vstPlug->activateBus (type, busDirection, busIndex, true) != kResultOk)
					numFailedActivations++;
			}
		}
	}

	if (numFailedActivations > 0)
		addErrorMessage (testResult, STR ("Bus activation failed."));

	return (numFailedActivations == 0);
}

//------------------------------------------------------------------------
} // Vst
} // Steinberg
