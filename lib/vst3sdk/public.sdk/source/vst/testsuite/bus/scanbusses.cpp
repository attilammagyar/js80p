//-----------------------------------------------------------------------------
// Flags       : clang-format SMTGSequencer
// Project     : VST SDK
//
// Category    : Validator
// Filename    : public.sdk/source/vst/testsuite/bus/scanbusses.cpp
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

#include "public.sdk/source/vst/testsuite/bus/scanbusses.h"
#include "public.sdk/source/vst/utility/stringconvert.h"

//------------------------------------------------------------------------
namespace Steinberg {
namespace Vst {

//------------------------------------------------------------------------
// ScanBussesTest
//------------------------------------------------------------------------
ScanBussesTest::ScanBussesTest (ITestPlugProvider* plugProvider) : TestBase (plugProvider)
{
}

//------------------------------------------------------------------------
bool PLUGIN_API ScanBussesTest::run (ITestResult* testResult)
{
	if (!testResult || !vstPlug)
		return false;

	printTestHeader (testResult);

	int32 numBusses = 0;

	for (MediaType mediaType = kAudio; mediaType < kNumMediaTypes; mediaType++)
	{
		int32 numInputs = vstPlug->getBusCount (mediaType, kInput);
		int32 numOutputs = vstPlug->getBusCount (mediaType, kOutput);

		numBusses += (numInputs + numOutputs);

		if ((mediaType == (kNumMediaTypes - 1)) && (numBusses == 0))
		{
			addErrorMessage (testResult, STR ("This component does not export any buses!!!"));
			return false;
		}

		addMessage (testResult,
		            printf ("=> %s Buses: [%d In(s) => %d Out(s)]",
		                    mediaType == kAudio ? "Audio" : "Event", numInputs, numOutputs));

		for (int32 i = 0; i < numInputs + numOutputs; ++i)
		{
			BusDirection busDirection = i < numInputs ? kInput : kOutput;
			int32 busIndex = busDirection == kInput ? i : i - numInputs;

			BusInfo busInfo = {};
			if (vstPlug->getBusInfo (mediaType, busDirection, busIndex, busInfo) == kResultTrue)
			{
				auto busName = StringConvert::convert (busInfo.name);

				if (busName.empty ())
				{
					addErrorMessage (testResult, printf ("Bus %d has no name!!!", busIndex));
					return false;
				}
				addMessage (
				    testResult,
				    printf ("     %s[%d]: \"%s\" (%s-%s) ", busDirection == kInput ? "In " : "Out",
				            busIndex, busName.data (), busInfo.busType == kMain ? "Main" : "Aux",
				            busInfo.kDefaultActive ? "Default Active" : "Default Inactive"));
			}
			else
				return false;
		}
	}
	return true;
}

//------------------------------------------------------------------------
} // Vst
} // Steinberg
