| Supported Targets | ESP32 | ESP32-C2 | ESP32-C3 | ESP32-C6 | ESP32-H2 | ESP32-S2 | ESP32-S3 |
| ----------------- | ----- | -------- | -------- | -------- | -------- | -------- | -------- |

| Supported LCD Controllers | SPD2010 | GC9B71 | SH8601 |
| ------------------------- | ------- | ------ | ------ |

# Waveshare ESP32-S3 Touch AMOLED 1.43" - Robot Eyes Project

## 📋 Product Features

This product adopts **ESP32-S3R8** chip with 2.4GHz WiFi and Bluetooth BLE 5 support, integrates high-capacity Flash and PSRAM. Onboard **1.43inch AMOLED display** which can smoothly run GUI programs such as LVGL. Onboard rich peripheral resources such as RTC chip, QMI8658 6-axis IMU, TF card slot, Lithium battery header, etc., and reserved USB, UART, I2C and GPIO pin headers, offering strong compatibility and expandability. It is suitable for the quick development of HMI and other ESP32-S3 applications.

### 🚀 Key Features

- ⚡ **High-performance Xtensa 32-bit LX7 dual-core processor**, up to **240MHz** main frequency
- 📶 Supports **2.4GHz Wi-Fi (802.11 b/g/n)** and **Bluetooth 5 (LE)**, with onboard antenna. Supports switching to use external antenna via resoldering an onboard resistor
- 💾 Built-in **512KB SRAM** and **384KB ROM**, with onboard **16MB Flash** and **8MB PSRAM**
- 🖥️ Onboard **1.43inch AMOLED display**, QSPI interface, **466 × 466 resolution**, **16.7M colors**
- 👆 Supports **capacitive touch** function controlled via I2C interface
- 🎯 Onboard **QMI8658 6-axis IMU** (3-axis accelerometer and 3-axis gyroscope) for detecting motion gestures, counting steps, etc.
- 🕐 Onboard **PCF85063 RTC chip** with reserved SH1.0 RTC battery header (supports charging) for RTC function requirement
- 🔘 Onboard RST button and programmable BOOT button for easy custom function development
- 🔋 Onboard **3.7V MX1.25 Lithium battery** recharge/discharge header
- 💿 Onboard **TF card slot** for external TF card storage of pictures or files
- 🔌 Onboard **USB Type-C** port for power supply, program downloading, and debugging
- 📌 Onboard UART and I2C SH1.0 4PIN connector, reserved **2 × 14PIN 1.27mm pitch headers**, adapting multiple GPIO interfaces
- 🎨 Optional CNC metal case, with clear and colorful label on the back

---

## 📊 AMOLED Display Specifications

| Parameter | Specification |
|-----------|---------------|
| **Display Panel** | AMOLED |
| **Display Size** | 1.43 inch |
| **Resolution** | 466 × 466 |
| **Display Colors** | 16.7M |
| **Brightness** | 350 cd/㎡ |
| **Contrast Ratio** | 60000:1 |
| **Communication Interface** | QSPI |
| **Driver IC** | **SH8601** |
| **Touch** | Supported |
| **Touch IC** | **FT3168** |

---

## 🔧 Technical Specifications Summary

| Component | Specification |
|-----------|---------------|
| **MCU** | ESP32-S3R8 (Dual-core LX7 @ 240MHz) |
| **Wi-Fi** | 802.11 b/g/n (2.4GHz) |
| **Bluetooth** | BLE 5.0 |
| **RAM** | 512KB SRAM + 8MB PSRAM |
| **Flash** | 16MB |
| **Display** | 1.43" AMOLED, 466×466, 16.7M colors |
| **Touch** | Capacitive (FT3168) |
| **IMU** | QMI8658 (6-axis) |
| **RTC** | PCF85063 |
| **Storage** | TF Card Slot |
| **Battery** | 3.7V Li-ion (MX1.25 connector) |
| **USB** | Type-C |

---

# QSPI LCD (with RAM) and Touch Panel Example

[esp_lcd](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/lcd.html) provides several panel drivers out-of box, e.g. ST7789, SSD1306, NT35510. However, there're a lot of other panels on the market, it's beyond `esp_lcd` component's responsibility to include them all.

`esp_lcd` allows user to add their own panel drivers in the project scope (i.e. panel driver can live outside of esp-idf), so that the upper layer code like LVGL porting code can be reused without any modifications, as long as user-implemented panel driver follows the interface defined in the `esp_lcd` component.

This example shows how to use SPD1020, GC9B71 or SH8601 display driver from Component manager in esp-idf project. These components are using API provided by `esp_lcd` component. This example will draw a fancy dash board with the LVGL library.

This example uses the [esp_timer](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/esp_timer.html) to generate the ticks needed by LVGL and uses a dedicated task to run the `lv_timer_handler()`. Since the LVGL APIs are not thread-safe, this example uses a mutex which be invoked before the call of `lv_timer_handler()` and released after it. The same mutex needs to be used in other tasks and threads around every LVGL (lv_...) related function call and code. For more porting guides, please refer to [LVGL porting doc](https://docs.lvgl.io/master/porting/index.html).

## Touch controller

In this example you can enable touch controller SPD2010 or CST816 connected via I2C.

## How to use the example

### Hardware Required

* An ESP development board
* A SPD1020, GC9B71 or SH8601 LCD panel, with QSPI interface (with/without touch)
* An USB cable for power supply and programming

### Hardware Connection

The connection between ESP Board and the LCD is as follows:

```
       ESP Board                SPD1020, GC9B71 or SH8601 Panel (QSPI)
┌──────────────────────┐              ┌────────────────────┐
│             GND      ├─────────────►│ GND                │
│                      │              │                    │
│             3V3      ├─────────────►│ VCC                │
│                      │              │                    │
│             CS       ├─────────────►│ CS                 │
│                      │              │                    │
│             SCK      ├─────────────►│ CLK                │
│                      │              │                    │
│             D3       ├─────────────►│ IO3                │
│                      │              │                    │
│             D2       ├─────────────►│ IO2                │
│                      │              │                    │
│             D1       ├─────────────►│ IO1                │
│                      │              │                    │
│             D0       ├─────────────►│ IO0                │
│                      │              │                    │
│             RST      ├─────────────►│ RSTN               │
│                      │              │                    │
│             (SCL)    ├─────────────►│ TP_SCL             │
│                      │              │                    │
│             (SDA)    ├─────────────►│ TP_SDA             │
│                      │              │                    │
│             (TP_INT) ├─────────────►│ TP_INT             │
│                      │              │                    │
│             (3V3)    ├─────────────►│ TP_RST             │
│                      │              │                    │
└──────────────────────┘              └────────────────────┘
```

The GPIO number used by this example can be changed in [example_qspi_with_ram.c](main/example_qspi_with_ram.c).
Especially, please pay attention to the level used to turn on the LCD backlight, some LCD module needs a low level to turn it on, while others take a high level. You can change the backlight level macro `EXAMPLE_LCD_BK_LIGHT_ON_LEVEL` in [example_qspi_with_ram.c](main/example_qspi_with_ram.c).
The LCD vendor specific initialization can be different between manufacturers and should consult the LCD supplier for initialization sequence code.

### Build and Flash

Run `idf.py -p PORT build flash monitor` to build, flash and monitor the project. A fancy animation will show up on the LCD as expected.

The first time you run `idf.py` for the example will cost extra time as the build system needs to address the component dependencies and downloads the missing components from registry into `managed_components` folder.

(To exit the serial monitor, type ``Ctrl-]``.)

See the [Getting Started Guide](https://docs.espressif.com/projects/esp-idf/en/latest/get-started/index.html) for full steps to configure and use ESP-IDF to build projects.

### Example Output

```bash
...
I (415) example: Turn off LCD backlight
I (420) gpio: GPIO[0]| InputEn: 0| OutputEn: 1| OpenDrain: 0| Pullup: 0| Pulldown: 0| Intr:0
I (429) example: Initialize SPI bus
I (434) example: Install panel IO
I (438) example: Install SPD2010 panel driver
I (442) gpio: GPIO[17]| InputEn: 0| OutputEn: 1| OpenDrain: 0| Pullup: 0| Pulldown: 0| Intr:0
I (452) spd2010: LCD panel create success, version: 0.0.1
I (741) example: Turn on LCD backlight
I (741) example: Initialize LVGL library
I (741) example: Register display driver to LVGL
I (746) example: Install LVGL tick timer
I (748) example: Starting LVGL task
I (795) example: Display LVGL demos
I (1038) main_task: Returned from app_main()
...
```

## Troubleshooting

For any technical queries, please open an [issue] (https://github.com/espressif/esp-iot-solution/issues) on GitHub. We will get back to you soon.
"# esp32-robot-eyes" 
