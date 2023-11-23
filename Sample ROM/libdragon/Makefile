BUILD_DIR=build
include $(N64_INST)/include/n64.mk

src = main.c sausage64.c
assets_png = $(wildcard assets/*.png)

assets_conv = $(addprefix filesystem/,$(notdir $(assets_png:%.png=%.sprite)))

MKSPRITE_FLAGS ?=

all: sausage64.z64

filesystem/%.sprite: assets/%.png
	@mkdir -p $(dir $@)
	@echo "    [SPRITE] $@"
	@$(N64_MKSPRITE) -v -m BOX -o "$(dir $@)" "$<"

$(BUILD_DIR)/sausage64.dfs: $(assets_conv)
$(BUILD_DIR)/sausage64.elf: $(src:%.c=$(BUILD_DIR)/%.o)

sausage64.z64: N64_ROM_TITLE="SAUSAGE64"
sausage64.z64: $(BUILD_DIR)/sausage64.dfs

clean:
	rm -rf $(BUILD_DIR) sausage64.z64

-include $(wildcard $(BUILD_DIR)/*.d)

.PHONY: all clean
