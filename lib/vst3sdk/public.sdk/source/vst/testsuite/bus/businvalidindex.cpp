//-----------------------------------------------------------------------------
// Flags       : clang-format SMTGSequencer
// Project     : VST SDK
//
// Category    : Validator
// Filename    : public.sdk/source/vst/testsuite/bus/businvalidindex.cpp
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

#include "public.sdk/source/vst/testsuite/bus/businvalidindex.h"

//------------------------------------------------------------------------
namespace Steinberg {
namespace Vst {

//------------------------------------------------------------------------
// BusInvalidIndexTest
//------------------------------------------------------------------------
BusInvalidIndexTest::BusInvalidIndexTest (ITestPlugProvider* plugProvider) : TestBase (plugProvider)
{
}

//------------------------------------------------------------------------
bool PLUGIN_API BusInvalidIndexTest::run (ITestResult* testResult)
{
	if (!testResult || !vstPlug)
		return false;

	printTestHeader (testResult);

	bool failed = false;
	int32 numInvalidDesc = 0;

	for (MediaType mediaType = kAudio; mediaType < kNumMediaTypes; mediaType++)
	{
		int32 numBusses =
		    vstPlug->getBusCount (mediaType, kInput) + vstPlug->getBusCount (mediaType, kOutput);
		for (BusDirection dir = kInput; dir <= kOutput; dir++)
		{
			BusInfo descBefore = {};
			BusInfo descAfter = {};

			int32 randIndex = 0;

			// todo: rand with negative numbers
			for (int32 i = 0; i <= numBusses * TestDefaults::instance ().numIterations; ++i)
			{
				randIndex = rand ();
				if (0 > randIndex || randIndex > numBusses)
				{
					/*tresult result =*/vstPlug->getBusInfo (mediaType, dir, randIndex, descAfter);

					if (memcmp ((void*)&descBefore, (void*)&descAfter, sizeof (BusInfo)) != 0)
					{
						failed |= true;
						numInvalidDesc++;
					}
				}
			}
		}
	}

	if (numInvalidDesc > 0)
	{
		addErrorMessage (testResult,
		                 printf ("The component returned %i buses queried with an invalid index!",
		                         numInvalidDesc));
	}

	return failed == false;
}

//------------------------------------------------------------------------
} // Vst
} // Steinberg
