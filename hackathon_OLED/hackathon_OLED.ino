typedef void function;

//Display Imports
#include <SPI.h>
#include <Wire.h>
#include <LOLIN_I2C_BUTTON.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Wifi Imports
#include <espnow.h>
#include <ESP8266WiFi.h>
#include "ESPAsyncWebServer.h"
#include "ESPAsyncTCP.h"
#include <Arduino_JSON.h>
#include <FS.h>
#include "LittleFS.h"

// SCL GPIO5
// SDA GPIO4
#define OLED_RESET 0 // GPIO0
Adafruit_SSD1306 display(OLED_RESET);
I2C_BUTTON displayButton(DEFAULT_I2C_BUTTON_ADDRESS);
String displayKeyString[] = {"None", "Press", "Long", "Double", "Hold"};

const char* ssid = "hack_OLED";
const char* password = "hackathon1";

const char* ssid_online = "Brick";
const char* password_online = "MattAttempt";

//Packet structure
typedef struct struct_message {
  int id;
  float temp;
  float humidity;
  int pressure;
  int readingId;
  String bAction;
} struct_message;
struct_message incomingReadings;

// Values
float pressure;
float temp;
float humidity;
String jsonString; 
int displayMode = 0;
String threatLevelTemp;
String threatLevelHumi;
String threatLevelPressure;

// Init Web Server
JSONVar board;

AsyncWebServer server(80);
AsyncEventSource events("/events");

const char* PARAM_INPUT_1 = "rangeTemp";
const char* PARAM_INPUT_2 = "rangeHum";
const char* PARAM_INPUT_3 = "rangePressure";

// Levels
const String communicationLevelNames[4] = {"energy-saving", "balanced", "performance", "immediate"};
float rangesTemp[4][2] = {{0, 0}, {0, 0}, {0, 0}, {0, 0}};
float rangesHumi[4][2] = {{0, 0}, {0, 0}, {0, 0}, {0, 0}};
float rangesPressure[4][2] = {{0, 0}, {0, 0}, {0, 0}, {0, 0}};
bool encryption;

// File Manipulation
void listDir(const char *dirname) {
  Serial.printf("Listing directory: %s\n", dirname);

  Dir root = LittleFS.openDir(dirname);

  while (root.next()) {
    File file = root.openFile("r");
    Serial.print("  FILE: ");
    Serial.print(root.fileName());
    Serial.print("  SIZE: ");
    Serial.print(file.size());
    time_t cr = file.getCreationTime();
    time_t lw = file.getLastWrite();
    file.close();
    struct tm *tmstruct = localtime(&cr);
    Serial.printf("    CREATION: %d-%02d-%02d %02d:%02d:%02d\n", (tmstruct->tm_year) + 1900, (tmstruct->tm_mon) + 1, tmstruct->tm_mday, tmstruct->tm_hour, tmstruct->tm_min, tmstruct->tm_sec);
    tmstruct = localtime(&lw);
    Serial.printf("  LAST WRITE: %d-%02d-%02d %02d:%02d:%02d\n", (tmstruct->tm_year) + 1900, (tmstruct->tm_mon) + 1, tmstruct->tm_mday, tmstruct->tm_hour, tmstruct->tm_min, tmstruct->tm_sec);
  }
}

void readFile(const char *path) {
  Serial.printf("Reading file: %s\n", path);

  File file = LittleFS.open(path, "r");
  if (!file) {
    Serial.println("Failed to open file for reading");
    return;
  }

  Serial.print("Read from file: ");
  while (file.available()) { 
    for (int i = 0; i < 4; i++) {
      for (int j = 0; j < 2; j ++) {
        rangesTemp[i][j] = file.readString().toFloat();
      }
    }

    for (int i = 0; i < 4; i++) {
      for (int j = 0; j < 2; j ++) {
        rangesHumi[i][j] = file.readString().toFloat();
      }
    }

    for (int i = 0; i < 4; i++) {
      for (int j = 0; j < 2; j ++) {
        rangesPressure[i][j] = file.readString().toFloat();
      }
    }
  }
  file.close();
}

void writeFile() {

  File file = LittleFS.open("/data.txt", "w");
  if (!file) {
    Serial.println("Failed to open file for writing");
    return;
  }
  
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 2; j ++) {
      file.println(rangesTemp[i][j]);
    }
  }

  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 2; j ++) {
     file.println(rangesHumi[i][j]);
    }
  }

  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 2; j ++) {
      file.println(rangesPressure[i][j]);
    }
  }
  delay(2000);  // Make sure the CREATE and LASTWRITE times are different
  file.close();
}

void appendFile(const char *path, const char *message) {
  Serial.printf("Appending to file: %s\n", path);

  File file = LittleFS.open(path, "a");
  if (!file) {
    Serial.println("Failed to open file for appending");
    return;
  }
  if (file.print(message)) {
    Serial.println("Message appended");
  } else {
    Serial.println("Append failed");
  }
  file.close();
}

void renameFile(const char *path1, const char *path2) {
  Serial.printf("Renaming file %s to %s\n", path1, path2);
  if (LittleFS.rename(path1, path2)) {
    Serial.println("File renamed");
  } else {
    Serial.println("Rename failed");
  }
}

void deleteFile(const char *path) {
  Serial.printf("Deleting file: %s\n", path);
  if (LittleFS.remove(path)) {
    Serial.println("File deleted");
  } else {
    Serial.println("Delete failed");
  }
}

void setRanges() {

}

void setThreatLevelTemp(int level) {
  switch(level) {
    case -1:
      threatLevelTemp = "UNDEFINED";
      break;
    case 0:
      threatLevelTemp = "NORMAL";
      break;
    case 1:
      threatLevelTemp = "WARNING";
      break;
    case 2:
      threatLevelTemp = "HAZARD";
      break;
    case 3:
      threatLevelTemp = "CRITICAL";
      break;
  }
}

void setThreatLevelHumi(int level) {
  switch(level) {
    case -1:
      threatLevelHumi = "UNDEFINED";
      break;
    case 0:
      threatLevelHumi = "NORMAL";
      break;
    case 1:
      threatLevelHumi = "WARNING";
      break;
    case 2:
      threatLevelHumi = "HAZARD";
      break;
    case 3:
      threatLevelHumi = "CRITICAL";
      break;
  }
}

void setThreatLevelPressure(int level) {
  switch(level) {
    case -1:
      threatLevelPressure = "UNDEFINED";
      break;
    case 0:
      threatLevelPressure = "NORMAL";
      break;
    case 1:
      threatLevelPressure = "WARNING";
      break;
    case 2:
      threatLevelPressure = "HAZARD";
      break;
    case 3:
      threatLevelPressure = "CRITICAL";
      break;
  }
}

void buttonPressed(String action) {
  Serial.println("BUTTON PRESSED:" + action);
}

void OnDataRecv(uint8_t * mac_addr, uint8_t *incomingData, uint8_t len) { 
   // Copies the sender mac address to a string
  char macStr[18];
  Serial.print("Packet received from: ");
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  Serial.println(macStr);
  memcpy(&incomingReadings, incomingData, sizeof(incomingReadings));

  if(incomingReadings.bAction != "") {
    buttonPressed(incomingReadings.bAction);
  }
  
  if(incomingReadings.id == 1) {
    temp = incomingReadings.temp;
    temp = (temp * 1.8) + 32;
    humidity = incomingReadings.humidity;
  }
  else if(incomingReadings.id == 2) {
    pressure = incomingReadings.pressure;
  }

  assThreat();

  board["id"] = incomingReadings.id;
  board["temperature"] = temp;
  board["humidity"] = humidity;
  board["pressure"] = (pressure / 1000);
  board["readingId"] = String(incomingReadings.readingId);
  board["threatLevelTemp"] = threatLevelTemp;
  board["threatLevelHumi"] = threatLevelHumi;
  board["threatLevelPressure"] = threatLevelPressure;
  jsonString = JSON.stringify(board);
  Serial.print(jsonString);
  events.send(jsonString.c_str(), "new_readings", millis());
  
  Serial.printf("Board ID %u: %u bytes\n", incomingReadings.id, len);
  Serial.printf("t value: %4.2f \n", incomingReadings.temp);
  Serial.printf("h value: %4.2f \n", incomingReadings.humidity);
  Serial.printf("p value: %d \n", incomingReadings.pressure);
  Serial.printf("readingID value: %d \n", incomingReadings.readingId);
  Serial.println();
}

void setup() {
  Serial.begin(115200);

  if(!LittleFS.begin()){
    Serial.println("An Error has occurred while mounting LittleFS");
    return;
  }

  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(ssid, password);
  IPAddress myIP = WiFi.softAPIP();
  Serial.print("\nAP IP address: ");
  Serial.println(myIP);
  Serial.println(WiFi.macAddress());

  readFile("/data.txt");

  // server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
  //   request->send_P(200, "text/html", index_html);
  // });

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
     if (request->hasParam("param")) {
      AsyncWebParameter* param = request->getParam("param");
      Serial.print("Parameter value: ");
      Serial.println(param->value());
      float value = param->value().toFloat();
      rangesTemp[0][0] = value;
      // Now you can use param->value() as needed
    }
    if (request->hasParam("param2")) {
      AsyncWebParameter* param = request->getParam("param2");
      Serial.print("Parameter value: ");
      Serial.println(param->value());
      float value = param->value().toFloat();
      rangesTemp[0][1] = value;
      // Now you can use param->value() as needed
    }
    if (request->hasParam("param3")) {
      AsyncWebParameter* param = request->getParam("param3");
      Serial.print("Parameter value: ");
      Serial.println(param->value());
      float value = param->value().toFloat();
      rangesTemp[1][0] = value;
      // Now you can use param->value() as needed
    }
    if (request->hasParam("param4")) {
      AsyncWebParameter* param = request->getParam("param4");
      Serial.print("Parameter value: ");
      Serial.println(param->value());
      float value = param->value().toFloat();
      rangesTemp[1][1] = value;
      // Now you can use param->value() as needed
    }
    if (request->hasParam("param5")) {
      AsyncWebParameter* param = request->getParam("param5");
      Serial.print("Parameter value: ");
      Serial.println(param->value());
      float value = param->value().toFloat();
      rangesTemp[2][0] = value;
      // Now you can use param->value() as needed
    }
    if (request->hasParam("param6")) {
      AsyncWebParameter* param = request->getParam("param6");
      Serial.print("Parameter value: ");
      Serial.println(param->value());
      float value = param->value().toFloat();
      rangesTemp[2][1] = value;
      // Now you can use param->value() as needed
    }
    if (request->hasParam("param7")) {
      AsyncWebParameter* param = request->getParam("param7");
      Serial.print("Parameter value: ");
      Serial.println(param->value());
      float value = param->value().toFloat();
      rangesTemp[3][0] = value;
      // Now you can use param->value() as needed
    }
    if (request->hasParam("param8")) {
      AsyncWebParameter* param = request->getParam("param8");
      Serial.print("Parameter value: ");
      Serial.println(param->value());
      float value = param->value().toFloat();
      rangesTemp[3][1] = value;
      Serial.print(rangesTemp[3][1]);
      // Now you can use param->value() as needed
    }
    request->send(LittleFS, "/index2.html", String(), false);
  });

  server.on("/1", HTTP_GET, [](AsyncWebServerRequest *request){
     if (request->hasParam("param")) {
      AsyncWebParameter* param = request->getParam("param");
      Serial.print("Parameter value: ");
      Serial.println(param->value());
      float value = param->value().toFloat();
      rangesHumi[0][0] = value;
      // Now you can use param->value() as needed
    }
    if (request->hasParam("param2")) {
      AsyncWebParameter* param = request->getParam("param2");
      Serial.print("Parameter value: ");
      Serial.println(param->value());
      float value = param->value().toFloat();
      rangesHumi[0][1] = value;
      // Now you can use param->value() as needed
    }
    if (request->hasParam("param3")) {
      AsyncWebParameter* param = request->getParam("param3");
      Serial.print("Parameter value: ");
      Serial.println(param->value());
      float value = param->value().toFloat();
      rangesHumi[1][0] = value;
      // Now you can use param->value() as needed
    }
    if (request->hasParam("param4")) {
      AsyncWebParameter* param = request->getParam("param4");
      Serial.print("Parameter value: ");
      Serial.println(param->value());
      float value = param->value().toFloat();
      rangesHumi[1][1] = value;
      // Now you can use param->value() as needed
    }
    if (request->hasParam("param5")) {
      AsyncWebParameter* param = request->getParam("param5");
      Serial.print("Parameter value: ");
      Serial.println(param->value());
      float value = param->value().toFloat();
      rangesHumi[2][0] = value;
      // Now you can use param->value() as needed
    }
    if (request->hasParam("param6")) {
      AsyncWebParameter* param = request->getParam("param6");
      Serial.print("Parameter value: ");
      Serial.println(param->value());
      float value = param->value().toFloat();
      rangesHumi[2][1] = value;
      // Now you can use param->value() as needed
    }
    if (request->hasParam("param7")) {
      AsyncWebParameter* param = request->getParam("param7");
      Serial.print("Parameter value: ");
      Serial.println(param->value());
      float value = param->value().toFloat();
      rangesHumi[3][0] = value;
      // Now you can use param->value() as needed
    }
    if (request->hasParam("param8")) {
      AsyncWebParameter* param = request->getParam("param8");
      Serial.print("Parameter value: ");
      Serial.println(param->value());
      float value = param->value().toFloat();
      rangesHumi[3][1] = value;
      // Now you can use param->value() as needed
    }
    request->send(LittleFS, "/index2.html", String(), false);
  });

  server.on("/2", HTTP_GET, [](AsyncWebServerRequest *request){
     if (request->hasParam("param")) {
      AsyncWebParameter* param = request->getParam("param");
      Serial.print("Parameter value: ");
      Serial.println(param->value());
      float value = param->value().toFloat();
      rangesPressure[0][0] = value;
      // Now you can use param->value() as needed
    }
    if (request->hasParam("param2")) {
      AsyncWebParameter* param = request->getParam("param2");
      Serial.print("Parameter value: ");
      Serial.println(param->value());
      float value = param->value().toFloat();
      rangesPressure[0][1] = value;
      // Now you can use param->value() as needed
    }
    if (request->hasParam("param3")) {
      AsyncWebParameter* param = request->getParam("param3");
      Serial.print("Parameter value: ");
      Serial.println(param->value());
      float value = param->value().toFloat();
      rangesPressure[1][0] = value;
      // Now you can use param->value() as needed
    }
    if (request->hasParam("param4")) {
      AsyncWebParameter* param = request->getParam("param4");
      Serial.print("Parameter value: ");
      Serial.println(param->value());
      float value = param->value().toFloat();
      rangesPressure[1][1] = value;
      // Now you can use param->value() as needed
    }
    if (request->hasParam("param5")) {
      AsyncWebParameter* param = request->getParam("param5");
      Serial.print("Parameter value: ");
      Serial.println(param->value());
      float value = param->value().toFloat();
      rangesPressure[2][0] = value;
      // Now you can use param->value() as needed
    }
    if (request->hasParam("param6")) {
      AsyncWebParameter* param = request->getParam("param6");
      Serial.print("Parameter value: ");
      Serial.println(param->value());
      float value = param->value().toFloat();
      rangesPressure[2][1] = value;
      // Now you can use param->value() as needed
    }
    if (request->hasParam("param7")) {
      AsyncWebParameter* param = request->getParam("param7");
      Serial.print("Parameter value: ");
      Serial.println(param->value());
      float value = param->value().toFloat();
      rangesPressure[3][0] = value;
      // Now you can use param->value() as needed
    }
    if (request->hasParam("param8")) {
      AsyncWebParameter* param = request->getParam("param8");
      Serial.print("Parameter value: ");
      Serial.println(param->value());
      float value = param->value().toFloat();
      rangesPressure[3][1] = value;
      // Now you can use param->value() as needed
    }
    request->send(LittleFS, "/index2.html", String(), false);
  });


  // server.on("/button", HTTP_GET, [](AsyncWebServerRequest *request){
  //   Serial.println("AOISDJUIOAWHJODIwajhd");
  // });

  server.addHandler(&events);
  server.begin();

  // Init ESP-NOW
  if (esp_now_init() != 0) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  esp_now_register_recv_cb(OnDataRecv);
  
  // Start display
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();

  int chan;
  if (int32_t n = WiFi.scanNetworks()) {
    for (uint8_t i=0; i<n; i++) {
        if (!strcmp(ssid, WiFi.SSID(i).c_str())) {
            chan = WiFi.channel(i); } } }

  wifi_set_channel(chan);
}

void assThreat() {
  if (temp > rangesTemp[0][0] && temp <= rangesTemp[0][1]) {
    setThreatLevelTemp(0);
  }
  else if (temp > rangesTemp[1][0] && temp <= rangesTemp[1][1]) {
    setThreatLevelTemp(1);
  }
  else if (temp > rangesTemp[2][0] && temp <= rangesTemp[2][1]) {
    setThreatLevelTemp(2);
  }
  else if (temp > rangesTemp[3][0] && temp <= rangesTemp[3][1]) {
    setThreatLevelTemp(3);
  }
  else {
    setThreatLevelTemp(-1);
  }
  if (humidity > rangesHumi[0][0] && humidity <= rangesHumi[0][1]) {
    setThreatLevelHumi(0);
  }
  else if (humidity > rangesHumi[1][0] && humidity <= rangesHumi[1][1]) {
    setThreatLevelHumi(1);
  }
  else if (humidity > rangesHumi[2][0] && humidity <= rangesHumi[2][1]) {
    setThreatLevelHumi(2);
  }
  else if (humidity > rangesHumi[3][0] && humidity <= rangesHumi[3][1]) {
    setThreatLevelHumi(3);
  }
  else {
    setThreatLevelHumi(-1);
  }
  if (pressure > rangesPressure[0][0] && pressure <= rangesPressure[0][1]) {
    setThreatLevelPressure(0);
  }
  else if (pressure > rangesPressure[1][0] && pressure <= rangesPressure[1][1]) {
    setThreatLevelPressure(1);
  }
  else if (pressure > rangesPressure[2][0] && pressure <= rangesPressure[2][1]) {
    setThreatLevelPressure(2);
  }
  else if (pressure > rangesPressure[3][0] && pressure <= rangesPressure[3][1]) {
    setThreatLevelPressure(3);
  }
  else {
    setThreatLevelPressure(-1);
  }
}

void statsDisplay() {
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.setTextColor(WHITE);
  display.clearDisplay();
  display.print("Hot: ");
  display.println(temp);
  display.print("Wet: ");
  display.println(humidity);
  display.print("kPa: ");
  display.println(pressure / 1000);
  if(incomingReadings.bAction != "") {
    display.print("B: " + incomingReadings.bAction);
  }
  display.display();
}

void threatLevelDisplay() {
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.setTextColor(WHITE);
  display.clearDisplay();
  display.println("T-Lvls: t-h-p");
  display.println(threatLevelTemp);
  display.println(threatLevelHumi);
  display.println(threatLevelPressure);
  display.display();
}

void encryptionDisplay() {
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.setTextColor(WHITE);
  display.clearDisplay();
  display.print("encryption");
  display.display();
}

void wifiInfoDisplay() {
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.setTextColor(WHITE);
  display.clearDisplay();
  display.print("MAC-");
  display.println(WiFi.macAddress());
  display.print("IP-");
  display.println(WiFi.softAPIP());
  display.display();
}

void loop() {
  assThreat();
  // Button handling
  if (displayButton.get() == 0) {
    if (displayButton.BUTTON_A) {
      if (displayMode != 0) {
        displayMode--;
      } else {
        displayMode = 3;
      }
    }
    if (displayButton.BUTTON_B) {
      if (displayMode != 3) {
        displayMode++;
      } else {
        displayMode = 0;
      }
    }
  }

  // Display handling based on displayMode
  switch (displayMode) {
    case 0:
      statsDisplay();
      break;
    case 1:
      threatLevelDisplay();
      break;
    case 2:
      encryptionDisplay();
      break;
    case 3:
      wifiInfoDisplay();
      break;
  }

  delay(100);
}