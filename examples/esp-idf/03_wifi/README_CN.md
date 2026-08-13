# 03_wifi

[中文](README_CN.md) | [English](README.md)

WiFi STA 连接示例：连上指定热点后在屏幕显示 IP。

SSID / 密码在 **menuconfig** 里改，不要把密码写进源码。

更完整的 VS Code 说明见 [../README_CN.md](../README_CN.md)。

## 准备工作

- 开发板 USB 连接电脑
- 2.4 GHz WiFi（ESP32-S3 不支持 5 GHz）
- 默认占位：`myssid` / `mypassword`，**烧录前必须改成你的热点**

## 用 VS Code / Cursor 编译烧录（推荐）

1. 安装 ESP-IDF v5.5.x 和扩展 **Espressif IDF**，按向导选好 IDF 路径并安装工具链。
2. **文件 → 打开文件夹…**，选择本目录 `examples/03_wifi`（须能看到本目录的 `CMakeLists.txt`）。不要打开上一级 `examples/`。
3. `Ctrl+Shift+P` → `ESP-IDF: Set Espressif device target` → 选 **esp32s3**。
4. 点状态栏串口，选当前 USB 口。
5. **配置热点（必须）**：`Ctrl+Shift+P` → `ESP-IDF: SDK Configuration editor (menuconfig)`  
   进入 **Example Configuration**：
   - `WiFi SSID`
   - `WiFi Password`（开放网络可留空）  
   保存并关闭编辑器。
6. 点状态栏 **Build**（锤子）。首次需联网下载组件。
7. 若 Monitor 已打开，先关掉。
8. 点 **Flash**，完成后再点 **Monitor**。也可执行 `ESP-IDF: Build, Flash and Monitor`。
9. 退出监视器：`Ctrl+]`。

改完 menuconfig 必须重新 Build 再烧录，否则仍是旧的 SSID。

## 命令行

```powershell
cd examples/03_wifi
idf.py set-target esp32s3
idf.py menuconfig
idf.py build
idf.py -p COMx flash monitor
```

`idf.py menuconfig` 中同样进入 **Example Configuration** 填写 SSID / 密码。

## 怎么判断烧录成功

- Flash 输出出现 `Hash of data verified` / `Leaving...`，没有 fatal error。
- 屏幕标题 **WiFi**，中间是你配置的 SSID，状态先为 `connecting...`。
- 监视器出现 `bsp display init done` 和 `connecting to "你的SSID"`。

## 怎么验证

成功：

```
I (...) wifi: got IP 192.168.x.x
```

屏幕变为绿色 **connected**，并显示 IP。

失败：红色 **connect failed**，串口 `connect failed (check SSID/password in menuconfig)`。

请核对：2.4 GHz、密码、距路由不要太远。
