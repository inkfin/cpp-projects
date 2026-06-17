CC ?= cc

TARGET := target/app
SRC := src/main.c
INCLUDE := -Iinclude

CFLAGS := -std=gnu11 -Wall -Wextra -Wshadow -Wconversion -Wformat=2 -Wundef -Wmissing-prototypes -Wimplicit-fallthrough
CFLAGS_DEBUG := -O0 -g3 -ggdb -fsanitize=address,undefined -fno-omit-frame-pointer -fstack-protector-strong -fno-common
CFLAGS_RELEASE := -O2 -DNDEBUG

LDFLAGS :=
LDFLAGS_DEBUG := -fsanitize=address,undefined
LDLIBS :=

.PHONY: all debug release clean

all: debug

debug: CFLAGS += $(CFLAGS_DEBUG)
debug: LDFLAGS += $(LDFLAGS_DEBUG)
debug: $(TARGET)

release: CFLAGS += $(CFLAGS_RELEASE)
release: $(TARGET)

$(TARGET): $(SRC)
	@mkdir -p $(@D)
	$(CC) $(INCLUDE) $(CFLAGS) $< $(LDFLAGS) $(LDLIBS) -o $@

clean:
	rm -rf target
