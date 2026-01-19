# Animation Manager - Quick Start Guide

## 🎯 ภาพรวม

Animation Manager ช่วยให้คุณเพิ่ม animation ใหม่ได้โดย**ไม่ต้องแก้ไขโค้ดเดิม**!

---

## 📁 โครงสร้างไฟล์

```
components/user_app/
├── anim_manager.c          ← ✅ ระบบจัดการ animation
├── anim_manager.h          ← ✅ Header
├── anim_registry.c         ← ✅ ลงทะเบียน animation (แก้ไขแค่ไฟล์นี้!)
├── ui_robo_eyes.c          ← ไม่ต้องแก้!
└── animations/             ← ✅ เก็บไฟล์ animation (สร้างโฟลเดอร์นี้)
    ├── blink_anim.c
    ├── blink_anim.h
    ├── happy_anim.c
    └── happy_anim.h
```

---

## 🚀 วิธีใช้งาน (3 ขั้นตอน)

### **ขั้นตอนที่ 1: Export Animation**

1. เปิด `web_robot_face/index.html`
2. วาด animation
3. ตั้งชื่อ: `blink_anim`
4. คลิก "⬇️ Export to C"
5. ได้ไฟล์: `blink_anim.c` และ `blink_anim.h`

---

### **ขั้นตอนที่ 2: เพิ่มไฟล์**

1. **สร้างโฟลเดอร์** (ถ้ายังไม่มี):
   ```
   components/user_app/animations/
   ```

2. **Copy ไฟล์**:
   ```
   Copy: blink_anim.c, blink_anim.h
   ไปที่: components/user_app/animations/
   ```

3. **แก้ไข CMakeLists.txt**:
   ```cmake
   # เพิ่มใน SRCS
   SRCS "anim_manager.c" "anim_registry.c" 
        "animations/blink_anim.c"
   ```

---

### **ขั้นตอนที่ 3: ลงทะเบียน Animation**

แก้ไข **`anim_registry.c`** (แค่ไฟล์เดียว!):

```c
// 1. เพิ่ม include
#include "animations/blink_anim.h"

// 2. ลงทะเบียนใน register_all_animations()
void register_all_animations(void) {
    anim_manager_register("blink", 
                          blink_anim_frames, 
                          blink_anim_frame_count, 
                          300);  // 300ms
}
```

**เสร็จแล้ว!** ✅

---

## 💻 วิธีใช้งานใน Code

### **1. เริ่มต้น (ครั้งเดียว)**

ใน `ui_robo_eyes.c` หรือ `user_app.c`:

```c
#include "anim_manager.h"

// ประกาศฟังก์ชันจาก anim_registry.c
extern void register_all_animations(void);

void init_robot_face(void) {
    // ... โค้ดเดิม ...
    
    // เริ่มต้น Animation Manager
    anim_manager_init(scr_eyes);  // scr_eyes = parent object
    
    // ลงทะเบียน animations ทั้งหมด
    register_all_animations();
}
```

---

### **2. เล่น Animation**

```c
// เล่นครั้งเดียว
anim_manager_play("blink", 1);

// เล่น 3 ครั้ง
anim_manager_play("happy", 3);

// เล่นแบบ loop ไม่รู้จบ
anim_manager_play("idle", 0);
```

---

### **3. ตัวอย่างการใช้งานจริง**

```c
// กระพริบอัตโนมัติทุก 3 วินาที
static void update_positions(void) {
    // ... โค้ดเดิม ...
    
    static uint32_t last_blink = 0;
    uint32_t now = lv_tick_get();
    
    if (now - last_blink > 3000) {
        anim_manager_play("blink", 1);
        last_blink = now;
    }
    
    // อัปเดต Animation Manager
    anim_manager_update();
}
```

---

### **4. ตัวอย่างกับ Emotion System**

```c
void set_robot_emotion(emotion_t emotion) {
    switch (emotion) {
        case EMO_HAPPY:
            anim_manager_play("happy", 0);  // Loop
            break;
            
        case EMO_SAD:
            anim_manager_play("sad", 0);
            break;
            
        case EMO_BLINK:
            anim_manager_play("blink", 1);  // Once
            break;
    }
}
```

---

### **5. ใช้ Callback**

```c
void on_animation_finished(const char* anim_name) {
    printf("Animation '%s' finished!\n", anim_name);
    
    // ทำอะไรต่อหลังจบ animation
    if (strcmp(anim_name, "blink") == 0) {
        // กลับไปแสดงตาปกติ
    }
}

void init_robot_face(void) {
    anim_manager_init(scr_eyes);
    register_all_animations();
    
    // ตั้ง callback
    anim_manager_set_finish_callback(on_animation_finished);
}
```

---

## 🎨 เพิ่ม Animation ใหม่

### **ตัวอย่าง: เพิ่ม "happy_anim"**

1. **Export จาก Web Studio**
   - ได้: `happy_anim.c`, `happy_anim.h`

2. **Copy ไฟล์**
   ```
   → components/user_app/animations/
   ```

3. **แก้ CMakeLists.txt**
   ```cmake
   SRCS "anim_manager.c" "anim_registry.c"
        "animations/blink_anim.c"
        "animations/happy_anim.c"  ← เพิ่ม
   ```

4. **แก้ anim_registry.c**
   ```c
   #include "animations/blink_anim.h"
   #include "animations/happy_anim.h"  // ← เพิ่ม
   
   void register_all_animations(void) {
       anim_manager_register("blink", blink_anim_frames, blink_anim_frame_count, 300);
       anim_manager_register("happy", happy_anim_frames, happy_anim_frame_count, 500);  // ← เพิ่ม
   }
   ```

5. **ใช้งาน**
   ```c
   anim_manager_play("happy", 0);  // ✅ เล่นได้เลย!
   ```

---

## 📋 API Reference

### **anim_manager_init(parent)**
เริ่มต้นระบบ
```c
anim_manager_init(scr_eyes);
```

### **anim_manager_register(name, frames, count, duration)**
ลงทะเบียน animation
```c
anim_manager_register("blink", blink_anim_frames, blink_anim_frame_count, 300);
```

### **anim_manager_play(name, loop)**
เล่น animation
```c
anim_manager_play("blink", 1);  // เล่น 1 ครั้ง
anim_manager_play("idle", 0);   // Loop ไม่รู้จบ
```

### **anim_manager_stop()**
หยุด animation ปัจจุบัน
```c
anim_manager_stop();
```

### **anim_manager_is_playing()**
เช็คว่ากำลังเล่นอยู่หรือไม่
```c
if (anim_manager_is_playing()) {
    // กำลังเล่นอยู่
}
```

### **anim_manager_get_current()**
ดูชื่อ animation ที่กำลังเล่น
```c
const char* current = anim_manager_get_current();
printf("Playing: %s\n", current);
```

### **anim_manager_update()**
อัปเดตระบบ (เรียกทุก frame)
```c
void update_positions(void) {
    anim_manager_update();
}
```

---

## ✅ ข้อดี

| ก่อน | หลัง |
|------|------|
| ❌ แก้ `ui_robo_eyes.c` ทุกครั้ง | ✅ แก้แค่ `anim_registry.c` |
| ❌ เพิ่ม include ทุกครั้ง | ✅ เพิ่มแค่ 1 บรรทัด |
| ❌ สร้างฟังก์ชันใหม่ทุกครั้ง | ✅ เรียก `anim_manager_play()` |
| ❌ โค้ดยุ่งเหยิง | ✅ โค้ดเป็นระเบียบ |

---

## 🎯 สรุป

**เพิ่ม Animation ใหม่ = แก้ไขแค่ 2 ไฟล์:**
1. `CMakeLists.txt` - เพิ่มชื่อไฟล์
2. `anim_registry.c` - ลงทะเบียน animation

**ไม่ต้องแก้:**
- ❌ `ui_robo_eyes.c`
- ❌ `ui_robo_eyes.h`
- ❌ โค้ดเดิมอื่นๆ

**ง่าย สะดวก ไม่ยุ่งยาก!** 🎉
