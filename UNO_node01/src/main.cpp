#include <Arduino.h>

//#include <SPI.h>
//#include <printf.h>
//#include <stdlib.h>
//#include "LowPower.h"
//#include <AltSoftSerial.h>
//AltSoftSerial altSerial;

#include <OneWire.h>
#include <DallasTemperature.h>
#define ONE_WIRE_BUS 2

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
DeviceAddress tempDeviceAddress;

//#include <avr/wdt.h>
#include <everytime.h>
#include <ELClient.h>
#include <ELClientCmd.h>
#include <ELClientMqtt.h>
#include <avr/sleep.h>
#include <avr/power.h>

#include <Adafruit_SSD1306.h>
#define OLED_RESET 4
Adafruit_SSD1306 display(OLED_RESET);

//unsigned long previousMillis = 0;

static String nodeID = "node01";

char dstemp[18] = "n/a";

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

String readDS()
{
        digitalWrite(LED_BUILTIN, 1);
        sensors.requestTemperatures();
        float tempC = 0;
        //Serial.println(sensors.getTempCByIndex(0));
        tempC = sensors.getTempC(tempDeviceAddress);
        //Serial.println(tempC, 2);
        char buffer[10];
        String str = dtostrf(tempC, 5, 2, buffer);
        if ((tempC > 60) or (tempC < 0)) str = "n/a";
        digitalWrite(LED_BUILTIN, 0);
        return str;
        //tem = ("Node01,tmp1,0," + tem + "\n");
        //tem.toCharArray(temper, 32);
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
        display.begin(SSD1306_SWITCHCAPVCC, 0x3C); // initialize with the I2C addr 0x3D (for the 128x64)
        display.clearDisplay();
        display.setTextSize(1);
        display.setTextColor(WHITE);
        display.setCursor(0, 0);
        display.print("Initializing");
        display.display();
        //wdt_disable();
        power_adc_disable();
        power_spi_disable();
        Serial.begin(115200);
        Serial.println("EL-Client starting!");

        display.print(".");
        display.display();

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

        display.print(".");
        display.display();

        // Set-up callbacks for events and initialize with es-link.
        mqtt.connectedCb.attach(mqttConnected);
        mqtt.disconnectedCb.attach(mqttDisconnected);
        mqtt.publishedCb.attach(mqttPublished);
        mqtt.dataCb.attach(mqttData);
        mqtt.setup();

        display.print(".");
        display.display();

        //Serial.println("ARDUINO: setup mqtt lwt");
        //mqtt.lwt("/lwt", "offline", 0, 0); //or mqtt.lwt("/lwt", "offline");

        Serial.println("EL-MQTT ready");
        //wdt_disable();
        pinMode(LED_BUILTIN, OUTPUT);
        sensors.begin();
        sensors.getAddress(tempDeviceAddress, 0);
        sensors.setResolution(tempDeviceAddress, 10);
        sensors.setWaitForConversion(false);

        display.print("Done");
        display.display();
        //display.clearDisplay();
        display.setTextSize(4);
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
                readDS().toCharArray(dstemp, 18);
                display.clearDisplay();
                display.setCursor(0, 0);
                display.print(dstemp);
                display.display();
                //Serial.print("dstemp: ");
                //Serial.println(dstemp);
        }

        if (newData) {
                //String rxstr = readAltSerial();
                if (topic.substring(12, 16) == "tmp1")
                {
                        mqtt.publish("/from/node01e/tmp1", dstemp);
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
