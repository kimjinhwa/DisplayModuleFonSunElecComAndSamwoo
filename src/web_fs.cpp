#include "web_fs.h"

#include <SPIFFS.h>

static File sWrite;
static File sRead;
static bool sReady = false;

bool webFsBegin()
{
  if (sReady)
  {
    return true;
  }
  if (!SPIFFS.begin(true))
  {
    Serial.println("[WEB] SPIFFS mount fail");
    return false;
  }
  sReady = true;
  Serial.printf("[WEB] SPIFFS used=%u total=%u\n",
                (unsigned)SPIFFS.usedBytes(), (unsigned)SPIFFS.totalBytes());
  return true;
}

static String normName(const String &name)
{
  String n = name;
  n.replace("\\", "/");
  const int slash = n.lastIndexOf('/');
  if (slash >= 0)
  {
    n = n.substring(slash + 1);
  }
  if (!n.startsWith("/"))
  {
    n = "/" + n;
  }
  return n;
}

bool webFsExists(const char *path)
{
  return sReady && SPIFFS.exists(path);
}

bool webFsOpenWrite(const String &name)
{
  if (!sReady)
  {
    return false;
  }
  if (sWrite)
  {
    sWrite.close();
  }
  const String path = normName(name);
  sWrite = SPIFFS.open(path, FILE_WRITE);
  Serial.printf("[WEB] upload start %s\n", path.c_str());
  return (bool)sWrite;
}

bool webFsWrite(const uint8_t *data, size_t len)
{
  if (!sWrite)
  {
    return false;
  }
  return sWrite.write(data, len) == len;
}

void webFsCloseWrite()
{
  if (sWrite)
  {
    Serial.printf("[WEB] upload end %u B\n", (unsigned)sWrite.size());
    sWrite.close();
  }
}

bool webFsReadOpen(const char *path, size_t *sizeOut)
{
  if (sRead)
  {
    sRead.close();
  }
  if (!sReady || !SPIFFS.exists(path))
  {
    return false;
  }
  sRead = SPIFFS.open(path, FILE_READ);
  if (!sRead)
  {
    return false;
  }
  if (sizeOut)
  {
    *sizeOut = sRead.size();
  }
  return true;
}

int webFsRead(uint8_t *buf, size_t len)
{
  if (!sRead)
  {
    return -1;
  }
  return (int)sRead.read(buf, len);
}

void webFsReadClose()
{
  if (sRead)
  {
    sRead.close();
  }
}

bool webUserLoad(WebUserInfo *out)
{
  if (!out)
  {
    return false;
  }
  memset(out, 0, sizeof(*out));
  strncpy(out->userid, "admin", sizeof(out->userid) - 1);
  strncpy(out->passwd, "admin", sizeof(out->passwd) - 1);
  if (!sReady || !SPIFFS.exists("/user.dat"))
  {
    return true;
  }
  File f = SPIFFS.open("/user.dat", FILE_READ);
  if (!f)
  {
    return true;
  }
  WebUserInfo tmp = {};
  const int n = f.read((uint8_t *)&tmp, sizeof(tmp));
  f.close();
  if (n == (int)sizeof(tmp) && tmp.userid[0] != '\0')
  {
    *out = tmp;
    out->userid[sizeof(out->userid) - 1] = '\0';
    out->passwd[sizeof(out->passwd) - 1] = '\0';
  }
  return true;
}

bool webUserSave(const WebUserInfo &in)
{
  if (!sReady)
  {
    return false;
  }
  File f = SPIFFS.open("/user.dat", FILE_WRITE);
  if (!f)
  {
    return false;
  }
  const size_t n = f.write((const uint8_t *)&in, sizeof(in));
  f.close();
  return n == sizeof(in);
}
