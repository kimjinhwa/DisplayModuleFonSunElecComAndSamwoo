import 'package:flutter/material.dart';
import 'package:flutter_blue_plus/flutter_blue_plus.dart';
import 'package:permission_handler/permission_handler.dart';
import 'package:provider/provider.dart';

import '../ble/ble_constants.dart';
import '../ble/nus_ble_service.dart';

class ScanScreen extends StatefulWidget {
  const ScanScreen({super.key});

  @override
  State<ScanScreen> createState() => _ScanScreenState();
}

class _ScanScreenState extends State<ScanScreen> {
  bool _requesting = false;
  bool _showAll = false;
  final _queryCtrl = TextEditingController();

  @override
  void dispose() {
    _queryCtrl.dispose();
    super.dispose();
  }

  @override
  void initState() {
    super.initState();
    WidgetsBinding.instance.addPostFrameCallback((_) => _onScan());
  }

  Future<void> _cancel() async {
    await context.read<NusBleService>().stopScan();
    if (mounted) Navigator.of(context).pop();
  }

  Future<bool> _requestPermissions() async {
    setState(() => _requesting = true);
    try {
      final statuses = await [
        Permission.bluetoothScan,
        Permission.bluetoothConnect,
        Permission.locationWhenInUse,
      ].request();

      final scanOk = statuses[Permission.bluetoothScan]?.isGranted ?? true;
      final connectOk = statuses[Permission.bluetoothConnect]?.isGranted ?? true;
      final loc = statuses[Permission.locationWhenInUse];
      final locOk = loc == null ||
          loc.isGranted ||
          loc.isLimited ||
          loc.isPermanentlyDenied == false;

      if (!scanOk || !connectOk) {
        if (mounted) {
          ScaffoldMessenger.of(context).showSnackBar(
            const SnackBar(content: Text('블루투스 권한이 필요합니다.')),
          );
        }
        return false;
      }
      if (loc != null && loc.isPermanentlyDenied) {
        if (mounted) {
          ScaffoldMessenger.of(context).showSnackBar(
            const SnackBar(
              content: Text('위치 권한이 거부되었습니다. 설정에서 허용해 주세요.'),
            ),
          );
        }
        await openAppSettings();
        return false;
      }
      return locOk || scanOk;
    } finally {
      if (mounted) setState(() => _requesting = false);
    }
  }

  Future<void> _onScan({bool showAll = false}) async {
    final ok = await _requestPermissions();
    if (!ok) return;
    if (!mounted) return;
    setState(() => _showAll = showAll);
    await context.read<NusBleService>().startScan(
          extraQuery: showAll ? '' : _queryCtrl.text,
          showAll: showAll,
        );
  }

  Future<void> _onConnect(ScanResult result) async {
    final ble = context.read<NusBleService>();
    await ble.connect(result.device);
    if (!mounted) return;
    if (ble.isConnected) {
      Navigator.of(context).pop();
    } else if (ble.error != null) {
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(content: Text(ble.error!)),
      );
    }
  }

  @override
  Widget build(BuildContext context) {
    final ble = context.watch<NusBleService>();
    final scanning = ble.state == BleConnectionState.scanning;
    final connecting = ble.state == BleConnectionState.connecting;

    return PopScope(
      canPop: !connecting,
      onPopInvokedWithResult: (didPop, _) async {
        if (didPop) {
          await context.read<NusBleService>().stopScan();
        }
      },
      child: Scaffold(
        appBar: AppBar(
          title: const Text('장비 연결'),
          leading: IconButton(
            tooltip: '뒤로',
            onPressed: connecting ? null : _cancel,
            icon: const Icon(Icons.arrow_back),
          ),
          actions: [
            TextButton(
              onPressed: connecting ? null : _cancel,
              child: const Text('취소'),
            ),
            if (scanning || connecting || _requesting)
              const Padding(
                padding: EdgeInsets.only(right: 16),
                child: Center(
                  child: SizedBox(
                    width: 22,
                    height: 22,
                    child: CircularProgressIndicator(strokeWidth: 2),
                  ),
                ),
              ),
          ],
        ),
        body: Column(
          crossAxisAlignment: CrossAxisAlignment.stretch,
          children: [
            Padding(
              padding: const EdgeInsets.fromLTRB(16, 16, 16, 8),
              child: Text(
                _showAll
                    ? '주변 BLE 장비를 모두 표시합니다. 광고에 이름이 없는 장치는 주소만 보입니다.\n뒤로 또는 취소하면 연결하지 않고 나갑니다.'
                    : '이름에 UPS, BMS, IFTECH, IFT가 포함된 장비를 찾습니다.\n검색어를 넣으면 그 글자가 들어간 이름도 함께 찾습니다.',
                style: Theme.of(context).textTheme.bodyMedium?.copyWith(
                      color: Theme.of(context).colorScheme.onSurfaceVariant,
                    ),
              ),
            ),
            Padding(
              padding: const EdgeInsets.symmetric(horizontal: 16),
              child: TextField(
                controller: _queryCtrl,
                enabled: !scanning && !connecting,
                textInputAction: TextInputAction.search,
                decoration: const InputDecoration(
                  border: OutlineInputBorder(),
                  labelText: '검색어',
                  hintText: '이름에 포함된 글자 (선택)',
                  prefixIcon: Icon(Icons.filter_alt_outlined),
                ),
                onSubmitted: (_) {
                  if (!scanning && !connecting) _onScan();
                },
              ),
            ),
            const SizedBox(height: 8),
            Padding(
              padding: const EdgeInsets.symmetric(horizontal: 16),
              child: Row(
                children: [
                  Expanded(
                    child: FilledButton.icon(
                      onPressed:
                          scanning || connecting ? null : () => _onScan(),
                      icon: Icon(
                        scanning ? Icons.bluetooth_searching : Icons.search,
                      ),
                      label: Text(scanning && !_showAll ? '검색 중…' : '검색'),
                    ),
                  ),
                  const SizedBox(width: 8),
                  Expanded(
                    child: OutlinedButton.icon(
                      onPressed: scanning || connecting
                          ? null
                          : () => _onScan(showAll: true),
                      icon: const Icon(Icons.radar),
                      label: Text(scanning && _showAll ? '전체 중…' : '전체검색'),
                    ),
                  ),
                ],
              ),
            ),
            if (ble.error != null)
              Padding(
                padding: const EdgeInsets.fromLTRB(16, 12, 16, 0),
                child: Text(
                  ble.error!,
                  style: TextStyle(color: Theme.of(context).colorScheme.error),
                ),
              ),
            const SizedBox(height: 8),
            Expanded(
              child: ble.scanResults.isEmpty
                  ? Center(
                      child: Text(
                        scanning ? '주변 장비를 찾는 중…' : '검색된 장비가 없습니다',
                        style: TextStyle(
                          color: Theme.of(context).colorScheme.onSurfaceVariant,
                        ),
                      ),
                    )
                  : ListView.separated(
                      padding: const EdgeInsets.all(12),
                      itemCount: ble.scanResults.length,
                      separatorBuilder: (_, _) => const SizedBox(height: 8),
                      itemBuilder: (context, index) {
                        final r = ble.scanResults[index];
                        final advertised = BleConstants.advertisedName(r);
                        final name = advertised.isNotEmpty
                            ? advertised
                            : r.device.remoteId.str;
                        return Card(
                          child: ListTile(
                            leading: CircleAvatar(
                              backgroundColor: Theme.of(context)
                                  .colorScheme
                                  .primaryContainer,
                              child: const Icon(Icons.bluetooth),
                            ),
                            title: Text(name),
                            subtitle: Text(
                              '${r.device.remoteId.str}\nRSSI ${r.rssi} dBm',
                            ),
                            isThreeLine: true,
                            trailing: connecting
                                ? const SizedBox(
                                    width: 24,
                                    height: 24,
                                    child: CircularProgressIndicator(
                                      strokeWidth: 2,
                                    ),
                                  )
                                : const Icon(Icons.chevron_right),
                            onTap: connecting ? null : () => _onConnect(r),
                          ),
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
