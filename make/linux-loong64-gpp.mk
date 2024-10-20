###############################################################################
# This file is part of JS80P, a synthesizer plugin.
# Copyright (C) 2024  @YHStar (https://github.com/YHStar)
#
# JS80P is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# JS80P is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.
###############################################################################

INSTRUCTION_SET = lsx

LIB_PATH ?= $(BUILD_DIR)/lib64
SYS_LIB_PATH ?= /usr/lib/loong64-linux-gnu
SUFFIX = loong64
OBJ_GUI_BFDNAME = elf64-loongarch
OBJ_GUI_BFDARCH = loongarch64
ARCH_CXXFLAGS = -march=loongarch64 -mabi=lp64d
ARCH_LFLAGS = -L$(LIB_PATH)

include make/linux-gpp.mk
