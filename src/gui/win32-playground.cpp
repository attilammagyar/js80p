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

#include <cstdio>

#include "gui/win32.hpp"


JS80P::Synth synth;
JS80P::Integer rendering_round = 0;


LRESULT window_procedure(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg) {
        case WM_DESTROY:
            PostQuitMessage(0);

            return 0;

        case WM_TIMER:
            ++rendering_round;
            rendering_round = rendering_round & 0x7fff;
            synth.generate_samples(rendering_round, 1);

            return 0;
    }

    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}


int WINAPI wWinMain(
        HINSTANCE hInstance,
        HINSTANCE hPrevInstance,
        PWSTR pCmdLine,
        int nCmdShow
) {
    WNDCLASS window_class;
    LPCTSTR class_name = TEXT("JS80PGUIPlayground");

    window_class.style = CS_DBLCLKS;
    window_class.lpfnWndProc = window_procedure;
    window_class.cbClsExtra = 0;
    window_class.cbWndExtra = 0;
    window_class.hInstance = hInstance;
    window_class.hIcon = NULL;
    window_class.hCursor = (HCURSOR)LoadImage(
        NULL, MAKEINTRESOURCE(OCR_NORMAL), IMAGE_CURSOR, 0, 0, LR_DEFAULTCOLOR
    );
    window_class.hbrBackground = (HBRUSH)COLOR_BACKGROUND;
    window_class.lpszMenuName = NULL;
    window_class.lpszClassName = class_name;

    if (0 == RegisterClass(&window_class)) {
        fprintf(stderr, "ERROR: RegisterClass(); code=%lu\n", GetLastError());
        return 1;
    }

    HWND main_window = CreateWindow(
        class_name,
        TEXT("JS80P GUI Playground"),
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        JS80P::GUI::WIDTH + 32,
        JS80P::GUI::HEIGHT + 32,
        NULL,
        NULL,
        hInstance,
        NULL
    );

    if (main_window == NULL) {
        fprintf(stderr, "ERROR: CreateWindow(); code=%lu\n", GetLastError());
        return 2;
    }

    SetWindowLongPtr(main_window, GWLP_USERDATA, (LONG_PTR)main_window);

    UINT_PTR timer_id = 1;
    SetTimer(main_window, timer_id, 100, NULL);

    JS80P::GUI* gui = new JS80P::GUI(
        (JS80P::GUI::PlatformData)hInstance,
        (JS80P::GUI::PlatformWidget)main_window,
        &synth,
        true
    );
    gui->show();

    ShowWindow(main_window, nCmdShow);

    MSG message;

    while (GetMessage(&message, NULL, 0, 0))
    {
        TranslateMessage(&message);
        DispatchMessage(&message);
    }

    KillTimer(main_window, timer_id);

    delete gui;

    return message.message;
}
