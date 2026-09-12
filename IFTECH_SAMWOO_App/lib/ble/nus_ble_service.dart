import 'dart:async';
import 'dart:convert';

import 'package:flutter/foundation.dart';
import 'package:flutter_blue_plus/flutter_blue_plus.dart';

import 'ble_constants.dart';

/// Parsed UPS nominal rating from `rating` CLI.
class UpsRatingConfig {
  const UpsRatingConfig({
    required this.kva,
    required this.batV,
    required this.inV,
    required this.outV,
  });

  final double kva;
  final int batV;
  final int inV;
  final int outV;
}

enum BleConnectionState {
  disconnected,
  scanning,
  connecting,
  connected,
  error,
}

/// BLE Nordic UART Service client for IFTECH UPS ESP32.
class NusBleService extends ChangeNotifier {
  BluetoothDevice? _device;
  BluetoothCharacteristic? _rx;
  BluetoothCharacteristic? _tx;
  StreamSubscription<List<int>>? _txSub;
  StreamSubscription<BluetoothConnectionState>? _connSub;
  StreamSubscription<List<ScanResult>>? _scanSub;

  BleConnectionState _state = BleConnectionState.disconnected;
  String? _error;
  final List<ScanResult> _scanResults = [];
  final StringBuffer _log = StringBuffer();
  final List<String> _logLines = [];
  String? _deviceSsid;
  String? _devicePass;
  String? _deviceName;
  UpsRatingConfig? _deviceRating;
  int _wifiConfigVersion = 0;
  int _nameConfigVersion = 0;
  int _ratingConfigVersion = 0;

  BleConnectionState get state => _state;
  String? get error => _error;
  List<ScanResult> get scanResults => List.unmodifiable(_scanResults);
  List<String> get logLines => List.unmodifiable(_logLines);
  String get logText => _log.toString();
  BluetoothDevice? get device => _device;
  bool get isConnected => _state == BleConnectionState.connected;

  /// Last SSID reported by device (`ssid` / `pass` CLI reply).
  String? get deviceSsid => _deviceSsid;

  /// Last password reported by device. Empty string means open AP.
  String? get devicePass => _devicePass;

  /// Bumps when device SSID/PASS lines are parsed (for UI autofill).
  int get wifiConfigVersion => _wifiConfigVersion;

  /// Last `NAME :` from device CLI.
  String? get deviceName => _deviceName;

  int get nameConfigVersion => _nameConfigVersion;

  /// Last UPS rating from `rating` CLI.
  UpsRatingConfig? get deviceRating => _deviceRating;

  /// Bumps when rating lines are parsed.
  int get ratingConfigVersion => _ratingConfigVersion;

  Future<bool> ensureAdapterOn() async {
    if (await FlutterBluePlus.isSupported == false) {
      _setError('이 기기는 BLE를 지원하지 않습니다.');
      return false;
    }
    final adapterState = await FlutterBluePlus.adapterState.first;
    if (adapterState != BluetoothAdapterState.on) {
      try {
        await FlutterBluePlus.turnOn();
      } catch (_) {
        _setError('블루투스를 켜 주세요.');
        return false;
      }
    }
    return true;
  }

  Future<void> startScan({Duration timeout = const Duration(seconds: 8)}) async {
    if (!await ensureAdapterOn()) return;

    await stopScan();
    _scanResults.clear();
    _state = BleConnectionState.scanning;
    _error = null;
    notifyListeners();

    try {
      await FlutterBluePlus.startScan(
        timeout: timeout,
        androidUsesFineLocation: true,
      );

      _scanSub = FlutterBluePlus.scanResults.listen((results) {
        final filtered = results.where((r) {
          final name = r.device.platformName;
          return name.startsWith(BleConstants.deviceNamePrefix);
        }).toList();

        // Also keep unnamed devices that advertise NUS service
        for (final r in results) {
          final hasNus = r.advertisementData.serviceUuids.any(
            (u) => u.str.toUpperCase() == BleConstants.serviceUuid.toUpperCase(),
          );
          if (hasNus && !filtered.any((f) => f.device.remoteId == r.device.remoteId)) {
            filtered.add(r);
          }
        }

        _scanResults
          ..clear()
          ..addAll(filtered);
        // Prefer strongest RSSI first
        _scanResults.sort((a, b) => b.rssi.compareTo(a.rssi));
        notifyListeners();
      });

      await FlutterBluePlus.isScanning.where((v) => v == false).first;
    } catch (e) {
      _setError('스캔 실패: $e');
    } finally {
      if (_state == BleConnectionState.scanning) {
        _state = BleConnectionState.disconnected;
        notifyListeners();
      }
    }
  }

  Future<void> stopScan() async {
    await _scanSub?.cancel();
    _scanSub = null;
    try {
      await FlutterBluePlus.stopScan();
    } catch (_) {}
  }

  Future<void> connect(BluetoothDevice device) async {
    await stopScan();
    await disconnect(notify: false);

    _device = device;
    _state = BleConnectionState.connecting;
    _error = null;
    _appendLog('연결 중: ${_displayName(device)}');
    notifyListeners();

    try {
      _connSub = device.connectionState.listen((s) {
        if (s == BluetoothConnectionState.disconnected &&
            _state == BleConnectionState.connected) {
          _appendLog('연결이 끊겼습니다.');
          _clearChars();
          _state = BleConnectionState.disconnected;
          notifyListeners();
        }
      });

      await device.connect(timeout: const Duration(seconds: 15));
      await device.requestMtu(185);

      final services = await device.discoverServices();
      BluetoothService? nus;
      for (final s in services) {
        if (s.uuid.str.toUpperCase() == BleConstants.serviceUuid.toUpperCase()) {
          nus = s;
          break;
        }
      }
      if (nus == null) {
        throw Exception('Nordic UART 서비스를 찾을 수 없습니다.');
      }

      for (final c in nus.characteristics) {
        final id = c.uuid.str.toUpperCase();
        if (id == BleConstants.rxUuid.toUpperCase()) {
          _rx = c;
        } else if (id == BleConstants.txUuid.toUpperCase()) {
          _tx = c;
        }
      }

      if (_rx == null || _tx == null) {
        throw Exception('RX/TX 캐릭터리스틱을 찾을 수 없습니다.');
      }

      await _tx!.setNotifyValue(true);
      _txSub = _tx!.onValueReceived.listen((bytes) {
        if (bytes.isEmpty) return;
        final text = utf8.decode(bytes, allowMalformed: true);
        _appendLog(text, fromDevice: true);
      });

      _state = BleConnectionState.connected;
      _appendLog('연결됨. 명령을 전송할 수 있습니다.');
      notifyListeners();
      Future.delayed(const Duration(milliseconds: 250), () async {
        await fetchStoredWifi();
        await fetchStoredName();
      });
    } catch (e) {
      _appendLog('연결 실패: $e');
      await disconnect(notify: false);
      _setError('연결 실패: $e');
    }
  }

  Future<void> disconnect({bool notify = true}) async {
    await _txSub?.cancel();
    _txSub = null;
    await _connSub?.cancel();
    _connSub = null;
    _clearChars();
    try {
      await _device?.disconnect();
    } catch (_) {}
    _device = null;
    _deviceSsid = null;
    _devicePass = null;
    _deviceName = null;
    _deviceRating = null;
    _state = BleConnectionState.disconnected;
    if (notify) {
      _appendLog('연결 해제');
      notifyListeners();
    }
  }

  /// Send a CLI command. Appends CR+LF if missing (ESP32 expects \\r or \\n).
  /// By default clears the log console so only this command's traffic is shown.
  Future<void> sendCommand(String command, {bool clearFirst = true}) async {
    if (!isConnected || _rx == null) {
      _setError('장치가 연결되지 않았습니다.');
      return;
    }
    var cmd = command.trim();
    if (cmd.isEmpty) return;
    if (!cmd.endsWith('\r') && !cmd.endsWith('\n')) {
      cmd = '$cmd\r\n';
    }

    if (clearFirst) {
      _log.clear();
      _logLines.clear();
    }

    try {
      final bytes = utf8.encode(cmd);
      // Chunk for BLE ATT payload (leave room under MTU)
      const chunk = 160;
      for (var i = 0; i < bytes.length; i += chunk) {
        final end = (i + chunk < bytes.length) ? i + chunk : bytes.length;
        await _rx!.write(bytes.sublist(i, end), withoutResponse: false);
      }
      _appendLog('> ${command.trim()}');
    } catch (e) {
      _appendLog('전송 실패: $e');
      _setError('전송 실패: $e');
    }
  }

  /// Query device UPS nominal rating (`rating` with no argument).
  Future<void> fetchStoredRating() async {
    if (!isConnected) return;
    await sendCommand('rating');
  }

  Future<void> setRating({
    required double kva,
    required int batV,
    required int inV,
    required int outV,
  }) async {
    final kvaText = kva == kva.roundToDouble()
        ? '${kva.toInt()}'
        : kva.toStringAsFixed(1);
    await sendCommand('rating $kvaText $batV $inV $outV', clearFirst: true);
    await Future.delayed(const Duration(milliseconds: 400));
    await sendCommand('rating', clearFirst: false);
  }

  Future<void> setWifi({required String ssid, required String password}) async {
    // 한 번의 사용자 동작이므로 로그만 한 번 비우고 ssid/pass를 이어서 표시
    await sendCommand('ssid $ssid', clearFirst: true);
    await Future.delayed(const Duration(milliseconds: 300));
    if (password.trim().isEmpty) {
      await sendCommand('pass none', clearFirst: false);
    } else {
      await sendCommand('pass $password', clearFirst: false);
    }
  }

  /// Query device for currently stored SSID/PASS (`ssid` with no argument).
  Future<void> fetchStoredWifi() async {
    if (!isConnected) return;
    await sendCommand('ssid');
  }

  Future<void> fetchStoredName() async {
    if (!isConnected) return;
    await sendCommand('name', clearFirst: false);
  }

  Future<void> setDeviceName(String name) async {
    await sendCommand('name $name');
  }

  void clearLog() {
    _log.clear();
    _logLines.clear();
    notifyListeners();
  }

  void _clearChars() {
    _rx = null;
    _tx = null;
  }

  void _setError(String message) {
    _error = message;
    _state = BleConnectionState.error;
    notifyListeners();
  }

  void _appendLog(String text, {bool fromDevice = false}) {
    final cleaned = text.replaceAll('\r\n', '\n').replaceAll('\r', '\n');
    var wifiUpdated = false;
    var ratingUpdated = false;
    var nameUpdated = false;
    for (final line in cleaned.split('\n')) {
      if (line.isEmpty && fromDevice) continue;
      _logLines.add(line);
      _log.writeln(line);
      if (fromDevice && _parseWifiConfigLine(line)) {
        wifiUpdated = true;
      }
      if (fromDevice && _parseNameLine(line)) {
        nameUpdated = true;
      }
      if (fromDevice && _parseRatingConfigLine(line)) {
        ratingUpdated = true;
      }
    }
    // Cap log size
    while (_logLines.length > 500) {
      _logLines.removeAt(0);
    }
    if (wifiUpdated) {
      _wifiConfigVersion++;
    }
    if (nameUpdated) {
      _nameConfigVersion++;
    }
    if (ratingUpdated) {
      _ratingConfigVersion++;
    }
    notifyListeners();
  }

  bool _parseNameLine(String line) {
    final m = RegExp(r'^NAME\s*:\s*(.*)$', caseSensitive: false)
        .firstMatch(line.trim());
    if (m == null) return false;
    _deviceName = m.group(1)?.trim() ?? '';
    return true;
  }

  /// Parse `SSID : xxx` / `PASS : yyy` / `PASS : (open)` from ESP32 CLI.
  bool _parseWifiConfigLine(String line) {
    final trimmed = line.trim();
    final ssidMatch = RegExp(r'^SSID\s*:\s*(.*)$', caseSensitive: false)
        .firstMatch(trimmed);
    if (ssidMatch != null) {
      _deviceSsid = ssidMatch.group(1)?.trim() ?? '';
      return true;
    }
    final passMatch = RegExp(r'^PASS\s*:\s*(.*)$', caseSensitive: false)
        .firstMatch(trimmed);
    if (passMatch != null) {
      final raw = passMatch.group(1)?.trim() ?? '';
      if (raw == '(open)' || raw.toLowerCase() == 'open') {
        _devicePass = '';
      } else {
        _devicePass = raw;
      }
      return true;
    }
    return false;
  }

  /// Parse `KVA : 10.0 kVA`, `BAT : 192 V`, `IN  : 220 V`, `OUT : 220 V`.
  bool _parseRatingConfigLine(String line) {
    final trimmed = line.trim();
    final kvaMatch =
        RegExp(r'^KVA\s*:\s*([0-9]+(?:\.[0-9]+)?)', caseSensitive: false)
            .firstMatch(trimmed);
    if (kvaMatch != null) {
      final kva = double.tryParse(kvaMatch.group(1) ?? '');
      if (kva != null) {
        _deviceRating = UpsRatingConfig(
          kva: kva,
          batV: _deviceRating?.batV ?? 192,
          inV: _deviceRating?.inV ?? 220,
          outV: _deviceRating?.outV ?? 220,
        );
        return true;
      }
    }
    final batMatch =
        RegExp(r'^BAT\s*:\s*(\d+)', caseSensitive: false).firstMatch(trimmed);
    if (batMatch != null) {
      final v = int.tryParse(batMatch.group(1) ?? '');
      if (v != null) {
        final prev = _deviceRating;
        _deviceRating = UpsRatingConfig(
          kva: prev?.kva ?? 10,
          batV: v,
          inV: prev?.inV ?? 220,
          outV: prev?.outV ?? 220,
        );
        return true;
      }
    }
    final inMatch =
        RegExp(r'^IN\s*:\s*(\d+)', caseSensitive: false).firstMatch(trimmed);
    if (inMatch != null) {
      final v = int.tryParse(inMatch.group(1) ?? '');
      if (v != null) {
        final prev = _deviceRating;
        _deviceRating = UpsRatingConfig(
          kva: prev?.kva ?? 10,
          batV: prev?.batV ?? 192,
          inV: v,
          outV: prev?.outV ?? 220,
        );
        return true;
      }
    }
    final outMatch =
        RegExp(r'^OUT\s*:\s*(\d+)', caseSensitive: false).firstMatch(trimmed);
    if (outMatch != null) {
      final v = int.tryParse(outMatch.group(1) ?? '');
      if (v != null) {
        final prev = _deviceRating;
        _deviceRating = UpsRatingConfig(
          kva: prev?.kva ?? 10,
          batV: prev?.batV ?? 192,
          inV: prev?.inV ?? 220,
          outV: v,
        );
        return true;
      }
    }
    return false;
  }

  String _displayName(BluetoothDevice d) {
    final n = d.platformName;
    return n.isNotEmpty ? n : d.remoteId.str;
  }

  @override
  void dispose() {
    stopScan();
    disconnect(notify: false);
    super.dispose();
  }
}
