LIB_PATH ?= $(BUILD_DIR)/lib64
SYS_LIB_PATH ?= /usr/lib/x86_64-linux-gnu
SUFFIX = 64bit
OBJ_GUI_BFDNAME = elf64-x86-64
OBJ_GUI_BFDARCH = i386:x86-64
ARCH_CXXFLAGS = -m64
ARCH_LFLAGS = -m64 -L$(LIB_PATH)

include make/linux-gpp.mk
