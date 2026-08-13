# 02_battery

[English](README.md) | [中文](README_CN.md)

Read battery voltage, SoC, and charging status. The BSP picks the PMIC at runtime:

- **V1**: GPIO1 ADC + OCV lookup
- **V2**: AXP2101 E-Gauge (I2C `0x34`)

The UI refreshes about once per second. Plug/unplug USB to switch between `charging / USB` and `on battery`.

This example has no soft-shutdown button: GPIO47 cannot power the board off while USB is connected.

Full VS Code walkthrough: [../README.md](../README.md).

## Before you start

- USB to the board (a charged cell is recommended so SoC is meaningful)
- No SD card required

## Build and flash with VS Code / Cursor (recommended)

1. Install ESP-IDF v5.5.x and the **Espressif IDF** extension. Complete the wizard (IDF path + toolchain).
2. **File → Open Folder…** and select `examples/02_battery` (you must see this folder’s `CMakeLists.txt`). Do not open the parent `examples/` directory.
3. `Ctrl+Shift+P` → `ESP-IDF: Set Espressif device target` → **esp32s3**.
4. Click the serial port on the status bar. Command: `ESP-IDF: Select port to use`.
5. Click **Build** (hammer). The first build needs network.
6. Stop Monitor if it is already open.
7. Click **Flash**, then **Monitor**. Or run `ESP-IDF: Build, Flash and Monitor`.
8. Leave the monitor with `Ctrl+]`.

## Command line

```powershell
cd examples/02_battery
idf.py set-target esp32s3
idf.py build
idf.py -p COMx flash monitor
```

## How to tell flashing succeeded

- Flash log shows `Hash of data verified` / `Leaving...` with no fatal error.
- Screen title **Battery**, with voltage and percent.
- Monitor prints `bsp display init done` and:

```
I (...) battery: PMIC=V2 AXP2101  cell=3.7V/600mAh
```

or `PMIC=V1 ADC` on boards without AXP2101.

## How to verify

- One log line per second: `bat: x.xxV nn% charging` or `on-battery`.
- USB plugged in: status `charging / USB` (green).
- USB unplugged (battery only): `on battery` (voltage may drop slightly under load).
- SoC is 0–100. V2 voltage is typically about 3.3–4.2 V.

If voltage stays at 0: confirm `bsp_battery_init()` succeeded; on V2 check that the CHIP ID log is `0x4A`.
