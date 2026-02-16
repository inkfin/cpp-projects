# ============================================================
# User Configuration
# ============================================================

CC := clang

SRC_DIRS := .
INCLUDE_DIR := include
TARGET_NAME := app

CFLAGS := -std=c99 -Wall -Wextra \
		  -Wno-unused-function
CFLAGS_DEBUG := -O0 -g -ggdb \
				-fsanitize=address,undefined \
			    -fno-omit-frame-pointer \
				-fstack-protector-strong \
				-fno-common
CFLAGS_RELEASE := -O2 -DNDEBUG

LDFLAGS :=
LDFLAGS_DEBUG := -fsanitize=address,undefined

LDLIBS :=


# ============================================================
# Internal Build Logic
# ============================================================

SRC := $(shell find $(SRC_DIRS) -name '*.c')

BUILD_DIR := target
OBJ_DIR   := $(BUILD_DIR)/objs
DEP_DIR   := $(BUILD_DIR)/deps
BIN_DIR   := $(BUILD_DIR)

OBJ := $(patsubst %.c,$(OBJ_DIR)/%.o,$(SRC))
DEP := $(patsubst %.c,$(DEP_DIR)/%.d,$(SRC))

TARGET := $(BIN_DIR)/$(TARGET_NAME)

INCLUDE_FLAG := -I$(INCLUDE_DIR)

.PHONY: all debug release clean print

all: debug


# ------------------------------------------------------------
# Build Modes
# ------------------------------------------------------------

debug:   CFLAGS += $(CFLAGS_DEBUG)
debug:   LDFLAGS += $(LDFLAGS_DEBUG)
debug:   $(TARGET)

release: CFLAGS += $(CFLAGS_RELEASE)
release: $(TARGET)


# ------------------------------------------------------------
# Linking Stage
# ------------------------------------------------------------

$(TARGET): $(OBJ)
	@mkdir -p $(BIN_DIR)
	$(CC) $(OBJ) $(LDFLAGS) $(LDLIBS) -o $@


# ------------------------------------------------------------
# Compilation + Auto Dependency
# ------------------------------------------------------------

$(OBJ_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	@mkdir -p $(DEP_DIR)/$(dir $*)
	$(CC) $(INCLUDE_FLAG) $(CFLAGS) -MMD -MP -MF $(DEP_DIR)/$*.d -c $< -o $@


# ------------------------------------------------------------
# Include Dependency Files
# ------------------------------------------------------------

-include $(DEP)


# ------------------------------------------------------------
# Utility
# ------------------------------------------------------------

clean:
	rm -rf $(BUILD_DIR)

print:
	@echo "SRC    = $(SRC)"
	@echo "OBJ    = $(OBJ)"
	@echo "DEP    = $(DEP)"
	@echo "TARGET = $(TARGET)"


# ============================================================
# Reference Guide
# ============================================================

# CC           : C compiler
# SRC_DIRS     : Source directories scanned recursively
# INCLUDE_DIR  : Header include directory
# TARGET_NAME  : Output executable name
#
# CFLAGS       : Compilation flags (warnings, debug, optimization)
# LDFLAGS      : Linker-specific flags
# LDLIBS       : Linked libraries (-lm, -lpthread, etc.)
#
# SRC          : All source files found in SRC_DIRS
# OBJ          : Object files generated from source files
# DEP          : Auto-generated dependency files (.d)
# TARGET       : Final executable output path
#
# Automatic Make Variables:
#   $@  : Target file of the rule
#   $<  : First prerequisite
#   $^  : All prerequisites
#   $*  : Stem matched by %
#
# Compiler Dependency Flags:
#   -MMD : Generate dependency file excluding system headers
#   -MP  : Add phony rules for headers to avoid make errors if deleted
#   -MF  : Specify dependency file output path
#   -c   : Compile only, do not link

