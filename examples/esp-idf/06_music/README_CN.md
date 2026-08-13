# 06_music

[中文](README_CN.md) | [English](README.md)

播放 SD 卡 `/Music` 目录下的 `.mp3`。

- 上一曲 / 暂停（播放）/ 下一曲
- **Library** 打开曲库（默认关闭），点曲目即播
- 当前曲播完自动下一首
- ES8311 固定 48 kHz；非 48k 源会做简易重采样

更完整的 VS Code 说明见 [../README_CN.md](../README_CN.md)。

## 准备工作

- FAT32 MicroSD
- 在电脑上建立目录 `Music`，放入至少一个 `.mp3`（可再套一层子文件夹）
- 插卡后再上电

## 用 VS Code / Cursor 编译烧录（推荐）

1. 安装 ESP-IDF v5.5.x 和扩展 **Espressif IDF**，按向导选好 IDF 路径并安装工具链。
2. **文件 → 打开文件夹…**，选择本目录 `examples/06_music`（须能看到本目录的 `CMakeLists.txt`）。不要打开上一级 `examples/`。
3. `Ctrl+Shift+P` → `ESP-IDF: Set Espressif device target` → 选 **esp32s3**。
4. 点状态栏串口，选当前 USB 口。命令：`ESP-IDF: Select port to use`。
5. 点状态栏 **Build**（锤子）。首次需联网下载组件。
6. 若 Monitor 已打开，先关掉。
7. 点 **Flash**，完成后再点 **Monitor**。也可执行 `ESP-IDF: Build, Flash and Monitor`。
8. 退出监视器：`Ctrl+]`。

## 命令行

```powershell
cd examples/06_music
idf.py set-target esp32s3
idf.py build
idf.py -p COMx flash monitor
```

## 怎么判断烧录成功

- Flash 输出出现 `Hash of data verified` / `Leaving...`，没有 fatal error。
- 屏幕标题 **Music**，有曲名、进度条和三个圆形按键。
- 监视器：

```
I (...) music: found N track(s)
I (...) bsp: bsp display init done ...
```

`N` 为扫描到的 MP3 数量。`found 0` 表示没扫到文件。

## 怎么验证

1. 点中间播放键，扬声器应出声；图标变为暂停。
2. 再点应变静音/暂停，状态显示 `Paused`。
3. 上一曲 / 下一曲会换文件，串口出现 `play [n] /sdcard/Music/...`。
4. 点 **Library** 弹出列表，点某一首开始播放，关闭后回到播放页。

无声：确认 `bsp_audio_init` 成功、音量约 75、MP3 未损坏、PA 未被静音。无曲目：路径必须是卡根目录下的 `Music`（注意大小写，部分电脑区分）。
