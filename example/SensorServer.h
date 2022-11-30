#include <DFRobot_IICSerial.h>
#include <Adafruit_NeoPixel.h>
#include <Adafruit_SGP30.h>
#include <M5_ENV.h>
#include <BH1750.h>
#include <WiFi.h>
#include <Wire.h>
#include <FRAM.h>

#define MAX_GPS_SENTENCE_SIZE 100
#define PIXELPIN 4
#define NUMPIXELS 7
#define FRAM_I2C_ADDRESS 0x50
#define FRAM_MAX 32600
#define FRAM_LASTWRITE_ADDRESS 32710
#define FRAM_REC_SIZE 100
#define RTC_LED 2
#define SENSOR_DATA_LED 0
#define WEBPAGE_DATA_LED 1
#define LOCATION_DATA_LED 2
#define POWER_DATA_LED 3
#define EXPECTED_I2C_COUNT 14
const char *ssid = "ESP32-Access-Point";
const char *password = "123";
static int Dark=0;

float Temperature, Humidity, Pressure, Lux, BatteryLevel = 0.0;
int Tvoc, Eco2 = 0;
int LatDegs, LatMins, LatTrail, LatVar, LonDegs, LonMins, LonTrail, LonVar = 0;
char LatDir, LonDir = 'X';

RTC_TimeTypeDef RTCTime;
RTC_DateTypeDef RTCDate;
