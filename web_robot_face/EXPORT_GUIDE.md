# Robot Face Studio - Quick Export Guide

## 🚀 วิธีใช้งาน (ไม่ต้อง Hardcode!)

### **วิธีที่ 1: Drag & Drop (ง่ายที่สุด!)**

1. **สร้าง Animation** ใน `index.html`
2. **Export JSON** (คลิกปุ่ม "💾 Export JSON")
3. **Drag & Drop** ไฟล์ `.json` ลงบน `quick_export.bat`
4. **ตั้งชื่อ** animation (เช่น `blink_anim`)
5. **เสร็จ!** ไฟล์จะถูกสร้างอัตโนมัติที่ `../components/user_app/`

### **วิธีที่ 2: Command Line**

```bash
# Export JSON จาก Web Studio ก่อน
python export_to_project.py my_animation.json
```

---

## 📁 ไฟล์ที่ถูกสร้างอัตโนมัติ

เมื่อคุณ export `blink_anim.json`:

```
components/user_app/
├── blink_anim.c        ← ✅ สร้างอัตโนมัติ
├── blink_anim.h        ← ✅ สร้างอัตโนมัติ
└── CMakeLists.txt      ← ✅ อัปเดตอัตโนมัติ
```

---

## 🎯 ขั้นตอนทั้งหมด

### **1. สร้าง Animation**
- เปิด `web_robot_face/index.html`
- วาดหน้าหุ่นยนต์
- สร้างหลายเฟรม
- ตั้งชื่อ animation

### **2. Export JSON**
- คลิก "💾 Export JSON"
- บันทึกไฟล์ (เช่น `blink.json`)

### **3. Auto-Generate C Files**
- **วิธี A**: Drag `blink.json` ลงบน `quick_export.bat`
- **วิธี B**: รัน `python export_to_project.py blink.json`

### **4. ใช้งานใน Code**

เปิด `components/user_app/ui_robo_eyes.c`:

```c
#include "blink_anim.h"  // ✅ เพิ่มบรรทัดนี้

void play_blink(void) {
    lv_obj_t* anim = lv_animimg_create(scr_eyes);
    lv_animimg_set_src(anim, 
                       (const void**)blink_anim_frames,
                       blink_anim_frame_count);
    lv_animimg_set_duration(anim, 300);
    lv_animimg_start(anim);
}
```

### **5. Build**
```bash
idf.py build
idf.py flash
```

---

## 💡 ข้อดี

| ก่อน (Hardcode) | ตอนนี้ (Auto) |
|----------------|---------------|
| ❌ ต้องสร้าง .h เอง | ✅ สร้างอัตโนมัติ |
| ❌ ต้องแก้ CMakeLists.txt เอง | ✅ อัปเดตอัตโนมัติ |
| ❌ ต้องจำชื่อตัวแปร | ✅ ตั้งชื่อเองได้ |
| ❌ ใช้เวลานาน | ✅ 5 วินาทีเสร็จ |

---

## 🔧 Troubleshooting

### **Script ไม่ทำงาน?**
```bash
# ตรวจสอบ Python
python --version

# ถ้าไม่มี Python ให้ติดตั้งก่อน
# Download: https://www.python.org/downloads/
```

### **ไฟล์ไม่ถูกสร้าง?**
- ตรวจสอบว่า JSON file ถูกต้อง
- ตรวจสอบว่าอยู่ในโฟลเดอร์ `web_robot_face`
- ตรวจสอบว่า `components/user_app` มีอยู่

### **CMakeLists.txt ไม่อัปเดต?**
- ดูไฟล์ backup: `CMakeLists.txt.backup`
- เพิ่มชื่อไฟล์เองใน SRCS

---

## 📝 ตัวอย่างการใช้งาน

```bash
# 1. Export blink animation
# (ใน Web Studio: Export JSON → blink.json)

# 2. Auto-generate
python export_to_project.py blink.json
# > Animation name: blink_anim
# > ✅ Generated: blink_anim.c
# > ✅ Generated: blink_anim.h
# > ✅ Updated CMakeLists.txt

# 3. Use in code
# #include "blink_anim.h"

# 4. Build
idf.py build
```

---

## 🎨 สร้าง Animation หลายๆ อัน

```bash
# Happy face
python export_to_project.py happy.json

# Sad face
python export_to_project.py sad.json

# Angry face
python export_to_project.py angry.json

# ทุกไฟล์จะถูกเพิ่มใน CMakeLists.txt อัตโนมัติ!
```

---

**ไม่ต้อง Hardcode อีกต่อไป! 🎉**
