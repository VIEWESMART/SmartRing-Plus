# SmartRing_Plus

SmartRing_Plus 是一个基于 ESP32-S3 的智能环形开发板项目，支持双屏幕版本，使用 LVGL 图形库进行界面开发。

## 项目简介

SmartRing_Plus 是一个功能丰富的智能设备开发平台，具有以下特性：
- 基于 ESP32-S3 微控制器
- 支持双屏幕版本（老屏幕版本 A 和新屏幕版本 B）
- 集成 LVGL 图形库
- 支持触摸屏交互
- 支持音频编解码
- 支持 IMU 传感器

## 快速开始

### 环境要求

- ESP-IDF v5.4.1 或更高版本
- Python 3.6 或更高版本
- CMake 3.5 或更高版本

### 编译和烧录

1. 配置项目：
```bash
idf.py menuconfig
```

2. 选择屏幕版本：
   - 在 `components/smartring_plus/smartring_plus.c` 文件中修改宏值：
     - 使用老屏幕版本：设置 `SMARTRING_PLUS_A = 1`，`SMARTRING_PLUS_B = 0`
     - 使用新屏幕版本（默认）：设置 `SMARTRING_PLUS_A = 0`，`SMARTRING_PLUS_B = 1`

3. 编译项目：
```bash
idf.py build
```

4. 烧录固件：
```bash
idf.py flash
```

5. 查看串口输出：
```bash
idf.py monitor
```

## 屏幕版本选择

SmartRing_Plus 支持两种屏幕版本，通过修改宏的值（1 或 0）进行选择：

### 使用方法

在 `components/smartring_plus/smartring_plus.c` 文件顶部找到以下宏定义：

```c
#define SMARTRING_PLUS_A    0  // 老屏幕版本，设置为 1 启用，0 禁用
#define SMARTRING_PLUS_B    1  // 新屏幕版本（默认），设置为 1 启用，0 禁用
```

### 老屏幕版本 (SMARTRING_PLUS_A)

修改宏值为：
```c
#define SMARTRING_PLUS_A    1  // 启用老屏幕版本
#define SMARTRING_PLUS_B    0  // 禁用新屏幕版本
```

### 新屏幕版本 (SMARTRING_PLUS_B) - 默认

修改宏值为（默认配置）：
```c
#define SMARTRING_PLUS_A    0  // 禁用老屏幕版本
#define SMARTRING_PLUS_B    1  // 启用新屏幕版本
```

**重要提示：**
- 只能将其中一个宏设置为 1，另一个必须为 0
- 如果两个宏都为 1 或都为 0，编译时会报错
- 默认使用新屏幕版本（SMARTRING_PLUS_B = 1）

## 项目结构

```
SmartRing_Plus/
├── components/              # 组件目录
│   ├── smartring_plus/ # BSP 板级支持包
│   ├── esp_codec/          # 音频编解码组件
│   ├── espressif__esp_lcd_touch_cst816s/  # 触摸屏驱动
│   └── espressif__esp_lvgl_port/          # LVGL 移植层
├── main/                   # 主程序
├── lottie_assets/          # Lottie 动画资源
├── CMakeLists.txt         # 项目构建配置
├── sdkconfig.defaults     # 默认配置文件
└── README.md              # 本文件
```

## 主要功能

### 显示功能
- 支持 ST77916 LCD 驱动芯片
- 支持 QSPI 接口
- 支持背光亮度调节
- 集成 LVGL 图形库

### 触摸功能
- 支持 CST816S 触摸控制器
- 支持中断模式
- 自动校准

### 音频功能
- 支持 ES8311 音频编解码器
- 支持 ES7210 ADC
- 支持音频播放和录音

### 传感器功能
- 支持 ICM42670 IMU 传感器
- 支持运动检测

## 配置说明

### 屏幕配置

在 `menuconfig` 中可以配置：
- LCD 帧缓冲区高度 (`CONFIG_BSP_LCD_DRAW_BUF_HEIGHT`)
- 是否启用双缓冲 (`CONFIG_BSP_LCD_DRAW_BUF_DOUBLE`)
- 背光 LEDC 通道 (`CONFIG_BSP_DISPLAY_BRIGHTNESS_LEDC_CH`)

### I2C 配置

- I2C 外设索引 (`CONFIG_BSP_I2C_NUM`)
- I2C 时钟速度 (`CONFIG_BSP_I2C_CLK_SPEED_HZ`)
- 是否启用快速模式 (`CONFIG_BSP_I2C_FAST_MODE`)

## 使用示例

### 基本显示初始化

```c
#include "bsp/smartring_plus.h"

void app_main(void)
{
    // 初始化显示
    bsp_display_cfg_t cfg = {
        .lvgl_port_cfg = ESP_LVGL_PORT_INIT_CONFIG(),
        .buffer_size = BSP_LCD_H_RES * CONFIG_BSP_LCD_DRAW_BUF_HEIGHT,
        .double_buffer = 0,
        .flags = {
            .buff_dma = true,
        }
    };
    bsp_display_start_with_config(&cfg);
    
    // 打开背光
    bsp_display_backlight_on();
    
    // 使用 LVGL 创建界面
    // ...
}
```

### 触摸输入处理

```c
lv_indev_t *indev = bsp_display_get_input_dev();
// 使用 LVGL 的输入设备处理触摸事件
```

## 故障排除

### 屏幕显示异常

如果屏幕显示不正常，请检查：
1. 是否正确选择了对应的屏幕版本宏（`SMARTRING_PLUS_A` 或 `SMARTRING_PLUS_B`）
2. 屏幕初始化命令是否正确
3. 硬件连接是否正确

### 触摸无响应

如果触摸无响应，请检查：
1. I2C 连接是否正常
2. 触摸中断引脚配置是否正确
3. 触摸控制器是否正常工作

## 许可证

本项目遵循 Apache-2.0 许可证。

## 贡献

欢迎提交 Issue 和 Pull Request。

## 更新日志

- 2024-01-XX: 初始版本，支持双屏幕版本选择
