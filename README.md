# Project-FIT - DARKBOX Environment Control

Project FIT in ALICE. As part of the project, we decided to create environmental monitoring within the black box. We will monitor pressure, temperature, humidity, and light intensity.

## Table of Contents

- [Requirements](#requirements)
- [Schematic](#schematic)
- [Project Progress](#project-progress)
- [Documentation](#documentation)
- [Authors](#authors)


## Requirements

- KA-Nucleo-Weather
- Arduino or a similar microcontroller
- RTC DS1307

## Schematic

![schematic](https://github.com/user-attachments/assets/cddea88b-0ec8-4fb5-b417-5ea7f5d9da89)

## Project Progress

- Week: October 17-24, 2024:
  1. Familiarization with equipment documentation.
  2. Establishing basic project assumptions.
- Two weeks: October 25, 2024 - November 7, 2024:
  1. Reading temperature, humidity, and pressure from sensors.
  2. Preparing the configuration screen.
- November 14, 2024 - November 21, 2024:
  1. Reading illuminance.
  2. Small changes in configuration screen.
  3. Few changes in displaying measurements.
- November 22, 2024 - November 28, 2024:
  1. Configuration screen using SerialMenu library.
  2. LED alert when light detected.
  3. Protection against user trying to put bad data to critical values.
  4. Some of code cleanup.
- November 29, 2024 - December 5, 2024:
  1. Added time measurement.
  2. Testing accuracy of time measurement.
- December 6, 2024 - December 12, 2024:
  1. Switched to measuring time with RTC DS1307
- December 13, 2024 - December 18, 2024:
  1. Improved continous display mode.
  2. LED signaling whether the device works.
  3. LED signaling temperature, pressure or humidity alerts.
  4. Added saving of critical values ​​to EEPROM.
- December 19, 2024 - January 9, 2025:
  1. Added saving of sensor values ​​to EEPROM memory.
- January 10, 2025 - January 16, 2025:
  1. Added functions for clearing EEPROM contents.
  2. Fixing EEPROM write

## Documentation
[View documentation](Documentation.pdf)

## Authors
Sara Libirt and Jakub Kozdrój.
