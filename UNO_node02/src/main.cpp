#include <Arduino.h>
//#include <SPI.h>
//#include <printf.h>
//#include <stdlib.h>
//#include "LowPower.h"
//#include <AltSoftSerial.h>
//AltSoftSerial altSerial;

#include <avr/sleep.h>
void AVRsleep()
{
        digitalWrite(LED_BUILTIN, 0);
        set_sleep_mode(SLEEP_MODE_IDLE); //sleeps for the rest of this millisecond or less if other trigger
        sleep_enable();
        sleep_mode();     // put the device to sleep
        sleep_disable();
        digitalWrite(LED_BUILTIN, 1);
}

#include "DHT.h"
DHT dht;
#include <Wire.h>
//#include <avr/wdt.h>
#include <everytime.h>

#include <ELClient.h>
#include <ELClientCmd.h>
#include <ELClientMqtt.h>

float temperature;
float humidity;
String temp = "n/a";
String hum = "n/a";

unsigned long previousMillis = 0;

static String nodeID = "node02";

extern String readAltSerial();
extern void mqttpub(String channel, String msg);
extern String FloatToString(float value);

bool newData = 0;
String topic = "";
String data = "";
char tempa[18];
char huma[18];

String FloatToString(float value)
{
        char buffer[10];
        String str = dtostrf(value, 5, 2, buffer);
        return str;
}

// Initialize a connection to esp-link using the normal hardware serial port both for
// SLIP and for debug messages.
ELClient esp(&Serial, &Serial);

// Initialize CMD client (for GetTime)
ELClientCmd cmd(&esp);

// Initialize the MQTT client
ELClientMqtt mqtt(&esp);

// Callback made from esp-link to notify of wifi status changes
// Here we just print something out for grins
void wifiCb(void* response) {
        ELClientResponse *res = (ELClientResponse*)response;
        if (res->argc() == 1) {
                uint8_t status;
                res->popArg(&status, 1);

                if(status == STATION_GOT_IP) {
                        Serial.println("WIFI CONNECTED");
                } else {
                        Serial.print("WIFI NOT READY: ");
                        Serial.println(status);
                }
        }
}

bool connected;

// Callback when MQTT is connected
void mqttConnected(void* response) {
        Serial.println("MQTT connected!");
        mqtt.subscribe("/to/node02e/#");
        //mqtt.subscribe("/hello/world/#");
        //mqtt.subscribe("/esp-link/2", 1);
        //mqtt.publish("/esp-link/0", "test1");
        connected = true;
        mqtt.publish("/from/node02e/status", "START", 1);
}

// Callback when MQTT is disconnected
void mqttDisconnected(void* response) {
        Serial.println("MQTT disconnected");
        connected = false;
}

// Callback when an MQTT message arrives for one of our subscriptions
void mqttData(void* response) {
        ELClientResponse *res = (ELClientResponse *)response;

        Serial.print("Received: topic=");
        topic = res->popString();
        Serial.println(topic);

        Serial.print("data=");
        data = res->popString();
        Serial.println(data);
        newData = 1;
}

void mqttPublished(void* response) {
        Serial.println("MQTT published");
}

void setup() {
        //wdt_disable();
        Serial.begin(115200);
        Serial.println("EL-Client starting!");

        // Sync-up with esp-link, this is required at the start of any sketch and initializes the
        // callbacks to the wifi status change callback. The callback gets called with the initial
        // status right after Sync() below completes.
        esp.wifiCb.attach(wifiCb); // wifi status change callback, optional (delete if not desired)
        bool ok;
        do {
                ok = esp.Sync(); // sync up with esp-link, blocks for up to 2 seconds
                if (!ok) Serial.println("EL-Client sync failed!");
        } while(!ok);
        Serial.println("EL-Client synced!");

        // Set-up callbacks for events and initialize with es-link.
        mqtt.connectedCb.attach(mqttConnected);
        mqtt.disconnectedCb.attach(mqttDisconnected);
        mqtt.publishedCb.attach(mqttPublished);
        mqtt.dataCb.attach(mqttData);
        mqtt.setup();

        //Serial.println("ARDUINO: setup mqtt lwt");
        //mqtt.lwt("/lwt", "offline", 0, 0); //or mqtt.lwt("/lwt", "offline");

        Serial.println("EL-MQTT ready");
        //wdt_disable();
        pinMode(LED_BUILTIN, OUTPUT);
        dht.setup(4);
}

void loop() {
        esp.Process();
        //wdt_reset();
        // unsigned long currentMillis = millis();
        // if ((unsigned long)(currentMillis - previousMillis) >= 122000) {
        //         //Serial.println("Sending reset...");
        //         Serial.println("PUB,/from/" + nodeID + "/status,RESET");
        //         delay(250);
        //         Serial.println("RST,,");
        //         previousMillis = currentMillis;
        // }

        every(15000) {
                //digitalWrite(LED_BUILTIN, 1);
                temp = FloatToString (dht.getTemperature());
                temp.toCharArray(tempa, 18);
                hum = FloatToString (dht.getHumidity());
                hum.toCharArray(huma, 18);
                //digitalWrite(LED_BUILTIN, 0);
        }


        if (newData) {
                //String rxstr = readAltSerial();
                if (topic.substring(12, 16) == "tmp1")
                {
                        mqtt.publish("/from/node02e/tmp1", tempa);
                        Serial.print("MQTT published: ");
                        Serial.println(tempa);
                }
                if (topic.substring(12, 16) == "hum1")
                {
                        mqtt.publish("/from/node02e/hum1", huma);
                        Serial.print("MQTT published: ");
                        Serial.println(huma);
                }
                //
                // if (rxstr.substring(0, 3) == "CMD")
                // {
                //         //previousMillis = currentMillis;
                // }
                newData = 0;
        }

        // if (Serial.available()) {
        //         String rxstr = readAltSerial();
        //
        //         if (rxstr.substring(15, 19) == "tmp1")
        //         {
        //                 mqttpub("tmp1", temp);
        //         }
        //
        //         if (rxstr.substring(15, 19) == "hum1")
        //         {
        //                 mqttpub("hum1", hum);
        //         }
        //
        //         if (rxstr.substring(0, 3) == "CMD")
        //         {
        //                 previousMillis = currentMillis;
        //         }
        //
        // } // Serial available end
        AVRsleep();
} //loop
