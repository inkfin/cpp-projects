
CFLAGS = -Wall -Wextra -std=c99 -Isrc

# Debug
CFLAGS += -g
# Release
#CFLAGS += -O2 -DNDEBUG

SRC = \
	src/main.c \
	src/arena.c
OBJDIR = target/obj
OBJ = $(addprefix $(OBJDIR)/, $(notdir $(SRC:.c=.o)))
OUT = target/main

.PHONY: all clean

all: $(OUT)

$(OUT): $(OBJ)
	mkdir -p target/
	$(CC) $(CFLAGS) -o $@ $(OBJ)

$(OBJDIR)/%.o: src/%.c
	mkdir -p $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf target/

