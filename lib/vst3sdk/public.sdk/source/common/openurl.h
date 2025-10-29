//------------------------------------------------------------------------
// Flags       : clang-format SMTGSequencer
//
// Category    : Helpers
// Project     : Steinberg Plug-In SDK
// Filename    : public.sdk/source/common/openurl.h
// Created by  : Steinberg 04.2020
// Description : Simple helper allowing to open a URL in the default associated application
//
//-----------------------------------------------------------------------------
// This file is part of a Steinberg SDK. It is subject to the license terms
// in the LICENSE file found in the top-level directory of this distribution
// and at www.steinberg.net/sdklicenses.
// No part of the SDK, including this file, may be copied, modified, propagated,
// or distributed except according to the terms contained in the LICENSE file.
//-----------------------------------------------------------------------------

#pragma once

#include "base/source/fstring.h"

namespace Steinberg {

/** Open the given URL into the default web browser.
\ingroup sdkBase

It returns true if a default application is found and opened else false.
Example:
\code{.cpp}
if (openURLInDefaultApplication ("https://www.steinberg.net/"))
{
    // everything seems to be ok
}
\endcode
*/
bool openURLInDefaultApplication (const String& address);

//------------------------------------------------------------------------
} // namespace Steinberg
