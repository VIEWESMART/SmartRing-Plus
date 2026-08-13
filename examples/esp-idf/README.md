# SmartRing-Plus Examples

[English](README.md) | [中文](README_CN.md)

Beginner-oriented examples for **SmartRing-Plus** (ESP32-S3-N16R8).  
Each subdirectory is a **standalone ESP-IDF project**. Board support comes from  
[`viewesmart/smartring_plus`](https://components.espressif.com/components/viewesmart/smartring_plus) `^1.1.1`.

On GitHub these folders are **siblings** under `examples/`.  
In VS Code / Cursor, **File → Open Folder** on **one** of them (the folder that contains that project’s `CMakeLists.txt`).  
Do **not** open this `examples/` directory itself — the IDF extension would pick the wrong project.

| Example | What it does |
|---------|----------------|
| [full-device](full-device) | Full product demo: clock, weather, recorder, music, IMU, album, settings |
| [01_i2c_scan](01_i2c_scan) | Scan the shared I2C bus and list addresses |
| [02_battery](02_battery) | Voltage, SoC, charging status |
| [03_wifi](03_wifi) | STA connect (SSID/password via menuconfig) |
| [04_lvgl_port](04_lvgl_port) | LVGL port; official Widgets Demo |
| [05_sd](05_sd) | Mount SD, write and read back `hello.txt` |
| [06_music](06_music) | Play MP3 files under `/Music` (prev / pause / next) |
| [07_recorder](07_recorder) | Record 5 s WAV and play it back |
| [08_album](08_album) | Slideshow from `/Photos` or `/DCIM` |
| [09_imu](09_imu) | IMU spirit level + calibration |

Suggested order: run `01`–`09` to learn one peripheral at a time, then flash [full-device](full-device) to see the complete UI.  
Each example has `README.md` (English) and `README_CN.md` (Chinese).

---

## Build and flash with VS Code / Cursor

These steps are the same for every example, including `full-device`. Replace the folder name as needed.

### 1. One-time setup

1. Install [ESP-IDF v5.5.x](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/get-started/index.html) (v5.5.4 recommended).
2. Install [VS Code](https://code.visualstudio.com/) or Cursor.
3. Install the **Espressif IDF** extension from the marketplace.
4. Complete the extension wizard:
   - Select your ESP-IDF path
   - Install the toolchain (`ESP-IDF: Configure ESP-IDF extension`)
5. Connect SmartRing-Plus over USB and note the serial port (Windows: `COMx`).

### 2. Open the example project

1. **File → Open Folder…**
2. Select **one** example folder, for example:
   - `examples/full-device`
   - `examples/01_i2c_scan`
3. You should see that folder’s `CMakeLists.txt`, `sdkconfig.defaults`, and `main/`
4. **Do not** open the parent `examples/` folder

### 3. Set target and serial port

Check the **status bar** at the bottom:

| Item | Expected | If wrong |
|------|----------|----------|
| IDF | Project detected | Command Palette: `ESP-IDF: Configure ESP-IDF extension` |
| Target | `esp32s3` | `ESP-IDF: Set Espressif device target` → `esp32s3` |
| Port | Current USB port, e.g. `COM5` | Click the port in the status bar, or `ESP-IDF: Select port to use` |

Command Palette: `Ctrl+Shift+P` (macOS: `Cmd+Shift+P`).

### 4. Build, flash, monitor

1. Click **Build** (hammer) on the status bar, or run `ESP-IDF: Build your project`.  
   The first build downloads `managed_components/` (needs network). `full-device` takes longer than the small examples.
2. If Monitor is already open, stop it first (a busy port causes flash to fail).
3. Click **Flash** (lightning), or `ESP-IDF: Flash your project`.
4. After flashing, click **Monitor**, or `ESP-IDF: Monitor device`.  
   Baud rate 115200; this board uses USB-Serial-JTAG.
5. Leave the monitor with `Ctrl+]`.

Useful commands:

| Command | Action |
|---------|--------|
| `ESP-IDF: Build your project` | Build |
| `ESP-IDF: Flash your project` | Flash |
| `ESP-IDF: Monitor device` | Serial log |
| `ESP-IDF: Build, Flash and Monitor` | All three |
| `ESP-IDF: SDK Configuration editor (menuconfig)` | GUI menuconfig (`03_wifi`) |
| `ESP-IDF: Full clean` | Clean rebuild |

For `03_wifi`, open the **SDK Configuration editor**, set SSID/password, save, then Build.  
For `full-device`, WiFi is configured on the **Settings** screen, not in menuconfig.

---

## Command line

```powershell
. D:\idf\.espressif\v5.5.4\esp-idf\export.ps1   # adjust to your machine

cd examples/full-device          # or examples/01_i2c_scan, ...
idf.py set-target esp32s3        # first time only for this example
idf.py build
idf.py -p COMx flash monitor     # replace COMx
```

---

## How to tell flashing succeeded

- Log shows `Hash of data verified` / `Leaving...` and no `A fatal error`.
- After reset the **round display lights up** (backlight on), not a black or garbled screen.
- Monitor prints `bsp display init done` plus that example’s TAG.
- If you still see an old UI, you flashed the wrong project: confirm the opened folder and that Flash completed.

Each example README lists how to verify that example.

---

## Shared conventions

- Call `bsp_display_init()` first (I2C, panel, touch, LVGL), then battery / audio / IMU / SD.
- Wrap LVGL updates with `esp_lv_adapter_lock(-1)` / `unlock()`.
- SD: FAT32, mount point `/sdcard`; this board has no card-detect pin.
- Music: `/sdcard/Music/*.mp3`; album: `/sdcard/Photos` or `/DCIM`; recorder: `/sdcard/Recordings/`.

Copyright © 2024-2026 Ayang / SHENZHEN VIEWE TECHNOLOGY CO.,LTD · Apache-2.0
