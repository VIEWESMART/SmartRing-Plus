# VIEWE SmartRing Plus

## 简介

VIEWE SmartRing Plus 是一款智能戒指形态 AI 助手设备，搭载 ESP32-S3 模组，360×360 圆形 QSPI 触摸屏，ES8311 单麦音频编解码，支持小智 2.0 语音助手。具备 ADC 电池监测、硬件关机控制（GPIO47）、低电量自动关机等功能。

本板型使用小智 2.0 原生的 `emote::EmoteDisplay` 表情系统，**支持自定义表情和聊天背景**，可通过 [xiaozhi-assets-generator](https://github.com/78/xiaozhi-assets-generator) 在线生成资源并 OTA 更新。

---

## 一、配置、编译与烧录

### 1.1 使用 release 脚本（推荐）

```bash
cd xiaozhi2.0-esp32-viewe-smartring-plus
python scripts/release.py viewe-smartring-plus
```

脚本会根据 `config.json` 自动完成目标设置、sdkconfig 配置和编译。

### 1.2 手动 menuconfig 配置

```bash
idf.py set-target esp32s3
idf.py menuconfig
```

配置项：

- `Xiaozhi Assistant` → `Board Type` → 选择 `VIEWE SmartRing Plus`
- `Xiaozhi Assistant` → `Select display style` → 选择 `Emote animation style`（推荐）
- 若使用表情风格，需配置 Flash 资源：
  - `Xiaozhi Assistant` → `Flash Assets` → `Flash Custom Assets`
  - `Custom Assets File` 填入资源地址（可使用 EchoEar 的 360×360 兼容资源）：
    ```
    https://dl.espressif.com/AE/wn9_nihaoxiaozhi_tts-font_puhui_common_20_4-echoear.bin
    ```

### 1.3 编译与烧录

```bash
idf.py build
idf.py flash
```

> **说明**：设备使用 16MB Flash，`config.json` 已配置 `partitions/v2/16m.csv` 分区表。

---

## 二、自定义表情与背景

小智 2.0 支持通过 [xiaozhi-assets-generator](https://github.com/78/xiaozhi-assets-generator) 自定义唤醒词、字体、表情和聊天背景，并在设备上 OTA 更新。

### 2.1 生成自定义 assets.bin

1. 打开 [xiaozhi-assets-generator](https://github.com/78/xiaozhi-assets-generator) 项目（可本地运行或使用在线版本）
2. **Step 1**：选择配置
   - 芯片型号：ESP32-S3
   - 屏幕分辨率：**360×360**（与 VIEWE SmartRing Plus 匹配）
   - 颜色：RGB565
3. **Step 2**：主题设计
   - **Tab 1 - 唤醒词**：预设（如「你好小智」）或自定义
   - **Tab 2 - 字体**：预设或自定义 TTF/WOFF
   - **Tab 3 - 表情集合**：21 张表情图（neutral、happy、sad 等），可选 Twemoji 预设或自定义 PNG/GIF
   - **Tab 4 - 聊天背景**：浅色/深色模式的背景色或背景图
4. **Step 3**：点击生成，在浏览器中下载 `assets.bin`

### 2.2 更新方式

#### 方式 A：编译时指定（首次烧录）

在 menuconfig 中设置 `Custom Assets File` 为你的 `assets.bin` 下载地址，编译/烧录后首次启动会下载该资源。

#### 方式 B：运行时 OTA 更新（推荐）

1. 将生成的 `assets.bin` 上传到可公网访问的 URL（如对象存储、自建服务器）
2. 通过小智 MCP 工具 `self.assets.set_download_url` 传入该 URL
3. 设备下次激活时会自动下载并写入 assets 分区，完成表情和背景更新

流程图：

```
用户 → xiaozhi-assets-generator 生成 assets.bin
     → 上传到可访问 URL
     → MCP 调用 self.assets.set_download_url(url)
     → 设备下次激活时自动下载并应用
```

---

## 三、硬件说明

| 项目     | 说明                            |
|----------|---------------------------------|
| 电源管理 | AXP2101（I2C 0x34），ALDO3=3.3V，充电 300mA |
| 关机     | AXP2101 PowerOff + GPIO47 拉高 3 秒以上 |
| 电池监测 | AXP2101 电量；≤15% 提示，≤5% 连续 3 次确认后 PowerOff+GPIO47 |
| 触摸     | CST816S，I2C 地址 0x15          |
| 音频     | ES8311，单麦输入                |
| 显示     | ST77916 QSPI，360×360 圆形屏    |

### 显示异常时切换屏幕初始化

若屏幕显示异常，可在 `viewe-smartring-plus.cc` 中修改宏：

- `SMARTRING_PLUS_USE_NEW_LCD 1`：新屏幕（默认）
- `SMARTRING_PLUS_USE_NEW_LCD 0`：老屏幕

---

## 四、与 viewe 原项目的区别

本板型基于小智 2.0 (xiaozhi2.0-esp32-viewe-smartring-plus) 移植，与 viewe 原项目的主要区别：

| 项目         | 小智 2.0 移植版           | viewe 原项目              |
|--------------|---------------------------|---------------------------|
| 表情系统     | emote::EmoteDisplay       | anim::EmoteDisplay        |
| 自定义支持   | 支持（表情、背景可自定义）| 固定 mmap 资源，不可自定义 |
| WiFi 配网    | EnterWifiConfigMode()     | ResetWifiConfiguration()  |
| 电池关机     | PowerManager + 硬件关机   | 同左                      |

---

## 五、参考链接

- [小智 AI 主项目](https://github.com/78/xiaozhi)
- [自定义 Assets 生成器](https://github.com/78/xiaozhi-assets-generator)
- [小智自定义开发板指南](../../../docs/custom-board.md)
