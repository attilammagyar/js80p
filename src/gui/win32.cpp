/*
 * This file is part of JS80P, a synthesizer plugin.
 * Copyright (C) 2023, 2024, 2025  Attila M. Magyar
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

#ifndef JS80P__GUI__WIN32_CPP
#define JS80P__GUI__WIN32_CPP

#include <cstdio>

#include <algorithm>
#include <cmath>

#include "gui/win32.hpp"

#include "gui/gui.cpp"


namespace JS80P
{

void GUI::idle()
{
}


void GUI::initialize()
{
}


void GUI::destroy()
{
}


std::string const Widget::FILTER_STR(
    "JS80P Patches (*.js80p)\x00*.js80p\x00"
    "All Files (*.*)\x00*.*\x00",
    53
);


UINT_PTR Widget::next_timer_id = 0x4a533830;


Widget::Text::Text() : wtext(NULL), ctext(NULL), capacity(0)
{
    set("");
}


Widget::Text::Text(std::string const& text) : wtext(NULL), ctext(NULL), capacity(0)
{
    set(text);
}


Widget::Text::~Text()
{
    free_buffers();
}


void Widget::Text::free_buffers()
{
    if (ctext != NULL) {
        delete[] ctext;
        ctext = NULL;
    }

    if (wtext != NULL) {
        delete[] wtext;
        wtext = NULL;
    }

    capacity = 0;
}


void Widget::Text::set(std::string const& text)
{
    size_t const length = (size_t)text.length();

    /*
    Align to 16 byte chunks so that we don't allocate too much bigger buffers
    than we need, but also avoid reallocation for slightly longer strings.

    Also make sure that we leave room for the terminating zero, e.g. if the
    new text is 16 bytes long, then we will allocate 32 bytes.

    Maybe we should just go with std::basic_string<WHCAR> instead?
    */
    size_t const required_capacity = std::max(
        (length + 16) & ~size_t(15),
        size_t(16)
    );

    this->text = text;

    if (required_capacity > capacity) {
        free_buffers();

        ctext = new char[required_capacity];

#ifdef UNICODE
        wtext = new WCHAR[required_capacity];
#endif

        capacity = required_capacity;
    }


    if (length == 0) {
        ctext[0] = 0;

#ifdef UNICODE
        wtext[0] = 0;
#endif
    } else {
        strncpy(ctext, text.c_str(), capacity);
        ctext[length] = 0;

#ifdef UNICODE
        MultiByteToWideChar(
            CP_UTF8,
            0,
            (LPCCH)text.c_str(),
            length,
            (LPWSTR)wtext,
            capacity
        );

        wtext[length] = 0;
#endif
    }
}


char* Widget::Text::c_str() const
{
    return ctext;
}


WCHAR* Widget::Text::c_wstr() const
{
    return wtext;
}


LPCTSTR Widget::Text::get_const() const
{
#ifdef UNICODE
    return (LPCTSTR)c_wstr();
#else
    return (LPCTSTR)c_str();
#endif
}


LPTSTR Widget::Text::get() const
{
#ifdef UNICODE
    return (LPTSTR)c_wstr();
#else
    return (LPTSTR)c_str();
#endif
}


COLORREF Widget::to_colorref(GUI::Color const color)
{
    return RGB(GUI::red(color), GUI::green(color), GUI::blue(color));
}


void Widget::set_text(char const* const text)
{
    text_text.set(text);
    WidgetBase::set_text(text);
}


GUI::Image Widget::load_image(
        GUI::PlatformData platform_data,
        char const* const name
) {
    Text name_text(name);

    // TODO: GetLastError()
    GUI::Image image = (GUI::Image)LoadImage(
        (HINSTANCE)platform_data,               /* hInst    */
        name_text.get_const(),                  /* name     */
        IMAGE_BITMAP,                           /* type     */
        0,                                      /* cx       */
        0,                                      /* cy       */
        LR_CREATEDIBSECTION                     /* fuLoad   */
    );

    return image;
}


void Widget::delete_image(GUI::Image image)
{
    if (image != NULL) {
        DeleteObject((HGDIOBJ)image);
    }
}


LRESULT Widget::process_message(
        HWND hwnd,
        UINT uMsg,
        WPARAM wParam,
        LPARAM lParam
) {
    Widget* const widget = (
        (Widget*)GetWindowLongPtr(hwnd, GWLP_USERDATA)
    );

    bool is_handled = false;

    switch (uMsg) {
        case WM_TIMER:
            if (widget->type == Type::BACKGROUND) {
                ((Background*)widget)->refresh();
                is_handled = true;
            }

            break;

        case WM_PAINT: {
            PAINTSTRUCT ps;
            widget->hdc = BeginPaint((HWND)widget->get_platform_widget(), &ps);

            is_handled = widget->paint();

            EndPaint((HWND)widget->get_platform_widget(), &ps);

            break;
        }

        case WM_LBUTTONDBLCLK:
            widget->release_captured_mouse();
            is_handled = widget->double_click();

            break;

        case WM_LBUTTONDOWN:
            widget->is_clicking = true;
            is_handled = widget->mouse_down(
                GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)
            );

            if (is_handled) {
                widget->capture_mouse();
            }

            break;

        case WM_LBUTTONUP:
            widget->release_captured_mouse();

            is_handled = widget->mouse_up(
                GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)
            );

            if (widget->is_clicking) {
                widget->is_clicking = false;
                widget->click();
            }

            break;

        case WM_MOUSEMOVE: {
            TRACKMOUSEEVENT track_mouse_event;
            track_mouse_event.cbSize = sizeof(TRACKMOUSEEVENT);
            track_mouse_event.dwFlags = TME_LEAVE;
            track_mouse_event.hwndTrack = (HWND)widget->get_platform_widget();
            track_mouse_event.dwHoverTime = 0;
            TrackMouseEvent(&track_mouse_event);

            is_handled = widget->mouse_move(
                GET_X_LPARAM(lParam),
                GET_Y_LPARAM(lParam),
                0 != (wParam & MK_CONTROL)
            );

            break;
        }

        case WM_MOUSELEAVE:
            is_handled = widget->mouse_leave(
                GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)
            );

            break;

        case WM_MOUSEWHEEL: {
            Number const delta = (
                (Number)GET_WHEEL_DELTA_WPARAM(wParam) * MOUSE_WHEEL_SCALE
            );

            is_handled = widget->mouse_wheel(delta, 0 != (wParam & MK_CONTROL));

            break;
        }

        case WM_DESTROY: {
            WNDPROC original_window_procedure = widget->original_window_procedure;

            if (original_window_procedure != NULL) {
                SetWindowLongPtr(
                    hwnd, GWLP_WNDPROC, (LONG_PTR)original_window_procedure
                );
            }

            return FALSE;
        }

        default:
            break;
    }

    if (!is_handled) {
        return widget->call_original_window_procedure(uMsg, wParam, lParam);
    }

    return 0;
}


Widget::Widget(char const* const text)
    : WidgetBase(text),
    hdc(NULL),
    dwStyle(0),
    original_window_procedure(NULL),
    is_mouse_captured(false)
{
}


Widget::Widget(
        GUI::PlatformData platform_data,
        GUI::PlatformWidget platform_widget,
        Type const type
) : WidgetBase(platform_data, platform_widget, type),
    hdc(NULL),
    dwStyle(0),
    original_window_procedure(NULL),
    is_mouse_captured(false)
{
}


Widget::Widget(
        char const* const text,
        int const left,
        int const top,
        int const width,
        int const height,
        Type const type
) : WidgetBase(text, left, top, width, height, type),
    hdc(NULL),
    class_name("STATIC"),
    text_text(text),
    dwStyle(0),
    original_window_procedure(NULL),
    timer_id(0),
    is_mouse_captured(false),
    is_timer_started(false)
{
    switch (type) {
        case Type::BACKGROUND:
            dwStyle = WS_CHILD | WS_VISIBLE | SS_BITMAP;
            break;

        case Type::KNOB:
            dwStyle = WS_CHILD | WS_VISIBLE | SS_BITMAP | SS_NOTIFY;
            break;

        default:
            dwStyle = WS_CHILD | WS_VISIBLE | SS_NOTIFY;
            break;
    }
}


Widget::~Widget()
{
    release_captured_mouse();
    destroy_children();

    if (type == Type::BACKGROUND) {
        KillTimer((HWND)platform_widget, timer_id);
    }

    if (platform_widget != NULL) {
        DestroyWindow((HWND)platform_widget);
        platform_widget = NULL;
    }
}


void Widget::set_up(GUI::PlatformData platform_data, WidgetBase* const parent)
{
    WidgetBase::set_up(platform_data, parent);

    HWND parent_hwnd = (HWND)parent->get_platform_widget();

    // TODO: GetLastError()
    HWND widget_hwnd = CreateWindow(
        (LPCTSTR)class_name.get_const(),        /* lpClassName  */
        (LPCTSTR)text_text.get_const(),         /* lpWindowName */
        dwStyle,                                /* dwStyle      */
        left,                                   /* x            */
        top,                                    /* y            */
        width,                                  /* nWidth       */
        height,                                 /* nHeight      */
        parent_hwnd,                            /* hWndParent   */
        NULL,                                   /* hMenu        */
        (HINSTANCE)platform_data,               /* hInstance    */
        NULL                                    /* lpParam      */
    );

    if (!widget_hwnd) {
        return;
    }

    SetWindowLongPtr(widget_hwnd, GWLP_USERDATA, (LONG_PTR)this);

    original_window_procedure = (
        (WNDPROC)SetWindowLongPtr(
            widget_hwnd, GWLP_WNDPROC, (LONG_PTR)(&process_message)
        )
    );

    platform_widget = (GUI::PlatformWidget)widget_hwnd;

    if (type == Type::BACKGROUND) {
        UINT const elapse = (UINT)std::ceil(1000.0 * GUI::REFRESH_RATE_SECONDS);
        timer_id = ++next_timer_id;
        is_timer_started = true;

        // TODO: GetLastError
        SetTimer(widget_hwnd, timer_id, elapse, NULL);
    }
}


void Widget::fill_rectangle(
        int const left,
        int const top,
        int const width,
        int const height,
        GUI::Color const color
) {
    RECT rect;
    rect.left = left;
    rect.top = top;
    rect.right = left + width;
    rect.bottom = top + height;
    HBRUSH brush = CreateSolidBrush(to_colorref(color));
    FillRect(hdc, (LPRECT)&rect, brush);
    DeleteObject((HGDIOBJ)brush);
}


void Widget::draw_text(
        char const* const text,
        int const font_size_px,
        int const left,
        int const top,
        int const width,
        int const height,
        GUI::Color const color,
        GUI::Color const background,
        FontWeight const font_weight,
        int const padding,
        TextAlignment const alignment
) {
    int const weight = font_weight == FontWeight::NORMAL ? FW_NORMAL : FW_BOLD;
    int const font_height = -MulDiv(font_size_px, GetDeviceCaps(hdc, LOGPIXELSY), 72);

    Text text_obj(text);

    HFONT font = CreateFont(
        font_height,                    /* cHeight          */
        0,                              /* cWidth           */
        0,                              /* cEscapement      */
        0,                              /* cOrientation     */
        weight,                         /* cWeight          */
        FALSE,                          /* bItalic          */
        FALSE,                          /* bUnderline       */
        FALSE,                          /* bStrikeOut       */
        ANSI_CHARSET,                   /* iCharSet         */
        OUT_DEFAULT_PRECIS,             /* iOutPrecision    */
        CLIP_DEFAULT_PRECIS,            /* iClipPrecision   */
        ANTIALIASED_QUALITY,            /* iQuality         */
        DEFAULT_PITCH | FF_DONTCARE,    /* iPitchAndFamily  */
        TEXT("Arial")                   /* pszFaceName      */
    );

    int orig_bk_mode = SetBkMode(hdc, OPAQUE);
    int orig_map_mode = SetMapMode(hdc, MM_TEXT);
    COLORREF orig_bk_color = SetBkColor(hdc, to_colorref(background));
    COLORREF orig_text_color = SetTextColor(hdc, to_colorref(color));
    HGDIOBJ orig_font = (HFONT)SelectObject(hdc, (HGDIOBJ)font);

    RECT text_rect;
    text_rect.left = left;
    text_rect.top = top;
    text_rect.right = left + width;
    text_rect.bottom = top + height;
    HBRUSH brush = CreateSolidBrush(to_colorref(background));
    FillRect(hdc, (LPRECT)&text_rect, brush);

    text_rect.left += padding;
    text_rect.right -= padding;

    UINT format;

    switch (alignment) {
        case TextAlignment::CENTER:
            format = DT_SINGLELINE | DT_CENTER | DT_VCENTER | DT_NOPREFIX;
            break;

        case TextAlignment::RIGHT:
            format = DT_SINGLELINE | DT_RIGHT | DT_VCENTER | DT_NOPREFIX;
            break;

        default:
            format = DT_SINGLELINE | DT_LEFT | DT_VCENTER | DT_NOPREFIX;
            break;
    }

    DrawText(hdc, text_obj.get_const(), -1, (LPRECT)&text_rect, format);

    SelectObject(hdc, orig_font);
    SetTextColor(hdc, orig_text_color);
    SetBkColor(hdc, orig_bk_color);
    SetMapMode(hdc, orig_map_mode);
    SetBkMode(hdc, orig_bk_mode);

    DeleteObject((HGDIOBJ)brush);
    DeleteObject((HGDIOBJ)font);
}


void Widget::draw_image(
        GUI::Image image,
        int const left,
        int const top,
        int const width,
        int const height
) {
    HDC image_hdc = CreateCompatibleDC(hdc);
    SelectObject(image_hdc, (HBITMAP)image);
    BitBlt(hdc, left, top, width, height, image_hdc, 0, 0, SRCCOPY);
    DeleteDC(image_hdc);
}


GUI::Image Widget::copy_image_region(
        GUI::Image source,
        int const left,
        int const top,
        int const width,
        int const height
) {
    HDC hdc = GetDC((HWND)platform_widget);

    HDC source_hdc = CreateCompatibleDC(hdc);
    HDC destination_hdc = CreateCompatibleDC(hdc);

    GUI::Image destination_image = CreateCompatibleBitmap(hdc, width, height);

    GUI::Image old_source_image = (
        (GUI::Image)SelectObject(source_hdc, (HGDIOBJ)source)
    );
    GUI::Image old_destination_image = (
        (GUI::Image)SelectObject(destination_hdc, (HGDIOBJ)destination_image)
    );

    BitBlt(destination_hdc, 0, 0, width, height, source_hdc, left, top, SRCCOPY);

    SelectObject(source_hdc, (HGDIOBJ)old_source_image);
    SelectObject(destination_hdc, (HGDIOBJ)old_destination_image);

    DeleteDC(source_hdc);
    DeleteDC(destination_hdc);

    ReleaseDC((HWND)platform_widget, hdc);

    return destination_image;
}


void Widget::show()
{
    // TODO: GetLastError()
    ShowWindow((HWND)platform_widget, SW_SHOWNORMAL);
}


void Widget::hide()
{
    // TODO: GetLastError()
    ShowWindow((HWND)platform_widget, SW_HIDE);
}


void Widget::focus()
{
    SetFocus((HWND)platform_widget);
}


void Widget::bring_to_top()
{
    BringWindowToTop((HWND)platform_widget);
}


void Widget::redraw()
{
    // TODO: GetLastError()
    RedrawWindow((HWND)platform_widget, NULL, NULL, RDW_INVALIDATE);
}


LRESULT Widget::call_original_window_procedure(
        UINT uMsg,
        WPARAM wParam,
        LPARAM lParam
) {
    if (original_window_procedure != NULL) {
        // TODO: GetLastError()
        return CallWindowProc(
            original_window_procedure, (HWND)platform_widget, uMsg, wParam, lParam
        );
    } else {
        return DefWindowProcW((HWND)platform_widget, uMsg, wParam, lParam);
    }
}


void Widget::capture_mouse()
{
    is_mouse_captured = true;
    SetCapture((HWND)platform_widget);
}


void Widget::release_captured_mouse()
{
    if (is_mouse_captured) {
        is_mouse_captured = false;
        ReleaseCapture();
    }
}


void ImportPatchButton::click()
{
    Widget::Text filter(FILTER_STR);
    Widget::Text ext("js80p");
    TCHAR file_name[MAX_PATH];
    OPENFILENAME ofn;

    file_name[0] = 0;

    ZeroMemory(&ofn, sizeof(ofn));

    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = (HWND)platform_widget;
    ofn.lpstrFilter = filter.get_const();
    ofn.lpstrFile = file_name;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
    ofn.lpstrDefExt = ext.get_const();

    if (!GetOpenFileName(&ofn)) {
        return;
    }

    HANDLE file;
    file = CreateFile(
        file_name,              /* lpFileName               */
        GENERIC_READ,           /* dwDesiredAccess          */
        FILE_SHARE_READ,        /* dwShareMode              */
        NULL,                   /* lpSecurityAttributes     */
        OPEN_EXISTING,          /* dwCreationDisposition    */
        FILE_ATTRIBUTE_NORMAL,  /* dwFlagsAndAttributes     */
        NULL                    /* hTemplateFile            */
    );

    // TODO: GetLastError
    if (file == INVALID_HANDLE_VALUE) {
        return;
    }

    DWORD size = GetFileSize(file, NULL);

    if (size == INVALID_FILE_SIZE) {
        // TODO: GetLastError
        CloseHandle(file);

        return;
    }

    DWORD read;
    char* const buffer = new char[Serializer::MAX_SIZE];

    std::fill_n(buffer, Serializer::MAX_SIZE, '\x00');

    if (
        !ReadFile(
            file,
            (LPVOID)buffer,
            std::min(size, (DWORD)Serializer::MAX_SIZE),
            &read,
            NULL
        )
    ) {
        // TODO: GetLastError
        CloseHandle(file);

        delete[] buffer;

        return;
    }

    CloseHandle(file);

    import_patch(buffer, (Integer)read);

    delete[] buffer;
}


void ExportPatchButton::click()
{
    Widget::Text filter(ImportPatchButton::FILTER_STR);
    Widget::Text ext("js80p");
    TCHAR file_name[MAX_PATH];
    OPENFILENAME ofn;

    file_name[0] = 0;

    ZeroMemory(&ofn, sizeof(ofn));

    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = (HWND)platform_widget;
    ofn.lpstrFilter = filter.get_const();
    ofn.lpstrFile = file_name;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = (
        OFN_EXPLORER
        | OFN_HIDEREADONLY
        | OFN_NOREADONLYRETURN
        | OFN_OVERWRITEPROMPT
        | OFN_PATHMUSTEXIST
    );
    ofn.lpstrDefExt = ext.get_const();

    if (!GetSaveFileName(&ofn)) {
        return;
    }

    HANDLE file;
    file = CreateFile(
        file_name,              /* lpFileName               */
        GENERIC_WRITE,          /* dwDesiredAccess          */
        0,                      /* dwShareMode              */
        NULL,                   /* lpSecurityAttributes     */
        CREATE_ALWAYS,          /* dwCreationDisposition    */
        FILE_ATTRIBUTE_NORMAL,  /* dwFlagsAndAttributes     */
        NULL                    /* hTemplateFile            */
    );

    // TODO: GetLastError
    if (file == INVALID_HANDLE_VALUE) {
        return;
    }

    std::string const& patch = Serializer::serialize(synth);
    DWORD written;

    if (
        !WriteFile(
            file, (LPCVOID)patch.c_str(), (DWORD)patch.length(), &written, NULL
        )
    ) {
        // TODO: GetLastError
    }

    CloseHandle(file);
}

}

#include "gui/widgets.cpp"

#endif
