#ifndef EVENT_HANDLERS_H
#define EVENT_HANDLERS_H

#include <lvgl.h>

// Existing handlers
void settings_event_handler(lv_event_t *e);
void launcher_event_handler(lv_event_t *e);
void touch_event_handler(lv_event_t *e);
void home_button_event_handler(lv_event_t *e);
void explorer_event_handler(lv_event_t *e);
void torch_event_handler(lv_event_t *e);

// NEW HANDLERS FOR CALCULATOR & INSTALLER
void fake_installer_event_handler(lv_event_t* e);
void calculator_event_handler(lv_event_t* e);

#endif