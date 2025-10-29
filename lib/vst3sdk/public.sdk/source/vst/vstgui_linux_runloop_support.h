//------------------------------------------------------------------------
// Project     : VST SDK
//
// Category    : Helpers
// Filename    : public.sdk/source/vst/vstgui_linux_runloop_support.h
// Created by  : Steinberg, 10/2025
// Description : VSTGUI Linux Runloop Support
//
//-----------------------------------------------------------------------------
// This file is part of a Steinberg SDK. It is subject to the license terms
// in the LICENSE file found in the top-level directory of this distribution
// and at www.steinberg.net/sdklicenses.
// No part of the SDK, including this file, may be copied, modified, propagated,
// or distributed except according to the terms contained in the LICENSE file.
//-----------------------------------------------------------------------------

#pragma once

#include "pluginterfaces/base/funknown.h"

//-----------------------------------------------------------------------------
namespace Steinberg::Linux {

bool setupVSTGUIRunloop (FUnknown* hostContext);

//-----------------------------------------------------------------------------
} // Steinberg::Linux
