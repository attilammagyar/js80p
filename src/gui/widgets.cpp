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

#ifndef JS80P__GUI__WIDGETS_CPP
#define JS80P__GUI__WIDGETS_CPP

#include <cstdio>
#include <cstring>

#include "gui/widgets.hpp"


namespace JS80P
{

ExternallyCreatedWindow::ExternallyCreatedWindow(
        GUI::PlatformData platform_data,
        GUI::PlatformWidget window
) : Widget(platform_data, window, Type::EXTERNALLY_CREATED_WINDOW)
{
}


ExternallyCreatedWindow::~ExternallyCreatedWindow()
{
    platform_widget = NULL;
}


TransparentWidget::TransparentWidget(
        char const* const label,
        int const left,
        int const top,
        int const width,
        int const height,
        Type const type
) : Widget(label, left, top, width, height, type)
{
}


bool TransparentWidget::paint()
{
    return true;
}


ImportPatchButton::ImportPatchButton(
        int const left,
        int const top,
        int const width,
        int const height,
        Synth& synth,
        TabBody* synth_gui_body
) : TransparentWidget("Import Patch", left, top, width, height, Type::IMPORT_PATCH_BUTTON),
    synth(synth),
    synth_gui_body(synth_gui_body)
{
}


ExportPatchButton::ExportPatchButton(
        int const left,
        int const top,
        int const width,
        int const height,
        Synth& synth
) : TransparentWidget("Export Patch", left, top, width, height, Type::EXPORT_PATCH_BUTTON),
    synth(synth)
{
}


TabBody::TabBody(char const* const label)
    : TransparentWidget(label, LEFT, TOP, WIDTH, HEIGHT, Type::TAB_BODY)
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
    : Widget("JS80P", 0, 0, GUI::WIDTH, GUI::HEIGHT, Type::BACKGROUND),
    body(NULL),
    next_full_refresh(FULL_REFRESH_TICKS)
{
}


Background::~Background()
{
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


void Background::set_up(GUI::PlatformData platform_data, WidgetBase* parent)
{
    Widget::set_up(platform_data, parent);

    start_timer(REFRESH_RATE);
}


bool Background::timer_tick()
{
    Widget::timer_tick();

    if (body == NULL) {
        return true;
    }

    --next_full_refresh;

    if (next_full_refresh == 0) {
        next_full_refresh = FULL_REFRESH_TICKS;
        body->refresh_param_editors();
    } else {
        body->refresh_controlled_param_editors();
    }

    return true;
}


TabSelector::TabSelector(
        Background* background,
        GUI::Bitmap bitmap,
        TabBody* tab_body,
        char const* const label,
        int const left
) : TransparentWidget(label, left, TOP, WIDTH, HEIGHT, Type::TAB_SELECTOR),
    background(background),
    tab_body(tab_body),
    bitmap(bitmap)
{
}


void TabSelector::click()
{
    TransparentWidget::click();

    background->set_bitmap(bitmap);
    background->replace_body(tab_body);
}


ControllerSelector::ControllerSelector(
        Background& background,
        Synth& synth
) : Widget("Select controller", LEFT, TOP, WIDTH, HEIGHT, Type::CONTROLLER_SELECTOR),
    background(background),
    synth(synth),
    param_editor(NULL),
    param_id(Synth::ParamId::MAX_PARAM_ID),
    selected_controller_id(Synth::ControllerId::MAX_CONTROLLER_ID)
{
}


void ControllerSelector::set_up(GUI::PlatformData platform_data, WidgetBase* parent)
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

    snprintf(
        title, TITLE_SIZE, "Select controller for \"%s\"", GUI::PARAMS[param_id]
    );

    this->param_id = param_id;
    this->param_editor = param_editor;
    this->selected_controller_id = selected_controller_id;

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
    bring_to_top();
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


bool ControllerSelector::paint()
{
    Widget::paint();

    fill_rectangle(0, 0, width, height, GUI::TEXT_BACKGROUND);
    draw_text(
        title,
        12,
        0,
        0,
        WIDTH,
        TITLE_HEIGHT,
        GUI::TEXT_COLOR,
        GUI::TEXT_BACKGROUND,
        FontWeight::BOLD,
        10,
        TextAlignment::LEFT
    );

    return true;
}


ControllerSelector::Controller::Controller(
        ControllerSelector& controller_selector,
        char const* const label,
        int const left,
        int const top,
        Synth::ControllerId const controller_id
) : Widget(label, left, top, WIDTH, HEIGHT, Type::CONTROLLER),
    label(label),
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


bool ControllerSelector::Controller::paint()
{
    Widget::paint();

    GUI::Color background;
    GUI::Color color;

    if (is_selected) {
        background = GUI::TEXT_COLOR;
        color = GUI::TEXT_BACKGROUND;
    } else if (is_mouse_over) {
        background = GUI::TEXT_HIGHLIGHT_BACKGROUND;
        color = GUI::TEXT_HIGHLIGHT_COLOR;
    } else {
        background = GUI::TEXT_BACKGROUND;
        color = GUI::TEXT_COLOR;
    }

    draw_text(
        label,
        12,
        0,
        0,
        width,
        height,
        color,
        background,
        FontWeight::BOLD,
        10,
        TextAlignment::LEFT
    );

    return true;
}


bool ControllerSelector::Controller::mouse_up(int const x, int const y)
{
    Widget::mouse_up(x, y);

    controller_selector.handle_selection_change(controller_id);

    return true;
}


bool ControllerSelector::Controller::mouse_move(
        int const x,
        int const y,
        bool const modifier
) {
    Widget::mouse_move(x, y, modifier);

    if (!is_mouse_over) {
        is_mouse_over = true;
        redraw();
    }

    return true;
}


bool ControllerSelector::Controller::mouse_leave(int const x, int const y)
{
    Widget::mouse_leave(x, y);

    if (is_mouse_over) {
        is_mouse_over = false;
        redraw();
    }

    return 0;
}


bool ParamEditor::knob_states_initialization_complete = false;

GUI::Bitmap ParamEditor::knob_states_active_bitmap = NULL;

GUI::Bitmap ParamEditor::knob_states_inactive_bitmap = NULL;

GUI::Bitmap ParamEditor::knob_states_active[KNOB_STATES_COUNT];

GUI::Bitmap ParamEditor::knob_states_inactive[KNOB_STATES_COUNT];


void ParamEditor::initialize_knob_states(
    WidgetBase* widget,
    GUI::Bitmap active,
    GUI::Bitmap inactive
) {
    if (knob_states_active_bitmap != NULL) {
        free_knob_states(widget);
    }

    knob_states_active_bitmap = active;
    knob_states_inactive_bitmap = inactive;

    knob_states_initialization_complete = false;
}


void ParamEditor::free_knob_states(WidgetBase* widget)
{
    if (knob_states_active_bitmap == NULL) {
        return;
    }

    widget->delete_bitmap(knob_states_active_bitmap);
    widget->delete_bitmap(knob_states_inactive_bitmap);

    for (int i = 0; i != KNOB_STATES_COUNT; ++i) {
        widget->delete_bitmap(knob_states_active[i]);
        widget->delete_bitmap(knob_states_inactive[i]);
    }

    knob_states_initialization_complete = false;
    knob_states_active_bitmap = NULL;
    knob_states_inactive_bitmap = NULL;
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
) : TransparentWidget(label, left, top, WIDTH, HEIGHT, Type::PARAM_EDITOR),
    param_id(param_id),
    format(format),
    label(label),
    scale(scale),
    options(NULL),
    number_of_options(0),
    value_font_size(11),
    controller_choices(controller_choices),
    controller_selector(controller_selector),
    synth(synth),
    ratio(0.0),
    knob(NULL),
    skip_refresh_calls(0),
    has_controller_(false)
{
    complete_knob_state_initialization();
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
) : TransparentWidget(label, left, top, WIDTH, HEIGHT, Type::PARAM_EDITOR),
    param_id(param_id),
    format(NULL),
    label(label),
    scale(1.0),
    options(options),
    number_of_options(number_of_options),
    value_font_size(10),
    controller_choices(controller_choices),
    controller_selector(controller_selector),
    synth(synth),
    ratio(0.0),
    knob(NULL),
    skip_refresh_calls(0),
    controller_id(Synth::ControllerId::NONE),
    has_controller_(false)
{
    complete_knob_state_initialization();
}


void ParamEditor::complete_knob_state_initialization()
{
    if (knob_states_initialization_complete) {
        return;
    }

    for (int i = 0; i != KNOB_STATES_COUNT; ++i) {
        int const left = i * Knob::WIDTH;

        knob_states_active[i] = copy_bitmap_region(
            knob_states_active_bitmap, left, 0, Knob::WIDTH, Knob::HEIGHT
        );
        knob_states_inactive[i] = copy_bitmap_region(
            knob_states_inactive_bitmap, left, 0, Knob::WIDTH, Knob::HEIGHT
        );
    }

    knob_states_initialization_complete = true;
}


void ParamEditor::set_up(GUI::PlatformData platform_data, WidgetBase* parent)
{
    TransparentWidget::set_up(platform_data, parent);

    knob = new Knob(
        *this,
        label,
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
    if (skip_refresh_calls > 0) {
        --skip_refresh_calls;

        return;
    }

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

    skip_refresh_calls = 2;

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
    GUI::param_ratio_to_str(
        synth,
        param_id,
        ratio,
        scale,
        format,
        options,
        number_of_options,
        value_str,
        TEXT_MAX_LENGTH
    );
}


void ParamEditor::update_controller_str()
{
    constexpr size_t last_index = TEXT_MAX_LENGTH - 1;

    strncpy(
        controller_str, GUI::get_controller(controller_id)->short_name, last_index
    );
    controller_str[last_index] = '\x00';
}


void ParamEditor::reset_default()
{
    handle_ratio_change(default_ratio);
}


bool ParamEditor::paint()
{
    TransparentWidget::paint();

    draw_text(
        value_str,
        value_font_size,
        1,
        HEIGHT - 20,
        WIDTH - 2,
        20,
        GUI::TEXT_COLOR,
        GUI::TEXT_BACKGROUND
    );

    if (controller_choices > 0) {
        draw_text(
            controller_str,
            10,
            1,
            HEIGHT - 36,
            WIDTH - 2,
            16,
            has_controller_ ? GUI::TEXT_BACKGROUND : GUI::TEXT_COLOR,
            has_controller_ ? GUI::rgb(145, 145, 151) : GUI::TEXT_BACKGROUND,
            has_controller_ ? FontWeight::BOLD : FontWeight::NORMAL
        );
    }

    return true;
}


bool ParamEditor::mouse_up(int const x, int const y)
{
    TransparentWidget::mouse_up(x, y);

    if (is_clicking && controller_choices > 0) {
        controller_selector.show(param_id, controller_choices, this);
    }

    return false;
}


ParamEditor::Knob::Knob(
        ParamEditor& editor,
        char const* const label,
        int const left,
        int const top,
        Number const steps
) : Widget(label, left, top, WIDTH, HEIGHT, Type::KNOB),
    steps(steps),
    editor(editor),
    knob_state(NULL),
    ratio(0.0),
    is_inactive(false)
{
}


ParamEditor::Knob::~Knob()
{
}


void ParamEditor::Knob::set_up(
        GUI::PlatformData platform_data,
        WidgetBase* parent
) {
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
    int const index = (int)(KNOB_STATES_LAST_INDEX * this->ratio);

    if (is_inactive) {
        set_bitmap(knob_states_inactive[index]);
    } else {
        set_bitmap(knob_states_active[index]);
    }
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


bool ParamEditor::Knob::double_click()
{
    Widget::double_click();

    if (is_inactive) {
        return true;
    }

    if (!is_clicking) {
        editor.reset_default();
    }

    return true;
}


bool ParamEditor::Knob::mouse_down(int const x, int const y)
{
    Widget::mouse_down(x, y);

    if (is_inactive) {
        return true;
    }

    prev_x = (Number)x;
    prev_y = (Number)y;

    return true;
}


bool ParamEditor::Knob::mouse_up(int const x, int const y)
{
    Widget::mouse_up(x, y);

    if (is_inactive) {
        return true;
    }

    focus();

    return true;
}


bool ParamEditor::Knob::mouse_move(
        int const x,
        int const y,
        bool const modifier
) {
    Widget::mouse_move(x, y, modifier);

    if (is_inactive) {
        return true;
    }

    if (is_clicking) {
        Number const scale = (
            modifier ? MOUSE_MOVE_FINE_SCALE : MOUSE_MOVE_COARSE_SCALE
        );
        Number const float_x = (Number)x;
        Number const float_y = (Number)y;
        Number const dx = float_x - prev_x;
        Number const dy = float_y - prev_y;
        Number const delta = (
            scale * ((std::fabs(dx) > std::fabs(dy)) ? dx : -dy)
        );

        prev_x = float_x;
        prev_y = float_y;
        editor.adjust_ratio(delta);
    }

    focus();

    return true;
}


bool ParamEditor::Knob::mouse_wheel(Number const delta, bool const modifier)
{
    Widget::mouse_wheel(delta, modifier);

    if (is_inactive) {
        return true;
    }

    Number const scale = (
        modifier ? MOUSE_WHEEL_FINE_SCALE : MOUSE_WHEEL_COARSE_SCALE
    );

    if (steps > 0.0) {
        editor.adjust_ratio(delta * scale * 10.0);
    } else {
        editor.adjust_ratio(delta * scale);
    }

    return true;
}

}

#endif
