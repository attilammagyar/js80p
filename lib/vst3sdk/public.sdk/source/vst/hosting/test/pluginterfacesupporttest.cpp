//-----------------------------------------------------------------------------
// Project     : VST SDK
//
// Category    : Helpers
// Filename    : public.sdk/source/vst/hosting/test/pluginterfacesupporttest.cpp
// Created by  : Steinberg, 08/2021
// Description : Test pluginterface support helper
// Flags       : clang-format SMTGSequencer
//
//-----------------------------------------------------------------------------
// This file is part of a Steinberg SDK. It is subject to the license terms
// in the LICENSE file found in the top-level directory of this distribution
// and at www.steinberg.net/sdklicenses. 
// No part of the SDK, including this file, may be copied, modified, propagated,
// or distributed except according to the terms contained in the LICENSE file.
//-----------------------------------------------------------------------------

#include "public.sdk/source/main/moduleinit.h"
#include "public.sdk/source/vst/hosting/pluginterfacesupport.h"
#include "public.sdk/source/vst/utility/testing.h"

#include "pluginterfaces/vst/ivstaudioprocessor.h"
#include "pluginterfaces/vst/ivsteditcontroller.h"
#include "pluginterfaces/vst/ivstmessage.h"
#include "pluginterfaces/vst/ivstunits.h"

//------------------------------------------------------------------------
namespace Steinberg {
namespace Vst {
namespace {

//------------------------------------------------------------------------
ModuleInitializer PlugInterfaceSupportTests ([] () {
	constexpr auto TestSuiteName = "PlugInterfaceSupport";
	registerTest (TestSuiteName, STR ("Initial interfaces"), [] (ITestResult* testResult) {
		PlugInterfaceSupport pis;
		//---VST 3.0.0--------------------------------
		EXPECT_EQ (pis.isPlugInterfaceSupported (IComponent::iid), kResultTrue);
		EXPECT_EQ (pis.isPlugInterfaceSupported (IAudioProcessor::iid), kResultTrue);
		EXPECT_EQ (pis.isPlugInterfaceSupported (IEditController::iid), kResultTrue);
		EXPECT_EQ (pis.isPlugInterfaceSupported (IConnectionPoint::iid), kResultTrue);
		EXPECT_EQ (pis.isPlugInterfaceSupported (IUnitInfo::iid), kResultTrue);
		EXPECT_EQ (pis.isPlugInterfaceSupported (IUnitData::iid), kResultTrue);
		EXPECT_EQ (pis.isPlugInterfaceSupported (IProgramListData::iid), kResultTrue);
		//---VST 3.0.1--------------------------------
		EXPECT_EQ (pis.isPlugInterfaceSupported (IMidiMapping::iid), kResultTrue);
		//---VST 3.1----------------------------------
		EXPECT_EQ (pis.isPlugInterfaceSupported (IEditController2::iid), kResultTrue);
		return true;
	});
	registerTest (TestSuiteName, STR ("Add interface"), [] (ITestResult* testResult) {
		PlugInterfaceSupport pis;
		EXPECT_NE (pis.isPlugInterfaceSupported (IEditControllerHostEditing::iid), kResultTrue);
		pis.addPlugInterfaceSupported (IEditControllerHostEditing::iid);
		EXPECT_EQ (pis.isPlugInterfaceSupported (IEditControllerHostEditing::iid), kResultTrue);
		return true;
	});
	registerTest (TestSuiteName, STR ("Remove interface"), [] (ITestResult* testResult) {
		PlugInterfaceSupport pis;
		EXPECT_NE (pis.isPlugInterfaceSupported (IEditControllerHostEditing::iid), kResultTrue);
		pis.addPlugInterfaceSupported (IEditControllerHostEditing::iid);
		EXPECT_EQ (pis.isPlugInterfaceSupported (IEditControllerHostEditing::iid), kResultTrue);
		EXPECT_TRUE (pis.removePlugInterfaceSupported (IEditControllerHostEditing::iid));
		EXPECT_NE (pis.isPlugInterfaceSupported (IEditControllerHostEditing::iid), kResultTrue);
		return true;
	});
});

//------------------------------------------------------------------------
} // anonymous
} // Vst
} // Steinberg
