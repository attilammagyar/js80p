//-----------------------------------------------------------------------------
// Flags       : clang-format SMTGSequencer
// Project     : VST SDK
//
// Category    : Validator
// Filename    : public.sdk/source/vst/testsuite/processing/processcontextrequirements.cpp
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

#include "public.sdk/source/vst/testsuite/processing/processcontextrequirements.h"
#include "public.sdk/source/vst/hosting/module.h"
#include "public.sdk/source/vst/utility/processcontextrequirements.h"
#include "public.sdk/source/vst/utility/versionparser.h"
#include "pluginterfaces/base/funknownimpl.h"

//------------------------------------------------------------------------
namespace Steinberg {
namespace Vst {
namespace {

//------------------------------------------------------------------------
VST3::Optional<VST3::Version> getPluginSDKVersion (ITestPlugProvider* plugProvider,
                                                   ITestResult* testResult)
{
	auto pp2 = U::cast<ITestPlugProvider2> (plugProvider);
	if (!pp2)
	{
		addErrorMessage (testResult, STR ("Internal test Error. Expected Interface not there!"));
		return {};
	}
	VST3::Hosting::PluginFactory pluginFactory (pp2->getPluginFactory ());
	if (!pluginFactory.get ())
	{
		addErrorMessage (testResult,
		                 STR ("Internal test Error. Expected PluginFactory not there!"));
		return {};
	}
	FUID fuid;
	if (pp2->getComponentUID (fuid) != kResultTrue)
	{
		addErrorMessage (testResult,
		                 STR ("Internal test Error. Could not query the UID of the plug-in!"));
		return {};
	}
	auto plugClassID = VST3::UID::fromTUID (fuid.toTUID ());
	auto classInfos = pluginFactory.classInfos ();
	auto it = std::find_if (classInfos.begin (), classInfos.end (),
	                        [&] (const auto& element) { return element.ID () == plugClassID; });
	if (it == classInfos.end ())
	{
		addErrorMessage (
		    testResult, STR ("Internal test Error. Could not find the class info of the plug-in!"));
		return {};
	}
	return VST3::Version::parse (it->sdkVersion ());
}

//------------------------------------------------------------------------
} // anonymous

//------------------------------------------------------------------------
// ProcessContextRequirementsTest
//------------------------------------------------------------------------
ProcessContextRequirementsTest::ProcessContextRequirementsTest (ITestPlugProvider* plugProvider)
: TestEnh (plugProvider, kSample32)
{
}

//------------------------------------------------------------------------
bool PLUGIN_API ProcessContextRequirementsTest::setup ()
{
	return TestEnh::setup ();
}

//------------------------------------------------------------------------
bool PLUGIN_API ProcessContextRequirementsTest::run (ITestResult* testResult)
{
	if (!vstPlug || !testResult || !audioEffect)
		return false;

	printTestHeader (testResult);

	// check if plug-in is build with any earlier VST SDK which does not support this interface
	auto sdkVersion = getPluginSDKVersion (plugProvider, testResult);
	if (!sdkVersion)
		return false;
	if (sdkVersion->getMajor () < 3 ||
	    (sdkVersion->getMajor () == 3 && sdkVersion->getMinor () < 7))
	{
		addMessage (testResult,
		            STR ("No ProcessContextRequirements required. Plug-In built with older SDK."));
		return true;
	}

	if (auto contextRequirements = U::cast<IProcessContextRequirements> (audioEffect))
	{
		ProcessContextRequirements req (contextRequirements->getProcessContextRequirements ());
		addMessage (testResult, STR ("ProcessContextRequirements:"));
		if (req.wantsNone ())
			addMessage (testResult, STR (" - None"));
		else
		{
			if (req.wantsSystemTime ())
				addMessage (testResult, STR (" - SystemTime"));
			if (req.wantsContinousTimeSamples ())
				addMessage (testResult, STR (" - ContinousTimeSamples"));
			if (req.wantsProjectTimeMusic ())
				addMessage (testResult, STR (" - ProjectTimeMusic"));
			if (req.wantsBarPositionMusic ())
				addMessage (testResult, STR (" - BarPosititionMusic"));
			if (req.wantsCycleMusic ())
				addMessage (testResult, STR (" - CycleMusic"));
			if (req.wantsSamplesToNextClock ())
				addMessage (testResult, STR (" - SamplesToNextClock"));
			if (req.wantsTempo ())
				addMessage (testResult, STR (" - Tempo"));
			if (req.wantsTimeSignature ())
				addMessage (testResult, STR (" - TimeSignature"));
			if (req.wantsChord ())
				addMessage (testResult, STR (" - Chord"));
			if (req.wantsFrameRate ())
				addMessage (testResult, STR (" - FrameRate"));
			if (req.wantsTransportState ())
				addMessage (testResult, STR (" - TransportState"));
		}
		return true;
	}

	addMessage (testResult,
	            STR ("Since VST SDK 3.7 you need to implement IProcessContextRequirements!"));
	addErrorMessage (testResult, STR ("Missing mandatory IProcessContextRequirements extension!"));
	return false;
}

//------------------------------------------------------------------------
} // Vst
} // Steinberg
