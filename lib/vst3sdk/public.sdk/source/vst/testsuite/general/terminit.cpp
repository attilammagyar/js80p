//-----------------------------------------------------------------------------
// Flags       : clang-format SMTGSequencer
// Project     : VST SDK
//
// Category    : Validator
// Filename    : public.sdk/source/vst/testsuite/general/terminit.cpp
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

#include "public.sdk/source/vst/testsuite/general/terminit.h"
#include "pluginterfaces/base/funknownimpl.h"

//------------------------------------------------------------------------
namespace Steinberg {
namespace Vst {

//------------------------------------------------------------------------
// TerminateInitializeTest
//------------------------------------------------------------------------
TerminateInitializeTest::TerminateInitializeTest (ITestPlugProvider* plugProvider)
: TestBase (plugProvider)
{
}

//------------------------------------------------------------------------
bool TerminateInitializeTest::run (ITestResult* testResult)
{
	if (!testResult || !vstPlug)
		return false;

	printTestHeader (testResult);
	auto plugBase = U::cast<IPluginBase> (vstPlug);
	if (!plugBase)
	{
		addErrorMessage (testResult, STR ("No IPluginBase interface available."));
		return false;
	}

	bool result = true;
	if (plugBase->terminate () != kResultTrue)
	{
		addErrorMessage (testResult, STR ("IPluginBase::terminate () failed."));
		result = false;
	}
	if (plugBase->initialize (TestingPluginContext::get ()) != kResultTrue)
	{
		addErrorMessage (testResult, STR ("IPluginBase::initialize (..) failed."));
		result = false;
	}
	return result;
}

//------------------------------------------------------------------------
} // Vst
} // Steinberg
