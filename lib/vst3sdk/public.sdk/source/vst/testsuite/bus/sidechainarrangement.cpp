//-----------------------------------------------------------------------------
// Flags       : clang-format SMTGSequencer
// Project     : VST SDK
//
// Category    : Validator
// Filename    : public.sdk/source/vst/testsuite/bus/sidechainarrangement.cpp
// Created by  : Steinberg, 11/2019
// Description : VST Test Suite
//
//-----------------------------------------------------------------------------
// This file is part of a Steinberg SDK. It is subject to the license terms
// in the LICENSE file found in the top-level directory of this distribution
// and at www.steinberg.net/sdklicenses.
// No part of the SDK, including this file, may be copied, modified, propagated,
// or distributed except according to the terms contained in the LICENSE file.
//-----------------------------------------------------------------------------

#include "public.sdk/source/vst/testsuite/bus/sidechainarrangement.h"
#include "pluginterfaces/base/funknownimpl.h"

//------------------------------------------------------------------------
namespace Steinberg {
namespace Vst {

//------------------------------------------------------------------------
// SideChainArrangementTest
//------------------------------------------------------------------------
SideChainArrangementTest::SideChainArrangementTest (ITestPlugProvider* plugProvider)
: TestBase (plugProvider)
{
}

//------------------------------------------------------------------------
bool PLUGIN_API SideChainArrangementTest::run (ITestResult* testResult)
{
	if (!testResult || !vstPlug)
		return false;

	printTestHeader (testResult);

	bool failed = false;

	auto audioEffect = U::cast<IAudioProcessor> (vstPlug);
	if (!audioEffect)
		return failed;
	// get the side chain arrangements
	// set Main/first Input and output to Mono
	// get the current arrangement and compare

	// check if Audio sideChain is supported
	bool hasInputSideChain = false;
	int32 numInBusses = vstPlug->getBusCount (kAudio, kInput);
	if (numInBusses < 2)
		return true;

	for (int32 busIndex = 0; busIndex < numInBusses; busIndex++)
	{
		BusInfo info;
		if (vstPlug->getBusInfo (kAudio, kInput, busIndex, info) != kResultTrue)
		{
			addErrorMessage (testResult, STR ("IComponent::getBusInfo (..) failed."));
			continue;
		}
		if (info.busType == kAux)
			hasInputSideChain = true;
	}
	if (!hasInputSideChain)
		return true;

	auto* inputArrArray = new SpeakerArrangement[numInBusses];
	for (int32 busIndex = 0; busIndex < numInBusses; busIndex++)
	{
		if (audioEffect->getBusArrangement (kInput, busIndex, inputArrArray[busIndex]) !=
		    kResultTrue)
		{
			addErrorMessage (testResult, STR ("IComponent::getBusArrangement (..) failed."));
		}
	}

	int32 numOutBusses = vstPlug->getBusCount (kAudio, kOutput);
	SpeakerArrangement* outputArrArray = nullptr;
	if (numOutBusses > 0)
	{
		outputArrArray = new SpeakerArrangement[numOutBusses];
		for (int32 busIndex = 0; busIndex < numOutBusses; busIndex++)
		{
			if (audioEffect->getBusArrangement (kOutput, busIndex, outputArrArray[busIndex]) !=
			    kResultTrue)
			{
				addErrorMessage (testResult, STR ("IComponent::getBusArrangement (..) failed."));
			}
		}
		outputArrArray[0] = kSpeakerM;
	}
	inputArrArray[0] = kSpeakerM;

	if (audioEffect->setBusArrangements (inputArrArray, numInBusses, outputArrArray,
	                                     numOutBusses) == kResultTrue)
	{
		for (int32 busIndex = 0; busIndex < numInBusses; busIndex++)
		{
			SpeakerArrangement tmp;
			if (audioEffect->getBusArrangement (kInput, busIndex, tmp) == kResultTrue)
			{
				if (tmp != inputArrArray[busIndex])
				{
					addErrorMessage (
					    testResult,
					    printf (
					        "Input %d: setBusArrangements was returning kResultTrue but getBusArrangement returns different arrangement!",
					        busIndex));
					failed = true;
				}
			}
		}
		for (int32 busIndex = 0; busIndex < numOutBusses; busIndex++)
		{
			SpeakerArrangement tmp;
			if (audioEffect->getBusArrangement (kOutput, busIndex, tmp) != kResultTrue)
			{
				if (tmp != outputArrArray[busIndex])
				{
					addErrorMessage (
					    testResult,
					    printf (
					        "Output %d: setBusArrangements was returning kResultTrue but getBusArrangement returns different arrangement!",
					        busIndex));
					failed = true;
				}
			}
		}
	}

	return failed == false;
}

//------------------------------------------------------------------------
} // Vst
} // Steinberg
