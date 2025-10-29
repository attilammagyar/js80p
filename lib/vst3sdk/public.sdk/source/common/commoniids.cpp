//-----------------------------------------------------------------------------
// Project     : SDK Core
// Version     : 1.0
//
// Category    : Common Base Classes
// Filename    : public.sdk/source/common/commoniids.cpp
// Created by  : Steinberg, 01/2019
// Description : Define some IIDs
//
//-----------------------------------------------------------------------------
// This file is part of a Steinberg SDK. It is subject to the license terms
// in the LICENSE file found in the top-level directory of this distribution
// and at www.steinberg.net/sdklicenses.
// No part of the SDK, including this file, may be copied, modified, propagated,
// or distributed except according to the terms contained in the LICENSE file.
//-----------------------------------------------------------------------------

#include "pluginterfaces/gui/iplugview.h"
#include "pluginterfaces/gui/iplugviewcontentscalesupport.h"

namespace Steinberg
{
//----VST 3.0--------------------------------
DEF_CLASS_IID (IPlugView)
DEF_CLASS_IID (IPlugFrame)

//----VST 3.6.0--------------------------------
DEF_CLASS_IID (IPlugViewContentScaleSupport)

#if SMTG_OS_LINUX
DEF_CLASS_IID (Linux::IEventHandler)
DEF_CLASS_IID (Linux::ITimerHandler)
DEF_CLASS_IID (Linux::IRunLoop)
#endif

//------------------------------------------------------------------------
} // namespace Steinberg
