# SancaBotApp

PlatformIO firmware for controlling the SancaBot robot from the SancaBot web application:

https://sancabot.com.br/app

This project was prepared and tested using **VSCode + PlatformIO** with an **ESP32-C3 Super Mini** board.

The purpose of this folder is to keep a reproducible local firmware version that can be compiled and uploaded from a local development environment, without depending only on a web-based flashing workflow.

## Purpose

This firmware allows the ESP32-C3 board used in the SancaBot robot to communicate through **Bluetooth Low Energy (BLE)** with the SancaBot web app.

This project was useful to verify the following parts of the robot:

* ESP32-C3 board
* BLE communication
* Continuous rotation servos
* Manual control from the web app
* Line-following firmware structure
* Obstacle-avoidance firmware structure

## Hardware platform

Target board:

* ESP32-C3 Super Mini

Main hardware connections used in this firmware:

| Function                        |   GPIO |
| ------------------------------- | -----: |
| Left continuous rotation servo  | GPIO 0 |
| Right continuous rotation servo | GPIO 1 |
| Left IR line sensor             | GPIO 5 |
| Right IR line sensor            | GPIO 6 |
| Ultrasonic trigger              | GPIO 7 |
| Ultrasonic echo                 | GPIO 8 |

Important note: in this firmware, GPIO 8 is used as the ultrasonic echo pin, following the SancaBot hardware configuration used in this test.

## Software environment

This project was tested with:

* VSCode
* PlatformIO
* Arduino framework for ESP32
* ESP32-C3 target board
* ESP32Servo library

The main source file is located at:

`src/main.cpp`

The PlatformIO configuration file is:

`platformio.ini`

## PlatformIO configuration

The project uses the following general PlatformIO setup:

* Platform: Espressif 32
* Board: ESP32-C3 DevKitM-1
* Framework: Arduino
* Upload protocol: esptool
* USB CDC enabled on boot
* Servo library: `madhephaestus/ESP32Servo`

## Build and upload

From the `SancaBotApp` folder, compile the project with:

`platformio run`

Upload the firmware with:

`platformio run --target upload`

Open the serial monitor with:

`platformio device monitor -b 115200`

If the upload port is different on your computer, update the `upload_port` field in `platformio.ini`.

## BLE device

When the firmware starts, it creates a BLE device with a dynamic name:

`SancaBot_XXXX`

The suffix depends on the ESP32-C3 chip ID.

The web app should detect this BLE device when connecting from:

https://sancabot.com.br/app

## BLE UUIDs

Service UUID:

`4fafc201-1fb5-459e-8fcc-c5c9c331914b`

Characteristic UUID:

`beb5483e-36e1-4688-b7f5-ea07361b26a8`

The characteristic supports:

* Read
* Write
* Write without response
* Notify

## BLE command protocol

The firmware receives text commands through the BLE characteristic.

Main commands:

| Command                   | Meaning                              |
| ------------------------- | ------------------------------------ |
| `P:left:right`            | Manual motor power, from -100 to 100 |
| `S`                       | Stop motors immediately              |
| `M:0`                     | Set manual mode                      |
| `G:0`                     | Stop autonomous mode                 |
| `G:1:mode`                | Start autonomous mode                |
| `C:spd:dF:dR:turn:tE:tD:` | General/manual configuration         |
| `L:spd:kp:inv:ldrift:`    | Line-following configuration         |
| `O:spd:dist:turnDrift:`   | Obstacle-avoidance configuration     |

Autonomous mode values:

| Mode | Meaning            |
| ---: | ------------------ |
|  `3` | Line following     |
|  `4` | Obstacle avoidance |

## BLE notifications

The firmware can notify the web app with status messages.

Main responses:

| Notification       | Meaning               |
| ------------------ | --------------------- |
| `A:OK`             | Command acknowledged  |
| `R:mode:L:R:dist:` | Periodic robot status |

The periodic status message includes:

* Current mode
* Left IR sensor state
* Right IR sensor state
* Ultrasonic distance in centimeters

## Important correction for PlatformIO

During the adaptation to VSCode + PlatformIO, the BLE write callback required a small correction when reading the characteristic value.

The corrected version uses:

`std::string raw = pC->getValue();`

and then converts it to an Arduino `String`:

`String v = String(raw.c_str());`

This avoids compilation issues related to direct conversion between `std::string` and Arduino `String`.

## Operating modes

### Manual mode

In manual mode, the web app sends motor power commands using:

`P:left:right`

where both values are between `-100` and `100`.

The firmware also includes a timeout mechanism. If no movement command is received for a short time, the motors are stopped automatically.

### Line-following mode

The line-following mode uses two IR sensors connected to GPIO 5 and GPIO 6.

The line-following behavior can be adjusted using the `L:` command.

### Obstacle-avoidance mode

The obstacle-avoidance mode uses an ultrasonic sensor connected to GPIO 7 and GPIO 8.

The obstacle distance threshold and turning tendency can be adjusted using the `O:` command.

## Repository

This folder is part of the repository:

https://github.com/HBG2025/sancaBot

It documents the local PlatformIO workflow used to test and upload the SancaBot firmware from VSCode.

## Educational purpose

This project is intended for learning, testing, documentation, and reproducibility of the SancaBot firmware workflow using PlatformIO.

The SancaBot project and its original educational ecosystem are acknowledged with gratitude.
