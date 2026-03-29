/*
 * This file is part of JS80P, a synthesizer plugin.
 * Copyright (C) 2026  Attila M. Magyar
 *
 * JS80P is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * JS80P is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#import <Cocoa/Cocoa.h>

#include "gui/macos.hpp"

#include "synth.hpp"


constexpr JS80P::Integer BLOCK_SIZE = 256;
constexpr JS80P::Frequency SAMPLE_RATE = 11025.0;
constexpr JS80P::Seconds GUI_UPDATE_TIMER_INTERVAL = 0.1;


@interface GUIPlaygroundAppDelegate : NSObject<NSApplicationDelegate>
    {
        JS80P::GUI* gui;
        JS80P::Synth* synth;
        JS80P::Integer rendering_round;
    }

    @property (strong) NSWindow* window;
    @property (strong) NSTimer* idle_timer;
@end


@implementation GUIPlaygroundAppDelegate

    - (id) init
    {
        if (!(self = [super init])) {
            return nil;
        }

        self->gui = NULL;
        self->synth = NULL;

        return self;
    }

    - (void) applicationDidFinishLaunching:(NSNotification*)notification
    {
        NSRect frame = NSMakeRect(0.0, 0.0, 1020.0, 640.0);

        self.window = [
            [NSWindow alloc]
            initWithContentRect:frame
            styleMask:(
                NSWindowStyleMaskTitled
                | NSWindowStyleMaskClosable
                | NSWindowStyleMaskResizable
                | NSWindowStyleMaskMiniaturizable
            )
            backing:NSBackingStoreBuffered
            defer:NO
        ];

        [self.window setTitle:@"JS80P GUI Playground"];
        [self.window center];

        NSView* content_view = [[NSView alloc] initWithFrame:frame];
        self.window.contentView = content_view;

        rendering_round = 0;

        synth = new JS80P::Synth();

        synth->set_block_size(BLOCK_SIZE);
        synth->set_sample_rate(SAMPLE_RATE);

        gui = new JS80P::GUI(
            NULL,
            NULL,
            (__bridge JS80P::GUI::PlatformWidget)content_view,
            *synth,
            true
        );
        gui->show();

        [self.window makeKeyAndOrderFront:nil];

        self.idle_timer = [
            NSTimer
            scheduledTimerWithTimeInterval:GUI_UPDATE_TIMER_INTERVAL
            repeats:YES
            block:^(NSTimer* timer) {
                if (synth != NULL) {
                    ++rendering_round;
                    rendering_round = rendering_round & 0x7fff;
                    synth->generate_samples(rendering_round, BLOCK_SIZE);
                }
            }
        ];
    }

    - (void) applicationWillTerminate:(NSNotification*)notification
    {
        [self.idle_timer invalidate];

        if (gui != NULL) {
            delete gui;

            gui = NULL;
        }

        if (synth != NULL) {
            delete synth;

            synth = NULL;
        }
    }

    - (BOOL) applicationShouldTerminateAfterLastWindowClosed:(NSApplication*)sender
    {
        return YES;
    }

@end


extern "C" void run_gui_playground()
{
    @autoreleasepool {
        NSApplication* app = [NSApplication sharedApplication];
        GUIPlaygroundAppDelegate* delegate = [GUIPlaygroundAppDelegate new];
        [app setDelegate:delegate];
        [app setActivationPolicy:NSApplicationActivationPolicyRegular];
        [app run];
    }
}
