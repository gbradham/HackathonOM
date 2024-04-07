#include <Wire.h>
#include <Adafruit_SHT31.h>
#include <espnow.h>
#include <ESP8266WiFi.h>
#include <LOLIN_HP303B.h>

LOLIN_HP303B HP303B;

uint8_t broadcastAddress[] = {0x4C, 0x11, 0xAE, 0x0D, 0xD5, 0x92};

#define BOARD_ID 3;

// Define a data structure
typedef struct struct_message {
  int id;
  float temp;
  float humidity;
  int pressure;
} struct_message;

// Create a structured object
struct_message myData;

unsigned long lastTime = 0;  
unsigned long timerDelay = 2000;

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
 
  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  // Init ESP-NOW
  if (esp_now_init() != 0) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Once ESPNow is successfully Init, we will register for Send CB to
  // get the status of Trasnmitted packet
  esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);
  esp_now_register_send_cb(OnDataSent);
  
  // Register peer
  esp_now_add_peer(broadcastAddress, ESP_NOW_ROLE_SLAVE, 1, NULL, 0);

  //Address of the HP303B (0x77 or 0x76)
  while (!Serial);
  HP303B.begin();
}

void loop() {
  if ((millis() - lastTime) > timerDelay) {
    // Read pressure
    int temperature;
    int pressure;
    int oversampling = 7;
    int ret;
    Serial.println();

    //lets the HP303B perform a Single temperature measurement with the last (or standard) configuration
    //The result will be written to the paramerter temperature
    //ret = HP303B.measureTempOnce(temperature);
    //the commented line below does exactly the same as the one above, but you can also config the precision
    //oversampling can be a value from 0 to 7
    //the HP303B will perform 2^oversampling internal temperature measurements and combine them to one result with higher precision
    //measurements with higher precision take more time, consult datasheet for more information
    ret = HP303B.measureTempOnce(temperature, oversampling);

    if (ret != 0)
    {
        //Something went wrong.
        //Look at the library code for more information about return codes
        Serial.print("FAIL! ret = ");
        Serial.println(ret);
    }
    else
    {
        Serial.print("Temperature: ");
        Serial.print(temperature);
        Serial.println(" degrees of Celsius");
    }

    //Pressure measurement behaves like temperature measurement
    //ret = HP303B.measurePressureOnce(pressure);
    ret = HP303B.measurePressureOnce(pressure, oversampling);
    if (ret != 0)
    {
        //Something went wrong.
        //Look at the library code for more information about return codes
        Serial.print("FAIL! ret = ");
        Serial.println(ret);
    }
    else
    {
        Serial.print("Pressure: ");
        Serial.print(pressure);
        Serial.println(" Pascal");
        myData.pressure = pressure;
    }

    myData.id = BOARD_ID;

    // Send message via ESP-NOW
    esp_now_send(0, (uint8_t *) &myData, sizeof(myData));

    lastTime = millis();
  }
}