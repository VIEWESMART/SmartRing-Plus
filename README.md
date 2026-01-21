# SmartRing-Plus-SmartDisplay
[中文](/README_CN.md)

<div align="center">

<a href=" " target="_blank">
  <img src="https://img.shields.io/badge/🛒_Official_Store-4051B5?style=for-the-badge" alt="Official Store"/>
</a>
&nbsp;&nbsp;&nbsp;&nbsp;
<a href="mailto:smartrd1@viewedisplay.com">
  <img src="https://img.shields.io/badge/📧_Technical_Support-555555?style=for-the-badge" alt="Technical Support"/>
</a>

</div>

<img width="1158" height="694" alt="SmartRing Plus Product Image 1" src="images/emoji.png" />

## 1. Introduction
The SmartRing-Plus-SmartDisplay is a compact high-performance smart display module designed by VIEWE, featuring a 1.85-inch circular IPS touch screen. Built around the ESP32-S3-N16R8 dual-core MCU, it integrates Wi-Fi and Bluetooth BLE5.0 wireless connectivity, supporting a wealth of practical functions and flexible secondary development. With its exquisite CNC-machined casing, rich peripheral interfaces, and low-power design, it is suitable for a variety of embedded application scenarios requiring human-machine interaction, multimedia playback, and IoT connectivity.

### 1.1 Product Features
- Processor
  - Main control chip: ESP32-S3-N16R8 (dual-core MCU)
  - Main frequency: 240MHz
  - Wireless connectivity: 2.4GHz Wi-Fi, Bluetooth BLE5.0, BLE Mesh
  - Operating system: RTOS

- Memory
  - SRAM: 520KB
  - PSRAM: 8MB
  - ROM: 448KB
  - Flash: 16MB

- Display & Touch
  - Screen size: 1.85-inch (circular)
  - Resolution: 360×360 pixels
  - Display mode: IPS, transmissive, normally black
  - Touch type: Capacitive touch (TP IC: CST816S)
  - Brightness: 400 cd/m²
  - Color gamut: 70% (262K colors)
  - Contrast ratio: 1200:1
  - PPI: 200

- Core Functions
  - Computer secondary screen (AIDA64): 5 built-in styles
  - Audio spectrum pickup
  - High-quality MP3 playback (supports 320K decoding)
  - Digital photo frame (supports user-added photos)
  - MJPEG video playback
  - Balance ball game (gyroscope control)
  - Themed clock (adjustable time zone: international/China)
  - Wireless power supply (supports QI protocol)
  - Real-time weather (via Wi-Fi internet connection)
  - Continuous function upgrades

- Peripheral Interfaces & Modules
  - UART/USB interfaces
  - TF card slot (SDMMC)
  - I2S digital microphone (ZTS6216)
  - I2S digital-to-analog conversion (ES8311)
  - Power amplifier (NS4150B)
  - IMU sensor (QMI8658A)
  - Rechargeable battery (3.7V, 600mAh)
  - Speaker interface (8Ω / 1W)
  - QSPI interface (for display driver)

- Development Support
  - Supported IDEs: Arduino IDE, ESP IDE, MicroPython, PlatformIO
  - UI development: LVGL
  - Secondary development: Full support with provided source code
  - Xiaozhi integration: Source code available

- Hardware Specifications
  - Outline dimension: φ57.6mm (circular)
  - Thickness: 12.2mm
  - Shell color: Metallic black/Silver/Customizable
  - Casing process: CNC-machined
  - Power supply: DC 5V, 1A (supports wireless charging via QI protocol)

### 1.2 Applications
With its compact size, rich functions, and wireless connectivity, the SmartRing-Plus is ideal for IoT devices and embedded systems in the following fields:

• Desktop smart assistant (computer secondary screen)
• Personal multimedia player (music/photo/video)
• Smart wearable accessory
• Home smart control panel
• IoT sensor data display terminal
• Portable entertainment device (built-in game)
• Customized themed clock/weather station
• Embedded HMI (Human-Machine Interface)
• Educational electronics
• Creative DIY projects

## 2. Hardware Description
### 2.1 Module Introduction

① ESP32-S3-N16R8 Main Control Chip：
Dual-core MCU with integrated Wi-Fi/BLE5.0, 8MB PSRAM, 16MB Flash

② 1.85-inch Circular Display：
IPS screen (360×360), capacitive touch (CST816S)

③ TF Card Slot：
Supports SDMMC protocol for storage expansion

④ I2S Audio Module：
Includes ES8311 DAC, NS4150B power amplifier, ZTS6216 microphone

⑤ IMU Sensor (QMI8658A)：
Gyroscope for motion detection (balance ball game)

⑥ Rechargeable Battery：
3.7V, 600mAh (supports wireless charging via QI)

⑦ USB Interface：
For power supply, program burning, and debugging

⑧ Reset Button：
Hardware reset function

⑨ BOOT Button：
Press when powering on/resetting to enter download mode

⑩ Wireless Charging Receiver：
Supports QI protocol for wireless power supply

### 2.2 GPIO Definition

| ESP Pin NO. | FUNCTION |
|-------------|----------|
| GPIO0       | BOOT/Power |
| GPIO1       | BAT_ADC |
| GPIO2       | SDMMC_D1 |
| GPIO3       | SDMMC_D0 |
| GPIO4       | SDMMC_SCK |
| GPIO5       | SDMMC_CMD |
| GPIO6       | SDMMC_D3 |
| GPIO7       | SDMMC_D2 |
| GPIO8       | TP_SDA |
| GPIO9       | TP_SCL |
| GPIO10      | LCD_QSPI_SCL |
| GPIO11      | LCD_QSPI_CS |
| GPIO12      | LCD_QSPI_D0 |
| GPIO13      | LCD_QSPI_D1 |
| GPIO14      | LCD_QSPI_D3 |
| GPIO15      | LCD_QSPI_D2 |
| GPIO16      | I2S_DO |
| GPIO17      | I2S_WS |
| GPIO18      | I2S_DI |
| GPIO19      | USB_N |
| GPIO20      | USB_P |
| GPIO21      | I2S_BCK |
| GPIO38      | LCD_TE |
| GPIO39      | LCD_RST |
| GPIO40      | TP_SCL |
| GPIO41      | TP_INT |
| GPIO42      | IMU_INT1 |
| GPIO43      | UART0_RX |
| GPIO44      | UART0_TX |
| GPIO45      | - |
| GPIO46      | TP_BLK |
| GPIO47      | PW_OFF |
| GPIO48      | I2S_MCLK |

### 2.3 Version Difference
The SmartRing-Plus series includes two versions with consistent hardware specifications except for screen models and initialization parameters:

| Product Version | Matching Screen Model | Core Difference |
|-----------------|-----------------------|-----------------|
| SmartRing-Plus-A | VIEWE UE018HV-RB39-A002A | Screen model and corresponding initialization parameters |
| SmartRing-Plus-B | VIEWE UE018HV-RB39-A004A | Screen model and corresponding initialization parameters |

**Version Identification Methods**:
1. Product packaging: Clearly marked with version model (A/B)
2. Firmware suffix: _A for version A, _B for version B
3. Sample code: Version-specific codes provided for secondary development


## 3. Instructions for Use
This tutorial guides users to set up the software development environment for SmartRing-Plus and demonstrates basic operations such as project configuration, compilation, and firmware burning through simple examples.

### Preparation
- Hardware
  - SmartRing-Plus Development Board (Version A/B)
  - USB data cable (for power supply, burning, debugging)
  - Wireless power bank (optional, QI-compliant)
  - Computer (Windows, Linux, or macOS)

- Software (Choose any development environment below)
  - VSCode + ESP-IDF plugin (recommended)
  - Arduino IDE
  - ESP-IDF
  - PlatformIO

## 4. Getting Started
### ESP-IDF
- Please refer to [ESP-IDF Quick Start](https://github.com/VIEWESMART/VIEWE-Tutorial/blob/main/esp-idf/esp-idf_Beginner_Tutorial.md) to set up the development environment and burn firmware.
- Application examples are stored in the [examples/esp-idf](https://github.com/VIEWESMART/SmartRing-Plus/tree/main/examples/esp-idf) directory. Run `idf.py menuconfig` to configure project options. Examples are being continuously updated; contact us for priority support if needed.

### Arduino IDE
- Install the ESP32 board support package in Arduino IDE.
- Download the SmartRing-Plus Arduino library and sample code from the GitHub repository.
- Follow the library documentation to quickly implement functions such as screen display and sensor reading.

### MicroPython / PlatformIO
- MicroPython firmware and sample scripts are available in the [examples/micropython](https://github.com/VIEWESMART/SmartRing-Plus/tree/main/examples/micropython) directory.
- PlatformIO project templates are provided in [examples/platformio](https://github.com/VIEWESMART/SmartRing-Plus/tree/main/examples/platformio), supporting one-click compilation and burning.

## 6. Related Documents
- [SmartRing-Plus Specification (PDF)](datasheet/SmartRing-Plus-SPEC-V1.1.pdf)
- [ESP32-S3-N16R8 Datasheet (English)](datasheet/chip/datasheet_en.pdf)
- [ESP32-S3-N16R8 Datasheet (Chinese)](datasheet/chip/datasheet_cn.pdf)
- [Display Specification (SmartRing-Plus-A)](datasheet/display/SmartRing-Plus-A%20Display%20Specification.pdf)
- [Display Specification (SmartRing-Plus-B)](datasheet/display/SmartRing-Plus-B%20Display%20Specification.pdf)
- [Audio Chip (ES8311) Datasheet](datasheet/peripheral/ES8311_datasheet.pdf)
- [Power Amplifier (NS4150B) Datasheet](datasheet/peripheral/NS4150B_datasheet.pdf)
- [Microphone (ZTS6216) Datasheet](datasheet/peripheral/ZTS6216_datasheet.pdf)
- [IMU Sensor (QMI8658A) Datasheet](datasheet/peripheral/QMI8658A_datasheet.pdf)
- [All Datasheets](/datasheet)

## 7. Dimension Drawing
![SmartRing-Plus Dimension Drawing](image/SmartRing-Plus-Size.png)
- Outline dimension: φ57.6mm (circular)
- Thickness: 12.2mm
- Display active area: 45.68(H)×45.68(V) mm
- Touch panel outline: 55(H)×55(V)×0.7(T) mm

## 8. Technical Support
- GitHub Repository: [https://github.com/VIEWESMART/SmartRing-Plus](https://github.com/VIEWESMART/SmartRing-Plus)
- Email: smartrd1@viewedisplay.com
- QQ Technical Exchange Group: 1014311090
- WhatsApp Business Account: [Contact via QR Code](image/Whatsapp.png)
- Documentation: [TAIJI VIEWE Pi](), [VIEWE Xiaozhi]()
