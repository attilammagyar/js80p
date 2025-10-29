//-----------------------------------------------------------------------------
// Flags       : clang-format SMTGSequencer
// Project     : VST SDK
//
// Category    : Validator
// Filename    : public.sdk/source/vst/testsuite/processing/process.h
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

#include "public.sdk/source/vst/hosting/processdata.h"
#include "public.sdk/source/vst/testsuite/testbase.h"

//------------------------------------------------------------------------
namespace Steinberg {
namespace Vst {

//------------------------------------------------------------------------
/** Test Process Test.
 * \ingroup TestClass
 */
class ProcessTest : public TestEnh
{
public:
	ProcessTest (ITestPlugProvider* plugProvider, ProcessSampleSize sampl);

	DECLARE_VSTTEST ("Process Test")

	// ITest
	bool PLUGIN_API setup () SMTG_OVERRIDE;
	bool PLUGIN_API run (ITestResult* testResult) SMTG_OVERRIDE;
	bool PLUGIN_API teardown () SMTG_OVERRIDE;

//------------------------------------------------------------------------
protected:
	virtual bool prepareProcessing (); ///< setup ProcessData and allocate buffers
	virtual bool unprepareProcessing (); ///< free dynamic memory of ProcessData
	virtual bool preProcess (ITestResult* testResult); ///< is called just before the process call
	virtual bool postProcess (ITestResult* testResult); ///< is called right after the process call

	bool setupBuffers (int32 numBusses, AudioBusBuffers* audioBuffers, BusDirection dir);
	bool setupBuffers (AudioBusBuffers& audioBuffers);
	bool freeBuffers (int32 numBuses, AudioBusBuffers* buses);
	bool canProcessSampleSize (ITestResult* testResult); ///< audioEffect has to be available

	HostProcessData processData;
};

//------------------------------------------------------------------------
} // Vst
} // Steinberg
