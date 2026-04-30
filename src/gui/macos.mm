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
#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>

#include <algorithm>
#include <cstring>

#include <mach/mach_time.h>

#include "gui/macos.hpp"


@interface CocoaWidget : NSView
    @property (assign) JS80P::Widget* cpp_widget;
    @property (strong) NSTimer* gui_idle_timer;
@end


@implementation CocoaWidget
    - (id) initWithFrame:(NSRect)frameRect
    {
        if (!(self = [super initWithFrame:frameRect])) {
            return nil;
        }

        self.cpp_widget = NULL;
        self.gui_idle_timer = nil;

        return self;
    }

    - (BOOL) isFlipped
    {
        return YES;
    }

    - (void) drawRect:(NSRect)dirtyRect
    {
        if (!self.cpp_widget) {
            return;
        }

        JS80P::Widget::notify_paint(self.cpp_widget);
    }

    - (void) updateTrackingAreas
    {
        [super updateTrackingAreas];

        for (NSTrackingArea* old_tracking_area in self.trackingAreas) {
            [self removeTrackingArea:old_tracking_area];
        }

        NSTrackingArea* new_tracking_area = [
            [NSTrackingArea alloc]
            initWithRect:self.bounds
            options:NSTrackingMouseMoved
                | NSTrackingMouseEnteredAndExited
                | NSTrackingActiveInKeyWindow
                | NSTrackingInVisibleRect
            owner:self
            userInfo:nil
        ];

        [self addTrackingArea:new_tracking_area];
    }

    - (void) mouseDown:(NSEvent*)event
    {
        if (!self.cpp_widget) {
            return;
        }

        if (event.clickCount >= 2) {
            JS80P::Widget::notify_double_click(self.cpp_widget);
        } else {
            NSPoint p = [
                self convertPoint:[event locationInWindow] fromView:nil
            ];

            JS80P::Widget::notify_mouse_down(
                self.cpp_widget, (int)p.x, (int)p.y
            );
        }
    }

    - (void) mouseUp:(NSEvent*)event
    {
        if (!self.cpp_widget) {
            return;
        }

        NSPoint p = [self convertPoint:[event locationInWindow] fromView:nil];

        JS80P::Widget::notify_mouse_up(self.cpp_widget, (int)p.x, (int)p.y);
    }

    - (void) mouseExited:(NSEvent*)event
    {
        if (!self.cpp_widget) {
            return;
        }

        NSPoint p = [self convertPoint:[event locationInWindow] fromView:nil];

        JS80P::Widget::notify_mouse_leave(self.cpp_widget, (int)p.x, (int)p.y);
    }

    - (void) mouseDragged:(NSEvent*)event
    {
        [self handle_mouse_move:event:YES];
    }

    - (void) mouseMoved:(NSEvent*)event
    {
        [self handle_mouse_move:event:NO];
    }

    - (void) scrollWheel:(NSEvent*)event
    {
        JS80P::Number const delta = std::clamp(
            [event deltaY] * ([event hasPreciseScrollingDeltas] ? 1.0 : 0.1),
            -1.0,
            1.0
        );

        if (std::fabs(delta) > 0.000001) {
            JS80P::Widget::notify_mouse_wheel(
                self.cpp_widget,
                delta,
                ([event modifierFlags] & NSEventModifierFlagControl) != 0
            );
        }
    }

    - (void) set_up_gui_idle_timer
    {
        self.gui_idle_timer = [
            NSTimer
            scheduledTimerWithTimeInterval:JS80P::GUI::REFRESH_RATE_SECONDS
            repeats:YES
            block:^(NSTimer* timer) {
                if (self.cpp_widget) {
                    JS80P::Widget::notify_timer_tick(self.cpp_widget);
                }
            }
        ];
    }

    - (void) destroy_gui_idle_timer
    {
        if (!self.gui_idle_timer) {
            return;
        }

        [self.gui_idle_timer invalidate];

        self.gui_idle_timer = nil;
    }

    - (void) handle_mouse_move:(NSEvent*)event :(BOOL)is_drag
    {
        if (!self.cpp_widget) {
            return;
        }

        NSPoint p = [self convertPoint:[event locationInWindow] fromView:nil];

        if (!(is_drag || NSPointInRect(p, self.bounds))) {
            /*
            In some REAPER versions, various widgets receive mouseMoved events
            even when the cursor is outside their area and no mouse buttons are
            pressed.
            */
            return;
        }

        JS80P::Widget::notify_mouse_move(
            self.cpp_widget,
            (int)p.x,
            (int)p.y,
            ([event modifierFlags] & NSEventModifierFlagControl) != 0
        );
    }
@end


JS80P::GUI::PlatformWidget js80p_create_platform_widget(
        JS80P::Widget* cpp_widget,
        int const left,
        int const top,
        int const width,
        int const height,
        JS80P::GUI::PlatformWidget parent,
        JS80P::WidgetBase::Type const type
) {
    NSRect frame = NSMakeRect(
        (CGFloat)left, (CGFloat)top, (CGFloat)width, (CGFloat)height
    );

    CocoaWidget* cocoa_widget = [[CocoaWidget alloc] initWithFrame:frame];
    cocoa_widget.cpp_widget = cpp_widget;

    if (parent != NULL) {
        NSView* parent_view = (__bridge NSView*)parent;
        [parent_view addSubview:cocoa_widget];
    }

    if (type == JS80P::WidgetBase::Type::BACKGROUND) {
        [cocoa_widget set_up_gui_idle_timer];
    }

    return (__bridge_retained JS80P::GUI::PlatformWidget)cocoa_widget;
}


void js80p_destroy_platform_widget(JS80P::GUI::PlatformWidget platform_widget)
{
    CocoaWidget* cocoa_widget = (__bridge_transfer CocoaWidget*)platform_widget;

    if (!cocoa_widget) {
        return;
    }

    [cocoa_widget destroy_gui_idle_timer];

    if ([cocoa_widget superview]) {
        [cocoa_widget removeFromSuperview];
    }
}


void js80p_widget_resize(
        JS80P::GUI::PlatformWidget platform_widget,
        int const left,
        int const top,
        int const width,
        int const height
) {
    CocoaWidget* cocoa_widget = (__bridge CocoaWidget*)platform_widget;

    if (!cocoa_widget) {
        return;
    }

    NSPoint new_origin = NSMakePoint((CGFloat)left, (CGFloat)top);
    NSSize new_size = NSMakeSize((CGFloat)width, (CGFloat)height);

    [cocoa_widget setFrameOrigin:new_origin];
    [cocoa_widget setFrameSize:new_size];
}


void js80p_widget_show(JS80P::GUI::PlatformWidget platform_widget)
{
    CocoaWidget* cocoa_widget = (__bridge CocoaWidget*)platform_widget;

    if (!cocoa_widget) {
        return;
    }

    [cocoa_widget setHidden:NO];
}


void js80p_widget_hide(JS80P::GUI::PlatformWidget platform_widget)
{
    CocoaWidget* cocoa_widget = (__bridge CocoaWidget*)platform_widget;

    if (!cocoa_widget) {
        return;
    }

    [cocoa_widget setHidden:YES];
}


void js80p_widget_focus(JS80P::GUI::PlatformWidget platform_widget)
{
    CocoaWidget* cocoa_widget = (__bridge CocoaWidget*)platform_widget;

    if (!cocoa_widget) {
        return;
    }

    [[cocoa_widget window] makeFirstResponder:cocoa_widget];
}


void js80p_widget_bring_to_top(JS80P::GUI::PlatformWidget platform_widget)
{
    CocoaWidget* cocoa_widget = (__bridge CocoaWidget*)platform_widget;

    if (!cocoa_widget) {
        return;
    }

    NSView* superview = [cocoa_widget superview];

    [cocoa_widget removeFromSuperview];
    [superview addSubview:cocoa_widget];
}


void js80p_widget_redraw(JS80P::GUI::PlatformWidget platform_widget)
{
    CocoaWidget* cocoa_widget = (__bridge CocoaWidget*)platform_widget;

    if (!cocoa_widget) {
        return;
    }

    [cocoa_widget setNeedsDisplay:YES];
}


void js80p_widget_fill_rectangle(
        int const left,
        int const top,
        int const width,
        int const height,
        JS80P::GUI::Color const color
) {
    CGContextRef ctx = [[NSGraphicsContext currentContext] CGContext];

    CGFloat red = (CGFloat)JS80P::GUI::red(color) / 255.0;
    CGFloat green = (CGFloat)JS80P::GUI::green(color) / 255.0;
    CGFloat blue = (CGFloat)JS80P::GUI::blue(color) / 255.0;

    CGContextSetRGBFillColor(ctx, red, green, blue, 1.0);
    CGContextFillRect(
        ctx,
        CGRectMake((CGFloat)left, (CGFloat)top, (CGFloat)width, (CGFloat)height)
    );
}


void js80p_widget_draw_text(
        JS80P::GUI::PlatformWidget platform_widget,
        char const* const text,
        int const font_size_px,
        int const left,
        int const top,
        int const width,
        int const height,
        JS80P::GUI::Color const color,
        JS80P::WidgetBase::FontWeight const font_weight,
        JS80P::WidgetBase::TextAlignment const alignment
) {
    CocoaWidget* cocoa_widget = (__bridge CocoaWidget*)platform_widget;

    if (!cocoa_widget) {
        return;
    }

    NSString* ns_text = [NSString stringWithUTF8String:text];
    NSColor* ns_color = [
        NSColor
        colorWithDeviceRed:(CGFloat)JS80P::GUI::red(color) / 255.0
        green:(CGFloat)JS80P::GUI::green(color) / 255.0
        blue:(CGFloat)JS80P::GUI::blue(color) / 255.0
        alpha:1.0
    ];

    CGFloat font_size_scale = [[cocoa_widget window] backingScaleFactor];

    if (std::fabs(font_size_scale) < 0.000001) {
        font_size_scale = [[NSScreen mainScreen] backingScaleFactor];

        if (std::fabs(font_size_scale) < 0.000001) {
            font_size_scale = 1.0;
        }
    }

    CGFloat float_font_size_px = (CGFloat)font_size_px * font_size_scale * 0.63;

    NSFont* ns_font = (
        font_weight == JS80P::WidgetBase::FontWeight::BOLD
            ? [NSFont fontWithName:@"Helvetica-Bold" size:float_font_size_px]
            : [NSFont fontWithName:@"Helvetica" size:float_font_size_px]
    );

    if (!ns_font) {
        ns_font = (
            font_weight == JS80P::WidgetBase::FontWeight::BOLD
                ? [NSFont boldSystemFontOfSize:float_font_size_px]
                : [NSFont systemFontOfSize:float_font_size_px]
        );
    }

    NSMutableParagraphStyle* style = [
        [NSParagraphStyle defaultParagraphStyle]
        mutableCopy
    ];

    switch (alignment) {
        case JS80P::WidgetBase::TextAlignment::CENTER:
            style.alignment = NSTextAlignmentCenter;
            break;

        case JS80P::WidgetBase::TextAlignment::RIGHT:
            style.alignment = NSTextAlignmentRight;
            break;

        default:
            style.alignment = NSTextAlignmentLeft;
            break;
    }

    NSDictionary* attrs = @{
        NSFontAttributeName: ns_font,
        NSForegroundColorAttributeName: ns_color,
        NSParagraphStyleAttributeName: style,
    };

    NSSize text_size = [ns_text sizeWithAttributes:attrs];
    CGFloat float_height = (CGFloat)height;
    CGFloat float_top = (CGFloat)top + (float_height - text_size.height) / 2.0;

    [
        ns_text
        drawInRect:NSMakeRect(
            (CGFloat)left, float_top, (CGFloat)width, float_height
        )
        withAttributes:attrs
    ];
}


JS80P::GUI::Image js80p_widget_load_image(char const* const name)
{
    NSString* ns_name = [NSString stringWithUTF8String:name];
    NSString* path = [
        [NSBundle bundleForClass:[CocoaWidget class]]
        pathForResource:ns_name
        ofType:@"png"
        inDirectory:@"img"
    ];

    if (!path) {
        return NULL;
    }

    NSImage* ns_image = [[NSImage alloc] initWithContentsOfFile:path];
    CGImageRef cg_image = [
        ns_image CGImageForProposedRect:nil context:nil hints:nil
    ];
    CGImageRetain(cg_image);

    return (JS80P::GUI::Image)cg_image;
}


JS80P::GUI::Image js80p_widget_copy_image_region(
        JS80P::GUI::Image source,
        int const left,
        int const top,
        int const width,
        int const height
) {
    if (source == NULL) {
        return NULL;
    }

    return (JS80P::GUI::Image)CGImageCreateWithImageInRect(
        (CGImageRef)source,
        CGRectMake((CGFloat)left, (CGFloat)top, (CGFloat)width, (CGFloat)height)
    );
}


JS80P::GUI::Image js80p_widget_downscale_image(
        JS80P::GUI::Image source,
        int const old_width,
        int const old_height,
        int const new_width,
        int const new_height
) {
    if (source == NULL) {
        return NULL;
    }

    CGColorSpaceRef color_space = CGColorSpaceCreateDeviceRGB();
    CGContextRef ctx = CGBitmapContextCreate(
        NULL,                       /* data */
        (size_t)new_width,          /* width */
        (size_t)new_height,         /* height */
        8,                          /* bitsPerComponent */
        (size_t)new_width * 4,      /* bytesPerRow */
        color_space,                /* space */
        kCGImageAlphaNoneSkipLast   /* bitmapInfo */
    );
    CGContextSetInterpolationQuality(ctx, kCGInterpolationLow);
    CGContextDrawImage(
        ctx,
        CGRectMake(0.0, 0.0, (CGFloat)new_width, (CGFloat)new_height),
        (CGImageRef)source
    );
    CGImageRef scaled_img = CGBitmapContextCreateImage(ctx);
    CGContextRelease(ctx);
    CGColorSpaceRelease(color_space);

    return (JS80P::GUI::Image)scaled_img;
}


void js80p_widget_delete_image(JS80P::GUI::Image image)
{
    if (image == NULL) {
        return;
    }

    CGImageRelease((CGImageRef)image);
}


void js80p_widget_draw_image(
        JS80P::GUI::Image image,
        int const left,
        int const top,
        int const width,
        int const height
) {
    CGContextRef ctx = [[NSGraphicsContext currentContext] CGContext];

    CGContextSaveGState(ctx);
    CGContextTranslateCTM(ctx, (CGFloat)left, (CGFloat)(top + height));
    CGContextScaleCTM(ctx, 1.0, -1.0);
    CGContextDrawImage(
        ctx,
        CGRectMake(0.0, 0.0, (CGFloat)width, (CGFloat)height),
        (CGImageRef)image
    );
    CGContextRestoreGState(ctx);
}


bool js80p_import_patch(
        char* const buffer,
        size_t* length,
        size_t const max_length
) {
    NSOpenPanel* panel = [NSOpenPanel openPanel];

    if (@available(macOS 11.0, *)) {
        [
            panel
            setAllowedContentTypes:@[
                [UTType typeWithFilenameExtension:@"js80p"]
            ]
        ];
    } else {
        [panel setAllowedFileTypes:@[@"js80p"]];
    }

    panel.allowsOtherFileTypes = YES;
    panel.canChooseFiles = YES;
    panel.canChooseDirectories = NO;
    panel.allowsMultipleSelection = NO;

    if ([panel runModal] != NSModalResponseOK) {
        snprintf(buffer, max_length, "");
        *length = 0;

        return false;
    }

    NSURL* url = [panel URL];
    NSError* error = nil;
    NSData* data = [
        NSData
        dataWithContentsOfURL:url
        options:NSDataReadingMappedIfSafe
        error:&error
    ];

    if (error || !data) {
        snprintf(buffer, max_length, "");
        *length = 0;

        return false;
    }

    char const* const raw_data = (char const*)[data bytes];
    size_t const data_length = (size_t)[data length];

    if (raw_data == NULL) {
        snprintf(buffer, max_length, "");
        *length = 0;

        return false;
    }

    *length = std::min(max_length, data_length);

    memcpy(buffer, raw_data, *length);

    return true;
}


void js80p_export_patch(char const* const buffer, size_t const length)
{
    NSSavePanel* panel = [NSSavePanel savePanel];

    if (@available(macOS 11.0, *)) {
        [
            panel
            setAllowedContentTypes:@[
                [UTType typeWithFilenameExtension:@"js80p"]
            ]
        ];
    } else {
        [panel setAllowedFileTypes:@[@"js80p"]];
    }

    panel.allowsOtherFileTypes = YES;
    panel.canCreateDirectories = YES;
    panel.nameFieldStringValue = @"preset.js80p";

    if ([panel runModal] != NSModalResponseOK) {
        return;
    }

    NSData* data = [
        NSData
        dataWithBytes:(void const*)buffer
        length:(NSUInteger)length
    ];
    NSError* error = nil;

    [data writeToURL:[panel URL] options:NSDataWritingAtomic error:&error];
}
