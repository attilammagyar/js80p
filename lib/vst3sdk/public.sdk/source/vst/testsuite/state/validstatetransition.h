//-----------------------------------------------------------------------------
// Flags       : clang-format SMTGSequencer
// Project     : VST SDK
//
// Category    : Validator
// Filename    : public.sdk/source/vst/testsuite/state/validstatetransition.h
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

#include "public.sdk/source/vst/testsuite/processing/process.h"
#include "public.sdk/source/vst/testsuite/testbase.h"

//------------------------------------------------------------------------
namespace Steinberg {
namespace Vst {

//------------------------------------------------------------------------
/** Test Valid State Transition.
 * \ingroup TestClass
 */
class ValidStateTransitionTest : public ProcessTest
{
public:
	ValidStateTransitionTest (ITestPlugProvider* plugProvider, ProcessSampleSize sampleSize);

	bool PLUGIN_API run (ITestResult* testResult) SMTG_OVERRIDE;

	const char* getName () const SMTG_OVERRIDE { return name; }

protected:
	char name[256];
};

//------------------------------------------------------------------------
} // Vst
} // Steinberg
