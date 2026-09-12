#include "ip_finder.h"
#include "eth_w610.h"
#include "main.h"
#include "Version.h"
#include "UdpIpFinderService.h"

static UdpIpFinderService sIpFinder;
static constexpr uint16_t kIpFinderPort = 1234;
static const char *kIpFinderDeviceType = "ESP32";

static bool buildIpFinderSnapshot(UdpIpFinderService::DeviceSnapshot &out, void *ctx)
{
  (void)ctx;
  out.type = kIpFinderDeviceType;
  out.hostname = String(ipAddress_struct.deviceName);
  out.mac = ethW610MacString();
  out.ip = ethW610LocalIP();
  out.subnet = ethW610Subnet();
  out.gateway = ethW610Gateway();
  if ((uint32_t)out.ip == 0)
  {
    out.ip = IPAddress(ipAddress_struct.IPADDRESS);
    out.subnet = IPAddress(ipAddress_struct.SUBNETMASK);
    out.gateway = IPAddress(ipAddress_struct.GATEWAY);
  }
  out.webEnabled = false;
  out.webPort = ipAddress_struct.WEBSERVERPORT;
  if (out.webPort < 1)
  {
    out.webPort = 80;
  }
  out.trapEnabled = false;
  out.trapDestinationCount = 0;
  out.status = "active";
  out.uptimeSeconds = millis() / 1000;
  out.version = VERSION;
  return true;
}

static bool applyIpFinderNetworkConfig(const UdpIpFinderService::NetworkConfig &cfg, void *ctx)
{
  (void)ctx;
  ipAddress_struct.IPADDRESS = static_cast<uint32_t>(cfg.ip);
  ipAddress_struct.SUBNETMASK = static_cast<uint32_t>(cfg.subnet);
  ipAddress_struct.GATEWAY = static_cast<uint32_t>(cfg.gateway);
  if (cfg.webPort >= 1)
  {
    ipAddress_struct.WEBSERVERPORT = cfg.webPort;
  }
  nvsSave();
  Serial.printf("[IPFINDER] saved ip=%s sn=%s gw=%s\n",
                cfg.ip.toString().c_str(),
                cfg.subnet.toString().c_str(),
                cfg.gateway.toString().c_str());
  return true;
}

static void restartFromIpFinder(void *ctx)
{
  (void)ctx;
  ESP.restart();
}

void ipFinderPoll()
{
  sIpFinder.poll();
}

void ipFinderBegin()
{
  UdpIpFinderService::Config cfg;
  cfg.port = kIpFinderPort;
  cfg.deviceType = kIpFinderDeviceType;
  sIpFinder.setConfig(cfg);
  sIpFinder.setCallbacks(buildIpFinderSnapshot, applyIpFinderNetworkConfig, restartFromIpFinder, nullptr);
  if (!sIpFinder.begin(ethW610IpFinderUdp()))
  {
    Serial.println("[IPFINDER] UDP 1234 bind failed");
    return;
  }
}
