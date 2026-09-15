import 'package:flutter/material.dart';
import 'package:provider/provider.dart';

import 'ble/nus_ble_service.dart';
import 'screens/control_screen.dart';

const _fwCyan = Color(0xFF2DE0C8);
const _fwBg = Color(0xFF071014);

void main() {
  WidgetsFlutterBinding.ensureInitialized();
  runApp(const IftechUpsApp());
}

class IftechUpsApp extends StatelessWidget {
  const IftechUpsApp({super.key});

  @override
  Widget build(BuildContext context) {
    return ChangeNotifierProvider(
      create: (_) => NusBleService(),
      child: MaterialApp(
        title: 'FW UPDATE',
        debugShowCheckedModeBanner: false,
        theme: _fwTheme(),
        themeMode: ThemeMode.dark,
        home: const ControlScreen(),
      ),
    );
  }
}

ThemeData _fwTheme() {
  const scheme = ColorScheme.dark(
    primary: _fwCyan,
    onPrimary: Color(0xFF04221C),
    secondary: _fwCyan,
    surface: Color(0xFF0C181C),
    onSurface: Color(0xFFE8F6F3),
    onSurfaceVariant: Color(0xFF8BA8A8),
    surfaceContainerHigh: Color(0xFF122025),
    surfaceContainerHighest: Color(0xFF163038),
    outline: Color(0xFF2A6A62),
  );
  return ThemeData(
    brightness: Brightness.dark,
    colorScheme: scheme,
    scaffoldBackgroundColor: _fwBg,
    useMaterial3: true,
    appBarTheme: const AppBarTheme(
      backgroundColor: Colors.transparent,
      foregroundColor: _fwCyan,
      elevation: 0,
      scrolledUnderElevation: 0,
      titleTextStyle: TextStyle(
        fontSize: 22,
        fontWeight: FontWeight.w600,
        letterSpacing: 1.4,
        color: Color(0xFFB8FFF3),
      ),
    ),
    filledButtonTheme: FilledButtonThemeData(
      style: FilledButton.styleFrom(
        backgroundColor: _fwCyan,
        foregroundColor: const Color(0xFF04221C),
        padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 12),
        shape: const StadiumBorder(),
      ),
    ),
    inputDecorationTheme: InputDecorationTheme(
      isDense: true,
      filled: true,
      fillColor: const Color(0xFF0A161A),
      hintStyle: const TextStyle(color: Color(0xFF6F8B8B)),
      border: OutlineInputBorder(
        borderRadius: BorderRadius.circular(14),
        borderSide: const BorderSide(color: Color(0xFF2A6A62)),
      ),
      enabledBorder: OutlineInputBorder(
        borderRadius: BorderRadius.circular(14),
        borderSide: const BorderSide(color: Color(0xFF2A6A62)),
      ),
      focusedBorder: OutlineInputBorder(
        borderRadius: BorderRadius.circular(14),
        borderSide: const BorderSide(color: _fwCyan, width: 1.4),
      ),
    ),
  );
}
