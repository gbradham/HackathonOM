// ESPNOW Imports
#include <espnow.h>
#include <ESP8266WiFi.h>

// Temperature sensor imports/setup
#include <Wire.h>
#include <Adafruit_SHT31.h>
Adafruit_SHT31 sht31 = Adafruit_SHT31();

// This board's ID and main board address to send packets
#define BOARD_ID 1
uint8_t broadcastAddress[] = {0x4C, 0x11, 0xAE, 0x0D, 0xD5, 0x92};

// Data structure
typedef struct struct_message {
  int id;
  float temp;
  float humidity;
  int pressure;
  int readingId;
  String bAction;
} struct_message;
struct_message myData;

// Time between sent packets
unsigned long previousMillis = 0;
const long interval = 2000; 

// When last packet was sent
unsigned int readingId = 0;

// Wifi connection to server
constexpr char WIFI_SSID[] = "hack_OLED";
int32_t getWiFiChannel(const char *ssid) {
  if (int32_t n = WiFi.scanNetworks()) {
    for (uint8_t i=0; i<n; i++) {
      if (!strcmp(ssid, WiFi.SSID(i).c_str())) {
        return WiFi.channel(i);
      }
    }
  }
  return 0;
}

// Callback function executed when data is received
void OnDataSent(uint8_t *mac_addr, uint8_t sendStatus) {
  Serial.print("Last Packet Send Status: ");
  if (sendStatus == 0){
    Serial.println("Delivery success");
  }
  else{
    Serial.println("Delivery fail");
  }
}

void setup() {
  // Init Serial Monitor
  Serial.begin(115200);

  if (!sht31.begin(0x45)) {   // Set to 0x45 for alternate i2c address
    Serial.println("Couldn't find SHT31");
    while (1) delay(1);
  }
 
  // Set device as a wifi station
  WiFi.mode(WIFI_STA);

  // Setup wifi channel
  int32_t channel = getWiFiChannel(WIFI_SSID);
  WiFi.printDiag(Serial);
  wifi_promiscuous_enable(1);
  wifi_set_channel(channel);
  wifi_promiscuous_enable(0);
  WiFi.printDiag(Serial);

  // Init ESPNOW
  if (esp_now_init() != 0) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // get the status of trasnmitted packet
  esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);
  esp_now_register_send_cb(OnDataSent);
  
  // Register peer
  esp_now_add_peer(broadcastAddress, ESP_NOW_ROLE_SLAVE, 1, NULL, 0);
}

void loop() {
  unsigned long currentMillis = millis();
  if ((currentMillis - previousMillis) > interval) {
    previousMillis = currentMillis;

    // Read temperature
    float temp = sht31.readTemperature();
  
    // Read humidity
    float humidity = sht31.readHumidity();

    // Print temperature and humidity
    Serial.print("Temperature: ");
    Serial.print(temp);
    Serial.println(" °C");    
    Serial.print("Humidity: ");
    Serial.print(humidity);
    Serial.println(" %");

    // Set data to send in packet
    myData.id = BOARD_ID;
    myData.temp = temp;
    myData.humidity = humidity;

    // Send message via ESP-NOW
    esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));
    Serial.print("loop");
  }
}