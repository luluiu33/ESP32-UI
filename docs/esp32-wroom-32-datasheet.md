# ESP32-WROOM-32 模组核心技术摘要

> 基于乐鑫官方《ESP-WROOM-32 技术规格书》及《管脚清单》整理，适用于 **ESP32-WROOM-32** 及 **ESP32-WROOM-32E** 系列模组。

---

## 一、产品概述

ESP32-WROOM-32 是一款集成 **Wi-Fi 802.11 b/g/n**、**经典蓝牙** 和 **低功耗蓝牙** 的通用 MCU 模组。核心为 **ESP32-D0WDQ6** 芯片（双核 Xtensa 32-bit LX6），主频最高 **240 MHz**，适用于物联网、音频处理、传感器网络等场景。

### 关键特性

| 特性 | 说明 |
|------|------|
| CPU | 双核 Xtensa LX6，可独立控制或关闭 |
| 主频 | 80 MHz ~ 240 MHz 可调 |
| 内存 | 520 KB SRAM + 448 KB ROM |
| 外部存储 | 支持 QSPI Flash / SRAM（最大 16 MB） |
| 硬件加密 | AES、SHA、RSA、ECC |
| 低功耗 | 深度睡眠（Hibernation）电流 < 5 µA |

---

## 二、管脚定义与布局

### 2.1 模组尺寸与封装

| 参数 | 值 |
|------|-----|
| 尺寸 | 18.0 × 25.5 × 2.8 mm |
| 管脚数量 | 38 |
| 管脚间距 | 1.27 mm |
| PCB 厚度 | 0.8 ± 0.1 mm |

### 2.2 管脚功能速查表

| 序号 | 名称 | 主要功能（复用） | 注意事项 |
|------|------|------------------|----------|
| 1 | GND | 接地 | |
| 2 | 3V3 | 电源输入（3.0 ~ 3.6 V） | |
| 3 | EN | 芯片使能（高电平有效） | |
| 4 | SENSOR_VP | GPIO36, ADC1_CH0, 模拟前置放大器输入 | 仅输入，无上拉/下拉 |
| 5 | SENSOR_VN | GPIO39, ADC1_CH3, 模拟前置放大器输入 | 仅输入，无上拉/下拉 |
| 6 | IO34 | GPIO34, ADC1_CH6 | 仅输入 |
| 7 | IO35 | GPIO35, ADC1_CH7 | 仅输入 |
| 8 | IO32 | GPIO32, ADC1_CH4, 32kHz 晶振输入 | |
| 9 | IO33 | GPIO33, ADC1_CH5, 32kHz 晶振输出 | |
| 10 | IO25 | GPIO25, DAC_1, ADC2_CH8 | |
| 11 | IO26 | GPIO26, DAC_2, ADC2_CH9 | |
| 12 | IO27 | GPIO27, ADC2_CH7, Touch7 | |
| 13 | IO14 | GPIO14, ADC2_CH6, Touch6, HSPI_CLK | |
| 14 | IO12 | GPIO12, ADC2_CH5, Touch5, HSPI_Q | **Strapping 管脚 (Flash 电压)** |
| 15 | GND | 接地 | |
| 16 | IO13 | GPIO13, ADC2_CH4, Touch4, HSPI_D | |
| 17 | IO9 | GPIO9, SD_DATA2, SPIHD | 用于 Flash 通信 |
| 18 | IO10 | GPIO10, SD_DATA3, SPIWP | 用于 Flash 通信 |
| 19 | IO11 | GPIO11, SD_CMD, SPICS0 | 用于 Flash 通信 |
| 20 | IO6 | GPIO6, SD_CLK, SPICLK | 用于 Flash 通信 |
| 21 | IO7 | GPIO7, SD_DATA0, SPIQ | 用于 Flash 通信 |
| 22 | IO8 | GPIO8, SD_DATA1, SPID | 用于 Flash 通信 |
| 23 | IO15 | GPIO15, ADC2_CH3, Touch3, HSPI_CS0 | **Strapping 管脚** |
| 24 | IO2 | GPIO2, ADC2_CH2, Touch2, HSPI_WP | **Strapping 管脚** |
| 25 | IO0 | GPIO0, ADC2_CH1, Touch1 | **Strapping 管脚** |
| 26 | IO4 | GPIO4, ADC2_CH0, Touch0 | |
| 27 | IO16 | GPIO16, HS1_DATA4, U2RXD | |
| 28 | IO17 | GPIO17, HS1_DATA5, U2TXD | |
| 29 | IO5 | GPIO5, VSPI_CS0, HS1_DATA6 | **Strapping 管脚** |
| 30 | IO18 | GPIO18, VSPI_CLK, HS1_DATA7 | |
| 31 | IO19 | GPIO19, VSPI_Q, U0CTS | |
| 32 | NC | 未连接 | |
| 33 | IO21 | GPIO21, VSPI_HD, EMAC_TX_EN | |
| 34 | RXD0 | GPIO3, U0RXD | |
| 35 | TXD0 | GPIO1, U0TXD | 启动时输出日志 |
| 36 | IO22 | GPIO22, VSPI_WP, U0RTS | |
| 37 | IO23 | GPIO23, VSPI_D, HS1_STROBE | |
| 38 | GND | 接地 | |

---

## 三、重要功能说明

### 3.1 Strapping 管脚（启动配置）

以下管脚在芯片上电复位时决定启动模式，请特别注意：

| 管脚 | 默认状态 | 功能描述 |
|------|----------|----------|
| GPIO0 | 上拉 (1) | 0 → 下载模式；1 → SPI Flash 启动模式 |
| GPIO2 | 下拉 (0) | 下载模式时需保持为 0 |
| GPIO12 | 下拉 (0) | 0 → 内置 LDO 输出 3.3V（给 Flash）；1 → 输出 1.8V |
| GPIO15 | 上拉 (1) | 0 → 启动时 U0TXD 输出日志；1 → U0TXD 静止 |
| GPIO5 | 上拉 (1) | 用于 SDIO 从机时序配置（通常可忽略） |

> **⚠️ 特别提醒**：若 GPIO12 在上电时被拉高，Flash 将工作于 1.8V，可能导致读写失败（如 `Detected flash size: Unknown`）。如需使用 3.3V Flash，务必确保 GPIO12 在复位时为低电平。

### 3.2 外部 Flash 与 SRAM

| 项目 | 说明 |
|------|------|
| 容量 | 最多 4 个 16 MB 外部 QSPI Flash / SRAM |
| 代码空间 | 最大 16 MB（映射到 CPU 指令空间） |
| 数据空间 | 最大 8 MB（映射到 CPU 数据空间，Flash 只读，SRAM 可读写） |
| 安全 | 支持 AES 硬件加密，保护固件安全 |

### 3.3 功耗模式（典型值）

| 模式 | 功耗 | 说明 |
|------|------|------|
| Active (Wi-Fi Tx) | 160 ~ 260 mA | 发射功率 13 ~ 21 dBm |
| Active (Wi-Fi Rx) | 80 ~ 90 mA | 接收或侦听状态 |
| Modem-sleep | 3 ~ 20 mA | CPU 运行，射频关闭 |
| Light-sleep | 0.8 mA | CPU 暂停，RTC 工作 |
| Deep-sleep (ULP) | 0.15 mA | ULP 协处理器运行，可监测传感器 |
| Deep-sleep (RTC) | 20 µA | 仅 RTC 定时器 + RTC 存储器 |
| Hibernation | 5 µA | 仅 RTC 定时器唤醒 |

### 3.4 主要外设接口

| 接口 | 通道/数量 | 说明 |
|------|-----------|------|
| ADC | 2 × 12-bit | 18 个输入通道（部分管脚仅输入） |
| DAC | 2 × 8-bit | GPIO25, GPIO26 |
| 触摸传感器 | 10 个 | 电容式触摸，支持唤醒 |
| UART | 3 个 | 支持硬件流控和 DMA |
| SPI | 4 个 | 通用 SPI + 3 组专用（HSPI/VSPI） |
| I2C | 2 个 | 主/从模式 |
| I2S | 2 个 | 音频立体声输入输出，支持 LCD 并行数据 |
| SDIO | 1 个从机 | 符合 V2.0 标准 |
| SD/MMC 主机 | 2 个 | 支持 SD 卡 V3.01 标准 |
| 电机 PWM | 3 路 | 16-bit 定时器，带故障检测和捕获 |
| LED PWM | 16 路 | 16-bit 精度，可运行于 80 MHz |
| 红外遥控 | 8 路 | 支持多种波形标准 |
| 以太网 MAC | 1 个 | 支持 MII / RMII 接口 |
| JTAG | 1 个 | 用于调试 |

> 绝大多数外设信号（如 UART、I2C、PWM 等）均可通过 **GPIO 矩阵** 映射到任意 GPIO 管脚。

---

## 四、电气特性

### 4.1 建议工作条件

| 参数 | 最小值 | 典型值 | 最大值 |
|------|--------|--------|--------|
| 工作温度 | -40 ℃ | — | 85 ℃ |
| 供电电压 (VDD) | 2.2 V | 3.3 V | 3.6 V |

### 4.2 数字端口特性

| 参数 | 最小值 | 典型值 | 最大值 |
|------|--------|--------|--------|
| 输入低电平 (VIL) | -0.3 V | — | 0.25 × VDD |
| 输入高电平 (VIH) | 0.75 × VDD | — | VDD + 0.3 V |
| 输出低电平 (VOL) | — | — | 0.1 × VDD |
| 输出高电平 (VOH) | 0.8 × VDD | — | — |

### 4.3 Wi-Fi 射频（典型值）

| 项目 | 典型值 |
|------|--------|
| PA 输出功率 | 16.5 dBm |
| 灵敏度 (1 Mbps) | -98 dBm |
| 灵敏度 (11 Mbps) | -90 dBm |
| 灵敏度 (54 Mbps) | -75 dBm |

---

## 五、硬件设计注意事项

### 5.1 晶振配置

- 主晶振支持 **40 MHz**、**26 MHz**、**24 MHz**
- 外接低速晶振（32 kHz）用于低功耗精确唤醒，连接至 **GPIO32** 和 **GPIO33**
- 如需使用 GPIO32/GPIO33 的 ADC/Touch 功能，需移除 32 kHz 晶振并焊接 0Ω 电阻

### 5.2 Flash 电压选择

- GPIO12 在上电复位时决定内置 LDO（VDD_SDIO）输出为 **3.3V** 或 **1.8V**
- 绝大多数 ESP32-WROOM-32/32E 模组使用 3.3V Flash，必须确保 **GPIO12 在复位时拉低**

### 5.3 仅输入管脚（无上拉/下拉）

以下管脚 **只能作为输入**，且无内部上下拉电阻：

- GPIO36 (SENSOR_VP)
- GPIO39 (SENSOR_VN)
- GPIO34
- GPIO35

### 5.4 Flash 通信占用管脚

GPIO **6、7、8、9、10、11** 用于内部 Flash 通信，**不建议** 作为普通 GPIO 使用。

---

## 六、开发与调试

### 6.1 启动模式选择

| 模式 | GPIO0 | GPIO2 |
|------|-------|-------|
| SPI Flash 启动 | 1 | 任意 |
| 下载模式 (UART) | 0 | 0 |

> 大多数开发板通过 **BOOT 按钮** 和 **EN 按钮** 组合操作即可进入下载模式。

### 6.2 日志输出

- 默认启动时，U0TXD (GPIO1) 会输出固件日志
- 可通过 Strapping 管脚 GPIO15 控制是否翻转/静默

---

## 七、参考文档

- [乐鑫官网](https://www.espressif.com)

---

> 本文档为摘要总结，详细内容请参阅官方最新版本规格书。如有冲突，以官方文档为准。
