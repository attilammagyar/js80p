INSTRUCTION_SET = none

LIB_PATH ?= $(BUILD_DIR)/lib64
SYS_LIB_PATH ?= /usr/lib/riscv64-linux-gnu
SUFFIX = riscv64
OBJ_GUI_BFDNAME = elf64-littleriscv
OBJ_GUI_BFDARCH = riscv
ARCH_CXXFLAGS = -march=rv64gc -mabi=lp64d
ARCH_LFLAGS = -L$(LIB_PATH)

include make/linux-gpp.mk
