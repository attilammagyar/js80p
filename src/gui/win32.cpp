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

#include <cmath>
#include <cstdio>
#include <cstring>

#include "gui/win32.hpp"

#include "gui/gui.cpp"


namespace JS80P {

GUI* GUI::create_instance(
        Application application,
        Window parent_window,
        Synth& synth
) {
    return (
        (GUI*)new Win32GUI::GUI(
            (HINSTANCE)application, (HWND)parent_window, synth
        )
    );
}

}


namespace JS80P { namespace Win32GUI
{

Text::Text()
{
    set("");
}


Text::Text(char const* const text)
{
    set(text);
}


void Text::set(char const* const text)
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstringop-truncation"
    strncpy(this->text, text, MAX_LENGTH - 1);
#pragma GCC diagnostic pop

    this->text[MAX_LENGTH - 1] = '\0';

#ifdef UNICODE
    MultiByteToWideChar(
        CP_UTF8,
        0,
        (LPCCH)text,
        -1,
        (LPWSTR)wtext,
        MAX_LENGTH
    );
#endif
}


char const* Text::c_str() const
{
    return text;
}


WCHAR const* Text::c_wstr() const
{
    return wtext;
}


#ifdef UNICODE
    LPCTSTR Text::get() const
    {
        return (LPCTSTR)c_wstr();
    }
#else
    LPCTSTR Text::get() const
    {
        return (LPCTSTR)c_str();
    }
#endif


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


Widget::Widget(HINSTANCE application, HWND hwnd)
    : hwnd(hwnd),
    parent(NULL),
    application(application),
    bitmap(NULL),
    is_hidden(false),
    is_clicking(false),
    original_window_procedure(NULL)
{
}


Widget::Widget()
    : hwnd(NULL),
    parent(NULL),
    application(NULL),
    bitmap(NULL),
    is_hidden(false),
    is_clicking(false),
    original_window_procedure(NULL)
{
}


Widget::Widget(
        char const* const class_name,
        char const* const label,
        DWORD dwStyle,
        int left,
        int top,
        int width,
        int height
) : hwnd(NULL),
    parent(NULL),
    application(NULL),
    bitmap(NULL),
    class_name(class_name),
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

    if (hwnd != NULL) {
        DestroyWindow(hwnd);
    }
}


void Widget::set_up(HINSTANCE application, Widget* parent)
{
    this->application = application;
    this->parent = parent;

    // TODO: GetLastError()
    hwnd = CreateWindow(
        (LPCTSTR)class_name.get(),
        (LPCTSTR)label.get(),
        dwStyle,
        left,
        top,
        width,
        height,
        parent->hwnd,
        NULL,
        application,
        NULL
    );

    SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)this);

    original_window_procedure = (
        (WNDPROC)SetWindowLongPtr(
            hwnd, GWLP_WNDPROC, (LONG_PTR)(&process_message)
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
            original_window_procedure, hwnd, uMsg, wParam, lParam
        );
    } else {
        return DefWindowProcW(hwnd, uMsg, wParam, lParam);
    }
}


void Widget::fill_rectangle(
        HDC hdc,
        int const left,
        int const top,
        int const width,
        int const height,
        COLORREF color
) {
    RECT rect;
    rect.left = left;
    rect.top = top;
    rect.right = width;
    rect.bottom = top + height;
    HBRUSH brush = CreateSolidBrush(color);
    FillRect(hdc, (LPRECT)&rect, brush);
    DeleteObject((HGDIOBJ)brush);
}


void Widget::draw_text(
        HDC hdc,
        Text const& text,
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
    ShowWindow(hwnd, SW_SHOWNORMAL);
}


void Widget::hide()
{
    // TODO: GetLastError()
    is_hidden = true;
    ShowWindow(hwnd, SW_HIDE);
}


void Widget::redraw()
{
    // TODO: GetLastError()
    RedrawWindow(hwnd, NULL, NULL, RDW_INVALIDATE);
}


void Widget::click()
{
}


Widget* Widget::own(Widget* widget)
{
    children.push_back(widget);
    widget->set_up(application, (Widget*)this);

    return widget;
}


HBITMAP Widget::set_bitmap(HBITMAP bitmap)
{
    // TODO: GetLastError()
    HBITMAP old = this->bitmap;

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
        HDC hdc = BeginPaint(hwnd, &ps);

        HDC bitmap_hdc = CreateCompatibleDC(hdc);
        SelectObject(bitmap_hdc, bitmap);
        BitBlt(hdc, 0, 0, width, height, bitmap_hdc, 0, 0, SRCCOPY);

        EndPaint(hwnd, &ps);
        DeleteDC(bitmap_hdc);

        return 0;
    } else {
        return call_original_window_procedure(uMsg, wParam, lParam);
    }
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
        HINSTANCE application,
        HWND hwnd
) : Widget(application, hwnd)
{
}


ExternallyCreatedWindow::~ExternallyCreatedWindow()
{
    hwnd = NULL;
}


TransparentWidget::TransparentWidget(
        char const* const label,
        int left,
        int top,
        int width,
        int height
) : Widget(
        "STATIC",
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
    BeginPaint(hwnd, &ps);
    EndPaint(hwnd, &ps);

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


void TabBody::refresh()
{
    for (ParamEditors::iterator it = param_editors.begin(); it != param_editors.end(); ++it) {
        (*it)->refresh();
    }
}


Background::Background()
    : Widget(
        "STATIC",
        "JS80P",
        WS_CHILD | WS_VISIBLE | SS_BITMAP,
        0,
        0,
        JS80P::GUI::WIDTH,
        JS80P::GUI::HEIGHT
    ),
    body(NULL)
{
}


Background::~Background()
{
    KillTimer(hwnd, TIMER_ID);
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


void Background::refresh_body()
{
    if (body != NULL) {
        body->refresh();
    }
}


void Background::set_up(HINSTANCE application, Widget* parent)
{
    Widget::set_up(application, parent);

    // TODO: GetLastError
    SetTimer(hwnd, TIMER_ID, 83, NULL);
}


LRESULT Background::timer(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    refresh_body();

    return 0;
}


TabSelector::TabSelector(
        Background* background,
        HBITMAP bitmap,
        TabBody* tab_body,
        char const* const label,
        int left
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


ControllerSelector::ControllerSelector(
        Background& background,
        Synth& synth
) : Widget(
        "STATIC",
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
    param_id(ParamId::MAX_PARAM_ID),
    selected_controller_id(ControllerId::MAX_CONTROLLER_ID)
{
}


void ControllerSelector::set_up(HINSTANCE application, Widget* parent)
{
    Widget::set_up(application, parent);

    constexpr int max_top = HEIGHT - Controller::HEIGHT;
    int top = TITLE_HEIGHT;
    int left = 10;

    for (int i = 0; i != GUI::ALL_CTLS; ++i) {
        ControllerId const id = GUI::CONTROLLERS[i].id;
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
        ParamId const param_id,
        int const controller_choices,
        ParamEditor* param_editor
) {
    ControllerId const selected_controller_id = synth.get_param_controller_id(
        param_id
    );

    JS80P::GUI::Controller const* const controller = GUI::get_controller(
        selected_controller_id
    );

    if (controller == NULL) {
        return;
    }

    if (this->selected_controller_id < ControllerId::MAX_CONTROLLER_ID) {
        JS80P::GUI::Controller const* const old_controller = GUI::get_controller(
            this->selected_controller_id
        );
        controllers[old_controller->index]->unselect();
    }

    char title[Text::MAX_LENGTH];

    snprintf(
        title,
        Text::MAX_LENGTH,
        "Select controller for \"%s\"",
        JS80P::GUI::PARAMS[param_id]
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
    BringWindowToTop(hwnd);
}


void ControllerSelector::hide()
{
    background.show_body();
    Widget::hide();
}


void ControllerSelector::handle_selection_change(
        ControllerId const new_controller_id
) {
    hide();

    if (param_editor == NULL || param_id >= ParamId::MAX_PARAM_ID) {
        return;
    }

    param_editor->handle_controller_change(new_controller_id);
}


LRESULT ControllerSelector::paint(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);

    fill_rectangle(hdc, left, top, width, height, RGB(0, 0, 0));
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
    EndPaint(hwnd, &ps);

    return 0;
}


ControllerSelector::Controller::Controller(
        ControllerSelector& controller_selector,
        char const* const label,
        int const left,
        int const top,
        ControllerId const controller_id
) : Widget(
        "STATIC",
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
    HDC hdc = BeginPaint(hwnd, &ps);

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

    EndPaint(hwnd, &ps);

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
        track_mouse_event.hwndTrack = hwnd;
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
        int left,
        int top,
        ControllerSelector& controller_selector,
        Synth& synth,
        ParamId const param_id,
        int const controller_choices,
        char const* format,
        double const scale
) : TransparentWidget(label, left, top, WIDTH, HEIGHT),
    param_id(param_id),
    format(format),
    scale(scale),
    options(NULL),
    number_of_options(0),
    controller_choices(controller_choices),
    controller_selector(controller_selector),
    synth(synth),
    ratio(0.0),
    knob(NULL),
    has_controller(false)
{
}


ParamEditor::ParamEditor(
        char const* const label,
        int left,
        int top,
        ControllerSelector& controller_selector,
        Synth& synth,
        ParamId const param_id,
        int const controller_choices,
        char const* const* const options,
        int const number_of_options
) : TransparentWidget(label, left, top, WIDTH, HEIGHT),
    param_id(param_id),
    format(NULL),
    scale(1.0),
    options(options),
    number_of_options(number_of_options),
    controller_choices(controller_choices),
    controller_selector(controller_selector),
    synth(synth),
    ratio(0.0),
    knob(NULL),
    controller_id(ControllerId::NONE),
    has_controller(false)
{
}


void ParamEditor::set_up(HINSTANCE application, Widget* parent)
{
    TransparentWidget::set_up(application, parent);

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
        synth.get_param_controller_id(param_id)
    );
}


void ParamEditor::refresh()
{
    Number const new_ratio = synth.get_param_ratio_atomic(param_id);
    ControllerId const new_controller_id = (
        synth.get_param_controller_id(param_id)
    );
    bool const had_controller = has_controller;

    has_controller = new_controller_id > Synth::ControllerId::NONE;

    if (!(has_controller && had_controller)) {
        return;
    }

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
        ControllerId const new_controller_id
) {
    controller_id = new_controller_id;
    update_editor(new_ratio);
}


void ParamEditor::update_editor(Number const new_ratio)
{
    ratio = clamp(new_ratio);
    update_editor();
}


void ParamEditor::update_editor(ControllerId const new_controller_id)
{
    controller_id = new_controller_id;
    update_editor();
}


void ParamEditor::update_editor()
{
    has_controller = controller_id > Synth::ControllerId::NONE;

    update_value_str();
    update_controller_str();
    redraw();

    if (has_controller) {
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
    Number const ratio = clamp(new_ratio);

    synth.push_message(
        Synth::MessageType::SET_PARAM, param_id, ratio, 0
    );
    update_editor(ratio);
}


void ParamEditor::handle_controller_change(ControllerId const new_controller_id)
{
    synth.push_message(
        Synth::MessageType::ASSIGN_CONTROLLER,
        param_id,
        0.0,
        (Byte)new_controller_id
    );
    has_controller = true;
    update_editor(new_controller_id);
}


Number ParamEditor::clamp(Number const ratio) const
{
    return std::min(1.0, std::max(0.0, ratio));
}


void ParamEditor::update_value_str()
{
    if (format != NULL) {
        update_value_str_float();
    } else if (options != NULL) {
        update_value_str_int();
    }
}


void ParamEditor::update_value_str_float()
{
    double const value = (
        synth.float_param_ratio_to_display_value(param_id, ratio) * scale
    );
    char buffer[Text::MAX_LENGTH];

    snprintf(buffer, Text::MAX_LENGTH, format, value);

    if (strncmp("-0", buffer, Text::MAX_LENGTH) == 0) {
        value_str.set("0");
    } else if (strncmp("-0.0", buffer, Text::MAX_LENGTH) == 0) {
        value_str.set("0.0");
    } else {
        value_str.set(buffer);
    }
}


void ParamEditor::update_value_str_int()
{
    Byte const value = (
        synth.int_param_ratio_to_display_value(param_id, ratio)
    );

    if ((int)value >= number_of_options) {
        return;
    }

    char buffer[Text::MAX_LENGTH];

    strncpy(buffer, options[value], Text::MAX_LENGTH - 1);
    value_str.set(buffer);
}


void ParamEditor::update_controller_str()
{
    char buffer[Text::MAX_LENGTH];

    strncpy(
        buffer, GUI::get_controller(controller_id)->short_name, Text::MAX_LENGTH - 1
    );
    controller_str.set(buffer);
}


void ParamEditor::reset_default()
{
    handle_ratio_change(default_ratio);
}


LRESULT ParamEditor::paint(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);

    draw_text(
        hdc,
        value_str,
        11,
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
            has_controller ? RGB(0, 0, 0) : RGB(181, 181, 189),
            has_controller ? RGB(145, 145, 151) : RGB(0, 0, 0),
            has_controller ? FW_BOLD : FW_NORMAL
        );
    }

    EndPaint(hwnd, &ps);

    return 0;
}


LRESULT ParamEditor::lbuttonup(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (is_clicking && controller_choices > 0) {
        controller_selector.show(param_id, controller_choices, this);
    }

    return Widget::lbuttonup(uMsg, wParam, lParam);
}


HBITMAP ParamEditor::knob_states = NULL;

HBITMAP ParamEditor::knob_states_inactive = NULL;


ParamEditor::Knob::Knob(
        ParamEditor& editor,
        char const* const label,
        int left,
        int top,
        Number const steps
) : Widget(
        "STATIC",
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


void ParamEditor::Knob::set_up(HINSTANCE application, Widget* parent)
{
    Widget::set_up(application, parent);

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
    HDC hdc = GetDC(hwnd);

    HDC source_hdc = CreateCompatibleDC(hdc);
    HDC destination_hdc = CreateCompatibleDC(hdc);

    HBITMAP source_bitmap = is_inactive ? knob_states_inactive : knob_states;
    HBITMAP destination_bitmap = CreateCompatibleBitmap(hdc, WIDTH, HEIGHT);

    HBITMAP old_source_bitmap = (
        (HBITMAP)SelectObject(source_hdc, (HGDIOBJ)source_bitmap)
    );
    HBITMAP old_destination_bitmap = (
        (HBITMAP)SelectObject(destination_hdc, (HGDIOBJ)destination_bitmap)
    );

    int source_x = (int)(KNOB_STATES_LAST_INDEX * this->ratio) * WIDTH;
    BitBlt(destination_hdc, 0, 0, WIDTH, HEIGHT, source_hdc, source_x, 0, SRCCOPY);

    SelectObject(source_hdc, (HGDIOBJ)old_source_bitmap);
    SelectObject(destination_hdc, (HGDIOBJ)old_destination_bitmap);

    DeleteDC(source_hdc);
    DeleteDC(destination_hdc);

    ReleaseDC(hwnd, hdc);

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
    SetCapture(hwnd);
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

    SetFocus(hwnd);

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


#define ADD_PE(owner, left, top, synth, param_id, ctls, varg1, varg2)   \
    owner->own(                                                         \
        new ParamEditor(                                                \
            GUI::PARAMS[param_id],                                      \
            left,                                                       \
            top,                                                        \
            *controller_selector,                                       \
            synth,                                                      \
            param_id,                                                   \
            ctls,                                                       \
            varg1,                                                      \
            varg2                                                       \
        )                                                               \
    )

#define PE_W ParamEditor::WIDTH
#define PE_H ParamEditor::HEIGHT


GUI::GUI(
        HINSTANCE application,
        HWND parent_window,
        Synth& synth
)
    : synth(synth),
    application(application),
    parent_window(application, parent_window)
{
    ParamEditor::knob_states = load_bitmap(TEXT("KNOBSTATES"));
    ParamEditor::knob_states_inactive = load_bitmap(TEXT("KNOBSTATESINACTIVE"));

    about_bitmap = load_bitmap(TEXT("ABOUT"));
    controllers_bitmap = load_bitmap(TEXT("CONTROLLERS"));
    effects_bitmap = load_bitmap(TEXT("EFFECTS"));
    envelopes_bitmap = load_bitmap(TEXT("ENVELOPES"));
    lfos_bitmap = load_bitmap(TEXT("LFOS"));
    synth_bitmap = load_bitmap(TEXT("SYNTH"));

    background = new Background();
    GUI::parent_window.own(background);

    background->set_bitmap(synth_bitmap);

    controller_selector = new ControllerSelector(*background, synth);

    build_about_body();
    build_controllers_body();
    build_effects_body();
    build_envelopes_body();
    build_lfos_body();
    build_synth_body();

    background->own(
        new TabSelector(
            background,
            synth_bitmap,
            synth_body,
            "Synth",
            TabSelector::LEFT
        )
    );
    background->own(
        new TabSelector(
            background,
            effects_bitmap,
            effects_body,
            "Effects",
            TabSelector::LEFT + TabSelector::WIDTH
        )
    );
    background->own(
        new TabSelector(
            background,
            controllers_bitmap,
            controllers_body,
            "Controllers",
            TabSelector::LEFT + TabSelector::WIDTH * 2
        )
    );
    background->own(
        new TabSelector(
            background,
            envelopes_bitmap,
            envelopes_body,
            "Envelopes",
            TabSelector::LEFT + TabSelector::WIDTH * 3
        )
    );
    background->own(
        new TabSelector(
            background,
            lfos_bitmap,
            lfos_body,
            "LFOs",
            TabSelector::LEFT + TabSelector::WIDTH * 4
        )
    );
    background->own(
        new TabSelector(
            background,
            about_bitmap,
            about_body,
            "About",
            TabSelector::LEFT + TabSelector::WIDTH * 5
        )
    );

    background->replace_body(synth_body);

    background->own(controller_selector);
    controller_selector->hide();
}


void GUI::build_about_body()
{
    about_body = new TabBody("About");

    background->own(about_body);

    about_body->hide();
}


void GUI::build_controllers_body()
{
    controllers_body = new TabBody("Controllers");

    background->own(controllers_body);

    ADD_PE(controllers_body,  21,             44, synth, ParamId::C1IN,     MIDI_CTLS, "%.2f", 100.0);
    ADD_PE(controllers_body,  21 + PE_W,      44, synth, ParamId::C1MIN,    MIDI_CTLS, "%.2f", 100.0);
    ADD_PE(controllers_body,  21 + PE_W * 2,  44, synth, ParamId::C1MAX,    MIDI_CTLS, "%.2f", 100.0);
    ADD_PE(controllers_body,  21,            164, synth, ParamId::C1AMT,    MIDI_CTLS, "%.2f", 100.0);
    ADD_PE(controllers_body,  21 + PE_W,     164, synth, ParamId::C1DST,    MIDI_CTLS, "%.2f", 100.0);
    ADD_PE(controllers_body,  21 + PE_W * 2, 164, synth, ParamId::C1RND,    MIDI_CTLS, "%.2f", 100.0);

    ADD_PE(controllers_body, 211,             44, synth, ParamId::C2IN,     MIDI_CTLS, "%.2f", 100.0);
    ADD_PE(controllers_body, 211 + PE_W,      44, synth, ParamId::C2MIN,    MIDI_CTLS, "%.2f", 100.0);
    ADD_PE(controllers_body, 211 + PE_W * 2,  44, synth, ParamId::C2MAX,    MIDI_CTLS, "%.2f", 100.0);
    ADD_PE(controllers_body, 211,            164, synth, ParamId::C2AMT,    MIDI_CTLS, "%.2f", 100.0);
    ADD_PE(controllers_body, 211 + PE_W,     164, synth, ParamId::C2DST,    MIDI_CTLS, "%.2f", 100.0);
    ADD_PE(controllers_body, 211 + PE_W * 2, 164, synth, ParamId::C2RND,    MIDI_CTLS, "%.2f", 100.0);

    ADD_PE(controllers_body, 401,             44, synth, ParamId::C3IN,     MIDI_CTLS, "%.2f", 100.0);
    ADD_PE(controllers_body, 401 + PE_W,      44, synth, ParamId::C3MIN,    MIDI_CTLS, "%.2f", 100.0);
    ADD_PE(controllers_body, 401 + PE_W * 2,  44, synth, ParamId::C3MAX,    MIDI_CTLS, "%.2f", 100.0);
    ADD_PE(controllers_body, 401,            164, synth, ParamId::C3AMT,    MIDI_CTLS, "%.2f", 100.0);
    ADD_PE(controllers_body, 401 + PE_W,     164, synth, ParamId::C3DST,    MIDI_CTLS, "%.2f", 100.0);
    ADD_PE(controllers_body, 401 + PE_W * 2, 164, synth, ParamId::C3RND,    MIDI_CTLS, "%.2f", 100.0);

    ADD_PE(controllers_body, 591,             44, synth, ParamId::C4IN,     MIDI_CTLS, "%.2f", 100.0);
    ADD_PE(controllers_body, 591 + PE_W,      44, synth, ParamId::C4MIN,    MIDI_CTLS, "%.2f", 100.0);
    ADD_PE(controllers_body, 591 + PE_W * 2,  44, synth, ParamId::C4MAX,    MIDI_CTLS, "%.2f", 100.0);
    ADD_PE(controllers_body, 591,            164, synth, ParamId::C4AMT,    MIDI_CTLS, "%.2f", 100.0);
    ADD_PE(controllers_body, 591 + PE_W,     164, synth, ParamId::C4DST,    MIDI_CTLS, "%.2f", 100.0);
    ADD_PE(controllers_body, 591 + PE_W * 2, 164, synth, ParamId::C4RND,    MIDI_CTLS, "%.2f", 100.0);

    ADD_PE(controllers_body, 781,             44, synth, ParamId::C5IN,     MIDI_CTLS, "%.2f", 100.0);
    ADD_PE(controllers_body, 781 + PE_W,      44, synth, ParamId::C5MIN,    MIDI_CTLS, "%.2f", 100.0);
    ADD_PE(controllers_body, 781 + PE_W * 2,  44, synth, ParamId::C5MAX,    MIDI_CTLS, "%.2f", 100.0);
    ADD_PE(controllers_body, 781,            164, synth, ParamId::C5AMT,    MIDI_CTLS, "%.2f", 100.0);
    ADD_PE(controllers_body, 781 + PE_W,     164, synth, ParamId::C5DST,    MIDI_CTLS, "%.2f", 100.0);
    ADD_PE(controllers_body, 781 + PE_W * 2, 164, synth, ParamId::C5RND,    MIDI_CTLS, "%.2f", 100.0);

    ADD_PE(controllers_body,  21,            324, synth, ParamId::C6IN,     MIDI_CTLS, "%.2f", 100.0);
    ADD_PE(controllers_body,  21 + PE_W,     324, synth, ParamId::C6MIN,    MIDI_CTLS, "%.2f", 100.0);
    ADD_PE(controllers_body,  21 + PE_W * 2, 324, synth, ParamId::C6MAX,    MIDI_CTLS, "%.2f", 100.0);
    ADD_PE(controllers_body,  21,            444, synth, ParamId::C6AMT,    MIDI_CTLS, "%.2f", 100.0);
    ADD_PE(controllers_body,  21 + PE_W,     444, synth, ParamId::C6DST,    MIDI_CTLS, "%.2f", 100.0);
    ADD_PE(controllers_body,  21 + PE_W * 2, 444, synth, ParamId::C6RND,    MIDI_CTLS, "%.2f", 100.0);

    ADD_PE(controllers_body, 211,            324, synth, ParamId::C7IN,     MIDI_CTLS, "%.2f", 100.0);
    ADD_PE(controllers_body, 211 + PE_W,     324, synth, ParamId::C7MIN,    MIDI_CTLS, "%.2f", 100.0);
    ADD_PE(controllers_body, 211 + PE_W * 2, 324, synth, ParamId::C7MAX,    MIDI_CTLS, "%.2f", 100.0);
    ADD_PE(controllers_body, 211,            444, synth, ParamId::C7AMT,    MIDI_CTLS, "%.2f", 100.0);
    ADD_PE(controllers_body, 211 + PE_W,     444, synth, ParamId::C7DST,    MIDI_CTLS, "%.2f", 100.0);
    ADD_PE(controllers_body, 211 + PE_W * 2, 444, synth, ParamId::C7RND,    MIDI_CTLS, "%.2f", 100.0);

    ADD_PE(controllers_body, 401,            324, synth, ParamId::C8IN,     MIDI_CTLS, "%.2f", 100.0);
    ADD_PE(controllers_body, 401 + PE_W,     324, synth, ParamId::C8MIN,    MIDI_CTLS, "%.2f", 100.0);
    ADD_PE(controllers_body, 401 + PE_W * 2, 324, synth, ParamId::C8MAX,    MIDI_CTLS, "%.2f", 100.0);
    ADD_PE(controllers_body, 401,            444, synth, ParamId::C8AMT,    MIDI_CTLS, "%.2f", 100.0);
    ADD_PE(controllers_body, 401 + PE_W,     444, synth, ParamId::C8DST,    MIDI_CTLS, "%.2f", 100.0);
    ADD_PE(controllers_body, 401 + PE_W * 2, 444, synth, ParamId::C8RND,    MIDI_CTLS, "%.2f", 100.0);

    ADD_PE(controllers_body, 591,            324, synth, ParamId::C9IN,     MIDI_CTLS, "%.2f", 100.0);
    ADD_PE(controllers_body, 591 + PE_W,     324, synth, ParamId::C9MIN,    MIDI_CTLS, "%.2f", 100.0);
    ADD_PE(controllers_body, 591 + PE_W * 2, 324, synth, ParamId::C9MAX,    MIDI_CTLS, "%.2f", 100.0);
    ADD_PE(controllers_body, 591,            444, synth, ParamId::C9AMT,    MIDI_CTLS, "%.2f", 100.0);
    ADD_PE(controllers_body, 591 + PE_W,     444, synth, ParamId::C9DST,    MIDI_CTLS, "%.2f", 100.0);
    ADD_PE(controllers_body, 591 + PE_W * 2, 444, synth, ParamId::C9RND,    MIDI_CTLS, "%.2f", 100.0);

    ADD_PE(controllers_body, 781,            324, synth, ParamId::C10IN,     MIDI_CTLS, "%.2f", 100.0);
    ADD_PE(controllers_body, 781 + PE_W,     324, synth, ParamId::C10MIN,    MIDI_CTLS, "%.2f", 100.0);
    ADD_PE(controllers_body, 781 + PE_W * 2, 324, synth, ParamId::C10MAX,    MIDI_CTLS, "%.2f", 100.0);
    ADD_PE(controllers_body, 781,            444, synth, ParamId::C10AMT,    MIDI_CTLS, "%.2f", 100.0);
    ADD_PE(controllers_body, 781 + PE_W,     444, synth, ParamId::C10DST,    MIDI_CTLS, "%.2f", 100.0);
    ADD_PE(controllers_body, 781 + PE_W * 2, 444, synth, ParamId::C10RND,    MIDI_CTLS, "%.2f", 100.0);

    controllers_body->hide();
}


void GUI::build_effects_body()
{
    effects_body = new TabBody("Effects");

    background->own(effects_body);

    constexpr char const* const* ft = JS80P::GUI::BIQUAD_FILTER_TYPES;
    constexpr int ftc = JS80P::GUI::BIQUAD_FILTER_TYPES_COUNT;

    ADD_PE(effects_body,  74,               57, synth, ParamId::EOG,    LFO_CTLS,   "%.2f", 100.0);
    ADD_PE(effects_body, 237,               57, synth, ParamId::EDG,    LFO_CTLS,   "%.2f", 100.0);

    ADD_PE(effects_body, 385,               57, synth, ParamId::EF1TYP, NO_CTLS,    ft, ftc);
    ADD_PE(effects_body, 385 + PE_W,        57, synth, ParamId::EF1FRQ, LFO_CTLS,   "%.1f", 1.0);
    ADD_PE(effects_body, 385 + PE_W * 2,    57, synth, ParamId::EF1Q,   LFO_CTLS,   "%.2f", 1.0);
    ADD_PE(effects_body, 385 + PE_W * 3,    57, synth, ParamId::EF1G,   LFO_CTLS,   "%.2f", 1.0);

    ADD_PE(effects_body, 690,               57, synth, ParamId::EF2TYP, NO_CTLS,    ft, ftc);
    ADD_PE(effects_body, 690 + PE_W,        57, synth, ParamId::EF2FRQ, LFO_CTLS,   "%.1f", 1.0);
    ADD_PE(effects_body, 690 + PE_W * 2,    57, synth, ParamId::EF2Q,   LFO_CTLS,   "%.2f", 1.0);
    ADD_PE(effects_body, 690 + PE_W * 3,    57, synth, ParamId::EF2G,   LFO_CTLS,   "%.2f", 1.0);

    effects_body->hide();
}


void GUI::build_envelopes_body()
{
    envelopes_body = new TabBody("Envelopes");

    background->own(envelopes_body);

    ADD_PE(envelopes_body,  37,             44, synth, ParamId::E1AMT,  MIDI_CTLS, "%.2f", 100.0);
    ADD_PE(envelopes_body,  37 + PE_W,      44, synth, ParamId::E1INI,  MIDI_CTLS, "%.2f", 100.0);
    ADD_PE(envelopes_body,  37 + PE_W * 2,  44, synth, ParamId::E1PK,   MIDI_CTLS, "%.2f", 100.0);
    ADD_PE(envelopes_body,  37 + PE_W * 3,  44, synth, ParamId::E1SUS,  MIDI_CTLS, "%.2f", 100.0);
    ADD_PE(envelopes_body,  37 + PE_W * 4,  44, synth, ParamId::E1FIN,  MIDI_CTLS, "%.2f", 100.0);
    ADD_PE(envelopes_body,  37,            164, synth, ParamId::E1DEL,  MIDI_CTLS, "%.3f", 1.0);
    ADD_PE(envelopes_body,  37 + PE_W,     164, synth, ParamId::E1ATK,  MIDI_CTLS, "%.3f", 1.0);
    ADD_PE(envelopes_body,  37 + PE_W * 2, 164, synth, ParamId::E1HLD,  MIDI_CTLS, "%.3f", 1.0);
    ADD_PE(envelopes_body,  37 + PE_W * 3, 164, synth, ParamId::E1DEC,  MIDI_CTLS, "%.3f", 1.0);
    ADD_PE(envelopes_body,  37 + PE_W * 4, 164, synth, ParamId::E1REL,  MIDI_CTLS, "%.3f", 1.0);

    ADD_PE(envelopes_body, 345,             44, synth, ParamId::E2AMT,  MIDI_CTLS, "%.2f", 100.0);
    ADD_PE(envelopes_body, 345 + PE_W,      44, synth, ParamId::E2INI,  MIDI_CTLS, "%.2f", 100.0);
    ADD_PE(envelopes_body, 345 + PE_W * 2,  44, synth, ParamId::E2PK,   MIDI_CTLS, "%.2f", 100.0);
    ADD_PE(envelopes_body, 345 + PE_W * 3,  44, synth, ParamId::E2SUS,  MIDI_CTLS, "%.2f", 100.0);
    ADD_PE(envelopes_body, 345 + PE_W * 4,  44, synth, ParamId::E2FIN,  MIDI_CTLS, "%.2f", 100.0);
    ADD_PE(envelopes_body, 345,            164, synth, ParamId::E2DEL,  MIDI_CTLS, "%.3f", 1.0);
    ADD_PE(envelopes_body, 345 + PE_W,     164, synth, ParamId::E2ATK,  MIDI_CTLS, "%.3f", 1.0);
    ADD_PE(envelopes_body, 345 + PE_W * 2, 164, synth, ParamId::E2HLD,  MIDI_CTLS, "%.3f", 1.0);
    ADD_PE(envelopes_body, 345 + PE_W * 3, 164, synth, ParamId::E2DEC,  MIDI_CTLS, "%.3f", 1.0);
    ADD_PE(envelopes_body, 345 + PE_W * 4, 164, synth, ParamId::E2REL,  MIDI_CTLS, "%.3f", 1.0);

    ADD_PE(envelopes_body, 653,             44, synth, ParamId::E3AMT,  MIDI_CTLS, "%.2f", 100.0);
    ADD_PE(envelopes_body, 653 + PE_W,      44, synth, ParamId::E3INI,  MIDI_CTLS, "%.2f", 100.0);
    ADD_PE(envelopes_body, 653 + PE_W * 2,  44, synth, ParamId::E3PK,   MIDI_CTLS, "%.2f", 100.0);
    ADD_PE(envelopes_body, 653 + PE_W * 3,  44, synth, ParamId::E3SUS,  MIDI_CTLS, "%.2f", 100.0);
    ADD_PE(envelopes_body, 653 + PE_W * 4,  44, synth, ParamId::E3FIN,  MIDI_CTLS, "%.2f", 100.0);
    ADD_PE(envelopes_body, 653,            164, synth, ParamId::E3DEL,  MIDI_CTLS, "%.3f", 1.0);
    ADD_PE(envelopes_body, 653 + PE_W,     164, synth, ParamId::E3ATK,  MIDI_CTLS, "%.3f", 1.0);
    ADD_PE(envelopes_body, 653 + PE_W * 2, 164, synth, ParamId::E3HLD,  MIDI_CTLS, "%.3f", 1.0);
    ADD_PE(envelopes_body, 653 + PE_W * 3, 164, synth, ParamId::E3DEC,  MIDI_CTLS, "%.3f", 1.0);
    ADD_PE(envelopes_body, 653 + PE_W * 4, 164, synth, ParamId::E3REL,  MIDI_CTLS, "%.3f", 1.0);

    ADD_PE(envelopes_body,  37,            326, synth, ParamId::E4AMT,  MIDI_CTLS, "%.2f", 100.0);
    ADD_PE(envelopes_body,  37 + PE_W,     326, synth, ParamId::E4INI,  MIDI_CTLS, "%.2f", 100.0);
    ADD_PE(envelopes_body,  37 + PE_W * 2, 326, synth, ParamId::E4PK,   MIDI_CTLS, "%.2f", 100.0);
    ADD_PE(envelopes_body,  37 + PE_W * 3, 326, synth, ParamId::E4SUS,  MIDI_CTLS, "%.2f", 100.0);
    ADD_PE(envelopes_body,  37 + PE_W * 4, 326, synth, ParamId::E4FIN,  MIDI_CTLS, "%.2f", 100.0);
    ADD_PE(envelopes_body,  37,            446, synth, ParamId::E4DEL,  MIDI_CTLS, "%.3f", 1.0);
    ADD_PE(envelopes_body,  37 + PE_W,     446, synth, ParamId::E4ATK,  MIDI_CTLS, "%.3f", 1.0);
    ADD_PE(envelopes_body,  37 + PE_W * 2, 446, synth, ParamId::E4HLD,  MIDI_CTLS, "%.3f", 1.0);
    ADD_PE(envelopes_body,  37 + PE_W * 3, 446, synth, ParamId::E4DEC,  MIDI_CTLS, "%.3f", 1.0);
    ADD_PE(envelopes_body,  37 + PE_W * 4, 446, synth, ParamId::E4REL,  MIDI_CTLS, "%.3f", 1.0);

    ADD_PE(envelopes_body, 345,            326, synth, ParamId::E5AMT,  MIDI_CTLS, "%.2f", 100.0);
    ADD_PE(envelopes_body, 345 + PE_W,     326, synth, ParamId::E5INI,  MIDI_CTLS, "%.2f", 100.0);
    ADD_PE(envelopes_body, 345 + PE_W * 2, 326, synth, ParamId::E5PK,   MIDI_CTLS, "%.2f", 100.0);
    ADD_PE(envelopes_body, 345 + PE_W * 3, 326, synth, ParamId::E5SUS,  MIDI_CTLS, "%.2f", 100.0);
    ADD_PE(envelopes_body, 345 + PE_W * 4, 326, synth, ParamId::E5FIN,  MIDI_CTLS, "%.2f", 100.0);
    ADD_PE(envelopes_body, 345,            446, synth, ParamId::E5DEL,  MIDI_CTLS, "%.3f", 1.0);
    ADD_PE(envelopes_body, 345 + PE_W,     446, synth, ParamId::E5ATK,  MIDI_CTLS, "%.3f", 1.0);
    ADD_PE(envelopes_body, 345 + PE_W * 2, 446, synth, ParamId::E5HLD,  MIDI_CTLS, "%.3f", 1.0);
    ADD_PE(envelopes_body, 345 + PE_W * 3, 446, synth, ParamId::E5DEC,  MIDI_CTLS, "%.3f", 1.0);
    ADD_PE(envelopes_body, 345 + PE_W * 4, 446, synth, ParamId::E5REL,  MIDI_CTLS, "%.3f", 1.0);

    ADD_PE(envelopes_body, 653,            326, synth, ParamId::E6AMT,  MIDI_CTLS, "%.2f", 100.0);
    ADD_PE(envelopes_body, 653 + PE_W,     326, synth, ParamId::E6INI,  MIDI_CTLS, "%.2f", 100.0);
    ADD_PE(envelopes_body, 653 + PE_W * 2, 326, synth, ParamId::E6PK,   MIDI_CTLS, "%.2f", 100.0);
    ADD_PE(envelopes_body, 653 + PE_W * 3, 326, synth, ParamId::E6SUS,  MIDI_CTLS, "%.2f", 100.0);
    ADD_PE(envelopes_body, 653 + PE_W * 4, 326, synth, ParamId::E6FIN,  MIDI_CTLS, "%.2f", 100.0);
    ADD_PE(envelopes_body, 653,            446, synth, ParamId::E6DEL,  MIDI_CTLS, "%.3f", 1.0);
    ADD_PE(envelopes_body, 653 + PE_W,     446, synth, ParamId::E6ATK,  MIDI_CTLS, "%.3f", 1.0);
    ADD_PE(envelopes_body, 653 + PE_W * 2, 446, synth, ParamId::E6HLD,  MIDI_CTLS, "%.3f", 1.0);
    ADD_PE(envelopes_body, 653 + PE_W * 3, 446, synth, ParamId::E6DEC,  MIDI_CTLS, "%.3f", 1.0);
    ADD_PE(envelopes_body, 653 + PE_W * 4, 446, synth, ParamId::E6REL,  MIDI_CTLS, "%.3f", 1.0);

    envelopes_body->hide();
}


void GUI::build_lfos_body()
{
    lfos_body = new TabBody("LFOs");

    background->own(lfos_body);

    lfos_body->hide();
}


void GUI::build_synth_body()
{
    synth_body = new TabBody("Synth");

    background->own(synth_body);

    constexpr char const* const* wf = JS80P::GUI::WAVEFORMS;
    constexpr int wfc = JS80P::GUI::WAVEFORMS_COUNT;
    constexpr char const* const* ft = JS80P::GUI::BIQUAD_FILTER_TYPES;
    constexpr int ftc = JS80P::GUI::BIQUAD_FILTER_TYPES_COUNT;

    ADD_PE(synth_body,  12,  64,                    synth, ParamId::VOL,    LFO_CTLS,   "%.2f", 100.0);
    ADD_PE(synth_body,  12,  64 + (PE_H + 10),      synth, ParamId::ADD,    LFO_CTLS,   "%.2f", 100.0);
    ADD_PE(synth_body,  12,  64 + (PE_H + 10) * 2,  synth, ParamId::FM,     ALL_CTLS,   "%.2f", 100.0 / Constants::FM_MAX);
    ADD_PE(synth_body,  12,  64 + (PE_H + 10) * 3,  synth, ParamId::AM,     ALL_CTLS,   "%.2f", 100.0 / Constants::AM_MAX);

    ADD_PE(synth_body,  87,              36,        synth, ParamId::MWAV,   NO_CTLS,    wf, wfc);
    ADD_PE(synth_body,  87 + PE_W * 1,   36,        synth, ParamId::MPRT,   MIDI_CTLS,  "%.3f", 1.0);
    ADD_PE(synth_body,  87 + PE_W * 2,   36,        synth, ParamId::MPRD,   MIDI_CTLS,  "%.2f", 1.0);
    ADD_PE(synth_body,  87 + PE_W * 3,   36,        synth, ParamId::MDTN,   MIDI_CTLS,  "%.f", 0.01);
    ADD_PE(synth_body,  87 + PE_W * 4,   36,        synth, ParamId::MFIN,   ALL_CTLS,   "%.2f", 1.0);
    ADD_PE(synth_body,  87 + PE_W * 5,   36,        synth, ParamId::MAMP,   ALL_CTLS,   "%.2f", 100.0);
    ADD_PE(synth_body,  87 + PE_W * 6,   36,        synth, ParamId::MFLD,   ALL_CTLS,   "%.2f", 100.0 / Constants::FOLD_MAX);
    ADD_PE(synth_body,  87 + PE_W * 7,   36,        synth, ParamId::MVS,    MIDI_CTLS,  "%.2f", 100.0);
    ADD_PE(synth_body,  87 + PE_W * 8,   36,        synth, ParamId::MVOL,   ALL_CTLS,   "%.2f", 100.0);

    ADD_PE(synth_body, 735,              36,        synth, ParamId::MF1TYP, NO_CTLS,    ft, ftc);
    ADD_PE(synth_body, 735 + PE_W,       36,        synth, ParamId::MF1FRQ, ALL_CTLS,   "%.1f", 1.0);
    ADD_PE(synth_body, 735 + PE_W * 2,   36,        synth, ParamId::MF1Q,   ALL_CTLS,   "%.2f", 1.0);
    ADD_PE(synth_body, 735 + PE_W * 3,   36,        synth, ParamId::MF1G,   ALL_CTLS,   "%.2f", 1.0);

    ADD_PE(synth_body, 116,             168,        synth, ParamId::MC1,    MIDI_CTLS,  "%.2f", 100.0);
    ADD_PE(synth_body, 116 + PE_W * 1,  168,        synth, ParamId::MC2,    MIDI_CTLS,  "%.2f", 100.0);
    ADD_PE(synth_body, 116 + PE_W * 2,  168,        synth, ParamId::MC3,    MIDI_CTLS,  "%.2f", 100.0);
    ADD_PE(synth_body, 116 + PE_W * 3,  168,        synth, ParamId::MC4,    MIDI_CTLS,  "%.2f", 100.0);
    ADD_PE(synth_body, 116 + PE_W * 4,  168,        synth, ParamId::MC5,    MIDI_CTLS,  "%.2f", 100.0);
    ADD_PE(synth_body, 116 + PE_W * 5,  168,        synth, ParamId::MC6,    MIDI_CTLS,  "%.2f", 100.0);
    ADD_PE(synth_body, 116 + PE_W * 6,  168,        synth, ParamId::MC7,    MIDI_CTLS,  "%.2f", 100.0);
    ADD_PE(synth_body, 116 + PE_W * 7,  168,        synth, ParamId::MC8,    MIDI_CTLS,  "%.2f", 100.0);
    ADD_PE(synth_body, 116 + PE_W * 8,  168,        synth, ParamId::MC9,    MIDI_CTLS,  "%.2f", 100.0);
    ADD_PE(synth_body, 116 + PE_W * 9,  168,        synth, ParamId::MC10,   MIDI_CTLS,  "%.2f", 100.0);

    ADD_PE(synth_body, 735,             168,        synth, ParamId::MF2TYP, NO_CTLS,    ft, ftc);
    ADD_PE(synth_body, 735 + PE_W,      168,        synth, ParamId::MF2FRQ, ALL_CTLS,   "%.1f", 1.0);
    ADD_PE(synth_body, 735 + PE_W * 2,  168,        synth, ParamId::MF2Q,   ALL_CTLS,   "%.2f", 1.0);
    ADD_PE(synth_body, 735 + PE_W * 3,  168,        synth, ParamId::MF2G,   ALL_CTLS,   "%.2f", 1.0);

    ADD_PE(synth_body,  87,             316,        synth, ParamId::CWAV,   NO_CTLS,    wf, wfc);
    ADD_PE(synth_body,  87 + PE_W * 1,  316,        synth, ParamId::CPRT,   MIDI_CTLS,  "%.3f", 1.0);
    ADD_PE(synth_body,  87 + PE_W * 2,  316,        synth, ParamId::CPRD,   MIDI_CTLS,  "%.2f", 1.0);
    ADD_PE(synth_body,  87 + PE_W * 3,  316,        synth, ParamId::CDTN,   MIDI_CTLS,  "%.f", 0.01);
    ADD_PE(synth_body,  87 + PE_W * 4,  316,        synth, ParamId::CFIN,   ALL_CTLS,   "%.2f", 1.0);
    ADD_PE(synth_body,  87 + PE_W * 5,  316,        synth, ParamId::CAMP,   ALL_CTLS,   "%.2f", 100.0);
    ADD_PE(synth_body,  87 + PE_W * 6,  316,        synth, ParamId::CFLD,   ALL_CTLS,   "%.2f", 100.0 / Constants::FOLD_MAX);
    ADD_PE(synth_body,  87 + PE_W * 7,  316,        synth, ParamId::CVS,    MIDI_CTLS,  "%.2f", 100.0);
    ADD_PE(synth_body,  87 + PE_W * 8,  316,        synth, ParamId::CVOL,   ALL_CTLS,   "%.2f", 100.0);

    ADD_PE(synth_body, 735,             316,        synth, ParamId::CF1TYP, NO_CTLS,    ft, ftc);
    ADD_PE(synth_body, 735 + PE_W,      316,        synth, ParamId::CF1FRQ, ALL_CTLS,   "%.1f", 1.0);
    ADD_PE(synth_body, 735 + PE_W * 2,  316,        synth, ParamId::CF1Q,   ALL_CTLS,   "%.2f", 1.0);
    ADD_PE(synth_body, 735 + PE_W * 3,  316,        synth, ParamId::CF1G,   ALL_CTLS,   "%.2f", 1.0);

    ADD_PE(synth_body, 116,             448,        synth, ParamId::CC1,    MIDI_CTLS,  "%.2f", 100.0);
    ADD_PE(synth_body, 116 + PE_W * 1,  448,        synth, ParamId::CC2,    MIDI_CTLS,  "%.2f", 100.0);
    ADD_PE(synth_body, 116 + PE_W * 2,  448,        synth, ParamId::CC3,    MIDI_CTLS,  "%.2f", 100.0);
    ADD_PE(synth_body, 116 + PE_W * 3,  448,        synth, ParamId::CC4,    MIDI_CTLS,  "%.2f", 100.0);
    ADD_PE(synth_body, 116 + PE_W * 4,  448,        synth, ParamId::CC5,    MIDI_CTLS,  "%.2f", 100.0);
    ADD_PE(synth_body, 116 + PE_W * 5,  448,        synth, ParamId::CC6,    MIDI_CTLS,  "%.2f", 100.0);
    ADD_PE(synth_body, 116 + PE_W * 6,  448,        synth, ParamId::CC7,    MIDI_CTLS,  "%.2f", 100.0);
    ADD_PE(synth_body, 116 + PE_W * 7,  448,        synth, ParamId::CC8,    MIDI_CTLS,  "%.2f", 100.0);
    ADD_PE(synth_body, 116 + PE_W * 8,  448,        synth, ParamId::CC9,    MIDI_CTLS,  "%.2f", 100.0);
    ADD_PE(synth_body, 116 + PE_W * 9,  448,        synth, ParamId::CC10,   MIDI_CTLS,  "%.2f", 100.0);

    ADD_PE(synth_body, 735,             448,        synth, ParamId::CF2TYP, NO_CTLS,    ft, ftc);
    ADD_PE(synth_body, 735 + PE_W,      448,        synth, ParamId::CF2FRQ, ALL_CTLS,   "%.1f", 1.0);
    ADD_PE(synth_body, 735 + PE_W * 2,  448,        synth, ParamId::CF2Q,   ALL_CTLS,   "%.2f", 1.0);
    ADD_PE(synth_body, 735 + PE_W * 3,  448,        synth, ParamId::CF2G,   ALL_CTLS,   "%.2f", 1.0);

    synth_body->show();
}


HBITMAP GUI::load_bitmap(LPCTSTR name)
{
    // TODO: GetLastError()
    HBITMAP bitmap = (HBITMAP)LoadImage(
        application, name, IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION
    );

    return bitmap;
}


GUI::~GUI()
{
    HBITMAP knob_states_bitmap = ParamEditor::knob_states;
    ParamEditor::knob_states = NULL;
    DeleteObject((HGDIOBJ)knob_states_bitmap);

    knob_states_bitmap = ParamEditor::knob_states_inactive;
    ParamEditor::knob_states_inactive = NULL;
    DeleteObject((HGDIOBJ)knob_states_bitmap);

    DeleteObject((HGDIOBJ)about_bitmap);
    DeleteObject((HGDIOBJ)controllers_bitmap);
    DeleteObject((HGDIOBJ)effects_bitmap);
    DeleteObject((HGDIOBJ)envelopes_bitmap);
    DeleteObject((HGDIOBJ)lfos_bitmap);
    DeleteObject((HGDIOBJ)synth_bitmap);
}


void GUI::show()
{
    background->show();
}

} }
