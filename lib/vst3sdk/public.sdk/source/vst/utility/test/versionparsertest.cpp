//-----------------------------------------------------------------------------
// Project     : VST SDK
//
// Category    : Helpers
// Filename    : public.sdk/source/vst/utility/test/versionparsertest.cpp
// Created by  : Steinberg, 12/2019
// Description : Test version parser
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
#include "public.sdk/source/vst/utility/testing.h"
#include "public.sdk/source/vst/utility/versionparser.h"
#include "pluginterfaces/base/fstrdefs.h"

//------------------------------------------------------------------------
namespace Steinberg {
namespace Vst {

//------------------------------------------------------------------------
static ModuleInitializer InitVersionParserTests ([] () {
	registerTest ("VersionParser", STR ("Parsing 'SDK 3.7'"), [] (ITestResult* testResult) {
		auto version = VST3::Version::parse ("SDK 3.7");
		if (version.getMajor () != 3 || version.getMinor () != 7 || version.getSub () != 0 ||
		    version.getBuildnumber () != 0)
		{
			testResult->addErrorMessage (STR ("Parsing 'SDK 3.7' failed"));
			return false;
		}
		return true;
	});
	registerTest ("VersionParser", STR ("Parsing 'SDK 3.7.1.38'"), [] (ITestResult* testResult) {
		auto version = VST3::Version::parse ("3.7.1.38");
		if (version.getMajor () != 3 || version.getMinor () != 7 || version.getSub () != 1 ||
		    version.getBuildnumber () != 38)
		{
			testResult->addErrorMessage (STR ("Parsing '3.7.1.38' failed"));
			return false;
		}
		return true;
	});
	registerTest ("VersionParser", STR ("Parsing 'SDK 3.7 Prerelease'"),
	              [] (ITestResult* testResult) {
		              auto version = VST3::Version::parse ("SDK 3.7 Prerelease");
		              if (version.getMajor () != 3 || version.getMinor () != 7 ||
		                  version.getSub () != 0 || version.getBuildnumber () != 0)
		              {
			              testResult->addErrorMessage (STR ("Parsing 'SDK 3.7 Prerelease' failed"));
			              return false;
		              }
		              return true;
	              });
	registerTest ("VersionParser", STR ("Parsing 'SDK 3.7-99'"), [] (ITestResult* testResult) {
		auto version = VST3::Version::parse ("SDK 3.7-99");
		if (version.getMajor () != 3 || version.getMinor () != 7 || version.getSub () != 0 ||
		    version.getBuildnumber () != 0)
		{
			testResult->addErrorMessage (STR ("Parsing 'SDK 3.7-99' failed"));
			return false;
		}
		return true;
	});
	registerTest ("VersionParser", STR ("Parsing 'No version at all'"),
	              [] (ITestResult* testResult) {
		              auto version = VST3::Version::parse ("No version at all");
		              if (version.getMajor () != 0 || version.getMinor () != 0 ||
		                  version.getSub () != 0 || version.getBuildnumber () != 0)
		              {
			              testResult->addErrorMessage (STR ("Parsing 'No version at all' failed"));
			              return false;
		              }
		              return true;
	              });
});

//------------------------------------------------------------------------
} // Vst
} // Steinberg
