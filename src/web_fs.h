#ifndef WEB_FS_H
#define WEB_FS_H

#include <Arduino.h>

bool webFsBegin();
bool webFsExists(const char *path);
bool webFsOpenWrite(const String &name);
bool webFsWrite(const uint8_t *data, size_t len);
void webFsCloseWrite();
bool webFsReadOpen(const char *path, size_t *sizeOut);
int webFsRead(uint8_t *buf, size_t len);
void webFsReadClose();

struct WebUserInfo
{
  char userid[20];
  char passwd[20];
};

bool webUserLoad(WebUserInfo *out);
bool webUserSave(const WebUserInfo &in);

#endif
