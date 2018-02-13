#include <Arduino.h>
#include <SPI.h>
#include <avr/wdt.h>

#include "DHT.h"
DHT dht;

//#include <printf.h>
#include <Wire.h>
#include <Adafruit_PN532.h>
#include <everytime.h>
#include <EEPROM.h>

static String nodeID = "node00";

unsigned long previousMillis = 0;

#define PN532_IRQ   (2)
#define PN532_RESET (3)  // Not connected by default on the NFC Shield

Adafruit_PN532 nfc(PN532_IRQ, PN532_RESET);

const uint8_t maxKeyLength = 4;
uint8_t validKeys[][maxKeyLength] = {
        {
                0x36, 0xE6, 0x1D, 0x9E
        }
        ,
        {
                0x06, 0x45, 0x12, 0x9E
        }
        ,
        {
                0x56, 0x28, 0x17, 0x9E
        }
};

int keyCount = sizeof validKeys / maxKeyLength;
bool lockStatus;
String User = "Owner";

float temperature;
float humidity;
String temp = "n/a";
String hum = "n/a";
String armstate = "n/a";

//**------------functions begin--------------**//

void mqttpub(String channel, String msg)
{
  delay(25);
        Serial1.println("PUB,/from/" + nodeID + "/" + channel + "," + msg);
        //altSerial.print("PUB,/from/" + nodeID + "/" + channel + "," + msg);
}

void setLockState(bool state) {
        if (!state)
        {
                mqttpub("arm1", User + " disarmed!");
                // Serial1.print("Node00,arm1,1,0,");
                // Serial1.print(User);
                // Serial1.println(" disarmed!");
                armstate = "Disarmed";
                mqttpub("arms", armstate);
        }
        else
        {
                mqttpub("arm1", User + " armed!");
                // Serial1.print("Node00,arm1,1,1,");
                // Serial1.print(User);
                // Serial1.println(" armed!");
                armstate = "Armed";
                mqttpub("arms", armstate);
        }
        EEPROM.write(0, state);
        digitalWrite(22, state);
        digitalWrite(24, !state);
        lockStatus = state;
        //mqttpub("arms", armstate);
}

String FloatToString(float value)
{
        char buffer[10];
        String str = dtostrf(value, 5, 2, buffer);
        return str;
}

String readAltSerial()
{
        delay(10);
        char c;
        String str = "";
        while (Serial1.available() > 0) {
                c = Serial1.read();
                //Serial.print(c);
                str += c;
        }
        return str;
}

//**------------functions end--------------**//

void setup(void) {
        wdt_disable();
        pinMode(22, OUTPUT);
        pinMode(24, OUTPUT);
        pinMode(23, INPUT);

        dht.setup(4);
        //Serial.begin(9600);

        Serial1.begin(19200);

        nfc.begin();
        uint32_t versiondata = nfc.getFirmwareVersion();
        if (!versiondata) {
                mqttpub("status", User + " Didn't find PN53x board");
                while (1); // halt
        }

        //mqttpub("status", "PN53x online");

        // configure board to read RFID tags
        nfc.SAMConfig();
        lockStatus = EEPROM.read(0);
        digitalWrite(22, lockStatus);
        digitalWrite(24, !lockStatus);
        setLockState(lockStatus);
        wdt_enable(WDTO_4S);
        //mqttpub("status", "Arduino 00 online");
        //mqttpub("arms", armstate);
}

void loop(void) {
        wdt_reset();
        unsigned long currentMillis = millis();
        if ((unsigned long)(currentMillis - previousMillis) >= 42000) {
                //Serial.println("Sending reset...");
                Serial1.println("PUB,/from/" + nodeID + "/status,RESET");
                delay(250);
                Serial1.println("RST,,");
                previousMillis = currentMillis;
        }
        uint8_t NFCsuccess;
        uint8_t NFCuid[] = {
                0, 0, 0, 0, 0, 0, 0
        };                            // Buffer to store the returned UID
        uint8_t uidLength;                  // Length of the UID (4 or 7 bytes depending on ISO14443A card type)
        User = "Owner";

        every(600) {
                NFCsuccess = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, NFCuid, &uidLength, 200);
        }

        if (NFCsuccess) {
                // Display some basic information about the card
                //Serial.println("Found an ISO14443A card");
                //Serial.print("  UID Length: "); Serial.print(uidLength, DEC); Serial.println(" bytes");
                //nfc.PrintHex(NFCuid, uidLength);

                bool valid = false;

                // Compare this key to the valid once registered here in sketch
                for (int i = 0; i < keyCount && !valid; i++) {
                        for (int j = 0; j < uidLength && !valid; j++) {
                                if (NFCuid[j] != validKeys[i][j]) {
                                        break;
                                }
                                if (j == uidLength - 1) {
                                        switch (i) {
                                        case 0:
                                                User = "Kata";
                                                break;
                                        case 1:
                                                User = "Andras";
                                                break;
                                        }
                                        valid = true;
                                }
                        }
                }
                if (valid) {
                        // Switch lock status
                        setLockState(!lockStatus);
                }

                // Wait for card/tag to leave reader
                //while (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, NFCuid, &uidLength, 200));
        }

        every(1000) {
                if (!lockStatus) digitalWrite(22, !digitalRead(23));
                if (lockStatus) digitalWrite(24, !digitalRead(23));
                if (lockStatus & digitalRead(23)) {
                        // Serial1.println("Node00,mot1,1,1,Movement detected!"); //Movement detected!
                        mqttpub("alm1", "Movement detected!");
                }
        }

        if (Serial1.available()) {
                String rxstr = readAltSerial();
                //Serial.println(rxstr);

                if (rxstr.substring(15, 19) == "tmp1")
                {
                        float temperature = dht.getTemperature();
                        temp = FloatToString (temperature-1);
                        mqttpub("tmp1", temp);
                }

                if (rxstr.substring(15, 19) == "hum1")
                {
                        hum = FloatToString (dht.getHumidity());
                        mqttpub("hum1", hum);
                }

                if (rxstr.substring(15, 19) == "dht1")
                {
                        float temperature = dht.getTemperature();
                        temp = FloatToString (temperature-1);
                        hum = FloatToString (dht.getHumidity());
                        mqttpub("dht1", temp + ";" + hum);
                }

                if (rxstr.substring(15, 19) == "arms")
                {
                        setLockState(!lockStatus);
                        mqttpub("arms", armstate);
                }

                if (rxstr.substring(15, 19) == "arma")
                {
                        mqttpub("arms", armstate);
                }

                if (rxstr.substring(0, 3) == "CMD")
                {
                        previousMillis = currentMillis;
                }

        }

}
