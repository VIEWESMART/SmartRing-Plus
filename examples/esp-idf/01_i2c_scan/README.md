# 01_i2c_scan

[English](README.md) | [中文](README_CN.md)

Scan the board’s shared I2C bus (SDA=GPIO8, SCL=GPIO9, 400 kHz). Addresses that ACK are printed to the serial log and shown on the round display.

Use this to confirm touch, codec, IMU, and (on V2) the PMIC are present on the bus.

Full VS Code walkthrough: [../README.md](../README.md).

## Before you start

- Connect SmartRing-Plus to the PC over USB
- No SD card required

## Build and flash with VS Code / Cursor (recommended)

1. Install [ESP-IDF v5.5.x](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/get-started/index.html) and the **Espressif IDF** extension. Complete the wizard (IDF path + toolchain).
2. **File → Open Folder…** and select this folder `examples/01_i2c_scan` (you must see this folder’s `CMakeLists.txt`). Do not open the parent `examples/` directory.
3. `Ctrl+Shift+P` → `ESP-IDF: Set Espressif device target` → **esp32s3**.
4. Click the serial port on the status bar and pick the USB port (Windows: `COMx`). Command: `ESP-IDF: Select port to use`.
5. Click **Build** (hammer), or `ESP-IDF: Build your project`. The first build needs network to fetch components.
6. If Monitor is open, stop it first (a busy port causes flash to fail).
7. Click **Flash**, then **Monitor**. Or run `ESP-IDF: Build, Flash and Monitor`.
8. Leave the monitor with `Ctrl+]`.

## Command line

```powershell
cd examples/01_i2c_scan
idf.py set-target esp32s3
idf.py build
idf.py -p COMx flash monitor
```

## How to tell flashing succeeded

- Flash log shows `Hash of data verified` / `Leaving...` with no fatal error.
- After reset the display lights up with title **I2C Scan**.
- Monitor prints `bsp display init done` and TAG `i2c_scan`.

## How to verify

Serial output looks like:

```
I (...) i2c_scan: scan I2C (SDA=8 SCL=9 400kHz) ...
I (...) i2c_scan:   found 0x15  CST816 touch
I (...) i2c_scan:   found 0x18  ES8311 codec
I (...) i2c_scan:   found 0x6B  QMI8658A IMU
I (...) i2c_scan: done, N device(s)
```

- V2 boards may also show `0x34  AXP2101 PMIC (V2)`.
- The screen lists the same addresses.
- `done, 0 device(s)` / `no ACK on bus`: wrong firmware, or an I2C hardware issue.

## Code notes

`bsp_display_init()` creates the shared I2C bus. Scanning uses `i2c_master_probe()` over `0x08~0x77`.
