/*
 * This file is part of JS80P, a synthesizer plugin.
 * Copyright (C) 2023  Attila M. Magyar
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

#include <algorithm>
#include <cmath>

#include "gui/win32.hpp"

#include "gui/gui.cpp"


namespace JS80P
{

Widget::Text::Text() : wtext(NULL)
{
    set("");
}


Widget::Text::Text(std::string const text) : wtext(NULL)
{
    set(text);
}


Widget::Text::~Text()
{
    free_wtext();
}


void Widget::Text::free_wtext()
{
    if (wtext == NULL) {
        return;
    }

    delete[] wtext;
    wtext = NULL;
}


void Widget::Text::set(std::string const text)
{
    this->text = text;

#ifdef UNICODE
    int const length = (int)text.length();
    int const size = length + 1;

    free_wtext();
    wtext = new WCHAR[size];

    if (length == 0) {
        wtext[0] = 0;
    } else {
        MultiByteToWideChar(
            CP_UTF8,
            0,
            (LPCCH)text.c_str(),
            length,
            (LPWSTR)wtext,
            size
        );

        wtext[std::min(size - 1, length)] = 0;
    }
#endif
}


char const* Widget::Text::c_str() const
{
    return text.c_str();
}


WCHAR const* Widget::Text::c_wstr() const
{
    return wtext;
}


LPCTSTR Widget::Text::get() const
{
#ifdef UNICODE
    return (LPCTSTR)c_wstr();
#else
    return (LPCTSTR)c_str();
#endif
}


GUI::Bitmap Widget::load_bitmap(GUI::PlatformData platform_data, char const* name)
{
    Text name_text(name);

    // TODO: GetLastError()
    GUI::Bitmap bitmap = (GUI::Bitmap)LoadImage(
        (HINSTANCE)platform_data, name_text.get(), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION
    );

    return bitmap;
}


void Widget::delete_bitmap(GUI::Bitmap bitmap)
{
    if (bitmap != NULL) {
        DeleteObject((HGDIOBJ)bitmap);
    }
}


Widget::Color Widget::rgb(
        unsigned char const red,
        unsigned char const green,
        unsigned char const blue
) {
    return RGB(red, green, blue);
}


LRESULT Widget::process_message(
        HWND hwnd,
        UINT uMsg,
        WPARAM wParam,
        LPARAM lParam
) {
    Widget* widget = (
        (Widget*)GetWindowLongPtr(hwnd, GWLP_USERDATA)
    );

    bool is_handled = false;

    switch (uMsg) {
        case WM_TIMER:
            is_handled = widget->timer();

            break;

        case WM_PAINT: {
            PAINTSTRUCT ps;
            widget->hdc = BeginPaint((HWND)widget->window, &ps);

            is_handled = widget->paint();

            EndPaint((HWND)widget->window, &ps);

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
            track_mouse_event.hwndTrack = (HWND)widget->window;
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

            bool const modifier = 0 != (wParam & MK_CONTROL);
            Number const scale = (
                modifier ? MOUSE_WHEEL_FINE_SCALE : MOUSE_WHEEL_COARSE_SCALE
            );
            Number const delta = (Number)GET_WHEEL_DELTA_WPARAM(wParam) * scale;

            is_handled = widget->mouse_wheel(delta);

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


Widget::Widget(GUI::PlatformData platform_data, GUI::Window window)
    : GUI::Object(),
    window(window),
    platform_data(platform_data),
    bitmap(NULL),
    parent(NULL),
    hdc(NULL),
    dwStyle(0),
    original_window_procedure(NULL),
    is_mouse_captured(false),
    is_timer_started(false)
{
}


Widget::Widget()
    : GUI::Object(),
    window(NULL),
    platform_data(NULL),
    bitmap(NULL),
    parent(NULL),
    hdc(NULL),
    dwStyle(0),
    original_window_procedure(NULL),
    is_mouse_captured(false),
    is_timer_started(false)
{
}


Widget::Widget(
        char const* const label,
        int const left,
        int const top,
        int const width,
        int const height,
        Type const type
) : GUI::Object(left, top, width, height),
    window(NULL),
    platform_data(NULL),
    bitmap(NULL),
    parent(NULL),
    hdc(NULL),
    class_name("STATIC"),
    label(label),
    dwStyle(0),
    original_window_procedure(NULL),
    is_mouse_captured(false),
    is_timer_started(false)
{
    switch (type) {
        case Type::BITMAP:
            dwStyle = WS_CHILD | WS_VISIBLE | SS_BITMAP;
            break;

        case Type::CLICKABLE_BITMAP:
            dwStyle = WS_CHILD | WS_VISIBLE | SS_BITMAP | SS_NOTIFY;
            break;

        default:
            dwStyle = WS_CHILD | WS_VISIBLE | SS_NOTIFY;
            break;
    }
}


void Widget::destroy_window()
{
    if (is_timer_started) {
        KillTimer((HWND)window, TIMER_ID);
    }

    if (window != NULL) {
        DestroyWindow((HWND)window);
        window = NULL;
    }
}


void Widget::set_up(GUI::PlatformData platform_data, Widget* parent)
{
    this->platform_data = platform_data;
    this->parent = parent;

    // TODO: GetLastError()
    window = (GUI::Window)CreateWindow(
        (LPCTSTR)class_name.get(),
        (LPCTSTR)label.get(),
        dwStyle,
        left,
        top,
        width,
        height,
        (HWND)parent->window,
        NULL,
        (HINSTANCE)platform_data,
        NULL
    );

    SetWindowLongPtr((HWND)window, GWLP_USERDATA, (LONG_PTR)this);

    original_window_procedure = (
        (WNDPROC)SetWindowLongPtr(
            (HWND)window, GWLP_WNDPROC, (LONG_PTR)(&process_message)
        )
    );
}


void Widget::start_timer(Frequency const frequency)
{
    UINT const elapse = (UINT)std::ceil(1000.0 / frequency);

    // TODO: GetLastError
    SetTimer((HWND)window, TIMER_ID, elapse, NULL);

    is_timer_started = true;
}


void Widget::fill_rectangle(
        int const left,
        int const top,
        int const width,
        int const height,
        Color const color
) {
    RECT rect;
    rect.left = left;
    rect.top = top;
    rect.right = left + width;
    rect.bottom = top + height;
    HBRUSH brush = CreateSolidBrush((COLORREF)color);
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
        Color const color,
        Color const background,
        FontWeight const font_weight,
        int const padding,
        TextAlignment const alignment
) {
    int const weight = font_weight == FontWeight::NORMAL ? FW_NORMAL : FW_BOLD;

    Text text_obj(text);

    HFONT font = CreateFont(
        -MulDiv(font_size_px, GetDeviceCaps(hdc, LOGPIXELSY), 72), // cHeight
        0,                              // cWidth
        0,                              // cEscapement
        0,                              // cOrientation
        weight,                         // cWeight
        FALSE,                          // bItalic
        FALSE,                          // bUnderline
        FALSE,                          // bStrikeOut
        ANSI_CHARSET,                   // iCharSet
        OUT_DEFAULT_PRECIS,             // iOutPrecision
        CLIP_DEFAULT_PRECIS,            // iClipPrecision
        DEFAULT_QUALITY,                // iQuality
        DEFAULT_PITCH | FF_DONTCARE,    // iPitchAndFamily
        TEXT("Arial")                   // pszFaceName
    );

    int orig_bk_mode = SetBkMode(hdc, OPAQUE);
    int orig_map_mode = SetMapMode(hdc, MM_TEXT);
    COLORREF orig_bk_color = SetBkColor(hdc, (COLORREF)background);
    COLORREF orig_text_color = SetTextColor(hdc, (COLORREF)color);
    HGDIOBJ orig_font = (HFONT)SelectObject(hdc, (HGDIOBJ)font);

    RECT text_rect;
    text_rect.left = left;
    text_rect.top = top;
    text_rect.right = left + width;
    text_rect.bottom = top + height;
    HBRUSH brush = CreateSolidBrush((COLORREF)background);
    FillRect(hdc, (LPRECT)&text_rect, brush);

    text_rect.left += padding;
    text_rect.right -= padding;

    UINT format;

    if (alignment == TextAlignment::CENTER) {
        format = DT_SINGLELINE | DT_CENTER | DT_VCENTER;
    } else {
        format = DT_SINGLELINE | DT_LEFT | DT_VCENTER;
    }

    DrawText(hdc, text_obj.get(), -1, (LPRECT)&text_rect, format);

    SelectObject(hdc, orig_font);
    SetTextColor(hdc, orig_text_color);
    SetBkColor(hdc, orig_bk_color);
    SetMapMode(hdc, orig_map_mode);
    SetBkMode(hdc, orig_bk_mode);

    DeleteObject((HGDIOBJ)brush);
    DeleteObject((HGDIOBJ)font);
}


GUI::Bitmap Widget::copy_bitmap_region(
        GUI::Bitmap source,
        int const left,
        int const top,
        int const width,
        int const height
) {
    HDC hdc = GetDC((HWND)window);

    HDC source_hdc = CreateCompatibleDC(hdc);
    HDC destination_hdc = CreateCompatibleDC(hdc);

    GUI::Bitmap destination_bitmap = CreateCompatibleBitmap(hdc, width, height);

    GUI::Bitmap old_source_bitmap = (
        (GUI::Bitmap)SelectObject(source_hdc, (HGDIOBJ)source)
    );
    GUI::Bitmap old_destination_bitmap = (
        (GUI::Bitmap)SelectObject(destination_hdc, (HGDIOBJ)destination_bitmap)
    );

    BitBlt(destination_hdc, 0, 0, width, height, source_hdc, left, top, SRCCOPY);

    SelectObject(source_hdc, (HGDIOBJ)old_source_bitmap);
    SelectObject(destination_hdc, (HGDIOBJ)old_destination_bitmap);

    DeleteDC(source_hdc);
    DeleteDC(destination_hdc);

    ReleaseDC((HWND)window, hdc);

    return destination_bitmap;
}


void Widget::show()
{
    // TODO: GetLastError()
    ShowWindow((HWND)window, SW_SHOWNORMAL);
}


void Widget::hide()
{
    // TODO: GetLastError()
    ShowWindow((HWND)window, SW_HIDE);
}


void Widget::focus()
{
    SetFocus((HWND)window);
}


void Widget::bring_to_top()
{
    BringWindowToTop((HWND)window);
}


void Widget::redraw()
{
    // TODO: GetLastError()
    RedrawWindow((HWND)window, NULL, NULL, RDW_INVALIDATE);
}


bool Widget::paint()
{
    if (bitmap == NULL) {
        return false;
    }

    HDC bitmap_hdc = CreateCompatibleDC(hdc);
    SelectObject(bitmap_hdc, (HBITMAP)bitmap);
    BitBlt(hdc, 0, 0, width, height, bitmap_hdc, 0, 0, SRCCOPY);
    DeleteDC(bitmap_hdc);

    return true;
}


LRESULT Widget::call_original_window_procedure(
        UINT uMsg,
        WPARAM wParam,
        LPARAM lParam
) {
    if (original_window_procedure != NULL) {
        // TODO: GetLastError()
        return CallWindowProc(
            original_window_procedure, (HWND)window, uMsg, wParam, lParam
        );
    } else {
        return DefWindowProcW((HWND)window, uMsg, wParam, lParam);
    }
}


void Widget::capture_mouse()
{
    is_mouse_captured = true;
    SetCapture((HWND)window);
}


void Widget::release_captured_mouse()
{
    if (is_mouse_captured) {
        is_mouse_captured = false;
        ReleaseCapture();
    }
}


std::string const ImportPatchButton::FILTER_STR(
    "JS80P Patches (*.js80p)\x00*.js80p\x00"
    "All Files (*.*)\x00*.*\x00",
    53
);


void ImportPatchButton::click()
{
    Widget::Text filter(ImportPatchButton::FILTER_STR);
    Widget::Text ext("js80p");
    TCHAR file_name[MAX_PATH];
    OPENFILENAME ofn;

    file_name[0] = 0;

    ZeroMemory(&ofn, sizeof(ofn));

    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = (HWND)window;
    ofn.lpstrFilter = filter.get();
    ofn.lpstrFile = file_name;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
    ofn.lpstrDefExt = ext.get();

    if (!GetOpenFileName(&ofn)) {
        return;
    }

    HANDLE file;
    file = CreateFile(
        file_name,              // lpFileName
        GENERIC_READ,           // dwDesiredAccess
        FILE_SHARE_READ,        // dwShareMode
        NULL,                   // lpSecurityAttributes
        OPEN_EXISTING,          // dwCreationDisposition
        FILE_ATTRIBUTE_NORMAL,  // dwFlagsAndAttributes
        NULL                    // hTemplateFile
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

        return;
    }

    CloseHandle(file);

    std::string const patch(
        buffer,
        std::min(
            (std::string::size_type)read,
            (std::string::size_type)Serializer::MAX_SIZE
        )
    );

    Serializer::import(synth, patch);

    synth_gui_body->refresh_param_editors();
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
    ofn.hwndOwner = (HWND)window;
    ofn.lpstrFilter = filter.get();
    ofn.lpstrFile = file_name;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = (
        OFN_EXPLORER
        | OFN_HIDEREADONLY
        | OFN_NOREADONLYRETURN
        | OFN_OVERWRITEPROMPT
        | OFN_PATHMUSTEXIST
    );
    ofn.lpstrDefExt = ext.get();

    if (!GetSaveFileName(&ofn)) {
        return;
    }

    HANDLE file;
    file = CreateFile(
        file_name,              // lpFileName
        GENERIC_WRITE,          // dwDesiredAccess
        0,                      // dwShareMode
        NULL,                   // lpSecurityAttributes
        CREATE_ALWAYS,          // dwCreationDisposition
        FILE_ATTRIBUTE_NORMAL,  // dwFlagsAndAttributes
        NULL                    // hTemplateFile
    );

    // TODO: GetLastError
    if (file == INVALID_HANDLE_VALUE) {
        return;
    }

    std::string const patch = Serializer::serialize(synth);
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

#include "widgets.cpp"

#endif
