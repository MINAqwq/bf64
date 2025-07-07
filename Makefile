all: bf.z64
.PHONY: all

BUILD_DIR = build
include $(N64_INST)/include/n64.mk

N64_C_AND_CXX_FLAGS += -fdiagnostics-color=never

OBJS = $(BUILD_DIR)/bf.o

bf.z64: N64_ROM_TITLE = "Brainfuck" 
bf.z64: $(BUILD_DIR)/bf.dfs

$(BUILD_DIR)/bf.elf: $(OBJS)

$(BUILD_DIR)/bf.dfs: $(wildcard $(SOURCE_DIR)/filesystem/*) filesystem/audio/menu.wav64

filesystem/audio/%.wav64: assets/%.wav
	@mkdir -p $(dir $@)
	@echo "    [AUDIO] $@"
	@$(N64_AUDIOCONV) -o filesystem/audio --wav-loop true "$<"

clean:
	rm -rf $(BUILD_DIR) *.z64 filesystem/audio/*.wav64
.PHONY: clean

-include $(wildcard $(BUILD_DIR)/*.d)
