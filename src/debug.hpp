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

#ifndef JS80P__DEBUG_HPP
#define JS80P__DEBUG_HPP

#ifndef JS80P_DEBUG_LOG

#define JS80P_DEBUG(message_template, ...)
#define JS80P_DEBUG_ARRAY(message, arr, len, format)

#endif

#ifdef JS80P_DEBUG_LOG

#include <cstdio>
#include <cstring>

#include "js80p.hpp"


#ifdef _WIN32
#include <windows.h>
#define _JS80P_GET_TID() ((unsigned int)GetCurrentThreadId())
#else
#include <sys/types.h>
#define _JS80P_GET_TID() ((unsigned int)gettid())
#endif


#define _JS80P_DEBUG_CTX(debug_action) do {                                 \
    constexpr char const* last_slash = strrchr(__FILE__, '/');              \
    constexpr char const* basename = (                                      \
        last_slash != NULL ? last_slash + 1 : __FILE__                      \
    );                                                                      \
    FILE* _js80p_f = fopen(JS80P_TO_STRING(JS80P_DEBUG_LOG), "a+");         \
                                                                            \
    if (_js80p_f) {                                                         \
        fprintf(                                                            \
            _js80p_f,                                                       \
            "%s:%d/%s():\tTID=%x\t",                                        \
            basename,                                                       \
            __LINE__,                                                       \
            __FUNCTION__,                                                   \
            _JS80P_GET_TID()                                                \
        );                                                                  \
                                                                            \
        debug_action;                                                       \
                                                                            \
        fprintf(_js80p_f, "\n");                                            \
        fclose(_js80p_f);                                                   \
    }                                                                       \
} while (false)


#define JS80P_DEBUG(message_template, ...) do {                             \
    _JS80P_DEBUG_CTX(                                                       \
        do {                                                                \
            fprintf(_js80p_f, (message_template), ## __VA_ARGS__);          \
        } while (false)                                                     \
    );                                                                      \
} while (false)


#define JS80P_DEBUG_ARRAY(message, array, length, format_string)            \
    _JS80P_DEBUG_CTX(                                                       \
        if ((array) == NULL) {                                              \
            fprintf(__f, "%s: <NULL>", (message));                          \
        } else {                                                            \
            int _js80p_i;                                                   \
                                                                            \
            fprintf(__f, "%s: [ ", (message));                              \
                                                                            \
            for (_js80p_i = 0; _js80p_i < (length); ++_js80p_i) {           \
                if (_js80p_i > 0) {                                         \
                    fprintf(__f, ", ");                                     \
                }                                                           \
                                                                            \
                fprintf(__f, (format_string), (array)[_js80p_i]);           \
            }                                                               \
                                                                            \
            fprintf(__f, " ]");                                             \
        }                                                                   \
    )

#endif

#endif
