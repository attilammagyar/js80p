//-----------------------------------------------------------------------------
// Flags       : clang-format SMTGSequencer
// Project     : VST SDK
//
// Category    : Validator
// Filename    : public.sdk/source/vst/testsuite/processing/automation.h
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
#include <vector>

//------------------------------------------------------------------------
namespace Steinberg {
namespace Vst {

class ParamChanges;
//------------------------------------------------------------------------
/** Test Automation.
 * \ingroup TestClass
 */
class AutomationTest : public ProcessTest, public IParameterChanges
{
public:
	AutomationTest (ITestPlugProvider* plugProvider, ProcessSampleSize sampl, int32 everyNSamples,
	                int32 numParams, bool sampleAccuracy);
	~AutomationTest () override;

	const char* getName () const SMTG_OVERRIDE;
	// ITest
	bool PLUGIN_API setup () SMTG_OVERRIDE;
	bool PLUGIN_API run (ITestResult* testResult) SMTG_OVERRIDE;
	bool PLUGIN_API teardown () SMTG_OVERRIDE;

	// IParameterChanges
	int32 PLUGIN_API getParameterCount () SMTG_OVERRIDE;
	IParamValueQueue* PLUGIN_API getParameterData (int32 index) SMTG_OVERRIDE;
	IParamValueQueue* PLUGIN_API addParameterData (const ParamID& id, int32& index) SMTG_OVERRIDE;

	// FUnknown
	DELEGATE_REFCOUNT (ProcessTest)
	tresult PLUGIN_API queryInterface (const TUID _iid, void** obj) override;

//------------------------------------------------------------------------
protected:
	bool preProcess (ITestResult* testResult) SMTG_OVERRIDE;
	bool postProcess (ITestResult* testResult) SMTG_OVERRIDE;
	ParamID bypassId;

	using ParamChangeVector = std::vector<IPtr<ParamChanges>>;
	ParamChangeVector paramChanges;
	int32 countParamChanges;
	int32 everyNSamples;
	int32 numParams;
	bool sampleAccuracy;
	bool onceExecuted;
};

//------------------------------------------------------------------------
/** Test Parameters Flush (no Buffer).
 * \ingroup TestClass
 */
class FlushParamTest : public AutomationTest
{
public:
	FlushParamTest (ITestPlugProvider* plugProvider, ProcessSampleSize sampl);

	bool PLUGIN_API run (ITestResult* testResult) SMTG_OVERRIDE;

	DECLARE_VSTTEST ("Parameters Flush (no Buffer)")
protected:
	virtual void prepareProcessData ();
};

//------------------------------------------------------------------------
/** Test Parameters Flush 2 (no Buffer).
 * \ingroup TestClass
 */
class FlushParamTest2 : public FlushParamTest
{
public:
	FlushParamTest2 (ITestPlugProvider* plugProvider, ProcessSampleSize sampl);

	bool PLUGIN_API teardown () SMTG_OVERRIDE;

	DECLARE_VSTTEST ("Parameters Flush 2 (only numChannel==0)")
protected:
	void prepareProcessData () SMTG_OVERRIDE;
	int32 numInputs {0};
	int32 numOutputs {0};
	int32 numChannelsIn {0};
	int32 numChannelsOut {0};
};

//------------------------------------------------------------------------
/** Test Parameters Flush 3 (no Buffer, no parameter change).
 * \ingroup TestClass
 */
class FlushParamTest3 : public FlushParamTest
{
public:
	FlushParamTest3 (ITestPlugProvider* plugProvider, ProcessSampleSize sampl);

	DECLARE_VSTTEST ("Parameters Flush 2 (no Buffer, no parameter change)")
protected:
};

//------------------------------------------------------------------------
} // Vst
} // Steinberg
