//------------------------------------------------------------------------
// Flags       : clang-format SMTGSequencer
//
// Project     : Steinberg Plug-In SDK
// Filename    : public.sdk/source/common/systemclipboard.h
// Created by  : Steinberg 04.2020
// Description : Simple helper allowing to copy/retrieve text to/from the system clipboard
//
//-----------------------------------------------------------------------------
// This file is part of a Steinberg SDK. It is subject to the license terms
// in the LICENSE file found in the top-level directory of this distribution
// and at www.steinberg.net/sdklicenses.
// No part of the SDK, including this file, may be copied, modified, propagated,
// or distributed except according to the terms contained in the LICENSE file.
//-----------------------------------------------------------------------------

#pragma once

#include <string>

//------------------------------------------------------------------------
namespace Steinberg {
namespace SystemClipboard {

//-----------------------------------------------------------------------------
/** Copies the given text into the system clipboard
\ingroup sdkBase

\param text UTF-8 encoded text
\return true on success
*/
bool copyTextToClipboard (const std::string& text);

//-----------------------------------------------------------------------------
/** Retrieves the current text from the system clipboard
\ingroup sdkBase

\param text UTF-8 encoded text
\return true on success
*/
bool getTextFromClipboard (std::string& text);

//-----------------------------------------------------------------------------
} // namespace SystemClipboard
} // namespace Steinberg
