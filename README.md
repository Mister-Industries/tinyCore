# iotaCore Microcontroller

<div align="center">

![iotaCore Logo](assets/iota-logo.png)

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Open Source Hardware](https://img.shields.io/badge/Hardware-Open%20Source-brightgreen)](https://www.oshwa.org/)
[![Documentation Status](https://readthedocs.org/projects/iotacore/badge/?version=latest)](https://iotacore.readthedocs.io)

</div>

## Overview

The iotaCore is an open-source microcontroller platform designed for IoT applications, combining powerful processing capabilities with extensive connectivity options in a compact form factor.

### Key Features

- **Processor**: ARM Cortex-M4 running at 120MHz
- **Memory**: 512KB Flash, 128KB RAM
- **Connectivity**: 
  - WiFi 802.11 b/g/n
  - Bluetooth 5.0 LE
  - USB-C
- **I/O**:
  - 24 GPIO pins
  - 8 Analog inputs
  - 4 UART interfaces
  - 2 I2C buses
  - 2 SPI buses
- **Power**: 
  - USB-C powered
  - LiPo battery support with charging circuit
  - Low power sleep modes
- **Dimensions**: 45mm x 30mm

## Getting Started

### Hardware Setup

1. Connect your iotaCore to your computer using a USB-C cable
2. The onboard LED should pulse blue indicating proper power
3. Your computer should recognize the device as a serial port

### Software Installation

```bash
# Install the iotaCore CLI tools
pip install iotacore-cli

# Install required dependencies
iotacore-cli install
```

### First Project

```cpp
#include <iotaCore.h>

void setup() {
    // Initialize LED on pin 13
    pinMode(13, OUTPUT);
}

void loop() {
    digitalWrite(13, HIGH);
    delay(1000);
    digitalWrite(13, LOW);
    delay(1000);
}
```

## Documentation

- [Full Documentation](https://iotacore.readthedocs.io/)
- [Examples](https://github.com/Mister-Industries/iotaCore/software/Arduino/)
- [Hardware Design Files](https://github.com/Mister-Industries/iotaCore/Hardware)

## Hardware

### Schematic

![iotaCore Schematic](assets/schematic.png)

The complete hardware design files are available in the [hardware repository](https://github.com/Mister-Industries/iotaCore/Hardware), including:
- Schematic files 
- PCB layout files (Altium/KiCad format)
- Bill of Materials (BOM)
- Manufacturing files (Gerber)
- Assembly instructions

### Pin Mapping

| Pin | Function | Notes |
|-----|----------|-------|
| D0  | UART RX  | Serial communication |
| D1  | UART TX  | Serial communication |
| D2-D12 | GPIO | Digital I/O |
| A0-A7 | Analog Input | 12-bit ADC |
| SCL | I2C Clock | I2C Bus 0 |
| SDA | I2C Data | I2C Bus 0 |

## Development

### Building from Source

```bash
# Clone the repository
git clone https://github.com/iotacore/iotacore.git

# Install dependencies
cd iotacore
make deps

# Build firmware
make firmware

# Build documentation
make docs
```

### Contributing

We welcome contributions! Please see our [Contributing Guide](CONTRIBUTING.md) for details on:
- Code style guidelines
- Development workflow
- Testing requirements
- Pull request process

## Community

- [Discord Server](https://discord.gg/iotakit)
- [Twitter](https://twitter.com/misterindustries)

## License

This project is licensed under the GPL-3.0 License - see the [LICENSE](LICENSE) file for details.

### Hardware License

The hardware design files are released under the [CERN Open Hardware Licence Version 2 - Strongly Reciprocal](https://ohwr.org/cern_ohl_s_v2.txt).

## Acknowledgments

Special thanks to:
- The Arduino team for inspiration
- ARM for microcontroller architecture
- All our open source contributors

## Support

- [Issue Tracker](https://github.com/iotacore/iotacore/issues)
- [Commercial Support](https://iotacore.org/support)
- Email: support@iotacore.org
