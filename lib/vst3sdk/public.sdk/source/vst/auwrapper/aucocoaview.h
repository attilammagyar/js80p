//------------------------------------------------------------------------
// Project     : VST SDK
//
// Category    : Helpers
// Filename    : public.sdk/source/vst/auwrapper/aucocoaview.h
// Created by  : Steinberg, 12/2007
// Description : VST 3 -> AU Wrapper
//
//-----------------------------------------------------------------------------
// This file is part of a Steinberg SDK. It is subject to the license terms
// in the LICENSE file found in the top-level directory of this distribution
// and at www.steinberg.net/sdklicenses.
// No part of the SDK, including this file, may be copied, modified, propagated,
// or distributed except according to the terms contained in the LICENSE file.
//-----------------------------------------------------------------------------
/// \cond ignore

#pragma once

#ifndef SMTG_AUCocoaUIBase_CLASS_NAME
#import "aucocoaclassprefix.h"
#endif

#import <Foundation/Foundation.h>
#import <AudioUnit/AUCocoaUIView.h>

//------------------------------------------------------------------------
@interface SMTG_AUCocoaUIBase_CLASS_NAME : NSObject<AUCocoaUIBase>
@end

/// \endcond
