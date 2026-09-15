import 'package:flutter_blue_plus/flutter_blue_plus.dart';

/// ESP32 Nordic UART Service (NUS) UUIDs — matches myBlueTooth.h
class BleConstants {
  static const String deviceNamePrefix = 'IFTECH_';

  /// Default BLE name fragments (case-insensitive contains).
  static const List<String> defaultNameTokens = [
    'UPS',
    'BMS',
    'IFTECH',
    'IFT',
  ];

  static String advertisedName(ScanResult r) {
    final adv = r.advertisementData.advName.trim();
    final platform = r.device.platformName.trim();
    if (adv.isNotEmpty) return adv;
    if (platform.isNotEmpty) return platform;
    return '';
  }

  static bool matchesScanFilter(
    ScanResult r, {
    String extraQuery = '',
    bool showAll = false,
  }) {
    if (showAll) return true;
    final name = advertisedName(r).toUpperCase();
    final tokens = <String>[
      ...defaultNameTokens,
      if (extraQuery.trim().isNotEmpty) extraQuery.trim(),
    ];
    if (tokens.any((t) => name.contains(t.toUpperCase()))) return true;
    return r.advertisementData.serviceUuids.any(
      (u) => u.str.toUpperCase() == serviceUuid.toUpperCase(),
    );
  }

  static const String serviceUuid = '6E400001-B5A3-F393-E0A9-E50E24DCCA9B';
  static const String rxUuid = '6E400002-B5A3-F393-E0A9-E50E24DCCA9B';
  static const String txUuid = '6E400003-B5A3-F393-E0A9-E50E24DCCA9B';
}
