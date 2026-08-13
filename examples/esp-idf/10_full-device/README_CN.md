# smartring_plus_full_device

[中文](README_CN.md) | [English](README.md)

**SmartRing-Plus**（ESP32-S3-N16R8）**整机功能演示**：时钟、天气、录音、音乐、IMU 水平仪、相册、设置。

本目录是**独立 ESP-IDF 工程**。在 GitHub 上它与其它入门示例同级，都在 `examples/` 下。通用 VS Code 步骤见 [../README_CN.md](../README_CN.md)。

| 项 | 内容 |
|----|------|
| 工程名（CMake） | `smartring_plus_full_device` |
| 作者 | **Ayang** |
| 公司 | **SHENZHEN VIEWE TECHNOLOGY CO.,LTD**（深圳市优奕视界有限公司） |
| 许可证 | Apache-2.0 |
| IDF | v5.5.x（建议 v5.5.4） |
| 目标芯片 | `esp32s3` |
| BSP | [`viewesmart/smartring_plus`](https://components.espressif.com/components/viewesmart/smartring_plus) `^1.1.1` |

---

## 准备工作

- SmartRing-Plus 用 USB 接到电脑
- 音乐 / 相册 / 录音需要 FAT32 MicroSD（可选）：

| 卡上路径 | 用途 |
|----------|------|
| `/Music/` | `.mp3`（可再套一层子目录） |
| `/Photos/` 或 `/DCIM/` | `.jpg` / `.jpeg` / `.png` |
| `/Recordings/` | 录音输出（可自动创建） |

插卡后再上电。本板没有卡检测脚。

---

## 用 VS Code / Cursor 编译烧录（推荐）

1. 安装 [ESP-IDF v5.5.x](https://docs.espressif.com/projects/esp-idf/zh_CN/latest/esp32s3/get-started/index.html) 和扩展 **Espressif IDF**，按向导选好 IDF 路径并安装工具链。
2. **文件 → 打开文件夹…**，选择**本目录** `examples/full-device`（须能看到本目录的 `CMakeLists.txt`）。**不要**打开上一级 `examples/`。
3. `Ctrl+Shift+P` → `ESP-IDF: Set Espressif device target` → 选 **esp32s3**。
4. 点状态栏串口，选当前 USB 口（Windows 常见 `COMx`）。命令：`ESP-IDF: Select port to use`。
5. 点状态栏 **Build**（锤子），或 `ESP-IDF: Build your project`。首次需联网下载组件；本工程比 `01`～`09` 更大、编译更久。
6. 若 Monitor 已打开，先关掉（端口占用会导致烧录失败）。
7. 点 **Flash**，完成后再点 **Monitor**。也可执行 `ESP-IDF: Build, Flash and Monitor`。
8. 退出监视器：`Ctrl+]`。

更完整的状态栏说明见 [../README_CN.md](../README_CN.md)。

## 命令行

```powershell
. D:\idf\.espressif\v5.5.4\esp-idf\export.ps1   # 路径按本机修改
cd examples/full-device
idf.py set-target esp32s3
idf.py build
idf.py -p COMx flash monitor
```

干净重建：`idf.py fullclean` 后再 `idf.py build`。

---

## 怎么判断烧录成功

- Flash 输出出现 `Hash of data verified` / `Leaving...`，没有 fatal error。
- 复位后圆屏亮起，是**圆形主页**（彩色图标围着小表盘），不是某个单示例标题（如 `I2C Scan`）。
- 监视器（TAG `smartring_plus_full_device`）：

```
I (...) smartring_plus_full_device: smartring_plus_full_device starting
I (...) bsp: bsp display init done ...
I (...) smartring_plus_full_device: PMIC path: V2_AXP2101
I (...) smartring_plus_full_device: 初始化完成
```

`PMIC path` 为 `V2_AXP2101` 或 `V1_ADC`。  
未插 FAT32 卡时可能出现 `SD 卡未挂载` 警告，其它界面仍可使用。

---

## 怎么验证

主页：点圆形图标进入功能页，用返回控件回到主页。

| 功能 | 预期 |
|------|------|
| **Clock** | 实时 `HH:MM:SS`。WiFi + SNTP 成功前日期可能停在 1970。 |
| **Weather** | 先在 **Settings** 连 WiFi，再打开天气，应有城市 / 天气 / 温度。日志 TAG `weather_service`。 |
| **Recorder** | 需要 SD。录音后扬声器回放。日志：`recorded: /sdcard/Recordings/...` |
| **Music** | 需要 `/Music/*.mp3`。播放 / 暂停 / 切歌。日志：`play: ...` |
| **IMU** | 倾斜时气泡移动。放平后点 **Calibrate**。 |
| **Album** | 图片放在 `/Photos` 或 `/DCIM`。轮播；上滑退出。 |
| **Settings** | 电池 mV / 电量 / 是否充电。**Scan WiFi** → 点热点 → 输入密码 → 显示 IP。**Soft Power Off** 仅电池供电时有效（插着 USB 时 GPIO47 无法掉电）。 |

WiFi 连上后日志：`connected, IP: x.x.x.x SSID: ...`，随后 `SNTP ok`。

---

## 目录结构

```
full-device/
├── CMakeLists.txt              # project(smartring_plus_full_device)
├── sdkconfig.defaults
├── partitions.csv
├── AUTHORS / LICENSE / README.md / README_CN.md
├── main/                       # 应用入口
├── components/
│   ├── ui_shell / ui_theme     # 圆形主页与主题
│   ├── feature_*               # 时钟 / 天气 / 录音 / 音乐 / IMU / 相册 / 设置
│   ├── wifi_service
│   └── weather_service
├── managed_components/         # Component Manager（BSP、LVGL、codec 等，编译时自动拉取）
└── .vscode/settings.json
```

`managed_components/` 在首次 `idf.py build` 时自动生成。

构建产物：`build/smartring_plus_full_device.elf` / `.bin`。

---

## 硬件摘要

- MCU：ESP32-S3-N16R8（16MB Flash + 8MB Octal PSRAM）
- 屏：ST77916 QSPI **360×360** 圆屏 + CST816 触摸
- 音频：ES8311，48 kHz / 16bit / 立体声
- 存储：SDMMC 4-bit，挂载点 **`/sdcard`**
- 电池：**V1** GPIO1 ADC；**V2** AXP2101（I2C `0x34`，运行时探测）；软关机 GPIO47（两版相同）

引脚定义见组件库 `viewesmart/smartring_plus` 中的 `bsp/board_config.h`。

---

## 应用组件

BSP 由组件管理器拉取，头文件：

```c
#include "bsp/smartring_plus.h"
```

| API | 作用 |
|-----|------|
| `bsp_display_init()` | 显示 + 触摸 + LVGL 适配器 + 背光 |
| `bsp_battery_init()` / `bsp_battery_get_pmic_type()` | 电池（V1 ADC / V2 AXP2101） |
| `bsp_power_init()` / `bsp_power_shutdown()` | 电源（GPIO47） |
| `bsp_sd_init()` | SD 卡 |
| `bsp_audio_init()` / `bsp_audio_write()` | 音频 |
| `bsp_imu_*` | IMU |

改 LVGL 界面请配合 `esp_lv_adapter_lock(-1)` / `unlock()`。

| 组件 | 说明 |
|------|------|
| `ui_theme` | 颜色、字体、圆屏安全区宏 |
| `ui_shell` | 圆形主页、导航、低电提示 |
| `feature_*` | 各功能页 |
| `wifi_service` | WiFi STA 扫描/连接 |
| `weather_service` | 天气查询（HTTP） |

可按需删除某个 `feature_*`，并同步修改 `main/CMakeLists.txt` 与 `main/main.c` 注册表。

---

## 常见问题

**串口被占用 / 烧录失败**（`Could not open COMx, the port is busy`）：先关掉 Monitor / 其它串口工具；USB 重插后 COM 号可能变化。

**开机花屏 / `ESP_ERR_NO_MEM`（SPI DMA）**：日志出现 `setup_dma_priv_buffer ... Failed to allocate`。保持 BSP 里较小的 LVGL flush 条带高度（如 40），不要改成整屏高度。

**触摸点了没反应**：确认已 `bsp_display_init()` 且日志含 CST816；子对象勿抢占 `CLICKABLE`。

**录音失败 / 文件名异常**：`sdkconfig.defaults` 已开 `CONFIG_FATFS_LFN_HEAP=y`。确认 SD 已挂载。

**音乐卡住**：确认 `/sdcard/Music` 下为合法 MP3。

**天气失败**：先在设置页连接 WiFi。本工程使用 HTTP；若改 HTTPS 需处理证书。

**Component Manager 下载失败**：检查网络与代理；删除 `managed_components` 与 `dependencies.lock` 后重新 `idf.py build`。

**`export.ps1` 失败（Windows）**：用「ESP-IDF PowerShell」快捷方式，或扩展状态栏自带终端。

**圆屏文字被裁切**：控件需收进安全区（参考 `ui_theme.h`）。

**改完代码后行为仍是旧的**：确认烧录的是本工程 `build/*.bin`；尝试 `idf.py fullclean && idf.py build flash`。

---

Copyright © 2024-2026 **Ayang** / **SHENZHEN VIEWE TECHNOLOGY CO.,LTD**  
Licensed under the Apache License, Version 2.0 — 见 `LICENSE`、`AUTHORS`。
