/* eth_w610.cpp 에서만 include. Ethernet_Generic 단일 TU + 최소 HTTP. */

#include "web_http.h"
#include "web_fs.h"
#include "web_pages.h"
#include "main.h"
#include "Version.h"
#include "samwoo_poll.h"
#include "status_leds.h"
#include "board_rtc.h"
#include "eth_w610.h"
#include <ArduinoJson.h>
#include <esp_task_wdt.h>

class HttpListen : public EthernetServer
{
public:
  explicit HttpListen(uint16_t port) : EthernetServer(port) {}
  void begin(uint16_t port = 0) override
  {
    (void)port;
    EthernetServer::begin();
  }
};

static HttpListen *sHttp = nullptr;
static EthernetClient *sOut = nullptr;
static bool sWebUp = false;
static uint16_t sWebPort = 0;
static String sSession;
static uint32_t sSessionExp = 0;
static WebUserInfo sUser;
static String sCookie;
static String sBody;
static String sUri;
static String sMethod;
static String sExtraHeaders;

static constexpr uint32_t kSessionMs = 7UL * 24UL * 60UL * 60UL * 1000UL;
static constexpr uint32_t kHdrTimeoutMs = 4000;
static constexpr uint32_t kBodyTimeoutMs = 25000;

bool webHttpIsUp() { return sWebUp; }
uint16_t webHttpListenPort() { return sWebPort; }

static String cookieVal(const String &name)
{
  const String key = name + "=";
  int p = sCookie.indexOf(key);
  if (p < 0)
  {
    return "";
  }
  p += key.length();
  const int e = sCookie.indexOf(';', p);
  return (e < 0) ? sCookie.substring(p) : sCookie.substring(p, e);
}

static String makeToken()
{
  char buf[33];
  snprintf(buf, sizeof(buf), "%08x%08x%08x%08x",
           (unsigned)esp_random(), (unsigned)esp_random(),
           (unsigned)esp_random(), (unsigned)esp_random());
  return String(buf);
}

static bool sessionOk()
{
  if (sSession.length() == 0)
  {
    return false;
  }
  if ((int32_t)(millis() - sSessionExp) >= 0)
  {
    return false;
  }
  return cookieVal("session") == sSession;
}

static void sendRaw(int code, const char *ctype, const char *body, size_t bodyLen)
{
  if (!sOut)
  {
    return;
  }
  char line[96];
  snprintf(line, sizeof(line), "HTTP/1.1 %d %s\r\n", code, code == 200 ? "OK" : (code == 204 ? "No Content" : "ERR"));
  sOut->print(line);
  sOut->print("Connection: close\r\n");
  sOut->print("Access-Control-Allow-Origin: *\r\n");
  sOut->print("Access-Control-Allow-Headers: Content-Type, Cookie\r\n");
  sOut->print("Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n");
  sOut->print("Cache-Control: no-cache, no-store, must-revalidate\r\n");
  if (sExtraHeaders.length())
  {
    sOut->print(sExtraHeaders);
    sExtraHeaders = "";
  }
  if (ctype)
  {
    sOut->print("Content-Type: ");
    sOut->print(ctype);
    sOut->print("\r\n");
  }
  snprintf(line, sizeof(line), "Content-Length: %u\r\n\r\n", (unsigned)bodyLen);
  sOut->print(line);
  if (body && bodyLen)
  {
    sOut->write((const uint8_t *)body, bodyLen);
  }
}

static void sendJson(int code, const String &body)
{
  sendRaw(code, "application/json", body.c_str(), body.length());
}

static void sendHtmlP(const char *html)
{
  sendRaw(200, "text/html; charset=utf-8", html, strlen(html));
}

static void sendText(int code, const char *text)
{
  sendRaw(code, "text/plain", text, strlen(text));
}

static const char *ctypeOf(const String &u)
{
  if (u.endsWith(".css"))
    return "text/css";
  if (u.endsWith(".js"))
    return "text/javascript";
  if (u.endsWith(".json"))
    return "application/json";
  if (u.endsWith(".png"))
    return "image/png";
  if (u.endsWith(".ico"))
    return "image/x-icon";
  if (u.endsWith(".svg"))
    return "image/svg+xml";
  return "text/html; charset=utf-8";
}

static bool sendSpiffs(const char *ctype, const char *path)
{
  size_t sz = 0;
  if (!webFsReadOpen(path, &sz) || !sOut)
  {
    return false;
  }
  char line[96];
  sOut->print("HTTP/1.1 200 OK\r\nConnection: close\r\n");
  sOut->print("Cache-Control: no-cache, no-store, must-revalidate\r\n");
  sOut->print("Content-Type: ");
  sOut->print(ctype);
  sOut->print("\r\n");
  snprintf(line, sizeof(line), "Content-Length: %u\r\n\r\n", (unsigned)sz);
  sOut->print(line);
  uint8_t buf[512];
  for (;;)
  {
    const int n = webFsRead(buf, sizeof(buf));
    if (n <= 0)
    {
      break;
    }
    sOut->write(buf, (size_t)n);
    esp_task_wdt_reset();
  }
  webFsReadClose();
  return true;
}

static String uriLowerOf(String u)
{
  const int q = u.indexOf('?');
  if (q >= 0)
  {
    u.remove(q);
  }
  u.toLowerCase();
  return u;
}

static bool serveByUri()
{
  const String u = uriLowerOf(sUri);
  if (u == "/fileupload" || u == "/fileupload.html")
  {
    sendHtmlP(FILE_UPLOAD_HTML);
    return true;
  }
  if (u == "/serverindex" || u == "/serverindex.html")
  {
    sendHtmlP(SERVER_INDEX_HTML);
    return true;
  }
  if (u == "/" || u == "/index.html")
  {
    if (sendSpiffs("text/html; charset=utf-8", "/index.html"))
      return true;
    sendHtmlP(FILE_UPLOAD_HTML);
    return true;
  }

  struct Map
  {
    const char *uri;
    const char *ctype;
    const char *path;
  };
  static const Map kMap[] = {
      {"/login.html", "text/html; charset=utf-8", "/login.html"},
      {"/login.css", "text/css", "/login.css"},
      {"/index.js", "text/javascript", "/index.js"},
      {"/settings.html", "text/html; charset=utf-8", "/settings.html"},
      {"/settings.js", "text/javascript", "/settings.js"},
      {"/snmptest.html", "text/html; charset=utf-8", "/snmpTest.html"},
      {"/snmptest.js", "text/javascript", "/snmpTest.js"},
      {"/help.html", "text/html; charset=utf-8", "/help.html"},
  };
  for (const auto &m : kMap)
  {
    if (u == m.uri)
    {
      return sendSpiffs(m.ctype, m.path);
    }
  }
  if (u.endsWith(".html") || u.endsWith(".js") || u.endsWith(".css") || u.endsWith(".ico") ||
      u.endsWith(".png") || u.endsWith(".svg"))
  {
    String path = u;
    if (!path.startsWith("/"))
    {
      path = "/" + path;
    }
    return sendSpiffs(ctypeOf(u), path.c_str());
  }
  return false;
}

static void addPackJson(JsonObject p, uint8_t i)
{
  p["ok"] = samwooOk[i];
  p["soc"] = (int)samwooReg(i, SAMWOO_REG_SOC);
  p["soh"] = (int)samwooReg(i, SAMWOO_REG_SOH);
  p["capacity"] = samwooReg(i, SAMWOO_REG_CAP) / 10.0;
  p["volt"] = samwooReg(i, SAMWOO_REG_VOLT) / 10.0;
  p["amp"] = ((int16_t)samwooReg(i, SAMWOO_REG_CUR)) / 10.0f;
  uint8_t nCell = (uint8_t)samwooReg(i, SAMWOO_REG_CELLNUM);
  if (nCell > SAMWOO_REG_CELLS)
  {
    nCell = SAMWOO_REG_CELLS;
  }
  p["cellNum"] = nCell;
  uint16_t h = 0, l = 0xFFFF;
  JsonArray cells = p.createNestedArray("cells");
  for (uint8_t c = 0; c < SAMWOO_REG_CELLS; ++c)
  {
    const uint16_t mv = samwooReg(i, (uint16_t)(SAMWOO_REG_CELL0 + c));
    cells.add(mv);
    if (c < nCell && mv > 0)
    {
      if (mv > h)
        h = mv;
      if (mv < l)
        l = mv;
    }
  }
  if (l == 0xFFFF)
  {
    l = 0;
  }
  p["hvol"] = h / 1000.0;
  p["lvol"] = l / 1000.0;
  p["diff"] = (int)h - (int)l;
  JsonArray temps = p.createNestedArray("temps");
  float tsum = 0;
  uint8_t tcnt = 0;
  for (uint8_t t = 0; t < 8; ++t)
  {
    const float tc = ((int16_t)samwooReg(i, (uint16_t)(SAMWOO_REG_TEMP0 + t))) / 10.0f;
    temps.add(tc);
    if (t < 4)
    {
      tsum += tc;
      tcnt++;
    }
  }
  p["temp"] = tcnt ? (tsum / tcnt) : 0;
  const uint16_t relay = samwooReg(i, SAMWOO_REG_RELAY);
  p["charge"] = (relay & 0x0001) != 0;
  p["discharge"] = (relay & 0x0002) != 0;
  p["fault"] = (int)samwooReg(i, SAMWOO_REG_FAULT);
  p["protect"] = (int)samwooReg(i, SAMWOO_REG_PROTECT);
  p["warning"] = (int)samwooReg(i, SAMWOO_REG_WARNING);
  p["warn"] = statusLedsHasAlarm(i);
  const char *cause = statusLedsAlarmCause(i);
  if (!samwooOk[i])
  {
    p["alarm"] = "통신";
    p["comm"] = false;
  }
  else if (cause)
  {
    p["alarm"] = cause;
    p["comm"] = true;
  }
  else
  {
    p["alarm"] = "정상";
    p["comm"] = true;
  }
}

static void handleBms()
{
  DynamicJsonDocument doc(3072);
  doc["ok"] = true;
  doc["name"] = ipAddress_struct.deviceName;
  doc["version"] = VERSION;
  doc["uptime"] = millis() / 1000;
  doc["ip"] = ethW610LocalIP().toString();
  doc["mac"] = ethW610MacString();
  doc["err"] = samwooErrorCount;
  JsonArray packs = doc.createNestedArray("packs");
  addPackJson(packs.createNestedObject(), 0);
  addPackJson(packs.createNestedObject(), 1);
  String out;
  serializeJson(doc, out);
  sendJson(200, out);
}

static int oidInt(const String &oid, bool *found)
{
  *found = true;
  String o = oid;
  if (!o.startsWith("."))
  {
    o = "." + o;
  }
  auto packOid = [&](uint8_t pack, const char *suffix) -> bool {
    char buf[48];
    snprintf(buf, sizeof(buf), ".1.3.6.1.2.1.32.%u%s", (unsigned)(pack + 1), suffix);
    return o == buf;
  };
  for (uint8_t p = 0; p < 2; ++p)
  {
    if (packOid(p, ".3.0"))
      return samwooReg(p, SAMWOO_REG_SOC);
    if (packOid(p, ".8.0"))
      return (int16_t)samwooReg(p, SAMWOO_REG_VOLT);
    if (packOid(p, ".2.0"))
      return (int16_t)samwooReg(p, SAMWOO_REG_CUR);
    if (packOid(p, ".4.0"))
      return samwooReg(p, SAMWOO_REG_CAP);
    if (packOid(p, ".9.0"))
      return samwooReg(p, SAMWOO_REG_SOH);
    if (packOid(p, ".7.0"))
      return samwooReg(p, SAMWOO_REG_VER);
    if (packOid(p, ".11.0"))
      return samwooReg(p, SAMWOO_REG_VMAX);
    if (packOid(p, ".12.0"))
      return samwooReg(p, SAMWOO_REG_VMIN);
    if (packOid(p, ".15.0"))
      return samwooReg(p, SAMWOO_REG_CELLNUM);
    if (packOid(p, ".6.1.0"))
      return samwooReg(p, SAMWOO_REG_FAULT);
    if (packOid(p, ".10.0"))
      return samwooReg(p, SAMWOO_REG_PROTECT);
    if (packOid(p, ".6.2.0"))
      return samwooReg(p, SAMWOO_REG_WARNING);
    if (packOid(p, ".6.4.0"))
      return (samwooReg(p, SAMWOO_REG_RELAY) & 1) ? 1 : 0;
    if (packOid(p, ".6.5.0"))
      return (samwooReg(p, SAMWOO_REG_RELAY) & 2) ? 1 : 0;
    for (uint8_t c = 1; c <= SAMWOO_REG_CELLS; ++c)
    {
      char sfx[16];
      snprintf(sfx, sizeof(sfx), ".1.%u.0", (unsigned)c);
      if (packOid(p, sfx))
        return samwooReg(p, (uint16_t)(SAMWOO_REG_CELL0 + c - 1));
    }
    for (uint8_t t = 1; t <= SAMWOO_REG_TEMPS; ++t)
    {
      char sfx[16];
      snprintf(sfx, sizeof(sfx), ".5.%u.0", (unsigned)t);
      if (packOid(p, sfx))
        return (int16_t)samwooReg(p, (uint16_t)(SAMWOO_REG_TEMP0 + t - 1));
    }
  }
  if (o == ".1.3.6.1.2.1.1.7.0")
    return 72;
  *found = false;
  return 0;
}

static bool parseJsonBody(DynamicJsonDocument &doc)
{
  if (sBody.length() == 0)
  {
    return false;
  }
  return deserializeJson(doc, sBody) == DeserializationError::Ok;
}

static void handleSnmpGet()
{
  if (!sessionOk())
  {
    sendJson(401, "{\"ok\":false,\"error\":\"auth\"}");
    return;
  }
  String oid;
  DynamicJsonDocument req(256);
  if (parseJsonBody(req))
  {
    oid = req["oid"].as<String>();
  }
  oid.trim();
  if (oid == ".1.3.6.1.2.1.1.1.0" || oid == "1.3.6.1.2.1.1.1.0")
  {
    sendJson(200, "{\"ok\":true,\"oid\":\".1.3.6.1.2.1.1.1.0\",\"value\":\"Samwoo BMS Display Pack1+Pack2\"}");
    return;
  }
  if (oid == ".1.3.6.1.2.1.1.5.0" || oid == "1.3.6.1.2.1.1.5.0")
  {
    sendJson(200, String("{\"ok\":true,\"oid\":\".1.3.6.1.2.1.1.5.0\",\"value\":\"") + ipAddress_struct.deviceName + "\"}");
    return;
  }
  bool found = false;
  const int v = oidInt(oid, &found);
  if (!found)
  {
    sendJson(404, "{\"ok\":false,\"error\":\"unknown oid\"}");
    return;
  }
  char buf[160];
  snprintf(buf, sizeof(buf), "{\"ok\":true,\"oid\":\"%s\",\"value\":%d}", oid.c_str(), v);
  sendJson(200, buf);
}

static void handleNetworkGet()
{
  DynamicJsonDocument doc(512);
  doc["ok"] = true;
  doc["ip"] = IPAddress(ipAddress_struct.IPADDRESS).toString();
  doc["subnet"] = IPAddress(ipAddress_struct.SUBNETMASK).toString();
  doc["gateway"] = IPAddress(ipAddress_struct.GATEWAY).toString();
  doc["webEnabled"] = ipAddress_struct.WEBSERVERPORT != 0;
  doc["webPort"] = ipAddress_struct.WEBSERVERPORT ? ipAddress_struct.WEBSERVERPORT : 80;
  doc["name"] = ipAddress_struct.deviceName;
  doc["ssid"] = ipAddress_struct.ssid;
  doc["version"] = VERSION;
  doc["mac"] = ethW610MacString();
  BoardRtcTime t = {};
  if (boardRtcRead(&t))
  {
    char ts[24];
    snprintf(ts, sizeof(ts), "%04u-%02u-%02u %02u:%02u:%02u",
             t.year, t.month, t.day, t.hour, t.minute, t.second);
    doc["time"] = ts;
  }
  String out;
  serializeJson(doc, out);
  sendJson(200, out);
}

static void handleNetworkPost()
{
  if (!sessionOk())
  {
    sendJson(401, "{\"ok\":false,\"error\":\"auth\"}");
    return;
  }
  DynamicJsonDocument req(768);
  if (!parseJsonBody(req))
  {
    sendJson(400, "{\"ok\":false,\"error\":\"json\"}");
    return;
  }
  bool reboot = false;
  if (req.containsKey("ip"))
  {
    IPAddress a, b, c;
    if (a.fromString(req["ip"].as<const char *>()) &&
        b.fromString(req["subnet"] | "255.255.255.0") &&
        c.fromString(req["gateway"] | "192.168.0.1"))
    {
      if ((uint32_t)a != ipAddress_struct.IPADDRESS ||
          (uint32_t)b != ipAddress_struct.SUBNETMASK ||
          (uint32_t)c != ipAddress_struct.GATEWAY)
      {
        reboot = true;
      }
      ipAddress_struct.IPADDRESS = (uint32_t)a;
      ipAddress_struct.SUBNETMASK = (uint32_t)b;
      ipAddress_struct.GATEWAY = (uint32_t)c;
    }
  }
  if (req.containsKey("name"))
  {
    strncpy(ipAddress_struct.deviceName, req["name"].as<const char *>(), sizeof(ipAddress_struct.deviceName) - 1);
    ipAddress_struct.deviceName[sizeof(ipAddress_struct.deviceName) - 1] = '\0';
  }
  if (req.containsKey("ssid"))
  {
    strncpy(ipAddress_struct.ssid, req["ssid"].as<const char *>(), sizeof(ipAddress_struct.ssid) - 1);
    ipAddress_struct.ssid[sizeof(ipAddress_struct.ssid) - 1] = '\0';
  }
  if (req.containsKey("pass"))
  {
    const char *pw = req["pass"].as<const char *>();
    if (pw && strcmp(pw, "none") == 0)
    {
      ipAddress_struct.password[0] = '\0';
    }
    else if (pw)
    {
      strncpy(ipAddress_struct.password, pw, sizeof(ipAddress_struct.password) - 1);
      ipAddress_struct.password[sizeof(ipAddress_struct.password) - 1] = '\0';
    }
  }
  if (req.containsKey("webEnabled"))
  {
    const bool en = req["webEnabled"];
    uint16_t port = req["webPort"] | 80;
    if (port < 1)
    {
      port = 80;
    }
    const uint16_t next = en ? port : 0;
    if (next != ipAddress_struct.WEBSERVERPORT)
    {
      reboot = true;
    }
    ipAddress_struct.WEBSERVERPORT = next;
  }
  if (req.containsKey("userid") && req.containsKey("passwd"))
  {
    WebUserInfo u = {};
    strncpy(u.userid, req["userid"].as<const char *>(), sizeof(u.userid) - 1);
    strncpy(u.passwd, req["passwd"].as<const char *>(), sizeof(u.passwd) - 1);
    webUserSave(u);
    sUser = u;
  }
  if (req.containsKey("time"))
  {
    BoardRtcTime t = {};
    const char *ts = req["time"].as<const char *>();
    int y = 0, mo = 0, d = 0, h = 0, mi = 0, se = 0;
    if (sscanf(ts, "%d-%d-%d %d:%d:%d", &y, &mo, &d, &h, &mi, &se) == 6)
    {
      t.year = (uint16_t)y;
      t.month = (uint8_t)mo;
      t.day = (uint8_t)d;
      t.hour = (uint8_t)h;
      t.minute = (uint8_t)mi;
      t.second = (uint8_t)se;
      boardRtcWrite(t);
    }
  }
  nvsSave();
  setMemoryDataToLCD();
  if (reboot)
  {
    sendJson(200, "{\"ok\":true,\"reboot\":true}");
    delay(400);
    ESP.restart();
    return;
  }
  sendJson(200, "{\"ok\":true,\"reboot\":false}");
}

static void handleLogin()
{
  String userid, passwd;
  DynamicJsonDocument req(256);
  if (parseJsonBody(req))
  {
    userid = req["userid"].as<String>();
    passwd = req["passwd"].as<String>();
  }
  if (userid != String(sUser.userid) || passwd != String(sUser.passwd))
  {
    sendJson(401, "{\"ok\":false,\"error\":\"invalid credentials\"}");
    return;
  }
  sSession = makeToken();
  sSessionExp = millis() + kSessionMs;
  sExtraHeaders = "Set-Cookie: session=" + sSession + "; Max-Age=604800; Path=/; HttpOnly; SameSite=Lax\r\n";
  sendJson(200, "{\"ok\":true}");
}

static void handleMe()
{
  if (sessionOk())
    sendJson(200, "{\"ok\":true,\"authenticated\":true}");
  else
    sendJson(200, "{\"ok\":true,\"authenticated\":false}");
}

static void handleLogout()
{
  sSession = "";
  sExtraHeaders = "Set-Cookie: session=; Max-Age=0; Path=/\r\n";
  sendJson(200, "{\"ok\":true}");
}

static int readByteWait(EthernetClient &c, uint32_t t0, uint32_t timeout)
{
  while ((int32_t)(millis() - t0) < (int32_t)timeout)
  {
    if (c.available() > 0)
    {
      return c.read();
    }
    if (!c.connected())
    {
      return -1;
    }
    esp_task_wdt_reset();
    delay(1);
  }
  return -1;
}

static bool readHeaders(EthernetClient &c, String &hdrs)
{
  hdrs = "";
  const uint32_t t0 = millis();
  int crlf = 0;
  while ((int32_t)(millis() - t0) < (int32_t)kHdrTimeoutMs)
  {
    const int ch = readByteWait(c, t0, kHdrTimeoutMs);
    if (ch < 0)
    {
      return hdrs.length() > 0;
    }
    hdrs += (char)ch;
    if (hdrs.length() > 4096)
    {
      return false;
    }
    if (ch == '\n')
    {
      crlf++;
      if (crlf >= 2)
      {
        return true;
      }
    }
    else if (ch != '\r')
    {
      crlf = 0;
    }
  }
  return false;
}

static String headerVal(const String &hdrs, const char *name)
{
  String key = String(name) + ":";
  String low = hdrs;
  String keyLow = key;
  low.toLowerCase();
  keyLow.toLowerCase();
  int p = low.indexOf(keyLow);
  if (p < 0)
  {
    return "";
  }
  p = hdrs.indexOf(':', p) + 1;
  while (p < (int)hdrs.length() && hdrs[p] == ' ')
  {
    p++;
  }
  int e = hdrs.indexOf('\r', p);
  if (e < 0)
  {
    e = hdrs.indexOf('\n', p);
  }
  if (e < 0)
  {
    return hdrs.substring(p);
  }
  return hdrs.substring(p, e);
}

static bool readBody(EthernetClient &c, int contentLen)
{
  sBody = "";
  if (contentLen <= 0)
  {
    return true;
  }
  if (contentLen > 4096)
  {
    return false;
  }
  const uint32_t t0 = millis();
  sBody.reserve(contentLen);
  while ((int)sBody.length() < contentLen)
  {
    const int ch = readByteWait(c, t0, kBodyTimeoutMs);
    if (ch < 0)
    {
      return false;
    }
    sBody += (char)ch;
  }
  return true;
}

static String extractFilename(const String &partHdr)
{
  int p = partHdr.indexOf("filename=");
  if (p < 0)
  {
    return "upload.bin";
  }
  p += 9;
  if (p < (int)partHdr.length() && partHdr[p] == '"')
  {
    p++;
    const int e = partHdr.indexOf('"', p);
    return e > p ? partHdr.substring(p, e) : "upload.bin";
  }
  const int e = partHdr.indexOf('\r', p);
  return partHdr.substring(p, e > p ? e : partHdr.length());
}

static bool handleUpload(EthernetClient &c, const String &contentType, int contentLen)
{
  String b = contentType;
  int p = b.indexOf("boundary=");
  if (p < 0)
  {
    sendText(400, "no boundary");
    return true;
  }
  String boundary = b.substring(p + 9);
  boundary.trim();
  if (boundary.startsWith("\"") && boundary.endsWith("\"") && boundary.length() >= 2)
  {
    boundary = boundary.substring(1, boundary.length() - 1);
  }
  const String mark = "--" + boundary;
  const String endFile = "\r\n--" + boundary;
  const uint32_t t0 = millis();
  String acc;
  acc.reserve(256);
  bool inHeaders = false;
  bool inFile = false;
  String partHdr;
  int remain = contentLen > 0 ? contentLen : 256 * 1024;
  auto take = [&]() -> int {
    if (remain <= 0)
    {
      return -1;
    }
    const int ch = readByteWait(c, t0, kBodyTimeoutMs);
    if (ch >= 0)
    {
      remain--;
    }
    return ch;
  };

  while (!inFile)
  {
    const int ch = take();
    if (ch < 0)
    {
      sendText(400, "upload header");
      return true;
    }
    acc += (char)ch;
    if (!inHeaders)
    {
      if (acc.endsWith(mark + "\r\n"))
      {
        inHeaders = true;
        partHdr = "";
        acc = "";
      }
      else if (acc.length() > 256)
      {
        acc.remove(0, acc.length() - (int)mark.length() - 4);
      }
    }
    else
    {
      partHdr += (char)ch;
      if (partHdr.endsWith("\r\n\r\n"))
      {
        const String name = extractFilename(partHdr);
        webFsOpenWrite(name);
        inFile = true;
      }
      else if (partHdr.length() > 1024)
      {
        sendText(400, "part header");
        return true;
      }
    }
  }

  acc = "";
  const size_t keep = endFile.length();
  while (true)
  {
    const int ch = take();
    if (ch < 0)
    {
      break;
    }
    acc += (char)ch;
    if (acc.length() > keep)
    {
      const uint8_t outc = (uint8_t)acc[0];
      webFsWrite(&outc, 1);
      acc.remove(0, 1);
    }
    if (acc == endFile)
    {
      break;
    }
    if (acc.length() == keep && !endFile.startsWith(acc))
    {
      /* keep matching prefix only */
      while (acc.length() && !endFile.startsWith(acc))
      {
        const uint8_t outc = (uint8_t)acc[0];
        webFsWrite(&outc, 1);
        acc.remove(0, 1);
      }
    }
    esp_task_wdt_reset();
  }
  webFsCloseWrite();
  sendText(200, "OK");
  return true;
}

static bool dispatchApi()
{
  const String u = uriLowerOf(sUri);
  if (sMethod == "OPTIONS" &&
      (u == "/api/login" || u == "/api/me" || u == "/api/logout" || u == "/api/bms" ||
       u == "/api/network-config" || u == "/api/snmp-get" || u == "/upload"))
  {
    sendRaw(204, "text/plain", "", 0);
    return true;
  }
  if (u == "/api/login" && sMethod == "POST")
  {
    handleLogin();
    return true;
  }
  if (u == "/api/me" && sMethod == "GET")
  {
    handleMe();
    return true;
  }
  if (u == "/api/logout" && sMethod == "POST")
  {
    handleLogout();
    return true;
  }
  if (u == "/api/bms" && sMethod == "GET")
  {
    handleBms();
    return true;
  }
  if (u == "/api/network-config" && sMethod == "GET")
  {
    handleNetworkGet();
    return true;
  }
  if (u == "/api/network-config" && sMethod == "POST")
  {
    handleNetworkPost();
    return true;
  }
  if (u == "/api/snmp-get" && sMethod == "POST")
  {
    handleSnmpGet();
    return true;
  }
  return false;
}

static void handleClient(EthernetClient &c)
{
  sOut = &c;
  sCookie = "";
  sBody = "";
  sUri = "";
  sMethod = "";
  sExtraHeaders = "";

  String hdrs;
  if (!readHeaders(c, hdrs))
  {
    sendText(400, "bad request");
    c.stop();
    sOut = nullptr;
    return;
  }
  const int sp1 = hdrs.indexOf(' ');
  const int sp2 = hdrs.indexOf(' ', sp1 + 1);
  if (sp1 < 0 || sp2 < 0)
  {
    sendText(400, "bad request");
    c.stop();
    sOut = nullptr;
    return;
  }
  sMethod = hdrs.substring(0, sp1);
  sMethod.toUpperCase();
  sUri = hdrs.substring(sp1 + 1, sp2);
  sCookie = headerVal(hdrs, "Cookie");
  const String ctype = headerVal(hdrs, "Content-Type");
  const int contentLen = headerVal(hdrs, "Content-Length").toInt();
  const String u = uriLowerOf(sUri);

  if (sMethod == "POST" && u == "/upload")
  {
    handleUpload(c, ctype, contentLen);
  }
  else
  {
    if (sMethod == "POST" && contentLen > 0)
    {
      if (!readBody(c, contentLen))
      {
        sendText(400, "body");
        c.stop();
        sOut = nullptr;
        return;
      }
    }
    if (!dispatchApi() && !serveByUri())
    {
      sendText(404, "not found");
    }
  }
  delay(5);
  c.stop();
  sOut = nullptr;
}

void webHttpBegin()
{
  webUserLoad(&sUser);
  if (ipAddress_struct.WEBSERVERPORT == 0)
  {
    sWebUp = false;
    sWebPort = 0;
    Serial.println("[WEB] disabled (WEBSERVERPORT=0)");
    return;
  }
  sWebPort = ipAddress_struct.WEBSERVERPORT;
  if (sHttp)
  {
    delete sHttp;
    sHttp = nullptr;
  }
  sHttp = new HttpListen(sWebPort);
  sHttp->begin(sWebPort);
  sWebUp = true;
  Serial.printf("[WEB] http://%s:%u  login=admin/admin\n",
                ethW610LocalIP().toString().c_str(), (unsigned)sWebPort);
}

void webHttpLoop()
{
  if (!sWebUp || !sHttp)
  {
    return;
  }
  EthernetClient c = sHttp->available();
  if (c)
  {
    handleClient(c);
  }
}
