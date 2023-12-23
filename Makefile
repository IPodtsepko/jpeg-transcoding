CC = clang
CFLAGS = -O3
BIN_DIR = ./bin
SRC_DIR = ./src
OUT_DIR = ./out
DECODER_BINARY = $(BIN_DIR)/jpegdec

all: create_bin_dir jpegdec

create_bin_dir:
	mkdir -p $(BIN_DIR)

jpegdec: $(SRC_DIR)/jpegdec.cpp
	$(CC) $(CFLAGS) $< -o $(DECODER_BINARY)

process: ${DECODER_BINARY} $(OUT_DIR)
ifndef folder
	$(error folder is not defined. Please specify the 'folder' argument)
endif
	echo $$(seq -s ',' 0 63) > ${OUT_DIR}/coefficients.csv
	@$(foreach file, $(wildcard $(folder)/*.jpg), \
		${DECODER_BINARY} $(file) out.ppm >> ${OUT_DIR}/coefficients.csv;)
	rm out.ppm

$(OUT_DIR):
	mkdir -p $(OUT_DIR)

clean:
	rm -rf $(BIN_DIR) ${OUT_DIR}

.PHONY: all create_bin_dir clean
