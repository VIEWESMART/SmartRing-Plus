# SmartRing-Plus 示例

[中文](README_CN.md) | [English](README.md)

面向开发者的 **SmartRing-Plus**（ESP32-S3-N16R8）入门示例。  
每个子目录都是**独立 ESP-IDF 工程**。板级驱动来自组件库  
[`viewesmart/smartring_plus`](https://components.espressif.com/components/viewesmart/smartring_plus) `^1.1.1`。

在 GitHub 上，这些目录都是 `examples/` 下的**同级文件夹**。  
请用 VS Code / Cursor **打开某一个子目录**（能看到该工程自己的 `CMakeLists.txt`），  
**不要**打开本 `examples/` 目录本身，否则 IDF 扩展会认错工程。

| 示例 | 说明 |
|------|------|
| [01_i2c_scan](01_i2c_scan) | 扫描共享 I2C 并列出设备地址 |
| [02_battery](02_battery) | 显示电压、电量、充电状态 |
| [03_wifi](03_wifi) | STA 连网（SSID/密码在 menuconfig） |
| [04_lvgl_port](04_lvgl_port) | LVGL 移植，运行官方 Widgets Demo |
| [05_sd](05_sd) | 挂载 SD，写入并读回 `hello.txt` |
| [06_music](06_music) | 播放 `/Music` 下 MP3（上一曲/暂停/下一曲） |
| [07_recorder](07_recorder) | 录 5 秒 WAV 并回放 |
| [08_album](08_album) | 轮播 `/Photos` 或 `/DCIM` 图片 |
| [09_imu](09_imu) | IMU 水平仪 + 校准 |
| [10_full-device](10_full-device) | SmartRing-Plus 整机演示：时钟、天气、录音、音乐、IMU、相册、设置 |
| [11_xiaozhi](11_xiaozhi) | AI Xiaozhi 2.0 示例 |

建议顺序：先跑 `01`～`09` 熟悉单个外设，再烧录 [10_full-device](10_full-device) 看完整界面。  
每个示例目录同时提供 `README.md`（英文）与 `README_CN.md`（中文）。

---

## 通用：用 VS Code / Cursor 编译烧录

以下步骤对所有示例相同（含 `full-device`）。把目录名换成你要跑的那个即可。

### 1. 环境准备（只需做一次）

1. 安装 [ESP-IDF v5.5.x](https://docs.espressif.com/projects/esp-idf/zh_CN/latest/esp32s3/get-started/index.html)（建议 v5.5.4）。
2. 安装 [VS Code](https://code.visualstudio.com/) 或 Cursor。
3. 打开扩展市场，搜索并安装 **Espressif IDF**。
4. 按扩展向导完成：
   - 选择本机 ESP-IDF 路径
   - 安装工具链（ESP-IDF: Configure ESP-IDF extension）
5. USB 连接 SmartRing-Plus，在设备管理器记下串口（Windows 常见 `COMx`）。

### 2. 打开示例工程

1. **文件 → 打开文件夹…**（File → Open Folder）
2. 选中**某一个**示例目录，例如：
   - `examples/full-device`
   - `examples/01_i2c_scan`
3. 资源管理器里应能直接看到该目录的 `CMakeLists.txt`、`sdkconfig.defaults`、`main/`
4. **不要**打开上一级 `examples/` 目录

### 3. 设置目标芯片和串口

看窗口**底部状态栏**：

| 项 | 应显示 | 不对时怎么改 |
|----|--------|----------------|
| IDF | 已识别当前工程 | 命令面板：`ESP-IDF: Configure ESP-IDF extension` |
| Target | `esp32s3` | 命令面板：`ESP-IDF: Set Espressif device target` → `esp32s3` |
| 串口 | 当前 USB 口，如 `COM5` | 点状态栏端口，或命令面板：`ESP-IDF: Select port to use` |

命令面板快捷键：`Ctrl+Shift+P`（macOS：`Cmd+Shift+P`）。

### 4. 编译、烧录、看日志

1. 点状态栏 **Build**（锤子图标），或命令面板：`ESP-IDF: Build your project`  
   首次会下载 `managed_components/`，需联网。`full-device` 比单外设示例编译更久。
2. 编译成功后，若已经打开 Monitor，先点 **Stop** / 关掉监视器（端口被占用会导致烧录失败）。
3. 点状态栏 **Flash**（闪电），或：`ESP-IDF: Flash your project`
4. 烧录完成后点 **Monitor**（插头），或：`ESP-IDF: Monitor device`  
   波特率 115200，本板走 USB-Serial-JTAG。
5. 退出监视器：在监视器终端按 `Ctrl+]`。

常用命令面板：

| 命令 | 作用 |
|------|------|
| `ESP-IDF: Build your project` | 编译 |
| `ESP-IDF: Flash your project` | 烧录 |
| `ESP-IDF: Monitor device` | 串口日志 |
| `ESP-IDF: Build, Flash and Monitor` | 三步一次做完 |
| `ESP-IDF: SDK Configuration editor (menuconfig)` | 图形 menuconfig（`03_wifi` 要用） |
| `ESP-IDF: Full clean` | 干净重建 |

`03_wifi` 必须先用 **SDK Configuration editor** 改 SSID/密码，保存后再 Build。  
`full-device` 的 WiFi 在屏幕 **Settings** 里配置，不走 menuconfig。

---

## 通用：命令行

```powershell
. D:\idf\.espressif\v5.5.4\esp-idf\export.ps1   # 路径按本机修改

cd examples/full-device          # 或 examples/01_i2c_scan 等
idf.py set-target esp32s3        # 仅该示例第一次需要
idf.py build
idf.py -p COMx flash monitor     # 把 COMx 换成实际端口
```

---

## 怎么判断烧录成功

- 输出出现 `Hash of data verified` / `Leaving...`，没有 `A fatal error`。
- 板子复位后**圆屏亮起**（背光开），不是花屏或黑屏。
- 监视器出现 `bsp display init done`，以及该示例的 TAG。
- 若一直停在旧界面，说明烧的不是当前示例：确认打开的是对应子目录，并已 Flash 成功。

各示例 README 里有该例的验证步骤和串口关键字。

---

## 公共约定

- 必须先 `bsp_display_init()`（创建 I2C、屏、触摸、LVGL），再初始化电池 / 音频 / IMU / SD。
- 改 LVGL 界面前后要 `esp_lv_adapter_lock(-1)` / `unlock()`。
- SD 卡：FAT32，挂载点 `/sdcard`；本板无卡检测脚。
- 音乐：`/sdcard/Music/*.mp3`；相册：`/sdcard/Photos` 或 `/DCIM`；录音：`/sdcard/Recordings/`。

Copyright © 2024-2026 Ayang / SHENZHEN VIEWE TECHNOLOGY CO.,LTD · Apache-2.0
