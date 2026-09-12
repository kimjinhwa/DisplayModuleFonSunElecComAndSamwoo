import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import 'package:share_plus/share_plus.dart';

import '../ble/nus_ble_service.dart';
import '../widgets/log_console.dart';
import '../widgets/wifi_scan_sheet.dart';
import 'scan_screen.dart';

class ControlScreen extends StatefulWidget {
  const ControlScreen({super.key});

  @override
  State<ControlScreen> createState() => _ControlScreenState();
}

class _ControlScreenState extends State<ControlScreen> {
  final _ssidCtrl = TextEditingController();
  final _passCtrl = TextEditingController();
  final _cmdCtrl = TextEditingController();
  bool _obscurePass = true;
  int _lastWifiVersion = -1;
  bool _wifiFieldsTouched = false;
  bool _wifiExpanded = true;
  bool _quickCmdExpanded = true;

  @override
  void initState() {
    super.initState();
    _ssidCtrl.addListener(() => _wifiFieldsTouched = true);
    _passCtrl.addListener(() => _wifiFieldsTouched = true);
    WidgetsBinding.instance.addPostFrameCallback((_) {
      final ble = context.read<NusBleService>();
      if (ble.isConnected) {
        _applyDeviceWifi(ble, force: true);
        if (ble.deviceSsid == null) {
          ble.fetchStoredWifi();
        }
      }
    });
  }

  @override
  void dispose() {
    _ssidCtrl.dispose();
    _passCtrl.dispose();
    _cmdCtrl.dispose();
    super.dispose();
  }

  void _applyDeviceWifi(NusBleService ble, {bool force = false}) {
    if (!force && _wifiFieldsTouched) return;
    if (ble.wifiConfigVersion == _lastWifiVersion && !force) return;

    final ssid = ble.deviceSsid;
    final pass = ble.devicePass;
    if (ssid == null && pass == null) {
      _lastWifiVersion = ble.wifiConfigVersion;
      return;
    }

    setState(() {
      _lastWifiVersion = ble.wifiConfigVersion;
      if (ssid != null) _ssidCtrl.text = ssid;
      if (pass != null) _passCtrl.text = pass;
      _wifiFieldsTouched = false;
    });
  }

  Future<void> _pickWifiSsid() async {
    final ssid = await WifiScanSheet.show(context);
    if (ssid == null || !mounted) return;
    setState(() {
      _ssidCtrl.text = ssid;
      _wifiFieldsTouched = true;
    });
  }

  Future<void> _reloadFromDevice() async {
    _wifiFieldsTouched = false;
    await context.read<NusBleService>().fetchStoredWifi();
    if (mounted) {
      ScaffoldMessenger.of(context).showSnackBar(
        const SnackBar(
          content: Text('장비에서 Wi-Fi 설정을 읽는 중…'),
          duration: Duration(seconds: 1),
        ),
      );
    }
  }

  Future<void> _confirmAndSend(String label, String command) async {
    final ok = await showDialog<bool>(
      context: context,
      builder: (ctx) => AlertDialog(
        title: Text(label),
        content: Text('"$command" 명령을 보낼까요?'),
        actions: [
          TextButton(
            onPressed: () => Navigator.pop(ctx, false),
            child: const Text('취소'),
          ),
          FilledButton(
            onPressed: () => Navigator.pop(ctx, true),
            child: const Text('실행'),
          ),
        ],
      ),
    );
    if (ok == true && mounted) {
      await context.read<NusBleService>().sendCommand(command);
    }
  }

  Future<void> _syncPhoneTime() async {
    final now = DateTime.now();
    final stamp =
        '${now.year.toString().padLeft(4, '0')}-'
        '${now.month.toString().padLeft(2, '0')}-'
        '${now.day.toString().padLeft(2, '0')} '
        '${now.hour.toString().padLeft(2, '0')}:'
        '${now.minute.toString().padLeft(2, '0')}:'
        '${now.second.toString().padLeft(2, '0')}';
    final cmd =
        'time ${now.year} ${now.month} ${now.day} '
        '${now.hour} ${now.minute} ${now.second}';

    final ok = await showDialog<bool>(
      context: context,
      builder: (ctx) => AlertDialog(
        title: const Text('시간설정'),
        content: Text('휴대폰 현재 시간으로 장비 RTC를 맞출까요?\n\n$stamp'),
        actions: [
          TextButton(
            onPressed: () => Navigator.pop(ctx, false),
            child: const Text('취소'),
          ),
          FilledButton(
            onPressed: () => Navigator.pop(ctx, true),
            child: const Text('설정'),
          ),
        ],
      ),
    );
    if (ok == true && mounted) {
      await context.read<NusBleService>().sendCommand(cmd);
    }
  }

  Future<void> _editName() async {
    final ble = context.read<NusBleService>();
    await ble.fetchStoredName();
    await Future.delayed(const Duration(milliseconds: 250));
    if (!mounted) return;
    final ctrl = TextEditingController(text: ble.deviceName ?? '');
    final result = await showDialog<String>(
      context: context,
      builder: (ctx) => AlertDialog(
        title: const Text('장치 이름'),
        content: TextField(
          controller: ctrl,
          autofocus: true,
          maxLength: 19,
          decoration: const InputDecoration(
            labelText: 'NAME',
            hintText: '예: BAT RACK1',
            border: OutlineInputBorder(),
          ),
        ),
        actions: [
          TextButton(
            onPressed: () => Navigator.pop(ctx),
            child: const Text('취소'),
          ),
          FilledButton(
            onPressed: () => Navigator.pop(ctx, ctrl.text.trim()),
            child: const Text('저장'),
          ),
        ],
      ),
    );
    ctrl.dispose();
    if (result != null && result.isNotEmpty && mounted) {
      await ble.setDeviceName(result);
    }
  }

  Future<void> _editIp() async {
    final ble = context.read<NusBleService>();
    await ble.sendCommand('ip');
    await Future.delayed(const Duration(milliseconds: 250));
    if (!mounted) return;

    String lastVal(String prefix, String fallback) {
      final rx = RegExp('^$prefix\\s*:\\s*(.+)\$', caseSensitive: false);
      for (var i = ble.logLines.length - 1; i >= 0; i--) {
        final m = rx.firstMatch(ble.logLines[i].trim());
        if (m != null) return (m.group(1) ?? '').trim();
      }
      return fallback;
    }

    final ipCtrl = TextEditingController(text: lastVal('SET IP', '192.168.0.57'));
    final snCtrl = TextEditingController(text: lastVal('SET SN', '255.255.255.0'));
    final gwCtrl = TextEditingController(text: lastVal('SET GW', '192.168.0.1'));
    final ok = await showDialog<bool>(
      context: context,
      builder: (ctx) => AlertDialog(
        title: const Text('이더넷 IP'),
        content: Column(
          mainAxisSize: MainAxisSize.min,
          children: [
            TextField(
              controller: ipCtrl,
              decoration: const InputDecoration(
                labelText: 'IP',
                border: OutlineInputBorder(),
              ),
            ),
            const SizedBox(height: 8),
            TextField(
              controller: snCtrl,
              decoration: const InputDecoration(
                labelText: 'Subnet',
                border: OutlineInputBorder(),
              ),
            ),
            const SizedBox(height: 8),
            TextField(
              controller: gwCtrl,
              decoration: const InputDecoration(
                labelText: 'Gateway',
                border: OutlineInputBorder(),
              ),
            ),
            const SizedBox(height: 8),
            const Text('저장 후 reboot 해야 적용됩니다.'),
          ],
        ),
        actions: [
          TextButton(
            onPressed: () => Navigator.pop(ctx, false),
            child: const Text('취소'),
          ),
          FilledButton(
            onPressed: () => Navigator.pop(ctx, true),
            child: const Text('저장'),
          ),
        ],
      ),
    );
    final ip = ipCtrl.text.trim();
    final sn = snCtrl.text.trim();
    final gw = gwCtrl.text.trim();
    ipCtrl.dispose();
    snCtrl.dispose();
    gwCtrl.dispose();
    if (ok == true && ip.isNotEmpty && mounted) {
      await ble.sendCommand('ip $ip $sn $gw');
    }
  }

  Future<void> _openConnect() async {
    await Navigator.of(context).push(
      MaterialPageRoute(builder: (_) => const ScanScreen()),
    );
    if (!mounted) return;
    final ble = context.read<NusBleService>();
    if (ble.isConnected) {
      _applyDeviceWifi(ble, force: true);
      if (ble.deviceSsid == null) {
        await ble.fetchStoredWifi();
      }
    }
  }

  void _pushFullScreenLog({String title = 'LOG'}) {
    final ble = context.read<NusBleService>();
    final shareTitle =
        'IFTECH SAMWOO Log - ${ble.device?.platformName ?? "SAMWOO"}';
    Navigator.of(context).push(
      MaterialPageRoute(
        builder: (_) => ChangeNotifierProvider.value(
          value: ble,
          child: Consumer<NusBleService>(
            builder: (_, service, _) => FullScreenLogPage(
              title: title,
              lines: service.logLines,
              shareTitle: shareTitle,
              onClear: () => service.clearLog(),
            ),
          ),
        ),
      ),
    );
  }

  @override
  Widget build(BuildContext context) {
    final ble = context.watch<NusBleService>();
    if (ble.wifiConfigVersion != _lastWifiVersion) {
      WidgetsBinding.instance.addPostFrameCallback((_) {
        if (!mounted) return;
        _applyDeviceWifi(context.read<NusBleService>());
      });
    }
    final name = ble.device?.platformName ?? 'SAMWOO';
    final storedHint = ble.deviceSsid == null
        ? null
        : '장비 저장값: ${ble.deviceSsid}'
            '${(ble.devicePass == null || ble.devicePass!.isEmpty) ? ' / (open)' : ' / ••••••'}';

    return Scaffold(
        appBar: AppBar(
          title: const Text('IFTECH SAMWOO'),
          actions: [
            IconButton(
              tooltip: '로그 공유',
              onPressed: ble.logLines.isEmpty
                  ? null
                  : () async {
                      final text = ble.logLines.join('\n');
                      final box = context.findRenderObject() as RenderBox?;
                      await Share.share(
                        text,
                        subject: 'IFTECH SAMWOO Log - $name',
                        sharePositionOrigin: box != null
                            ? box.localToGlobal(Offset.zero) & box.size
                            : null,
                      );
                    },
              icon: const Icon(Icons.share),
            ),
            IconButton(
              tooltip: '로그 지우기',
              onPressed: () => ble.clearLog(),
              icon: const Icon(Icons.delete_outline),
            ),
          ],
        ),
        body: Column(
          children: [
            _buildConnectionBar(ble, name),
            Expanded(
              flex: 5,
              child: SingleChildScrollView(
                padding: const EdgeInsets.fromLTRB(16, 12, 16, 8),
                child: Column(
                  crossAxisAlignment: CrossAxisAlignment.stretch,
                  children: [
                    InkWell(
                      borderRadius: BorderRadius.circular(8),
                      onTap: () =>
                          setState(() => _wifiExpanded = !_wifiExpanded),
                      child: Padding(
                        padding: const EdgeInsets.symmetric(vertical: 4),
                        child: Row(
                          children: [
                            Icon(
                              _wifiExpanded
                                  ? Icons.expand_less
                                  : Icons.expand_more,
                              size: 22,
                            ),
                            const SizedBox(width: 4),
                            Text(
                              'Wi-Fi 설정',
                              style: Theme.of(context).textTheme.titleMedium,
                            ),
                            if (!_wifiExpanded && storedHint != null) ...[
                              const SizedBox(width: 8),
                              Expanded(
                                child: Text(
                                  storedHint,
                                  overflow: TextOverflow.ellipsis,
                                  style: Theme.of(context)
                                      .textTheme
                                      .bodySmall
                                      ?.copyWith(
                                        color: Theme.of(context)
                                            .colorScheme
                                            .onSurfaceVariant,
                                      ),
                                ),
                              ),
                            ] else
                              const Spacer(),
                            if (_wifiExpanded)
                              TextButton.icon(
                                onPressed:
                                    !ble.isConnected ? null : _reloadFromDevice,
                                icon: const Icon(Icons.download, size: 18),
                                label: const Text('장비값 읽기'),
                              ),
                          ],
                        ),
                      ),
                    ),
                    AnimatedCrossFade(
                      firstChild: const SizedBox.shrink(),
                      secondChild: _buildWifiSection(ble, storedHint),
                      crossFadeState: _wifiExpanded
                          ? CrossFadeState.showSecond
                          : CrossFadeState.showFirst,
                      duration: const Duration(milliseconds: 250),
                    ),
                    const SizedBox(height: 16),
                    OutlinedButton.icon(
                      onPressed: !ble.isConnected ? null : _syncPhoneTime,
                      icon: const Icon(Icons.access_time),
                      label: const Text('시간설정 (휴대폰 → RTC)'),
                    ),
                    const SizedBox(height: 8),
                    OutlinedButton.icon(
                      onPressed: !ble.isConnected ? null : _editName,
                      icon: const Icon(Icons.badge_outlined),
                      label: const Text('장치 이름 변경'),
                    ),
                    const SizedBox(height: 8),
                    OutlinedButton.icon(
                      onPressed: !ble.isConnected ? null : _editIp,
                      icon: const Icon(Icons.lan_outlined),
                      label: const Text('이더넷 IP 변경'),
                    ),
                    const SizedBox(height: 20),
                    InkWell(
                      borderRadius: BorderRadius.circular(8),
                      onTap: () => setState(
                          () => _quickCmdExpanded = !_quickCmdExpanded),
                      child: Padding(
                        padding: const EdgeInsets.symmetric(vertical: 4),
                        child: Row(
                          children: [
                            Icon(
                              _quickCmdExpanded
                                  ? Icons.expand_less
                                  : Icons.expand_more,
                              size: 22,
                            ),
                            const SizedBox(width: 4),
                            Text(
                              '빠른 명령',
                              style: Theme.of(context).textTheme.titleMedium,
                            ),
                          ],
                        ),
                      ),
                    ),
                    AnimatedCrossFade(
                      firstChild: const SizedBox.shrink(),
                      secondChild: Padding(
                        padding: const EdgeInsets.only(top: 8),
                        child: Wrap(
                          spacing: 8,
                          runSpacing: 8,
                          children: [
                            _CmdChip(
                              label: 'help',
                              onTap: () => ble.sendCommand('help'),
                            ),
                            _CmdChip(
                              label: 'version',
                              onTap: () => ble.sendCommand('version'),
                            ),
                            _CmdChip(
                              label: 'status',
                              onTap: () => ble.sendCommand('status'),
                            ),
                            _CmdChip(
                              label: 'ip',
                              onTap: () => ble.sendCommand('ip'),
                            ),
                            _CmdChip(
                              label: 'mac',
                              onTap: () => ble.sendCommand('mac'),
                            ),
                            _CmdChip(
                              label: 'name',
                              onTap: () => ble.sendCommand('name'),
                            ),
                            _CmdChip(
                              label: 'ssid?',
                              onTap: () {
                                _wifiFieldsTouched = false;
                                ble.fetchStoredWifi();
                              },
                            ),
                            _CmdChip(
                              label: 'time?',
                              onTap: () => ble.sendCommand('time'),
                            ),
                            _CmdChip(
                              label: '시간설정',
                              onTap: _syncPhoneTime,
                            ),
                            _CmdChip(
                              label: 'update',
                              color: Colors.deepOrange,
                              onTap: () => _confirmAndSend(
                                'OTA 업데이트 모드',
                                'update',
                              ),
                            ),
                            _CmdChip(
                              label: 'reboot',
                              color: Colors.redAccent,
                              onTap: () => _confirmAndSend('재부팅', 'reboot'),
                            ),
                          ],
                        ),
                      ),
                      crossFadeState: _quickCmdExpanded
                          ? CrossFadeState.showSecond
                          : CrossFadeState.showFirst,
                      duration: const Duration(milliseconds: 250),
                    ),
                    const SizedBox(height: 20),
                    Text(
                      '직접 명령',
                      style: Theme.of(context).textTheme.titleMedium,
                    ),
                    const SizedBox(height: 8),
                    Row(
                      children: [
                        Expanded(
                          child: TextField(
                            controller: _cmdCtrl,
                            decoration: const InputDecoration(
                              border: OutlineInputBorder(),
                              hintText: '예: status  또는  ip 192.168.0.57',
                            ),
                            onSubmitted: (v) async {
                              await ble.sendCommand(v);
                              _cmdCtrl.clear();
                            },
                          ),
                        ),
                        const SizedBox(width: 8),
                        FilledButton(
                          onPressed: !ble.isConnected
                              ? null
                              : () async {
                                  await ble.sendCommand(_cmdCtrl.text);
                                  _cmdCtrl.clear();
                                },
                          child: const Text('전송'),
                        ),
                      ],
                    ),
                  ],
                ),
              ),
            ),
            Expanded(
              flex: 4,
              child: Padding(
                padding: const EdgeInsets.fromLTRB(12, 0, 12, 12),
                child: LogConsole(
                  lines: ble.logLines,
                  shareTitle: 'IFTECH SAMWOO Log - $name',
                  onClear: () => ble.clearLog(),
                  onFullScreen: () => _pushFullScreenLog(),
                ),
              ),
            ),
          ],
        ),
    );
  }

  Widget _buildConnectionBar(NusBleService ble, String name) {
    final connected = ble.isConnected;
    return Material(
      color: Theme.of(context).colorScheme.surfaceContainerHighest,
      child: Padding(
        padding: const EdgeInsets.fromLTRB(16, 10, 16, 10),
        child: Row(
          children: [
            Icon(
              connected ? Icons.bluetooth_connected : Icons.bluetooth_disabled,
              color: connected
                  ? Theme.of(context).colorScheme.primary
                  : Theme.of(context).colorScheme.onSurfaceVariant,
            ),
            const SizedBox(width: 8),
            Expanded(
              child: Text(
                connected ? name : '장비가 연결되지 않았습니다',
                overflow: TextOverflow.ellipsis,
                style: Theme.of(context).textTheme.titleSmall,
              ),
            ),
            if (connected)
              OutlinedButton.icon(
                onPressed: () => ble.disconnect(),
                icon: const Icon(Icons.link_off, size: 18),
                label: const Text('해제'),
              )
            else
              FilledButton.icon(
                onPressed: _openConnect,
                icon: const Icon(Icons.bluetooth_searching, size: 18),
                label: const Text('연결'),
              ),
          ],
        ),
      ),
    );
  }

  Widget _buildWifiSection(NusBleService ble, String? storedHint) {
    return Column(
      crossAxisAlignment: CrossAxisAlignment.stretch,
      children: [
        if (storedHint != null) ...[
          const SizedBox(height: 4),
          Text(
            storedHint,
            style: Theme.of(context).textTheme.bodySmall?.copyWith(
                  color: Theme.of(context).colorScheme.onSurfaceVariant,
                ),
          ),
        ],
        const SizedBox(height: 8),
        TextField(
          controller: _ssidCtrl,
          decoration: InputDecoration(
            labelText: 'SSID',
            border: const OutlineInputBorder(),
            hintText: '무선 AP 이름 (직접 입력 가능)',
            suffixIcon: IconButton(
              tooltip: '주변 Wi-Fi 검색',
              onPressed: _pickWifiSsid,
              icon: const Icon(Icons.wifi_find),
            ),
          ),
          textInputAction: TextInputAction.next,
        ),
        const SizedBox(height: 8),
        OutlinedButton.icon(
          onPressed: _pickWifiSsid,
          icon: const Icon(Icons.wifi_find),
          label: const Text('주변 Wi-Fi 검색해서 선택'),
        ),
        const SizedBox(height: 10),
        TextField(
          controller: _passCtrl,
          obscureText: _obscurePass,
          decoration: InputDecoration(
            labelText: '비밀번호',
            border: const OutlineInputBorder(),
            hintText: '비우면 open (pass none)',
            suffixIcon: IconButton(
              onPressed: () =>
                  setState(() => _obscurePass = !_obscurePass),
              icon: Icon(
                _obscurePass ? Icons.visibility : Icons.visibility_off,
              ),
            ),
          ),
          onSubmitted: (_) async {
            await ble.setWifi(
              ssid: _ssidCtrl.text.trim(),
              password: _passCtrl.text,
            );
          },
        ),
        const SizedBox(height: 10),
        FilledButton.icon(
          onPressed: !ble.isConnected
              ? null
              : () async {
                  final ssid = _ssidCtrl.text.trim();
                  if (ssid.isEmpty) {
                    ScaffoldMessenger.of(context).showSnackBar(
                      const SnackBar(content: Text('SSID를 입력하세요')),
                    );
                    return;
                  }
                  await ble.setWifi(
                    ssid: ssid,
                    password: _passCtrl.text,
                  );
                },
          icon: const Icon(Icons.upload),
          label: const Text('SSID / PASS 장비로 전송'),
        ),
      ],
    );
  }
}

class _CmdChip extends StatelessWidget {
  const _CmdChip({
    required this.label,
    required this.onTap,
    this.color,
  });

  final String label;
  final VoidCallback onTap;
  final Color? color;

  @override
  Widget build(BuildContext context) {
    return ActionChip(
      label: Text(label),
      avatar: Icon(
        Icons.terminal,
        size: 16,
        color: color ?? Theme.of(context).colorScheme.primary,
      ),
      onPressed: onTap,
      side: color == null
          ? null
          : BorderSide(color: color!.withValues(alpha: 0.5)),
    );
  }
}
