#include <lvgl.h>
#include "home_screen.h"
#include "event_handlers.h"
#include "utils.h"
#include <Arduino.h>
#include "ui.h"
#include "settings.h"
#include "SD_utils.h"
#include <math.h>

// S25 WALLPAPER COLORS
#define COLOR_S25_DARK lv_color_make(20, 20, 30)     // Deep space blue
#define COLOR_S25_PURPLE lv_color_make(40, 30, 60)   // Cosmic purple

// Your original colors
#define COLOR_PRIMARY lv_color_make(41, 121, 255)    // Blue
#define COLOR_SECONDARY lv_color_make(255, 59, 48)   // Red
#define COLOR_ACCENT lv_color_make(52, 199, 89)      // Green
#define COLOR_DARK_BG lv_color_black()
#define COLOR_WHITE_BG lv_color_white()

// Calculator color
#define COLOR_CALCULATOR lv_color_make(255, 214, 0)  // Yellow for calculator

// App data structure
typedef struct {
    const char* symbol;
    const char* name;
    lv_event_cb_t event_cb;
    lv_color_t color;
} AppIcon;

// WORKING CALCULATOR FUNCTIONS
static void calculator_event_handler(lv_event_t* e);
static void create_calculator_screen();
static void calc_btn_event_handler(lv_event_t* e);
static void calc_back_handler(lv_event_t* e);
static void calc_perform_calculation();

// Calculator variables
static char calc_buffer[32] = "0";
static double calc_current = 0;
static double calc_stored = 0;
static char calc_operator = '\0';
static bool calc_new_input = true;
static lv_obj_t* calc_display = NULL;

// Apps with icons and colors - REPLACED BATTERY WITH CALCULATOR
static const AppIcon apps[] = {
    {LV_SYMBOL_SETTINGS, "Settings", settings_event_handler, COLOR_PRIMARY},
    {LV_SYMBOL_DOWNLOAD, "FWFlash", launcher_event_handler, COLOR_SECONDARY},
    {LV_SYMBOL_SD_CARD, "Files", explorer_event_handler, COLOR_ACCENT},
    {LV_SYMBOL_WIFI, "WiFi", settings_event_handler, lv_color_make(255, 149, 0)},
    {LV_SYMBOL_BLUETOOTH, "BT", launcher_event_handler, lv_color_make(0, 122, 255)},
    {"🧮", "Calc", calculator_event_handler, COLOR_CALCULATOR},  // USING EMOJI
    {LV_SYMBOL_IMAGE, "Gallery", launcher_event_handler, lv_color_make(175, 82, 222)},
    {LV_SYMBOL_AUDIO, "Music", explorer_event_handler, lv_color_make(255, 45, 85)},
    {LV_SYMBOL_GPS, "Maps", settings_event_handler, lv_color_make(90, 200, 250)},
};

static lv_obj_t* time_label = NULL;

void create_s25_background(lv_obj_t* scr) {
    // Create S25 gradient background WITHOUT changing your layout
    lv_obj_t* bg = lv_obj_create(scr);
    lv_obj_set_size(bg, 240, 320);
    lv_obj_set_style_bg_color(bg, COLOR_S25_DARK, 0);
    lv_obj_set_style_bg_grad_color(bg, COLOR_S25_PURPLE, 0);
    lv_obj_set_style_bg_grad_dir(bg, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_radius(bg, 0, 0);
    lv_obj_set_style_border_width(bg, 0, 0);
    lv_obj_align(bg, LV_ALIGN_TOP_LEFT, 0, 0);
    
    // Send to back (background layer)
    lv_obj_move_to_index(bg, 0);
}

void update_time_cb(lv_timer_t* timer) {
    static uint8_t hour = 12;
    static uint8_t minute = 0;
    
    minute++;
    if (minute >= 60) {
        minute = 0;
        hour = (hour + 1) % 24;
    }
    
    char time_str[6];
    snprintf(time_str, sizeof(time_str), "%02d:%02d", hour, minute);
    lv_label_set_text(time_label, time_str);
}

// CALCULATOR HANDLERS
static void calculator_event_handler(lv_event_t* e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        create_calculator_screen();
    }
}

static void calc_btn_event_handler(lv_event_t* e) {
    const char* btn_text = (const char*)lv_event_get_user_data(e);
    
    if (strcmp(btn_text, "C") == 0) {
        // Clear everything
        strcpy(calc_buffer, "0");
        calc_current = 0;
        calc_stored = 0;
        calc_operator = '\0';
        calc_new_input = true;
    } 
    else if (strcmp(btn_text, "←") == 0) {
        // Backspace
        int len = strlen(calc_buffer);
        if (len > 1) {
            calc_buffer[len - 1] = '\0';
            calc_current = atof(calc_buffer);
        } else {
            strcpy(calc_buffer, "0");
            calc_current = 0;
            calc_new_input = true;
        }
    }
    else if (strcmp(btn_text, "=") == 0) {
        if (calc_operator != '\0') {
            calc_perform_calculation();
            calc_operator = '\0';
        }
        calc_new_input = true;
    }
    else if (strchr("/*-+", btn_text[0]) != NULL) {
        // Operator button
        if (calc_operator != '\0') {
            calc_perform_calculation();
        } else {
            calc_stored = calc_current;
        }
        calc_operator = btn_text[0];
        calc_new_input = true;
    }
    else {
        // Number or decimal point
        if (calc_new_input) {
            strcpy(calc_buffer, btn_text);
            calc_new_input = false;
        } else {
            if (strlen(calc_buffer) < 20) {
                if (strcmp(calc_buffer, "0") == 0 && btn_text[0] != '.') {
                    strcpy(calc_buffer, btn_text);
                } else {
                    // Check for duplicate decimal points
                    if (btn_text[0] == '.') {
                        char* dot = strchr(calc_buffer, '.');
                        if (dot == NULL) {
                            strcat(calc_buffer, btn_text);
                        }
                    } else {
                        strcat(calc_buffer, btn_text);
                    }
                }
            }
        }
        calc_current = atof(calc_buffer);
    }
    
    lv_label_set_text(calc_display, calc_buffer);
}

static void calc_perform_calculation() {
    switch (calc_operator) {
        case '+':
            calc_stored += calc_current;
            break;
        case '-':
            calc_stored -= calc_current;
            break;
        case '*':
            calc_stored *= calc_current;
            break;
        case '/':
            if (calc_current != 0) {
                calc_stored /= calc_current;
            } else {
                strcpy(calc_buffer, "Error");
                lv_label_set_text(calc_display, calc_buffer);
                return;
            }
            break;
    }
    
    calc_current = calc_stored;
    snprintf(calc_buffer, sizeof(calc_buffer), "%.10g", calc_stored);
    
    // Remove trailing zeros
    char* dot = strchr(calc_buffer, '.');
    if (dot) {
        char* p = calc_buffer + strlen(calc_buffer) - 1;
        while (p > dot && *p == '0') {
            *p-- = '\0';
        }
        if (p == dot) {
            *p = '\0';
        }
    }
}

static void calc_back_handler(lv_event_t* e) {
    drawHomeScreen();
}

static void create_calculator_screen() {
    lv_obj_t* scr = lv_scr_act();
    lv_obj_clean(scr);
    
    // Calculator background
    lv_obj_t* calc_bg = lv_obj_create(scr);
    lv_obj_set_size(calc_bg, 240, 320);
    lv_obj_set_style_bg_color(calc_bg, lv_color_make(30, 30, 40), 0);
    lv_obj_align(calc_bg, LV_ALIGN_TOP_LEFT, 0, 0);
    
    // Title
    lv_obj_t* title = lv_label_create(scr);
    lv_label_set_text(title, "Calculator");
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);
    
    // Display
    lv_obj_t* display_bg = lv_obj_create(scr);
    lv_obj_set_size(display_bg, 220, 50);
    lv_obj_set_style_bg_color(display_bg, lv_color_make(20, 20, 25), 0);
    lv_obj_set_style_radius(display_bg, 10, 0);
    lv_obj_align(display_bg, LV_ALIGN_TOP_MID, 0, 50);
    
    calc_display = lv_label_create(display_bg);
    lv_label_set_text(calc_display, "0");
    lv_obj_set_style_text_color(calc_display, lv_color_white(), 0);
    lv_obj_align(calc_display, LV_ALIGN_RIGHT_MID, -10, 0);
    
    // Buttons - CORRECT LAYOUT
    const char* buttons[] = {
        "7", "8", "9", "/",
        "4", "5", "6", "*",
        "1", "2", "3", "-",
        "C", "0", ".", "+",
        "=", "←"
    };
    
    int btn_size = 50;
    int margin = 5;
    
    // Create 4x4 grid of buttons
    for (int row = 0; row < 4; row++) {
        for (int col = 0; col < 4; col++) {
            int i = row * 4 + col;
            int x = 10 + col * (btn_size + margin);
            int y = 120 + row * (btn_size + margin);
            
            lv_obj_t* btn = lv_btn_create(scr);
            lv_obj_set_size(btn, btn_size, btn_size);
            lv_obj_set_pos(btn, x, y);
            lv_obj_set_style_radius(btn, 10, 0);
            
            // Set colors
            if (strchr("/*-+", buttons[i][0]) != NULL) {
                lv_obj_set_style_bg_color(btn, COLOR_PRIMARY, 0);
            } else if (strcmp(buttons[i], "C") == 0) {
                lv_obj_set_style_bg_color(btn, COLOR_SECONDARY, 0);
            } else if (strcmp(buttons[i], "=") == 0) {
                lv_obj_set_style_bg_color(btn, COLOR_ACCENT, 0);
            } else if (strcmp(buttons[i], "←") == 0) {
                lv_obj_set_style_bg_color(btn, lv_color_make(255, 149, 0), 0);
            } else {
                lv_obj_set_style_bg_color(btn, lv_color_make(50, 50, 70), 0);
            }
            
            lv_obj_add_event_cb(btn, calc_btn_event_handler, LV_EVENT_CLICKED, (void*)buttons[i]);
            
            lv_obj_t* label = lv_label_create(btn);
            lv_label_set_text(label, buttons[i]);
            lv_obj_center(label);
        }
    }
    
    // Create = button (big, at bottom)
    lv_obj_t* equals_btn = lv_btn_create(scr);
    lv_obj_set_size(equals_btn, btn_size * 2 + margin, btn_size);
    lv_obj_set_pos(equals_btn, 10, 120 + 4 * (btn_size + margin));
    lv_obj_set_style_radius(equals_btn, 10, 0);
    lv_obj_set_style_bg_color(equals_btn, COLOR_ACCENT, 0);
    lv_obj_add_event_cb(equals_btn, calc_btn_event_handler, LV_EVENT_CLICKED, (void*)"=");
    
    lv_obj_t* equals_label = lv_label_create(equals_btn);
    lv_label_set_text(equals_label, "=");
    lv_obj_center(equals_label);
    
    // Create ← button
    lv_obj_t* backspace_btn = lv_btn_create(scr);
    lv_obj_set_size(backspace_btn, btn_size, btn_size);
    lv_obj_set_pos(backspace_btn, 10 + 2 * (btn_size + margin) + 5, 120 + 4 * (btn_size + margin));
    lv_obj_set_style_radius(backspace_btn, 10, 0);
    lv_obj_set_style_bg_color(backspace_btn, lv_color_make(255, 149, 0), 0);
    lv_obj_add_event_cb(backspace_btn, calc_btn_event_handler, LV_EVENT_CLICKED, (void*)"←");
    
    lv_obj_t* backspace_label = lv_label_create(backspace_btn);
    lv_label_set_text(backspace_label, "←");
    lv_obj_center(backspace_label);
    
    // Back button to return home (separate)
    lv_obj_t* home_btn = lv_btn_create(scr);
    lv_obj_set_size(home_btn, 100, 40);
    lv_obj_set_style_bg_color(home_btn, lv_color_make(80, 80, 100), 0);
    lv_obj_set_style_radius(home_btn, 15, 0);
    lv_obj_align(home_btn, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_add_event_cb(home_btn, calc_back_handler, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t* home_label = lv_label_create(home_btn);
    lv_label_set_text(home_label, LV_SYMBOL_HOME " Back");
    lv_obj_center(home_label);
    
    // Reset calculator
    strcpy(calc_buffer, "0");
    calc_current = 0;
    calc_stored = 0;
    calc_operator = '\0';
    calc_new_input = true;
}

void drawHomeScreen() {
    if (!init_sd_card()) {
        Serial.println("Warning: SD card initialization failed in home screen");
    }
    
    lv_obj_t* scr = lv_scr_act();
    short int iconsize = 67;
    
    // 1. ADD S25 BACKGROUND FIRST (send to back)
    create_s25_background(scr);
    
    // 2. Status bar - KEEP YOUR EXACT SIZE
    lv_obj_t* status_bar = lv_obj_create(scr);
    lv_obj_set_size(status_bar, 240, 30);
    lv_obj_clear_flag(status_bar, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_align(status_bar, LV_ALIGN_TOP_MID, 0, 0);
    
    // Make status bar glass-like
    lv_obj_set_style_bg_color(status_bar, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(status_bar, LV_OPA_70, 0); // Semi-transparent
    lv_obj_set_style_border_width(status_bar, 0, 0);
    
    time_label = lv_label_create(status_bar);
    lv_label_set_text(time_label, "12:00");
    lv_obj_set_style_text_color(time_label, COLOR_WHITE_BG, 0);
    lv_obj_align(time_label, LV_ALIGN_CENTER, 0, 0);
    
    // 3. Button container - KEEP YOUR EXACT DIMENSIONS
    lv_obj_t* btn_cont = lv_obj_create(scr);
    lv_obj_set_size(btn_cont, 240, 290);
    lv_obj_align(btn_cont, LV_ALIGN_BOTTOM_MID, 0, 0);
    
    // Make container glass-like
    lv_obj_set_style_bg_color(btn_cont, lv_color_make(30, 30, 40), 0);
    lv_obj_set_style_bg_opa(btn_cont, LV_OPA_80, 0); // Semi-transparent
    lv_obj_set_style_radius(btn_cont, 0, 0); // No rounded corners to keep your exact fit
    
    lv_obj_clear_flag(btn_cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_layout(btn_cont, LV_LAYOUT_GRID);
    lv_obj_set_style_pad_all(btn_cont, 10, 0);
    
    // KEEP YOUR EXACT GRID
    static lv_coord_t col_dsc[] = {iconsize, iconsize, iconsize, LV_GRID_TEMPLATE_LAST};
    static lv_coord_t row_dsc[] = {iconsize, iconsize, iconsize, LV_GRID_TEMPLATE_LAST};
    lv_obj_set_grid_dsc_array(btn_cont, col_dsc, row_dsc);
    
    // Create all 9 buttons with improved styling
    for (int i = 0; i < 9; i++) {
        int col = i % 3;
        int row = i / 3;
        
        lv_obj_t* btn = lv_btn_create(btn_cont);
        lv_obj_set_size(btn, iconsize, iconsize);
        lv_obj_set_grid_cell(btn, LV_GRID_ALIGN_CENTER, col, 1, LV_GRID_ALIGN_CENTER, row, 1);
        
        // IMPROVED STYLING - rounded corners, shadows, colors
        lv_obj_set_style_radius(btn, 15, 0);
        if (i < sizeof(apps) / sizeof(apps[0])) {
            lv_obj_set_style_bg_color(btn, apps[i].color, 0);
            lv_obj_set_style_shadow_width(btn, 10, 0);
            lv_obj_set_style_shadow_color(btn, lv_color_black(), 0);
            lv_obj_set_style_shadow_ofs_y(btn, 3, 0);
            
            if (apps[i].event_cb) {
                lv_obj_add_event_cb(btn, apps[i].event_cb, LV_EVENT_CLICKED, NULL);
            }
        } else {
            // Default styling for extra buttons
            lv_obj_set_style_bg_color(btn, lv_color_make(100, 100, 100), 0);
        }
        
        lv_obj_t* label = lv_label_create(btn);
        if (i < sizeof(apps) / sizeof(apps[0])) {
            // Use icon + text for first 9 apps
            char label_text[32];
            snprintf(label_text, sizeof(label_text), "%s\n%s", apps[i].symbol, apps[i].name);
            lv_label_set_text(label, label_text);
        } else if (i == 3) {
            lv_label_set_text(label, LV_SYMBOL_WIFI "\nWiFi");
        } else if (i == 4) {
            lv_label_set_text(label, "BT");
        } else if (i == 5) {
           lv_label_set_text(label, "🧮\nCalc");
        } else if (i == 6) {
            lv_label_set_text(label, LV_SYMBOL_IMAGE "\nGallery");
        } else if (i == 7) {
            lv_label_set_text(label, LV_SYMBOL_AUDIO "\nMusic");
        } else if (i == 8) {
            lv_label_set_text(label, LV_SYMBOL_GPS "\nMaps");
        }
        
        lv_obj_set_style_text_color(label, COLOR_WHITE_BG, 0);
        lv_obj_center(label);
    }
    
    // Create timer for time updates
    lv_timer_create(update_time_cb, 60000, NULL);
    
    // Draw navigation bar
    drawNavBar();
}