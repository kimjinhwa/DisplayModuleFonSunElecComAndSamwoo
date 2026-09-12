import 'package:flutter/material.dart';
import 'package:provider/provider.dart';

import '../ble/nus_ble_service.dart';

/// UPS 정격 설정 (BLE `rating` CLI).
class UpsSettingsScreen extends StatefulWidget {
  const UpsSettingsScreen({super.key});

  @override
  State<UpsSettingsScreen> createState() => _UpsSettingsScreenState();
}

class _UpsSettingsScreenState extends State<UpsSettingsScreen> {
  final _kvaCtrl = TextEditingController(text: '10');
  final _batCtrl = TextEditingController(text: '192');
  final _inCtrl = TextEditingController(text: '220');
  final _outCtrl = TextEditingController(text: '220');
  bool _fieldsTouched = false;
  int _lastRatingVersion = -1;
  bool _saving = false;

  @override
  void initState() {
    super.initState();
    for (final c in [_kvaCtrl, _batCtrl, _inCtrl, _outCtrl]) {
      c.addListener(() => _fieldsTouched = true);
    }
    WidgetsBinding.instance.addPostFrameCallback((_) {
      _reloadFromDevice(force: true);
    });
  }

  @override
  void dispose() {
    _kvaCtrl.dispose();
    _batCtrl.dispose();
    _inCtrl.dispose();
    _outCtrl.dispose();
    super.dispose();
  }

  void _applyDeviceRating(NusBleService ble, {bool force = false}) {
    if (!force && _fieldsTouched) return;
    if (ble.ratingConfigVersion == _lastRatingVersion && !force) return;

    final r = ble.deviceRating;
    if (r == null) {
      _lastRatingVersion = ble.ratingConfigVersion;
      return;
    }

    setState(() {
      _lastRatingVersion = ble.ratingConfigVersion;
      _kvaCtrl.text = _formatKva(r.kva);
      _batCtrl.text = '${r.batV}';
      _inCtrl.text = '${r.inV}';
      _outCtrl.text = '${r.outV}';
      _fieldsTouched = false;
    });
  }

  String _formatKva(double kva) {
    if (kva == kva.roundToDouble()) {
      return '${kva.toInt()}';
    }
    return kva.toStringAsFixed(1);
  }

  Future<void> _reloadFromDevice({bool force = false}) async {
    if (force) _fieldsTouched = false;
    await context.read<NusBleService>().fetchStoredRating();
    if (mounted) {
      ScaffoldMessenger.of(context).showSnackBar(
        const SnackBar(
          content: Text('장비에서 UPS 정격을 읽는 중…'),
          duration: Duration(seconds: 1),
        ),
      );
    }
  }

  Future<void> _save() async {
    final kva = double.tryParse(_kvaCtrl.text.trim().replaceAll(',', '.'));
    final bat = int.tryParse(_batCtrl.text.trim());
    final input = int.tryParse(_inCtrl.text.trim());
    final output = int.tryParse(_outCtrl.text.trim());

    if (kva == null || kva <= 0) {
      _showError('KVA 값을 확인하세요');
      return;
    }
    if (bat == null || bat <= 0 || input == null || input <= 0 || output == null || output <= 0) {
      _showError('전압 값을 확인하세요');
      return;
    }

    setState(() => _saving = true);
    try {
      await context.read<NusBleService>().setRating(
            kva: kva,
            batV: bat,
            inV: input,
            outV: output,
          );
      if (mounted) {
        setState(() => _fieldsTouched = false);
        ScaffoldMessenger.of(context).showSnackBar(
          const SnackBar(content: Text('UPS 정격이 저장되었습니다')),
        );
      }
    } finally {
      if (mounted) setState(() => _saving = false);
    }
  }

  void _showError(String msg) {
    ScaffoldMessenger.of(context).showSnackBar(
      SnackBar(content: Text(msg)),
    );
  }

  @override
  Widget build(BuildContext context) {
    final ble = context.watch<NusBleService>();
    if (ble.ratingConfigVersion != _lastRatingVersion) {
      WidgetsBinding.instance.addPostFrameCallback((_) {
        if (!mounted) return;
        _applyDeviceRating(context.read<NusBleService>());
      });
    }

    return Scaffold(
      appBar: AppBar(
        title: const Text('UPS설정기본'),
        actions: [
          TextButton.icon(
            onPressed: !ble.isConnected || _saving
                ? null
                : () => _reloadFromDevice(force: true),
            icon: const Icon(Icons.download, size: 18),
            label: const Text('장비값 읽기'),
          ),
        ],
      ),
      body: ListView(
        padding: const EdgeInsets.all(16),
        children: [
          Text(
            'INVT 등 Modbus UPS의 초기화면 정격을 설정합니다.\n'
            'Megatec는 F/GF 응답이 우선 적용됩니다.',
            style: Theme.of(context).textTheme.bodyMedium?.copyWith(
                  color: Theme.of(context).colorScheme.onSurfaceVariant,
                ),
          ),
          const SizedBox(height: 20),
          TextField(
            controller: _kvaCtrl,
            keyboardType: const TextInputType.numberWithOptions(decimal: true),
            decoration: const InputDecoration(
              labelText: '용량 (KVA)',
              border: OutlineInputBorder(),
              suffixText: 'kVA',
            ),
          ),
          const SizedBox(height: 12),
          TextField(
            controller: _batCtrl,
            keyboardType: TextInputType.number,
            decoration: const InputDecoration(
              labelText: '배터리 정격 전압',
              border: OutlineInputBorder(),
              suffixText: 'V',
            ),
          ),
          const SizedBox(height: 12),
          TextField(
            controller: _inCtrl,
            keyboardType: TextInputType.number,
            decoration: const InputDecoration(
              labelText: '입력 정격 전압',
              border: OutlineInputBorder(),
              suffixText: 'V',
            ),
          ),
          const SizedBox(height: 12),
          TextField(
            controller: _outCtrl,
            keyboardType: TextInputType.number,
            decoration: const InputDecoration(
              labelText: '출력 정격 전압',
              border: OutlineInputBorder(),
              suffixText: 'V',
            ),
          ),
          const SizedBox(height: 24),
          FilledButton.icon(
            onPressed: !ble.isConnected || _saving ? null : _save,
            icon: _saving
                ? const SizedBox(
                    width: 18,
                    height: 18,
                    child: CircularProgressIndicator(strokeWidth: 2),
                  )
                : const Icon(Icons.save),
            label: Text(_saving ? '저장 중…' : '장비에 저장'),
          ),
        ],
      ),
    );
  }
}
