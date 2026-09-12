#ifndef UDP_IP_FINDER_SERVICE_H
#define UDP_IP_FINDER_SERVICE_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <Udp.h>

class UdpIpFinderService
{
public:
  struct Config
  {
    uint16_t port = 1234;
    String deviceType = "ESP32";
  };

  struct NetworkConfig
  {
    IPAddress ip;
    IPAddress subnet;
    IPAddress gateway;
    bool webEnabled = true;
    uint16_t webPort = 80;
    bool trapEnabled = false;
    IPAddress trapDestinations[5];
    uint8_t trapDestinationCount = 0;
  };

  struct DeviceSnapshot
  {
    String type;
    String hostname;
    String mac;
    IPAddress ip;
    IPAddress subnet;
    IPAddress gateway;
    bool webEnabled = true;
    uint16_t webPort = 80;
    bool trapEnabled = false;
    IPAddress trapDestinations[5];
    uint8_t trapDestinationCount = 0;
    String status;
    uint32_t uptimeSeconds;
    String version;
  };

  using BuildSnapshotCallback = bool (*)(DeviceSnapshot &out, void *ctx);
  using ApplyNetworkCallback = bool (*)(const NetworkConfig &cfg, void *ctx);
  using RestartCallback = void (*)(void *ctx);
  using TrapTestCallback = bool (*)(void *ctx);

  bool begin(UDP *udp);
  void poll();
  void setConfig(const Config &config);

  void setCallbacks(BuildSnapshotCallback buildSnapshot,
                    ApplyNetworkCallback applyNetwork,
                    RestartCallback restartDevice,
                    void *ctx);
  void setTrapTestCallback(TrapTestCallback trapTest, void *ctx);

private:
  void handleSetNetworkConfig(const JsonDocument &request);
  void handleDeviceDiscovery(const JsonDocument &request);
  void handleTrapTest(const JsonDocument &request);
  void sendJsonResponse(const JsonDocument &response);

  UDP *udp_ = nullptr;
  BuildSnapshotCallback buildSnapshotCb_ = nullptr;
  ApplyNetworkCallback applyNetworkCb_ = nullptr;
  RestartCallback restartCb_ = nullptr;
  TrapTestCallback trapTestCb_ = nullptr;
  void *ctx_ = nullptr;
  void *trapTestCtx_ = nullptr;
  Config config_ = {};
};

#endif
