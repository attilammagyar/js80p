//-----------------------------------------------------------------------------
// Flags       : clang-format SMTGSequencer
// Project     : VST SDK
//
// Category    : Validator
// Filename    : public.sdk/source/vst/testsuite/bus/busconsistency.cpp
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

#include "public.sdk/source/vst/testsuite/bus/busconsistency.h"

//------------------------------------------------------------------------
namespace Steinberg {
namespace Vst {

//------------------------------------------------------------------------
// BusConsistencyTest
//------------------------------------------------------------------------
BusConsistencyTest::BusConsistencyTest (ITestPlugProvider* plugProvider) : TestBase (plugProvider)
{
}

//------------------------------------------------------------------------
bool PLUGIN_API BusConsistencyTest::run (ITestResult* testResult)
{
	if (!testResult || !vstPlug)
		return false;

	printTestHeader (testResult);

	bool failed = false;
	int32 numFalseDescQueries = 0;

	for (MediaType mediaType = kAudio; mediaType < kNumMediaTypes; mediaType++)
	{
		for (BusDirection dir = kInput; dir <= kOutput; dir++)
		{
			int32 numBusses = vstPlug->getBusCount (mediaType, dir);
			if (numBusses > 0)
			{
				auto* busArray = new BusInfo[numBusses];
				if (busArray)
				{
					// get all bus descriptions and save them in an array
					int32 busIndex;
					for (busIndex = 0; busIndex < numBusses; busIndex++)
					{
						memset (&busArray[busIndex], 0, sizeof (BusInfo));
						vstPlug->getBusInfo (mediaType, dir, busIndex, busArray[busIndex]);
					}

					// test by getting descriptions randomly and comparing with saved ones
					int32 randIndex = 0;
					BusInfo info = {};

					for (busIndex = 0;
					     busIndex <= numBusses * TestDefaults::instance ().numIterations;
					     busIndex++)
					{
						randIndex = rand () % (numBusses);

						memset (&info, 0, sizeof (BusInfo));

						/*tresult result =*/vstPlug->getBusInfo (mediaType, dir, randIndex, info);
						if (memcmp ((void*)&busArray[randIndex], (void*)&info, sizeof (BusInfo)) !=
						    TestDefaults::instance ().buffersAreEqual)
						{
							failed |= true;
							numFalseDescQueries++;
						}
					}
					delete[] busArray;
				}
			}
		}
	}

	if (numFalseDescQueries > 0)
	{
		addErrorMessage (
		    testResult,
		    printf (
		        "The component returned %i inconsistent buses! (getBusInfo () returns sometime different info for the same bus!",
		        numFalseDescQueries));
	}

	return failed == false;
}

//------------------------------------------------------------------------
} // Vst
} // Steinberg
