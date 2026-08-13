# 03_wifi

[English](README.md) | [中文](README_CN.md)

WiFi STA example: connect to an AP and show the IP on the display.

Set SSID / password in **menuconfig**. Do not hard-code secrets in source.

Full VS Code walkthrough: [../README.md](../README.md).

## Before you start

- USB to the board
- 2.4 GHz WiFi only (ESP32-S3 has no 5 GHz radio)
- Defaults are placeholders `myssid` / `mypassword` — **change them before flashing**

## Build and flash with VS Code / Cursor (recommended)

1. Install ESP-IDF v5.5.x and the **Espressif IDF** extension. Complete the wizard (IDF path + toolchain).
2. **File → Open Folder…** and select `examples/03_wifi` (you must see this folder’s `CMakeLists.txt`). Do not open the parent `examples/` directory.
3. `Ctrl+Shift+P` → `ESP-IDF: Set Espressif device target` → **esp32s3**.
4. Click the serial port on the status bar.
5. **Configure the AP (required):** `Ctrl+Shift+P` → `ESP-IDF: SDK Configuration editor (menuconfig)`  
   Open **Example Configuration**:
   - `WiFi SSID`
   - `WiFi Password` (leave empty for an open network)  
   Save and close the editor.
6. Click **Build** (hammer). The first build needs network.
7. Stop Monitor if it is already open.
8. Click **Flash**, then **Monitor**. Or run `ESP-IDF: Build, Flash and Monitor`.
9. Leave the monitor with `Ctrl+]`.

After changing menuconfig you must Build again, or the old SSID stays in the firmware.

## Command line

```powershell
cd examples/03_wifi
idf.py set-target esp32s3
idf.py menuconfig
idf.py build
idf.py -p COMx flash monitor
```

In `idf.py menuconfig`, fill SSID / password under **Example Configuration**.

## How to tell flashing succeeded

- Flash log shows `Hash of data verified` / `Leaving...` with no fatal error.
- Screen title **WiFi**, SSID in the middle, status starts as `connecting...`.
- Monitor prints `bsp display init done` and `connecting to "your SSID"`.

## How to verify

Success:

```
I (...) wifi: got IP 192.168.x.x
```

The screen turns green **connected** and shows the IP.

Failure: red **connect failed**, log `connect failed (check SSID/password in menuconfig)`.

Check 2.4 GHz, password, and distance to the AP.
