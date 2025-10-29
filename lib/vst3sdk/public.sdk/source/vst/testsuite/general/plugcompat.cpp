//-----------------------------------------------------------------------------
// Flags       : clang-format SMTGSequencer
// Project     : VST SDK
//
// Category    : Validator
// Filename    : public.sdk/source/vst/testsuite/general/plugcompat.cpp
// Created by  : Steinberg, 03/2022
// Description : VST Test Suite
//
//-----------------------------------------------------------------------------
// This file is part of a Steinberg SDK. It is subject to the license terms
// in the LICENSE file found in the top-level directory of this distribution
// and at www.steinberg.net/sdklicenses.
// No part of the SDK, including this file, may be copied, modified, propagated,
// or distributed except according to the terms contained in the LICENSE file.
//-----------------------------------------------------------------------------

#include "public.sdk/source/vst/testsuite/general/plugcompat.h"
#include "public.sdk/source/vst/moduleinfo/moduleinfoparser.h"
#include "pluginterfaces/base/funknownimpl.h"
#include <string>

//------------------------------------------------------------------------
namespace Steinberg {

//------------------------------------------------------------------------
namespace {

//------------------------------------------------------------------------
struct StringStream : U::ImplementsNonDestroyable<U::Directly<IBStream>>
{
	std::string str;
	tresult PLUGIN_API read (void*, int32, int32*) override { return kNotImplemented; }
	tresult PLUGIN_API write (void* buffer, int32 numBytes, int32* numBytesWritten) override
	{
		str.append (static_cast<char*> (buffer), numBytes);
		if (numBytesWritten)
			*numBytesWritten = numBytes;
		return kResultTrue;
	}
	tresult PLUGIN_API seek (int64, int32, int64*) override { return kNotImplemented; }
	tresult PLUGIN_API tell (int64*) override { return kNotImplemented; }
};

//------------------------------------------------------------------------
} // anonymous

//------------------------------------------------------------------------
bool checkPluginCompatibility (VST3::Hosting::Module::Ptr& module,
                               IPtr<IPluginCompatibility> compat, std::ostream* errorStream)
{
	bool failure = false;
	if (auto moduleInfoPath = VST3::Hosting::Module::getModuleInfoPath (module->getPath ()))
	{
		if (errorStream)
		{
			*errorStream
			    << "Warning: The module contains a moduleinfo.json file and the module factory exports a IPluginCompatibility class. The moduleinfo.json one is preferred.\n";
		}
	}
	StringStream strStream;
	if (compat->getCompatibilityJSON (&strStream) != kResultTrue)
	{
		if (errorStream)
		{
			*errorStream
			    << "Error: Call to IPluginCompatiblity::getCompatibilityJSON (IBStream*) failed\n";
		}
		failure = true;
	}
	else if (auto result = ModuleInfoLib::parseCompatibilityJson (strStream.str, errorStream))
	{
		// TODO: Check that the "New" classes are part of the Module;
	}
	else
	{
		failure = true;
	}
	return !failure;
}

//------------------------------------------------------------------------
} // Steinberg
