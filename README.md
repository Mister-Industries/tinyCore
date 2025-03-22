# tinyCore - A Better ESP32 Starter Kit

<div align="center">

![tinyCore Logo](assets/iota-logo.png)

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Open Source Hardware](https://img.shields.io/badge/Hardware-Open%20Source-brightgreen)](https://www.oshwa.org/)

</div>

![image](https://github.com/user-attachments/assets/16b84b4e-978e-4d1d-b7d1-50d26df883d8)

![image](https://github.com/user-attachments/assets/2e7e69f3-f873-4301-b462-13db0d8916a2)


## Overview

tinyCore is an engineering learning platform based around a truly open-source microcontroller designed to help educate individuals on the world of advanced embedded systems. tinyCore’s mission is to bridge the gap between undergrad and industry, equipping young professionals with the resources they need to build their portfolio and bring their ideas to life. 

### Key Features

- **Processor**: ESP32-S3 Mini Dual-Core No-PSRAM
- **Memory**: 512KB Flash, 128KB RAM
- **Connectivity**: 
  - WiFi & Bluetooth LE
  - USB-C
  - 2x STEMMA/QWIIC I2C Connectors
  - Micro SD Card Slot
- **I/O**:
  - 6 Digital pins
  - 6 Analog inputs
  - UART/I2C/SPI Serial Buses
- **Power**: 
  - USB-C powered
  - LiPo battery support with charging circuit
  - Low power sleep modes, Dedicated I2C PWR Rail
- **Dimensions**: 50mm x 50mm

## Getting Started

### Hardware Setup

1. Connect the tinyCore to your computer using a USB-C cable
2. The onboard LED should pulse blue indicating proper power
3. Your computer should recognize the device as a serial port

## Hardware

### Schematic

![tinyCore Schematic](assets/schematic.png)

The complete hardware design files are available in the [hardware folder](https://github.com/Mister-Industries/tinyCore/tree/main/Hardware), including:
- Schematic files 
- PCB layout files (Altium/KiCad format)
- Bill of Materials (BOM)
- Manufacturing files (Gerber)
- Assembly instructions

### Pin Mapping

| Pin | Function | Notes |
|-----|----------|-------|

## Development

### Contributing

We welcome contributions! Please see our [Contributing Guide](CONTRIBUTING.md) for details on:
- Code style guidelines
- Development workflow
- Testing requirements
- Pull request process

## Community

- [Discord Server](https://discord.gg/hvJZhwfQsF)
- [Bluesky](https://bsky.app/profile/mr.industries)

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Acknowledgments
The tinyCore project thrives thanks to the generous support of our manufacturing and development partners:
### --- PCBWay ---

PCBWay provides exceptional PCB manufacturing services and has been instrumental in bringing tinyCore to life through their reliable prototyping capabilities. You can order our open-source designs directly from [their website](https://www.pcbway.com/project/shareproject/iota_The_Open_Source_Advanced_IoT_Learning_Platform_12776757.html)

Special thanks to:
- The Adafruit team for inspiration
- All our open source contributors



## Support

- [Issue Tracker](https://github.com/tinycore/tinycore/issues)
- Support Email: support@mr.industries
