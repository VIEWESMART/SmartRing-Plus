# 07_recorder

[English](README.md) | [中文](README_CN.md)

Tap **Record** to capture 5 seconds (48 kHz / 16-bit / stereo) to `/sdcard/Recordings/hello.wav`, then play it back automatically.

Full VS Code walkthrough: [../README.md](../README.md).

## Before you start

- FAT32 MicroSD (recording writes to the card)
- Insert the card, then power on
- Speak near the on-board microphone or make some ambient sound

## Build and flash with VS Code / Cursor (recommended)

1. Install ESP-IDF v5.5.x and the **Espressif IDF** extension. Complete the wizard (IDF path + toolchain).
2. **File → Open Folder…** and select `examples/07_recorder` (you must see this folder’s `CMakeLists.txt`). Do not open the parent `examples/` directory.
3. `Ctrl+Shift+P` → `ESP-IDF: Set Espressif device target` → **esp32s3**.
4. Click the serial port on the status bar. Command: `ESP-IDF: Select port to use`.
5. Click **Build** (hammer). The first build needs network.
6. Stop Monitor if it is already open.
7. Click **Flash**, then **Monitor**. Or run `ESP-IDF: Build, Flash and Monitor`.
8. Leave the monitor with `Ctrl+]`.

## Command line

```powershell
cd examples/07_recorder
idf.py set-target esp32s3
idf.py build
idf.py -p COMx flash monitor
```

## How to tell flashing succeeded

- Flash log shows `Hash of data verified` / `Leaving...` with no fatal error.
- Screen title **Recorder**, a **Record** button, hint `tap to record 5s`.
- Monitor prints `bsp display init done`. Without a card: `SD not mounted` / `insert SD card`.

## How to verify

1. Tap **Record**; status becomes `recording 5s...` for about 5 seconds.
2. Then `playing...`; the speaker should play what you just recorded.
3. Finally `done`. Log:

```
I (...) recorder: recorded /sdcard/Recordings/hello.wav (... bytes)
```

On a PC you should be able to play `Recordings/hello.wav`.

No playback: mic gain is 36 dB in this example — stay close to the mic. Write failure: check the card is not read-only and is FAT32.
