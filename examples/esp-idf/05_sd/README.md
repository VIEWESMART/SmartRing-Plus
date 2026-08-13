# 05_sd

[English](README.md) | [中文](README_CN.md)

Mount SDMMC 4-bit FAT at `/sdcard`, write `hello.txt`, read it back, and list the root directory.

This board has **no card-detect pin**: “no card” and “mount failed” look the same in software.

Full VS Code walkthrough: [../README.md](../README.md).

## Before you start

- FAT32 MicroSD (32 GB or smaller recommended)
- Insert the card before power-on / reset
- USB to the PC

## Build and flash with VS Code / Cursor (recommended)

1. Install ESP-IDF v5.5.x and the **Espressif IDF** extension. Complete the wizard (IDF path + toolchain).
2. **File → Open Folder…** and select `examples/05_sd` (you must see this folder’s `CMakeLists.txt`). Do not open the parent `examples/` directory.
3. `Ctrl+Shift+P` → `ESP-IDF: Set Espressif device target` → **esp32s3**.
4. Click the serial port on the status bar. Command: `ESP-IDF: Select port to use`.
5. Click **Build** (hammer). The first build needs network.
6. Stop Monitor if it is already open.
7. Click **Flash**, then **Monitor**. Or run `ESP-IDF: Build, Flash and Monitor`.
8. Leave the monitor with `Ctrl+]`.

## Command line

```powershell
cd examples/05_sd
idf.py set-target esp32s3
idf.py build
idf.py -p COMx flash monitor
```

## How to tell flashing succeeded

- Flash log shows `Hash of data verified` / `Leaving...` with no fatal error.
- Screen title **SD Card**.
- Monitor prints `bsp display init done` and TAG `sd`.

## How to verify

On success the log shows:

```
I (...) sd: wrote /sdcard/hello.txt
```

The screen shows `wrote hello.txt`, the read-back `Hello SmartRing-Plus!`, and root-directory names.

On a PC the card should contain `hello.txt`.

Failure: `mount failed` / `insert a FAT32 card`. Check that a card is inserted, it is FAT32, and the contacts are clean. exFAT/NTFS will not mount.
