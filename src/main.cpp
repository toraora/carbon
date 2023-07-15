#include <Arduino.h>
#include <math.h>
#include <TFT_eSPI.h>
#include <HTTPClient.h>
#include <WiFi.h>

#include "config.h"
#include "sntp.h"
#include "senseair.h"
#include "time.h"

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite img = TFT_eSprite(&tft);
HTTPClient client;

uint32_t getBatteryMilliVolts();
double getBatteryPercentage();
void sendCarbon(int value);

void setup()
{
    // enable battery operation
    pinMode(PIN_POWER_ON, OUTPUT);
    digitalWrite(PIN_POWER_ON, HIGH);
    Serial.begin(115200);

    // enable wifi
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    // enable ntp
    sntp_servermode_dhcp(1); // (optional)
    configTime(GMT_OFFSET_SEC, DAY_LIGHT_OFFSET_SEC, NTP_SERVER1, NTP_SERVER2);
    setenv("TZ", "PST8PDT,M3.2.0,M11.1.0", 1); // set time zone
    tzset();

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
    unsigned long startTime = millis();
    img.fillRect(0, 0, 320, 170, TFT_BLACK);
    if (count % 2 == 0)
    {
        img.fillCircle(50, 50, 20, TFT_RED);
    }
    else
    {
        img.fillCircle(50, 50, 20, TFT_BLUE);
    }

    bool connected = WiFi.status() == WL_CONNECTED;
    char wifi_buf[30];
    sprintf(wifi_buf, "Wifi: %s      ", connected ? "connected" : "disconnected");
    img.setFreeFont(&FreeSans9pt7b);
    img.drawString(wifi_buf, 20, 135);
    img.setFreeFont(&FreeSansBold18pt7b);

    if (!connected)
    {
        WiFi.disconnect();
        WiFi.reconnect();
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

    sendCarbon(co2_value);

    img.pushSprite(0, 0);
    count++;

    unsigned long elapsedTime = millis() - startTime;
    delay(max(0UL, 1000 * INTERVAL - elapsedTime));
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

void sendCarbon(int value)
{
    struct timeval tv_now;
    gettimeofday(&tv_now, NULL);
    String body = String("[") + "{\"name\":\"carbon\",\"interval\":" + INTERVAL + ",\"value\":" + value + ",\"mtype\":\"gauge\",\"time\":" + tv_now.tv_sec + "}]";
    client.setTimeout(3000);
    client.setConnectTimeout(3000);
    client.begin(GRAFANA_URL);
    client.setAuthorization(GRAFANA_USER, GRAFANA_KEY);
    client.addHeader("Content-Type", "application/json");

    int code = client.POST(body);
    Serial.printf("Graphite response code: %i\n", code);
    client.end();
}