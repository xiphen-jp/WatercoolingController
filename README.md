# WatercoolingController

Fan/Pump Controller for Watercooling PC using Arduino

*[English](README.md), [日本語](README.ja.md)*

## Description

- Control pump/fan speed according to thermal condition of water and room
  - Difference between room and water temperature
  - Water temperature
- Cooperate with [WatercoolingMonitor](https://github.com/xiphen-jp/WatercoolingMonitor)
  - Monitoring on PC
  - Receive CPU/GPU usage from PC (Can be used for speed control)

## Circuit Diagram

![circuit diagram](https://raw.githubusercontent.com/wiki/xiphen-jp/WatercoolingController/images/watercooling_controller_circuit.png)
![breadboard](https://raw.githubusercontent.com/wiki/xiphen-jp/WatercoolingController/images/watercooling_controller_breadboard.png)

- Pump/fan can be added
- Possible to detect rpm pulse up to 14 total number of pump/fan
  - Low-pass filter and pull-up resistor required separately

## Requirement

- [milesburton/Arduino-Temperature-Control-Library](https://github.com/milesburton/Arduino-Temperature-Control-Library)

### Hardware

- Arduino Nano v3.0 or compatible board
- PWM control fans/pumps
- DS18B20 Digital Thermometer
- PCB, Pin Headers/Sockets, Connectors, Registers and Capacitors
- USB Cable

## Author

[@_kure_](https://twitter.com/_kure_)
