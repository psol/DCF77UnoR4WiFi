#include "debug.h"
#include "HttpServer.h"

HttpServer::HttpServer(int port) : wiFiServer(port) {}
HttpServer::HttpServer() : wiFiServer(80) {}

bool HttpServer::tokenize(char ch) {
  switch (token) {
    case START:
      token = _405;
      current = 0;
      if (ch == 'G') token = G;
      break;
    case G:
      token = _405;
      if (ch == 'E') token = E;
      break;
    case E:
      token = _405;
      if (ch == 'T') token = T;
      break;
    case T:
      token = _405;
      if (ch == ' ') token = PATH;
      break;
    case PATH:
      if (ch == ' ') {
        path[current] = '\0';
        token = VERSION;
      }
      else {
        path[current++] = ch;
        if (current >= pathLen)
          token = _404;
      }
      break;
    case VERSION:
    case HEADER:
      if (ch == '\r') token = CR;
      break;
    case CR:
      token = _400;
      if (ch == '\n') token = LF;
      break;
    case LF:
      if (ch == '\r')
        token = EOH;
      else
        token = HEADER;
      break;
    case EOH:
      token = _400;
      if (ch == '\n') token = STOP;
      break;
  }
  return !isError() && token != STOP;
}

bool HttpServer::isError() {
  return token == _400 || token == _404 || token == _405;
}

void HttpServer::loop() {
  const __FlashStringHelper *errors[3] = { F("400 Bad request"), F("404 Not found"), F("405 Method not allowed") };

  if (WiFi.status() == WL_CONNECTED) {
    if (state != WAIT && millis() - prev > 1000)
      state = DONE;
    switch (state) {
      case WAIT:
        wiFiClient = wiFiServer.available();
        if (wiFiClient) {
          prev = millis();
          token = START;
          state = TOKENIZING;
          // no break, immediately tokenize the first buffer
        }
        else
          break;
      case TOKENIZING:
        if (wiFiClient.connected()) {
          int av = wiFiClient.available();
          if (av) {
            char len = wiFiClient.read(buffer, av < bufferLen ? av : bufferLen);
            for (unsigned i = 0;i < len;i++) {
              #ifdef DEBUG
                Serial.print((char)buffer[i]);
              #endif
              if(!tokenize(buffer[i])) break;
            }
            if (isError())
              state = ERROR;
            else if (token == STOP)
              state = BEGIN_RESP;
          }
        }
        break;
      case ERROR:
        #ifdef DEBUG
          Serial.println(errors[token]);
        #endif
        wiFiClient.print(F("HTTP/1.1 "));
        wiFiClient.println(errors[token]);
        wiFiClient.println(F("Connection: close"));
        wiFiClient.println();
        wiFiClient.flush();
        prev = millis();
        state = DONE;
        break;
      case BEGIN_RESP:
        #ifdef DEBUG
          Serial.println(F("HTTP/1.1 200 OK"));
        #endif
        wiFiClient.println(F("HTTP/1.1 200 OK"));
        printContentType(path, wiFiClient);
        wiFiClient.println(F("Connection: close"));  // no keep-alive...
        wiFiClient.println();
        if (printBody(true, path, wiFiClient))
          state = NEXT_RESP;
        else
          state = END_RESP;
        break;
      case NEXT_RESP:
        if (!printBody(false, path, wiFiClient)) 
          state = END_RESP;
        break;
      case END_RESP:
        wiFiClient.flush();
        prev = millis();
        state = DONE;
        break;
      case DONE:
        if (millis() - prev > 10) {
          wiFiClient.stop();
          state = WAIT;
        }
        break;
    }
  }
}

void HttpServer::begin() {
  unsigned int i = 0;
  wiFiServer.begin();
  //while (!wiFiServer && i++ < 10) {
    if (!wiFiServer)
    Serial.println("Failed to start WiFi server");
  // wiFiServer.end();
  //  wiFiServer.begin();
  //}
  state = WAIT;
}

void HttpServer::end() {
  wiFiServer.end();
}

void HttpServer::acceptPrinters(PrintContentType ct, PrintBody b) {
  printContentType = ct;
  printBody        = b;
}