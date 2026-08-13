# 08_album

[English](README.md) | [中文](README_CN.md)

Full-screen slideshow of images on the SD card, about one every 3 seconds. Scanned folders:

- `/sdcard/Photos`
- `/sdcard/DCIM`

Formats: `.jpg` / `.jpeg` / `.png`.

Full VS Code walkthrough: [../README.md](../README.md).

## Before you start

- FAT32 MicroSD
- Put a few moderate-size images in `Photos` or `DCIM` (a few hundred to ~1000 px on a side; huge files may decode slowly or run out of RAM)
- Insert the card, then power on

## Build and flash with VS Code / Cursor (recommended)

1. Install ESP-IDF v5.5.x and the **Espressif IDF** extension. Complete the wizard (IDF path + toolchain).
2. **File → Open Folder…** and select `examples/08_album` (you must see this folder’s `CMakeLists.txt`). Do not open the parent `examples/` directory.
3. `Ctrl+Shift+P` → `ESP-IDF: Set Espressif device target` → **esp32s3**.
4. Click the serial port on the status bar. Command: `ESP-IDF: Select port to use`.
5. Click **Build** (hammer). The first build needs network.
6. Stop Monitor if it is already open.
7. Click **Flash**, then **Monitor**. Or run `ESP-IDF: Build, Flash and Monitor`.
8. Leave the monitor with `Ctrl+]`.

## Command line

```powershell
cd examples/08_album
idf.py set-target esp32s3
idf.py build
idf.py -p COMx flash monitor
```

## How to tell flashing succeeded

- Flash log shows `Hash of data verified` / `Leaving...` with no fatal error.
- Full-screen image, or the hint `put jpg/png in /Photos`.
- Monitor:

```
I (...) album: found N image(s)
I (...) bsp: bsp display init done ...
```

## How to verify

- `N > 0`: image covers the round screen, `1 / N` at the bottom, advances about every 3 s, log `show S:...`.
- `N = 0`: copy images into `Photos` or `DCIM` and reset.
- A corrupt file is skipped (`decoder open failed`).

Use the names `Photos` and `DCIM` if the host filesystem is case-sensitive.
