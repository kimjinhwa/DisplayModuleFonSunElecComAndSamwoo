import 'package:flutter/material.dart';
import 'package:provider/provider.dart';

import 'ble/nus_ble_service.dart';
import 'screens/control_screen.dart';

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
        title: 'IFTECH SAMWOO',
        debugShowCheckedModeBanner: false,
        theme: ThemeData(
          colorScheme: ColorScheme.fromSeed(
            seedColor: const Color(0xFF0B6E4F),
            brightness: Brightness.light,
          ),
          useMaterial3: true,
          inputDecorationTheme: const InputDecorationTheme(
            isDense: true,
          ),
        ),
        darkTheme: ThemeData(
          colorScheme: ColorScheme.fromSeed(
            seedColor: const Color(0xFF0B6E4F),
            brightness: Brightness.dark,
          ),
          useMaterial3: true,
        ),
        themeMode: ThemeMode.system,
        home: const ControlScreen(),
      ),
    );
  }
}
