//-----------------------------------------------------------------------------
// Flags       : clang-format SMTGSequencer
// Project     : VST SDK
//
// Category    : Validator
// Filename    : public.sdk/source/vst/testsuite/general/plugcompat.h
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

#pragma once

#include "public.sdk/source/vst/hosting/module.h"
#include "pluginterfaces/base/iplugincompatibility.h"
#include <iosfwd>

//------------------------------------------------------------------------
namespace Steinberg {

//------------------------------------------------------------------------
bool checkPluginCompatibility (VST3::Hosting::Module::Ptr& module,
                               IPtr<IPluginCompatibility> compat, std::ostream* errorStream);

//------------------------------------------------------------------------
} // Steinberg
