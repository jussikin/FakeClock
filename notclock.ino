
#include <Adafruit_MQTT.h>
#include <Adafruit_MQTT_Client.h>
#include "config.h"
#include <ESP8266WiFi.h>
#include <TM16XX.h>
#include <TM1638.h>
#include <TM16XXFonts.h>
#include <TM1640.h>
#include <InvertedTM1638.h>
#include <TM1638QYF.h>

TM1638 module(D5, D6, D7,true,0);

WiFiClient client;
char ssid[] = WIFIHOTSPOT;
char pw[] = WIFIKEY;
Adafruit_MQTT_Client mqtt(&client,MQTTSERVER,MQTTPORT);

// Setup a feed called 'display' for subscribing to display changes.
Adafruit_MQTT_Subscribe displays = Adafruit_MQTT_Subscribe(&mqtt, DISPLAYADDRESS);
// Setup a feed called 'leds' for subscribing led changes.
Adafruit_MQTT_Subscribe leds = Adafruit_MQTT_Subscribe(&mqtt, LEDADDRESS);
// Setup feed to publish button presses
Adafruit_MQTT_Publish buttons = Adafruit_MQTT_Publish(&mqtt, BUTTONADDRESS);

void setup_wifi() {
  WiFi.begin(ssid,pw);
}


void setup() {
  int numer = 0;
  module.setDisplayToHexNumber(0x1234ABCD, 0xF0);
  setup_wifi();
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    module.setDisplayToDecNumber(numer,0);
    numer++;
  }
   module.setDisplayToString("CONNECTED");
   mqtt.subscribe(&displays);
   mqtt.subscribe(&leds);
}

void MQTT_connect() {
  int8_t ret;

  // Stop if already connected.
  if (mqtt.connected()) {
    return;
  }

  uint8_t retries = 3;
  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
       mqtt.disconnect();
       delay(5000);  // wait 5 seconds
       retries--;
       if (retries == 0) {
         // basically die and wait for WDT to reset me
         while (1);
       }
  }
}
byte keyslast = 0;
int pingcounter = 0;
void loop() {
   byte keys = module.getButtons();
   MQTT_connect();

    Adafruit_MQTT_Subscribe *subscription;
  while ((subscription = mqtt.readSubscription(200))) {
    if (subscription == &displays) {
      module.setDisplayToString((char*)displays.lastread);
    }
    if (subscription == &leds) {
      String ledss = (char*)leds.lastread;
      for(int i=0; i<ledss.length();i++){
        char c = ledss.charAt(i);
        if(c=='G'){
          module.setLED(TM1638_COLOR_GREEN,i);
        }
        if(c=='R'){
          module.setLED(TM1638_COLOR_RED,i);
        }
         if(c=='O'){
          module.setLED(TM1638_COLOR_NONE,i);
        }
      }
    }
  }
  if(keyslast!=keys){
     buttons.publish(keys);
     keyslast=keys;
  }
  if(pingcounter>100){
    buttons.publish(keys);
    pingcounter=0;
  }
  pingcounter++;
}
