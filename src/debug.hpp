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
#define _JS80P_TID_FMT "\tTID=%#x"
#elif __linux__ || __gnu_linux__
#include <unistd.h>
#include <sys/syscall.h>
#define _JS80P_GET_TID() ((unsigned int)syscall(SYS_gettid))
#define _JS80P_TID_FMT "\tTID=%#x"
#else
#define _JS80P_GET_TID() ("")
#define _JS80P_TID_FMT "%s"
#endif


#define _JS80P_DEBUG_CTX(debug_action) do {                                 \
    char const* const _js80p_last_slash = strrchr(__FILE__, '/');           \
    char const* const _js80p_basename = (                                   \
        _js80p_last_slash != NULL ? _js80p_last_slash + 1 : __FILE__        \
    );                                                                      \
    bool const _js80p_use_stderr = (                                        \
        strncmp("STDERR", JS80P_TO_STRING(JS80P_DEBUG_LOG), 7) == 0         \
    );                                                                      \
    FILE* _js80p_f = (                                                      \
        _js80p_use_stderr                                                   \
            ? stderr                                                        \
            : fopen(JS80P_TO_STRING(JS80P_DEBUG_LOG), "a+")                 \
    );                                                                      \
                                                                            \
    if (_js80p_f) {                                                         \
        fprintf(                                                            \
            _js80p_f,                                                       \
            "%s:%d/%s():" _JS80P_TID_FMT "\t",                              \
            _js80p_basename,                                                \
            __LINE__,                                                       \
            __FUNCTION__,                                                   \
            _JS80P_GET_TID()                                                \
        );                                                                  \
                                                                            \
        debug_action;                                                       \
                                                                            \
        fprintf(_js80p_f, "\n");                                            \
                                                                            \
        if (!_js80p_use_stderr) {                                           \
            fclose(_js80p_f);                                               \
        }                                                                   \
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
            fprintf(_js80p_f, "%s: <NULL>", (message));                     \
        } else {                                                            \
            int _js80p_i;                                                   \
                                                                            \
            fprintf(_js80p_f, "%s: [ ", (message));                         \
                                                                            \
            for (_js80p_i = 0; _js80p_i < (length); ++_js80p_i) {           \
                if (_js80p_i > 0) {                                         \
                    fprintf(_js80p_f, ", ");                                \
                }                                                           \
                                                                            \
                fprintf(_js80p_f, (format_string), (array)[_js80p_i]);      \
            }                                                               \
                                                                            \
            fprintf(_js80p_f, " ]");                                        \
        }                                                                   \
    )

#endif

#endif
