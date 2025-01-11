/*
 * A fairly simple web server for the Arduino Uno R4 WiFi.
 * Focuses on returning to the loop ASAP.
 * Strictly mono-threaded (one request at a time).
 */

#pragma once

#include <WiFiS3.h>
#include <functional>

typedef std::function<void(const char*,Print&)>      PrintContentType;
typedef std::function<bool(bool,const char*,Print&)> PrintBody;

class HttpServer {
  private:
    enum Tokens       { _400 = 0, _404 = 1, _405 = 2, START, G, E, T, PATH, VERSION, CR, LF, EOH, HEADER, STOP };
    enum StateMachine { WAIT, TOKENIZING, ERROR, BEGIN_RESP, NEXT_RESP, END_RESP, DONE };
    static const unsigned pathLen   = 15,
                          bufferLen = 25;
    char          path[pathLen + 1];
    uint8_t       buffer[bufferLen];
    unsigned      current;
    unsigned long prev;
    Tokens        token;
    StateMachine  state;
    WiFiServer    wiFiServer;
    WiFiClient    wiFiClient;
    bool tokenize(char c);
    inline bool isError();
  protected:
    PrintContentType printContentType;
    PrintBody        printBody;
  public:
    HttpServer(int port);
    HttpServer();
    void begin();
    void loop();
    void end();
    void acceptPrinters(PrintContentType printContentType, PrintBody printBody);
};