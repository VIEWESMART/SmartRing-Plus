# 04_lvgl_port

[English](README.md) | [中文](README_CN.md)

Verify display + touch + LVGL: after `bsp_display_init()` the official `lv_demo_widgets()` runs.

On a 360×360 round panel the demo layout is tight and edges may be clipped. That is expected. Check that widgets render and touch works.

Full VS Code walkthrough: [../README.md](../README.md).

## Before you start

- USB to the board
- No SD card required

## Build and flash with VS Code / Cursor (recommended)

1. Install ESP-IDF v5.5.x and the **Espressif IDF** extension. Complete the wizard (IDF path + toolchain).
2. **File → Open Folder…** and select `examples/04_lvgl_port` (you must see this folder’s `CMakeLists.txt`). Do not open the parent `examples/` directory.
3. `Ctrl+Shift+P` → `ESP-IDF: Set Espressif device target` → **esp32s3**.
4. Click the serial port on the status bar. Command: `ESP-IDF: Select port to use`.
5. Click **Build** (hammer). Widgets Demo makes a larger firmware, so the build takes longer. The first build needs network.
6. Stop Monitor if it is already open.
7. Click **Flash**, then **Monitor**. Or run `ESP-IDF: Build, Flash and Monitor`.
8. Leave the monitor with `Ctrl+]`.

## Command line

```powershell
cd examples/04_lvgl_port
idf.py set-target esp32s3
idf.py build
idf.py -p COMx flash monitor
```

## How to tell flashing succeeded

- Flash log shows `Hash of data verified` / `Leaving...` with no fatal error.
- The screen shows the LVGL Demo (Profile / Analytics / Shop tabs), not the full-device home.
- Monitor:

```
I (...) lvgl_port: display ready, start lv_demo_widgets()
I (...) bsp: bsp display init done ...
```

## How to verify

- Sliders, charts, and buttons are visible.
- Touch swipe/tap works (tabs switch).
- Garbled screen or no touch: confirm this example was flashed, and the log contains `swap_bytes=1`.
