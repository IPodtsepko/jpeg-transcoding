CC = clang
CFLAGS = -O3
BIN_DIR = ./bin
SRC_DIR = ./src

all: create_bin_dir jpegdec

create_bin_dir:
    mkdir -p $(BIN_DIR)

jpegdec: $(SRC_DIR)/jpegdec.cpp
    $(CC) $(CFLAGS) $< -o $(BIN_DIR)/jpegdec

clean:
    rm -rf $(BIN_DIR)

.PHONY: all create_bin_dir clean
