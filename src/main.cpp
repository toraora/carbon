#include <Arduino.h>
#include <math.h>
#include <TFT_eSPI.h>

#include "config.h"
#include "senseair.h"

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite img = TFT_eSprite(&tft);

uint32_t getBatteryMilliVolts();
double getBatteryPercentage();

void setup()
{
    // enable battery operation
    pinMode(PIN_POWER_ON, OUTPUT);
    digitalWrite(PIN_POWER_ON, HIGH);
    Serial.begin(115200);

    // enable display
    tft.init();
    tft.setRotation(3);
    tft.setSwapBytes(true);
    tft.fillScreen(TFT_BLACK);
    img.createSprite(320, 170);
    img.setFreeFont(&FreeSansBold18pt7b);

    // enable co2
    co2_setup();
}

uint32_t count = 0;

void loop()
{
    img.fillRect(0, 0, 320, 170, TFT_BLACK);
    if (count % 2 == 0)
    {
        img.fillCircle(50, 50, 20, TFT_RED);
    }
    else
    {
        img.fillCircle(50, 50, 20, TFT_BLUE);
    }

    /*
    double batteryPercentage = getBatteryPercentage();
    char buf[20];
    if (batteryPercentage > 0)
    {
        sprintf(buf, "Battery: %0.4f", batteryPercentage);
    }
    else
    {
        sprintf(buf, "No battery");
    }

    tft.drawString(buf, 20, 100);
    */

    co2_requestValue();
    char buf[30];
    sprintf(buf, "CO2 (ppm): %u      ", co2_value);
    img.drawString(buf, 20, 100);

    img.pushSprite(0, 0);
    count++;
    delay(1000);
}

uint32_t getBatteryMilliVolts()
{
    return analogReadMilliVolts(4) * 2;
}

double getBatteryPercentage()
{
    uint32_t mv = getBatteryMilliVolts();

    // if we're over 4,300 mV, we're not plugged into a battery
    if (mv > 4300)
    {
        return -1;
    }

    return 123. - 123. / pow(1 + pow((double)mv / 3700., 80), 0.165);
}