CC ?= cc
TARGET := target/app
SRC := src/main.c
INCLUDE := -Iinclude

# Dawn/WebGPU library path (override to match your install)
DAWN_DIR ?= /usr/local
DAWN_LIB := $(DAWN_DIR)/lib/libwebgpu_dawn.a

# ============================================================
# Platform detection
# ============================================================
UNAME := $(shell uname)
ifeq ($(UNAME), Darwin)
  PLATFORM ?= macos
else ifeq ($(UNAME), Linux)
  PLATFORM ?= linux
else
  PLATFORM ?= windows
endif

# ============================================================
# Platform-specific configuration
# ============================================================
ifeq ($(PLATFORM), macos)
  CFLAGS   += -x objective-c
  INCLUDE  += -I$(DAWN_DIR)/include
  LDFLAGS  += $(DAWN_LIB) -lc++ -lobjc
  LDFLAGS  += -framework AppKit -framework QuartzCore -framework Metal
  LDFLAGS  += -framework CoreFoundation -framework IOKit -framework IOSurface

else ifeq ($(PLATFORM), linux)
  INCLUDE  += -I$(DAWN_DIR)/include
  LDFLAGS  += $(DAWN_LIB) -lc++ -ldl -lpthread -lm
  LDFLAGS  += -lX11 -lXi -lXcursor

else ifeq ($(PLATFORM), windows)
  INCLUDE  += -I$(DAWN_DIR)/include
  LDFLAGS  += $(DAWN_LIB) -lc++
  LDFLAGS  += -lkernel32 -luser32 -lshell32

else ifeq ($(PLATFORM), emscripten)
  CC := emcc
  LDFLAGS  += --use-port=emdawnwebgpu -s ASYNCIFY -s ALLOW_MEMORY_GROWTH
  TARGET   := target/app.html
endif

# ============================================================
# Common flags
# ============================================================
CFLAGS += -std=gnu11 -Wall -Wextra -Wshadow -Wconversion -Wformat=2 -Wundef
CFLAGS += -Wmissing-prototypes -Wimplicit-fallthrough

CFLAGS_DEBUG   := -O0 -g
CFLAGS_RELEASE := -O2 -DNDEBUG
LDFLAGS_DEBUG  :=

ifneq ($(PLATFORM), emscripten)
  CFLAGS_DEBUG += -fsanitize=address,undefined -fno-omit-frame-pointer
  LDFLAGS_DEBUG += -fsanitize=address,undefined
endif

# ============================================================
# Targets
# ============================================================
.PHONY: all debug release clean

all: debug

debug: CFLAGS += $(CFLAGS_DEBUG)
debug: LDFLAGS += $(LDFLAGS_DEBUG)
debug: $(TARGET)

release: CFLAGS += $(CFLAGS_RELEASE)
release: $(TARGET)

$(TARGET): $(SRC)
	@mkdir -p $(@D)
	$(CC) $(INCLUDE) $(CFLAGS) $< -x none $(LDFLAGS) -o $@

clean:
	rm -rf target
