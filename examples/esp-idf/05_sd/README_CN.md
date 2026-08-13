# 05_sd

[中文](README_CN.md) | [English](README.md)

挂载 SDMMC 4-bit FAT 到 `/sdcard`，写入 `hello.txt`，再读回内容并列出根目录。

本板**没有卡检测脚**：没插卡和挂载失败在软件上无法区分。

更完整的 VS Code 说明见 [../README_CN.md](../README_CN.md)。

## 准备工作

- FAT32 格式的 MicroSD（建议 32 GB 及以下）
- 卡插入板载卡槽后再上电/复位
- USB 连接电脑

## 用 VS Code / Cursor 编译烧录（推荐）

1. 安装 ESP-IDF v5.5.x 和扩展 **Espressif IDF**，按向导选好 IDF 路径并安装工具链。
2. **文件 → 打开文件夹…**，选择本目录 `examples/05_sd`（须能看到本目录的 `CMakeLists.txt`）。不要打开上一级 `examples/`。
3. `Ctrl+Shift+P` → `ESP-IDF: Set Espressif device target` → 选 **esp32s3**。
4. 点状态栏串口，选当前 USB 口。命令：`ESP-IDF: Select port to use`。
5. 点状态栏 **Build**（锤子）。首次需联网下载组件。
6. 若 Monitor 已打开，先关掉。
7. 点 **Flash**，完成后再点 **Monitor**。也可执行 `ESP-IDF: Build, Flash and Monitor`。
8. 退出监视器：`Ctrl+]`。

## 命令行

```powershell
cd examples/05_sd
idf.py set-target esp32s3
idf.py build
idf.py -p COMx flash monitor
```

## 怎么判断烧录成功

- Flash 输出出现 `Hash of data verified` / `Leaving...`，没有 fatal error。
- 屏幕标题 **SD Card**。
- 监视器出现 `bsp display init done`，以及 TAG `sd`。

## 怎么验证

成功时串口：

```
I (...) sd: wrote /sdcard/hello.txt
```

屏幕显示 `wrote hello.txt` 以及读回的 `Hello SmartRing-Plus!`，下面是根目录文件名。

把卡拿到电脑上，应能看到 `hello.txt`。

失败：`mount failed` / `insert a FAT32 card`。请检查：是否插卡、是否 FAT32、接触是否良好。exFAT/NTFS 无法挂载。
