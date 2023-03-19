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

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

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
    DeleteObject((HGDIOBJ)bitmap);
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

    switch (uMsg) {
        case WM_TIMER:
            return widget->timer(uMsg, wParam, lParam);

        case WM_PAINT:
            return widget->paint(uMsg, wParam, lParam);

        case WM_LBUTTONDBLCLK:
            return widget->lbuttondblclk(uMsg, wParam, lParam);

        case WM_LBUTTONDOWN:
            return widget->lbuttondown(uMsg, wParam, lParam);

        case WM_LBUTTONUP:
            return widget->lbuttonup(uMsg, wParam, lParam);

        case WM_MOUSEMOVE:
            return widget->mousemove(uMsg, wParam, lParam);

        case WM_MOUSELEAVE:
            return widget->mouseleave(uMsg, wParam, lParam);

        case WM_MOUSEWHEEL:
            return widget->mousewheel(uMsg, wParam, lParam);

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
            return widget->call_original_window_procedure(uMsg, wParam, lParam);
    }
}


Widget::Widget(GUI::PlatformData platform_data, GUI::Window window)
    : window(window),
    platform_data(platform_data),
    bitmap(NULL),
    parent(NULL),
    is_hidden(false),
    is_clicking(false),
    original_window_procedure(NULL)
{
}


Widget::Widget()
    : window(NULL),
    platform_data(NULL),
    bitmap(NULL),
    parent(NULL),
    is_hidden(false),
    is_clicking(false),
    original_window_procedure(NULL)
{
}


Widget::Widget(
        char const* const label,
        DWORD const dwStyle,
        int const left,
        int const top,
        int const width,
        int const height
) : window(NULL),
    platform_data(NULL),
    bitmap(NULL),
    parent(NULL),
    class_name("STATIC"),
    label(label),
    dwStyle(dwStyle),
    left(left),
    top(top),
    width(width),
    height(height),
    is_hidden(false),
    is_clicking(false),
    original_window_procedure(NULL)
{
}


Widget::~Widget()
{
    for (Children::iterator it = children.begin(); it != children.end(); ++it) {
        delete *it;
    }

    if (window != NULL) {
        DestroyWindow((HWND)window);
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


void Widget::fill_rectangle(
        HDC hdc,
        int const left,
        int const top,
        int const width,
        int const height,
        COLORREF const color
) {
    RECT rect;
    rect.left = left;
    rect.top = top;
    rect.right = left + width;
    rect.bottom = top + height;
    HBRUSH brush = CreateSolidBrush(color);
    FillRect(hdc, (LPRECT)&rect, brush);
    DeleteObject((HGDIOBJ)brush);
}


void Widget::draw_text(
        HDC hdc,
        Widget::Text const& text,
        int const font_size_px,
        int const left,
        int const top,
        int const width,
        int const height,
        COLORREF const color,
        COLORREF const background,
        int const weight,
        int const padding,
        UINT const format
) {
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
    COLORREF orig_bk_color = SetBkColor(hdc, background);
    COLORREF orig_text_color = SetTextColor(hdc, color);
    HGDIOBJ orig_font = (HFONT)SelectObject(hdc, (HGDIOBJ)font);

    RECT text_rect;
    text_rect.left = left;
    text_rect.top = top;
    text_rect.right = left + width;
    text_rect.bottom = top + height;
    HBRUSH brush = CreateSolidBrush(background);
    FillRect(hdc, (LPRECT)&text_rect, brush);

    text_rect.left += padding;
    text_rect.right -= padding;

    DrawText(
        hdc,
        text.get(),
        -1,
        (LPRECT)&text_rect,
        format
    );

    SelectObject(hdc, orig_font);
    SetTextColor(hdc, orig_text_color);
    SetBkColor(hdc, orig_bk_color);
    SetMapMode(hdc, orig_map_mode);
    SetBkMode(hdc, orig_bk_mode);

    DeleteObject((HGDIOBJ)brush);
    DeleteObject((HGDIOBJ)font);
}


void Widget::show()
{
    // TODO: GetLastError()
    is_hidden = false;
    ShowWindow((HWND)window, SW_SHOWNORMAL);
}


void Widget::hide()
{
    // TODO: GetLastError()
    is_hidden = true;
    ShowWindow((HWND)window, SW_HIDE);
}


void Widget::redraw()
{
    // TODO: GetLastError()
    RedrawWindow((HWND)window, NULL, NULL, RDW_INVALIDATE);
}


void Widget::click()
{
}


Widget* Widget::own(Widget* widget)
{
    children.push_back(widget);
    widget->set_up(platform_data, (Widget*)this);

    return widget;
}


GUI::Bitmap Widget::set_bitmap(GUI::Bitmap bitmap)
{
    // TODO: GetLastError()
    GUI::Bitmap old = this->bitmap;

    this->bitmap = bitmap;

    redraw();

    return old;
}


LRESULT Widget::timer(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return call_original_window_procedure(uMsg, wParam, lParam);
}


LRESULT Widget::paint(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (bitmap != NULL) {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint((HWND)window, &ps);

        HDC bitmap_hdc = CreateCompatibleDC(hdc);
        SelectObject(bitmap_hdc, (HBITMAP)bitmap);
        BitBlt(hdc, 0, 0, width, height, bitmap_hdc, 0, 0, SRCCOPY);

        EndPaint((HWND)window, &ps);
        DeleteDC(bitmap_hdc);

        return 0;
    }

    return call_original_window_procedure(uMsg, wParam, lParam);
}


LRESULT Widget::lbuttondblclk(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return call_original_window_procedure(uMsg, wParam, lParam);
}


LRESULT Widget::lbuttondown(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    is_clicking = true;

    return call_original_window_procedure(uMsg, wParam, lParam);
}


LRESULT Widget::lbuttonup(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (is_clicking) {
        is_clicking = false;
        click();
    }

    return call_original_window_procedure(uMsg, wParam, lParam);
}


LRESULT Widget::mousemove(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return call_original_window_procedure(uMsg, wParam, lParam);
}


LRESULT Widget::mouseleave(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return call_original_window_procedure(uMsg, wParam, lParam);
}


LRESULT Widget::mousewheel(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return call_original_window_procedure(uMsg, wParam, lParam);
}


ExternallyCreatedWindow::ExternallyCreatedWindow(
        GUI::PlatformData platform_data,
        GUI::Window window
) : Widget(platform_data, window)
{
}


ExternallyCreatedWindow::~ExternallyCreatedWindow()
{
    window = NULL;
}


TransparentWidget::TransparentWidget(
        char const* const label,
        int const left,
        int const top,
        int const width,
        int const height
) : Widget(
        label,
        WS_CHILD | WS_VISIBLE | SS_NOTIFY,
        left,
        top,
        width,
        height
    )
{
}


LRESULT TransparentWidget::paint(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    /*
    Though we don't want to paint anything, we still need to validate the
    region that is to be painted - BeginPaint() and EndPaint() does
    everything that needs to be done about this.
    */

    PAINTSTRUCT ps;
    BeginPaint((HWND)window, &ps);
    EndPaint((HWND)window, &ps);

    return 0;
}


TabBody::TabBody(char const* const label)
    : TransparentWidget(label, LEFT, TOP, WIDTH, HEIGHT)
{
}


ParamEditor* TabBody::own(ParamEditor* param_editor)
{
    Widget::own(param_editor);

    param_editors.push_back(param_editor);

    return param_editor;
}


void TabBody::refresh_controlled_param_editors()
{
    GUI::refresh_controlled_param_editors(param_editors);
}


void TabBody::refresh_param_editors()
{
    GUI::refresh_param_editors(param_editors);
}


Background::Background()
    : Widget(
        "JS80P",
        WS_CHILD | WS_VISIBLE | SS_BITMAP,
        0,
        0,
        GUI::WIDTH,
        GUI::HEIGHT
    ),
    body(NULL),
    next_full_refresh(FULL_REFRESH_TICKS)
{
}


Background::~Background()
{
    KillTimer((HWND)window, TIMER_ID);
}


void Background::replace_body(TabBody* new_body)
{
    if (body != NULL) {
        body->hide();
    }

    body = new_body;
    body->show();
}


void Background::hide_body()
{
    if (body != NULL) {
        body->hide();
    }
}


void Background::show_body()
{
    if (body != NULL) {
        body->show();
    }
}


void Background::set_up(GUI::PlatformData platform_data, Widget* parent)
{
    Widget::set_up(platform_data, parent);

    constexpr UINT elapse = (UINT)std::ceil(1000.0 / REFRESH_RATE);

    // TODO: GetLastError
    SetTimer((HWND)window, TIMER_ID, elapse, NULL);
}


LRESULT Background::timer(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (body == NULL) {
        return 0;
    }

    --next_full_refresh;

    if (next_full_refresh == 0) {
        next_full_refresh = FULL_REFRESH_TICKS;
        body->refresh_param_editors();
    } else {
        body->refresh_controlled_param_editors();
    }

    return 0;
}


TabSelector::TabSelector(
        Background* background,
        GUI::Bitmap bitmap,
        TabBody* tab_body,
        char const* const label,
        int const left
) : TransparentWidget(label, left, TOP, WIDTH, HEIGHT),
    background(background),
    tab_body(tab_body),
    bitmap(bitmap)
{
}


void TabSelector::click()
{
    background->set_bitmap(bitmap);
    background->replace_body(tab_body);
}


std::string const ImportPatchButton::FILTER_STR(
    "JS80P Patches (*.js80p)\x00*.js80p\x00"
    "All Files (*.*)\x00*.*\x00",
    53
);


ImportPatchButton::ImportPatchButton(
        int const left,
        int const top,
        int const width,
        int const height,
        Synth& synth,
        TabBody* synth_gui_body
) : TransparentWidget("Import Patch", left, top, width, height),
    synth(synth),
    synth_gui_body(synth_gui_body)
{
}


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


ExportPatchButton::ExportPatchButton(
        int const left,
        int const top,
        int const width,
        int const height,
        Synth& synth
) : TransparentWidget("Export Patch", left, top, width, height),
    synth(synth)
{
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


ControllerSelector::ControllerSelector(
        Background& background,
        Synth& synth
) : Widget(
        "Select controller",
        WS_CHILD | WS_VISIBLE | SS_NOTIFY,
        LEFT,
        TOP,
        WIDTH,
        HEIGHT
    ),
    background(background),
    synth(synth),
    param_editor(NULL),
    param_id(Synth::ParamId::MAX_PARAM_ID),
    selected_controller_id(Synth::ControllerId::MAX_CONTROLLER_ID)
{
}


void ControllerSelector::set_up(GUI::PlatformData platform_data, Widget* parent)
{
    Widget::set_up(platform_data, parent);

    constexpr int max_top = HEIGHT - Controller::HEIGHT;
    int top = TITLE_HEIGHT;
    int left = 10;

    for (int i = 0; i != GUI::ALL_CTLS; ++i) {
        Synth::ControllerId const id = GUI::CONTROLLERS[i].id;
        char const* const label = GUI::CONTROLLERS[i].long_name;

        controllers[i] = (Controller*)this->own(
            new Controller(*this, label, left, top, id)
        );

        top += Controller::HEIGHT;

        if (i == 0 || i == 3 || i == 82 || i == 90) {
            top += 11;
        }

        if (top > max_top || i == 72) {
            top = TITLE_HEIGHT;
            left += Controller::WIDTH;
        }
    }
}


void ControllerSelector::show(
        Synth::ParamId const param_id,
        int const controller_choices,
        ParamEditor* param_editor
) {
    Synth::ControllerId const selected_controller_id = (
        synth.get_param_controller_id_atomic(param_id)
    );

    GUI::Controller const* const controller = GUI::get_controller(
        selected_controller_id
    );

    if (controller == NULL) {
        return;
    }

    if (this->selected_controller_id < Synth::ControllerId::MAX_CONTROLLER_ID) {
        GUI::Controller const* const old_controller = GUI::get_controller(
            this->selected_controller_id
        );
        controllers[old_controller->index]->unselect();
    }

    constexpr int title_size = 128;
    char title[title_size];

    snprintf(
        title, title_size, "Select controller for \"%s\"", GUI::PARAMS[param_id]
    );

    this->param_id = param_id;
    this->param_editor = param_editor;
    this->selected_controller_id = selected_controller_id;
    this->title.set(title);

    controllers[controller->index]->select();

    for (int i = 0; i != controller_choices; ++i) {
        controllers[i]->show();
    }

    for (int i = controller_choices; i != GUI::ALL_CTLS; ++i) {
        controllers[i]->hide();
    }

    redraw();
    Widget::show();
    background.hide_body();
    BringWindowToTop((HWND)window);
}


void ControllerSelector::hide()
{
    background.show_body();
    Widget::hide();
}


void ControllerSelector::handle_selection_change(
        Synth::ControllerId const new_controller_id
) {
    hide();

    if (param_editor == NULL || param_id >= Synth::Synth::ParamId::MAX_PARAM_ID) {
        return;
    }

    param_editor->handle_controller_change(new_controller_id);
}


LRESULT ControllerSelector::paint(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint((HWND)window, &ps);

    fill_rectangle(hdc, 0, 0, width, height, RGB(0, 0, 0));
    draw_text(
        hdc,
        title,
        12,
        0,
        0,
        WIDTH,
        TITLE_HEIGHT,
        RGB(181, 181, 189),
        RGB(0, 0, 0),
        FW_BOLD,
        10,
        DT_SINGLELINE | DT_LEFT | DT_VCENTER
    );
    EndPaint((HWND)window, &ps);

    return 0;
}


ControllerSelector::Controller::Controller(
        ControllerSelector& controller_selector,
        char const* const label,
        int const left,
        int const top,
        Synth::ControllerId const controller_id
) : Widget(
        label,
        WS_CHILD | WS_VISIBLE | SS_NOTIFY,
        left,
        top,
        WIDTH,
        HEIGHT
    ),
    controller_id(controller_id),
    controller_selector(controller_selector),
    is_selected(false),
    is_mouse_over(false)
{
}


void ControllerSelector::Controller::select()
{
    is_selected = true;
    redraw();
}


void ControllerSelector::Controller::unselect()
{
    is_selected = false;
    redraw();
}


LRESULT ControllerSelector::Controller::paint(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint((HWND)window, &ps);

    COLORREF background;
    COLORREF color;

    if (is_selected) {
        background = RGB(181, 181, 189);
        color = RGB(0, 0, 0);
    } else if (is_mouse_over) {
        background = RGB(63, 63, 66);
        color = RGB(225, 225, 235);
    } else {
        background = RGB(0, 0, 0);
        color = RGB(181, 181, 189);
    }

    draw_text(
        hdc,
        label,
        12,
        0,
        0,
        width,
        height,
        color,
        background,
        FW_BOLD,
        10,
        DT_SINGLELINE | DT_LEFT | DT_VCENTER
    );

    EndPaint((HWND)window, &ps);

    return 0;
}


LRESULT ControllerSelector::Controller::lbuttonup(
        UINT uMsg,
        WPARAM wParam,
        LPARAM lParam
) {
    controller_selector.handle_selection_change(controller_id);

    return 0;
}


LRESULT ControllerSelector::Controller::mousemove(
        UINT uMsg,
        WPARAM wParam,
        LPARAM lParam
) {
    if (!is_mouse_over) {
        TRACKMOUSEEVENT track_mouse_event;
        track_mouse_event.cbSize = sizeof(TRACKMOUSEEVENT);
        track_mouse_event.dwFlags = TME_LEAVE;
        track_mouse_event.hwndTrack = (HWND)window;
        track_mouse_event.dwHoverTime = 0;
        TrackMouseEvent(&track_mouse_event);

        is_mouse_over = true;
        redraw();
    }

    return 0;
}


LRESULT ControllerSelector::Controller::mouseleave(
        UINT uMsg,
        WPARAM wParam,
        LPARAM lParam
) {
    if (is_mouse_over) {
        is_mouse_over = false;
        redraw();
    }

    return 0;
}


ParamEditor::ParamEditor(
        char const* const label,
        int const left,
        int const top,
        ControllerSelector& controller_selector,
        Synth& synth,
        Synth::ParamId const param_id,
        int const controller_choices,
        char const* format,
        double const scale
) : TransparentWidget(label, left, top, WIDTH, HEIGHT),
    param_id(param_id),
    format(format),
    scale(scale),
    options(NULL),
    number_of_options(0),
    value_font_size(11),
    controller_choices(controller_choices),
    controller_selector(controller_selector),
    synth(synth),
    ratio(0.0),
    knob(NULL),
    has_controller_(false)
{
}


ParamEditor::ParamEditor(
        char const* const label,
        int const left,
        int const top,
        ControllerSelector& controller_selector,
        Synth& synth,
        Synth::ParamId const param_id,
        int const controller_choices,
        char const* const* const options,
        int const number_of_options
) : TransparentWidget(label, left, top, WIDTH, HEIGHT),
    param_id(param_id),
    format(NULL),
    scale(1.0),
    options(options),
    number_of_options(number_of_options),
    value_font_size(10),
    controller_choices(controller_choices),
    controller_selector(controller_selector),
    synth(synth),
    ratio(0.0),
    knob(NULL),
    controller_id(Synth::ControllerId::NONE),
    has_controller_(false)
{
}


void ParamEditor::set_up(GUI::PlatformData platform_data, Widget* parent)
{
    TransparentWidget::set_up(platform_data, parent);

    knob = new Knob(
        *this,
        label.c_str(),
        (WIDTH - ParamEditor::Knob::WIDTH) / 2,
        16,
        number_of_options > 1 ? (Number)(number_of_options - 1) : 0.0
    );

    own(knob);
    default_ratio = synth.get_param_default_ratio(param_id);
    update_editor(
        synth.get_param_ratio_atomic(param_id),
        synth.get_param_controller_id_atomic(param_id)
    );
}


bool ParamEditor::has_controller() const
{
    return has_controller_;
}


void ParamEditor::refresh()
{
    Synth::ControllerId const new_controller_id = (
        synth.get_param_controller_id_atomic(param_id)
    );
    Number const new_ratio = synth.get_param_ratio_atomic(param_id);

    has_controller_ = new_controller_id > Synth::Synth::ControllerId::NONE;

    if (new_ratio != ratio || new_controller_id != controller_id) {
        update_editor(new_ratio, new_controller_id);
    } else {
        synth.push_message(
            Synth::MessageType::REFRESH_PARAM, param_id, 0.0, 0
        );
    }
}


void ParamEditor::update_editor(
        Number const new_ratio,
        Synth::ControllerId const new_controller_id
) {
    controller_id = new_controller_id;
    update_editor(new_ratio);
}


void ParamEditor::update_editor(Number const new_ratio)
{
    ratio = GUI::clamp_ratio(new_ratio);
    update_editor();
}


void ParamEditor::update_editor(Synth::ControllerId const new_controller_id)
{
    controller_id = new_controller_id;
    update_editor();
}


void ParamEditor::update_editor()
{
    has_controller_ = controller_id > Synth::ControllerId::NONE;

    update_value_str();
    update_controller_str();
    redraw();

    if (has_controller_) {
        knob->deactivate();
    } else {
        knob->activate();
    }

    knob->update(ratio);
}


void ParamEditor::adjust_ratio(Number const delta)
{
    handle_ratio_change(ratio + delta);
}


void ParamEditor::handle_ratio_change(Number const new_ratio)
{
    Number const ratio = GUI::clamp_ratio(new_ratio);

    synth.push_message(
        Synth::MessageType::SET_PARAM, param_id, ratio, 0
    );
    update_editor(ratio);
}


void ParamEditor::handle_controller_change(Synth::ControllerId const new_controller_id)
{
    synth.push_message(
        Synth::MessageType::ASSIGN_CONTROLLER,
        param_id,
        0.0,
        (Byte)new_controller_id
    );
    has_controller_ = true;
    update_editor(new_controller_id);
}


void ParamEditor::update_value_str()
{
    constexpr size_t buffer_size = 16;
    char buffer[buffer_size];

    GUI::param_ratio_to_str(
        synth,
        param_id,
        ratio,
        scale,
        format,
        options,
        number_of_options,
        buffer,
        buffer_size
    );

    value_str.set(buffer);
}


void ParamEditor::update_controller_str()
{
    constexpr size_t last_index = 15;
    char buffer[last_index + 1];

    strncpy(
        buffer, GUI::get_controller(controller_id)->short_name, last_index
    );
    buffer[last_index] = '\x00';
    controller_str.set(buffer);
}


void ParamEditor::reset_default()
{
    handle_ratio_change(default_ratio);
}


LRESULT ParamEditor::paint(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint((HWND)window, &ps);

    draw_text(
        hdc,
        value_str,
        value_font_size,
        1,
        HEIGHT - 20,
        WIDTH - 2,
        20,
        RGB(181, 181, 189),
        RGB(0, 0, 0)
    );

    if (controller_choices > 0) {
        draw_text(
            hdc,
            controller_str,
            10,
            1,
            HEIGHT - 36,
            WIDTH - 2,
            16,
            has_controller_ ? RGB(0, 0, 0) : RGB(181, 181, 189),
            has_controller_ ? RGB(145, 145, 151) : RGB(0, 0, 0),
            has_controller_ ? FW_BOLD : FW_NORMAL
        );
    }

    EndPaint((HWND)window, &ps);

    return 0;
}


LRESULT ParamEditor::lbuttonup(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (is_clicking && controller_choices > 0) {
        controller_selector.show(param_id, controller_choices, this);
    }

    return Widget::lbuttonup(uMsg, wParam, lParam);
}


GUI::Bitmap ParamEditor::knob_states = NULL;

GUI::Bitmap ParamEditor::knob_states_inactive = NULL;


ParamEditor::Knob::Knob(
        ParamEditor& editor,
        char const* const label,
        int const left,
        int const top,
        Number const steps
) : Widget(
        label,
        WS_CHILD | WS_VISIBLE | SS_BITMAP | SS_NOTIFY,
        left,
        top,
        WIDTH,
        HEIGHT
    ),
    steps(steps),
    editor(editor),
    knob_state(NULL),
    ratio(0.0),
    is_inactive(false),
    is_clicking(false),
    is_mouse_captured(false)
{
}


ParamEditor::Knob::~Knob()
{
    if (knob_state != NULL) {
        DeleteObject((HGDIOBJ)knob_state);
        knob_state = NULL;
    }

    release_captured_mouse();
}


void ParamEditor::Knob::set_up(GUI::PlatformData platform_data, Widget* parent)
{
    Widget::set_up(platform_data, parent);

    update(0.0);
}


void ParamEditor::Knob::update(Number const ratio)
{
    this->ratio = (
        steps > 0.0 ? std::round(ratio * steps) / steps : ratio
    );

    update();
}


void ParamEditor::Knob::update()
{
    HDC hdc = GetDC((HWND)window);

    HDC source_hdc = CreateCompatibleDC(hdc);
    HDC destination_hdc = CreateCompatibleDC(hdc);

    GUI::Bitmap source_bitmap = is_inactive ? knob_states_inactive : knob_states;
    GUI::Bitmap destination_bitmap = CreateCompatibleBitmap(hdc, WIDTH, HEIGHT);

    GUI::Bitmap old_source_bitmap = (
        (GUI::Bitmap)SelectObject(source_hdc, (HGDIOBJ)source_bitmap)
    );
    GUI::Bitmap old_destination_bitmap = (
        (GUI::Bitmap)SelectObject(destination_hdc, (HGDIOBJ)destination_bitmap)
    );

    int source_x = (int)(KNOB_STATES_LAST_INDEX * this->ratio) * WIDTH;
    BitBlt(destination_hdc, 0, 0, WIDTH, HEIGHT, source_hdc, source_x, 0, SRCCOPY);

    SelectObject(source_hdc, (HGDIOBJ)old_source_bitmap);
    SelectObject(destination_hdc, (HGDIOBJ)old_destination_bitmap);

    DeleteDC(source_hdc);
    DeleteDC(destination_hdc);

    ReleaseDC((HWND)window, hdc);

    if (knob_state != NULL) {
        DeleteObject((HGDIOBJ)knob_state);
    }

    knob_state = destination_bitmap;
    set_bitmap(knob_state);
}


void ParamEditor::Knob::activate()
{
    is_inactive = false;
    update();
}


void ParamEditor::Knob::deactivate()
{
    is_inactive = true;
    update();
}


void ParamEditor::Knob::capture_mouse()
{
    is_mouse_captured = true;
    SetCapture((HWND)window);
}


void ParamEditor::Knob::release_captured_mouse()
{
    if (is_mouse_captured) {
        is_mouse_captured = false;
        ReleaseCapture();
    }
}


LRESULT ParamEditor::Knob::lbuttondblclk(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (is_inactive) {
        return 0;
    }

    if (!is_clicking) {
        editor.reset_default();
    }

    release_captured_mouse();

    return 0;
}


LRESULT ParamEditor::Knob::lbuttondown(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (is_inactive) {
        return 0;
    }

    is_clicking = true;

    prev_x = GET_X_LPARAM(lParam);
    prev_y = GET_Y_LPARAM(lParam);

    capture_mouse();

    return 0;
}


LRESULT ParamEditor::Knob::lbuttonup(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (is_inactive) {
        return 0;
    }

    is_clicking = false;
    mousemove(WM_MOUSEMOVE, wParam, lParam);

    release_captured_mouse();

    return 0;
}


LRESULT ParamEditor::Knob::mousemove(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (is_inactive) {
        return 0;
    }

    if (is_clicking) {
        bool const control = 0 != (wParam & MK_CONTROL);
        Number const scale = (
            control ? MOUSE_MOVE_FINE_SCALE : MOUSE_MOVE_COARSE_SCALE
        );
        Number const x = (Number)GET_X_LPARAM(lParam);
        Number const y = (Number)GET_Y_LPARAM(lParam);
        Number const dx = x - prev_x;
        Number const dy = y - prev_y;
        Number const delta = (
            scale * ((std::fabs(dx) > std::fabs(dy)) ? dx : -dy)
        );

        prev_x = x;
        prev_y = y;
        editor.adjust_ratio(delta);
    }

    SetFocus((HWND)window);

    return 0;
}


LRESULT ParamEditor::Knob::mouseleave(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    release_captured_mouse();

    return 0;
}


LRESULT ParamEditor::Knob::mousewheel(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (is_inactive) {
        return 0;
    }

    bool const control = 0 != (wParam & MK_CONTROL);
    Number const scale = (
        control ? MOUSE_WHEEL_FINE_SCALE : MOUSE_WHEEL_COARSE_SCALE
    );
    Number const delta = (Number)GET_WHEEL_DELTA_WPARAM(wParam) * scale;

    if (steps > 0.0) {
        editor.adjust_ratio(delta * 10.0);
    } else {
        editor.adjust_ratio(delta);
    }

    return 0;
}

}
