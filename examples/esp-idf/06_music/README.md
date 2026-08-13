# 06_music

[English](README.md) | [中文](README_CN.md)

Play `.mp3` files from the SD card `/Music` folder.

- Previous / pause (play) / next
- **Library** opens the playlist (hidden by default); tap a track to play
- Auto-advance when a track ends
- ES8311 is fixed at 48 kHz; other rates are resampled simply

Full VS Code walkthrough: [../README.md](../README.md).

## Before you start

- FAT32 MicroSD
- Create a `Music` folder on the card and put at least one `.mp3` in it (one extra subfolder level is OK)
- Insert the card, then power on

## Build and flash with VS Code / Cursor (recommended)

1. Install ESP-IDF v5.5.x and the **Espressif IDF** extension. Complete the wizard (IDF path + toolchain).
2. **File → Open Folder…** and select `examples/06_music` (you must see this folder’s `CMakeLists.txt`). Do not open the parent `examples/` directory.
3. `Ctrl+Shift+P` → `ESP-IDF: Set Espressif device target` → **esp32s3**.
4. Click the serial port on the status bar. Command: `ESP-IDF: Select port to use`.
5. Click **Build** (hammer). The first build needs network.
6. Stop Monitor if it is already open.
7. Click **Flash**, then **Monitor**. Or run `ESP-IDF: Build, Flash and Monitor`.
8. Leave the monitor with `Ctrl+]`.

## Command line

```powershell
cd examples/06_music
idf.py set-target esp32s3
idf.py build
idf.py -p COMx flash monitor
```

## How to tell flashing succeeded

- Flash log shows `Hash of data verified` / `Leaving...` with no fatal error.
- Screen title **Music**, with title, progress bar, and three round buttons.
- Monitor:

```
I (...) music: found N track(s)
I (...) bsp: bsp display init done ...
```

`N` is the number of MP3 files. `found 0` means none were found.

## How to verify

1. Tap the center play button: the speaker should play; the icon becomes pause.
2. Tap again: audio pauses, status shows `Paused`.
3. Prev / next changes the file; the log shows `play [n] /sdcard/Music/...`.
4. Tap **Library** for the list, tap a track to play, close to return to the player.

No sound: confirm `bsp_audio_init` succeeded, volume ~75, the MP3 is valid, PA is not muted. No tracks: the folder must be `Music` at the card root (case-sensitive on some hosts).
