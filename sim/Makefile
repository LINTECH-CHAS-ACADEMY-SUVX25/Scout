# Scout UI-simulator. Bygger LVGL från det vendored källträdet i
# scout_screen och länkar mot SDL2. Objektfiler hamnar under build/.

LVGL  := ../scout_screen/components/lvgl_port/lvgl__lvgl
RC    := ../shared_components/rc_protocol
BUILD := build

CC      := cc
CFLAGS  := -O2 -g -Wall -I. -I$(LVGL) -I$(RC) \
           -DLV_CONF_INCLUDE_SIMPLE -DLV_LVGL_H_INCLUDE_SIMPLE \
           $(shell sdl2-config --cflags)
LDFLAGS := $(shell sdl2-config --libs) -lm

LVGL_SRC := $(shell find $(LVGL)/src -name '*.c')
LVGL_OBJ := $(patsubst $(LVGL)/%.c,$(BUILD)/lvgl/%.o,$(LVGL_SRC))
APP_SRC  := $(wildcard *.c)
APP_OBJ  := $(patsubst %.c,$(BUILD)/%.o,$(APP_SRC))

sim: $(APP_OBJ) $(LVGL_OBJ)
	$(CC) $^ -o $@ $(LDFLAGS)

$(BUILD)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/lvgl/%.o: $(LVGL)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

run: sim
	./sim

clean:
	rm -rf $(BUILD) sim

.PHONY: run clean
