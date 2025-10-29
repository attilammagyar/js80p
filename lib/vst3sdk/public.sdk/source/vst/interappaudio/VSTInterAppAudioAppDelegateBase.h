//-----------------------------------------------------------------------------
// Project     : VST SDK
//
// Category    : Helpers
// Filename    : public.sdk/source/vst/interappaudio/VSTInterAppAudioAppDelegateBase.h
// Created by  : Steinberg, 08/2013.
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

#import <UIKit/UIKit.h>

//------------------------------------------------------------------------
/** Base UIApplicationDelegate class.
 *	This class provides the base handling of the audio engine, plug-in and plug-in editor\n
 *	You should subclass it for customization\n
 *	Make sure to call the methods of this class if you override one in your subclass !
 */
//------------------------------------------------------------------------
@interface VSTInterAppAudioAppDelegateBase : UIResponder <UIApplicationDelegate>
//------------------------------------------------------------------------
@property (strong, nonatomic) UIWindow* window;

- (BOOL)application:(UIApplication*)application
    willFinishLaunchingWithOptions:(NSDictionary*)launchOptions;
- (BOOL)application:(UIApplication*)application
    didFinishLaunchingWithOptions:(NSDictionary*)launchOptions;
- (BOOL)application:(UIApplication*)application shouldSaveApplicationState:(NSCoder*)coder;
- (BOOL)application:(UIApplication*)application shouldRestoreApplicationState:(NSCoder*)coder;
- (void)applicationDidBecomeActive:(UIApplication*)application;
- (void)applicationWillResignActive:(UIApplication*)application;

@end
