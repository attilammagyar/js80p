//-----------------------------------------------------------------------------
// Flags       : clang-format SMTGSequencer
// Project     : VST SDK
//
// Category    : Validator
// Filename    : public.sdk/source/vst/testsuite/general/editorclasses.cpp
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

#include "public.sdk/source/vst/testsuite/general/editorclasses.h"

//------------------------------------------------------------------------
namespace Steinberg {
namespace Vst {

//------------------------------------------------------------------------
// EditorClassesTest
//------------------------------------------------------------------------
EditorClassesTest::EditorClassesTest (ITestPlugProvider* plugProvider) : TestBase (plugProvider)
{
}

//------------------------------------------------------------------------
bool PLUGIN_API EditorClassesTest::run (ITestResult* testResult)
{
	if (!testResult || !vstPlug)
		return false;

	printTestHeader (testResult);

	// no controller is allowed...
	if (FUnknownPtr<IEditController> (vstPlug).getInterface ())
	{
		addMessage (testResult, STR ("Processor and edit controller united."));
		return true;
	}

	TUID controllerClassTUID;
	if (vstPlug->getControllerClassId (controllerClassTUID) != kResultOk)
	{
		addMessage (testResult,
		            STR ("This component does not export an edit controller class ID!!!"));
		return true;
	}
	FUID controllerClassUID;
	controllerClassUID = FUID::fromTUID (controllerClassTUID);
	if (controllerClassUID.isValid () == false)
	{
		addErrorMessage (testResult, STR ("The edit controller class has no valid UID!!!"));
		return false;
	}

	addMessage (testResult, STR ("This component has an edit controller class"));

	char8 cidString[50];

	controllerClassUID.toRegistryString (cidString);
	addMessage (testResult, printf ("   Controller CID: %s", cidString));

	return true;
}

//------------------------------------------------------------------------
} // Vst
} // Steinberg
