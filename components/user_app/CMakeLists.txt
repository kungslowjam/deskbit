idf_component_register(
    SRCS "anim_manager_example.c" "anim_registry.c" "anim_manager.c" "blink_anim.c" "custom_anim.c" "user_app.c" "ui_robo_eyes.c" "ui_settings.c" "settings_icon.c" "deskbot_icon.c" "pomodoro_icon.c" "magic_book.c" "book_anim.c" "pomo_gif.c" "ui_custom_anim.c" "animations/blink_anim.c"
  PRIV_REQUIRES touch_bsp pcf85063 qmi8658c adc_bsp sd_card_bsp ble_scan_bsp esp_wifi_bsp gpio_bsp button_bsp nvs_flash
  REQUIRES ui_bsp
  INCLUDE_DIRS "./")
