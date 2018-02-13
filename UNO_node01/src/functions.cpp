#include <Arduino.h>
#include <avr/sleep.h>
static String nodeID = "node01";

void AVRsleep()
{
        digitalWrite(LED_BUILTIN, 0);
        set_sleep_mode(SLEEP_MODE_IDLE); //sleeps for the rest of this millisecond or less if other trigger
        sleep_enable();
        sleep_mode();     // put the device to sleep
        sleep_disable();
        digitalWrite(LED_BUILTIN, 1);
}

String readAltSerial()
{
        delay(10);
        char c;
        String str = "";
        while (Serial.available() > 0) {
                c = Serial.read();
                //Serial.print(c);
                str += c;
        }
        return str;
}

void mqttpub(String channel, String msg)
{
        Serial.println("PUB,/from/" + nodeID + "/" + channel + "," + msg);
        //altSerial.print("PUB,/from/" + nodeID + "/" + channel + "," + msg);
}

String FloatToString(float value)
{
        char buffer[10];
        String str = dtostrf(value, 5, 2, buffer);
        return str;
}
