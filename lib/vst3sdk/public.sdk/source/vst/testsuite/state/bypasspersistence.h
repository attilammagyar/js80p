//-----------------------------------------------------------------------------
// Flags       : clang-format SMTGSequencer
// Project     : VST SDK
//
// Category    : Validator
// Filename    : public.sdk/source/vst/testsuite/state/bypassstate.h
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

#pragma once

#include "public.sdk/source/vst/testsuite/processing/automation.h"

//------------------------------------------------------------------------
namespace Steinberg {
namespace Vst {

//------------------------------------------------------------------------
/** Test Parameter Bypass persistence.
 * \ingroup TestClass
 */
class BypassPersistenceTest : public AutomationTest
{
public:
	BypassPersistenceTest (ITestPlugProvider* plugProvider, ProcessSampleSize sampl);

	bool PLUGIN_API run (ITestResult* testResult) override;

	DECLARE_VSTTEST ("Parameter Bypass persistence")
};

//------------------------------------------------------------------------
} // Vst
} // Steinberg
