//-----------------------------------------------------------------------------
// Project     : VST SDK
//
// Category    : Helpers
// Filename    :
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

#import "AUv3WrapperFactory.h"

@implementation AUv3WrapperViewController (AUAudioUnitFactory)

- (AUv3Wrapper *) createAudioUnitWithComponentDescription:(AudioComponentDescription) desc error:(NSError **)error {
	@synchronized (self)
	{
		if (!self.audioUnit)
		{
			if (![NSThread isMainThread])
			{
				dispatch_sync(dispatch_get_main_queue(), [&]{
					self.audioUnit = [[AUv3Wrapper alloc] initWithComponentDescription:desc error:error];
				});
			}
			else
			{
				self.audioUnit = [[AUv3Wrapper alloc] initWithComponentDescription:desc error:error];
			}
		}
	}
	return self.audioUnit;
}

@end
