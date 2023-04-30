LIB_PATH ?= $(BUILD_DIR)/lib32
SYS_LIB_PATH ?= /usr/lib/i386-linux-gnu
SUFFIX = 32bit
OBJ_GUI_BFDNAME = elf32-i386
OBJ_GUI_BFDARCH = i386
ARCH_CXXFLAGS = -m32
ARCH_LFLAGS = -m32 -L$(LIB_PATH)

include make/linux-gpp.mk
