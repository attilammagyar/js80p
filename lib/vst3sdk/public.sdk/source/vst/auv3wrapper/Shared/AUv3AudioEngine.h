//-----------------------------------------------------------------------------
// Project     : VST SDK
//
// Category    : Helpers
// Filename    : AUv3AudioEngine.h
// Created by  : Steinberg, 07/2017.
// Description : VST 3 AUv3Wrapper
//
//-----------------------------------------------------------------------------
// This file is part of a Steinberg SDK. It is subject to the license terms
// in the LICENSE file found in the top-level directory of this distribution
// and at www.steinberg.net/sdklicenses.
// No part of the SDK, including this file, may be copied, modified, propagated,
// or distributed except according to the terms contained in the LICENSE file.
//-----------------------------------------------------------------------------

#pragma once

#import <AVFoundation/AVFoundation.h>

@interface AUv3AudioEngine : NSObject

@property (assign) AUAudioUnit* currentAudioUnit;

- (NSError*)loadAudioFile:(NSURL*)url;
- (instancetype)initWithComponentType:(uint32_t)unitComponentType;
- (void)loadAudioUnitWithComponentDescription:(AudioComponentDescription)desc
                                   completion:(void (^) (void))completionBlock;
- (BOOL)startStop;
- (void)shutdown;
@end
