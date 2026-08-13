# 01_i2c_scan

[中文](README_CN.md) | [English](README.md)

扫描板上共享 I2C 总线（SDA=GPIO8、SCL=GPIO9、400 kHz），把有应答的 7 位地址打印到串口，并显示在圆屏上。

本例用来确认触摸、codec、IMU、V2 PMIC 是否挂在总线上。

更完整的 VS Code 说明见 [../README_CN.md](../README_CN.md)。

## 准备工作

- SmartRing-Plus 用 USB 接到电脑
- 不需要 SD 卡

## 用 VS Code / Cursor 编译烧录（推荐）

1. 安装 [ESP-IDF v5.5.x](https://docs.espressif.com/projects/esp-idf/zh_CN/latest/esp32s3/get-started/index.html) 和扩展 **Espressif IDF**，按向导选好 IDF 路径并安装工具链。
2. **文件 → 打开文件夹…**，选择本目录 `examples/01_i2c_scan`（须能看到本目录的 `CMakeLists.txt`）。不要打开上一级 `examples/`。
3. `Ctrl+Shift+P` → `ESP-IDF: Set Espressif device target` → 选 **esp32s3**。
4. 点状态栏串口，选当前 USB 口（Windows 常见 `COMx`）。命令：`ESP-IDF: Select port to use`。
5. 点状态栏 **Build**（锤子），或 `ESP-IDF: Build your project`。首次需联网下载组件。
6. 若 Monitor 已打开，先关掉（端口占用会导致烧录失败）。
7. 点 **Flash**（闪电），完成后再点 **Monitor**。也可一次执行 `ESP-IDF: Build, Flash and Monitor`。
8. 退出监视器：`Ctrl+]`。

## 命令行

```powershell
cd examples/01_i2c_scan
idf.py set-target esp32s3
idf.py build
idf.py -p COMx flash monitor
```

## 怎么判断烧录成功

- Flash 输出出现 `Hash of data verified` / `Leaving...`，没有 fatal error。
- 复位后屏幕亮，标题为 **I2C Scan**。
- 监视器出现 `bsp display init done`，以及 TAG `i2c_scan`。

## 怎么验证

串口类似：

```
I (...) i2c_scan: scan I2C (SDA=8 SCL=9 400kHz) ...
I (...) i2c_scan:   found 0x15  CST816 touch
I (...) i2c_scan:   found 0x18  ES8311 codec
I (...) i2c_scan:   found 0x6B  QMI8658A IMU
I (...) i2c_scan: done, N device(s)
```

- V2 板还可能出现 `0x34  AXP2101 PMIC (V2)`。
- 屏幕列出相同地址。
- 若 `done, 0 device(s)` / `no ACK on bus`：检查是否烧错工程，或 I2C 硬件异常。

## 代码要点

`bsp_display_init()` 会创建共享 I2C。扫描用 `i2c_master_probe()`，范围内 `0x08~0x77`。
