/*
  Arduino IDE recommended settings for lower power:

  Tools -> CPU Frequency -> 80MHz
  Tools -> Flash Frequency -> 40MHz

*/




#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include "time.h"

WebServer server(80);
Preferences preferences;

const char* AP_SSID = "ESP32-WiFi-Setup";
const char* NTP_SERVER = "pool.ntp.org";

// ===== RESET BUTTON =====
// Button between GPIO 33 and GND
// Must be held LOW for 3 seconds during boot
const int RESET_WIFI_PIN = 33;

// ===== CLOCK PINS =====
// GPIO 32 is used as BOTH:
// - analog input to sense clock state
// - output HIGH to start clock exactly at second 00
const int CLOCK_ENABLE_SENSE_PIN = 32;

const int CLOCK_HOUR_PIN   = 26;
const int CLOCK_MINUTE_PIN = 27;

// 0V means reset/off
// ~1.4V means running
// Anything below this is treated as reset/off
const int CLOCK_RESET_THRESHOLD_MV = 700;

// Pulse timing
const int PULSE_HIGH_MS = 50;
const int PULSE_LOW_MS  = 20;

// true  = 24-hour clock: 0-23 hour pulses
// false = 12-hour clock: 1-12 hour pulses
const bool USE_24_HOUR_CLOCK = true;

// Clock setup state
bool clockSetupRunning = false;
bool clockSetRequested = false;
bool timeIsKnown = false;
bool portalMode = false;

unsigned long lastClockCheckTime = 0;
unsigned long lastClockSetupTime = 0;

const unsigned long CLOCK_CHECK_INTERVAL_MS = 250;
const unsigned long CLOCK_RECHECK_DELAY_MS = 5000;

String savedTimezone = "UTC0";

String htmlPage() {
  return R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <title>WiFi Config</title>
  <style>
    body {
      font-family: Arial, sans-serif;
      background: #f7f9fc;
      padding: 40px;
      display: flex;
      flex-direction: column;
      align-items: center;
      color: #333;
    }

    h2 {
      color: #2c3e50;
    }

    form {
      background: white;
      padding: 20px 25px;
      border-radius: 10px;
      box-shadow: 0 2px 8px rgba(0,0,0,0.1);
      width: 100%;
      max-width: 340px;
    }

    label {
      display: block;
      margin-bottom: 6px;
      font-weight: bold;
    }

    input, select {
      width: 100%;
      padding: 10px;
      margin-bottom: 15px;
      border: 1.5px solid #ccc;
      border-radius: 5px;
      box-sizing: border-box;
      font-size: 14px;
    }

    input[type="submit"] {
      background: #3498db;
      color: white;
      border: none;
      cursor: pointer;
      font-weight: bold;
    }

    input[type="submit"]:hover {
      background: #2980b9;
    }
  </style>
</head>
<body>

  <h2>Configure WiFi</h2>

  <form id="wifiForm">
    <label for="ssid">SSID:</label>
    <input type="text" id="ssid" required>

    <label for="password">Password:</label>
    <input type="password" id="password">

    <label for="timezone">Time Zone:</label>
    <select id="timezone" required></select>

    <input type="submit" value="Submit">
  </form>

  <script>
    const timezoneSelect = document.getElementById("timezone");

    for (let offset = -12; offset <= 14; offset++) {
      const option = document.createElement("option");

      const sign = offset >= 0 ? "+" : "-";
      const absOffset = Math.abs(offset);

      option.textContent = "UTC" + sign + String(absOffset).padStart(2, "0") + ":00";

      // POSIX timezone format is reversed:
      // UTC+01:00 becomes UTC-1
      // UTC-05:00 becomes UTC5
      option.value = offset === 0 ? "UTC0" : "UTC" + String(-offset);

      if (offset === 0) {
        option.selected = true;
      }

      timezoneSelect.appendChild(option);
    }

    document.getElementById("wifiForm").addEventListener("submit", function(e) {
      e.preventDefault();

      fetch("/set", {
        method: "POST",
        headers: { "Content-Type": "application/x-www-form-urlencoded" },
        body: "ssid=" + encodeURIComponent(document.getElementById("ssid").value) +
              "&password=" + encodeURIComponent(document.getElementById("password").value) +
              "&timezone=" + encodeURIComponent(document.getElementById("timezone").value)
      })
      .then(response => response.text())
      .then(text => alert(text))
      .catch(err => alert("Error: " + err));
    });
  </script>

</body>
</html>
)rawliteral";
}

void printDivider() {
  Serial.println();
  Serial.println("========================================");
}

void floatClockEnableSensePin() {
  pinMode(CLOCK_ENABLE_SENSE_PIN, INPUT);
  Serial.println("[CLOCK] Enable/sense pin is INPUT / high impedance");
}

void driveClockEnableHigh() {
  pinMode(CLOCK_ENABLE_SENSE_PIN, OUTPUT);
  digitalWrite(CLOCK_ENABLE_SENSE_PIN, HIGH);
  Serial.println("[CLOCK] Enable/sense pin driven HIGH");
}

int readClockSenseMillivolts() {
  floatClockEnableSensePin();
  delay(2);

#if defined(ESP32)
  return analogReadMilliVolts(CLOCK_ENABLE_SENSE_PIN);
#else
  int raw = analogRead(CLOCK_ENABLE_SENSE_PIN);
  return map(raw, 0, 4095, 0, 3300);
#endif
}

bool isClockInResetState() {
  int mv = readClockSenseMillivolts();

  Serial.print("[CLOCK] Sense voltage: ");
  Serial.print(mv);
  Serial.println(" mV");

  return mv < CLOCK_RESET_THRESHOLD_MV;
}

void setupClockPins() {
  pinMode(CLOCK_HOUR_PIN, OUTPUT);
  pinMode(CLOCK_MINUTE_PIN, OUTPUT);

  digitalWrite(CLOCK_HOUR_PIN, LOW);
  digitalWrite(CLOCK_MINUTE_PIN, LOW);

  analogReadResolution(12);
  analogSetPinAttenuation(CLOCK_ENABLE_SENSE_PIN, ADC_11db);

  floatClockEnableSensePin();

  Serial.println("[CLOCK] Clock pins initialized");
  Serial.print("[CLOCK] Enable/sense pin: GPIO ");
  Serial.println(CLOCK_ENABLE_SENSE_PIN);
  Serial.print("[CLOCK] Hour pin: GPIO ");
  Serial.println(CLOCK_HOUR_PIN);
  Serial.print("[CLOCK] Minute pin: GPIO ");
  Serial.println(CLOCK_MINUTE_PIN);
  Serial.println("[CLOCK] Separate off-state sensing wire removed");
}

void pulsePin(int pin, int count, const char* label) {
  Serial.print("[CLOCK] Pulsing ");
  Serial.print(label);
  Serial.print(" pin ");
  Serial.print(count);
  Serial.println(" times");

  for (int i = 0; i < count; i++) {
    digitalWrite(pin, HIGH);
    delay(PULSE_HIGH_MS);

    digitalWrite(pin, LOW);
    delay(PULSE_LOW_MS);

    Serial.print("[CLOCK] ");
    Serial.print(label);
    Serial.print(" pulse ");
    Serial.print(i + 1);
    Serial.print("/");
    Serial.println(count);
  }
}

bool internalTimeLooksValid() {
  time_t now = time(nullptr);
  return now > 1700000000;  // Roughly after 2023
}

void printCurrentLocalTime(const char* prefix) {
  time_t now = time(nullptr);

  struct tm timeInfo;
  localtime_r(&now, &timeInfo);

  char timeString[64];
  strftime(timeString, sizeof(timeString), "%A, %d %B %Y %H:%M:%S", &timeInfo);

  Serial.print(prefix);
  Serial.println(timeString);
}

void waitForWiFiForever(String ssid, String password) {
  printDivider();
  Serial.println("[WIFI] Waiting for WiFi connection forever...");
  Serial.print("[WIFI] SSID: ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.persistent(false);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.println("[WIFI] Starting connection attempt...");
    WiFi.disconnect(false);
    delay(500);
    WiFi.begin(ssid.c_str(), password.c_str());

    unsigned long startTime = millis();

    while (WiFi.status() != WL_CONNECTED && millis() - startTime < 15000) {
      Serial.print(".");
      delay(500);
    }

    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("[WIFI] Connected successfully!");
      Serial.print("[WIFI] IP address: ");
      Serial.println(WiFi.localIP());
      Serial.print("[WIFI] Gateway: ");
      Serial.println(WiFi.gatewayIP());
      Serial.print("[WIFI] Signal strength RSSI: ");
      Serial.print(WiFi.RSSI());
      Serial.println(" dBm");
      return;
    }

    Serial.println("[WIFI] Connection failed. Retrying...");
    delay(2000);
  }
}

void syncTimeUntilSuccess(String ssid, String password, String timezone) {
  printDivider();
  Serial.println("[TIME] Will retry NTP until valid time is received");

  configTzTime(timezone.c_str(), NTP_SERVER);

  while (!internalTimeLooksValid()) {
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("[TIME] WiFi is not connected while waiting for NTP");
      waitForWiFiForever(ssid, password);
      configTzTime(timezone.c_str(), NTP_SERVER);
    }

    Serial.print("[TIME] Waiting for NTP sync");

    struct tm timeInfo;
    bool gotTime = false;

    for (int attempts = 0; attempts < 20; attempts++) {
      if (getLocalTime(&timeInfo, 500)) {
        gotTime = true;
        break;
      }

      Serial.print(".");

      if (WiFi.status() != WL_CONNECTED) {
        Serial.println();
        Serial.println("[TIME] WiFi disconnected during NTP sync");
        break;
      }
    }

    Serial.println();

    if (gotTime && internalTimeLooksValid()) {
      timeIsKnown = true;
      Serial.println("[TIME] Time synchronized successfully");
      printCurrentLocalTime("[TIME] Current local time: ");
      return;
    }

    Serial.println("[TIME] NTP sync failed. Retrying...");
    delay(2000);
  }

  timeIsKnown = true;
  Serial.println("[TIME] Internal time already looks valid");
  printCurrentLocalTime("[TIME] Current local time: ");
}

void disableWiFiAfterTimeSync() {
  printDivider();
  Serial.println("[WIFI] Time is known, disabling WiFi now");
  Serial.println("[WIFI] ESP32 will continue using internal clock");

  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);

  delay(200);

  Serial.println("[WIFI] WiFi disabled");
}

void waitForZeroSecondAndStartClock() {
  Serial.println("[CLOCK] Waiting for real time second 00...");
  Serial.println("[CLOCK] Enable/sense pin will be driven HIGH immediately at second 00");

  int lastSecond = -1;

  while (true) {
    time_t now = time(nullptr);

    struct tm timeInfo;
    localtime_r(&now, &timeInfo);

    if (timeInfo.tm_sec != lastSecond) {
      Serial.print("[CLOCK] Current second: ");
      Serial.println(timeInfo.tm_sec);
      lastSecond = timeInfo.tm_sec;
    }

    if (timeInfo.tm_sec == 0) {
      driveClockEnableHigh();

      char timeString[64];
      strftime(timeString, sizeof(timeString), "%A, %d %B %Y %H:%M:%S", &timeInfo);

      Serial.print("[CLOCK] Clock start pin driven HIGH at exact sync point: ");
      Serial.println(timeString);

      break;
    }

    delay(1);
  }
}

void setClockAfterZeroSecond() {
  if (clockSetupRunning) {
    Serial.println("[CLOCK] Clock setup is already running, ignoring request");
    return;
  }

  if (!timeIsKnown) {
    Serial.println("[CLOCK] Clock setup requested but time is not known yet");
    return;
  }

  clockSetupRunning = true;

  printDivider();
  Serial.println("[CLOCK] Clock setup requested");
  printCurrentLocalTime("[CLOCK] Internal time now: ");

  floatClockEnableSensePin();
  Serial.println("[CLOCK] Enable/sense pin floating before synchronization");

  waitForZeroSecondAndStartClock();

  time_t now = time(nullptr);

  struct tm timeInfo;
  localtime_r(&now, &timeInfo);

  int hour = timeInfo.tm_hour;
  int minute = timeInfo.tm_min;

  int hourPulses;

  if (USE_24_HOUR_CLOCK) {
    hourPulses = hour;
  } else {
    hourPulses = hour % 12;

    if (hourPulses == 0) {
      hourPulses = 12;
    }
  }

  int minutePulses = minute;

  char setTimeString[64];
  strftime(setTimeString, sizeof(setTimeString), "%A, %d %B %Y %H:%M:%S", &timeInfo);

  Serial.print("[CLOCK] Setting clock to: ");
  Serial.println(setTimeString);

  Serial.print("[CLOCK] Hour pulses needed: ");
  Serial.println(hourPulses);

  Serial.print("[CLOCK] Minute pulses needed: ");
  Serial.println(minutePulses);

  pulsePin(CLOCK_HOUR_PIN, hourPulses, "HOUR");
  pulsePin(CLOCK_MINUTE_PIN, minutePulses, "MINUTE");

  Serial.println("[CLOCK] Clock setup finished");

  floatClockEnableSensePin();
  Serial.println("[CLOCK] Enable/sense pin returned to INPUT mode for voltage sensing");

  lastClockSetupTime = millis();
  clockSetupRunning = false;
}

void startPortal() {
  printDivider();
  Serial.println("[PORTAL] Starting WiFi configuration portal...");

  portalMode = true;

  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID);

  IPAddress apIP = WiFi.softAPIP();

  Serial.println("[PORTAL] Access Point started");
  Serial.print("[PORTAL] AP SSID: ");
  Serial.println(AP_SSID);
  Serial.println("[PORTAL] AP password: none / open network");
  Serial.print("[PORTAL] AP IP address: ");
  Serial.println(apIP);
  Serial.println("[PORTAL] Connect to this WiFi and open the IP above");

  server.on("/", []() {
    Serial.println("[HTTP] GET / - Sending WiFi config page");
    server.send(200, "text/html", htmlPage());
  });

  server.on("/set", HTTP_POST, []() {
    Serial.println("[HTTP] POST /set - Received setup data");

    String ssid = server.arg("ssid");
    String password = server.arg("password");
    String timezone = server.arg("timezone");

    Serial.print("[WIFI] Received SSID: ");
    Serial.println(ssid);

    Serial.print("[WIFI] Password length: ");
    Serial.println(password.length());

    Serial.print("[TIME] Received timezone: ");
    Serial.println(timezone);

    if (ssid == "") {
      Serial.println("[ERROR] Empty SSID received");
      server.send(400, "text/plain", "SSID is required");
      return;
    }

    if (timezone == "") {
      Serial.println("[ERROR] Empty timezone received");
      server.send(400, "text/plain", "Timezone is required");
      return;
    }

    Serial.println("[NVS] Saving settings to flash...");

    preferences.begin("wifi", false);
    preferences.putString("ssid", ssid);
    preferences.putString("password", password);
    preferences.putString("timezone", timezone);
    preferences.end();

    Serial.println("[NVS] Settings saved successfully");
    Serial.println("[SYSTEM] Restarting ESP32 in 1 second...");

    server.send(200, "text/plain", "Saved. ESP32 will restart and connect.");

    delay(1000);
    ESP.restart();
  });

  server.onNotFound([]() {
    Serial.print("[HTTP] Redirecting unknown page: ");
    Serial.println(server.uri());

    server.sendHeader("Location", "/", true);
    server.send(302, "text/plain", "");
  });

  server.begin();

  Serial.println("[HTTP] Web server started on port 80");
}

void checkWiFiResetButton() {
  pinMode(RESET_WIFI_PIN, INPUT_PULLUP);

  Serial.print("[RESET] WiFi reset pin: GPIO ");
  Serial.println(RESET_WIFI_PIN);
  Serial.println("[RESET] Hold button LOW for 3 seconds during boot to clear saved WiFi settings");

  delay(100);

  Serial.print("[RESET] Raw reset pin state: ");
  Serial.println(digitalRead(RESET_WIFI_PIN) == LOW ? "LOW" : "HIGH");

  if (digitalRead(RESET_WIFI_PIN) == HIGH) {
    Serial.println("[RESET] Reset button not pressed");
    return;
  }

  Serial.println("[RESET] Reset pin is LOW, checking if it stays LOW for 3 seconds...");

  unsigned long startTime = millis();

  while (millis() - startTime < 3000) {
    if (digitalRead(RESET_WIFI_PIN) == HIGH) {
      Serial.println("[RESET] Reset button released before 3 seconds");
      Serial.println("[RESET] WiFi settings will NOT be cleared");
      return;
    }

    delay(50);
  }

  printDivider();
  Serial.println("[RESET] Reset button held LOW for 3 seconds");
  Serial.println("[RESET] Clearing saved WiFi credentials and timezone...");

  preferences.begin("wifi", false);
  preferences.clear();
  preferences.end();

  Serial.println("[RESET] Saved settings cleared");
  Serial.println("[RESET] Starting configuration portal");

  startPortal();

  while (true) {
    server.handleClient();
    delay(2);
  }
}

void connectSavedWiFiAndGetTime() {
  printDivider();
  Serial.println("[BOOT] Reading saved settings...");

  preferences.begin("wifi", true);
  String ssid = preferences.getString("ssid", "");
  String password = preferences.getString("password", "");
  savedTimezone = preferences.getString("timezone", "UTC0");
  preferences.end();

  if (ssid == "") {
    Serial.println("[BOOT] No saved SSID found");
    Serial.println("[BOOT] Starting configuration portal");
    startPortal();
    return;
  }

  Serial.print("[BOOT] Saved SSID found: ");
  Serial.println(ssid);

  Serial.print("[BOOT] Saved password length: ");
  Serial.println(password.length());

  Serial.print("[BOOT] Saved timezone: ");
  Serial.println(savedTimezone);

  waitForWiFiForever(ssid, password);
  syncTimeUntilSuccess(ssid, password, savedTimezone);
  disableWiFiAfterTimeSync();

  if (isClockInResetState()) {
    Serial.println("[CLOCK] Clock is in reset/off state after time sync");
    clockSetRequested = true;
  } else {
    Serial.println("[CLOCK] Clock appears to already be running after time sync");
  }
}

void checkClockSensePin() {
  if (!timeIsKnown) {
    return;
  }

  if (clockSetupRunning) {
    return;
  }

  if (millis() - lastClockCheckTime < CLOCK_CHECK_INTERVAL_MS) {
    return;
  }

  lastClockCheckTime = millis();

  if (millis() - lastClockSetupTime < CLOCK_RECHECK_DELAY_MS) {
    return;
  }

  if (isClockInResetState()) {
    Serial.println("[CLOCK] Clock reset/off voltage detected");
    clockSetRequested = true;
  }
}

void handleClockSetRequest() {
  if (!clockSetRequested) {
    return;
  }

  clockSetRequested = false;

  if (!timeIsKnown) {
    Serial.println("[CLOCK] Clock setup requested but time is not known");
    return;
  }

  setClockAfterZeroSecond();
}

void setLowPowerRuntimeStart() {
  printDivider();
  Serial.println("[POWER] Setting low-power runtime mode at startup...");

  Serial.print("[POWER] CPU frequency before: ");
  Serial.print(getCpuFrequencyMhz());
  Serial.println(" MHz");

  // Try 80 first. If stable, you can test 40.
  bool ok = setCpuFrequencyMhz(80);

  if (ok) {
    Serial.println("[POWER] CPU frequency changed successfully");
  } else {
    Serial.println("[POWER] CPU frequency change failed");
  }

  Serial.print("[POWER] CPU frequency now: ");
  Serial.print(getCpuFrequencyMhz());
  Serial.println(" MHz");

  Serial.print("[POWER] Running on core: ");
  Serial.println(xPortGetCoreID());

  Serial.println("[POWER] Memory/flash frequency cannot normally be changed safely at runtime in Arduino");
  Serial.println("[POWER] Second core cannot be fully disabled safely from normal Arduino sketch code");
}

void setup() {

  btStop();
  
  Serial.begin(115200);
  delay(1000);

  printDivider();
  Serial.println("[SYSTEM] ESP32 booting...");
  Serial.println("[SYSTEM] Serial Monitor baud rate: 115200");

  setupClockPins();

  checkWiFiResetButton();

  connectSavedWiFiAndGetTime();
}

void loop() {

  if (portalMode) {
    server.handleClient();
    return;
  }

  checkClockSensePin();
  handleClockSetRequest();

  delay(1000);

}