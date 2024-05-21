BUILD_DIR=build
include $(N64_INST)/include/n64.mk

src = main.c sausage64.c
assets_png = $(wildcard assets/*.png)

assets_conv = $(addprefix filesystem/,$(notdir $(assets_png:%.png=%.sprite)))

MKSPRITE_FLAGS ?=

all: sausage.z64

filesystem/%.sprite: assets/%.png
	@mkdir -p $(dir $@)
	@echo "    [SPRITE] $@"
	@$(N64_MKSPRITE) -v -m BOX -o "$(dir $@)" "$<"

$(BUILD_DIR)/sausage.dfs: $(assets_conv)
$(BUILD_DIR)/sausage.elf: $(src:%.c=$(BUILD_DIR)/%.o)

sausage.z64: N64_ROM_TITLE="SAUSAGE64"
sausage.z64: $(BUILD_DIR)/sausage.dfs

clean:
	rm -rf $(BUILD_DIR) sausage.z64

-include $(wildcard $(BUILD_DIR)/*.d)

.PHONY: all clean
