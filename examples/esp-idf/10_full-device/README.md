# smartring_plus_full_device

[English](README.md) | [中文](README_CN.md)

Full product demo for **SmartRing-Plus** (ESP32-S3-N16R8): clock, weather, recorder, music, IMU spirit level, album, and settings.

This folder is a **standalone ESP-IDF project**. On GitHub it lives next to the focused examples under `examples/` — see [../README.md](../README.md) for the shared VS Code walkthrough.

| Item | Value |
|------|--------|
| CMake project | `smartring_plus_full_device` |
| Author | **Ayang** |
| Company | **SHENZHEN VIEWE TECHNOLOGY CO.,LTD** |
| License | Apache-2.0 |
| IDF | v5.5.x (v5.5.4 recommended) |
| Target | `esp32s3` |
| BSP | [`viewesmart/smartring_plus`](https://components.espressif.com/components/viewesmart/smartring_plus) `^1.1.1` |

---

## Before you start

- USB cable from SmartRing-Plus to the PC
- Optional FAT32 MicroSD for music / album / recorder:

| Path on card | Use |
|--------------|-----|
| `/Music/` | `.mp3` (one extra subfolder level is OK) |
| `/Photos/` or `/DCIM/` | `.jpg` / `.jpeg` / `.png` |
| `/Recordings/` | Created automatically when you record |

Insert the card before power-on. This board has no card-detect pin.

---

## Build and flash with VS Code / Cursor (recommended)

1. Install [ESP-IDF v5.5.x](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/get-started/index.html) and the **Espressif IDF** extension. Complete the wizard (IDF path + toolchain).
2. **File → Open Folder…** and select **this** folder `examples/full-device` (you must see this folder’s `CMakeLists.txt`). Do **not** open the parent `examples/` directory.
3. `Ctrl+Shift+P` → `ESP-IDF: Set Espressif device target` → **esp32s3**.
4. Click the serial port on the status bar (Windows: `COMx`). Command: `ESP-IDF: Select port to use`.
5. Click **Build** (hammer), or `ESP-IDF: Build your project`. The first build needs network; this demo is larger than `01`–`09`.
6. If Monitor is open, stop it first (a busy port causes flash to fail).
7. Click **Flash**, then **Monitor**. Or run `ESP-IDF: Build, Flash and Monitor`.
8. Leave the monitor with `Ctrl+]`.

More status-bar detail: [../README.md](../README.md).

## Command line

```powershell
. D:\idf\.espressif\v5.5.4\esp-idf\export.ps1   # adjust to your machine
cd examples/full-device
idf.py set-target esp32s3
idf.py build
idf.py -p COMx flash monitor
```

Clean rebuild: `idf.py fullclean` then `idf.py build`.

---

## How to tell flashing succeeded

- Flash log shows `Hash of data verified` / `Leaving...` with no fatal error.
- After reset the round display lights up with the **circular home** (colored icon tiles around a small analog clock), not a single-example title such as `I2C Scan`.
- Monitor (TAG `smartring_plus_full_device`):

```
I (...) smartring_plus_full_device: smartring_plus_full_device starting
I (...) bsp: bsp display init done ...
I (...) smartring_plus_full_device: PMIC path: V2_AXP2101
I (...) smartring_plus_full_device: 初始化完成
```

`PMIC path` is `V2_AXP2101` or `V1_ADC`.  
`SD 卡未挂载` is only a warning if no FAT32 card is inserted — the rest of the UI still runs.

---

## How to verify

Home: tap a round tile to open a feature; use the back control to return.

| Feature | What you should see |
|---------|---------------------|
| **Clock** | Live `HH:MM:SS`. Until WiFi + SNTP succeed, the date may stay at 1970. |
| **Weather** | Needs WiFi from **Settings**. After connect, open Weather — city / condition / temperature. Log TAG `weather_service`. |
| **Recorder** | Needs SD. Record, then play back from the speaker. Log: `recorded: /sdcard/Recordings/...` |
| **Music** | Needs `/Music/*.mp3`. Play / pause / next. Log: `play: ...` |
| **IMU** | Bubble moves when you tilt. **Calibrate** with the board flat. |
| **Album** | Needs images in `/Photos` or `/DCIM`. Slideshow; swipe up to exit. |
| **Settings** | Battery mV / % / charging. **Scan WiFi** → tap an AP → password → IP. **Soft Power Off** only works on battery (GPIO47 cannot cut power while USB is plugged in). |

WiFi log when connected: `connected, IP: x.x.x.x SSID: ...` then `SNTP ok`.

---

## Directory layout

```
full-device/
├── CMakeLists.txt              # project(smartring_plus_full_device)
├── sdkconfig.defaults
├── partitions.csv
├── AUTHORS / LICENSE / README.md / README_CN.md
├── main/                       # app entry
├── components/
│   ├── ui_shell / ui_theme     # round home + theme
│   ├── feature_*               # clock / weather / recorder / music / imu / album / settings
│   ├── wifi_service
│   └── weather_service
├── managed_components/         # Component Manager (BSP, LVGL, codec, …)
└── .vscode/settings.json
```

`managed_components/` is created on the first `idf.py build`.

Build outputs: `build/smartring_plus_full_device.elf` / `.bin`.

---

## Hardware

- MCU: ESP32-S3-N16R8 (16 MB Flash + 8 MB Octal PSRAM)
- Panel: ST77916 QSPI **360×360** round LCD + CST816 touch
- Audio: ES8311, 48 kHz / 16-bit / stereo
- Storage: SDMMC 4-bit, mount point **`/sdcard`**
- Battery: **V1** GPIO1 ADC; **V2** AXP2101 (I2C `0x34`, detected at runtime); soft-off GPIO47 (same on both)

Pins: BSP header `bsp/board_config.h` in `viewesmart/smartring_plus`.

---

## Application components

BSP (registry), include:

```c
#include "bsp/smartring_plus.h"
```

| API | Role |
|-----|------|
| `bsp_display_init()` | Display + touch + LVGL adapter + backlight |
| `bsp_battery_init()` / `bsp_battery_get_pmic_type()` | V1 ADC / V2 AXP2101 |
| `bsp_power_init()` / `bsp_power_shutdown()` | GPIO47 |
| `bsp_sd_init()` | SD card |
| `bsp_audio_init()` / `bsp_audio_write()` | Audio |
| `bsp_imu_*` | IMU |

Wrap LVGL updates with `esp_lv_adapter_lock(-1)` / `unlock()`.

| Component | Role |
|-----------|------|
| `ui_theme` | Colors, fonts, round safe-area macros |
| `ui_shell` | Home, navigation, low-battery hint |
| `feature_*` | Feature screens |
| `wifi_service` | STA scan / connect |
| `weather_service` | Weather over HTTP |

To drop a feature, remove it from `main/CMakeLists.txt` and the register table in `main/main.c`.

---

## Troubleshooting

**Port busy / flash fails** (`Could not open COMx, the port is busy`): stop Monitor and other serial tools; USB replug may change COM.

**Garbled boot / `ESP_ERR_NO_MEM` (SPI DMA)**: log `setup_dma_priv_buffer ... Failed to allocate`. Keep a small LVGL flush height in the BSP (e.g. 40). Do not set it to the full screen height.

**Touch does nothing**: confirm `bsp_display_init()` and CST816 in the log; child objects must not steal `CLICKABLE`.

**Recorder / long file names**: `CONFIG_FATFS_LFN_HEAP=y` is already in `sdkconfig.defaults`. Confirm SD mounted.

**Music stalls**: confirm valid MP3 under `/sdcard/Music`.

**Weather fails**: connect WiFi in Settings first. This project uses HTTP; HTTPS would need a certificate bundle.

**Component Manager download fails**: check network / proxy; delete `managed_components` and `dependencies.lock`, then `idf.py build`.

**`export.ps1` fails (Windows)**: use the ESP-IDF PowerShell shortcut, or the extension’s status-bar terminal.

**Text clipped on the round panel**: keep widgets inside the safe area (`ui_theme.h`).

**Old behavior after a code change**: confirm you flashed this project’s `build/*.bin`; try `idf.py fullclean && idf.py build flash`.

---

Copyright © 2024-2026 **Ayang** / **SHENZHEN VIEWE TECHNOLOGY CO.,LTD**  
Licensed under the Apache License, Version 2.0 — see `LICENSE`, `AUTHORS`.
