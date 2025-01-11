/*
 * Initially I had this custom class for for the LED display, inspired by Clemens Valens.
 * It needed less memory than Arduino's own LED Matrix library but it relied on turning
 * the LEDs on/off very quickly. And this proved a problem when combined with WiFi
 * as the loop was no longer called fast enough.
 * The trick is clever because when you turn on a LED in the matrix, you can turn off
 * another LED. So the Arduino library needs more memory to "composite" the final image
 * but it has the benefit that it works well even when the loop is busy.
 * Anyway I wanted to keep the project files somewhere, in case I ever need them.
 * This is an excerpt from the .ino to refresh my mind on how to use the R4Display class.
 */

#include <RTC.h>
#include "R4Display.h"   // import .h

void setup() {
  Serial.begin(115200);
  while(!Serial) ;

  RTC.begin();

  // begin the display and load data in the buffer
  R4DISPLAY.begin();
  updateDisplay();

  // other setup here
}

void loop() {
  static unsigned state = 0;

  // the loop scrolls and redraw the display
  switch (state) {
    case 0:
      scrollDisplay();
      break;
    case 1:
      updateDisplay();
      break;
  }
  if (++state > 1) state = 0;

  // other loop code would come here
}

// render the frame and scrolls
void scrollDisplay() {
  R4DISPLAY.showFrame();
  R4DISPLAY.nextFrame();
}

// every minute loads a new text in the buffer
void updateDisplay() {
  const int      len         = 25;
  static int     prev = -1;
  static RTCTime now;
  static char    buffer[len];

  RTC.getTime(now);
  if (prev != now.getMinutes()) {
    snprintf(buffer, len, "%.2d:%.2d", now.getHour(), now.getMinutes());
    R4DISPLAY.setText(buffer);
    prev = now.getMinutes();
  }
}