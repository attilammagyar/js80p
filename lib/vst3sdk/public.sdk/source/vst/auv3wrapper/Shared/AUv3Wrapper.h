//-----------------------------------------------------------------------------
// Project     : VST SDK
//
// Category    : Helpers
// Filename    : AUv3Wrapper.h
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

#import <CoreAudioKit/AUViewController.h>

@class AUv3Wrapper;

//------------------------------------------------------------------------
@interface AUv3WrapperViewController : AUViewController

@property (nonatomic, strong) AUv3Wrapper* audioUnit;

@end

//------------------------------------------------------------------------
@interface AUv3Wrapper : AUAudioUnit
- (void)beginEdit:(int32_t)tag;
- (void)endEdit:(int32_t)tag;
- (void)performEdit:(int32_t)tag value:(double)value;
- (void)syncParameterValues;
- (void)updateParameters;
- (void)onTimer;
- (void)onParamTitlesChanged;
- (void)onNoteExpressionChanged;
- (void)onLatencyChanged;
- (BOOL)enableMPESupport:(BOOL)state;
- (BOOL)setMPEInputDeviceMasterChannel:(NSInteger)masterChannel
                    memberBeginChannel:(NSInteger)memberBeginChannel
                      memberEndChannel:(NSInteger)memberEndChannel;
@end
