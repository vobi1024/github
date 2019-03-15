#include <Arduino.h>

//#include <SPI.h>
//#include <printf.h>
//#include <stdlib.h>
//#include "LowPower.h"
//#include <AltSoftSerial.h>
//AltSoftSerial altSerial;

//#include <avr/wdt.h>
#include <everytime.h>
#include <ELClient.h>
#include <ELClientCmd.h>
#include <ELClientMqtt.h>
#include <avr/sleep.h>
#include <avr/power.h>

//unsigned long previousMillis = 0;

static String nodeID = "node03";

bool newData = 0;
String topic = "";
String data = "";

void AVRsleep()
{
        //digitalWrite(LED_BUILTIN, 0);
        set_sleep_mode(SLEEP_MODE_IDLE); //sleeps for the rest of this millisecond or less if other trigger
        sleep_enable();
        MCUCR |= (1<<BODS) | (1<<BODSE);
        MCUCR &= ~(1<<BODSE); // must be done right before sleep
        sleep_mode();     // put the device to sleep
        sleep_disable();
        //digitalWrite(LED_BUILTIN, 1);
}

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
        mqtt.subscribe("/to/node01e/#", 1);
        //mqtt.subscribe("/hello/world/#");
        //mqtt.subscribe("/esp-link/2", 1);
        //mqtt.publish("/esp-link/0", "test1");
        connected = true;
        mqtt.publish("/from/node01e/status", "START", 1);
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
        pinMode(6, OUTPUT);
        pinMode(7, OUTPUT);
        power_adc_disable();
        power_spi_disable();
        Serial.begin(115200);
        Serial.println("EL-Client starting!");

        //display.print(".");
        //display.display();

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

        //display.print(".");
        //display.display();

        // Set-up callbacks for events and initialize with es-link.
        mqtt.connectedCb.attach(mqttConnected);
        mqtt.disconnectedCb.attach(mqttDisconnected);
        mqtt.publishedCb.attach(mqttPublished);
        mqtt.dataCb.attach(mqttData);
        mqtt.setup();

        //display.print(".");
        //display.display();

        //Serial.println("ARDUINO: setup mqtt lwt");
        //mqtt.lwt("/lwt", "offline", 0, 0); //or mqtt.lwt("/lwt", "offline");

        Serial.println("EL-MQTT ready");
        //wdt_disable();

        //display.print("Done");
        //display.display();
        //display.clearDisplay();
        //display.setTextSize(4);
        //display.display();
        //while (!Serial) ; // wait for Arduino Serial Monitor to open
        //Serial.println("AltSoftSerial Test Begin");
        //  altSerial.begin(19200);
        //wdt_enable(WDTO_4S);
        //mqttpub("status", "Arduino 01 online");
}

void loop() {
        esp.Process();
        //wdt_reset();
        // unsigned long currentMillis = millis();
        // if ((unsigned long)(currentMillis - previousMillis) >= 42000) {
        //         //Serial.println("Sending reset...");
        //         Serial.println("PUB,/from/" + nodeID + "/status,RESET");
        //         delay(250);
        //         Serial.println("RST,,");
        //         previousMillis = currentMillis;
        // }

        every(10000)
        {

        }

        if (newData) {
                //String rxstr = readAltSerial();
                if (topic.substring(12, 16) == "tmp1")
                {
                        mqtt.publish("/from/node03e/tmp1", dstemp);
                        Serial.print("MQTT published: ");
                        Serial.println(dstemp);
                }
                //
                // if (rxstr.substring(0, 3) == "CMD")
                // {
                //         //previousMillis = currentMillis;
                // }
                newData = 0;
        } //incoming msg end
        AVRsleep();
} //loop