# 🌱 Mei-v2: ESP32-S3 Based Smart Agritech & IoT Irrigation System

<img width="3037" height="2160" alt="3D_PCB3_2026-04-25 (1)" src="https://github.com/user-attachments/assets/fea1b105-c802-45de-833f-70dc8b719882" />


<p align="center">
  <i>An end-to-end automated irrigation and environmental monitoring system designed for sensitive plant care, featuring asynchronous Telegram control and ThingSpeak data logging.</i>
</p>

---

##  Project Overview
Mei-v2 is built to solve real-world agricultural automation challenges on a micro-scale. Leveraging the powerful **ESP32-S3** microcontroller, it provides real-time data logging, cloud analytics, and secure remote control via a Telegram Bot interface. 

The software architecture focuses on **non-blocking asynchronous task management** (using `yield()` and custom hardware timers) to prevent Watchdog Timer (WDT) resets and SSL handshake bottlenecks during heavy API traffic. A custom PCB has been designed to eliminate wiring interference and ensure industrial-grade reliability.

##  Parts Required

**Core Components:**
* 1x ESP32 DevKit
* 1x 1.14" IPS LCD Display (ST7789 Driver, SPI Interface)
* 1x DHT22 Temperature & Humidity Sensor
* 2x Capacitive Soil Moisture Sensors (v1.2 recommended)
* 2x 5V Mini Submersible Water Pumps
* 1x Water Level Float Switch (Tank dry-run protection)

**Electronic Components & Power (Custom PCB Specs):**
* 2x N-Channel Power MOSFETs *IRLZ44N [not IRFZ] for driving the pumps
* 2x 1N4007 Flyback Diodes (Crucial for preventing motor voltage spikes)
* 1x 10kΩ Resistor (Pull-up for DHT22 data line)(if there is non between gnd and signal pin)
* 1x 5V Power Supply (Min 2A recommended to handle dual pump current spikes)

**Additional - Connectors & Modularity (JST-XH Series):**
* 4x JST-XH 2-pin connector
* 3x JST-XH 3-pin connector
* 1x JST-XH 8-pin connector

---

##  Uploading the Code to the ESP32

Upload the [firmware](Mei%20v2%20Autonomous%20Watering%20System/Software/Mei_v2/Mei_v2.ino) using **Arduino IDE**. Before uploading, ensure you have installed the following libraries via the Arduino Library Manager:

* `UniversalTelegramBot` by Brian Lough
* `ThingSpeak` by MathWorks
* `Adafruit GFX Library` & `Adafruit ST7735 and ST7789 Library`
* `DHT sensor library` by Adafruit
* `WiFiManager` by tzapu
* `ArduinoJson`

---

##  Hardware Architecture & Pinout

The hardware has been meticulously routed on a custom PCB. The pinout below reflects the optimized "Hardware-Software Co-design" approach to ensure clean signal routing and prevent Wi-Fi/ADC resource conflicts.

### 1.14" ST7789 TFT Display (Hardware SPI)
*Requires 3.3V logic. The BLK pin is driven by a PWM signal for dynamic brightness control.*

| ST7789 Pin | ESP32-S3 Pin | Notes |
| :--- | :--- | :--- |
| **VCC** | 3.3V | (+) Power |
| **GND** | GND | (-) Ground |
| **SCL** | GPIO 14 | SPI Clock |
| **SDA** | GPIO 13 | SPI MOSI |
| **RES** | GPIO 12 | Hardware Reset |
| **DC** | GPIO 11 | Data/Command |
| **CS** | GPIO 10 | Chip Select |
| **BLK** | GPIO 9  | PWM Brightness Output |

### DHT22 Temperature & Humidity Sensor
*Provides ambient environmental data for the dashboard.*

| DHT22 Pin | ESP32-S3 Pin | Additional Component |
| :--- | :--- | :--- |
| **VCC** | 3.3V / 5V | (+) Power |
| **GND** | GND | (-) Ground |
| **DATA** | GPIO 4 | Add a **10kΩ Pull-up Resistor** between DATA and VCC. |

### Capacitive Soil Moisture Sensors
*⚠️ **Hardware Note:** Sensors are strictly routed to **ADC1** pins (GPIO 5 & 6) to prevent resource conflicts with the ESP32 Wi-Fi module, which relies on ADC2.*

| Sensor Pin | ESP32-S3 Pin | Notes |
| :--- | :--- | :--- |
| **VCC (Both)** | 3.3V | Powering with 3.3V extends sensor lifespan. |
| **GND (Both)** | GND | (-) Ground |
| **SIG (Plant 1)**| GPIO 5 | ADC1 Channel (Mapped 4095 to 1000) |
| **SIG (Plant 2)** | GPIO 6 | ADC1 Channel (Mapped 4095 to 1000) |

### Submersible Water Pumps (Actuators)
*Driven by IRLZ44N MOSFETs located on the right side of the PCB for optimized trace routing.*

| Module / Pin | ESP32-S3 Pin | Additional Connection |
| :--- | :--- | :--- |
| **Pump 1** | GPIO 1 | Connect to MOSFET 1 Gate |
| **Pump 2** | GPIO 40 | Connect to MOSFET 2 Gate |
| **Pump Power (VCC)**| N/A | Connect to an **External 5V Power Supply** |
| **Pump GND** | N/A | Must share a **Common Ground** with the ESP32 |
| **Motor Terminals** | N/A | Add a **1N4007 Flyback Diode** parallel to the motor. |

### Water Level Float Switch (Safety Failsafe)
*Prevents pumps from running dry. Uses internal pull-up resistors.*

| Switch Pin | ESP32-S3 / Power Pin | Notes |
| :--- | :--- | :--- |
| **Pin 1** | GPIO 46 | Configured as `INPUT_PULLUP`. |
| **Pin 2** | GND | Triggers `HIGH` when the tank is empty, halting the system. |

---

##  Step-by-Step Setup Guide

### 1. Create Your Telegram Bot
1. Open Telegram and search for **`@BotFather`**.
2. Send `/newbot` and follow the instructions to get your **API Token**.
3. Search for **`@IDBot`** to get your personal **Chat ID** (Required for security whitelist).

### 2. Configure ThingSpeak (Cloud Analytics)
1. Create a free account on [ThingSpeak](https://thingspeak.com/).
2. Create a **New Channel** and enable 8 Fields:
   * Field 1: Temperature (°C)
   * Field 2: Air Humidity (%)
   * Field 3: Purple Plant Moisture (%)
   * Field 4: White Plant Moisture (%)
   * Field 5: Target Humidity (%)
   * Field 6: Water Tank Status
   * Field 7: Pump 1 Runs
   * Field 8: Pump 2 Runs
3. Go to the **API Keys** tab and copy your `Channel ID` and `Write API Key`.

<img width="1190" height="852" alt="Schematic_View_Mei_v2" src="https://github.com/user-attachments/assets/2b528a7e-313b-4020-b6cc-1ab8ad046026" />

---

##  Telegram Command Interface
*Note: The system is deployed for personal use in a real-world environment, therefore custom interaction messages are kept in the native language (Turkish) to maintain its organic feel.*

* `📊 /status` - Fetches real-time sensor data, screen brightness, and water tank status.
* `💧 /nem <value>` - Dynamically sets the target soil moisture threshold (20% - 75%).
* `💡 /light <value>` - Adjusts the physical TFT screen brightness via PWM (0 - 100).
* `⏳ /time <value>` - Sets the watering pump active duration in seconds (1 - 10).
* `⏱️ /wait <value>` - Sets the cooldown period between waterings in minutes (1 - 60).
* `❓ /help` - Displays the command menu.

##  Roadmap & Milestones
* [x] **Custom PCB Design:** Successfully migrated from protoboard to a custom-designed, noise-isolated PCB via EasyEDA. (Gerber & Source files available in the [Hardware](Mei%20v2%20Autonomous%20Watering%20System/Hardware/) directory).
* [ ] **Predictive Analysis:** Utilizing the exported CSV time-series data from ThingSpeak to train a basic Machine Learning regression model for water depletion forecasting.

<img width="2160" height="2335" alt="3D_PCB3_2026-04-25 (3)" src="https://github.com/user-attachments/assets/18a9842b-71f1-4aa8-9c53-d6b50b36cf76" />

