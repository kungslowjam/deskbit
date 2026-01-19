# Robot Face Studio - Integration Guide

## วิธีใช้อนิเมชั่นจาก Robot Face Studio กับหุ่นยนต์

### ขั้นตอนที่ 1: สร้างอนิเมชั่น
1. เปิด `web_robot_face/index.html` ในเบราว์เซอร์
2. วาดหน้าหุ่นยนต์ที่ต้องการ
3. กด **Add Frame** เพื่อเพิ่มเฟรมใหม่
4. ทำซ้ำจนครบทุกเฟรมที่ต้องการ

### ขั้นตอนที่ 2: Export
1. กดปุ่ม **"Export to C"** มุมขวาบน
2. บันทึกไฟล์ `custom_anim.c`

### ขั้นตอนที่ 3: ใส่ไฟล์ลงโปรเจค
```
components/user_app/
├── custom_anim.c          <-- ใส่ไฟล์ที่นี่
├── ui_custom_anim.c       <-- (มีแล้ว)
├── ui_custom_anim.h       <-- (มีแล้ว)
└── CMakeLists.txt         <-- เพิ่ม "custom_anim.c"
```

แก้ไข `CMakeLists.txt`:
```cmake
SRCS "..." "ui_custom_anim.c" "custom_anim.c"
```

### ขั้นตอนที่ 4: เรียกใช้ในโค้ด

```c
#include "ui_custom_anim.h"

// ประกาศ extern
extern const lv_img_dsc_t* my_anim_imgs[];
extern const uint16_t my_anim_count;

void show_my_face(void) {
    // สร้าง player
    custom_anim_t *face = ui_custom_anim_create(lv_scr_act());
    
    // โหลดเฟรม (83ms = 12 FPS)
    ui_custom_anim_set_src(face, my_anim_imgs, my_anim_count, 83);
    
    // จัดตำแหน่งกลางจอ
    ui_custom_anim_align(face, LV_ALIGN_CENTER, 0, 0);
    
    // เล่นวนซ้ำ
    ui_custom_anim_start(face, LV_ANIM_REPEAT_INFINITE);
}
```

### API Reference

| Function | Description |
|----------|-------------|
| `ui_custom_anim_create(parent)` | สร้าง animation player ใหม่ |
| `ui_custom_anim_set_src(anim, imgs, count, ms)` | โหลดภาพจาก export |
| `ui_custom_anim_start(anim, repeat)` | เริ่มเล่น |
| `ui_custom_anim_stop(anim)` | หยุดเล่น |
| `ui_custom_anim_align(anim, align, x, y)` | จัดตำแหน่ง |
| `ui_custom_anim_set_size(anim, w, h)` | ปรับขนาด |
| `ui_custom_anim_delete(anim)` | ลบและคืน memory |
| `ui_custom_anim_is_playing(anim)` | เช็คว่ากำลังเล่นอยู่ไหม |

### Tips
- FPS 12 ใช้ `frame_duration_ms = 83`
- FPS 24 ใช้ `frame_duration_ms = 42`
- ใช้ `LV_ANIM_REPEAT_INFINITE` เพื่อเล่นวนซ้ำตลอด
- ใช้ repeat count = 1 เพื่อเล่นครั้งเดียว
