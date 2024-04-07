#include "avdweb_Switch.h"
#include <espnow.h>
#include <ESP8266WiFi.h>

#define BOARD_ID 3
uint8_t broadcastAddress[] = {0x4C, 0x11, 0xAE, 0x0D, 0xD5, 0x92};

typedef struct struct_message {
  int id;
  float temp;
  float humidity;
  int pressure;
  int readingId;
  String bAction;
} struct_message;
struct_message myData;

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


const int buttonPin = D3; // Define the pin connected to the button

Switch button = Switch(buttonPin); // Initialize the button object with the button pin

void setup() {
  Serial.begin(115200); // Initialize serial communication at 9600 baud rate
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

  
  button.poll(); // Refresh the button state
  
  // Check if the button is pressed
  if (button.singleClick()) {
    Serial.println("Single Click"); // Print message to the Serial Monitor
    myData.bAction = "Single";
    esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));
  }
  if (button.doubleClick()) {
    Serial.println("Double Click"); // Print message to the Serial Monitor
    myData.bAction = "Double";
    esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));
  } else if (button.longPress()) {
    Serial.println("Long Click"); // Print message to the Serial Monitor
    myData.bAction = "Long";
    esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));
  }
}
