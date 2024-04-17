###############################################################################
# This file is part of JS80P, a synthesizer plugin.
# Copyright (C) 2024  Attila M. Magyar
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

RM ?= rm -f
MKDIR ?= mkdir
COPY ?= cp -v

CPP_DEV_PLATFORM ?= /usr/bin/g++
CPPCHECK ?= /usr/bin/cppcheck
DOXYGEN ?= /usr/bin/doxygen
VALGRIND ?= /usr/bin/valgrind

PDFLATEX ?= /usr/bin/pdflatex

PDFLATEX_FLAGS = \
	-halt-on-error \
	-file-line-error \
	-output-directory $(DEV_DIR)

LINK_DEV_EXE = $(CPP_DEV_PLATFORM) -Wall

DEV_EXE =

CPPCHECK_FLAGS = \
	--enable=all \
	--error-exitcode=1 \
	--force \
	--inline-suppr \
	--quiet \
	--suppressions-list=.cppcheck \
	-I./src \
	-I./tests

VALGRIND_FLAGS = --error-exitcode=99 --track-origins=yes --leak-check=yes --quiet

doc/pm-fm-equivalence.pdf: $(DEV_DIR)/pm-fm-equivalence.pdf
	$(COPY) $< $@

TEX_ARTIFACTS = \
	$(DEV_DIR)/pm-fm-equivalence.aux \
	$(DEV_DIR)/pm-fm-equivalence.log \
	$(DEV_DIR)/pm-fm-equivalence.out \
	$(DEV_DIR)/pm-fm-equivalence.pdf \
	$(DEV_DIR)/pm-fm-equivalence.toc

$(DEV_DIR)/pm-fm-equivalence.pdf: doc/pm-fm-equivalence.tex | $(DEV_DIR)
	$(PDFLATEX) $(PDFLATEX_FLAGS) $<
	$(PDFLATEX) $(PDFLATEX_FLAGS) $<
