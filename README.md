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
