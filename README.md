Fingerprint-Based Secure SD Card Access System Using Microcontroller
An ESP32-based biometric access control system that uses a fingerprint sensor to authenticate users and retrieve corresponding records from a local SD card database, displayed on a 16x2 I2C LCD.
Team
Aarif Mohammed Ali Sheik
S Yogesh
G K Pranav Sankar
Objective
To implement administrator-controlled security using biometric authentication via fingerprint, where an admin ("BOSS") controls enrollment of new users and each enrolled fingerprint maps to a specific record stored on an SD card.
System Architecture
Microcontroller: ESP32 NodeMCU-32 board coordinating hardware communication via SPI and Serial (UART) lines.
User Interface: 16x2 I2C LCD displaying real-time system status, prompts, operating modes, and decoded storage data.
Biometric Hardware: Adafruit-compatible fingerprint sensor (R307 / R307S) connected via hardware UART (`HardwareSerial 2`).
Storage Hardware: MicroSD card reader wired via SPI bus.
Central File Database (`/database.txt`): Regenerated on every boot; contains 20 predefined records (car models), one per line.
Slot 1 (Admin / BOSS ID): Reserved as an execution toggle to switch the system between Enrollment Mode and Scanning Mode — it does not map to a database record.
Slots 2–21+ (User IDs): Biometric templates mapped 1:1 to lines in the SD card database.
Hardware Components
Component	Interface	Notes
ESP32 NodeMCU-32	—	Central microcontroller
Fingerprint Sensor (R307 / R307S)	UART (TX/RX)	Adafruit-compatible
MicroSD Card Module	SPI (MOSI, MISO, SCK, CS)	Stores `/database.txt`
I2C LCD Display (16x2)	I2C (SDA, SCL – 5V logic)	Status/prompt display
Logic Level Shifter	3.3V (LV) ↔ 5V (HV)	Bridges ESP32's 3.3V I2C to the LCD's 5V I2C
External 5V Power Adapter	5V / GND	Powers the ESP32 board
Laptop / PC	USB (Power & Data)	Programming and serial monitor
Wiring notes:
The ESP32 communicates with the fingerprint sensor over UART and with the MicroSD module over SPI.
The ESP32's I2C bus runs at 3.3V logic; a logic level shifter steps it up to 5V for the I2C LCD.
All modules share a common ground with the ESP32 and the power adapter.
Circuit Diagram
```


```
Workflow
Phase 1: Bootup & Hard Reset
Power on / reset the board.
Initialize LCD and SD card — halts on failure.
Write 20 predefined car model records to `/database.txt`, overwriting any old data.
Wipe the fingerprint sensor's flash memory (empties all stored templates).
Prompt for the mandatory BOSS fingerprint (2-stage scan) and store it in Slot 1.
Automatically enter Enrollment Mode.
Phase 2: Enrollment Mode (Adding Users)
LCD shows the next open slot: `Enroll User: \\\[X]` → `Place Finger`.
On finger placement, the sensor runs a fast look-up search:
Match = BOSS (Slot 1): exits enrollment and switches to Scanning Mode.
Match = Existing User (Slot > 1): LCD shows `Already Exists!` with the user's slot ID; waits for the finger to be lifted, then resets the prompt.
No match (clean finger): runs the dual-stage enrollment sequence (`Scan \\\[1/2]` → `Lift Finger` → `Scan \\\[2/2]`), stores the new template, and increments `nextID`.
New users are enrolled sequentially starting at Slot 2, so a user's database line = `Slot ID - 1`.
Phase 3: Scanning Mode (Access Control)
LCD idles on `Ready to Scan`, polling the sensor every 1000 ms.
BOSS scan (Slot 1): switches the system back into Enrollment Mode.
Valid user scan (Slot 2+): LCD shows `Access Granted`, reads `/database.txt`, jumps to line `Slot ID - 1`, and prints the matching record (e.g., Slot 2 → Line 1 → "Toyota Corolla") on the bottom LCD row.
Unknown fingerprint: LCD shows `Access Denied: Unknown User`, then clears and returns to idle.
Getting Started
Prerequisites
Arduino IDE with ESP32 board support installed
Libraries: `LiquidCrystal\\\_I2C`, `SPI`, `SD`, `Adafruit\\\_Fingerprint`, `HardwareSerial`
Pin Configuration (as used in code)
SD card chip select: `SD\\\_CS = 5`
Fingerprint sensor: `HardwareSerial(2)` at 57600 baud, RX = 16, TX = 17
LCD I2C address: `0x27`
Flashing
Open the sketch in Arduino IDE and select your ESP32 board.
Install the required libraries listed above.
Wire the hardware as described in the circuit diagram.
Upload the sketch. On first boot, the system will erase all fingerprint templates and prompt for BOSS enrollment — follow the on-screen instructions.
Code
```cpp
#include <LiquidCrystal\\\_I2C.h>
#include <SPI.h>
#include <SD.h>
#include <Adafruit\\\_Fingerprint.h>
#include <HardwareSerial.h>

LiquidCrystal\\\_I2C lcd(0x27, 16, 2);
#define SD\\\_CS 5
#define BOSS\\\_ID 1
const char\\\* DB\\\_FILE = "/database.txt";

HardwareSerial mySerial(2);
Adafruit\\\_Fingerprint finger = Adafruit\\\_Fingerprint(\\\&mySerial);

unsigned long lastScan = 0;
const unsigned long scanInterval = 1000;
int nextID = 2;
bool isEnrollmentMode = true;

const char\\\* carModels\\\[20] = {
  "Toyota Corolla", "Honda Civic", "Hyundai Creta", "Maruti Suzuki Swift",
  "Tata Nexon", "Mahindra Scorpio", "Kia Seltos", "Toyota Fortuner",
  "Hyundai Verna", "Honda City", "Volkswagen Virtus", "Skoda Slavia",
  "Renault Kwid", "Nissan Magnite", "MG Hector", "Jeep Compass",
  "BMW 3 Series", "Mercedes-Benz C-Class", "Audi A4", "Tesla Model 3"
};

void createDatabaseFile() {
  if (SD.exists(DB\\\_FILE)) {
    SD.remove(DB\\\_FILE);
  }
  File file = SD.open(DB\\\_FILE, FILE\\\_WRITE);
  if (file) {
    Serial.println("Creating fresh database.txt with car models...");
    for (int i = 0; i < 20; i++) {
      file.println(carModels\\\[i]);
    }
    file.close();
    Serial.println("database.txt created successfully.");
  } else {
    Serial.println("Failed to create database.txt!");
    lcd.clear();
    lcd.print("File Gen Fail");
    delay(2000);
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("System Starting");

  if (!SD.begin(SD\\\_CS)) {
    lcd.setCursor(0, 1);
    lcd.print("SD Fail");
    Serial.println("SD Init Failed!");
    while (1);
  } else {
    lcd.setCursor(0, 1);
    lcd.print("SD Ready");
    createDatabaseFile();
  }

  delay(1500);
  lcd.clear();

  mySerial.begin(57600, SERIAL\\\_8N1, 16, 17);
  finger.begin(57600);
  if (!finger.verifyPassword()) {
    lcd.print("FP Sensor Error");
    while (1);
  }

  lcd.setCursor(0, 0);
  lcd.print("Erasing Flash...");
  finger.emptyDatabase();
  delay(1500);
  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("Register BOSS");
  lcd.setCursor(0, 1);
  lcd.print("Place Finger");
  delay(1500);

  while (!enrollFingerprint(BOSS\\\_ID));

  isEnrollmentMode = true;
  nextID = 2;
  lcd.clear();
  lcd.print("Enrollment Mode");
  delay(1500);
  lcd.clear();
}

void loop() {
  if (isEnrollmentMode) {
    runEnrollmentWorkflow();
  } else {
    runScanningWorkflow();
  }
}

void runScanningWorkflow() {
  unsigned long currentMillis = millis();
  if (currentMillis - lastScan >= scanInterval) {
    lastScan = currentMillis;
    lcd.setCursor(0, 0);
    lcd.print("Scan Finger...  ");

    int result = getFingerprintID();

    if (result == BOSS\\\_ID) {
      lcd.clear();
      lcd.print("Boss Detected");
      lcd.setCursor(0, 1);
      lcd.print("Entering Enroll");
      delay(2000);
      isEnrollmentMode = true;
      lcd.clear();
    } else if (result > 1) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Access Granted");
      showSDDataFromLine(result);
      delay(3000);
      lcd.clear();
    } else if (result == -1) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Access Denied");
      lcd.setCursor(0, 1);
      lcd.print("Unknown User");
      delay(2000);
      lcd.clear();
    }
  }
}

void runEnrollmentWorkflow() {
  lcd.setCursor(0, 0);
  lcd.print("Enroll User: \\\[");
  lcd.print(nextID);
  lcd.print("]");
  lcd.setCursor(0, 1);
  lcd.print("Place Finger    ");

  int p = finger.getImage();
  if (p == FINGERPRINT\\\_OK) {
    p = finger.image2Tz(1);
    if (p != FINGERPRINT\\\_OK) return;

    p = finger.fingerFastSearch();
    if (p == FINGERPRINT\\\_OK) {
      if (finger.fingerID == BOSS\\\_ID) {
        lcd.clear();
        lcd.print("Boss Detected");
        lcd.setCursor(0, 1);
        lcd.print("Entering Scan...");
        delay(2000);
        isEnrollmentMode = false;
        lcd.clear();
        return;
      } else {
        lcd.clear();
        lcd.print("Already Exists!");
        lcd.setCursor(0, 1);
        lcd.print("ID Slot: ");
        lcd.print(finger.fingerID);
        delay(2000);
        lcd.clear();
        while (finger.getImage() != FINGERPRINT\\\_NOFINGER);
        return;
      }
    }

    lcd.clear();
    if (enrollFingerprint(nextID)) {
      nextID++;
    }
    lcd.clear();
  }
}

bool enrollFingerprint(int id) {
  int p = -1;
  lcd.setCursor(0, 0);
  if (id == BOSS\\\_ID) lcd.print("Enrolling BOSS  ");
  else { lcd.print("User Slot: "); lcd.print(id); }

  lcd.setCursor(0, 1);
  lcd.print("Scan \\\[1/2]      ");
  while (p != FINGERPRINT\\\_OK) {
    p = finger.getImage();
  }

  p = finger.image2Tz(1);
  if (p != FINGERPRINT\\\_OK) return false;

  lcd.setCursor(0, 1);
  lcd.print("Lift Finger     ");
  delay(1000);
  while (finger.getImage() != FINGERPRINT\\\_NOFINGER);

  lcd.setCursor(0, 1);
  lcd.print("Scan \\\[2/2]      ");
  while (finger.getImage() != FINGERPRINT\\\_OK);

  p = finger.image2Tz(2);
  if (p != FINGERPRINT\\\_OK) return false;

  p = finger.createModel();
  if (p != FINGERPRINT\\\_OK) {
    lcd.clear();
    lcd.print("Match Error!");
    delay(1500);
    return false;
  }

  p = finger.storeModel(id);
  if (p == FINGERPRINT\\\_OK) {
    lcd.clear();
    lcd.print("Saved Online!");
    delay(1500);
    return true;
  } else {
    lcd.clear();
    lcd.print("Save Error!");
    delay(1500);
    return false;
  }
}

void showSDDataFromLine(int id) {
  File file = SD.open(DB\\\_FILE, FILE\\\_READ);
  if (!file) {
    lcd.setCursor(0, 1);
    lcd.print("Missing DB File ");
    return;
  }

  int targetLine = id - 1;
  int currentLine = 1;
  String targetContent = "";
  bool lineFound = false;

  while (file.available()) {
    String line = file.readStringUntil('\\\\n');
    if (currentLine == targetLine) {
      line.trim();
      targetContent = line;
      lineFound = true;
      break;
    }
    currentLine++;
  }
  file.close();

  lcd.setCursor(0, 1);
  if (lineFound) {
    lcd.print("                ");
    lcd.setCursor(0, 1);
    lcd.print(targetContent.substring(0, 16));
    Serial.print("Matched line "); Serial.print(targetLine); Serial.print(": "); Serial.println(targetContent);
  } else {
    lcd.print("No Line Data    ");
    Serial.print("No line entry found mapping target line index: "); Serial.println(targetLine);
  }
}

int getFingerprintID() {
  int p = finger.getImage();
  if (p == FINGERPRINT\\\_NOFINGER) return -2;
  if (p != FINGERPRINT\\\_OK)       return -1;

  p = finger.image2Tz();
  if (p != FINGERPRINT\\\_OK)       return -1;

  p = finger.fingerFastSearch();
  if (p != FINGERPRINT\\\_OK)       return -1;

  return finger.fingerID;
}
```
Folder Structure
```
.
├── README.md
├── fingerprint.ino               # Main Arduino sketch (ESP32)
├── images/
│   └── circuit\\\_diagram.png       # Wiring / block diagram
└── docs/
    └── workflow.docx             # Original architecture \\\& workflow writeup
```
Notes / Known Limitations
On every reboot, `/database.txt` is fully overwritten and all fingerprint templates are wiped, requiring BOSS re-enrollment. This is by design for the current prototype and should be revisited before any persistent/production use.
The database currently holds 20 fixed records (car models); Slot 1 is reserved for BOSS and never maps to a record.
Future Work
Persistent storage: Move away from wipe-on-boot behavior — persist fingerprint template metadata and the `nextID` counter in EEPROM/NVS (or a flag file on the SD card) so the system retains enrolled users across power cycles.
Real user records: Replace the placeholder car-model database with actual user-relevant data (e.g., name, access level, timestamp log) mapped to each fingerprint slot.
Access logging: Log every scan attempt (granted/denied/unknown) with a timestamp to the SD card for an auditable access history.
Fallback authentication: Add a PIN/password fallback (e.g., via keypad) for cases where the fingerprint sensor fails to read.
Slot management: Add a BOSS-only "delete user" flow so revoking access doesn't require a full system wipe.
Scalability: Test and document behavior as the number of enrolled users approaches the sensor's template capacity.
License
For academic/project use only. No formal license has been applied yet — contact the team before reuse or distribution.
