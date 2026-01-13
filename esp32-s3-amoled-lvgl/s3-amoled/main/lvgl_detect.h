#pragma once

#if defined(__has_include)
#if __has_include("lvgl.h")
#include "lvgl.h"
#define LVGL_AVAILABLE 1
#else
#define LVGL_AVAILABLE 0
#endif
#else
#include "lvgl.h"
#define LVGL_AVAILABLE 1
#endif
