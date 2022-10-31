# Modbus RTU to TCP Bridge
A simple Modbus RTU Master to Modbus TCP Server Bridge that is done with off-the shelf parts utilizing the ESP-8266

## Hardware
### Specifications
  - Current Requirements: ~30mA@12V
### BOM
 - ESP-8266 https://esp8266-shop.com/product/nodemcu-esp8266-esp-12e/
 - RS485 Board https://www.waveshare.com/rs485-board-3.3v.htm or equivalent
 - 3V3 DC-DC Regulator https://www.pololu.com/category/209/d24v5fx-step-down-voltage-regulators
 - Wire, solder etc.
### Schematic
![Modbus Bridge_bb](https://user-images.githubusercontent.com/91916713/199119731-2b869dc3-026b-4651-bbf5-00f1b1e5f31f.png)
The waveshare board has all the passive components and connector incorporated.
### Configuration

## Known Limitations
 - Only one TCP Client/Master is supported at the same time. **Unpredicted behaviour on end application may occur if multiple TCP Clients make requests at the same time.**
