//-----------------------------------------------------------------------------
// Project     : VST SDK
//
// Category    : Helpers
// Filename    : public.sdk/source/vst/interappaudio/PresetSaveViewController.h
// Created by  : Steinberg, 09/2013
// Description : VST 3 InterAppAudio
// Flags       : clang-format SMTGSequencer
//
//-----------------------------------------------------------------------------
// This file is part of a Steinberg SDK. It is subject to the license terms
// in the LICENSE file found in the top-level directory of this distribution
// and at www.steinberg.net/sdklicenses.
// No part of the SDK, including this file, may be copied, modified, propagated,
// or distributed except according to the terms contained in the LICENSE file.
//-----------------------------------------------------------------------------

#pragma once

/// \cond ignore

#ifdef __OBJC__

#import <UIKit/UIKit.h>
#import <functional>

@interface PresetSaveViewController : UIViewController <UIAlertViewDelegate, UITextFieldDelegate>

- (id)initWithCallback:(std::function<void (const char* presetPath)>)callback;

@end

#endif //__OBJC__

/// \endcond
