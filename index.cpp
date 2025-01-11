#include <Arduino.h>
#include <RTC.h>
#include "debug.h"
#include "index.h"

const __FlashStringHelper *START_HTML    = F("<!DOCTYPE html>\n<html><head><title>DCF77 on Arduino Uno R4 WiFi</title>"
"<style>.collapsible { background-color: #eee; color: #444; cursor: pointer; padding: 18px; width: 100%; border: none; text-align: left; outline: none; }\n"
".active, .collapsible:hover { background-color: #ccc; }\n"
".content { padding: 0 18px; display: none; overflow: hidden; background-color: #f1f1f1; }</style>"
"</head><body><h1>DCF77 on Arduino Uno R4 WiFi</h1><h2>Current time (from Arduino RTC)</h2><p>");
const __FlashStringHelper *SECTION2_HTML = F("</p><h2>Times received from DCF77</h2>");
const __FlashStringHelper *END_HTML      = F("<script type='text/javascript'>var coll = document.getElementsByClassName('collapsible'); var i; for (i = 0; i < coll.length; i++) { coll[i].addEventListener('click', function() { this.classList.toggle('active'); var content = this.nextElementSibling; if (content.style.display === 'block') content.style.display = 'none'; else content.style.display = 'block'; }); }</script></body></html>");

const unsigned len = 512;
char           buffer[len];

inline unsigned asInt(byte raw) {
  return (raw >> 4) * 10 + (raw & 0x0F);
}

bool printIndex(bool start, TimeHistory &history, Print &client) {
  static int i;

  if(start) {
    i = 0;
    history.iterate();
    client.print(START_HTML);
    RTCTime now;
    RTC.getTime(now);
    snprintf(buffer, len, "%.2d/%.2d/%.4d %.2d:%.2d", now.getDayOfMonth(), now.getMonth(), now.getYear(), now.getHour(), now.getMinutes());
    client.print(buffer);
    client.print(SECTION2_HTML);
  }
  for(int j = 0;i < history.length() && j < 10;i++, j++) {
    unsigned long long time = history.next();
    unsigned wx  = (time >>  1) & 0x3FFF;  // weather = bit 1-14
    byte year    = (time >> 50) & 0xFF;    // year = bit 50-57
    byte month   = (time >> 45) & 0x1F;    // month = bit 45-49
    byte weekday = (time >> 42) & 0x07;    // day of the week = bit 42-44
    byte day     = (time >> 36) & 0x3F;    // day of the month = bit 36-41
    byte hour    = (time >> 29) & 0x3F;    // hour = bit 29-34
    byte minute  = (time >> 21) & 0x7F;    // minute = 21-27 

    bool antenna = (time >> 15) & 0x01;
    bool summer  = (time >> 16) & 0x01;
    bool cest    = (time >> 17) & 0x01;
    bool cet     = (time >> 18) & 0x01;
    bool leap    = (time >> 19) & 0x01;

    bool minuteParity = (time >> 28) & 0x01;
    bool hourParity   = (time >> 35) & 0x01;
    bool dateParity   = (time >> 58) & 0x01;
    snprintf(buffer, len, "<button type='button' class='collapsible'>0x%08lX%08lX %.2d/%.2d/%.4d %.2d:%.2d CEST: %d CET: %d</button>",
      (unsigned long)((time >> 32) & 0xffffffff), (unsigned long)(time & 0xffffffff), asInt(day), asInt(month), asInt(year) + 2000, asInt(hour), asInt(minute), cest, cet);
    #ifdef DEBUG
      Serial.println(buffer);
    #endif
    client.print(buffer);
    snprintf(buffer, len, "<div class='content'><p>Weekday: %d Sec. antenna: %d Summer ann.: %d Leap ann.: %d Parity (min, hr, dt): %d %d %d Weather: 0x%04X</p></div>",
      weekday, antenna, summer, leap, minuteParity, hourParity, dateParity, wx);
    client.print(buffer);
  }
  if(i == history.length()) {
    client.println(END_HTML);
    return false;
  }
  return true;
}