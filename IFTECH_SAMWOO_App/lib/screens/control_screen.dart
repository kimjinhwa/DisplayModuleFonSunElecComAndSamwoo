import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:provider/provider.dart';

import '../ble/nus_ble_service.dart';
import '../widgets/log_console.dart';
import '../widgets/wifi_scan_sheet.dart';
import 'help_screen.dart';
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
  bool _wifiExpanded = false;
  bool _quickCmdExpanded = false;
  final _scaffoldKey = GlobalKey<ScaffoldState>();

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
      _openLog();
      ScaffoldMessenger.of(context).showSnackBar(
        const SnackBar(
          content: Text('장비에서 Wi-Fi 설정을 읽는 중…'),
          duration: Duration(seconds: 1),
        ),
      );
    }
  }

  void _openLog() {
    _scaffoldKey.currentState?.openEndDrawer();
  }

  Future<void> _sendAndShowLog(Future<void> Function() action) async {
    await action();
    if (mounted) _openLog();
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
      await _sendAndShowLog(
        () => context.read<NusBleService>().sendCommand(command),
      );
    }
  }

  Future<void> _editTime() async {
    final ble = context.read<NusBleService>();
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

    final choice = await showDialog<String>(
      context: context,
      builder: (ctx) => AlertDialog(
        title: const Text('시간'),
        content: Text(
          '장비 RTC를 읽거나, 휴대폰 시각으로 맞출 수 있습니다.\n\n휴대폰: $stamp',
        ),
        actions: [
          TextButton(
            onPressed: () => Navigator.pop(ctx),
            child: const Text('취소'),
          ),
          TextButton(
            onPressed: () => Navigator.pop(ctx, 'read'),
            child: const Text('현재값 읽기'),
          ),
          FilledButton(
            onPressed: () => Navigator.pop(ctx, 'set'),
            child: const Text('휴대폰으로 설정'),
          ),
        ],
      ),
    );
    if (!mounted || choice == null) return;
    if (choice == 'read') {
      await _sendAndShowLog(() => ble.sendCommand('time'));
    } else if (choice == 'set') {
      await _sendAndShowLog(() => ble.sendCommand(cmd));
    }
  }

  Future<void> _syncPhoneTime() async {
    await _editTime();
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
      await _sendAndShowLog(() => ble.setDeviceName(result));
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
      await _sendAndShowLog(() => ble.sendCommand('ip $ip $sn $gw'));
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
        'FW UPDATE Log - ${ble.device?.platformName ?? "device"}';
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
    final name = ble.device?.platformName ?? '장비';
    final storedHint = ble.deviceSsid == null
        ? null
        : '장비 저장값: ${ble.deviceSsid}'
            '${(ble.devicePass == null || ble.devicePass!.isEmpty) ? ' / (open)' : ' / ••••••'}';

    return Scaffold(
        key: _scaffoldKey,
        backgroundColor: const Color(0xFF071014),
        endDrawerEnableOpenDragGesture: true,
        endDrawer: Drawer(
          backgroundColor: const Color(0xFF0C181C),
          width: MediaQuery.sizeOf(context).width * 0.92,
          child: SafeArea(
            child: Padding(
              padding: const EdgeInsets.fromLTRB(8, 8, 8, 12),
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.stretch,
                children: [
                  Row(
                    children: [
                      IconButton(
                        tooltip: '닫기',
                        onPressed: () =>
                            _scaffoldKey.currentState?.closeEndDrawer(),
                        icon: const Icon(Icons.chevron_right),
                      ),
                      Text('로그', style: Theme.of(context).textTheme.titleMedium),
                      const Spacer(),
                    ],
                  ),
                  const SizedBox(height: 4),
                  Expanded(
                    child: LogConsole(
                      lines: ble.logLines,
                      shareTitle: 'FW UPDATE Log - $name',
                      onClear: () => ble.clearLog(),
                      onFullScreen: () => _pushFullScreenLog(),
                    ),
                  ),
                ],
              ),
            ),
          ),
        ),
        appBar: AppBar(
          title: const Text('FW UPDATE'),
          actions: [
            IconButton(
              tooltip: '도움말',
              onPressed: () {
                Navigator.of(context).push(
                  MaterialPageRoute(builder: (_) => const HelpScreen()),
                );
              },
              icon: const Icon(Icons.help_outline),
            ),
            IconButton(
              tooltip: '로그',
              onPressed: _openLog,
              icon: const Icon(Icons.terminal_outlined),
            ),
          ],
        ),
        body: Stack(
          children: [
            const Positioned.fill(child: CustomPaint(painter: _CircuitPainter())),
            Column(
              children: [
                Expanded(
                  child: SingleChildScrollView(
                    padding: const EdgeInsets.fromLTRB(16, 4, 16, 16),
                    child: Column(
                      crossAxisAlignment: CrossAxisAlignment.stretch,
                      children: [
                        _buildConnectionBar(ble, name),
                        const SizedBox(height: 18),
                        const _SectionLabel('SYSTEM CONFIG'),
                        const SizedBox(height: 8),
                        _buildWifiCard(ble, storedHint),
                        const SizedBox(height: 10),
                        _buildTimeRow(ble),
                        const SizedBox(height: 18),
                        if (ble.isConnected) ...[
                          _buildDeviceCard(ble, name),
                          const SizedBox(height: 18),
                        ],
                        const _SectionLabel('빠른 명령'),
                        const SizedBox(height: 8),
                        _buildQuickCmdCard(ble),
                      ],
                    ),
                  ),
                ),
                SafeArea(
                  top: false,
                  child: _buildDirectCommandBar(ble),
                ),
              ],
            ),
          ],
        ),
    );
  }

  Widget _buildConnectionBar(NusBleService ble, String name) {
    final connected = ble.isConnected;
    return _GlowCard(
      padding: const EdgeInsets.fromLTRB(12, 10, 10, 10),
      child: Row(
        children: [
          Container(
            width: 46,
            height: 46,
            decoration: BoxDecoration(
              shape: BoxShape.circle,
              color: const Color(0xFF0A2228),
              border: Border.all(color: const Color(0xFF2DE0C8), width: 1.4),
            ),
            child: Icon(
              connected ? Icons.bluetooth : Icons.bluetooth_disabled,
              color: const Color(0xFF2DE0C8),
            ),
          ),
          const SizedBox(width: 10),
          Expanded(
            child: Text(
              connected ? name : '장비가 연결되지 않았습니다',
              overflow: TextOverflow.ellipsis,
              style: const TextStyle(
                fontWeight: FontWeight.w600,
                letterSpacing: 0.2,
              ),
            ),
          ),
          _StatusDots(on: connected),
          const SizedBox(width: 8),
          if (connected)
            FilledButton.icon(
              onPressed: () => ble.disconnect(),
              icon: const Icon(Icons.link_off, size: 16),
              label: const Text('해제'),
            )
          else
            FilledButton.icon(
              onPressed: _openConnect,
              icon: const Icon(Icons.bluetooth_searching, size: 16),
              label: const Text('연결'),
            ),
        ],
      ),
    );
  }

  Widget _buildTimeRow(NusBleService ble) {
    return _GlowCard(
      padding: const EdgeInsets.fromLTRB(14, 10, 10, 10),
      child: Row(
        children: [
          const Icon(Icons.access_time, color: Color(0xFF2DE0C8), size: 22),
          const SizedBox(width: 10),
          const Expanded(
            child: Text('시간설정 (휴대폰 → RTC)'),
          ),
          FilledButton(
            onPressed: ble.isConnected ? _syncPhoneTime : null,
            child: const Text('Update'),
          ),
        ],
      ),
    );
  }

  Widget _buildDeviceCard(NusBleService ble, String name) {
    final deviceName =
        ble.deviceName?.isNotEmpty == true ? ble.deviceName! : name;
    return _GlowCard(
      padding: const EdgeInsets.fromLTRB(16, 16, 12, 14),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Row(
            children: [
              const Expanded(
                child: Text(
                  '장치 상세 정보',
                  style: TextStyle(
                    fontSize: 16,
                    fontWeight: FontWeight.w700,
                  ),
                ),
              ),
              IconButton(
                tooltip: '이름 변경',
                onPressed: ble.isConnected ? _editName : null,
                icon: const Icon(Icons.edit_outlined, size: 18),
              ),
              IconButton(
                tooltip: '이더넷 IP',
                onPressed: ble.isConnected ? _editIp : null,
                icon: const Icon(Icons.lan_outlined, size: 18),
              ),
            ],
          ),
          const SizedBox(height: 8),
          _kv('Device Name', deviceName),
          _kv(
            'MacAddress :',
            ble.displayMac ?? '-',
            copyValue: ble.displayMac,
          ),
          if (ble.ethMac != null &&
              ble.ethMac!.isNotEmpty &&
              ble.ethMac != ble.displayMac)
            _kv('ETH MAC :', ble.ethMac!, copyValue: ble.ethMac),
        ],
      ),
    );
  }

  Widget _kv(String label, String value, {String? copyValue}) {
    return Padding(
      padding: const EdgeInsets.only(bottom: 10),
      child: Row(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          SizedBox(
            width: 108,
            child: Text(
              label,
              style: const TextStyle(
                color: Color(0xFF8BA8A8),
                fontSize: 13,
              ),
            ),
          ),
          Expanded(
            child: Text(
              value,
              textAlign: TextAlign.right,
              style: const TextStyle(
                fontWeight: FontWeight.w700,
                fontSize: 15,
                letterSpacing: 0.2,
              ),
            ),
          ),
          if (copyValue != null && copyValue.isNotEmpty)
            IconButton(
              visualDensity: VisualDensity.compact,
              tooltip: '복사',
              onPressed: () async {
                await Clipboard.setData(ClipboardData(text: copyValue));
                if (!mounted) return;
                ScaffoldMessenger.of(context).showSnackBar(
                  const SnackBar(content: Text('복사했습니다')),
                );
              },
              icon: const Icon(Icons.copy, size: 16, color: Color(0xFF2DE0C8)),
            ),
        ],
      ),
    );
  }

  Widget _buildWifiCard(NusBleService ble, String? storedHint) {
    final hasSsid = ble.deviceSsid != null && ble.deviceSsid!.isNotEmpty;
    return _GlowCard(
      padding: EdgeInsets.zero,
      child: Column(
        children: [
          ListTile(
            onTap: () => setState(() => _wifiExpanded = !_wifiExpanded),
            leading: const Icon(Icons.wifi, color: Color(0xFF2DE0C8)),
            title: const Text('Wi-Fi 설정'),
            subtitle: Text(
              hasSsid
                  ? '장비 저장값: ${ble.deviceSsid}'
                  : 'OTA용 AP SSID / 비밀번호',
              overflow: TextOverflow.ellipsis,
            ),
            trailing: Row(
              mainAxisSize: MainAxisSize.min,
              children: [
                Container(
                  width: 10,
                  height: 10,
                  decoration: BoxDecoration(
                    shape: BoxShape.circle,
                    color: hasSsid
                        ? const Color(0xFF3DFF8A)
                        : const Color(0xFF3A5555),
                  ),
                ),
                const SizedBox(width: 8),
                Icon(
                  _wifiExpanded ? Icons.expand_less : Icons.expand_more,
                  color: const Color(0xFF2DE0C8),
                ),
              ],
            ),
          ),
          AnimatedCrossFade(
            firstChild: const SizedBox.shrink(),
            secondChild: Padding(
              padding: const EdgeInsets.fromLTRB(16, 0, 16, 16),
              child: Column(
                children: [
                  Align(
                    alignment: Alignment.centerRight,
                    child: TextButton.icon(
                      onPressed: !ble.isConnected ? null : _reloadFromDevice,
                      icon: const Icon(Icons.download, size: 18),
                      label: const Text('장비값 읽기'),
                    ),
                  ),
                  _buildWifiSection(ble, storedHint),
                ],
              ),
            ),
            crossFadeState: _wifiExpanded
                ? CrossFadeState.showSecond
                : CrossFadeState.showFirst,
            duration: const Duration(milliseconds: 220),
          ),
        ],
      ),
    );
  }

  Widget _buildQuickCmdCard(NusBleService ble) {
    return _GlowCard(
      padding: EdgeInsets.zero,
      child: Column(
        children: [
          ListTile(
            onTap: () =>
                setState(() => _quickCmdExpanded = !_quickCmdExpanded),
            leading: const Icon(Icons.bolt, color: Color(0xFF2DE0C8)),
            title: const Text('빠른 명령'),
            trailing: Icon(
              _quickCmdExpanded ? Icons.expand_less : Icons.expand_more,
              color: const Color(0xFF2DE0C8),
            ),
          ),
          AnimatedCrossFade(
            firstChild: const SizedBox.shrink(),
            secondChild: Padding(
              padding: const EdgeInsets.fromLTRB(12, 0, 12, 14),
              child: Wrap(
                spacing: 8,
                runSpacing: 8,
                children: [
                  _CmdChip(
                    label: 'help',
                    onTap: () => _sendAndShowLog(
                      () => ble.sendCommand('help'),
                    ),
                  ),
                  _CmdChip(
                    label: 'version',
                    onTap: () => _sendAndShowLog(
                      () => ble.sendCommand('version'),
                    ),
                  ),
                  _CmdChip(
                    label: 'status',
                    onTap: () => _sendAndShowLog(
                      () => ble.sendCommand('status'),
                    ),
                  ),
                  _CmdChip(
                    label: 'ip',
                    onTap: _editIp,
                  ),
                  _CmdChip(
                    label: 'mac',
                    onTap: () => _sendAndShowLog(
                      () => ble.sendCommand('mac'),
                    ),
                  ),
                  _CmdChip(
                    label: 'name',
                    onTap: _editName,
                  ),
                  _CmdChip(
                    label: 'ssid?',
                    onTap: () {
                      _wifiFieldsTouched = false;
                      _sendAndShowLog(() => ble.fetchStoredWifi());
                    },
                  ),
                  _CmdChip(
                    label: 'time',
                    onTap: _editTime,
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
            duration: const Duration(milliseconds: 220),
          ),
        ],
      ),
    );
  }

  Widget _buildDirectCommandBar(NusBleService ble) {
    return Padding(
      padding: const EdgeInsets.fromLTRB(16, 0, 16, 8),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.stretch,
        mainAxisSize: MainAxisSize.min,
        children: [
          const _SectionLabel('직접 명령'),
          const SizedBox(height: 8),
          _GlowCard(
            padding: const EdgeInsets.fromLTRB(12, 10, 10, 10),
            child: Row(
              children: [
                Expanded(
                  child: TextField(
                    controller: _cmdCtrl,
                    decoration: const InputDecoration(
                      hintText: '예: status  또는  ip 192.168.0.57',
                      border: InputBorder.none,
                      enabledBorder: InputBorder.none,
                      focusedBorder: InputBorder.none,
                      filled: false,
                    ),
                    onSubmitted: (v) async {
                      await _sendAndShowLog(() => ble.sendCommand(v));
                      _cmdCtrl.clear();
                    },
                  ),
                ),
                FilledButton(
                  onPressed: !ble.isConnected
                      ? null
                      : () async {
                          await _sendAndShowLog(
                            () => ble.sendCommand(_cmdCtrl.text),
                          );
                          _cmdCtrl.clear();
                        },
                  child: const Text('전송  ▶'),
                ),
              ],
            ),
          ),
        ],
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
            await _sendAndShowLog(() => ble.setWifi(
                  ssid: _ssidCtrl.text.trim(),
                  password: _passCtrl.text,
                ));
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
                  await _sendAndShowLog(() => ble.setWifi(
                        ssid: ssid,
                        password: _passCtrl.text,
                      ));
                },
          icon: const Icon(Icons.upload),
          label: const Text('SSID / PASS 장비로 전송'),
        ),
      ],
    );
  }
}

class _GlowCard extends StatelessWidget {
  const _GlowCard({required this.child, this.padding = const EdgeInsets.all(12)});

  final Widget child;
  final EdgeInsets padding;

  @override
  Widget build(BuildContext context) {
    return Material(
      color: const Color(0xCC0E1C22),
      elevation: 0,
      shadowColor: const Color(0x2200FFCC),
      clipBehavior: Clip.antiAlias,
      shape: RoundedRectangleBorder(
        borderRadius: BorderRadius.circular(18),
        side: const BorderSide(color: Color(0x662DE0C8)),
      ),
      child: Padding(padding: padding, child: child),
    );
  }
}

class _SectionLabel extends StatelessWidget {
  const _SectionLabel(this.text);
  final String text;

  @override
  Widget build(BuildContext context) {
    return Text(
      text,
      style: const TextStyle(
        color: Color(0xFF7AD9CC),
        fontSize: 12,
        letterSpacing: 1.4,
        fontWeight: FontWeight.w700,
      ),
    );
  }
}

class _StatusDots extends StatelessWidget {
  const _StatusDots({required this.on});
  final bool on;

  @override
  Widget build(BuildContext context) {
    final color = on ? const Color(0xFF3DFF8A) : const Color(0xFF3A5555);
    return Row(
      mainAxisSize: MainAxisSize.min,
      children: [
        for (var i = 0; i < 2; i++)
          Padding(
            padding: const EdgeInsets.only(left: 4),
            child: Container(
              width: 8,
              height: 8,
              decoration: BoxDecoration(shape: BoxShape.circle, color: color),
            ),
          ),
      ],
    );
  }
}

class _CircuitPainter extends CustomPainter {
  const _CircuitPainter();

  @override
  void paint(Canvas canvas, Size size) {
    final paint = Paint()
      ..color = const Color(0x142DE0C8)
      ..strokeWidth = 1
      ..style = PaintingStyle.stroke;
    final path = Path()
      ..moveTo(0, size.height * 0.12)
      ..lineTo(size.width * 0.35, size.height * 0.12)
      ..lineTo(size.width * 0.35, size.height * 0.38)
      ..lineTo(size.width * 0.78, size.height * 0.38)
      ..moveTo(size.width * 0.55, 0)
      ..lineTo(size.width * 0.55, size.height * 0.22)
      ..lineTo(size.width, size.height * 0.22)
      ..moveTo(size.width * 0.18, size.height)
      ..lineTo(size.width * 0.18, size.height * 0.62)
      ..lineTo(size.width * 0.92, size.height * 0.62)
      ..lineTo(size.width * 0.92, size.height * 0.85);
    canvas.drawPath(path, paint);
  }

  @override
  bool shouldRepaint(covariant CustomPainter oldDelegate) => false;
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
      side: BorderSide(
        color: color ?? const Color(0xFF2DE0C8).withValues(alpha: 0.55),
      ),
    );
  }
}
