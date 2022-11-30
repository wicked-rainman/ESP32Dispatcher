/*
M5Station-bat used as a portable environment server

Devices attached (On an I2C string) are:
1. An I2C to Serial bridge with a GPS hooked up to the serial interface
2. An M5Stack ENV III
3. 32K Fram module
4. M5Stack Lux sensor
5. M5Stack eCO2/TVOC sensor
The internal batteries allow the unit to run standalone for about 15 hours
*/
#include <M5Station.h>
#include "SensorServer.h"
#include "dispatch.h"
IPAddress IP;
WiFiServer server(80);
WiFiClient client;
AXP192 axp;
SHT3X sht30;
QMP6988 qmp6988;
BH1750 LuxSensor;
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, PIXELPIN, NEO_GRB + NEO_KHZ800);
Adafruit_SGP30 sgp;
DFRobot_IICSerial iicSerial1(Wire, SUBUART_CHANNEL_1, 0, 0);
FRAM fram;
//--------------------------------------------------------------------
// Setup.
//--------------------------------------------------------------------
void setup() {
  M5.begin(true, true, true);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.fillScreen(BLACK);

  //Draw battery icon (Screen dimension is 240x185 pixels)
  M5.Lcd.drawLine(2, 115, 54, 115, TFT_WHITE);
  M5.Lcd.drawLine(2, 130, 54, 130, TFT_WHITE);
  M5.Lcd.drawLine(2, 115, 2, 130, TFT_WHITE);
  M5.Lcd.drawLine(54, 115, 54, 130, TFT_WHITE);
  M5.Lcd.fillRect(54, 118, 6, 9, TFT_WHITE);

  //Draw message to say TAI fix lost
  M5.Lcd.fillRect(1, 45, 150, 20, TFT_RED);
  M5.Lcd.setCursor(2, 48);
  M5.Lcd.print("TAI fix lost");

  //Draw message to say GPS fix lost
  M5.Lcd.fillRect(1, 19, 150, 20, TFT_RED);
  M5.Lcd.setCursor(2, 22);
  M5.Lcd.print("GPS fix lost");
  WiFi.softAP(ssid, password);

  IP = WiFi.softAPIP();

  //Draw message to indicate server IP adddress
  M5.Lcd.setCursor(1, 1);
  M5.Lcd.print(IP);
  axp.begin();
  Wire.begin();
  qmp6988.init();
  sgp.begin();
  LuxSensor.begin();
  ScanI2CBus();
  InitI2CBridge();
  UpdateBatteryStatus();
  fram.begin(FRAM_I2C_ADDRESS);
  Serial.printf("#Startup: Fram write start point is %d\n", fram.read32(FRAM_LASTWRITE_ADDRESS));
  M5.Rtc.GetTime(&RTCTime);
  M5.Rtc.GetDate(&RTCDate);
  server.begin();
  AddTask("Update sensor data", UpdateSensorData, 2000, 0);
  AddTask("Update location fix", UpdateLocationData, 5000, 5000);
  AddTask("Align RTC/TAI", UpdateRtc, 300000, 15000);
  AddTask("Show battery status", UpdateBatteryStatus, 60000, 0);
  AddTask("Update screen clock", UpdateClockDisplay, 1000, 0);
}

//--------------------------------------------------------------------
// Main loop
// Call dispatch function to invoke eash function at a
// specified time offset.
//
// Then, display an optional web page (Not much point in this if
// the web page isn't viewed!).
//---------------------------------------------------------------------
void loop() {
  Dispatch(0);
  ServeWebPage();
}
//--------------------------------------------------------------------
// Update the current battery status if it's changed
// If not in dark mode (See web page), draw the battery icon.
//--------------------------------------------------------------------
void UpdateBatteryStatus() {
  static int BatteryCellsScreenPosition[4] = { 5, 17, 29, 41 };
  static int StoredBatteryLevel = 0;

  BatteryLevel = axp.GetBatteryLevel();
  if (BatteryLevel == StoredBatteryLevel) return;
  Serial.printf("#UpdateBatteryStatus: %3.1f%%\n", BatteryLevel);
  if (Dark == 1) return;

  else {
    StoredBatteryLevel = round(BatteryLevel);
    M5.Lcd.setTextSize(2);
    M5.Lcd.setCursor(65, 115);
    M5.Lcd.fillRect(65, 114, 50, 16, TFT_BLACK);
    M5.Lcd.printf("%2.0f%%", BatteryLevel);

    if (BatteryLevel >= 75) {
      M5.Lcd.fillRect(BatteryCellsScreenPosition[0], 117, 10, 11, TFT_GREEN);
      M5.Lcd.fillRect(BatteryCellsScreenPosition[1], 117, 10, 11, TFT_GREEN);
      M5.Lcd.fillRect(BatteryCellsScreenPosition[2], 117, 10, 11, TFT_GREEN);
      M5.Lcd.fillRect(BatteryCellsScreenPosition[3], 117, 10, 11, TFT_GREEN);
      LedControl(POWER_DATA_LED, 5, 5, 5);
      return;
    }

    else if (BatteryLevel >= 50) {
      M5.Lcd.fillRect(BatteryCellsScreenPosition[0], 117, 10, 11, TFT_GREEN);
      M5.Lcd.fillRect(BatteryCellsScreenPosition[1], 117, 10, 11, TFT_GREEN);
      M5.Lcd.fillRect(BatteryCellsScreenPosition[2], 117, 10, 11, TFT_GREEN);
      M5.Lcd.fillRect(BatteryCellsScreenPosition[3], 117, 10, 11, TFT_BLACK);
      LedControl(POWER_DATA_LED, 0, 30, 0);
      return;
    }

    else if (BatteryLevel >= 25) {
      M5.Lcd.fillRect(BatteryCellsScreenPosition[0], 117, 10, 11, TFT_GREEN);
      M5.Lcd.fillRect(BatteryCellsScreenPosition[1], 117, 10, 11, TFT_GREEN);
      M5.Lcd.fillRect(BatteryCellsScreenPosition[2], 117, 10, 11, TFT_BLACK);
      M5.Lcd.fillRect(BatteryCellsScreenPosition[3], 117, 10, 11, TFT_BLACK);
      LedControl(POWER_DATA_LED, 30, 20, 0);
      return;
    }

    else {
      M5.Lcd.fillRect(BatteryCellsScreenPosition[0], 117, 10, 11, TFT_GREEN);
      M5.Lcd.fillRect(BatteryCellsScreenPosition[1], 117, 10, 11, TFT_BLACK);
      M5.Lcd.fillRect(BatteryCellsScreenPosition[2], 117, 10, 11, TFT_BLACK);
      M5.Lcd.fillRect(BatteryCellsScreenPosition[3], 117, 10, 11, TFT_BLACK);
      LedControl(POWER_DATA_LED, 30, 0, 0);
    }
  }
}

//----------------------------------------------------------------------
// Update the real time clock.
//
// Run by the dispatcher function when it's due.
// Update might not be needed, but checking the GPS and RTC are aligned
// just isn't worth the cpu cycles - so they just get blindly updated from
// the TAI value.
//
// As a function, this has to be completed within the time frame
// specified by the dispatcher, else it will just return (KillMe macro).
// This is because if the GPS is slow,garbled, or hasn't got a signal this
// function would loop searching for valid records that might never arrive).
//----------------------------------------------------------------------

void UpdateRtc() {
  int Rtcok = 0;
  char GpsSentence[100];
  char gpsdata[7][50];
  char StrGpsHour[3], StrGpsMin[3], StrGpsSec[3], StrGpsYear[5], StrGpsMonth[3], StrGpsDay[3];
  char ch = '\0';
  int SentenceSize, GpsYear, dlmcount;
  LedControl(RTC_LED, 50, 0, 0);
  Rtcok = 0;

  while (Rtcok == 0) {
    if KILLME {
      Serial.print("#UpdateRtc: Time allocation expired\n");
      LedControl(RTC_LED, 0, 0, 0);
      return;
    }

    else {
      while (ch != '$') {
        if (iicSerial1.available()) ch = iicSerial1.read();
      }
      memset(GpsSentence, 0, 100);
      SentenceSize = 1;
      GpsSentence[0] = ch;
      //Read a sentence
      while ((ch != '\n') && (SentenceSize < MAX_GPS_SENTENCE_SIZE)) {
        if (iicSerial1.available()) {
          ch = iicSerial1.read();
          GpsSentence[SentenceSize++] = ch;
        }
      }
      GpsSentence[SentenceSize - 1] = '\0';
      if (!memcmp("$GNZDA", GpsSentence, 6)) {
        dlmcount = DlmCount(',', GpsSentence);
        if (dlmcount != 6) {
          Serial.printf("#UpdateRtc: Rejected(%d): %s\n", dlmcount, GpsSentence);
          M5.Lcd.fillRect(1, 45, 150, 20, TFT_RED);
          M5.Lcd.setCursor(2, 48);
          M5.Lcd.print("TAI fix lost");
        }

        else {
          Str2Array(50, 6, ",", gpsdata[0], GpsSentence);
          strncpy(StrGpsYear, gpsdata[4], 4);
          StrGpsYear[4] = 0x0;
          GpsYear = atoi(StrGpsYear);

          if (GpsYear > 2000) {
            Rtcok = 1;
            Serial.printf("#UpdateRtc: Set by: \"%s\"\n", GpsSentence);
            M5.Lcd.fillRect(1, 45, 150, 20, TFT_GREEN);
            M5.Lcd.setCursor(2, 48);
            M5.Lcd.setTextColor(TFT_BLACK);
            M5.Lcd.print("TAI fix good");
            M5.Lcd.setTextColor(TFT_WHITE);
            RTCDate.Year = GpsYear;
            strncpy(StrGpsSec, gpsdata[1] + 4, 2);
            StrGpsSec[2] = 0x0;
            RTCTime.Seconds = atoi(StrGpsSec);
            strncpy(StrGpsMin, gpsdata[1] + 2, 2);
            StrGpsMin[2] = 0x0;
            RTCTime.Minutes = atoi(StrGpsMin);
            strncpy(StrGpsHour, gpsdata[1], 2);
            StrGpsHour[2] = 0x0;
            RTCTime.Hours = atoi(StrGpsHour);
            strncpy(StrGpsDay, gpsdata[2], 2);
            StrGpsDay[2] = 0x0;
            RTCDate.Date = atoi(StrGpsDay);
            strncpy(StrGpsMonth, gpsdata[3], 2);
            StrGpsMonth[2] = 0x0;
            RTCDate.Month = atoi(StrGpsMonth);
            M5.Rtc.SetTime(&RTCTime);
            M5.Rtc.SetDate(&RTCDate);
            LedControl(RTC_LED, 0, 0, 0);
            return;
          }

          else {
            Serial.printf("#UpdateRtc: Rejected (Wrong year) \"%s\"\n", GpsSentence);
            M5.Lcd.fillRect(1, 45, 150, 20, TFT_RED);
            M5.Lcd.setCursor(2, 48);
            M5.Lcd.print("TAI fix lost");
          }
        }
      }
    }
  }
  LedControl(RTC_LED, 0, 0, 0);
}

//----------------------------------------------------------------------
// UpdateLocationData
// Hunt for $GNGGA records that have a fix
// Give up if the KILLME macro test is passed.
//----------------------------------------------------------------------
void UpdateLocationData() {
  static int StoredLatDegs, StoredLatMins, StoredLatTrail, StoredLonDegs, StoredLonMins, StoredLonTrail = 60;
  char GpsSentence[100];
  char gpsdata[7][50];
  char ch = '\0';
  int SentenceSize = 0;
  char StringBit[10];
  int NoLock = 1;
  int dlmcount;
  LedControl(LOCATION_DATA_LED, 0, 50, 0);

  while (NoLock) {
    if KILLME {
      Serial.print("UpdateLocationData: Time allocation expired\n");
      LedControl(LOCATION_DATA_LED, 0, 0, 0);
      return;
    } else {
      while (ch != '$') {
        if (iicSerial1.available()) ch = iicSerial1.read();
      }
      memset(GpsSentence, 0, 100);
      SentenceSize = 1;
      GpsSentence[0] = ch;
      //Read a sentence
      while ((ch != '\n') && (SentenceSize < MAX_GPS_SENTENCE_SIZE)) {
        if (iicSerial1.available()) {
          ch = iicSerial1.read();
          GpsSentence[SentenceSize++] = ch;
        }
      }
      GpsSentence[SentenceSize - 1] = '\0';
      if (!memcmp("$GNGGA", GpsSentence, 6)) {
        dlmcount = DlmCount(',', GpsSentence);
        if (dlmcount != 14) {
          Serial.printf("#UpdateLocationData: Rejected GNGGA(Field count %d): \"%s\"\n", dlmcount, GpsSentence);
          if (Dark == 1) return;
          M5.Lcd.fillRect(1, 19, 150, 20, TFT_RED);
          M5.Lcd.setCursor(2, 22);
          M5.Lcd.print("GPS fix lost");
        }

        else {
          Str2Array(50, 7, ",", gpsdata[0], GpsSentence);
          LatDir = gpsdata[3][0];
          LonDir = gpsdata[5][0];
          if ((LatDir != 'N') && (LatDir != 'S')) {
            Serial.printf("#UpdateLocationData: Rejected (No Fix) \"%s\"\n", GpsSentence);
            if (Dark == 1) return;
            M5.Lcd.fillRect(1, 19, 150, 20, TFT_RED);
            M5.Lcd.setCursor(2, 22);
            M5.Lcd.print("GPS fix lost");
          }

          else {
            //Serial.printf("#UpdateLocationData: set by \"%s\"\n", GpsSentence);
            //Hopefully, good values
            if (Dark == 0) {
              M5.Lcd.fillRect(1, 19, 150, 20, TFT_GREEN);
              M5.Lcd.setTextColor(TFT_BLACK);
              M5.Lcd.setCursor(2, 22);
              M5.Lcd.print("GPS fix ok");
              M5.Lcd.setTextColor(TFT_WHITE);
            }
            memcpy(StringBit, gpsdata[2], 2);
            StringBit[2] = 0x0;
            LatDegs = atoi(StringBit);

            memcpy(StringBit, gpsdata[2] + 2, 2);
            StringBit[2] = 0x0;
            LatMins = atoi(StringBit);

            memcpy(StringBit, gpsdata[2] + 5, 5);
            StringBit[5] = 0x0;
            LatTrail = atoi(StringBit);
            memcpy(StringBit, gpsdata[4], 3);
            StringBit[3] = 0x0;
            LonDegs = atoi(StringBit);

            memcpy(StringBit, gpsdata[4] + 3, 2);
            StringBit[2] = 0x0;
            LonMins = atoi(StringBit);

            memcpy(StringBit, gpsdata[4] + 6, 5);
            StringBit[5] = 0x0;
            LonTrail = atoi(StringBit);
            NoLock = 0;
          }
        }
      }
    }
  }
  if (LatDegs != StoredLatDegs) {
    StoredLatDegs = LatDegs;
    if (Dark == 0) {
      M5.Lcd.fillRect(1, 70, 65, 16, TFT_BLACK);
      M5.Lcd.setCursor(1, 70);
      M5.Lcd.printf("%c %03d ", LatDir, LatDegs);
    }
  }
  if (LatMins != StoredLatMins) {
    StoredLatMins = LatMins;
    if (Dark == 0) {
      M5.Lcd.fillRect(70, 70, 35, 16, TFT_BLACK);
      M5.Lcd.setCursor(70, 70);
      M5.Lcd.printf("%02d.", LatMins);
    }
  }
  if (LatTrail != StoredLatTrail) {
    LatVar = CalcDiff(StoredLatTrail, LatTrail);
    StoredLatTrail = LatTrail;
    if (Dark == 0) {
      M5.Lcd.fillRect(110, 70, 120, 16, TFT_BLACK);
      M5.Lcd.setCursor(110, 70);
      M5.Lcd.printf("%05d", LatTrail);
    }
  }
  if (LonDegs != StoredLonDegs) {
    StoredLonDegs = LonDegs;
    if (Dark == 0) {
      M5.Lcd.fillRect(1, 89, 65, 16, TFT_BLACK);
      M5.Lcd.setCursor(1, 89);
      M5.Lcd.printf("%c %03d ", LonDir, LonDegs);
    }
  }
  if (LonMins != StoredLonMins) {
    StoredLonMins = LonMins;
    if (Dark == 0) {
      M5.Lcd.fillRect(70, 89, 35, 16, TFT_BLACK);
      M5.Lcd.setCursor(70, 89);
      M5.Lcd.printf("%02d.", LonMins);
    }
  }
  if (LonTrail != StoredLonTrail) {
    LonVar = CalcDiff(StoredLonTrail, LonTrail);
    StoredLonTrail = LonTrail;
    if (Dark == 0) {
      M5.Lcd.fillRect(110, 89, 120, 16, TFT_BLACK);
      M5.Lcd.setCursor(110, 89);
      M5.Lcd.printf("%05d", LonTrail);
    }
  }
  LedControl(LOCATION_DATA_LED, 0, 0, 0);
}

//--------------------------------------------------------------------
// UpdateSensorData
//
// Go through each of the attached sensors collecting data.
// Store the data in globals
//---------------------------------------------------------------------
void UpdateSensorData() {
  static char Buffer[201];
  Tvoc = 0;
  Eco2 = 0;
  Temperature = 0;
  Humidity = 0;
  LedControl(SENSOR_DATA_LED, 0, 0, 50);
  Pressure = (qmp6988.calcPressure() / 100.0);
    Lux = LuxSensor.readLightLevel();
  if (sgp.IAQmeasure()) {
    Tvoc = sgp.TVOC;
    Eco2 = sgp.eCO2;
  }

  if (sht30.get() == 0) {
    Temperature = sht30.cTemp;
    Humidity = sht30.humidity;
  }

  snprintf(Buffer, 200, "%04d/%02d/%02d-%02d:%02d:%02d %2.1f %2.0f %4.2f %d %d %.0f %2.1f %c%02d%02d.%05d %c%03d%02d.%05d\n",
           RTCDate.Year, RTCDate.Month, RTCDate.Date,
           RTCTime.Hours, RTCTime.Minutes, RTCTime.Seconds,
           Temperature, Humidity, Pressure, Tvoc, Eco2, Lux, BatteryLevel,
           LatDir, LatDegs, LatMins, LatTrail, LonDir, LonDegs, LonMins, LonTrail);

  WriteFramBuffer(Buffer);
  LedControl(SENSOR_DATA_LED, 0, 0, 0);
}

//----------------------------------------------------------------------
// UpdateClockDisplay
//----------------------------------------------------------------------
void UpdateClockDisplay() {
  static int StoredHours, StoredMins, StoredSecs = 60;
  M5.Rtc.GetTime(&RTCTime);
  M5.Rtc.GetDate(&RTCDate);
  if (StoredSecs == RTCTime.Seconds) return;
  StoredSecs = RTCTime.Seconds;
  if (RTCTime.Hours != StoredHours) {
    StoredHours = RTCTime.Hours;
    if (Dark == 0) {
      M5.Lcd.setCursor(140, 115);
      M5.Lcd.fillRect(140, 114, 30, 16, TFT_BLACK);
      M5.Lcd.printf("%02d:", StoredHours);
    }
  }
  if (RTCTime.Minutes != StoredMins) {
    StoredMins = RTCTime.Minutes;
    if (Dark == 0) {
      M5.Lcd.setCursor(175, 115);
      M5.Lcd.fillRect(175, 114, 30, 16, TFT_BLACK);
      M5.Lcd.printf("%02d:", StoredMins);
    }
  }
  if (Dark == 0) {
    M5.Lcd.setCursor(208, 115);
    M5.Lcd.fillRect(208, 114, 30, 16, TFT_BLACK);
    M5.Lcd.printf("%02d", StoredSecs);
  }
}

//--------------------------------------------------------------------
void Str2Array(int len, int fieldcount, char *dlm, char *ary, char *fields) {
  int k, i;
  int startpos = 0;

  for (k = 0; k < fieldcount - 1; k++) {
    i = strpos(dlm, fields + startpos);
    memset(ary + (k * len), 0, len);
    strncpy(ary + (k * len), fields + startpos, i);
    startpos += (i + 1);
  }
  strcpy(ary + (k * len), fields + startpos);
}
//-------------------------------------------------------------------------
int strpos(char *needle, char *haystack) {
  char *offset;
  offset = strstr(haystack, needle);
  if (offset != NULL) {
    return offset - haystack;
  }
  return -1;
}
//----------------------------------------------------------------------
int DlmCount(char dlm, char *targetStr) {
  int ctr = 0;
  int len = strlen(targetStr);
  for (int k = 0; k < len; k++) {
    if (targetStr[k] == dlm) ctr++;
  }
  return ctr;
}
//----------------------------------------------------------------------
int CalcDiff(int val1, int val2) {
  if (val1 >= val2) return (val1 - val2);
  else return (val2 - val1);
}



//--------------------------------------------------------------------
// ScanI2CBus - Check 14 addresses are valid
//---------------------------------------------------------------------
void ScanI2CBus() {
  int address;
  int error;
  int DeviceCount = 0;
  int ctr;
  Serial.print("#ScanI2CBus: scanning Address [HEX] ");
  for (address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
    if (error == 0) {
      Serial.printf("%x ", address);
      DeviceCount++;
    }
    delay(10);
  }
  Serial.printf("\n#ScanI2CBus: Found %d device(s)\n", DeviceCount);
  if (DeviceCount != EXPECTED_I2C_COUNT) {
    Serial.println("#ScanI2CBus: Device count not equal to EXPECTED_I2C_COUNT\n Halt.. ");
    while (1)
      ;
  }
}

//--------------------------------------------------------------------
//

void InitI2CBridge() {
  while (iicSerial1.begin(9600) != 0) {
    Serial.println("I2C UART bridge init failed");
    LedControl(6, 50, 0, 0);
    while (1)
      ;
  }
}
//----------------------------------------------------------------------
// WriteFramBuffer
// Parameters - Just the character array to write
//
// Each write will take up 100 bytes.
// If the incomming string is less than 100, end of
// write is padded out with nulls.
//----------------------------------------------------------------------
void WriteFramBuffer(char *buff) {
  int k, FramLastWrite, len, leftover;
  char c;
  len = strlen(buff);
  if (len > 100) {
    Serial.println("#WriteFramBuffer: Passed record longer than 100. Ignored it");
    return;
  }
  FramLastWrite = fram.read32(FRAM_LASTWRITE_ADDRESS);
  //If Fram is full, then set the write point back to address 0
  if (FramLastWrite >= FRAM_MAX) {
    Serial.printf("#WriteFramBuffer: Circular buffer full at %02d:%02d:%02d\n", RTCTime.Hours, RTCTime.Minutes, RTCTime.Seconds);
    FramLastWrite = 0;
    fram.write32(FRAM_LASTWRITE_ADDRESS, FramLastWrite);
  }
  for (k = 0; k < FRAM_REC_SIZE; k++) {
    if (k < len) fram.write8(FramLastWrite++, buff[k]);
    else fram.write8(FramLastWrite++, 0x0);
  }
  fram.write32(FRAM_LASTWRITE_ADDRESS, FramLastWrite);
}

//----------------------------------------------------------------------
// LedControl
// Parameters are, the LED number to light and the RGB values
// If the global Dark is set, then the target LED is extinguished
//----------------------------------------------------------------------
void LedControl(int LedNo, int StateRed, int StateGreen, int StateBlue) {
  if (Dark == 1) {
    pixels.setPixelColor(LedNo, pixels.Color(0, 0, 0));
    pixels.show();
  }

  else {
    pixels.setPixelColor(LedNo, pixels.Color(StateRed, StateGreen, StateBlue));
    pixels.show();
  }
}
//----------------------------------------------------------------------
// FramDump
// Dump the contents of Fram to a web client
//----------------------------------------------------------------------
void FramDump() {
  int k;
  char c;
  for (k = 0; k < FRAM_MAX; k++) {
    c = fram.read8(k);
    if (c != 0x0) client.printf("%c", c);
  }
}

//----------------------------------------------------------------------
// OMG This is really ugly
//----------------------------------------------------------------------
void ServeWebPage() {
  char c;
  int Ctr, ShowPage, DumpRecs;
  static int AutoRefresh = 0;
  static int JustDumped = 0;
  String ClientRequest;
  ShowPage = 0;
  DumpRecs = 0;
  client = server.available();
  if (client) {
    LedControl(WEBPAGE_DATA_LED, 30, 0, 0);
    ClientRequest = "";
    while (client.connected()) {
      c = client.read();
      if (c == '\n') {
        ClientRequest += c;
        if (Ctr == 0) {
          ShowPage = 1;
          break;
        }

        else {
          Ctr = 0;
        }
      }

      else if (c != '\r') {
        ClientRequest += c;
        Ctr++;
      }
    }
  }
  if (ShowPage) {
    if (ClientRequest.indexOf("GET /AutoRefresh/on") >= 0) AutoRefresh = 1;
    else if (ClientRequest.indexOf("GET /AutoRefresh/off") >= 0) AutoRefresh = 0;

    //------------------------------------------------------
    // If Dark mode (save power) is requested, then turn
    // the power button light off and the screen.
    // When Dark ==1, any call to turn on an leds using the
    // function LedControl will return without being actioned
    //------------------------------------------------------
    if (ClientRequest.indexOf("GET /Dark/on") >= 0) {
      Dark = 1;
      LedControl(POWER_DATA_LED, 0, 0, 0);
      M5.Axp.SetLcdVoltage(2500);
    }
    //------------------------------------------------------
    // If Dark mode turned off, turn the screen back on
    //------------------------------------------------------

    else if (ClientRequest.indexOf("GET /Dark/off") >= 0) {
      Dark = 0;
      M5.Axp.SetLcdVoltage(2800);
      //At this stage, the battery state is unknown
      //Light the power on button as dull white.
      LedControl(POWER_DATA_LED, 5, 5, 5);
    }
    if (ClientRequest.indexOf("GET /PowerOff") >= 0) M5.shutdown();
    //------------------------------------------------------
    // If a dump is requested, turn autorefresh off
    //------------------------------------------------------

    if (ClientRequest.indexOf("GET /DumpRecs") >= 0) {
      DumpRecs = 1;
      AutoRefresh = 0;
    }
    client.println("HTTP/1.1 200 OK\nContent-type:text/html\n");
    client.println("<!DOCTYPE html><html>\n<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
    client.println("<link rel=\"icon\" href=\"data:,\">");

    if (AutoRefresh == 1) {
      client.println("<meta http-equiv=\"refresh\" content=\"5\">");  //Auto refresh
    }
    client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
    client.println(".button { background-color: #4CAF50; border-radius: 8px; color: white; padding: 5px 10px;");
    client.println("text-decoration: none; font-size: 16px; margin: 2px; cursor: pointer;}");

    if (AutoRefresh == 1) client.println(".button3 {background-color: #f44336;}");

    if (Dark == 1) client.println(".button2 {background-color: #f44336;}");

    if (DumpRecs == 1) client.println(".button4 {background-color: #f44336;}");

    client.println("table, th, td { text-align: left; padding: 5px; border: 1px solid black; border-radius: 10px; } </style></head>");

    client.printf("<body><h2>Sensor Server (Battery: %3.1f%%)</h2>", BatteryLevel);
    client.print("<center><table>");
    client.printf("<tr><td><b>Lattitude</b></td><td>%d&#176; %02d.%d %c </td>", LatDegs, LatMins, LatTrail, LatDir);
    client.printf("<td><b>Longitude</b></td><td>%d&#176; %02d.%d %c </td></tr>", LonDegs, LonMins, LonTrail, LonDir);
    client.printf("<tr><td><b>LatDrift</b></td><td>.%05d</td>", LonVar);
    client.printf("<td><b>LonDrift</b></td><td>.%05d</td></tr>", LatVar);
    client.printf("<tr><td><b>Date</b></td><td>%02d/%02d/%04d</td>", RTCDate.Date, RTCDate.Month, RTCDate.Year);
    client.printf("<td><b>Time</b></td><td>%02d:%02d:%02d</td></tr>", RTCTime.Hours, RTCTime.Minutes, RTCTime.Seconds);
    client.printf("<tr><td><b>Temperature</b></td><td>%2.1f &#8451;</td>", Temperature);
    client.printf("<td><b>Pressure</b></td><td>%4.2f</td></tr>", Pressure);
    client.printf("<tr><td><b>Humidity</b></td><td>%3.1f%%</td>", Humidity);
    client.printf("<td><b>Lux</b></td><td>%.0f</td></tr>", Lux);
    client.printf("<tr><td><b>TVOC</b></td><td>%d</td>", Tvoc);
    client.printf("<td><b>eCO2</b></td><td>%d</td></tr>", Eco2);
    client.println("</table>");

    //Power off button
    client.println("<button class=\"button button1\" onClick=\"window.location.href='/PowerOff';\">Power Off</button>");

    //Dark button
    if (Dark == 1) client.println("<button class=\"button button2\" onClick=\"window.location.href='/Dark/off';\">Dark off</button>");
    else client.println("<button class=\"button button2\" onClick=\"window.location.href='/Dark/on';\">Dark on</button>");

    //Refresh button
    if (AutoRefresh == 1) client.println("<button class=\"button button3\" onClick=\"window.location.href='/AutoRefresh/off';\">Refresh off</button>");
    else client.println("<button class=\"button button3\" onClick=\"window.location.href='/AutoRefresh/on';\">Refresh on</button>");

    //Dump button
    client.println("<button class=\"button button4\" onClick=\"window.location.href='/DumpRecs';\">Dump</button></p>");

    //------------------------------------------------------
    // If a dump is requested, paint a text area and fill
    // it with the dump records. Suitable for cut and paste.
    // Paint a done button when the dump is completed.
    //------------------------------------------------------
    if (DumpRecs == 1) {
      client.println("<textarea cols=\"62\" rows=\"10\" style=\"overflow:auto;\">");
      FramDump();
      client.println("</textarea><p>");
      DumpRecs = 0;
      //client.println("<p> Dump complete</p>");
      client.println("<button class=\"button button5\" style=\"background-color:#f44336;\" autofocus onClick=\"window.location.href='/AutoRefresh/on';\">Done</button>");
    }

    client.println("</center></body></html>\n");
    LedControl(WEBPAGE_DATA_LED, 0, 0, 0);
    client.stop();
  }

  else {
    LedControl(WEBPAGE_DATA_LED, 0, 0, 0);
  }
}
//----------------------------------------------------------------------
// AddTask
//https://www.tutorialspoint.com/data_structures_algorithms/linked_list_program_in_c.htm
//----------------------------------------------------------------------
void AddTask(char *Descr, void (*Fptr)(), unsigned long delay, unsigned long killtime) {
  static struct RunTable *_link_ = NULL;
  static struct RunTable *_lastnode_ = NULL;
  if (_DispatchList_ == NULL) {
    _DispatchList_ = (struct RunTable *)malloc(sizeof(struct RunTable));
    strncpy(_DispatchList_->TaskDescription, Descr, 100);
    _DispatchList_->ScheduleInterval = delay;
    _DispatchList_->MaxRunTime = killtime;
    _DispatchList_->Fptr = Fptr;
    _DispatchList_->next = NULL;
    Serial.printf("#AddTask: Inserted \"%s\", Interval=%lu, Runtime=%lu, Func=%p\n",
                  _DispatchList_->TaskDescription, _DispatchList_->ScheduleInterval, _DispatchList_->MaxRunTime, _DispatchList_->Fptr);
    _lastnode_ = _DispatchList_;
  }

  else {
    _link_ = (struct RunTable *)malloc(sizeof(struct RunTable));
    strncpy(_link_->TaskDescription, Descr, 100);
    _link_->ScheduleInterval = delay;
    _link_->MaxRunTime = killtime;
    _link_->Fptr = Fptr;
    _link_->next = NULL;
    Serial.printf("#AddTask: Inserted \"%s\", Interval=%lu, Runtime=%lu, Func=%p\n",
                  _link_->TaskDescription, _link_->ScheduleInterval, _link_->MaxRunTime, _link_->Fptr);
    _lastnode_->next = _link_;
    _lastnode_ = _link_;
  }
}

//--------------------------------------------------------------------
// Dispatch - The poor man's version of an OS task dispatcher.
//---------------------------------------------------------------------
void Dispatch(int debugLevel) {
  struct RunTable *ptr = _DispatchList_;
  struct RunTable *Candidate;
  unsigned long LapseTime;
  unsigned long Overrun = 0;
  unsigned long StoredOverrun = 0;
  Candidate = NULL;
  while (ptr != NULL) {
    LapseTime = millis() - ptr->LastRunTime;
    if (LapseTime >= ptr->ScheduleInterval) {
      Overrun = LapseTime - ptr->ScheduleInterval;
      if (Overrun > 0) {
        if (debugLevel == 1 || debugLevel == 3) Serial.printf("#Dispatch: \"%s\" overdue (%luMs)\n", ptr->TaskDescription, Overrun);
        if (Overrun > StoredOverrun) {
          StoredOverrun = Overrun;
          Candidate = ptr;
        }
      }
    }
    ptr = ptr->next;
  }
  if (Candidate != NULL) {
    _GlobalKillTime_ = Candidate->MaxRunTime + millis();
    Candidate->LastRunTime = millis();
    Candidate->Fptr();
    if (debugLevel >= 2) Serial.printf("#Dispatch: \"%s\" took %luMs\n", Candidate->TaskDescription, (millis() - Candidate->LastRunTime));
  }
}
