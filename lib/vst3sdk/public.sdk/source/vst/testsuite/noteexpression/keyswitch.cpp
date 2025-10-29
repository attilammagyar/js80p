//-----------------------------------------------------------------------------
// Flags       : clang-format SMTGSequencer
// Project     : VST SDK
//
// Category    : Validator
// Filename    : public.sdk/source/vst/testsuite/noteexpression/keyswitch.cpp
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

#include "public.sdk/source/vst/testsuite/noteexpression/keyswitch.h"
#include "pluginterfaces/base/funknownimpl.h"
#include "pluginterfaces/vst/ivstnoteexpression.h"

//------------------------------------------------------------------------
namespace Steinberg {
namespace Vst {

//------------------------------------------------------------------------
// KeyswitchTest
//------------------------------------------------------------------------
KeyswitchTest::KeyswitchTest (ITestPlugProvider* plugProvider) : TestBase (plugProvider)
{
}

//------------------------------------------------------------------------
bool PLUGIN_API KeyswitchTest::run (ITestResult* testResult)
{
	if (!testResult || !vstPlug)
		return false;

	printTestHeader (testResult);

	if (!controller)
	{
		addMessage (testResult, STR ("No Edit Controller supplied!"));
		return true;
	}

	auto keyswitch = U::cast<IKeyswitchController> (controller);
	if (!keyswitch)
	{
		addMessage (testResult, STR ("No Keyswitch interface supplied!"));
		return true;
	}

	int32 eventBusCount = vstPlug->getBusCount (kEvent, kInput);

	for (int32 bus = 0; bus < eventBusCount; bus++)
	{
		BusInfo busInfo;
		vstPlug->getBusInfo (kEvent, kInput, bus, busInfo);

		for (int16 channel = 0; channel < busInfo.channelCount; channel++)
		{
			int32 count = keyswitch->getKeyswitchCount (bus, channel);

			if (count > 0)
			{
				addMessage (testResult, printf ("Keyswitch support bus[%d], channel[%d]: %d", bus,
				                                channel, count));
			}

			for (int32 i = 0; i < count; ++i)
			{
				KeyswitchInfo info;
				if (keyswitch->getKeyswitchInfo (bus, channel, i, info) == kResultTrue)
				{
				}
				else
				{
					addErrorMessage (
					    testResult,
					    printf ("Keyswitch getKeyswitchInfo (%d, %d, %d) return kResultFalse!", bus,
					            channel, i));
					return false;
				}
			}
		}
	}

	return true;
}

//------------------------------------------------------------------------
} // Vst
} // Steinberg
