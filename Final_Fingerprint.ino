#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <SD.h>
#include <Adafruit_Fingerprint.h>
#include <HardwareSerial.h>

#include <WiFi.h>
#include <HTTPClient.h>
#include <time.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);

#define SD_CS 5
#define BOSS_ID 1
const char* DB_FILE = "/database.txt";

HardwareSerial mySerial(2);
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);

unsigned long lastScan = 0;
const unsigned long scanInterval = 1000;

int nextSequentialID = 2;
bool isEnrollmentMode = true;

const char* WIFI_SSID = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";
const char* GAS_URL = "https://script.google.com/macros/s/YOUR_DEPLOYMENT_ID_HERE/exec";
bool wifiConnected = false;
bool isLoggedIn[128] = {false};
bool slotOccupied[128] = {false};
int freeSlotList[128];
int freeSlotCount = 0;
int activeEmployeeCount = 0;

const char* toyotaModels[27] = {
  "Toyota Corolla", "Toyota Camry", "Toyota Fortuner", "Toyota Hilux",
  "Toyota Innova Hycross", "Toyota Urban Cruiser", "Toyota Glanza", "Toyota Yaris",
  "Toyota Land Cruiser", "Toyota Supra", "Toyota GR86", "Toyota Prius",
  "Toyota Crown", "Toyota Avalon", "Toyota Sequoia", "Toyota Tacoma",
  "Toyota Tundra", "Toyota RAV4", "Toyota Highlander", "Toyota Vellfire",
  "Toyota Alphard", "Toyota C-HR", "Toyota Aygo", "Toyota bZ4X",
  "Toyota Mirai", "Toyota Century", "Toyota Sienta"
};

const char* toyotaPins[27] = {
  "492851", "771204", "185933", "302847",
  "958162", "647503", "110395", "884721",
  "539046", "216578", "403912", "725864",
  "148239", "976541", "352087", "681954",
  "047362", "593018", "824675", "306492",
  "715803", "468925", "157340", "982416",
  "503871", "241658", "739204"
};

void connectWiFi() {
  if (strlen(WIFI_SSID) == 0 || strcmp(WIFI_SSID, "YOUR_SSID_HERE") == 0) {
    Serial.println("Wi-Fi credentials not configured.");
    wifiConnected = false;
    return;
  }
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
    Serial.println("\nWi-Fi Connected.");
  } else {
    wifiConnected = false;
    Serial.println("\nWi-Fi Connection Failed. Continuing offline.");
  }
}

String getTimestamp() {
  if (!wifiConnected) return "OFFLINE";
  time_t now = time(nullptr);
  if (now < 100000) return "TIME_NOT_SET";
  struct tm* timeinfo = localtime(&now);
  char buf[30];
  strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", timeinfo);
  return String(buf);
}

String urlEncode(const String &str) {
  String encoded = "";
  char buf[4];
  for (unsigned int i = 0; i < str.length(); i++) {
    char c = str.charAt(i);
    if (isalnum(c)) encoded += c;
    else if (c == ' ') encoded += '+';
    else { sprintf(buf, "%%%02X", (unsigned char)c); encoded += buf; }
  }
  return encoded;
}

void logToGoogleSheets(int slot, const String& project, const String& pin, const String& status) {
  if (!wifiConnected) {
    Serial.println("Wi-Fi offline. Skipping Google Sheets log.");
    return;
  }
  if (strlen(GAS_URL) == 0 || strcmp(GAS_URL, "YOUR_GOOGLE_APPS_SCRIPT_URL_HERE") == 0) {
    Serial.println("Google Apps Script URL not configured. Skipping log.");
    return;
  }

  String url = String(GAS_URL) +
               "?timestamp=" + urlEncode(getTimestamp()) +
               "&slot=" + String(slot) +
               "&project=" + urlEncode(project) +
               "&pin=" + urlEncode(pin) +
               "&status=" + urlEncode(status);

  HTTPClient http;
  http.setTimeout(10000);
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  http.begin(url);
  http.setUserAgent("ESP32-ToyotaTerminal/1.0");

  int httpCode = http.GET();
  Serial.print("GAS Response Code: ");
  Serial.println(httpCode);

  if (httpCode > 0) {
    String response = http.getString();
    Serial.println("Response: " + response.substring(0, 200));
  } else {
    Serial.print("GAS GET Failed: ");
    Serial.println(http.errorToString(httpCode).c_str());
  }
  http.end();
}

void createDatabaseFile() {
  if (SD.exists(DB_FILE)) {
    SD.remove(DB_FILE);
  }

  File file = SD.open(DB_FILE, FILE_WRITE);
  if (file) {
    Serial.println("Creating fresh database.txt with Toyota projects...");
    for (int i = 0; i < 27; i++) {
      file.print(toyotaModels[i]);
      file.print(",");
      file.println(toyotaPins[i]);
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


int peekNextFreeSlot() {
  if (activeEmployeeCount >= 27) {
    return -1; 
  }
  if (freeSlotCount > 0) {
    int lowest = freeSlotList[0];
    for (int i = 1; i < freeSlotCount; i++) {
      if (freeSlotList[i] < lowest) {
        lowest = freeSlotList[i];
      }
    }
    return lowest;
  }
  if (nextSequentialID <= 127) {
    return nextSequentialID;
  }
  return -1;
}

void consumeFreeSlot(int slot) {
  for (int i = 0; i < freeSlotCount; i++) {
    if (freeSlotList[i] == slot) {
      freeSlotList[i] = freeSlotList[freeSlotCount - 1];
      freeSlotCount--;
      return;
    }
  }
  if (slot == nextSequentialID) {
    nextSequentialID++;
  }
}

void addFreeSlot(int slot) {
  if (slot >= 2 && slot <= 127 && freeSlotCount < 128) {
    freeSlotList[freeSlotCount++] = slot;
    activeEmployeeCount--;
    Serial.print("Slot freed: ");
    Serial.println(slot);
  }
}

bool readEmployeeData(int id, String &projectName, String &pin) {
  int targetLine = id - 1;
  if (targetLine < 1 || targetLine > 27) {
    Serial.println("Slot ID out of project range.");
    return false;
  }

  File file = SD.open(DB_FILE, FILE_READ);
  if (!file) {
    Serial.println("Failed to open database.txt!");
    return false;
  }

  int currentLine = 1;
  while (file.available()) {
    String line = file.readStringUntil('\n');
    if (currentLine == targetLine) {
      line.trim();
      int commaIndex = line.indexOf(',');
      if (commaIndex > 0) {
        projectName = line.substring(0, commaIndex);
        pin = line.substring(commaIndex + 1);
      } else {
        projectName = line;
        pin = "000000";
      }
      file.close();
      return true;
    }
    currentLine++;
  }
  file.close();
  return false;
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  lcd.init();
  lcd.backlight();

  lcd.setCursor(0, 0);
  lcd.print("System Starting");

  // Initialize SD Card
  if (!SD.begin(SD_CS)) {
    lcd.setCursor(0, 1);
    lcd.print("SD Fail");
    Serial.println("SD Init Failed!");
    while(1); 
  } else {
    lcd.setCursor(0, 1);
    lcd.print("SD Ready");
    createDatabaseFile();
  }
  delay(1500);
  lcd.clear();

  connectWiFi();
  if (wifiConnected) {
    lcd.setCursor(0, 0);
    lcd.print("WiFi Connected");
    configTime(0, 0, "pool.ntp.org", "time.nist.gov");
    delay(1000);
  } else {
    lcd.setCursor(0, 0);
    lcd.print("WiFi Offline");
    Serial.println("Continuing in offline mode.");
  }
  delay(1500);
  lcd.clear();

  mySerial.begin(57600, SERIAL_8N1, 16, 17);
  finger.begin(57600);

  if (!finger.verifyPassword()) {
    lcd.print("FP Sensor Error");
    while (1);
  }

  lcd.setCursor(0, 0);
  lcd.print("Erasing Flash...");
  for (int i = 2; i <= 127; i++) {
    finger.deleteModel(i);
  }
  delay(500);
  lcd.clear();

  for (int i = 0; i < 128; i++) {
    isLoggedIn[i] = false;
    slotOccupied[i] = false;
  }
  freeSlotCount = 0;
  activeEmployeeCount = 0;
  nextSequentialID = 2;

  lcd.setCursor(0, 0);
  lcd.print("Register BOSS");
  lcd.setCursor(0, 1);
  lcd.print("Place Finger");
  delay(1500);


  if (finger.loadModel(BOSS_ID) != FINGERPRINT_OK) { 
    while (!enrollFingerprint(BOSS_ID));
    slotOccupied[BOSS_ID] = true;
  } else {
    slotOccupied[BOSS_ID] = true;
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Scan BOSS...");
    lcd.setCursor(0, 1);
    lcd.print("to continue");
    delay(1500);

    bool bossVerified = false;
    while (!bossVerified) {
      int scanResult = getFingerprintID();
      if (scanResult == BOSS_ID) {
        bossVerified = true;
        lcd.clear();
        lcd.print("BOSS Verified");
        lcd.setCursor(0, 1);
        lcd.print("Proceeding...");
        delay(1500);
        lcd.clear();
      } else if (scanResult == -1) {
        lcd.setCursor(0, 0);
        lcd.print("Unknown Finger");
        lcd.setCursor(0, 1);
        lcd.print("Try Again    ");
        delay(1000);
        lcd.setCursor(0, 0);
        lcd.print("Scan BOSS...  ");
        lcd.setCursor(0, 1);
        lcd.print("to continue   ");
      } else if (scanResult > 1) {
        lcd.setCursor(0, 0);
        lcd.print("Not Authorized");
        lcd.setCursor(0, 1);
        lcd.print("BOSS only     ");
        delay(1500);
        lcd.setCursor(0, 0);
        lcd.print("Scan BOSS...  ");
        lcd.setCursor(0, 1);
        lcd.print("to continue   ");
      }
      delay(200);
    }
  }

  isEnrollmentMode = true;

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

    if (result == BOSS_ID) {
      lcd.clear();
      lcd.print("Boss Detected");
      lcd.setCursor(0, 1);
      lcd.print("Entering Enroll");
      delay(2000);
      isEnrollmentMode = true;
      lcd.clear();
    } 
    else if (result > 1) {
      String projectName, pin;
      bool dataFound = readEmployeeData(result, projectName, pin);

      if (!isLoggedIn[result]) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Access Granted");
        lcd.setCursor(0, 1);
        lcd.print("Fingerprint OK");
        delay(2000);
        lcd.clear();

        
        if (dataFound) {
          lcd.setCursor(0, 0);
          lcd.print(projectName.substring(0, 16));
          lcd.setCursor(0, 1);
          lcd.print("PIN: ");
          lcd.print(pin.substring(0, 6));
          delay(4000);
          lcd.clear();

          
          logToGoogleSheets(result, projectName, pin, "LOGIN");
        }

        isLoggedIn[result] = true;
      } else {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Logout Successful");
        lcd.setCursor(0, 1);
        lcd.print("Have a nice day");
        delay(3000);
        lcd.clear();

        if (dataFound) {
          logToGoogleSheets(result, projectName, pin, "LOGOUT");
        }
        finger.deleteModel(result);
        isLoggedIn[result] = false;
        slotOccupied[result] = false;
        addFreeSlot(result);
      }
    } 
    else if (result == -1) {
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
  int enrollID = peekNextFreeSlot();
  if (enrollID == -1) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Max Projects");
    lcd.setCursor(0, 1);
    lcd.print("Reached!");
    delay(2000);
    lcd.clear();
    return;
  }

  lcd.setCursor(0, 0);
  lcd.print("Enroll User: [");
  lcd.print(enrollID);
  lcd.print("]");
  lcd.setCursor(0, 1);
  lcd.print("Place Finger    ");

  int p = finger.getImage();
  if (p == FINGERPRINT_OK) {
    p = finger.image2Tz(1);
    if (p != FINGERPRINT_OK) return;

    p = finger.fingerFastSearch();
    if (p == FINGERPRINT_OK) {
      if (finger.fingerID == BOSS_ID) {
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
        while (finger.getImage() != FINGERPRINT_NOFINGER);
        return;
      }
    }

    lcd.clear();
    if (enrollFingerprint(enrollID)) {
      slotOccupied[enrollID] = true;
      isLoggedIn[enrollID] = false;
      consumeFreeSlot(enrollID);
      activeEmployeeCount++;
    }
    lcd.clear();
  }
}

bool enrollFingerprint(int id) {
  int p = -1;
  
  lcd.setCursor(0, 0);
  if (id == BOSS_ID) lcd.print("Enrolling BOSS  ");
  else { lcd.print("User Slot: "); lcd.print(id); }
  lcd.setCursor(0, 1);
  lcd.print("Scan [1/2]      ");
  
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
  }

  p = finger.image2Tz(1);
  if (p != FINGERPRINT_OK) return false;

  lcd.setCursor(0, 1);
  lcd.print("Lift Finger     ");
  delay(1000);
  while (finger.getImage() != FINGERPRINT_NOFINGER);

  lcd.setCursor(0, 1);
  lcd.print("Scan [2/2]      ");
  while (finger.getImage() != FINGERPRINT_OK);

  p = finger.image2Tz(2);
  if (p != FINGERPRINT_OK) return false;

  p = finger.createModel();
  if (p != FINGERPRINT_OK) {
    lcd.clear();
    lcd.print("Match Error!");
    delay(1500);
    return false;
  }

  p = finger.storeModel(id);
  if (p == FINGERPRINT_OK) {
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

int getFingerprintID() {
  int p = finger.getImage();
  if (p == FINGERPRINT_NOFINGER) return -2;
  if (p != FINGERPRINT_OK)       return -1;

  p = finger.image2Tz();
  if (p != FINGERPRINT_OK)       return -1;

  p = finger.fingerFastSearch();
  if (p != FINGERPRINT_OK)       return -1;

  return finger.fingerID;
}
