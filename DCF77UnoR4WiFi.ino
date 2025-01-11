#include <RTC.h>
#include <WiFiS3.h>
#include <ArduinoGraphics.h>
#include <Arduino_LED_Matrix.h>
#include <TextAnimation.h>  // must be included after ArduinoGraphics.h
#include "index.h"
#include "HttpServer.h"
#include "TimeHistory.h"
#include "arduino_secrets.h" 

#ifndef ARDUINO_UNOWIFIR4
  #error "The sketch is designed specifically for an Arduino Uno R4 WiFi"
#endif

const char ssid[] = SECRET_SSID;    // your network SSID (name)
const char pass[] = SECRET_PASS;    // your network password (use for WPA, or use as key for WEP)

const pin_size_t            dcfPin        = 15; // A1
volatile unsigned long long dcfData       = 0;
volatile bool               loadAnimation = true;
uint8_t                     wiFiStatus    = WL_IDLE_STATUS;
HttpServer                  httpServer;
TimeHistory                 history;
ArduinoLEDMatrix            display;

void setup() {
  Serial.begin(115200);
  while(!Serial) ;

  RTC.begin();

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(dcfPin, INPUT);
  attachInterrupt(digitalPinToInterrupt(dcfPin), dcf77Interrupt, CHANGE);

  display.begin();
  display.textFont(Font_5x7);
  display.setCallback(animationEnd);
  display.textScrollSpeed(60);

  httpServer.acceptPrinters(&printContentType,&printResource);
}

void loop() {
  static unsigned           state  = 0;       // must be global/static
  static unsigned long long shadow = 0ll;     // avoid the overhead of stack
  static bool               valid  = false;

  // loop ensures atomicity without calling noInterrupts()
  // which is overkilled at a frequency of 0.017Hz… (1 update/minute)
  while (shadow != dcfData) {
    shadow = dcfData;
    if (shadow != 0) {
      valid = updateRTC(shadow);
      if (valid)
        history.push(shadow);
    }
    else
      valid = false;
  }

  switch (state) {
    case 0:
      blink(!valid);
      break;
    case 1:
      animate();
      break;
    case 2:
      connectWiFi();
      break;
    case 3:
      httpServer.loop();
      break;
  }
  if (++state > 3) state = 0;
}

void dcf77Interrupt() {
  static volatile unsigned long      lastInterrupt = 0l;    // must be global/static
  static volatile unsigned long long buffer        = 0ll;
  static volatile unsigned           pos           = 0;

  // substraction is rollover-safe
  unsigned long duration = millis() - lastInterrupt;

  if(digitalRead(dcfPin)) {
    if (duration > 1500){
      if (pos >= 59)
        dcfData = buffer;
      else
        dcfData = 0ll;
      pos = 0;
      buffer = 0ll;
    }
  }
  else {
    if (duration > 150)
      buffer |= ((unsigned long long)1 << pos);
    pos++;
  }
  lastInterrupt = millis();
}

void animationEnd() {
  loadAnimation = true;
}

boolean updateRTC(unsigned long long data) {
  byte year    = (data >> 50) & 0xFF; // year = bit 50-57
  byte month   = (data >> 45) & 0x1F; // month = bit 45-49
  byte weekday = (data >> 42) & 0x07; // day of the week = bit 42-44
  byte day     = (data >> 36) & 0x3F; // day of the month = bit 36-41
  byte hour    = (data >> 29) & 0x3F; // hour = bit 29-34
  byte minute  = (data >> 21) & 0x7F; // minute = 21-27 

  bool cest = (data >> 17) & 0x01;
  bool cet  = (data >> 18) & 0x01;

  bool minuteParity = (data >> 28) & 0x01;
  bool hourParity   = (data >> 35) & 0x01;
  bool dateParity   = (data >> 58) & 0x01;

  bool dateIsEven = (isEven(day) + isEven(weekday) + isEven(month) + isEven(year)) % 2;

  if(isEven(minute) == minuteParity && isEven(hour) == hourParity && dateIsEven == dateParity && cest != cet) {
    RTCTime dcfTime(asInt(day), Month(asInt(month)), asInt(year) + 2000,
                    asInt(hour), asInt(minute), 0, DayOfWeek(asInt(weekday)),
                    cest ? SaveLight::SAVING_TIME_ACTIVE : SaveLight::SAVING_TIME_INACTIVE);
    RTC.setTime(dcfTime);
    return true;
  } else
    return false;
}

inline unsigned asInt(byte raw) {
  return (raw >> 4) * 10 + (raw & 0x0F);
}

inline bool isEven(byte value) {
  value ^= value >> 4;
  value ^= value >> 2;
  value ^= value >> 1;
  value &= 0x01;
  return value;
}

void blink(bool on) {
  static unsigned long prev = 0l;      // must be global/static
  static bool          high = false;

  if(!on && high) {
    digitalWrite(LED_BUILTIN, LOW);
    high = false;
  }
  if(on && (millis() - prev >= 500)) {
    digitalWrite(LED_BUILTIN, high ? LOW : HIGH);
    high = !high;
    prev = millis();
  }
}

void animate() {
  static RTCTime now;          // avoid the overhead of stack management
  static char    buffer[25];
  // Maximum length of text to print (http://xxxx.xxxx.xxxx.xxx)
  // multiplied by font width (5 for Font_5x7), so 25 chars * 5 px = 125
  static TEXT_ANIMATION_DEFINE(anim, 125)

  if (loadAnimation) {
    loadAnimation = false;

    RTC.getTime(now);
    display.beginText(0, 1, 0xFFFFFF);
    if (wiFiStatus == WL_CONNECTED && now.getSeconds() > 30 && now.getSeconds() < 40) {
      IPAddress ip = WiFi.localIP();
      snprintf(buffer, 25, "http://%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
      display.println(buffer);
    }
    else {
      snprintf(buffer, 25, "%.2d:%.2d", now.getHour(), now.getMinutes());
      display.println(buffer);
    }
    display.endTextAnimation(SCROLL_LEFT, anim);
    display.loadTextAnimationSequence(anim);
    display.play();
  }
}

void connectWiFi() {
  enum StateMachine { BEGIN, CONNECT, CONNECTING, CONNECTED, LOST };
  static StateMachine  state = BEGIN;   // must be global/static
  static unsigned long prev  = 0l;
  
  switch(state) {
    case BEGIN:
      if (WiFi.status() == WL_NO_MODULE)
        Serial.println(F("ESP32-S3 does not reply"));
      if (WiFi.firmwareVersion() < WIFI_FIRMWARE_LATEST_VERSION)
        Serial.println(F("Time to launch the firmware updater"));
      state = CONNECT;
      break;
    case CONNECT:
      prev = millis();
      if (wiFiStatus != WL_CONNECTED) {
        wiFiStatus = WiFi.begin(ssid, pass);
        state = CONNECTING;
      }
      else {
        Serial.println(F("Invalid state: status == WL_CONNECTED && state == CONNECT"));
        state = CONNECTED;
      }
      break;
    case CONNECTING:
      if (millis() - prev > 10000) {
        if (wiFiStatus != WL_CONNECTED)
          state = CONNECT;
        else {
          IPAddress ip = WiFi.localIP();
          Serial.print(F("SSID: "));
          Serial.print(WiFi.SSID());
          Serial.print(F(" IP Address: "));
          Serial.print(ip);
          Serial.print(F(" RSSI: "));
          Serial.println(WiFi.RSSI());
          httpServer.begin();
          prev = millis();
          state = CONNECTED;
        }
      }
      break;
    case CONNECTED:
      if (millis() - prev > 30000) {
        prev = millis();
        if (WiFi.RSSI() == 0)
          state = LOST;
      }
      break;
    case LOST:
      // two checks at 30 seconds intervals
      if (millis() - prev > 30000 && WiFi.RSSI() == 0) {
        httpServer.end();
        wiFiStatus = WiFi.disconnect();
        state = CONNECT;
      }
      break;
  }
}

void printContentType(const char *path, Print &client) {
  client.println(F("Content-Type: text/html"));
}

bool printResource(bool start, const char *path, Print &client) {
  return printIndex(start, history, client);
}