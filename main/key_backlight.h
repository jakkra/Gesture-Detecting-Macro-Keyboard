#pragma once

typedef enum {
    KEY_BACKLIGHT_RAINBOW,
    KEY_BACKLIGHT_OFF,
    KEY_BACKLIGHT_BLINKING
} key_backlight_mode_t;

void key_backlight_init(void);
void key_backlight_set_mode(key_backlight_mode_t mode);