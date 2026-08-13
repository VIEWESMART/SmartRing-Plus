# 08_album

[中文](README_CN.md) | [English](README.md)

全屏轮播 SD 卡里的图片，约每 3 秒一张。扫描目录：

- `/sdcard/Photos`
- `/sdcard/DCIM`

支持 `.jpg` / `.jpeg` / `.png`。

更完整的 VS Code 说明见 [../README_CN.md](../README_CN.md)。

## 准备工作

- FAT32 MicroSD
- 在 `Photos` 或 `DCIM` 放入若干不太大的图片（建议边长几百到一千像素，过大可能解码慢或内存不足）
- 插卡后上电

## 用 VS Code / Cursor 编译烧录（推荐）

1. 安装 ESP-IDF v5.5.x 和扩展 **Espressif IDF**，按向导选好 IDF 路径并安装工具链。
2. **文件 → 打开文件夹…**，选择本目录 `examples/08_album`（须能看到本目录的 `CMakeLists.txt`）。不要打开上一级 `examples/`。
3. `Ctrl+Shift+P` → `ESP-IDF: Set Espressif device target` → 选 **esp32s3**。
4. 点状态栏串口，选当前 USB 口。命令：`ESP-IDF: Select port to use`。
5. 点状态栏 **Build**（锤子）。首次需联网下载组件。
6. 若 Monitor 已打开，先关掉。
7. 点 **Flash**，完成后再点 **Monitor**。也可执行 `ESP-IDF: Build, Flash and Monitor`。
8. 退出监视器：`Ctrl+]`。

## 命令行

```powershell
cd examples/08_album
idf.py set-target esp32s3
idf.py build
idf.py -p COMx flash monitor
```

## 怎么判断烧录成功

- Flash 输出出现 `Hash of data verified` / `Leaving...`，没有 fatal error。
- 屏幕全屏显示图片，或提示 `put jpg/png in /Photos`。
- 监视器：

```
I (...) album: found N image(s)
I (...) bsp: bsp display init done ...
```

## 怎么验证

- `N > 0`：图片铺满圆屏，底部有 `1 / N`，约 3 秒切换，串口 `show S:...`。
- `N = 0`：按提示把图片放到 `Photos` 或 `DCIM` 后复位。
- 某张损坏会被跳过（`decoder open failed`）。

路径区分大小写时，请用 `Photos`、`DCIM` 这些名字。
