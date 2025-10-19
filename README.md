# Solar Energy Monitoring System 🌞

A project utilizing the ESP32 to monitor real-time power output from a solar panel and autonomously manage a connected load based on available energy.

## Overview

This system tracks the instantaneous electrical output (Voltage, Current, and Power) of a solar array. It employs a smart load-shedding mechanism using a relay, ensuring that the connected appliance is only powered when the generated solar power exceeds a minimum operational threshold. This prevents battery deep discharge and ensures efficient use of solar energy.

## Features

* **Real-Time Monitoring:** Calculates and displays Voltage (V), Current (A), and Power (W) output.
* **Web Interface:** Access monitoring data and load status via a local Wi-Fi web server (auto-refreshing).
* **Smart Load Control:** Automatically connects the load when power $\ge$ `MIN_POWER_TO_CONNECT_W` and disconnects it when power drops.
* **Calibration Ready:** Code includes variables for easy calibration of the voltage and current sensors specific to your setup.

## Hardware Requirements

| Component | Quantity | Notes |
| :--- | :--- | :--- |
| **ESP32** or **ESP8266** | 1 | Microcontroller with built-in Wi-Fi and ADC inputs. |
| **Voltage Sensor Module** | 1 | Used for high voltage measurement (e.g., up to 25V DC). |
| **Current Sensor Module** | 1 | e.g., **ACS712** (5A or 20A version) for DC current measurement. |
| **1-Channel Relay Module** | 1 | To control the connected load. |
| **Solar Panel & Load** | 1 | The energy source and appliance to monitor. |
| **Jumper Wires** | Various | |

## Wiring Diagram

**⚠️ WARNING:** Working with solar panels and loads requires careful handling. Ensure all connections are secure, especially the current sensor which goes in series with the load.

1.  **Voltage Sensor:**
    * **VCC** $\rightarrow$ **ESP32 3.3V**
    * **GND** $\rightarrow$ **ESP32 GND**
    * **Signal (S)** $\rightarrow$ **ESP32 GPIO 34**

2.  **Current Sensor (ACS712 DC):**
    * **VCC** $\rightarrow$ **ESP32 5V** (or external 5V)
    * **GND** $\rightarrow$ **ESP32 GND**
    * **Vout (Analog Output)** $\rightarrow$ **ESP32 GPIO 35**
    * **Load:** Connect the solar panel positive output $\rightarrow$ Current Sensor **IN**, Current Sensor **OUT** $\rightarrow$ Relay/Load.

3.  **Load Relay Module:**
    * **VCC** $\rightarrow$ **ESP32 5V**
    * **GND** $\rightarrow$ **ESP32 GND**
    * **IN** $\rightarrow$ **ESP32 GPIO 27**
    * **Load Connection:** Connect the power line for the load through the relay's **COM** and **NO** (Normally Open) terminals.

## Software Setup and Installation

1.  **Arduino IDE:** Ensure the ESP32 board core is installed.
2.  **Libraries:** `WiFi.h` and `WebServer.h` (Standard ESP core libraries).
3.  **Critical Calibration:**
    * **Voltage:** Adjust `VOLTAGE_SCALE_FACTOR` based on your sensor's voltage divider ratio.
    * **Current:** **Crucially, measure the actual analog value** on `CURRENT_PIN` when no current is flowing and adjust the `ZERO_CURRENT_OFFSET` and `CURRENT_SENSITIVITY` if necessary for accuracy.
    * Set the `MIN_POWER_TO_CONNECT_W` threshold to your desired value.
4.  **Upload:** Update Wi-Fi credentials and upload the code to the ESP32.
5.  **Access:** Open the Serial Monitor to find the assigned **IP Address**. Access this IP in a web browser to view the solar energy metrics.
