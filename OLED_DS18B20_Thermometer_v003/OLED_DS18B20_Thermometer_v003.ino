#include "LowPower.h"

#include <OneWire.h>
#include <DallasTemperature.h>
#define ONE_WIRE_BUS 12
#define TEMPERATURE_PRECISION 10 // Lower resolution
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

#include <Adafruit_SSD1306.h>
#define OLED_RESET 4
Adafruit_SSD1306 display(OLED_RESET);

int numberOfDevices; // Number of temperature devices found
DeviceAddress tempDeviceAddress;
uint8_t timer;
float tempC = 0;
float tempCprev = 0;

void setup() {

  pinMode(LED_BUILTIN, OUTPUT);

  // put your setup code here, to run once:
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3D (for the 128x64)
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.print("Initializing");
  display.display();
  sensors.begin();
  display.print(".");
  display.display();
  numberOfDevices = sensors.getDeviceCount();
  display.print(".");
  display.display();
  sensors.getAddress(tempDeviceAddress, 0);
  display.println("."); display.println();
  display.display();
  display.print("NumDevices: ");
  display.display();
  display.println(numberOfDevices, DEC);
  display.display();
  display.print("Parasite power is:");
  display.display();
  if (sensors.isParasitePowerMode()) display.println("ON");
  else display.println("OFF");
  display.display();
  display.print("Resolution set to: ");
  display.display();
  sensors.setResolution(tempDeviceAddress, TEMPERATURE_PRECISION);
  display.println(sensors.getResolution(tempDeviceAddress), DEC);
  display.display();
  printAddress(tempDeviceAddress); display.println(); display.println();
  display.display();
  // Clear the buffer.
  display.print("Requesting...");
  display.display();
  sensors.requestTemperatures(); // Send the command to get temperatures
  display.println("Done!");
  display.display();
  sensors.setWaitForConversion(false);
  delay(1000);
  display.clearDisplay();
}

void loop() {
  {
    digitalWrite(LED_BUILTIN, 0);
    LowPower.powerDown(SLEEP_1S, ADC_OFF, BOD_OFF);  
    digitalWrite(LED_BUILTIN, 1);
  }

  {
    sensors.requestTemperatures();
    tempCprev = tempC;
    display.clearDisplay();
    display.setCursor(0, 0);
    display.setTextSize(2);
    tempC = sensors.getTempC(tempDeviceAddress);
    display.print(tempC, 2);
    display.print(" "); display.print((char)247); display.print("C");
    display.display();

    display.setTextSize(1);
    display.setCursor(0, 20);

    if (tempC == tempCprev)
    {
      timer++;
      display.setCursor(0, 20);
      display.print("    ");
      display.setCursor(0, 20);
      display.print(timer);
      display.display();
    } else
    {
      timer = 0;
    }
  }
}
// function to print a device address
void printAddress(DeviceAddress deviceAddress)
{
  for (uint8_t i = 0; i < 8; i++)
  {
    if (deviceAddress[i] < 16) display.print("0");
    display.print(deviceAddress[i], HEX);
  }
}
