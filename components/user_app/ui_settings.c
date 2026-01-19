#include "ui_settings.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "src/extra/libs/gif/lv_gif.h"
#include "ui_robo_eyes.h"
#include <stdio.h>

// -----------------------------------------------------------------------------
// Variables
// -----------------------------------------------------------------------------
static lv_obj_t *scr_settings = NULL;
static lv_obj_t *cont_launcher = NULL;
static lv_obj_t *cont_settings_menu = NULL;
static lv_obj_t *cont_brightness = NULL;
static lv_obj_t *cont_wifi = NULL;

// Brightness controls
static lv_obj_t *slider_brightness = NULL;
static lv_obj_t *label_brightness_val = NULL;
static lv_obj_t *cont_brightness_info = NULL;
static lv_obj_t *btn_brightness_done = NULL;

static void (*brightness_cb)(uint8_t) = NULL;
static void (*wifi_scan_cb)(void) = NULL;
static void (*wifi_connect_cb)(const char *ssid, const char *password) = NULL;
static bool is_visible = false;

// WiFi UI elements (for dynamic updates)
static lv_obj_t *wifi_lbl_ssid = NULL;
static lv_obj_t *wifi_lbl_status = NULL;
static lv_obj_t *wifi_lbl_ip = NULL;
static lv_obj_t *wifi_net_list = NULL;
static lv_obj_t *wifi_sw = NULL;

// WiFi Password Dialog
static lv_obj_t *wifi_pwd_dialog = NULL;
static lv_obj_t *wifi_pwd_input = NULL;
static lv_obj_t *wifi_pwd_kb = NULL;
static char wifi_selected_ssid[64] = {0};

// Global WiFi Status Indicator (shown on all pages)
static lv_obj_t *global_wifi_indicator = NULL;
static bool global_wifi_connected = false;

// -----------------------------------------------------------------------------
// Styles
// -----------------------------------------------------------------------------
static lv_style_t style_scr;
static lv_style_t style_icon_cont;

// -----------------------------------------------------------------------------
// Forward Declarations
// -----------------------------------------------------------------------------
LV_IMG_DECLARE(settings_icon);
LV_IMG_DECLARE(deskbot_icon);
LV_IMG_DECLARE(pomodoro_icon);

static void show_brightness_panel(void);
static void show_launcher_panel(void);
static void show_settings_menu(void);
static void show_wifi_panel(void);
static void update_pomodoro_ui(void);

// -----------------------------------------------------------------------------
// Pomodoro Variables
// -----------------------------------------------------------------------------
static lv_obj_t *cont_pomodoro_view = NULL;
static lv_obj_t *label_pomodoro_time = NULL;
static lv_obj_t *label_session_len = NULL;  // For the setting
static lv_obj_t *label_break_len = NULL;    // For the setting
static lv_obj_t *btn_pomodoro_start = NULL; // Start/Pause
static lv_obj_t *btn_pomodoro_reset = NULL;
static lv_obj_t *label_start_text = NULL;
static lv_timer_t *timer_pomodoro = NULL;

static lv_obj_t *arc_pomodoro = NULL;

// Animation target objects
static lv_obj_t *pomo_timer_card = NULL;
static lv_obj_t *pomo_action_bar = NULL;
static lv_obj_t *pomo_ambient_glow = NULL;
static lv_obj_t *pomo_cont_session = NULL;

static int32_t session_length_mins = 25;
static int32_t break_length_mins = 5;
static int32_t pomodoro_time_sec = 25 * 60; // 25 minutes
static bool pomodoro_running = false;
static bool is_break_mode = false;

extern const lv_img_dsc_t *book_anim_anim_imgs[];
extern const uint16_t book_anim_count;
extern const lv_img_dsc_t *pomo_anim_anim_imgs[];
extern const uint16_t pomo_anim_count;
static lv_obj_t *anim_img_book = NULL;
static lv_obj_t *anim_img_pomo = NULL; // Promoted to static for animation

// -----------------------------------------------------------------------------
// NVS Settings Persistence
// -----------------------------------------------------------------------------
#define SETTINGS_NAMESPACE "ui_settings"
#define KEY_BRIGHTNESS "brightness"
#define KEY_SESSION_LEN "session_len"
#define KEY_BREAK_LEN "break_len"
#define KEY_WIFI_ENABLED "wifi_en"

static uint8_t saved_brightness = 89;   // Default 35%
static bool saved_wifi_enabled = false; // Default WiFi OFF

static void save_settings_to_nvs(void) {
  nvs_handle_t nvs_handle;
  esp_err_t err = nvs_open(SETTINGS_NAMESPACE, NVS_READWRITE, &nvs_handle);
  if (err != ESP_OK) {
    printf("[SETTINGS] Failed to open NVS for save: %d\n", err);
    return;
  }

  err = nvs_set_u8(nvs_handle, KEY_BRIGHTNESS, saved_brightness);
  if (err != ESP_OK)
    printf("[SETTINGS] Save brightness err: %d\n", err);

  err = nvs_set_i32(nvs_handle, KEY_SESSION_LEN, session_length_mins);
  if (err != ESP_OK)
    printf("[SETTINGS] Save session err: %d\n", err);

  err = nvs_set_i32(nvs_handle, KEY_BREAK_LEN, break_length_mins);
  if (err != ESP_OK)
    printf("[SETTINGS] Save break err: %d\n", err);

  err = nvs_set_u8(nvs_handle, KEY_WIFI_ENABLED, saved_wifi_enabled ? 1 : 0);
  if (err != ESP_OK)
    printf("[SETTINGS] Save WiFi state err: %d\n", err);

  err = nvs_commit(nvs_handle);
  if (err != ESP_OK)
    printf("[SETTINGS] *** COMMIT FAILED: %d ***\n", err);
  nvs_close(nvs_handle);

  printf("[SETTINGS] Saved: Brightness=%d, Session=%d, Break=%d, WiFi=%s "
         "(commit=%s)\n",
         saved_brightness, (int)session_length_mins, (int)break_length_mins,
         saved_wifi_enabled ? "ON" : "OFF", err == 0 ? "OK" : "FAIL");
}

static void load_settings_from_nvs(void) {
  nvs_handle_t nvs_handle;
  esp_err_t err = nvs_open(SETTINGS_NAMESPACE, NVS_READONLY, &nvs_handle);
  if (err != ESP_OK) {
    printf("[SETTINGS] No saved settings found (using defaults)\n");
    return; // Use defaults
  }

  uint8_t brightness = 89;
  int32_t session = 25;
  int32_t break_time = 5;
  uint8_t wifi_en = 0;

  if (nvs_get_u8(nvs_handle, KEY_BRIGHTNESS, &brightness) == ESP_OK) {
    saved_brightness = brightness;
  }
  if (nvs_get_i32(nvs_handle, KEY_SESSION_LEN, &session) == ESP_OK) {
    session_length_mins = session;
    pomodoro_time_sec = session * 60;
  }
  if (nvs_get_i32(nvs_handle, KEY_BREAK_LEN, &break_time) == ESP_OK) {
    break_length_mins = break_time;
  }
  if (nvs_get_u8(nvs_handle, KEY_WIFI_ENABLED, &wifi_en) == ESP_OK) {
    saved_wifi_enabled = (wifi_en != 0);
  }

  nvs_close(nvs_handle);

  printf("[SETTINGS] Loaded: Brightness=%d, Session=%d, Break=%d, WiFi=%s\n",
         saved_brightness, (int)session_length_mins, (int)break_length_mins,
         saved_wifi_enabled ? "ON" : "OFF");
}

// Apply loaded settings to UI controls (call after UI is created)
static void apply_loaded_settings_to_ui(void) {
  // Apply brightness to arc and call callback
  if (slider_brightness) {
    int32_t brightness_percent = (saved_brightness * 100) / 255;
    lv_arc_set_value(slider_brightness, brightness_percent);

    // Update label
    if (label_brightness_val) {
      lv_label_set_text_fmt(label_brightness_val, "%d%%",
                            (int)brightness_percent);
    }

    // Apply to hardware
    if (brightness_cb) {
      brightness_cb(saved_brightness);
    }
  }

  // Apply session length to label
  if (label_session_len) {
    lv_label_set_text_fmt(label_session_len, "%d", (int)session_length_mins);
  }

  // Apply break length to label
  if (label_break_len) {
    lv_label_set_text_fmt(label_break_len, "%d", (int)break_length_mins);
  }

  // Apply WiFi enabled state to switch
  if (wifi_sw) {
    if (saved_wifi_enabled) {
      lv_obj_add_state(wifi_sw, LV_STATE_CHECKED);
    } else {
      lv_obj_clear_state(wifi_sw, LV_STATE_CHECKED);
    }
  }

  // Update pomodoro timer display
  update_pomodoro_ui();
}

// -----------------------------------------------------------------------------
// Animations
// -----------------------------------------------------------------------------

static void anim_arc_value_cb(void *var, int32_t v) {
  lv_arc_set_value((lv_obj_t *)var, (int16_t)v);
}

static void anim_zoom_cb(void *var, int32_t v) {
  lv_obj_set_style_transform_zoom((lv_obj_t *)var, v, 0);
}

static void anim_opa_cb(void *var, int32_t v) {
  lv_obj_set_style_opa((lv_obj_t *)var, v, 0);
}

// Floating animation callback (Sine wave effect)
static void anim_float_y_cb(void *var, int32_t v) {
  lv_obj_set_y((lv_obj_t *)var, v);
}

static void animate_brightness_view(void) {
  // 1. Arc Animation (Fill up)
  // Check if arc is valid
  if (slider_brightness) {
    int32_t target_val = lv_arc_get_value(slider_brightness);
    lv_arc_set_value(slider_brightness, 0); // Reset for animation

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, slider_brightness);
    lv_anim_set_values(&a, 0, target_val);
    lv_anim_set_time(&a, 1000);
    lv_anim_set_exec_cb(&a, anim_arc_value_cb);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
    lv_anim_start(&a);
  }

  // 2. Info Text Pop-in (Scale)
  if (cont_brightness_info) {
    lv_obj_set_style_transform_zoom(cont_brightness_info, 1,
                                    0); // Start tiny but non-zero

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, cont_brightness_info);
    lv_anim_set_values(&a, 1, 256); // 256 is 1.0 scale
    lv_anim_set_time(&a, 800);
    lv_anim_set_delay(&a, 200); // Slight delay
    lv_anim_set_exec_cb(&a, anim_zoom_cb);
    lv_anim_set_path_cb(&a, lv_anim_path_overshoot); // Bouncy pop
    lv_anim_start(&a);
  }

  // 3. Done Button Zoom & Fade
  if (btn_brightness_done) {
    lv_obj_set_style_transform_zoom(btn_brightness_done, 1,
                                    0); // Start tiny but non-zero

    // Ensure it's in correct position (though layout should handle it)
    // We don't move it, just pop it in.

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, btn_brightness_done);
    lv_anim_set_values(&a, 1, 256);
    lv_anim_set_time(&a, 600);
    lv_anim_set_delay(&a, 400); // Delayed pop
    lv_anim_set_exec_cb(&a, anim_zoom_cb);
    lv_anim_set_path_cb(&a, lv_anim_path_overshoot);
    lv_anim_start(&a);
  }
}

static void anim_rotate_cb(void *var, int32_t v) {
  lv_obj_set_style_transform_angle((lv_obj_t *)var, v, 0);
}

static void animate_pomodoro_view(void) {
  // ==========================================================================
  // SIMPLE ENTRANCE ANIMATION - Optimized for performance
  // ==========================================================================

  // Simple fade-in for the main container content
  // Simple fade-in for the main container content
  if (pomo_timer_card) {
    // FIX: Ensure visibility immediately (bypass animation to rule out opacity
    // issues)
    lv_obj_set_style_opa(pomo_timer_card, LV_OPA_COVER, 0);
  }
}

// -----------------------------------------------------------------------------
// Callbacks
// -----------------------------------------------------------------------------

static void back_to_launcher_cb(lv_event_t *e);
static void update_pomodoro_ui(void);    // Forward declare
static void update_session_len_ui(void); // Forward declare
static void update_break_len_ui(void);   // Forward declare

static void app_home_cb(lv_event_t *e) { ui_settings_hide(); }

static void app_settings_cb(lv_event_t *e) { show_settings_menu(); }

static void show_pomodoro_panel(void) {
  if (cont_launcher)
    lv_obj_add_flag(cont_launcher, LV_OBJ_FLAG_HIDDEN);
  if (cont_brightness)
    lv_obj_add_flag(cont_brightness, LV_OBJ_FLAG_HIDDEN);

  if (cont_pomodoro_view) {
    lv_obj_clear_flag(cont_pomodoro_view, LV_OBJ_FLAG_HIDDEN);
    update_pomodoro_ui();    // Ensure UI is up to date
    update_session_len_ui(); // Ensure session length UI is up to date
    animate_pomodoro_view();
    if (anim_img_book) {
      lv_animimg_start(anim_img_book);
    }
  }
}

static void app_pomodoro_cb(lv_event_t *e) { show_pomodoro_panel(); }

// -----------------------------------------------------------------------------
// Pomodoro Logic
// -----------------------------------------------------------------------------
static void update_pomodoro_ui(void) {
  int32_t min = pomodoro_time_sec / 60;
  int32_t sec = pomodoro_time_sec % 60;
  if (label_pomodoro_time) {
    lv_label_set_text_fmt(label_pomodoro_time, "%02d:%02d", (int)min, (int)sec);
  }

  // Update Arc if it exists
  if (arc_pomodoro) {
    int32_t total_sec =
        (is_break_mode ? break_length_mins : session_length_mins) * 60;
    if (total_sec > 0) {
      int32_t val = (pomodoro_time_sec * 100) / total_sec;
      lv_arc_set_value(arc_pomodoro, val);
    }
  }

  // Update Visuals for Mode
  if (pomo_timer_card) {
    if (is_break_mode) {
      lv_obj_set_style_bg_color(pomo_timer_card, lv_color_hex(0xE8F5E9),
                                0); // Light Green for Break
    } else {
      lv_obj_set_style_bg_color(pomo_timer_card, lv_color_hex(0xF4F8F0),
                                0); // Default White/Greenish
    }
  }
}

static void update_session_len_ui(void) {
  if (label_session_len) {
    lv_label_set_text_fmt(label_session_len, "%d", (int)session_length_mins);
  }
}

static void update_break_len_ui(void) {
  if (label_break_len) {
    lv_label_set_text_fmt(label_break_len, "%d", (int)break_length_mins);
  }
}

static void btn_session_minus_cb(lv_event_t *e) {
  if (session_length_mins > 1 && !pomodoro_running) {
    session_length_mins--;
    if (!is_break_mode) {
      pomodoro_time_sec = session_length_mins * 60;
      update_pomodoro_ui();
    }
    update_session_len_ui();
    save_settings_to_nvs();
  }
}

static void btn_session_plus_cb(lv_event_t *e) {
  if (session_length_mins < 60 && !pomodoro_running) {
    session_length_mins++;
    if (!is_break_mode) {
      pomodoro_time_sec = session_length_mins * 60;
      update_pomodoro_ui();
    }
    update_session_len_ui();
    save_settings_to_nvs();
  }
}

static void btn_break_minus_cb(lv_event_t *e) {
  if (break_length_mins > 1 && !pomodoro_running) {
    break_length_mins--;
    if (is_break_mode) {
      pomodoro_time_sec = break_length_mins * 60;
      update_pomodoro_ui();
    }
    update_break_len_ui();
    save_settings_to_nvs();
  }
}

static void btn_break_plus_cb(lv_event_t *e) {
  if (break_length_mins < 30 && !pomodoro_running) {
    break_length_mins++;
    if (is_break_mode) {
      pomodoro_time_sec = break_length_mins * 60;
      update_pomodoro_ui();
    }
    update_break_len_ui();
    save_settings_to_nvs();
  }
}

static void update_pomodoro_mode_ui(void) {
  // Helper to update specific mode UI elements if needed
  if (pomo_timer_card) {
    if (is_break_mode) {
      lv_obj_set_style_bg_color(pomo_timer_card, lv_color_hex(0xD0F0C0),
                                0); // Tea Green for break
    } else {
      lv_obj_set_style_bg_color(pomo_timer_card, lv_color_hex(0xF4F8F0),
                                0); // Default
    }
  }
}

static void pomodoro_timer_cb(lv_timer_t *t) {
  if (pomodoro_time_sec > 0) {
    pomodoro_time_sec--;
    update_pomodoro_ui();
  } else {
    // Phase complete
    pomodoro_running = false;
    lv_timer_pause(timer_pomodoro);

    // Toggle Mode
    is_break_mode = !is_break_mode;

    if (is_break_mode) {
      pomodoro_time_sec = break_length_mins * 60;
      if (label_start_text)
        lv_label_set_text(label_start_text, "Start Break");
    } else {
      pomodoro_time_sec = session_length_mins * 60;
      if (label_start_text)
        lv_label_set_text(label_start_text, "Start Session");
    }

    update_pomodoro_ui();
    update_pomodoro_mode_ui();
  }
}

static void btn_pomodoro_start_cb(lv_event_t *e) {
  if (pomodoro_time_sec == 0)
    return;

  if (pomodoro_running) {
    // Pause
    pomodoro_running = false;
    if (timer_pomodoro)
      lv_timer_pause(timer_pomodoro);

    if (label_start_text) {
      if (is_break_mode)
        lv_label_set_text(label_start_text, "Resume Break");
      else
        lv_label_set_text(label_start_text, "Resume");
    }
  } else {
    // Start
    pomodoro_running = true;
    if (!timer_pomodoro) {
      timer_pomodoro = lv_timer_create(pomodoro_timer_cb, 1000, NULL);
    } else {
      lv_timer_resume(timer_pomodoro);
    }

    if (label_start_text)
      lv_label_set_text(label_start_text, "Pause");

    if (anim_img_book)
      lv_animimg_start(anim_img_book);
  }
}

static void btn_pomodoro_reset_cb(lv_event_t *e) {
  pomodoro_running = false;
  if (timer_pomodoro)
    lv_timer_pause(timer_pomodoro);

  // Reset to Session Mode default
  is_break_mode = false;
  pomodoro_time_sec = session_length_mins * 60; // Reset to set length

  update_pomodoro_ui();
  update_pomodoro_mode_ui();

  if (label_start_text)
    lv_label_set_text(label_start_text, "Start Timer");
}

// -----------------------------------------------------------------------------
// Pomodoro View
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
// Pomodoro View - Modern Minimalist (AMOLED Dark Mode)
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
// Pomodoro View - Cozy Orange Theme (Reference Adaptation)
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
// Pomodoro View - Card Style (Reference Faithful)
// -----------------------------------------------------------------------------
static void create_pomodoro_view(void) {
  if (cont_pomodoro_view)
    return;

  // ==========================================================================
  // RETRO GADGET UI - Polished & Centered 🎀
  // strictly using FLEX Layout for perfect centering
  // ==========================================================================

  // External font declarations
  extern lv_font_t lv_font_montserratMedium_23;
  extern lv_font_t lv_font_montserratMedium_20;
  extern lv_font_t lv_font_montserratMedium_18;

  // Color Palette
  lv_color_t col_bg = lv_color_hex(0xBCAD82); // Exact Book GIF Match (0xBD70)
  // lv_color_t col_body = lv_color_hex(0xE0E5E5); // Retro Grey Body (Unused)
  lv_color_t col_display = lv_color_hex(0xF4F8F0);    // Greenish-White LCD
  lv_color_t col_btn_dark = lv_color_hex(0x2C2C2C);   // Dark Grey/Black
  lv_color_t col_btn_accent = lv_color_hex(0xD95E52); // Red/Orange Accent
  lv_color_t col_text_dark = lv_color_hex(0x1A1A1A);  // Almost Black
  lv_color_t col_text_white = lv_color_hex(0xFFFFFF);

  // 1. Full Screen Container
  cont_pomodoro_view = lv_obj_create(scr_settings);
  lv_obj_set_size(cont_pomodoro_view, 466, 466);
  lv_obj_set_style_bg_color(cont_pomodoro_view, col_bg, 0);
  lv_obj_set_style_border_width(cont_pomodoro_view, 0, 0);
  lv_obj_add_flag(cont_pomodoro_view, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(cont_pomodoro_view, LV_OBJ_FLAG_SCROLLABLE);

  // 2. Main Body (Watch Face)
  lv_obj_t *body = lv_obj_create(cont_pomodoro_view);
  lv_obj_set_size(body, 456, 456);
  lv_obj_center(body);
  lv_obj_set_style_radius(body, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_color(body, col_bg, 0); // Match background seamlessly
  lv_obj_set_style_bg_opa(body, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(body, 0,
                                0); // No border for cleaner look with GIF
  lv_obj_clear_flag(body, LV_OBJ_FLAG_SCROLLABLE);

  // --- SECT 0: MASCOT GIF (BOOK) & POMODORO ICON ---
  // Book Animation (Left)
  // Book Animation (Left) - Size 120x120
  anim_img_book = lv_animimg_create(body);
  lv_animimg_set_src(anim_img_book, (const void **)book_anim_anim_imgs,
                     book_anim_count);
  lv_animimg_set_duration(anim_img_book, 1500);
  lv_animimg_set_repeat_count(anim_img_book, LV_ANIM_REPEAT_INFINITE);
  lv_obj_align(anim_img_book, LV_ALIGN_TOP_MID, -70,
               25); // Start at 25 to center with Pomo
  lv_animimg_start(anim_img_book);

  // Pomodoro GIF Animation (Right) - Using LV_GIF
  LV_IMG_DECLARE(pomo_gif);            // Declare the descriptor
  anim_img_pomo = lv_gif_create(body); // Assign to static
  lv_gif_set_src(anim_img_pomo, &pomo_gif);
  lv_obj_align(anim_img_pomo, LV_ALIGN_TOP_MID, 70, 10);

  // --- ANIMATIONS: Floating Floating Icons (Retro Feel) ---
  // Book Float (Height 120px) -> Center Y at 25+60 = 85
  lv_anim_t a_book;
  lv_anim_init(&a_book);
  lv_anim_set_var(&a_book, anim_img_book);
  lv_anim_set_values(&a_book, 25, 35); // Shifted down 15px to align centers
  lv_anim_set_time(&a_book, 2000);
  lv_anim_set_playback_time(&a_book, 2000);
  lv_anim_set_repeat_count(&a_book, LV_ANIM_REPEAT_INFINITE);
  lv_anim_set_path_cb(&a_book, lv_anim_path_ease_in_out);
  lv_anim_set_exec_cb(&a_book, anim_float_y_cb);
  lv_anim_start(&a_book);

  // Pomo Float (Offset timing)
  lv_anim_t a_pomo;
  lv_anim_init(&a_pomo);
  lv_anim_set_var(&a_pomo, anim_img_pomo);
  lv_anim_set_values(&a_pomo, 10, 20);
  lv_anim_set_time(&a_pomo, 2300); // Slightly different speed
  lv_anim_set_playback_time(&a_pomo, 2300);
  lv_anim_set_repeat_count(&a_pomo, LV_ANIM_REPEAT_INFINITE);
  lv_anim_set_path_cb(&a_pomo, lv_anim_path_ease_in_out);
  lv_anim_set_exec_cb(&a_pomo, anim_float_y_cb);
  lv_anim_start(&a_pomo);

  // --- SECT 1: LCD SCREEN (Retro Style) ---
  pomo_timer_card = lv_obj_create(body);
  lv_obj_set_size(pomo_timer_card, 260, 115); // Widescreen LCD
  lv_obj_align(pomo_timer_card, LV_ALIGN_TOP_MID, 0, 135);

  // Retro LCD Style
  lv_obj_set_style_bg_color(pomo_timer_card, lv_color_hex(0x98A585),
                            0); // Gameboy Greenish
  lv_obj_set_style_bg_opa(pomo_timer_card, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(pomo_timer_card, 4, 0);
  lv_obj_set_style_border_color(pomo_timer_card, lv_color_hex(0x2F3624),
                                0); // Dark Frame
  lv_obj_set_style_radius(pomo_timer_card, 8, 0);
  lv_obj_set_style_shadow_width(pomo_timer_card, 40, 0);
  lv_obj_set_style_shadow_opa(pomo_timer_card, LV_OPA_20, 0); // Subtle depth
  lv_obj_clear_flag(pomo_timer_card, LV_OBJ_FLAG_SCROLLABLE);

  // Manual Layout
  lv_obj_set_style_pad_all(pomo_timer_card, 0, 0);

  // Session Label (Inside LCD)
  lv_obj_t *lbl_sess_num = lv_label_create(pomo_timer_card);
  lv_label_set_text(lbl_sess_num, "SESSION");
  lv_obj_set_style_text_font(lbl_sess_num, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(lbl_sess_num, lv_color_hex(0x1F2418),
                              0); // LCD Ink Color
  lv_obj_set_style_text_letter_space(lbl_sess_num, 2, 0);
  lv_obj_align(lbl_sess_num, LV_ALIGN_TOP_MID, 0, 10); // Top of LCD

  // Timer Label (Huge & Clean - Inside LCD)
  label_pomodoro_time = lv_label_create(pomo_timer_card);
  lv_label_set_text(label_pomodoro_time, "25:00");

  lv_obj_set_style_text_font(label_pomodoro_time, &lv_font_montserrat_48, 0);
  lv_obj_set_style_text_color(label_pomodoro_time, lv_color_hex(0x1F2418),
                              0); // LCD Ink Color
  lv_obj_set_style_opa(label_pomodoro_time, LV_OPA_COVER, 0);
  lv_obj_set_style_transform_zoom(label_pomodoro_time, 256, 0); // 1x

  // Center in LCD
  lv_obj_align(label_pomodoro_time, LV_ALIGN_CENTER, 0, 5);

  // --- SECT 2: SETTINGS (Mid Row) ---
  lv_obj_t *settings_row = lv_obj_create(body);
  lv_obj_set_size(settings_row, 340, 75);
  lv_obj_align(settings_row, LV_ALIGN_TOP_MID, 0,
               265); // Moved up to clear buttons
  lv_obj_set_style_bg_opa(settings_row, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(settings_row, 0, 0);
  lv_obj_clear_flag(settings_row, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_flex_flow(settings_row, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(settings_row, LV_FLEX_ALIGN_SPACE_BETWEEN,
                        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_gap(settings_row, 2, 0);

  // Break Config (Left Pill) - Flex Column Centered
  lv_obj_t *grp_break = lv_obj_create(settings_row);
  lv_obj_set_size(grp_break, 145, 80); // Increased height from 65 to 80
  lv_obj_set_style_bg_color(grp_break, col_btn_dark, 0);
  lv_obj_set_style_radius(grp_break, 15, 0);
  lv_obj_set_style_border_width(grp_break, 0, 0);
  lv_obj_set_style_pad_top(grp_break, 5, 0); // Add top padding
  lv_obj_clear_flag(grp_break, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_flex_flow(grp_break, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(grp_break, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_gap(grp_break, 2, 0);

  lv_obj_t *lbl_brk_t = lv_label_create(grp_break);
  lv_label_set_text(lbl_brk_t, "BREAK");
  lv_obj_set_style_text_color(lbl_brk_t, col_text_white, 0);
  lv_obj_set_style_text_font(lbl_brk_t, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_align(lbl_brk_t, LV_TEXT_ALIGN_CENTER, 0);

  label_break_len = lv_label_create(grp_break);
  lv_label_set_text_fmt(label_break_len, "%d", (int)break_length_mins);
  lv_obj_set_style_text_color(label_break_len, col_display, 0);
  lv_obj_set_style_text_font(label_break_len, &lv_font_montserratMedium_23, 0);
  lv_obj_set_style_text_align(label_break_len, LV_TEXT_ALIGN_CENTER, 0);

  // Break Buttons (Styled Circular Floating)
  // Minus (Left)
  lv_obj_t *btn_bm = lv_btn_create(grp_break);
  lv_obj_set_size(btn_bm, 32, 32);
  lv_obj_align(btn_bm, LV_ALIGN_LEFT_MID, 4, 18); // Lowered by 18px
  lv_obj_set_style_bg_color(btn_bm, lv_color_hex(0x4A4A4A), 0);
  lv_obj_set_style_radius(btn_bm, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_shadow_width(btn_bm, 0, 0);
  lv_obj_add_event_cb(btn_bm, btn_break_minus_cb, LV_EVENT_CLICKED, NULL);
  lv_obj_add_flag(btn_bm, LV_OBJ_FLAG_FLOATING);

  lv_obj_t *l_bbm = lv_label_create(btn_bm);
  lv_label_set_text(l_bbm, LV_SYMBOL_MINUS);
  lv_obj_center(l_bbm);
  lv_obj_set_style_text_color(l_bbm, lv_color_hex(0xFFFFFF), 0);

  // Plus (Right)
  lv_obj_t *btn_bp = lv_btn_create(grp_break);
  lv_obj_set_size(btn_bp, 32, 32);
  lv_obj_align(btn_bp, LV_ALIGN_RIGHT_MID, -4, 18); // Lowered by 18px
  lv_obj_set_style_bg_color(btn_bp, lv_color_hex(0x4A4A4A), 0);
  lv_obj_set_style_radius(btn_bp, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_shadow_width(btn_bp, 0, 0);
  lv_obj_add_event_cb(btn_bp, btn_break_plus_cb, LV_EVENT_CLICKED, NULL);
  lv_obj_add_flag(btn_bp, LV_OBJ_FLAG_FLOATING);

  lv_obj_t *l_bbp = lv_label_create(btn_bp);
  lv_label_set_text(l_bbp, LV_SYMBOL_PLUS);
  lv_obj_center(l_bbp);
  lv_obj_set_style_text_color(l_bbp, lv_color_hex(0xFFFFFF), 0);

  // Session Config (Right Pill) - Flex Column Centered
  pomo_cont_session = lv_obj_create(settings_row);
  lv_obj_set_size(pomo_cont_session, 145, 80); // Increased height
  lv_obj_set_style_bg_color(pomo_cont_session, col_btn_dark, 0);
  lv_obj_set_style_radius(pomo_cont_session, 15, 0);
  lv_obj_set_style_border_width(pomo_cont_session, 0, 0);
  lv_obj_set_style_pad_top(pomo_cont_session, 5, 0); // Add top padding
  lv_obj_clear_flag(pomo_cont_session, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_flex_flow(pomo_cont_session, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(pomo_cont_session, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_gap(pomo_cont_session, 2, 0);

  lv_obj_t *lbl_sess_t = lv_label_create(pomo_cont_session);
  lv_label_set_text(lbl_sess_t, "SESSION");
  lv_obj_set_style_text_color(lbl_sess_t, col_text_white, 0);
  lv_obj_set_style_text_font(lbl_sess_t, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_align(lbl_sess_t, LV_TEXT_ALIGN_CENTER, 0);

  label_session_len = lv_label_create(pomo_cont_session);
  lv_label_set_text_fmt(label_session_len, "%d", (int)session_length_mins);
  lv_obj_set_style_text_color(label_session_len, col_display, 0);
  lv_obj_set_style_text_font(label_session_len, &lv_font_montserratMedium_23,
                             0);
  lv_obj_set_style_text_align(label_session_len, LV_TEXT_ALIGN_CENTER, 0);

  // Overlay Buttons for Interaction
  // Minus (Left)
  lv_obj_t *btn_minus = lv_btn_create(pomo_cont_session);
  lv_obj_set_size(btn_minus, 32, 32);
  lv_obj_align(btn_minus, LV_ALIGN_LEFT_MID, 4, 18); // Lowered by 18px
  lv_obj_set_style_bg_color(btn_minus, lv_color_hex(0x4A4A4A), 0);
  lv_obj_set_style_radius(btn_minus, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_shadow_width(btn_minus, 0, 0);
  lv_obj_add_event_cb(btn_minus, btn_session_minus_cb, LV_EVENT_CLICKED, NULL);
  lv_obj_add_flag(btn_minus, LV_OBJ_FLAG_FLOATING); // Ignore Flex

  lv_obj_t *l_bm = lv_label_create(btn_minus);
  lv_label_set_text(l_bm, LV_SYMBOL_MINUS);
  lv_obj_center(l_bm);
  lv_obj_set_style_text_color(l_bm, lv_color_hex(0xFFFFFF), 0);

  // Plus (Right)
  lv_obj_t *btn_plus = lv_btn_create(pomo_cont_session);
  lv_obj_set_size(btn_plus, 32, 32);
  lv_obj_align(btn_plus, LV_ALIGN_RIGHT_MID, -4, 18); // Lowered by 18px
  lv_obj_set_style_bg_color(btn_plus, lv_color_hex(0x4A4A4A), 0);
  lv_obj_set_style_radius(btn_plus, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_shadow_width(btn_plus, 0, 0);
  lv_obj_add_event_cb(btn_plus, btn_session_plus_cb, LV_EVENT_CLICKED, NULL);
  lv_obj_add_flag(btn_plus, LV_OBJ_FLAG_FLOATING); // Ignore Flex

  lv_obj_t *l_bp = lv_label_create(btn_plus);
  lv_label_set_text(l_bp, LV_SYMBOL_PLUS);
  lv_obj_center(l_bp);
  lv_obj_set_style_text_color(l_bp, lv_color_hex(0xFFFFFF), 0);

  // --- SECT 3: ACTION BUTTONS (BOTTOM) ---
  lv_obj_t *action_row = lv_obj_create(body);
  lv_obj_set_size(action_row, 300, 80);
  lv_obj_align(action_row, LV_ALIGN_BOTTOM_MID, 0, -10); // Shifted lower
  lv_obj_set_style_bg_opa(action_row, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(action_row, 0, 0);
  lv_obj_clear_flag(action_row, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_flex_flow(action_row, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(action_row, LV_FLEX_ALIGN_SPACE_EVENLY,
                        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

  // Reset Button (Small Circular)
  btn_pomodoro_reset = lv_btn_create(action_row);
  lv_obj_set_size(btn_pomodoro_reset, 60, 60);
  lv_obj_set_style_radius(btn_pomodoro_reset, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_color(btn_pomodoro_reset, lv_color_hex(0xE0C040),
                            0); // Yellow/Gold for Reset
  lv_obj_set_style_shadow_width(btn_pomodoro_reset, 6, 0);
  lv_obj_set_style_shadow_ofs_y(btn_pomodoro_reset, 3, 0);
  lv_obj_set_style_shadow_opa(btn_pomodoro_reset, LV_OPA_30, 0);
  lv_obj_set_style_border_width(btn_pomodoro_reset, 2, 0);
  lv_obj_set_style_border_color(btn_pomodoro_reset, lv_color_hex(0xB09030), 0);
  lv_obj_add_event_cb(btn_pomodoro_reset, btn_pomodoro_reset_cb,
                      LV_EVENT_CLICKED, NULL);

  lv_obj_t *lbl_reset = lv_label_create(btn_pomodoro_reset);
  lv_label_set_text(lbl_reset, LV_SYMBOL_REFRESH);
  lv_obj_center(lbl_reset);
  lv_obj_set_style_text_color(lbl_reset, col_text_dark, 0);
  lv_obj_set_style_text_font(lbl_reset, &lv_font_montserratMedium_20, 0);

  // Start Button (Big Capsule)
  btn_pomodoro_start = lv_btn_create(action_row);
  lv_obj_set_size(btn_pomodoro_start, 180, 60);
  lv_obj_set_style_radius(btn_pomodoro_start, 30, 0);
  lv_obj_set_style_bg_color(btn_pomodoro_start, col_btn_accent, 0); // Red
  lv_obj_set_style_shadow_width(btn_pomodoro_start, 8, 0);
  lv_obj_set_style_shadow_ofs_y(btn_pomodoro_start, 4, 0);
  lv_obj_set_style_border_width(btn_pomodoro_start, 0, 0);
  lv_obj_add_event_cb(btn_pomodoro_start, btn_pomodoro_start_cb,
                      LV_EVENT_CLICKED, NULL);

  label_start_text = lv_label_create(btn_pomodoro_start);
  lv_label_set_text(label_start_text, "Start Timer");
  lv_obj_center(label_start_text);
  lv_obj_set_style_text_font(label_start_text, &lv_font_montserratMedium_20, 0);

  // Back Button (Top Edge)
  lv_obj_t *btn_back = lv_btn_create(cont_pomodoro_view);
  lv_obj_set_size(btn_back, 100, 50); // Increased size
  lv_obj_align(btn_back, LV_ALIGN_TOP_MID, 0, 10);
  lv_obj_set_style_bg_opa(btn_back, LV_OPA_TRANSP, 0);
  lv_obj_set_style_shadow_width(btn_back, 0, 0);
  lv_obj_add_event_cb(btn_back, back_to_launcher_cb, LV_EVENT_CLICKED, NULL);

  lv_obj_t *lbl_back = lv_label_create(btn_back);
  lv_label_set_text(lbl_back, LV_SYMBOL_UP);
  lv_obj_set_style_text_font(lbl_back, &lv_font_montserratMedium_23,
                             0); // Larger Icon
  lv_obj_set_style_text_color(lbl_back, lv_color_hex(0x888888), 0);
  lv_obj_center(lbl_back);

  // Cleanup
  arc_pomodoro = NULL;
  pomo_ambient_glow = NULL;
  pomo_action_bar = NULL;
  anim_img_book = NULL;
}

static void back_to_launcher_cb(lv_event_t *e) { show_launcher_panel(); }

// -----------------------------------------------------------------------------
// Global WiFi Status Indicator (Top Right Corner)
// -----------------------------------------------------------------------------
static void create_global_wifi_indicator(void) {
  if (global_wifi_indicator != NULL) {
    return; // Already created
  }

  // Create indicator on main settings screen (always on top)
  global_wifi_indicator = lv_label_create(scr_settings);
  lv_label_set_text(global_wifi_indicator, LV_SYMBOL_WIFI);

  // Position for round display - Safe Zone (avoid corners)
  // Use TOP_MID and offset to the right (approx 1 o'clock position)
  lv_obj_align(global_wifi_indicator, LV_ALIGN_TOP_MID, 65, 25);

  // Larger font for better visibility
  extern lv_font_t lv_font_montserratMedium_20;
  lv_obj_set_style_text_font(global_wifi_indicator,
                             &lv_font_montserratMedium_20, 0);
  lv_obj_set_style_text_color(global_wifi_indicator, lv_color_hex(0x888888),
                              0); // Gray (disconnected)

  // Add subtle background for visibility
  lv_obj_set_style_bg_opa(global_wifi_indicator, LV_OPA_30, 0);
  lv_obj_set_style_bg_color(global_wifi_indicator, lv_color_black(), 0);
  lv_obj_set_style_radius(global_wifi_indicator, 4, 0);
  lv_obj_set_style_pad_all(global_wifi_indicator, 3, 0);

  // Keep on top of all other elements
  lv_obj_move_foreground(global_wifi_indicator);

  printf("[WIFI_INDICATOR] Created WiFi indicator at top-right corner (always "
         "visible)\n");
}

static void update_global_wifi_indicator(bool connected) {
  if (global_wifi_indicator == NULL) {
    return;
  }
  global_wifi_connected = connected;

  if (connected) {
    // Bright green when connected
    lv_obj_set_style_text_color(global_wifi_indicator, lv_color_hex(0x4CAF50),
                                0);
    printf("[WIFI_INDICATOR] Status: CONNECTED (Green)\n");
  } else {
    // Gray when disconnected
    lv_obj_set_style_text_color(global_wifi_indicator, lv_color_hex(0x888888),
                                0);
    printf("[WIFI_INDICATOR] Status: DISCONNECTED (Gray)\n");
  }

  // Ensure it stays on top
  lv_obj_move_foreground(global_wifi_indicator);
}

static void arc_brightness_cb(lv_event_t *e) {
  lv_obj_t *arc = lv_event_get_target(e);
  int32_t val = lv_arc_get_value(arc);

  if (label_brightness_val) {
    lv_label_set_text_fmt(label_brightness_val, "%d%%", (int)val);
  }

  // Convert 0-100% to 0-255
  uint8_t hw_val = (uint8_t)((val * 255) / 100);
  saved_brightness = hw_val; // Store for NVS

  if (brightness_cb) {
    brightness_cb(hw_val);
  }

  save_settings_to_nvs();
}

// -----------------------------------------------------------------------------
// Helper: Create iOS-style App Icon (Squircle) - Scaled Up for Carousel
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// Layout Builders
// -----------------------------------------------------------------------------

static void create_launcher_view(void) {
  if (cont_launcher)
    return;

  cont_launcher = lv_obj_create(scr_settings);
  lv_obj_set_size(cont_launcher, LV_PCT(100), LV_PCT(100));
  lv_obj_set_style_bg_opa(cont_launcher, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(cont_launcher, 0, 0);

  // Carousel Flow (Center Snapping)
  lv_obj_set_flex_flow(cont_launcher, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(cont_launcher, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

  // Enable Snapping
  lv_obj_set_scroll_snap_y(cont_launcher, LV_SCROLL_SNAP_CENTER);
  lv_obj_clear_flag(cont_launcher, LV_OBJ_FLAG_SCROLL_ELASTIC); // Sharp snaps
  lv_obj_add_flag(cont_launcher, LV_OBJ_FLAG_SCROLL_ONE);       // One at a time

  // Padding to ensure first/last items can be centered
  // Screen height ~466. Item height 200.
  // Need top/bottom pad ~130.
  lv_obj_set_style_pad_top(cont_launcher, 130, 0);
  lv_obj_set_style_pad_bottom(cont_launcher, 130, 0);
  lv_obj_set_style_pad_gap(cont_launcher, 40, 0); // Gap between cards

  // 1. Deskbot (Home)
  lv_obj_t *cont_deskbot = lv_obj_create(cont_launcher);
  lv_obj_set_size(cont_deskbot, 180, 200);
  lv_obj_set_style_bg_opa(cont_deskbot, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(cont_deskbot, 0, 0);
  lv_obj_set_flex_flow(cont_deskbot, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(cont_deskbot, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_clear_flag(cont_deskbot, LV_OBJ_FLAG_SCROLLABLE);

  // Image Button
  lv_obj_t *btn_deskbot = lv_imgbtn_create(cont_deskbot);
  lv_imgbtn_set_src(btn_deskbot, LV_IMGBTN_STATE_RELEASED, NULL, &deskbot_icon,
                    NULL);
  lv_imgbtn_set_src(btn_deskbot, LV_IMGBTN_STATE_PRESSED, NULL, &deskbot_icon,
                    NULL);
  lv_obj_set_size(btn_deskbot, 140, 140);

  // Press effect
  static lv_style_t style_pr_deskbot;
  if (style_pr_deskbot.prop_cnt == 0) {
    lv_style_init(&style_pr_deskbot);
    lv_style_set_img_recolor_opa(&style_pr_deskbot, LV_OPA_30);
    lv_style_set_img_recolor(&style_pr_deskbot, lv_color_black());
    lv_style_set_transform_zoom(&style_pr_deskbot,
                                240); // Slight shrink (256 is 100%)
  }
  lv_obj_add_style(btn_deskbot, &style_pr_deskbot, LV_STATE_PRESSED);

  lv_obj_add_event_cb(btn_deskbot, app_home_cb, LV_EVENT_CLICKED, NULL);

  // Label
  lv_obj_t *lbl_deskbot = lv_label_create(cont_deskbot);
  lv_label_set_text(lbl_deskbot, "Deskbot");
  lv_obj_set_style_text_color(lbl_deskbot, lv_color_white(), 0);
  lv_obj_set_style_text_font(lbl_deskbot, LV_FONT_DEFAULT, 0);
  lv_obj_set_style_pad_top(lbl_deskbot, 10, 0);

  // 2. Pomodoro App
  lv_obj_t *cont_pomodoro = lv_obj_create(cont_launcher);
  lv_obj_set_size(cont_pomodoro, 180, 200);
  lv_obj_set_style_bg_opa(cont_pomodoro, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(cont_pomodoro, 0, 0);
  lv_obj_set_flex_flow(cont_pomodoro, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(cont_pomodoro, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_clear_flag(cont_pomodoro, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *btn_pomodoro = lv_imgbtn_create(cont_pomodoro);
  lv_imgbtn_set_src(btn_pomodoro, LV_IMGBTN_STATE_RELEASED, NULL,
                    &pomodoro_icon, NULL);
  lv_imgbtn_set_src(btn_pomodoro, LV_IMGBTN_STATE_PRESSED, NULL, &pomodoro_icon,
                    NULL);
  lv_obj_set_size(btn_pomodoro, 140, 140);

  // Press effect
  static lv_style_t style_pr_pomodoro;
  if (style_pr_pomodoro.prop_cnt == 0) {
    lv_style_init(&style_pr_pomodoro);
    lv_style_set_img_recolor_opa(&style_pr_pomodoro, LV_OPA_30);
    lv_style_set_img_recolor(&style_pr_pomodoro, lv_color_black());
    lv_style_set_transform_zoom(&style_pr_pomodoro, 240);
  }
  lv_obj_add_style(btn_pomodoro, &style_pr_pomodoro, LV_STATE_PRESSED);

  lv_obj_add_event_cb(btn_pomodoro, app_pomodoro_cb, LV_EVENT_CLICKED, NULL);

  lv_obj_t *lbl_pomodoro = lv_label_create(cont_pomodoro);
  lv_label_set_text(lbl_pomodoro, "Pomodoro");
  lv_obj_set_style_text_color(lbl_pomodoro, lv_color_white(), 0);
  lv_obj_set_style_text_font(lbl_pomodoro, LV_FONT_DEFAULT, 0);
  lv_obj_set_style_pad_top(lbl_pomodoro, 10, 0);

  // 3. Settings (Brightness) - Using Image Button
  // Container (transparent, for layout)
  lv_obj_t *cont_settings = lv_obj_create(cont_launcher);
  lv_obj_set_size(cont_settings, 180, 200); // Standard layout size
  lv_obj_set_style_bg_opa(cont_settings, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(cont_settings, 0, 0);
  lv_obj_set_flex_flow(cont_settings, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(cont_settings, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_clear_flag(cont_settings, LV_OBJ_FLAG_SCROLLABLE);

  // Image Button
  lv_obj_t *imgbtn = lv_imgbtn_create(cont_settings);
  lv_imgbtn_set_src(imgbtn, LV_IMGBTN_STATE_RELEASED, NULL, &settings_icon,
                    NULL);
  lv_imgbtn_set_src(imgbtn, LV_IMGBTN_STATE_PRESSED, NULL, &settings_icon,
                    NULL); // Use same image for pressed for now

  // Visual tweaks for the Image Button
  lv_obj_set_size(imgbtn, 140, 140); // Enforce size
  lv_obj_add_event_cb(imgbtn, app_settings_cb, LV_EVENT_CLICKED, NULL);

  // Optional: Add a transform style on press to give feedback since we don't
  // have a separate pressed image
  static lv_style_t style_pr;
  if (style_pr.prop_cnt == 0) {
    lv_style_init(&style_pr);
    lv_style_set_img_recolor_opa(&style_pr, LV_OPA_30);
    lv_style_set_img_recolor(&style_pr, lv_color_black());
    lv_style_set_transform_zoom(&style_pr,
                                240); // Zoom out slightly (256 is 1.0)
  }
  lv_obj_add_style(imgbtn, &style_pr, LV_STATE_PRESSED);

  // App Label
  lv_obj_t *lbl = lv_label_create(cont_settings);
  lv_label_set_text(lbl, "Settings");
  lv_obj_set_style_text_color(lbl, lv_color_white(), 0);
  lv_obj_set_style_text_font(lbl, LV_FONT_DEFAULT, 0);
  lv_obj_set_style_pad_top(lbl, 10, 0);
}

static void create_brightness_view(void) {
  if (cont_brightness)
    return;

  extern lv_font_t lv_font_montserratMedium_20;

  // =========================================================================
  // RETRO GADGET THEME - Matching Pomodoro Style
  // =========================================================================
  lv_color_t col_bg = lv_color_hex(0xBCAD82);        // Retro Beige
  lv_color_t col_lcd_bg = lv_color_hex(0x98A585);    // Gameboy Green LCD
  lv_color_t col_lcd_ink = lv_color_hex(0x1F2418);   // Dark LCD Ink
  lv_color_t col_track = lv_color_hex(0x6B5D4A);     // Brown Track
  lv_color_t col_indicator = lv_color_hex(0xD95E52); // Retro Orange/Red
  lv_color_t col_btn_dark = lv_color_hex(0x2C2C2C);  // Dark Button

  cont_brightness = lv_obj_create(scr_settings);
  lv_obj_set_size(cont_brightness, LV_PCT(100), LV_PCT(100));
  lv_obj_set_style_bg_color(cont_brightness, col_bg, 0);
  lv_obj_set_style_bg_opa(cont_brightness, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(cont_brightness, 0, 0);
  lv_obj_add_flag(cont_brightness, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(cont_brightness, LV_OBJ_FLAG_SCROLLABLE);

  // --- Main Arc Slider (Retro Knob Style) ---
  lv_obj_t *arc = lv_arc_create(cont_brightness);
  lv_obj_set_size(arc, 320, 320);
  lv_obj_center(arc);
  lv_arc_set_rotation(arc, 135);
  lv_arc_set_bg_angles(arc, 0, 270);
  lv_arc_set_value(arc, 50);

  // Track (Brown groove)
  lv_obj_set_style_arc_color(arc, col_track, LV_PART_MAIN);
  lv_obj_set_style_arc_width(arc, 25, LV_PART_MAIN);
  lv_obj_set_style_arc_rounded(arc, true, LV_PART_MAIN);

  // Indicator (Orange fill)
  lv_obj_set_style_arc_color(arc, col_indicator, LV_PART_INDICATOR);
  lv_obj_set_style_arc_width(arc, 25, LV_PART_INDICATOR);
  lv_obj_set_style_arc_rounded(arc, true, LV_PART_INDICATOR);

  // Knob (Dark tactile button)
  lv_obj_set_style_bg_color(arc, col_btn_dark, LV_PART_KNOB);
  lv_obj_set_style_pad_all(arc, 8, LV_PART_KNOB);
  lv_obj_set_style_border_width(arc, 3, LV_PART_KNOB);
  lv_obj_set_style_border_color(arc, lv_color_hex(0x1A1A1A), LV_PART_KNOB);
  lv_obj_set_style_shadow_width(arc, 0, LV_PART_KNOB);

  lv_obj_add_event_cb(arc, arc_brightness_cb, LV_EVENT_VALUE_CHANGED, NULL);
  slider_brightness = arc;

  // --- Center LCD Display (Gameboy Style) ---
  cont_brightness_info = lv_obj_create(cont_brightness);
  lv_obj_set_size(cont_brightness_info, 180, 120);
  lv_obj_center(cont_brightness_info);
  lv_obj_set_style_radius(cont_brightness_info, 10, 0);
  lv_obj_set_style_bg_color(cont_brightness_info, col_lcd_bg, 0);
  lv_obj_set_style_bg_opa(cont_brightness_info, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(cont_brightness_info, 4, 0);
  lv_obj_set_style_border_color(cont_brightness_info, lv_color_hex(0x2F3624),
                                0);
  lv_obj_set_style_shadow_width(cont_brightness_info, 0, 0);
  lv_obj_clear_flag(cont_brightness_info, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_set_flex_flow(cont_brightness_info, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(cont_brightness_info, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_gap(cont_brightness_info, 2, 0);

  // --- Title Label (LCD Ink) ---
  lv_obj_t *lbl_title = lv_label_create(cont_brightness_info);
  lv_label_set_text(lbl_title, "BRIGHTNESS");
  lv_obj_set_style_text_font(lbl_title, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(lbl_title, col_lcd_ink, 0);
  lv_obj_set_style_text_letter_space(lbl_title, 2, 0);

  // --- Percentage Value (Big LCD Numbers) ---
  label_brightness_val = lv_label_create(cont_brightness_info);
  lv_label_set_text(label_brightness_val, "50%");
  lv_obj_set_style_text_font(label_brightness_val, &lv_font_montserrat_48, 0);
  lv_obj_set_style_text_color(label_brightness_val, col_lcd_ink, 0);

  // --- Done Button (Dark Retro Button) ---
  btn_brightness_done = lv_btn_create(cont_brightness);
  lv_obj_set_size(btn_brightness_done, 100, 50);
  lv_obj_align(btn_brightness_done, LV_ALIGN_BOTTOM_MID, 0, -35);
  lv_obj_set_style_radius(btn_brightness_done, 10, 0);
  lv_obj_set_style_bg_color(btn_brightness_done, col_btn_dark, 0);
  lv_obj_set_style_border_width(btn_brightness_done, 2, 0);
  lv_obj_set_style_border_color(btn_brightness_done, lv_color_hex(0x1A1A1A), 0);
  lv_obj_set_style_shadow_width(btn_brightness_done, 0, 0);
  lv_obj_add_event_cb(btn_brightness_done, back_to_launcher_cb,
                      LV_EVENT_CLICKED, NULL);

  lv_obj_t *lbl_done = lv_label_create(btn_brightness_done);
  lv_label_set_text(lbl_done, LV_SYMBOL_OK);
  lv_obj_center(lbl_done);
  lv_obj_set_style_text_font(lbl_done, &lv_font_montserratMedium_20, 0);
  lv_obj_set_style_text_color(lbl_done, lv_color_white(), 0);
}

static void show_brightness_panel(void) {
  if (cont_launcher)
    lv_obj_add_flag(cont_launcher, LV_OBJ_FLAG_HIDDEN);
  if (cont_settings_menu)
    lv_obj_add_flag(cont_settings_menu, LV_OBJ_FLAG_HIDDEN);
  if (cont_wifi)
    lv_obj_add_flag(cont_wifi, LV_OBJ_FLAG_HIDDEN);
  if (cont_brightness) {
    lv_obj_clear_flag(cont_brightness, LV_OBJ_FLAG_HIDDEN);
    animate_brightness_view();
  }
}

static void show_wifi_panel(void) {
  if (cont_launcher)
    lv_obj_add_flag(cont_launcher, LV_OBJ_FLAG_HIDDEN);
  if (cont_settings_menu)
    lv_obj_add_flag(cont_settings_menu, LV_OBJ_FLAG_HIDDEN);
  if (cont_brightness)
    lv_obj_add_flag(cont_brightness, LV_OBJ_FLAG_HIDDEN);
  if (cont_wifi)
    lv_obj_clear_flag(cont_wifi, LV_OBJ_FLAG_HIDDEN);
}

static void show_settings_menu(void) {
  if (cont_launcher)
    lv_obj_add_flag(cont_launcher, LV_OBJ_FLAG_HIDDEN);
  if (cont_brightness)
    lv_obj_add_flag(cont_brightness, LV_OBJ_FLAG_HIDDEN);
  if (cont_wifi)
    lv_obj_add_flag(cont_wifi, LV_OBJ_FLAG_HIDDEN);
  if (cont_settings_menu)
    lv_obj_clear_flag(cont_settings_menu, LV_OBJ_FLAG_HIDDEN);
}

static void show_launcher_panel(void) {
  if (cont_brightness)
    lv_obj_add_flag(cont_brightness, LV_OBJ_FLAG_HIDDEN);
  if (cont_pomodoro_view)
    lv_obj_add_flag(cont_pomodoro_view, LV_OBJ_FLAG_HIDDEN);
  if (cont_settings_menu)
    lv_obj_add_flag(cont_settings_menu, LV_OBJ_FLAG_HIDDEN);
  if (cont_wifi)
    lv_obj_add_flag(cont_wifi, LV_OBJ_FLAG_HIDDEN);
  if (cont_launcher)
    lv_obj_clear_flag(cont_launcher, LV_OBJ_FLAG_HIDDEN);
}

// Callbacks for Settings Menu
static void menu_brightness_cb(lv_event_t *e) { show_brightness_panel(); }
static void menu_wifi_cb(lv_event_t *e) { show_wifi_panel(); }
static void back_to_settings_menu_cb(lv_event_t *e) { show_settings_menu(); }

// WiFi Switch Toggle Callback
static void wifi_switch_cb(lv_event_t *e) {
  lv_obj_t *sw = lv_event_get_target(e);
  bool is_on = lv_obj_has_state(sw, LV_STATE_CHECKED);

  // Save WiFi enabled state
  saved_wifi_enabled = is_on;
  save_settings_to_nvs();

  if (is_on) {
    // WiFi enabled - trigger scan
    if (wifi_scan_cb) {
      // Clear existing networks and show scanning message
      if (wifi_net_list) {
        lv_obj_clean(wifi_net_list);
        lv_obj_t *lbl_scan = lv_label_create(wifi_net_list);
        lv_label_set_text(lbl_scan, "Scanning...");
        lv_obj_set_style_text_font(lbl_scan, &lv_font_montserrat_14, 0);
        lv_obj_set_style_text_color(lbl_scan, lv_color_hex(0x1F2418), 0);
      }
      wifi_scan_cb();
    }
  } else {
    // WiFi disabled - clear networks and show placeholder
    if (wifi_net_list) {
      lv_obj_clean(wifi_net_list);
      lv_obj_t *lbl_off = lv_label_create(wifi_net_list);
      lv_label_set_text(lbl_off, "WiFi is OFF");
      lv_obj_set_style_text_font(lbl_off, &lv_font_montserrat_14, 0);
      lv_obj_set_style_text_color(lbl_off, lv_color_hex(0x1F2418), 0);
    }
    // Update status to disconnected
    if (wifi_lbl_ssid)
      lv_label_set_text(wifi_lbl_ssid, "Not Connected");
    if (wifi_lbl_status) {
      lv_label_set_text(wifi_lbl_status, "Disconnected");
      lv_obj_set_style_text_color(wifi_lbl_status, lv_color_hex(0xD95E52), 0);
    }
    if (wifi_lbl_ip)
      lv_label_set_text(wifi_lbl_ip, "IP: ---");
  }
}

// Forward declare
static void show_wifi_password_dialog(const char *ssid);

// Password Dialog - Cancel Button Callback
static void wifi_pwd_cancel_cb(lv_event_t *e) {
  if (wifi_pwd_dialog) {
    lv_obj_del(wifi_pwd_dialog);
    wifi_pwd_dialog = NULL;
    wifi_pwd_input = NULL;
    wifi_pwd_kb = NULL;
  }
}

// Password Dialog - Connect Button Callback
static void wifi_pwd_connect_cb(lv_event_t *e) {
  if (wifi_pwd_input && wifi_connect_cb) {
    const char *password = lv_textarea_get_text(wifi_pwd_input);
    wifi_connect_cb(wifi_selected_ssid, password);
  }

  // Close dialog
  if (wifi_pwd_dialog) {
    lv_obj_del(wifi_pwd_dialog);
    wifi_pwd_dialog = NULL;
    wifi_pwd_input = NULL;
    wifi_pwd_kb = NULL;
  }

  // Update status to connecting
  if (wifi_lbl_status) {
    lv_label_set_text(wifi_lbl_status, "Connecting...");
    lv_obj_set_style_text_color(wifi_lbl_status, lv_color_hex(0xFFC107), 0);
  }
  if (wifi_lbl_ssid) {
    lv_label_set_text(wifi_lbl_ssid, wifi_selected_ssid);
  }
}

// Password Dialog - Keyboard Ready Callback
static void wifi_pwd_kb_ready_cb(lv_event_t *e) {
  // Enter pressed on keyboard - same as connect button
  wifi_pwd_connect_cb(e);
}

// Show Password Dialog
static void show_wifi_password_dialog(const char *ssid) {
  if (wifi_pwd_dialog)
    return; // Already showing

  // Store selected SSID
  strncpy(wifi_selected_ssid, ssid, sizeof(wifi_selected_ssid) - 1);
  wifi_selected_ssid[sizeof(wifi_selected_ssid) - 1] = '\0';

  extern lv_font_t lv_font_montserratMedium_20;

  // Colors
  lv_color_t col_bg = lv_color_hex(0xBCAD82);
  lv_color_t col_lcd = lv_color_hex(0x98A585);
  lv_color_t col_ink = lv_color_hex(0x1F2418);
  lv_color_t col_btn = lv_color_hex(0x2C2C2C);
  lv_color_t col_green = lv_color_hex(0x4CAF50);
  lv_color_t col_red = lv_color_hex(0xD95E52);

  // Full screen overlay
  wifi_pwd_dialog = lv_obj_create(cont_wifi);
  lv_obj_set_size(wifi_pwd_dialog, 466, 466);
  lv_obj_center(wifi_pwd_dialog);
  lv_obj_set_style_bg_color(wifi_pwd_dialog, col_bg, 0);
  lv_obj_set_style_bg_opa(wifi_pwd_dialog, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(wifi_pwd_dialog, 0, 0);
  lv_obj_clear_flag(wifi_pwd_dialog, LV_OBJ_FLAG_SCROLLABLE);

  // Dialog Card
  lv_obj_t *card = lv_obj_create(wifi_pwd_dialog);
  lv_obj_set_size(card, 380, 180);
  lv_obj_align(card, LV_ALIGN_TOP_MID, 0, 30);
  lv_obj_set_style_radius(card, 15, 0);
  lv_obj_set_style_bg_color(card, col_lcd, 0);
  lv_obj_set_style_border_width(card, 3, 0);
  lv_obj_set_style_border_color(card, lv_color_hex(0x2F3624), 0);
  lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

  // Title
  lv_obj_t *lbl_title = lv_label_create(card);
  lv_label_set_text(lbl_title, "Enter Password");
  lv_obj_align(lbl_title, LV_ALIGN_TOP_MID, 0, 10);
  lv_obj_set_style_text_font(lbl_title, &lv_font_montserratMedium_20, 0);
  lv_obj_set_style_text_color(lbl_title, col_ink, 0);

  // SSID Label
  lv_obj_t *lbl_ssid = lv_label_create(card);
  lv_label_set_text_fmt(lbl_ssid, "Network: %s", ssid);
  lv_obj_align(lbl_ssid, LV_ALIGN_TOP_MID, 0, 40);
  lv_obj_set_style_text_font(lbl_ssid, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(lbl_ssid, col_ink, 0);

  // Password Input
  wifi_pwd_input = lv_textarea_create(card);
  lv_obj_set_size(wifi_pwd_input, 340, 45);
  lv_obj_align(wifi_pwd_input, LV_ALIGN_TOP_MID, 0, 65);
  lv_textarea_set_placeholder_text(wifi_pwd_input, "Password");
  lv_textarea_set_password_mode(wifi_pwd_input, true);
  lv_textarea_set_one_line(wifi_pwd_input, true);
  lv_obj_set_style_bg_color(wifi_pwd_input, lv_color_hex(0xF4F8F0), 0);
  lv_obj_set_style_border_width(wifi_pwd_input, 2, 0);
  lv_obj_set_style_border_color(wifi_pwd_input, col_ink, 0);
  lv_obj_set_style_text_color(wifi_pwd_input, col_ink, 0);
  lv_obj_set_style_radius(wifi_pwd_input, 8, 0);

  // Button Row
  lv_obj_t *btn_row = lv_obj_create(card);
  lv_obj_set_size(btn_row, 340, 50);
  lv_obj_align(btn_row, LV_ALIGN_BOTTOM_MID, 0, -5);
  lv_obj_set_style_bg_opa(btn_row, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(btn_row, 0, 0);
  lv_obj_clear_flag(btn_row, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_flex_flow(btn_row, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(btn_row, LV_FLEX_ALIGN_SPACE_EVENLY,
                        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

  // Cancel Button
  lv_obj_t *btn_cancel = lv_btn_create(btn_row);
  lv_obj_set_size(btn_cancel, 120, 40);
  lv_obj_set_style_radius(btn_cancel, 10, 0);
  lv_obj_set_style_bg_color(btn_cancel, col_red, 0);
  lv_obj_add_event_cb(btn_cancel, wifi_pwd_cancel_cb, LV_EVENT_CLICKED, NULL);

  lv_obj_t *lbl_cancel = lv_label_create(btn_cancel);
  lv_label_set_text(lbl_cancel, "Cancel");
  lv_obj_center(lbl_cancel);
  lv_obj_set_style_text_color(lbl_cancel, lv_color_white(), 0);

  // Connect Button
  lv_obj_t *btn_connect = lv_btn_create(btn_row);
  lv_obj_set_size(btn_connect, 120, 40);
  lv_obj_set_style_radius(btn_connect, 10, 0);
  lv_obj_set_style_bg_color(btn_connect, col_green, 0);
  lv_obj_add_event_cb(btn_connect, wifi_pwd_connect_cb, LV_EVENT_CLICKED, NULL);

  lv_obj_t *lbl_connect = lv_label_create(btn_connect);
  lv_label_set_text(lbl_connect, "Connect");
  lv_obj_center(lbl_connect);
  lv_obj_set_style_text_color(lbl_connect, lv_color_white(), 0);

  // Keyboard
  wifi_pwd_kb = lv_keyboard_create(wifi_pwd_dialog);
  lv_obj_set_size(wifi_pwd_kb, 440, 220);
  lv_obj_align(wifi_pwd_kb, LV_ALIGN_BOTTOM_MID, 0, -10);
  lv_keyboard_set_textarea(wifi_pwd_kb, wifi_pwd_input);
  lv_obj_set_style_bg_color(wifi_pwd_kb, col_btn, 0);
  lv_obj_set_style_bg_color(wifi_pwd_kb, lv_color_hex(0x4A4A4A), LV_PART_ITEMS);
  lv_obj_set_style_text_color(wifi_pwd_kb, lv_color_white(), LV_PART_ITEMS);
  lv_obj_add_event_cb(wifi_pwd_kb, wifi_pwd_kb_ready_cb, LV_EVENT_READY, NULL);
}

// Network Item Click Callback
static void wifi_network_item_cb(lv_event_t *e) {
  const char *ssid = (const char *)lv_event_get_user_data(e);

  if (ssid) {
    show_wifi_password_dialog(ssid);
  }
}

// Helper: Get RSSI color based on signal strength
static lv_color_t get_rssi_color(int rssi) {
  if (rssi >= -50) {
    return lv_color_hex(0x4CAF50); // Green - Excellent
  } else if (rssi >= -60) {
    return lv_color_hex(0x8BC34A); // Light Green - Good
  } else if (rssi >= -70) {
    return lv_color_hex(0xFFC107); // Orange - Fair
  } else {
    return lv_color_hex(0xD95E52); // Red - Weak
  }
}

// -----------------------------------------------------------------------------
// Settings Menu View (Retro Style)
// -----------------------------------------------------------------------------
static void create_settings_menu(void) {
  if (cont_settings_menu)
    return;

  extern lv_font_t lv_font_montserratMedium_20;

  // Retro Colors
  lv_color_t col_bg = lv_color_hex(0xBCAD82);
  lv_color_t col_card = lv_color_hex(0x98A585);
  lv_color_t col_ink = lv_color_hex(0x1F2418);
  lv_color_t col_btn = lv_color_hex(0x2C2C2C);

  cont_settings_menu = lv_obj_create(scr_settings);
  lv_obj_set_size(cont_settings_menu, LV_PCT(100), LV_PCT(100));
  lv_obj_set_style_bg_color(cont_settings_menu, col_bg, 0);
  lv_obj_set_style_bg_opa(cont_settings_menu, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(cont_settings_menu, 0, 0);
  lv_obj_add_flag(cont_settings_menu, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(cont_settings_menu, LV_OBJ_FLAG_SCROLLABLE);

  // Title
  lv_obj_t *lbl_title = lv_label_create(cont_settings_menu);
  lv_label_set_text(lbl_title, "SETTINGS");
  lv_obj_align(lbl_title, LV_ALIGN_TOP_MID, 0, 40);
  lv_obj_set_style_text_font(lbl_title, &lv_font_montserratMedium_20, 0);
  lv_obj_set_style_text_color(lbl_title, col_ink, 0);
  lv_obj_set_style_text_letter_space(lbl_title, 3, 0);

  // Menu Container
  lv_obj_t *menu_cont = lv_obj_create(cont_settings_menu);
  lv_obj_set_size(menu_cont, 280, 280);
  lv_obj_center(menu_cont);
  lv_obj_set_style_bg_opa(menu_cont, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(menu_cont, 0, 0);
  lv_obj_set_flex_flow(menu_cont, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(menu_cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_gap(menu_cont, 20, 0);
  lv_obj_clear_flag(menu_cont, LV_OBJ_FLAG_SCROLLABLE);

  // --- Brightness Button ---
  lv_obj_t *btn_brightness = lv_btn_create(menu_cont);
  lv_obj_set_size(btn_brightness, 220, 70);
  lv_obj_set_style_radius(btn_brightness, 15, 0);
  lv_obj_set_style_bg_color(btn_brightness, col_card, 0);
  lv_obj_set_style_border_width(btn_brightness, 3, 0);
  lv_obj_set_style_border_color(btn_brightness, lv_color_hex(0x2F3624), 0);
  lv_obj_set_style_shadow_width(btn_brightness, 0, 0);
  lv_obj_add_event_cb(btn_brightness, menu_brightness_cb, LV_EVENT_CLICKED,
                      NULL);

  lv_obj_t *lbl_br = lv_label_create(btn_brightness);
  lv_label_set_text(lbl_br, LV_SYMBOL_IMAGE "  Brightness");
  lv_obj_center(lbl_br);
  lv_obj_set_style_text_font(lbl_br, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(lbl_br, col_ink, 0);

  // --- WiFi Button ---
  lv_obj_t *btn_wifi = lv_btn_create(menu_cont);
  lv_obj_set_size(btn_wifi, 220, 70);
  lv_obj_set_style_radius(btn_wifi, 15, 0);
  lv_obj_set_style_bg_color(btn_wifi, col_card, 0);
  lv_obj_set_style_border_width(btn_wifi, 3, 0);
  lv_obj_set_style_border_color(btn_wifi, lv_color_hex(0x2F3624), 0);
  lv_obj_set_style_shadow_width(btn_wifi, 0, 0);
  lv_obj_add_event_cb(btn_wifi, menu_wifi_cb, LV_EVENT_CLICKED, NULL);

  lv_obj_t *lbl_wifi = lv_label_create(btn_wifi);
  lv_label_set_text(lbl_wifi, LV_SYMBOL_WIFI "  WiFi");
  lv_obj_center(lbl_wifi);
  lv_obj_set_style_text_font(lbl_wifi, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(lbl_wifi, col_ink, 0);

  // --- Back Button ---
  lv_obj_t *btn_back = lv_btn_create(cont_settings_menu);
  lv_obj_set_size(btn_back, 80, 50);
  lv_obj_align(btn_back, LV_ALIGN_BOTTOM_MID, 0, -30);
  lv_obj_set_style_radius(btn_back, 10, 0);
  lv_obj_set_style_bg_color(btn_back, col_btn, 0);
  lv_obj_set_style_shadow_width(btn_back, 0, 0);
  lv_obj_add_event_cb(btn_back, back_to_launcher_cb, LV_EVENT_CLICKED, NULL);

  lv_obj_t *lbl_back = lv_label_create(btn_back);
  lv_label_set_text(lbl_back, LV_SYMBOL_LEFT);
  lv_obj_center(lbl_back);
  lv_obj_set_style_text_color(lbl_back, lv_color_white(), 0);
}

// -----------------------------------------------------------------------------
// WiFi View (Retro Style)
// -----------------------------------------------------------------------------
static void create_wifi_view(void) {
  if (cont_wifi)
    return;

  extern lv_font_t lv_font_montserratMedium_20;

  // Retro Colors
  lv_color_t col_bg = lv_color_hex(0xBCAD82);
  lv_color_t col_lcd = lv_color_hex(0x98A585);
  lv_color_t col_ink = lv_color_hex(0x1F2418);
  lv_color_t col_btn = lv_color_hex(0x2C2C2C);
  lv_color_t col_green = lv_color_hex(0x4CAF50);
  lv_color_t col_red = lv_color_hex(0xD95E52);

  cont_wifi = lv_obj_create(scr_settings);
  lv_obj_set_size(cont_wifi, LV_PCT(100), LV_PCT(100));
  lv_obj_set_style_bg_color(cont_wifi, col_bg, 0);
  lv_obj_set_style_bg_opa(cont_wifi, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(cont_wifi, 0, 0);
  lv_obj_add_flag(cont_wifi, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(cont_wifi, LV_OBJ_FLAG_SCROLLABLE);

  // --- Header Row (Title + Toggle) ---
  lv_obj_t *header = lv_obj_create(cont_wifi);
  lv_obj_set_size(header, 360, 50);
  lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 35);
  lv_obj_set_style_bg_opa(header, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(header, 0, 0);
  lv_obj_clear_flag(header, LV_OBJ_FLAG_SCROLLABLE);

  // Back Button (Left)
  lv_obj_t *btn_back = lv_btn_create(header);
  lv_obj_set_size(btn_back, 40, 32);
  lv_obj_align(btn_back, LV_ALIGN_LEFT_MID, 0, 0);
  lv_obj_set_style_radius(btn_back, 8, 0);
  lv_obj_set_style_bg_color(btn_back, col_btn, 0);
  lv_obj_set_style_shadow_width(btn_back, 0, 0);
  lv_obj_add_event_cb(btn_back, back_to_settings_menu_cb, LV_EVENT_CLICKED,
                      NULL);

  lv_obj_t *lbl_back = lv_label_create(btn_back);
  lv_label_set_text(lbl_back, LV_SYMBOL_LEFT);
  lv_obj_center(lbl_back);
  lv_obj_set_style_text_color(lbl_back, lv_color_white(), 0);

  // Title (Center)
  lv_obj_t *lbl_title = lv_label_create(header);
  lv_label_set_text(lbl_title, "WiFi");
  lv_obj_align(lbl_title, LV_ALIGN_CENTER, 0, 0);
  lv_obj_set_style_text_font(lbl_title, &lv_font_montserratMedium_20, 0);
  lv_obj_set_style_text_color(lbl_title, col_ink, 0);

  // WiFi Toggle Switch (Right)
  wifi_sw = lv_switch_create(header);
  lv_obj_set_size(wifi_sw, 50, 26);
  lv_obj_align(wifi_sw, LV_ALIGN_RIGHT_MID, 0, 0);
  lv_obj_clear_state(wifi_sw, LV_STATE_CHECKED); // Default OFF
  lv_obj_set_style_bg_color(wifi_sw, col_btn, 0);
  lv_obj_set_style_bg_color(wifi_sw, col_green,
                            LV_PART_INDICATOR | LV_STATE_CHECKED);
  lv_obj_set_style_bg_color(wifi_sw, lv_color_hex(0x666666), LV_PART_INDICATOR);
  lv_obj_add_event_cb(wifi_sw, wifi_switch_cb, LV_EVENT_VALUE_CHANGED, NULL);

  // --- Connection Status Card ---
  lv_obj_t *status_card = lv_obj_create(cont_wifi);
  lv_obj_set_size(status_card, 360, 85);
  lv_obj_align(status_card, LV_ALIGN_TOP_MID, 0, 95);
  lv_obj_set_style_radius(status_card, 12, 0);
  lv_obj_set_style_bg_color(status_card, col_lcd, 0);
  lv_obj_set_style_border_width(status_card, 3, 0);
  lv_obj_set_style_border_color(status_card, lv_color_hex(0x2F3624), 0);
  lv_obj_set_style_shadow_width(status_card, 0, 0);
  lv_obj_clear_flag(status_card, LV_OBJ_FLAG_SCROLLABLE);

  // WiFi Icon (Left side of card)
  lv_obj_t *wifi_icon = lv_label_create(status_card);
  lv_label_set_text(wifi_icon, LV_SYMBOL_WIFI);
  lv_obj_align(wifi_icon, LV_ALIGN_LEFT_MID, 12, 0);
  lv_obj_set_style_text_font(wifi_icon, &lv_font_montserrat_48, 0);
  lv_obj_set_style_text_color(wifi_icon, col_ink, 0);

  // Status Text Container (Right side)
  lv_obj_t *status_txt = lv_obj_create(status_card);
  lv_obj_set_size(status_txt, 250, 70);
  lv_obj_align(status_txt, LV_ALIGN_RIGHT_MID, -8, 0);
  lv_obj_set_style_bg_opa(status_txt, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(status_txt, 0, 0);
  lv_obj_clear_flag(status_txt, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_flex_flow(status_txt, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(status_txt, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_gap(status_txt, 2, 0);

  // SSID
  wifi_lbl_ssid = lv_label_create(status_txt);
  lv_label_set_text(wifi_lbl_ssid, "Not Connected");
  lv_obj_set_style_text_font(wifi_lbl_ssid, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(wifi_lbl_ssid, col_ink, 0);
  lv_label_set_long_mode(wifi_lbl_ssid, LV_LABEL_LONG_SCROLL_CIRCULAR);
  lv_obj_set_width(wifi_lbl_ssid, 230);

  // Status
  wifi_lbl_status = lv_label_create(status_txt);
  lv_label_set_text(wifi_lbl_status, "Disconnected");
  lv_obj_set_style_text_font(wifi_lbl_status, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(wifi_lbl_status, col_red, 0);

  // IP Address
  wifi_lbl_ip = lv_label_create(status_txt);
  lv_label_set_text(wifi_lbl_ip, "IP: ---");
  lv_obj_set_style_text_font(wifi_lbl_ip, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(wifi_lbl_ip, col_ink, 0);

  // --- Available Networks Section ---
  lv_obj_t *lbl_avail = lv_label_create(cont_wifi);
  lv_label_set_text(lbl_avail, "Available Networks");
  lv_obj_align(lbl_avail, LV_ALIGN_TOP_LEFT, 55, 190);
  lv_obj_set_style_text_font(lbl_avail, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(lbl_avail, col_ink, 0);

  // Scrollable Network List
  wifi_net_list = lv_obj_create(cont_wifi);
  lv_obj_set_size(wifi_net_list, 360, 215);
  lv_obj_align(wifi_net_list, LV_ALIGN_TOP_MID, 0, 215);
  lv_obj_set_style_radius(wifi_net_list, 12, 0);
  lv_obj_set_style_bg_color(wifi_net_list, col_lcd, 0);
  lv_obj_set_style_border_width(wifi_net_list, 3, 0);
  lv_obj_set_style_border_color(wifi_net_list, lv_color_hex(0x2F3624), 0);
  lv_obj_set_style_shadow_width(wifi_net_list, 0, 0);
  lv_obj_set_style_pad_all(wifi_net_list, 10, 0);
  lv_obj_set_flex_flow(wifi_net_list, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_pad_gap(wifi_net_list, 8, 0);

  // Placeholder text
  lv_obj_t *lbl_placeholder = lv_label_create(wifi_net_list);
  lv_label_set_text(lbl_placeholder, "Tap switch to scan...");
  lv_obj_set_style_text_font(lbl_placeholder, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(lbl_placeholder, col_ink, 0);
}

// -----------------------------------------------------------------------------
// Public Functions
// -----------------------------------------------------------------------------

void ui_settings_set_brightness_cb(void (*cb)(uint8_t)) { brightness_cb = cb; }

bool ui_settings_is_visible(void) { return is_visible; }

void ui_settings_init(void) {
  if (scr_settings)
    return;

  // Load saved settings from NVS first
  load_settings_from_nvs();

  // Screen Style (Pure Black for AMOLED Power)
  lv_style_init(&style_scr);
  lv_style_set_bg_color(&style_scr, lv_color_black());
  lv_style_set_bg_opa(&style_scr, LV_OPA_COVER);
  lv_style_set_text_color(&style_scr, lv_color_white());

  // Icon Container Helper Style
  lv_style_init(&style_icon_cont);
  lv_style_set_bg_opa(&style_icon_cont, LV_OPA_TRANSP);
  lv_style_set_border_width(&style_icon_cont, 0);
  // Flex layout properties are applied directly to the object now to avoid
  // style complexity compatibility
  lv_style_set_pad_all(&style_icon_cont, 0);
  lv_style_set_pad_gap(&style_icon_cont, 5);

  // Create Screen
  scr_settings = lv_obj_create(NULL);
  lv_obj_add_style(scr_settings, &style_scr, 0);
  lv_obj_clear_flag(scr_settings, LV_OBJ_FLAG_SCROLLABLE);

  // Initialize Views
  create_launcher_view();
  create_settings_menu();
  create_brightness_view();
  create_wifi_view();
  create_pomodoro_view();

  // Create global WiFi indicator (always on top)
  create_global_wifi_indicator();

  // Apply loaded settings to UI controls
  apply_loaded_settings_to_ui();

  // Default state: Launcher visible
  show_launcher_panel();
}

void ui_settings_show(void) {
  if (!scr_settings)
    ui_settings_init();

  show_launcher_panel();
  // Smooth Entry from Bottom
  lv_scr_load_anim(scr_settings, LV_SCR_LOAD_ANIM_MOVE_TOP, 300, 0, false);
  is_visible = true;
}

void ui_settings_hide(void) {
  lv_obj_t *eyes_scr = ui_robo_eyes_get_scr();
  if (eyes_scr) {
    lv_scr_load_anim(eyes_scr, LV_SCR_LOAD_ANIM_MOVE_BOTTOM, 300, 0, false);
  }
  is_visible = false;
}

// -----------------------------------------------------------------------------
// WiFi Public API Functions
// -----------------------------------------------------------------------------

void ui_settings_wifi_update_status(bool connected, const char *ssid,
                                    const char *ip) {
  if (wifi_lbl_ssid) {
    lv_label_set_text(wifi_lbl_ssid, ssid ? ssid : "Not Connected");
  }
  if (wifi_lbl_status) {
    if (connected) {
      lv_label_set_text(wifi_lbl_status, "Connected");
      lv_obj_set_style_text_color(wifi_lbl_status, lv_color_hex(0x4CAF50), 0);
    } else {
      lv_label_set_text(wifi_lbl_status, "Disconnected");
      lv_obj_set_style_text_color(wifi_lbl_status, lv_color_hex(0xD95E52), 0);
    }
  }
  if (wifi_lbl_ip) {
    if (ip) {
      char ip_str[32];
      snprintf(ip_str, sizeof(ip_str), "IP: %s", ip);
      lv_label_set_text(wifi_lbl_ip, ip_str);
    } else {
      lv_label_set_text(wifi_lbl_ip, "IP: ---");
    }
  }
  if (wifi_sw) {
    if (connected) {
      lv_obj_add_state(wifi_sw, LV_STATE_CHECKED);
    } else {
      lv_obj_clear_state(wifi_sw, LV_STATE_CHECKED);
    }
  }

  // Update global WiFi indicator
  update_global_wifi_indicator(connected);
}

void ui_settings_wifi_add_network(const char *ssid, int rssi) {
  if (!wifi_net_list || !ssid)
    return;

  lv_color_t col_ink = lv_color_hex(0x1F2418);
  lv_color_t col_rssi = get_rssi_color(rssi);

  lv_obj_t *net_item = lv_obj_create(wifi_net_list);
  lv_obj_set_size(net_item, 320, 40);
  lv_obj_set_style_radius(net_item, 10, 0);
  lv_obj_set_style_bg_color(net_item, lv_color_hex(0xA8B896), 0);
  lv_obj_set_style_border_width(net_item, 0, 0);
  lv_obj_set_style_shadow_width(net_item, 0, 0);
  lv_obj_clear_flag(net_item, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(net_item, LV_OBJ_FLAG_CLICKABLE);

  // Pressed style for better UX
  lv_obj_set_style_bg_color(net_item, lv_color_hex(0x8A9A7A), LV_STATE_PRESSED);

  // Store SSID for callback (allocate copy since ssid may be temporary)
  char *ssid_copy = lv_mem_alloc(strlen(ssid) + 1);
  if (ssid_copy) {
    strcpy(ssid_copy, ssid);
    lv_obj_add_event_cb(net_item, wifi_network_item_cb, LV_EVENT_CLICKED,
                        ssid_copy);
  }

  // Network Name
  lv_obj_t *lbl_net = lv_label_create(net_item);
  lv_label_set_text(lbl_net, ssid);
  lv_obj_align(lbl_net, LV_ALIGN_LEFT_MID, 10, 0);
  lv_obj_set_style_text_font(lbl_net, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(lbl_net, col_ink, 0);

  // Signal Strength Bar (Visual RSSI indicator)
  lv_obj_t *rssi_cont = lv_obj_create(net_item);
  lv_obj_set_size(rssi_cont, 30, 20);
  lv_obj_align(rssi_cont, LV_ALIGN_RIGHT_MID, -10, 0);
  lv_obj_set_style_bg_opa(rssi_cont, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(rssi_cont, 0, 0);
  lv_obj_set_style_pad_all(rssi_cont, 0, 0);
  lv_obj_clear_flag(rssi_cont, LV_OBJ_FLAG_SCROLLABLE);

  // Create signal bars (4 bars)
  int num_bars = 4;
  int bar_width = 5;
  int bar_gap = 2;
  int active_bars;

  // Determine active bars based on RSSI
  if (rssi >= -50)
    active_bars = 4; // Excellent
  else if (rssi >= -60)
    active_bars = 3; // Good
  else if (rssi >= -70)
    active_bars = 2; // Fair
  else if (rssi >= -80)
    active_bars = 1; // Weak
  else
    active_bars = 0; // Very weak

  for (int i = 0; i < num_bars; i++) {
    int bar_height = 4 + (i * 4); // Heights: 4, 8, 12, 16
    lv_obj_t *bar = lv_obj_create(rssi_cont);
    lv_obj_set_size(bar, bar_width, bar_height);
    lv_obj_align(bar, LV_ALIGN_BOTTOM_LEFT, i * (bar_width + bar_gap), 0);
    lv_obj_set_style_radius(bar, 1, 0);
    lv_obj_set_style_border_width(bar, 0, 0);
    lv_obj_clear_flag(bar, LV_OBJ_FLAG_SCROLLABLE);

    if (i < active_bars) {
      lv_obj_set_style_bg_color(bar, col_rssi, 0);
    } else {
      lv_obj_set_style_bg_color(bar, lv_color_hex(0x666666),
                                0); // Gray inactive
    }
  }
}

void ui_settings_wifi_clear_networks(void) {
  if (!wifi_net_list)
    return;
  lv_obj_clean(wifi_net_list);
}

void ui_settings_wifi_set_scan_cb(void (*cb)(void)) { wifi_scan_cb = cb; }

void ui_settings_wifi_set_connect_cb(void (*cb)(const char *ssid,
                                                const char *password)) {
  wifi_connect_cb = cb;
}

// Create WiFi indicator on external screen (e.g., robot eyes)
void ui_settings_create_wifi_indicator_on(lv_obj_t *parent) {
  if (!parent)
    return;

  static lv_obj_t *external_wifi_indicator = NULL;
  if (external_wifi_indicator != NULL) {
    return; // Already created
  }

  extern lv_font_t lv_font_montserratMedium_20;
  external_wifi_indicator = lv_label_create(parent);
  lv_label_set_text(external_wifi_indicator, LV_SYMBOL_WIFI);

  // Position for round display - Safe Zone (TOP_MID + offset)
  lv_obj_align(external_wifi_indicator, LV_ALIGN_TOP_MID, 65, 25);

  lv_obj_set_style_text_font(external_wifi_indicator,
                             &lv_font_montserratMedium_20, 0);
  lv_obj_set_style_text_color(external_wifi_indicator,
                              global_wifi_connected ? lv_color_hex(0x4CAF50)
                                                    : lv_color_hex(0x888888),
                              0);

  // Add subtle background for visibility
  lv_obj_set_style_bg_opa(external_wifi_indicator, LV_OPA_30, 0);
  lv_obj_set_style_bg_color(external_wifi_indicator, lv_color_black(), 0);
  lv_obj_set_style_radius(external_wifi_indicator, 4, 0);
  lv_obj_set_style_pad_all(external_wifi_indicator, 3, 0);

  // Keep on top
  lv_obj_move_foreground(external_wifi_indicator);

  printf("[WIFI_INDICATOR] Created external WiFi indicator on screen\n");
}
