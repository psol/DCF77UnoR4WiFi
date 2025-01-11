/*
 * Writes on the LED Matrix of the Arduino UNO R4 WiFi.
 * Adapted from Clemens Valens (Elektor)
 * https://github.com/ClemensAtElektor/Arduino-UNO-R4/tree/main/R4WiFi_led_matrix
 */

#pragma once

class R4LedDisplay {
  private:
    // Every byte represents a column of the LED matrix.
    // Can hold 32 5x8-font characters.
    // Buffer can be smaller at the price of more code.
    uint8_t buffer[5 * 32];
    unsigned len        = 0,
             offset     = 0,
             direction  = +1;
    unsigned long prev  = 0l,
                  ratio = 1000l;

  public:
    static const unsigned ROWS = 8,
                          COLS = 12;

  protected:
    // Pixel-to-pin translation table.
    // A HEX value encodes two pin numbers. The MSB is to be driven LOW,
    // the LSB is to be driven HIGH.
    // Example: pixel (4,2) contains the value 0x60, meaning that pin 6 must
    // be driven low and pin 0 must be driven high to activate the pixel.
    // The pin number is an offset to the constant led_matrix_pin_first
    // Note that they all appear in pairs, so you could make the table 50%
    // smaller at the cost of doing some swapping for odd or even columns.
    // (0,0) is upper left corner when the board's USB connector points to the left.
    static const uint8_t pixel2Pins[ROWS][COLS];
    static const pin_size_t firstPin = 28,
                            lastPin  = 38;

    void resetBuffer();
    void showFrame(unsigned offset, unsigned long duration);
    inline void lightPixel(unsigned x, unsigned y, unsigned long duration) {
      uint8_t pins = pixel2Pins[y][x];
      pin_size_t l = (pins >> 4)  + firstPin;
      pin_size_t h = (pins & 0xf) + firstPin;
      pinMode(l, OUTPUT);
      digitalWrite(l, LOW);
      pinMode(h, OUTPUT);
      digitalWrite(h, HIGH);
      // If ontime = 0, pixel remains active until it is deactivated
      // by another put_pixel that happens to use the same pin(s).
      if (duration != 0) {
        delayMicroseconds(duration);
        pinMode(l, INPUT);
        pinMode(h, INPUT);
      }
    }
    inline bool loadNextColInBuffer(uint8_t with) {
      if (len >= sizeof(buffer)) return false;
      buffer[len++] = with;
      return true;
    }
    unsigned getBufferLen() {
      return len;
    }

  public:
    void begin();
    unsigned setText(char *st);
    // in microseconds
    // 521 (us) * 96 (pixels) = 50 ms frame rate if all the pixels are on
    void showFrame(unsigned long duration = 521);
    unsigned nextFrame(unsigned long speed = 50000, bool pause = true);
};

extern R4LedDisplay R4DISPLAY;