
CFLAGS = -Wall -Wextra -std=c99

SRC = src/main.c
OUT = target/main

.PHONY: all app test clean

all: $(OUT)

$(OUT): $(SRC)
	mkdir -p target/
	$(CC) $(CFLAGS) -o $(OUT) $(SRC)

clean:
	rm -rf $(OUT)
