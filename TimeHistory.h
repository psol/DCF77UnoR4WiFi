#pragma once

class TimeHistory {
  private:
    static const unsigned max = 1440;  
    unsigned              len = 0,
                          last = 0,
                          i;
    unsigned long long    times[max];

  public:
    inline void push(unsigned long long t) {
      if (len && ++last >= max)   // will only inc if len != 0, so on first push, last remains at 0
        last = 0;
      if (++len > max) len = max;
      times[last] = t;
    }
    inline unsigned length() {
      return len;
    }
    static inline unsigned capacity() {
      return max;
    }
    inline unsigned long long peek() {
      return times[last];
    }
    // not multi-threaded, one iteration at a time
    // however it survives inserts during iterations
    // with limitations, see testTimeHistory()
    inline void iterate() {
      if (len < max || last == max - 1)
        i = 0;
      else
        i = last + 1;
    }
    inline unsigned long long next() {
      unsigned long long result = times[i];
      if (++i >= max) i = 0;
      return result;
    }
};

// unit tests, run in begin() during development
void testTimeHistory();