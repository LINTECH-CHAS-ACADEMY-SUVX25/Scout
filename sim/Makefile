# Scout UI-simulator. Bygger LVGL och den delade UI-koden från
# scout_screen/components/lvgl_port och länkar mot SDL2.
# Objektfiler hamnar under build/.

PORT  := ../scout_screen/components/lvgl_port
LVGL  := $(PORT)/lvgl__lvgl
RC    := ../shared_components/rc_protocol
BUILD := build

# -I. står före -I$(PORT) så att simulatorns lv_conf.h och display.h
# vinner över enhetens vid quote-include-sökningen.
CC      := cc
CFLAGS  := -O2 -g -Wall -I. -I$(LVGL) -I$(RC) -I$(PORT) \
           -DLV_CONF_INCLUDE_SIMPLE -DLV_LVGL_H_INCLUDE_SIMPLE \
           $(shell sdl2-config --cflags)
LDFLAGS := $(shell sdl2-config --libs) -lm

LVGL_SRC := $(shell find $(LVGL)/src -name '*.c')
LVGL_OBJ := $(patsubst $(LVGL)/%.c,$(BUILD)/lvgl/%.o,$(LVGL_SRC))
PORT_SRC := $(PORT)/lvgl_port_ui.c $(PORT)/press_start_2p_8.c $(PORT)/press_start_2p_96.c
PORT_OBJ := $(patsubst $(PORT)/%.c,$(BUILD)/port/%.o,$(PORT_SRC))
APP_SRC  := $(wildcard *.c)
APP_OBJ  := $(patsubst %.c,$(BUILD)/%.o,$(APP_SRC))

sim: $(APP_OBJ) $(PORT_OBJ) $(LVGL_OBJ)
	$(CC) $^ -o $@ $(LDFLAGS)

$(BUILD)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/port/%.o: $(PORT)/%.c
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
