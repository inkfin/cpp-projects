BUILD_DIR  := build
TARGET_DIR := target
TEST_DIR   := test

CFLAGS  := -Wall -Wextra -std=c89 -Og -g -fPIC
ARFLAGS := rcs

# ----- core (library) -----
CORE_NAME := inklisp
CORE_SRCS := lib.c mpc.c
CORE_OBJS := $(CORE_SRCS:%.c=$(BUILD_DIR)/%.o)
CORE_LIB  := $(TARGET_DIR)/lib$(CORE_NAME).a

# ----- app (executable) -----
APP_NAME := lisp
APP_SRCS := main.c
APP_OBJS := $(APP_SRCS:%.c=$(BUILD_DIR)/%.o)
APP_EXE  := $(TARGET_DIR)/$(APP_NAME)

.PHONY: all core app test clean
all: core app

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

core: $(CORE_LIB)

$(CORE_LIB): $(CORE_OBJS)
	@mkdir -p $(TARGET_DIR)
	$(AR) $(ARFLAGS) $@ $^

app: $(APP_EXE)

$(APP_EXE): $(APP_OBJS) $(CORE_LIB)
	@mkdir -p $(TARGET_DIR)
	$(CC) $(CFLAGS) $^ -o $@

$(BUILD_DIR)/%.o: $(TEST_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

test: $(TARGET_DIR)/test_mpc

$(TARGET_DIR)/test_mpc: $(BUILD_DIR)/test_mpc.o $(CORE_LIB)
	$(CC) $(CFLAGS) $^ -o $@

clean:
	rm -rf $(BUILD_DIR) $(TARGET_DIR)
