# 02_battery

[中文](README_CN.md) | [English](README.md)

读取电池电压、电量和充电状态。运行时自动区分：

- **V1**：GPIO1 ADC 查表估电量
- **V2**：AXP2101 E-Gauge（I2C `0x34`）

屏幕约每秒刷新一次；插拔 USB 时「charging / USB」与「on battery」会切换。

本例不含软关机：插着 USB 调试时 GPIO47 模拟长按电源键无法掉电。

更完整的 VS Code 说明见 [../README_CN.md](../README_CN.md)。

## 准备工作

- USB 连接开发板即可（建议电池也装上，便于看电量）
- 不需要 SD 卡

## 用 VS Code / Cursor 编译烧录（推荐）

1. 安装 ESP-IDF v5.5.x 和扩展 **Espressif IDF**，按向导选好 IDF 路径并安装工具链。
2. **文件 → 打开文件夹…**，选择本目录 `examples/02_battery`（须能看到本目录的 `CMakeLists.txt`）。不要打开上一级 `examples/`。
3. `Ctrl+Shift+P` → `ESP-IDF: Set Espressif device target` → 选 **esp32s3**。
4. 点状态栏串口，选当前 USB 口。命令：`ESP-IDF: Select port to use`。
5. 点状态栏 **Build**（锤子）。首次需联网下载组件。
6. 若 Monitor 已打开，先关掉。
7. 点 **Flash**，完成后再点 **Monitor**。也可执行 `ESP-IDF: Build, Flash and Monitor`。
8. 退出监视器：`Ctrl+]`。

## 命令行

```powershell
cd examples/02_battery
idf.py set-target esp32s3
idf.py build
idf.py -p COMx flash monitor
```

## 怎么判断烧录成功

- Flash 输出出现 `Hash of data verified` / `Leaving...`，没有 fatal error。
- 屏幕标题 **Battery**，能看到电压和百分比。
- 监视器出现 `bsp display init done`，以及：

```
I (...) battery: PMIC=V2 AXP2101  cell=3.7V/600mAh
```

或 `PMIC=V1 ADC`（无 AXP2101 的板）。

## 怎么验证

- 串口每秒一行：`bat: x.xxV nn% charging` 或 `on-battery`。
- 插上 USB：状态变为 `charging / USB`（绿色）。
- 拔掉 USB（仅电池供电）：变为 `on battery`（电压会随负载略降）。
- 电量 0~100，V2 电压一般在约 3.3~4.2 V。

若电压一直为 0、电量不变：确认 `bsp_battery_init()` 成功，V2 看 CHIP ID 日志是否为 `0x4A`。
