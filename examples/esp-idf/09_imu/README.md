# 09_imu

[English](README.md) | [中文](README_CN.md)

QMI8658A spirit-level demo: the bubble moves with tilt; the bottom shows 3-axis acceleration (m/s²). Tap **Calibrate** for a level calibration (keep the board flat for about 2 seconds).

Full VS Code walkthrough: [../README.md](../README.md).

## Before you start

- USB to the board
- No SD card required
- The IMU shares I2C, so display init must run first (this example already does)

## Build and flash with VS Code / Cursor (recommended)

1. Install ESP-IDF v5.5.x and the **Espressif IDF** extension. Complete the wizard (IDF path + toolchain).
2. **File → Open Folder…** and select `examples/09_imu` (you must see this folder’s `CMakeLists.txt`). Do not open the parent `examples/` directory.
3. `Ctrl+Shift+P` → `ESP-IDF: Set Espressif device target` → **esp32s3**.
4. Click the serial port on the status bar. Command: `ESP-IDF: Select port to use`.
5. Click **Build** (hammer). The first build needs network.
6. Stop Monitor if it is already open.
7. Click **Flash**, then **Monitor**. Or run `ESP-IDF: Build, Flash and Monitor`.
8. Leave the monitor with `Ctrl+]`.

## Command line

```powershell
cd examples/09_imu
idf.py set-target esp32s3
idf.py build
idf.py -p COMx flash monitor
```

## How to tell flashing succeeded

- Flash log shows `Hash of data verified` / `Leaving...` with no fatal error.
- Screen title **IMU**, round level with a blue bubble in the center.
- Monitor: `bsp display init done`, plus `bsp_imu init done` or `QMI8658A found`.

If `IMU init failed`, check I2C (run `01_i2c_scan`; you should see `0x6B`).

## How to verify

1. At rest, one axis should be near gravity (about ±9.8).
2. Tilt the board; the bubble moves toward the “downhill” side.
3. Lay the board flat, tap **Calibrate**, keep still: status `keep flat...` → `calibrated`. Too much motion or timeout → `calib failed`; tap again.

Calibration logs: `start level calibration` / `calib PASSED` or `FAILED`.
