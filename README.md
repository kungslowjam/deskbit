# 🤖 DeskBit - Desktop Companion Robot

<div align="center">

![DeskBit Logo](readme/620191051_25770710362555522_3038655505199516088_n.jpg)

**DeskBit** is a cute desktop companion device with a 1.43" round AMOLED display.  
Designed to be your desk buddy with adorable and useful features!

**DeskBit** คืออุปกรณ์ Desktop Companion ขนาดเล็กที่มาพร้อมหน้าจอ AMOLED กลม 1.43 นิ้ว  
ออกแบบมาเพื่อเป็นเพื่อนคู่ใจบนโต๊ะทำงานของคุณ พร้อมฟีเจอร์สุดน่ารักและมีประโยชน์!

[![ESP32-S3](https://img.shields.io/badge/MCU-ESP32--S3-blue?style=for-the-badge&logo=espressif)](https://www.espressif.com/)
[![LVGL](https://img.shields.io/badge/GUI-LVGL-brightgreen?style=for-the-badge)](https://lvgl.io/)
[![Made with Love](https://img.shields.io/badge/Made%20with-❤️-red?style=for-the-badge)]()

### 🎬 Demo Video

[![DeskBit Demo](https://img.youtube.com/vi/HIJs3vWXdCQ/0.jpg)](https://www.youtube.com/watch?v=HIJs3vWXdCQ)

▶️ **[Watch Demo on YouTube | ดูวิดีโอ Demo บน YouTube](https://www.youtube.com/watch?v=HIJs3vWXdCQ)**

</div>

---

## ✨ Main Features | คุณสมบัติหลัก

### 🎭 Robot Eyes | ดวงตาหุ่นยนต์
<div align="center">

![Robot Eyes](readme/622279751_1862890687678989_1698581453543050712_n.jpg)

</div>

| English | ไทย |
|---------|-----|
| Multiple expressions: Happy, Love, Angry, Sleep, Laugh and more | อารมณ์หลากหลาย: Happy, Love, Angry, Sleep, Laugh และอื่นๆ |
| Smooth physics-based animations | Animation ลื่นไหลใช้ระบบ Physics-based |
| Natural gaze system - eyes look around naturally | ระบบ Gaze - ดวงตามองไปรอบๆ อย่างเป็นธรรมชาติ |
| Random automatic blinking | กระพริบตาอัตโนมัติแบบ Random |
| Love mode with pink heart eyes 💕 | Love Mode - ดวงตาเป็นรูปหัวใจสีชมพู 💕 |

---

### 🍅 Pomodoro Timer | ตัวจับเวลา Pomodoro

| English | ไทย |
|---------|-----|
| Adjustable work sessions (1-60 min) | ตั้งเวลาทำงานได้ 1-60 นาที |
| Adjustable break time (1-30 min) | ตั้งเวลาพักได้ 1-30 นาที |
| Beautiful UI with progress arc animation | UI สวยงามพร้อม Progress Arc Animation |
| Settings saved to flash (NVS) | บันทึกการตั้งค่าลง Flash Memory (NVS) |

---

### ⚙️ Settings | หน้าจอตั้งค่า
<div align="center">

![Settings](readme/622422010_699211289822924_7182381935930133792_n.jpg)

</div>

| Feature | English | ไทย |
|---------|---------|-----|
| 🔆 **Brightness** | Adjust screen brightness 0-100% | ปรับความสว่างหน้าจอ 0-100% |
| 📶 **WiFi Manager** | Scan, connect with password dialog | สแกน, เชื่อมต่อพร้อม Password Dialog |
| 📍 **Status Display** | Show connection status & IP address | แสดงสถานะการเชื่อมต่อและ IP Address |
| 💾 **NVS Storage** | All settings saved to flash memory | บันทึกการตั้งค่าทั้งหมดลง Flash Memory |

---

## 🛠️ Hardware Specification | ข้อมูลจำเพาะฮาร์ดแวร์

| Component | Specification |
|-----------|---------------|
| **MCU** | ESP32-S3R8 (Dual-core LX7 @ 240MHz) |
| **Display** | 1.43" AMOLED, 466×466, 16.7M colors |
| **Touch** | Capacitive Touch (FT3168) |
| **RAM** | 512KB SRAM + 8MB PSRAM |
| **Flash** | 16MB |
| **IMU** | QMI8658 (6-axis Accelerometer/Gyroscope) |
| **RTC** | PCF85063 |
| **Connectivity** | WiFi 802.11 b/g/n + Bluetooth 5.0 |
| **Storage** | TF Card Slot |
| **Battery** | 3.7V Li-ion (MX1.25 connector) |
| **USB** | Type-C |

---

## 📁 Project Structure | โครงสร้างโปรเจกต์

```
deskbit/
├── main/                      # Main application entry
│   └── app_main.c            # Application main
├── components/               
│   ├── user_app/              # Main UI components
│   │   ├── ui_robo_eyes.c     # Robot eyes animation engine
│   │   ├── ui_settings.c      # Settings & Pomodoro UI
│   │   ├── ui_custom_anim.c   # Custom animation player
│   │   └── anim_manager.c     # Animation management system
│   ├── esp_wifi_bsp/          # WiFi driver
│   ├── touch_bsp/             # Touch driver
│   ├── pcf85063/              # RTC driver
│   ├── qmi8658c/              # IMU driver
│   └── lvgl/                  # LVGL graphics library
├── web_robot_face/            # Web-based Robot Face Studio
│   ├── index.html             # Web animation editor
│   ├── bridge_server.py       # ESP32 communication bridge
│   └── export_to_project.py   # Export animations to C code
└── readme/                    # Documentation images
```

---

## 🎨 Robot Eyes Expressions | อารมณ์ดวงตา

| Expression | English | ไทย |
|------------|---------|-----|
| 😊 **Happy** | Curved smiling eyes + rosy cheeks + smile | ตาโค้งยิ้ม + แก้มแดง + ปากยิ้ม |
| 😍 **Love** | Pink heart-shaped eyes | ดวงตาเป็นรูปหัวใจสีชมพู |
| 😠 **Angry** | Angled eyebrows + slight shake | คิ้วเฉียง + สั่นเล็กน้อย |
| 😴 **Sleep** | Closed eyes + floating Z's | ตาปิด + ตัว Z ลอยขึ้น |
| 😂 **Laugh** | Squinted eyes + open mouth + body shake | ตาหยี + ปากเปิดกว้าง + สั่นตัว |
| 😐 **Idle** | Normal eyes + auto blink | ดวงตาปกติ + กระพริบตาอัตโนมัติ |

---

## 🚀 Quick Start | เริ่มต้นใช้งาน

### 1. Build & Flash

```bash
# Set up ESP-IDF environment
. $HOME/esp/esp-idf/export.sh

# Build the project
idf.py build

# Flash to device
idf.py -p COM3 flash monitor
```

### 2. Web Robot Face Studio

```bash
cd web_robot_face

# Start the editor | เปิด Editor
start index.html

# OR start with bridge server (for live preview on device)
# หรือเปิดพร้อม Bridge Server สำหรับ Preview บนอุปกรณ์
python bridge_server.py
```

---

## 📱 Coming Soon | ฟีเจอร์ในอนาคต

| Feature | English | ไทย |
|---------|---------|-----|
| 🔔 **Notifications** | PC/Mobile notification support | รองรับการแจ้งเตือนจาก PC/Mobile |
| 🗣️ **Voice Commands** | Control with voice | สั่งงานด้วยเสียง |
| 📊 **Desktop Stats** | Display CPU/RAM stats | แสดงสถิติ CPU/RAM |
| 🎮 **Mini Games** | Fun mini games to relax | เกมเล็กๆ เล่นผ่อนคลาย |

---

## 📜 License

This project is for personal/educational use.  
โปรเจกต์นี้สำหรับใช้งานส่วนตัว/การศึกษา

---

<div align="center">

### Made with ❤️ for Desktop Companions
### สร้างด้วย ❤️ สำหรับเพื่อนบนโต๊ะทำงาน

**DeskBit** - Your Cute Desk Buddy! 🤖  
**DeskBit** - เพื่อนคู่ใจบนโต๊ะทำงานของคุณ! 🤖

</div>
