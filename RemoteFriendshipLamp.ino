/*
   Remote Friendship Lamp // Client Code
   23.11.2020
   Jürgen Krauß
   @MirUnauffaellig
*/
#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>
#include <math.h>

// Depending on the Board ID, the code decides which WiFi credentials to use
#define STASSID_dth "wifi_ssid_1"
#define STAPSK_dth  "wifi-psk-1"
#define STASSID_ka "wifi_ssid_2"
#define STAPSK_ka "wifi-psk-2"

#define LAMP_PIN   4
#define BUTTON_PIN 5
#define NUMPIXELS 24 // Number of leds on the Neopixel ring

#define K_ID   16047792 // These are my ESP board ids to distinct between the two lamps. 
#define S_ID   6553442
  
String target_url = "https://WEBSERVICE_URL/lamp/api.php?key=AUTH_KEY&identifier=";
String lamp_id ="";

int interval = 60000; // This is the 1-minute interval in which the lamps connect to the webservice
int since_start = 0;
int current_loop_time = 0;
int LEDr = 0;
int LEDg = 0;
int LEDb = 0;

Adafruit_NeoPixel pixels(NUMPIXELS, LAMP_PIN, NEO_GRB + NEO_KHZ800);
ESP8266WiFiMulti WiFiMulti;

void setup() {
  Serial.begin(115200); 
  Serial.print("\n");
  Serial.print("\n");
  Serial.print(" ESP8266 Chip id = ");
  Serial.print(String(ESP.getChipId()));
  Serial.print("\n");

  #if defined(__AVR_ATtiny85__) && (F_CPU == 16000000)  // Not really sure if this is needed ...
    clock_prescale_set(clock_div_1);
  #endif
  
  pixels.begin();
  pixels.clear();
  pinMode(BUTTON_PIN, INPUT);
  
  Serial.println();
  WiFi.disconnect(); // Otherwise it caches and remembers old login data
  delay(500);
  
  WiFi.mode(WIFI_STA);
  
  Serial.println("Using: ");
  Serial.print("  - ");
   
  if (ESP.getChipId() == K_ID) {  // By using the ESP.getId() function, we get an unique identifier for each lamp – 
                              // so the same code can be used on both lamps and let the software decide which WiFi credentials to use.
    WiFiMulti.addAP(STASSID_dth, STAPSK_dth);
    lamp_id = "lamp-1";
    Serial.print(STASSID_dth);
    Serial.print(" | ");
    Serial.print(STAPSK_dth);
  }
  if (ESP.getChipId() == S_ID) { 
    WiFiMulti.addAP(STASSID_ka, STAPSK_ka);
    lamp_id = "lamp-2";
    Serial.print(STASSID_ka);
    Serial.print(" | ");
    Serial.print(STAPSK_ka);
  }
  
   Serial.print("\n");
   Serial.print("  - Lamp ID: ");
   Serial.print(lamp_id);
   Serial.print("\n  - ");
   target_url = target_url + lamp_id;
   Serial.print(target_url + "\n");
   delay(500);
   
   while (WiFiMulti.run() != WL_CONNECTED) {  // Waiting for the WiFi to connect
    doAnimation("setup");
    Serial.println("Waiting for WiFi to connect");
    if (millis() > 60000 * 5) { // After 5 minutes, try the other WiFi credentials ... because probably it wasn't shipped yet
      Serial.println("Could not connect to WiFi for 5 minutes, trying the other WiFi network");
      if (ESP.getChipId() == S_ID) { 
        WiFiMulti.addAP(STASSID_dth, STAPSK_dth);
        Serial.print(STASSID_dth);
        Serial.print(" | ");
        Serial.print(STAPSK_dth);
      }
      if (ESP.getChipId() == K_ID) { 
        WiFiMulti.addAP(STASSID_ka, STAPSK_ka);
        Serial.print(STASSID_ka);
        Serial.print(" | ");
        Serial.print(STAPSK_ka);
      }
      Serial.print("\n");
    }
   }
   doAnimation("ack");
   delay(500);
}

void loop() {
   if (digitalRead(BUTTON_PIN) == 1) { // If the button is pressed, user feedback is given and a change of the lamps' states are requested via the webservice
    Serial.println("Button pressed!");
    doAnimation("ack");
    myHttpRequest("&mode=set-status");
   }
   
  if (current_loop_time == 0 || current_loop_time > interval) { // Do this only AFTER EACH INTERVAL (= 60s)
    Serial.printf("Program runs since: ");
    Serial.print(millis());
    Serial.print("\n");
    Serial.printf("Current loop time: ");
    Serial.print(current_loop_time);
    Serial.printf("\n");
    since_start = millis();
    current_loop_time = 0;

    myHttpRequest("&mode=get-status"); // This is the once-every-60-seconds status check
  }
  
  current_loop_time = millis() - since_start;
  delay(10); // I guess this is not really needed but it doesn't make much sense to continously loop through everything millisecond after millisecond
}

/* ---- ---- ---- ---- ---- ---- ---- ---- 
 FUNCTIONS
---- ---- ---- ---- ---- ---- ---- ---- ---- */

// Function for a smooth color change (led after led)
void changeColor(uint32_t targetColor, int animationSpeed) {
  int delayStep = round(animationSpeed / NUMPIXELS);
  for (int i = 0; i < NUMPIXELS; i++) {
       pixels.setPixelColor(i, targetColor);
       pixels.show();
       if (delayStep > 0) {
        delay(delayStep);
       }
  }
}

// Function to split a string into an array
String getValue(String data, char separator, int index)
{
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length()-1;

  for(int i=0; i<=maxIndex && found<=index; i++){
    if(data.charAt(i)==separator || i==maxIndex){
        found++;
        strIndex[0] = strIndex[1]+1;
        strIndex[1] = (i == maxIndex) ? i+1 : i;
    }
  }
  return found>index ? data.substring(strIndex[0], strIndex[1]) : "0";
}

// Functions for different kinds of animations
void doAnimation(String which) {
  if (which == "setup") {
    Serial.print("Setting up ... 5");
    changeColor(pixels.Color(100,50,0), 1000);
    Serial.print("... 4");
    changeColor(pixels.Color(80,30,0), 1000);
    Serial.print("... 3");
    changeColor(pixels.Color(100,50,0), 1000);
    Serial.print("... 2");
    changeColor(pixels.Color(80,30,0), 1000);
    Serial.print("... 1\n");
    changeColor(pixels.Color(100,50,0), 1000);
  }

  if (which == "error") {
    for (int i = 0; i < 5; i++) {
      Serial.println("ERROR");
      changeColor(pixels.Color(255,0,0), 0);
      delay(500);
      changeColor(pixels.Color(LEDr,LEDg,LEDb), 0);
      delay(500);
    }
  }

  if (which == "warning") {
    for (int i = 0; i < 3; i++) {
      Serial.println("WARNING");
      changeColor(pixels.Color(100,50,0), 1000);
      delay(500);
      changeColor(pixels.Color(LEDr,LEDg,LEDb), 0);
      delay(500);
    }
  }

  if (which == "ack" || which == "ack_off") {
    changeColor(pixels.Color(255,255,255), 0);
    delay(500);
    if (which == "ack_off") {
       LEDr = 0;
       LEDg = 0;
       LEDb = 0;
     }
    changeColor(pixels.Color(LEDr,LEDg,LEDb), 0);
  }
}

// This gets called to connect to the webservice, either for getting or for setting the lamp color
// Here a simple HTTPS web request is being use – but this could be easily changed to support i. e. web sockets or something completely different 
void myHttpRequest(String statusMode) {
   if ((WiFiMulti.run() == WL_CONNECTED)) {
      std::unique_ptr<BearSSL::WiFiClientSecure>client(new BearSSL::WiFiClientSecure);
      client->setInsecure();
      HTTPClient https;
      Serial.print("[HTTPS] begin...\n");
      if (https.begin(*client, target_url + statusMode)) {  
        Serial.print("[HTTPS] GET...");
        Serial.print(target_url + "&mode=get-status");
        Serial.print("\n");
        
        // start connection and send HTTP header
        int httpCode = https.GET();
    
        // httpCode will be negative on error
        if (httpCode > 0) {
          // HTTP header has been send and Server response header has been handled
          Serial.printf("[HTTPS] GET... code: %d\n", httpCode);
    
          // file found at server
          if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
            String payload = https.getString();
            Serial.println(payload);
            if (payload != "") {
                LEDr = getValue(payload, ',', 0).toInt(); 
                LEDg = getValue(payload, ',', 1).toInt();
                LEDb = getValue(payload, ',', 2).toInt();
                Serial.print("R: ");
                Serial.print(LEDr);
                Serial.print(" G: ");
                Serial.print(LEDg);
                Serial.print(" B: ");
                Serial.print(LEDb);
                Serial.print("\n");
                changeColor(pixels.Color(LEDr, LEDg, LEDb), 1000);
              } else {
                doAnimation("off");
              }
            }
          
        } else {
          Serial.printf("[GET] Request... failed, error: %s\n", https.errorToString(httpCode).c_str());
          doAnimation("error");
        }
    
        https.end();
      } else {
        Serial.printf("[HTTPS.begin] Unable to connect\n");
        doAnimation("error");
      }  
   } else {
    Serial.printf("[WiFiMulti.run] failed to connect\n");
    doAnimation("error");
  }  
}