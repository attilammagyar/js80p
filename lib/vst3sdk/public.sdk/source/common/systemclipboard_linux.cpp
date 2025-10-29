//------------------------------------------------------------------------
// Flags       : clang-format SMTGSequencer
//
// Project     : Steinberg Plug-In SDK
// Filename    : public.sdk/source/common/systemclipboard_linux.cpp
// Created by  : Steinberg 04.2023
// Description : Simple helper allowing to copy/retrieve text to/from the system clipboard
//
//-----------------------------------------------------------------------------
// This file is part of a Steinberg SDK. It is subject to the license terms
// in the LICENSE file found in the top-level directory of this distribution
// and at www.steinberg.net/sdklicenses.
// No part of the SDK, including this file, may be copied, modified, propagated,
// or distributed except according to the terms contained in the LICENSE file.
//-----------------------------------------------------------------------------

#include "systemclipboard.h"
#include "pluginterfaces/base/fplatform.h"

#if SMTG_OS_LINUX

//------------------------------------------------------------------------
namespace Steinberg {
namespace SystemClipboard {

//-----------------------------------------------------------------------------
bool copyTextToClipboard (const std::string& text)
{
	// TODO
	return false;
}

//-----------------------------------------------------------------------------
bool getTextFromClipboard (std::string& text)
{
	// TODO
	return false;
}

//------------------------------------------------------------------------
} // namespace SystemClipboard
} // namespace Steinberg

#endif // SMTG_OS_LINUX
