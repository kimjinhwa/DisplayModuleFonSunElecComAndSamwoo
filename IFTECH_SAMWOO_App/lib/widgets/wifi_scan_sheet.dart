import 'dart:io';

import 'package:flutter/foundation.dart';
import 'package:flutter/material.dart';
import 'package:permission_handler/permission_handler.dart';
import 'package:wifi_scan/wifi_scan.dart';

/// Bottom sheet: scan nearby Wi-Fi APs and pick an SSID.
/// Works on Android. On iOS, nearby AP scan is restricted by the OS.
class WifiScanSheet extends StatefulWidget {
  const WifiScanSheet({super.key});

  static Future<String?> show(BuildContext context) {
    return showModalBottomSheet<String>(
      context: context,
      isScrollControlled: true,
      showDragHandle: true,
      builder: (_) => const WifiScanSheet(),
    );
  }

  @override
  State<WifiScanSheet> createState() => _WifiScanSheetState();
}

class _WifiScanSheetState extends State<WifiScanSheet> {
  bool _scanning = false;
  String? _error;
  List<WiFiAccessPoint> _aps = [];

  @override
  void initState() {
    super.initState();
    WidgetsBinding.instance.addPostFrameCallback((_) => _scan());
  }

  Future<bool> _ensurePermissions() async {
    if (kIsWeb) return false;

    if (Platform.isIOS) {
      setState(() {
        _error =
            'iPhone은 주변 Wi-Fi 목록 스캔이 OS에서 제한됩니다.\nSSID를 직접 입력해 주세요.';
      });
      return false;
    }

    final loc = await Permission.locationWhenInUse.request();
    if (!loc.isGranted) {
      setState(() => _error = 'Wi-Fi 스캔에는 위치 권한이 필요합니다.');
      return false;
    }

    // Android 13+
    final nearby = await Permission.nearbyWifiDevices.request();
    if (nearby.isDenied || nearby.isPermanentlyDenied) {
      // Older Android may not have this permission — continue if location ok
    }

    return true;
  }

  Future<void> _scan() async {
    setState(() {
      _scanning = true;
      _error = null;
    });

    try {
      if (!await _ensurePermissions()) {
        return;
      }

      final can = await WiFiScan.instance.canStartScan();
      if (can != CanStartScan.yes) {
        setState(() {
          _error = 'Wi-Fi 스캔을 시작할 수 없습니다 ($can).\n'
              '위치/Wi-Fi가 켜져 있는지 확인해 주세요.';
        });
        return;
      }

      final started = await WiFiScan.instance.startScan();
      if (!started) {
        setState(() => _error = '스캔 시작에 실패했습니다.');
        return;
      }

      await Future.delayed(const Duration(seconds: 2));

      final canGet = await WiFiScan.instance.canGetScannedResults();
      if (canGet != CanGetScannedResults.yes) {
        setState(() => _error = '스캔 결과를 읽을 수 없습니다 ($canGet).');
        return;
      }

      final results = await WiFiScan.instance.getScannedResults();
      // Unique by SSID, keep strongest
      final best = <String, WiFiAccessPoint>{};
      for (final ap in results) {
        if (ap.ssid.trim().isEmpty) continue;
        final prev = best[ap.ssid];
        if (prev == null || ap.level > prev.level) {
          best[ap.ssid] = ap;
        }
      }
      final list = best.values.toList()
        ..sort((a, b) => b.level.compareTo(a.level));

      setState(() => _aps = list);
    } catch (e) {
      setState(() => _error = '스캔 오류: $e');
    } finally {
      if (mounted) setState(() => _scanning = false);
    }
  }

  IconData _signalIcon(int level) {
    if (level >= -55) return Icons.signal_wifi_4_bar;
    if (level >= -67) return Icons.network_wifi_3_bar;
    if (level >= -80) return Icons.network_wifi_2_bar;
    return Icons.network_wifi_1_bar;
  }

  @override
  Widget build(BuildContext context) {
    final height = MediaQuery.sizeOf(context).height * 0.65;
    return SafeArea(
      child: SizedBox(
        height: height,
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.stretch,
          children: [
            Padding(
              padding: const EdgeInsets.fromLTRB(16, 0, 8, 8),
              child: Row(
                children: [
                  Text(
                    '주변 Wi-Fi',
                    style: Theme.of(context).textTheme.titleLarge,
                  ),
                  const Spacer(),
                  IconButton(
                    tooltip: '다시 검색',
                    onPressed: _scanning ? null : _scan,
                    icon: _scanning
                        ? const SizedBox(
                            width: 22,
                            height: 22,
                            child: CircularProgressIndicator(strokeWidth: 2),
                          )
                        : const Icon(Icons.refresh),
                  ),
                ],
              ),
            ),
            if (_error != null)
              Padding(
                padding: const EdgeInsets.symmetric(horizontal: 16),
                child: Text(
                  _error!,
                  style: TextStyle(color: Theme.of(context).colorScheme.error),
                ),
              ),
            Expanded(
              child: _aps.isEmpty
                  ? Center(
                      child: Text(
                        _scanning ? '검색 중…' : '검색된 AP가 없습니다',
                        style: TextStyle(
                          color: Theme.of(context).colorScheme.onSurfaceVariant,
                        ),
                      ),
                    )
                  : ListView.separated(
                      itemCount: _aps.length,
                      separatorBuilder: (_, _) => const Divider(height: 1),
                      itemBuilder: (context, i) {
                        final ap = _aps[i];
                        final secure = ap.capabilities.contains('WPA') ||
                            ap.capabilities.contains('WEP') ||
                            ap.capabilities.contains('PSK');
                        return ListTile(
                          leading: Icon(_signalIcon(ap.level)),
                          title: Text(ap.ssid),
                          subtitle: Text('${ap.level} dBm'),
                          trailing: Icon(
                            secure ? Icons.lock_outline : Icons.lock_open,
                            size: 18,
                          ),
                          onTap: () => Navigator.pop(context, ap.ssid),
                        );
                      },
                    ),
            ),
          ],
        ),
      ),
    );
  }
}
