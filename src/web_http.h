#ifndef WEB_HTTP_H
#define WEB_HTTP_H

#include <Arduino.h>

void webHttpBegin();
void webHttpLoop();
bool webHttpIsUp();
uint16_t webHttpListenPort();

#endif
