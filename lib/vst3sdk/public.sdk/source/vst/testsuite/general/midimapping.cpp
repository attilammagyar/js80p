//-----------------------------------------------------------------------------
// Flags       : clang-format SMTGSequencer
// Project     : VST SDK
//
// Category    : Validator
// Filename    : public.sdk/source/vst/testsuite/general/midimapping.cpp
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

#include "public.sdk/source/vst/testsuite/general/midimapping.h"
#include "pluginterfaces/base/funknownimpl.h"
#include "pluginterfaces/vst/ivstmidicontrollers.h"
#include <unordered_set>

//------------------------------------------------------------------------
namespace Steinberg {
namespace Vst {

//------------------------------------------------------------------------
// MidiMappingTest
//------------------------------------------------------------------------
MidiMappingTest::MidiMappingTest (ITestPlugProvider* plugProvider) : TestBase (plugProvider)
{
}

//------------------------------------------------------------------------
bool PLUGIN_API MidiMappingTest::run (ITestResult* testResult)
{
	if (!testResult || !vstPlug)
		return false;

	printTestHeader (testResult);

	if (!controller)
	{
		addMessage (testResult, STR ("No Edit Controller supplied!"));
		return true;
	}

	auto midiMapping = U::cast<IMidiMapping> (controller);
	if (!midiMapping)
	{
		addMessage (testResult, STR ("No MIDI Mapping interface supplied!"));
		return true;
	}

	int32 numParameters = controller->getParameterCount ();
	int32 eventBusCount = vstPlug->getBusCount (kEvent, kInput);
	bool interruptProcess = false;

	std::unordered_set<ParamID> parameterIds;
	for (int32 i = 0; i < numParameters; ++i)
	{
		ParameterInfo parameterInfo;
		if (controller->getParameterInfo (i, parameterInfo) == kResultTrue)
			parameterIds.insert (parameterInfo.id);
	}
	for (int32 bus = 0; bus < eventBusCount + 1; bus++)
	{
		if (interruptProcess)
			break;

		BusInfo info;
		if (vstPlug->getBusInfo (kEvent, kInput, bus, info) == kResultTrue)
		{
			if (bus >= eventBusCount)
			{
				addMessage (testResult, STR ("getBusInfo supplied for an unknown event bus"));
				break;
			}
		}
		else
			break;

		for (int16 channel = 0; channel < info.channelCount; channel++)
		{
			if (interruptProcess)
				break;

			int32 foundCount = 0;
			// test with the cc outside the valid range too (>=kCountCtrlNumber)
			for (CtrlNumber cc = 0; cc < kCountCtrlNumber + 1; cc++)
			{
				ParamID tag;
				if (midiMapping->getMidiControllerAssignment (bus, channel, cc, tag) == kResultTrue)
				{
					if (bus >= eventBusCount)
					{
						addMessage (testResult,
						            STR ("MIDI Mapping supplied for an unknown event bus"));
						interruptProcess = true;
						break;
					}
					if (cc >= kCountCtrlNumber)
					{
						addMessage (
						    testResult,
						    STR (
						        "MIDI Mapping supplied for a wrong ControllerNumbers value (bigger than the max)"));
						break;
					}
					if (parameterIds.find (tag) == parameterIds.end ())
					{
						addErrorMessage (
						    testResult,
						    printf ("Unknown ParamID [%d] returned for MIDI Mapping", tag));
						return false;
					}
					foundCount++;
				}
				else
				{
					if (bus >= eventBusCount)
						interruptProcess = true;
				}
			}
			if (foundCount == 0 && (bus < eventBusCount))
			{
				addMessage (
				    testResult,
				    printf (
				        "MIDI Mapping getMidiControllerAssignment (%d, %d) : no assignment available!",
				        bus, channel));
			}
		}
	}

	return true;
}
//------------------------------------------------------------------------
} // Vst
} // Steinberg
