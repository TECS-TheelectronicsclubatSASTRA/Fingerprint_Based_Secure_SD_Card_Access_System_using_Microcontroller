# Fingerprint-Based Secure SD Card Access System Using Microcontroller

An ESP32-based biometric access control system that uses a fingerprint sensor to authenticate users and retrieve corresponding records from a local SD card database, displayed on a 16x2 I2C LCD.

## Team

* Aarif Mohammed Ali Sheik
* S Yogesh
* G K Pranav Sankar

## Objective

To implement administrator-controlled security using biometric authentication via fingerprint, where an admin ("BOSS") controls enrollment of new users and each enrolled fingerprint maps to a specific record stored on an SD card.

## System Architecture

* **Microcontroller:** ESP32 NodeMCU-32 board coordinating hardware communication via SPI and Serial (UART) lines.
* **User Interface:** 16x2 I2C LCD displaying real-time system status, prompts, operating modes, and decoded storage data.
* **Biometric Hardware:** Adafruit-compatible fingerprint sensor (R307 / R307S) connected via hardware UART (`HardwareSerial 2`).
* **Storage Hardware:** MicroSD card reader wired via SPI bus.
* **Central File Database (`/database.txt`):** Regenerated on every boot; contains 20 predefined records (car models), one per line.
* **Slot 1 (Admin / BOSS ID):** Reserved as an execution toggle to switch the system between Enrollment Mode and Scanning Mode — it does not map to a database record.
* **Slots 2–21+ (User IDs):** Biometric templates mapped 1:1 to lines in the SD card database.

## Hardware Components

| Component | Interface | Notes |
| --- | --- | --- |
| ESP32 NodeMCU-32 | — | Central microcontroller |
| Fingerprint Sensor (R307 / R307S) | UART (TX/RX) | Adafruit-compatible |
| MicroSD Card Module | SPI (MOSI, MISO, SCK, CS) | Stores `/database.txt` |
| I2C LCD Display (16x2) | I2C (SDA, SCL – 5V logic) | Status/prompt display |
| Logic Level Shifter | 3.3V (LV) ↔ 5V (HV) | Bridges ESP32's 3.3V I2C to the LCD's 5V I2C |
| External 5V Power Adapter | 5V / GND | Powers the ESP32 board |
| Laptop / PC | USB (Power & Data) | Programming and serial monitor |

**Wiring notes:**

* The ESP32 communicates with the fingerprint sensor over UART and with the MicroSD module over SPI.
* The ESP32's I2C bus runs at 3.3V logic; a logic level shifter steps it up to 5V for the I2C LCD.
* All modules share a common ground with the ESP32 and the power adapter.

## Circuit Diagram
![Circuit Diagram](./Circuit_Diagram.png)

## Workflow

### Phase 1: Bootup & Hard Reset

1. Power on / reset the board.
2. Initialize LCD and SD card — halts on failure.
3. Write 20 predefined car model records to `/database.txt`, overwriting any old data.
4. Wipe the fingerprint sensor's flash memory (empties all stored templates).
5. Prompt for the mandatory BOSS fingerprint (2-stage scan) and store it in **Slot 1**.
6. Automatically enter **Enrollment Mode**.

### Phase 2: Enrollment Mode (Adding Users)

* LCD shows the next open slot: `Enroll User: [X]` → `Place Finger`.
* On finger placement, the sensor runs a fast look-up search:
  * **Match = BOSS (Slot 1):** exits enrollment and switches to **Scanning Mode**.
  * **Match = Existing User (Slot > 1):** LCD shows `Already Exists!` with the user's slot ID; waits for the finger to be lifted, then resets the prompt.
  * **No match (clean finger):** runs the dual-stage enrollment sequence (`Scan [1/2]` → `Lift Finger` → `Scan [2/2]`), stores the new template, and increments `nextID`.
* New users are enrolled sequentially starting at Slot 2, so a user's database line = `Slot ID - 1`.

### Phase 3: Scanning Mode (Access Control)

* LCD idles on `Ready to Scan`, polling the sensor every 1000 ms.
* **BOSS scan (Slot 1):** switches the system back into Enrollment Mode.
* **Valid user scan (Slot 2+):** LCD shows `Access Granted`, reads `/database.txt`, jumps to line `Slot ID - 1`, and prints the matching record (e.g., Slot 2 → Line 1 → "Toyota Corolla") on the bottom LCD row.
* **Unknown fingerprint:** LCD shows `Access Denied: Unknown User`, then clears and returns to idle.
![Workflow Diagram](./Workflow.png)

## Hardware Setup Gallery

| Setup View 1 | Setup View 2 |
| :---: | :---: |
| ![Project Image 1](./Project_img1.png) | ![Project Image 2](./Project_img2.png) |

## Getting Started

### Prerequisites

* Arduino IDE with ESP32 board support installed
* Libraries: `LiquidCrystal_I2C`, `SPI`, `SD`, `Adafruit_Fingerprint`, `HardwareSerial`

### Pin Configuration (as used in code)

* SD card chip select: `SD_CS = 5`
* Fingerprint sensor: `HardwareSerial(2)` at 57600 baud, RX = 16, TX = 17
* LCD I2C address: `0x27`

### Flashing

1. Open the sketch in Arduino IDE and select your ESP32 board.
2. Install the required libraries listed above.
3. Wire the hardware as described in the circuit diagram.
4. Upload the sketch. On first boot, the system will erase all fingerprint templates and prompt for BOSS enrollment — follow the on-screen instructions.
5. 
## Notes / Known Limitations

* On every reboot, `/database.txt` is fully overwritten and **all fingerprint templates are wiped**, requiring BOSS re-enrollment. This is by design for the current prototype and should be revisited before any persistent/production use.
* The database currently holds 20 fixed records (car models); Slot 1 is reserved for BOSS and never maps to a record.

## Future Work

* **Persistent storage:** Move away from wipe-on-boot behavior — persist fingerprint template metadata and the `nextID` counter in EEPROM/NVS (or a flag file on the SD card) so the system retains enrolled users across power cycles.
* **Real user records:** Replace the placeholder car-model database with actual user-relevant data (e.g., name, access level, timestamp log) mapped to each fingerprint slot.
* **Access logging:** Log every scan attempt (granted/denied/unknown) with a timestamp to the SD card for an auditable access history.
* **Fallback authentication:** Add a PIN/password fallback (e.g., via keypad) for cases where the fingerprint sensor fails to read.
* **Slot management:** Add a BOSS-only "delete user" flow so revoking access doesn't require a full system wipe.
* **Scalability:** Test and document behavior as the number of enrolled users approaches the sensor's template capacity.

## License

For academic/project use only. No formal license has been applied yet — contact the team before reuse or distribution.
<div align="center">

# Fingerprint-Based Secure SD Card Access System

An ESP32-powered biometric access control system utilizing optical fingerprint authentication, an SPI-based SD card database, and a 16x2 I2C LCD interface.

[![Organization](https://img.shields.io/badge/TECS-SASTRA-333333?style=plastic)](https://github.com/TECS-TheelectronicsclubatSASTRA)
[![Hardware](https://img.shields.io/badge/Hardware-ESP32-007acc?style=plastic)](https://www.espressif.com/)
[![Firmware](https://img.shields.io/badge/Firmware-Arduino--C%2B%2B-00878f?style=plastic)](https://www.arduino.cc/)
[![Build](https://img.shields.io/badge/Build-Passing-28a745?style=plastic)]()

[Overview](#overview) • [Key Features](#key-features) • [Hardware Specifications](#hardware-specifications) • [Circuit Diagram](#circuit-diagram) • [System Workflow](#system-workflow) • [Hardware Gallery](#hardware-gallery) • [Getting Started](#getting-started) • [Repository Structure](#repository-structure) • [Team](#team)

</div>

---

## Overview

This project implements an administrator-controlled security system built on the ESP32 platform. Biometric authentication is managed via a two-tier hierarchy: an administrative fingerprint ("BOSS") controls system mode transitions, while standard user fingerprints map directly to indexed user data records stored in a MicroSD card database (`/database.txt`). System status, operational modes, and retrieved user data are rendered in real time on a 16x2 I2C LCD display.

---

## Key Features

* **Role-Based Access Control:** Slot 1 is reserved exclusively for the administrator ("BOSS") to toggle between Enrollment and Scanning modes.
* **Direct Database Mapping:** Biometric templates in slots 2+ map 1:1 to indexed text entries within the MicroSD card database file.
* **Real-Time Display Output:** Interfaced 16x2 I2C LCD provides immediate system state feedback and data retrieval displays.
* **Autonomous Database Initialization:** Generates database structures on bootup and validates module communication before operation.

---

## Hardware Specifications

| Component | Communication Interface | Description / Role |
| :--- | :---: | :--- |
| **ESP32 NodeMCU-32** | Main Controller | Manages system logic, SPI, I2C, and Hardware UART buses |
| **R307 / R307S Fingerprint Module** | UART (TX/RX) | Biometric template capture, storage, and matching |
| **MicroSD Card Module** | SPI | Hosts the central data registry (`/database.txt`) |
| **16x2 I2C LCD Display** | I2C (SDA/SCL) | Primary visual display interface |
| **Logic Level Shifter** | I2C Bridge | Converts ESP32 3.3V logic levels to 5V LCD logic levels |
| **External 5V Power Supply** | Power | Delivers stable power supply across all connected peripherals |

---

## Circuit Diagram

<div align="center">

![Circuit Diagram](./Circuit_Diagram.png)

</div>

---

## System Workflow

<div align="center">

![Workflow Diagram](./Workflow.png)

</div>

### Operational Modes

1. **Initialization:** On bootup, the system verifies SD module readiness, clears volatile memory, and prompts for primary administrator enrollment into Slot 1.
2. **Enrollment Mode:** Scanning the administrator fingerprint switches the system into enrollment. New user fingerprints are registered sequentially starting at Slot 2.
3. **Scanning Mode:** Continuous background scanning. Scanning a registered user fetches and renders the corresponding line entry from the SD database onto the LCD display.

---

## Hardware Gallery

<div align="center">

| System Assembly View 1 | System Assembly View 2 |
| :---: | :---: |
| <img src="./Project_img1.png" width="420"/> | <img src="./Project_img2.png" width="420"/> |

</div>

---

## Getting Started

### Prerequisites

* **Development Environment:** [Arduino IDE](https://www.arduino.cc/en/software) configured with ESP32 board support.
* **Required Libraries:**
  * `LiquidCrystal_I2C`
  * `SPI`
  * `SD`
  * `Adafruit_Fingerprint`
  * `HardwareSerial`

### Hardware Pinout Configuration

* **MicroSD Module CS:** `GPIO 5`
* **Fingerprint Sensor:** `UART2` (RX = `GPIO 16`, TX = `GPIO 17`) at `57600 baud`
* **I2C LCD Address:** `0x27`

### Setup and Upload Procedure

1. Clone or download this repository.
2. Open [`Final_Fingerprint.ino`](./Final_Fingerprint.ino) in Arduino IDE.
3. Select your ESP32 target board and corresponding serial port.
4. Compile and upload the sketch. View debug output via the Serial Monitor at `115200 baud`.

<div align="center">

# Fingerprint-Based Secure SD Card Access System

An ESP32-powered biometric access control system utilizing optical fingerprint authentication, an SPI-based SD card database, and a 16x2 I2C LCD interface.

[![Organization](https://img.shields.io/badge/TECS-SASTRA-333333?style=plastic)](https://github.com/TECS-TheelectronicsclubatSASTRA)
[![Hardware](https://img.shields.io/badge/Hardware-ESP32-007acc?style=plastic)](https://www.espressif.com/)
[![Firmware](https://img.shields.io/badge/Firmware-Arduino--C%2B%2B-00878f?style=plastic)](https://www.arduino.cc/)
[![Build](https://img.shields.io/badge/Build-Passing-28a745?style=plastic)]()

[Overview](#overview) • [Key Features](#key-features) • [Hardware Specifications](#hardware-specifications) • [Circuit & Pinout](#circuit-diagram--pinout) • [System Workflow](#system-workflow) • [Hardware Gallery](#hardware-gallery) • [Getting Started](#getting-started) • [Datasheets](#datasheets--documentation) • [Repository Structure](#repository-structure) • [Notes & Limitations](#notes--known-limitations) • [Future Work](#future-work) • [Team](#team) • [License](#license)

</div>

---

## Overview

This project implements an administrator-controlled security system built on the ESP32 platform. Biometric authentication is managed via a two-tier hierarchy: an administrative fingerprint ("BOSS") controls system mode transitions, while standard user fingerprints map directly to indexed user data records stored in a MicroSD card database (`/database.txt`). System status, operational modes, and retrieved user data are rendered in real time on a 16x2 I2C LCD display.

---

## Key Features

* **Role-Based Access Control:** Slot 1 is reserved exclusively for the administrator ("BOSS") to toggle between Enrollment and Scanning modes.
* **Direct Database Mapping:** Biometric templates in slots 2+ map 1:1 to indexed text entries within the MicroSD card database file.
* **Real-Time Display Output:** Interfaced 16x2 I2C LCD provides immediate system state feedback and data retrieval displays.
* **Autonomous Database Initialization:** Generates database structures on bootup and validates module communication before operation.

---

## Hardware Specifications

| Component | Communication Interface | Description / Role |
| :--- | :---: | :--- |
| **ESP32 NodeMCU-32** | Main Controller | Manages system logic, SPI, I2C, and Hardware UART buses |
| **R307 / R307S Fingerprint Module** | UART (TX/RX) | Biometric template capture, storage, and matching |
| **MicroSD Card Module** | SPI | Hosts the central data registry (`/database.txt`) |
| **16x2 I2C LCD Display** | I2C (SDA/SCL) | Primary visual display interface |
| **Logic Level Shifter** | I2C Bridge | Converts ESP32 3.3V logic levels to 5V LCD logic levels |
| **External 5V Power Supply** | Power | Delivers stable power supply across all connected peripherals |

---

## Circuit Diagram & Pinout

<div align="center">

| Circuit Diagram | ESP32 Pinout |
| :---: | :---: |
| <img src="./Circuit_Diagram.png" width="450"/> | <img src="./ESP32-Pinout.png" width="450"/> |

</div>

---

## System Workflow

<div align="center">

![Workflow Diagram](./Workflow.png)

</div>

### Operational Modes

1. **Initialization:** On bootup, the system verifies SD module readiness, clears volatile memory, and prompts for primary administrator enrollment into Slot 1.
2. **Enrollment Mode:** Scanning the administrator fingerprint switches the system into enrollment. New user fingerprints are registered sequentially starting at Slot 2.
3. **Scanning Mode:** Continuous background scanning. Scanning a registered user fetches and renders the corresponding line entry from the SD database onto the LCD display.

---

## Hardware Gallery

<div align="center">

| System Assembly View 1 | System Assembly View 2 |
| :---: | :---: |
| <img src="./Project_img1.png" width="420"/> | <img src="./Project_img2.png" width="420"/> |

</div>

---

## Getting Started

### Prerequisites

* **Development Environment:** [Arduino IDE](https://www.arduino.cc/en/software) configured with ESP32 board support.
* **Required Libraries:**
  * `LiquidCrystal_I2C`
  * `SPI`
  * `SD`
  * `Adafruit_Fingerprint`
  * `HardwareSerial`

### Hardware Pinout Configuration

* **MicroSD Module CS:** `GPIO 5`
* **Fingerprint Sensor:** `UART2` (RX = `GPIO 16`, TX = `GPIO 17`) at `57600 baud`
* **I2C LCD Address:** `0x27`

### Setup and Upload Procedure

1. Clone or download this repository.
2. Open [`Final_Fingerprint.ino`](./Final_Fingerprint.ino) in Arduino IDE.
3. Select your ESP32 target board and corresponding serial port.
4. Compile and upload the sketch. View debug output via the Serial Monitor at `115200 baud`.

---

## Datasheets & Documentation

Hardware reference documentation included in this repository:

* **Microcontroller Datasheet:** [`esp32_datasheet_en.pdf`](./esp32_datasheet_en.pdf)
* **Fingerprint Sensor Datasheet:** [`R307.PDF`](./R307.PDF)

---

## Repository Structure

```text
.
├── Circuit_Diagram.png       # Electrical connection schematic
├── ESP32-Pinout.png          # ESP32 pin configuration diagram
├── Final_Fingerprint.ino     # Main ESP32 source code
├── Project_img1.png          # Physical hardware assembly setup
├── Project_img2.png          # System display and sensor view
├── R307.PDF                  # R307 fingerprint sensor datasheet
├── README.md                 # Project documentation
├── Workflow.png              # Architectural workflow diagram
└── esp32_datasheet_en.pdf    # ESP32 microcontroller datasheet
