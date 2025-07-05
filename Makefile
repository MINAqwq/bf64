all: bf.z64
.PHONY: all

BUILD_DIR = build
include $(N64_INST)/include/n64.mk

N64_C_AND_CXX_FLAGS += -fdiagnostics-color=never

OBJS = $(BUILD_DIR)/bf.o

bf.z64: N64_ROM_TITLE = "Brainfuck"

$(BUILD_DIR)/bf.elf: $(OBJS)

clean:
	rm -rf $(BUILD_DIR) *.z64
.PHONY: clean

-include $(wildcard $(BUILD_DIR)/*.d)