# 07_recorder

[中文](README_CN.md) | [English](README.md)

触摸 **Record** 后录音 5 秒（48 kHz / 16bit / 立体声），保存为 `/sdcard/Recordings/hello.wav`，录完自动回放。

更完整的 VS Code 说明见 [../README_CN.md](../README_CN.md)。

## 准备工作

- FAT32 MicroSD（录音要写卡）
- 插卡后上电
- 对着板载麦克风说话或制造环境声

## 用 VS Code / Cursor 编译烧录（推荐）

1. 安装 ESP-IDF v5.5.x 和扩展 **Espressif IDF**，按向导选好 IDF 路径并安装工具链。
2. **文件 → 打开文件夹…**，选择本目录 `examples/07_recorder`（须能看到本目录的 `CMakeLists.txt`）。不要打开上一级 `examples/`。
3. `Ctrl+Shift+P` → `ESP-IDF: Set Espressif device target` → 选 **esp32s3**。
4. 点状态栏串口，选当前 USB 口。命令：`ESP-IDF: Select port to use`。
5. 点状态栏 **Build**（锤子）。首次需联网下载组件。
6. 若 Monitor 已打开，先关掉。
7. 点 **Flash**，完成后再点 **Monitor**。也可执行 `ESP-IDF: Build, Flash and Monitor`。
8. 退出监视器：`Ctrl+]`。

## 命令行

```powershell
cd examples/07_recorder
idf.py set-target esp32s3
idf.py build
idf.py -p COMx flash monitor
```

## 怎么判断烧录成功

- Flash 输出出现 `Hash of data verified` / `Leaving...`，没有 fatal error。
- 屏幕标题 **Recorder**，有 **Record** 按钮，提示 `tap to record 5s`。
- 监视器出现 `bsp display init done`。若未插卡，提示 `SD not mounted` / `insert SD card`。

## 怎么验证

1. 点 **Record**，状态变为 `recording 5s...`（约 5 秒）。
2. 随后变为 `playing...`，扬声器应能听到刚录的声音。
3. 结束后显示 `done`。串口：

```
I (...) recorder: recorded /sdcard/Recordings/hello.wav (... bytes)
```

把卡插到电脑，应能播放 `Recordings/hello.wav`。

无声回放：录音增益本例设为 36 dB，请靠近麦克风。写失败：检查 SD 是否只读、是否 FAT32。
