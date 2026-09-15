#include "UdpIpFinderService.h"

static String normalizeMacForCompare(const String &mac)
{
  String out = mac;
  out.trim();
  out.toUpperCase();
  out.replace("-", ":");
  return out;
}

bool UdpIpFinderService::begin(UDP *udp)
{
  sock_ = udp;
  udp_ = udp;
  if (!udp_ || !udp_->begin(config_.port))
  {
    Serial.println("[IPFINDER] Failed to start UDP");
    udp_ = nullptr;
    return false;
  }
  holdUntil_ = 0;
  txFail_ = 0;
  Serial.printf("[IPFINDER] UDP %u started\n", config_.port);
  return true;
}

void UdpIpFinderService::holdSockets(uint32_t ms)
{
  if (udp_)
  {
    udp_->stop();
    udp_ = nullptr;
  }
  holdUntil_ = millis() + ms;
  Serial.printf("[IPFINDER] UDP hold %u ms (no PHY; TX/size fail)\n", (unsigned)ms);
}

void UdpIpFinderService::setConfig(const Config &config)
{
  config_ = config;
}

void UdpIpFinderService::setCallbacks(BuildSnapshotCallback buildSnapshot,
                                      ApplyNetworkCallback applyNetwork,
                                      RestartCallback restartDevice,
                                      void *ctx)
{
  buildSnapshotCb_ = buildSnapshot;
  applyNetworkCb_ = applyNetwork;
  restartCb_ = restartDevice;
  ctx_ = ctx;
}

void UdpIpFinderService::setTrapTestCallback(TrapTestCallback trapTest, void *ctx)
{
  trapTestCb_ = trapTest;
  trapTestCtx_ = ctx;
}

void UdpIpFinderService::poll()
{
  if (holdUntil_)
  {
    if ((int32_t)(millis() - holdUntil_) < 0)
    {
      return;
    }
    holdUntil_ = 0;
    if (sock_ && !begin(sock_))
    {
      holdSockets(4000);
      return;
    }
  }
  if (!udp_)
  {
    return;
  }
  for (int n = 0; n < 4; ++n)
  {
    const int packetSize = udp_->parsePacket();
    if (packetSize <= 0)
    {
      return;
    }
    /* Cable unplug: Wiznet returns a fake size. flush() SPI-drains and trips WDT/crash. */
    if (packetSize > 1472)
    {
      holdSockets(4000);
      return;
    }

    static char incomingPacket[1024];
    memset(incomingPacket, 0x00, sizeof(incomingPacket));
    const int maxReadable = static_cast<int>(sizeof(incomingPacket) - 1);
    const int readLen = (packetSize < maxReadable) ? packetSize : maxReadable;
    const int len = udp_->read(incomingPacket, readLen);
    if (len <= 0)
    {
      udp_->flush();
      return;
    }
    incomingPacket[len] = '\0';
    /* Wiznet: leftover bytes from a truncated datagram corrupt the next parsePacket. */
    if (packetSize > len)
    {
      udp_->flush();
    }

    if (incomingPacket[0] != '{')
    {
      continue;
    }
    if (strstr(incomingPacket, "\"cmd\"") == NULL)
    {
      continue;
    }

    static StaticJsonDocument<1024> request;
    request.clear();
    const DeserializationError error = deserializeJson(request, incomingPacket);
    if (error)
    {
      continue;
    }

    const char *cmd = request["cmd"] | "";
    Serial.printf("[IPFINDER] RX cmd=%s from %s:%u\n",
                  cmd,
                  udp_->remoteIP().toString().c_str(),
                  udp_->remotePort());
    if (strcmp(cmd, "SET_NETWORK_CONFIG") == 0)
    {
      handleSetNetworkConfig(request);
      continue;
    }
    if (strcmp(cmd, "TRAP_TEST") == 0)
    {
      handleTrapTest(request);
      continue;
    }
    if (strcmp(cmd, "DEVICE_DISCOVERY") == 0)
    {
      handleDeviceDiscovery(request);
    }
  }
}

void UdpIpFinderService::handleSetNetworkConfig(const JsonDocument &request)
{
  if (!buildSnapshotCb_ || !applyNetworkCb_)
  {
    return;
  }

  DeviceSnapshot snapshot = {};
  if (!buildSnapshotCb_(snapshot, ctx_))
  {
    return;
  }

  const char *targetMac = request["target"] | "";
  const String targetNorm = normalizeMacForCompare(String(targetMac));
  const String localNorm = normalizeMacForCompare(snapshot.mac);
  if (localNorm.length() == 0 || targetNorm != localNorm)
  {
    Serial.printf("[IPFINDER] SET ignored: target MAC mismatch target=%s local=%s\n",
                  targetNorm.c_str(),
                  localNorm.c_str());
    return;
  }

  const char *ipStr = request["config"]["network"]["ip"] | "";
  const char *subnetStr = request["config"]["network"]["subnet"] | "";
  const char *gatewayStr = request["config"]["network"]["gateway"] | "";

  NetworkConfig cfg = {};
  if (!(cfg.ip.fromString(ipStr) && cfg.subnet.fromString(subnetStr) && cfg.gateway.fromString(gatewayStr)))
  {
    return;
  }
  const JsonVariantConst trapObj = request["config"]["network"]["trap"];
  const JsonVariantConst webObj = request["config"]["network"]["web"];
  if (!webObj.isNull())
  {
    cfg.webSpecified = true;
    cfg.webEnabled = webObj["enabled"] | true;
    uint16_t p = static_cast<uint16_t>(webObj["port"] | cfg.webPort);
    if (p < 1)
    {
      p = 80;
    }
    cfg.webPort = p;
  }
  const JsonVariantConst webPortDirect = request["config"]["network"]["webserverport"];
  if (!webPortDirect.isNull())
  {
    cfg.webSpecified = true;
    const int p = webPortDirect.as<int>();
    if (p >= 1 && p <= 65535)
    {
      cfg.webPort = static_cast<uint16_t>(p);
    }
  }
  if (!trapObj.isNull())
  {
    cfg.trapEnabled = trapObj["enabled"] | false;
    const JsonArrayConst destinations = trapObj["destinations"].as<JsonArrayConst>();
    if (!destinations.isNull())
    {
      for (JsonVariantConst v : destinations)
      {
        if (cfg.trapDestinationCount >= 5)
        {
          break;
        }
        const char *ipText = v.as<const char *>();
        if (ipText && cfg.trapDestinations[cfg.trapDestinationCount].fromString(ipText))
        {
          cfg.trapDestinationCount++;
        }
      }
    }
  }

  const bool updated = applyNetworkCb_(cfg, ctx_);
  Serial.printf("[IPFINDER] SET apply result=%s ip=%s gw=%s subnet=%s web=%u port=%u trap=%d destCount=%u\n",
                updated ? "success" : "failed",
                cfg.ip.toString().c_str(),
                cfg.gateway.toString().c_str(),
                cfg.subnet.toString().c_str(),
                static_cast<unsigned>(cfg.webEnabled ? 1U : 0U),
                static_cast<unsigned>(cfg.webPort),
                cfg.trapEnabled ? 1 : 0,
                cfg.trapDestinationCount);

  StaticJsonDocument<256> response;
  response["cmd"] = "NETWORK_CONFIG_RESPONSE";
  response["ver"] = "1.0";
  response["msgId"] = request["msgId"];
  response["status"] = updated ? "success" : "failed";
  response["message"] = updated ? "Network config updated. Rebooting..." : "Network config update failed";
  sendJsonResponse(response);

  if (updated && restartCb_)
  {
    delay(1000);
    restartCb_(ctx_);
  }
}

void UdpIpFinderService::handleTrapTest(const JsonDocument &request)
{
  StaticJsonDocument<384> response;
  response["cmd"] = "TRAP_TEST_RESPONSE";
  response["ver"] = "1.0";
  response["msgId"] = request["msgId"];
  response["timestamp"] = request["timestamp"];

  if (!buildSnapshotCb_)
  {
    response["status"] = "failed";
    response["message"] = "Device snapshot unavailable";
    sendJsonResponse(response);
    return;
  }

  DeviceSnapshot snapshot = {};
  if (!buildSnapshotCb_(snapshot, ctx_))
  {
    response["status"] = "failed";
    response["message"] = "Device snapshot failed";
    sendJsonResponse(response);
    return;
  }

  response["mac"] = snapshot.mac;

  const char *targetMac = request["target"] | "";
  const String targetNorm = normalizeMacForCompare(String(targetMac));
  const String localNorm = normalizeMacForCompare(snapshot.mac);
  if (localNorm.length() == 0 || targetNorm != localNorm)
  {
    Serial.printf("[IPFINDER] TRAP_TEST ignored: MAC mismatch target=%s local=%s\n",
                  targetNorm.c_str(),
                  localNorm.c_str());
    response["status"] = "failed";
    response["message"] = "MAC mismatch";
    sendJsonResponse(response);
    return;
  }

  if (!trapTestCb_)
  {
    response["status"] = "failed";
    response["message"] = "Trap test not supported";
    sendJsonResponse(response);
    return;
  }

  const bool ok = trapTestCb_(trapTestCtx_);
  response["status"] = ok ? "success" : "failed";
  response["message"] = ok ? "SNMP trap test packet sent" : "Trap test failed (check TRAP IP fields and firmware)";
  sendJsonResponse(response);
  Serial.printf("[IPFINDER] TRAP_TEST result=%s\n", ok ? "success" : "failed");
}

void UdpIpFinderService::handleDeviceDiscovery(const JsonDocument &request)
{
  if (!buildSnapshotCb_)
  {
    return;
  }

  DeviceSnapshot snapshot = {};
  if (!buildSnapshotCb_(snapshot, ctx_))
  {
    return;
  }

  StaticJsonDocument<768> response;
  response["cmd"] = "DEVICE_RESPONSE";
  response["ver"] = snapshot.version;
  response["hostname"] = snapshot.hostname;
  response["msgId"] = request["msgId"];
  response["timestamp"] = request["timestamp"];

  JsonObject device = response.createNestedObject("device");
  const char *type = snapshot.type.length() ? snapshot.type.c_str() : config_.deviceType.c_str();
  device["type"] = type;
  device["mac"] = snapshot.mac;
  JsonObject network = device.createNestedObject("network");
  network["ip"] = snapshot.ip.toString();
  network["subnet"] = snapshot.subnet.toString();
  network["gateway"] = snapshot.gateway.toString();
  network["webserverport"] = snapshot.webPort;
  JsonObject web = network.createNestedObject("web");
  web["enabled"] = snapshot.webEnabled;
  web["port"] = snapshot.webPort;
  JsonObject trap = network.createNestedObject("trap");
  trap["enabled"] = snapshot.trapEnabled;
  JsonArray trapDestinations = trap.createNestedArray("destinations");
  for (uint8_t i = 0; i < snapshot.trapDestinationCount && i < 5; ++i)
  {
    trapDestinations.add(snapshot.trapDestinations[i].toString());
  }
  device["status"] = snapshot.status;
  device["uptime"] = snapshot.uptimeSeconds;

  sendJsonResponse(response);
  Serial.printf("[IPFINDER] DISCOVERY response sent to broadcast port=%u\n", udp_->remotePort());
}

static bool sendUdpPayload(UDP *udp, IPAddress ip, uint16_t port, const String &payload)
{
  if (!udp->beginPacket(ip, port))
  {
    Serial.printf("[IPFINDER] beginPacket fail %s:%u\n", ip.toString().c_str(), (unsigned)port);
    return false;
  }
  udp->print(payload);
  if (!udp->endPacket())
  {
    Serial.printf("[IPFINDER] endPacket fail %s:%u\n", ip.toString().c_str(), (unsigned)port);
    return false;
  }
  return true;
}

void UdpIpFinderService::sendJsonResponse(const JsonDocument &response)
{
  String payload;
  serializeJson(response, payload);

  if (!udp_ || payload.length() == 0)
  {
    return;
  }

  /* Same LAN, any IP group: unicast/gateway ARP fails across subnets.
     Reply to 255.255.255.255 + the requester UDP source port. */
  const uint16_t port = udp_->remotePort();
  const IPAddress src = udp_->remoteIP();
  const bool uni = sendUdpPayload(udp_, src, port, payload);
  const bool bcast = sendUdpPayload(udp_, IPAddress(255, 255, 255, 255), port, payload);
  Serial.printf("[IPFINDER] TX %u bytes uni=%d bcast=%d to %s:%u\n",
                (unsigned)payload.length(),
                uni ? 1 : 0,
                bcast ? 1 : 0,
                src.toString().c_str(),
                (unsigned)port);
  if (uni || bcast)
  {
    txFail_ = 0;
  }
  else if (++txFail_ >= 2)
  {
    holdSockets(4000);
  }
}
