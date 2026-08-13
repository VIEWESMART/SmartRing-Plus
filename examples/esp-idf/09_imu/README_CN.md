# 09_imu

[中文](README_CN.md) | [English](README.md)

QMI8658A 水平仪示例：气泡随倾斜移动，底部显示三轴加速度（m/s²）。点 **Calibrate** 开始水平校准（把板子放平约 2 秒）。

更完整的 VS Code 说明见 [../README_CN.md](../README_CN.md)。

## 准备工作

- USB 连接开发板
- 不需要 SD 卡
- IMU 在共享 I2C 上，须先初始化显示（本例已按此顺序）

## 用 VS Code / Cursor 编译烧录（推荐）

1. 安装 ESP-IDF v5.5.x 和扩展 **Espressif IDF**，按向导选好 IDF 路径并安装工具链。
2. **文件 → 打开文件夹…**，选择本目录 `examples/09_imu`（须能看到本目录的 `CMakeLists.txt`）。不要打开上一级 `examples/`。
3. `Ctrl+Shift+P` → `ESP-IDF: Set Espressif device target` → 选 **esp32s3**。
4. 点状态栏串口，选当前 USB 口。命令：`ESP-IDF: Select port to use`。
5. 点状态栏 **Build**（锤子）。首次需联网下载组件。
6. 若 Monitor 已打开，先关掉。
7. 点 **Flash**，完成后再点 **Monitor**。也可执行 `ESP-IDF: Build, Flash and Monitor`。
8. 退出监视器：`Ctrl+]`。

## 命令行

```powershell
cd examples/09_imu
idf.py set-target esp32s3
idf.py build
idf.py -p COMx flash monitor
```

## 怎么判断烧录成功

- Flash 输出出现 `Hash of data verified` / `Leaving...`，没有 fatal error。
- 屏幕标题 **IMU**，中间有圆形水平仪和蓝色气泡。
- 监视器：`bsp display init done`，以及 `bsp_imu init done` 或 `QMI8658A found`。

若 `IMU init failed`：检查 I2C（可先跑 `01_i2c_scan`，应能扫到 `0x6B`）。

## 怎么验证

1. 静止时三轴读数中应有一轴接近重力（约 ±9.8）。
2. 倾斜开发板，气泡向“下滑”方向移动。
3. 放平后点 **Calibrate**，保持稳定：状态 `keep flat...` → `calibrated`。晃动过大或超时会 `calib failed`，可再点一次。

串口在校准时有 `start level calibration` / `calib PASSED` 或 `FAILED`。
