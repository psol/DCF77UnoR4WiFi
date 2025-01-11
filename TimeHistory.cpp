#include <Arduino.h>
#include "TimeHistory.h"

void testEqual(unsigned v1, unsigned v2, const __FlashStringHelper *ctx, const __FlashStringHelper *msg) {
  if (v1 != v2) {
    Serial.print(ctx);
    Serial.print(F(": "));
    Serial.print(msg);
    Serial.print(F(" (v1 = "));
    Serial.print(v1);
    Serial.print(F(", v2 = "));
    Serial.print(v2);
    Serial.println(F(")"));
  }
}

void testWithLoop(unsigned len, const __FlashStringHelper *ctx) {
  TimeHistory h;
  for (unsigned long long i = 0ll;i < len;i++)
    h.push(i);
  testEqual(h.peek(),   len - 1,                                 ctx, F("h.peek() incorrect"));
  testEqual(h.length(), len < h.capacity() ? len : h.capacity(), ctx, F("h.length() incorrect"));
  h.iterate();
  unsigned long long offset = len <= h.capacity() ? 0 : len - h.capacity();
  for (unsigned long long i = 0ll; i < h.length(); i++)
    testEqual(h.next(), i + offset, ctx, F("h.next() incorrect"));
}

void testPushIterate(unsigned len_init, unsigned len_iter, unsigned len_push,const __FlashStringHelper *ctx) {
  TimeHistory h;
  for (unsigned long long i = 0ll;i < len_init;i++)
    h.push(i);
  h.iterate();
  unsigned long long offset = len_init <= h.capacity() ? 0 : len_init - h.capacity();
  for (unsigned long long i = 0ll; i < len_iter; i++)
    testEqual(h.next(), i + offset, ctx, F("h.next() incorrect"));
  for (unsigned long long i = 0ll;i < len_push;i++)   // simulate insert during iteration
    h.push(len_init + i);                             // e.g. new interupt during HTTP request
  unsigned len_exp = len_init + len_push;
  if(len_exp > h.capacity()) len_exp = h.capacity();
  testEqual(h.length(), len_exp, ctx, F("h.length() not affected by the insertion"));
  unsigned count = len_iter;
  for (unsigned long long i = len_iter; i < h.length(); i++, count++)
    testEqual(h.next(), i + offset, ctx, F("h.next() incorrect"));
  unsigned count_exp = len_push + len_init;
  if(count_exp > h.capacity()) count_exp = h.capacity();
  testEqual(count, count_exp, ctx, F("not enough iteration"));
}

void testTimeHistory() {
  TimeHistory h;
  const __FlashStringHelper *ctx = F("single");
  h.push(0x05ll);
  h.iterate();
  testEqual(h.peek(), 0x05ll,   ctx, F("h.peek() incorrect"));
  testEqual(h.length(), 1,      ctx, F("h.length() incorrect"));
  testEqual(h.next(),   0x05ll, ctx, F("h.next() incorrect"));

  testWithLoop(5,                F("[0-4]"));
  testWithLoop(h.capacity(),     F("at capacity"));
  testWithLoop(h.capacity() - 1, F("below capacity"));
  testWithLoop(h.capacity() + 1, F("above capacity"));
  testWithLoop(3 * h.capacity(), F("large dataset"));

  // testing for small number of inserts: at a rate of one per minute, there will not be many
  testPushIterate(5,                2,                1, F("insert"));
  testPushIterate(h.capacity(),     h.capacity() / 2, 1, F("insert at capacity"));
  testPushIterate(h.capacity() - 1, h.capacity() / 2, 2, F("insert below capacity"));
  testPushIterate(h.capacity() + 1, h.capacity() / 3, 3, F("insert above capacity"));
  testPushIterate(3 * h.capacity(), h.capacity(),     4, F("insert in large dataset"));

  Serial.println("testTimeHistory() finished");
}