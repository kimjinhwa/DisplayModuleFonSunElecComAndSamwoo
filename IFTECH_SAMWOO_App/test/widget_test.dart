import 'package:flutter_test/flutter_test.dart';
import 'package:iftech_samwoo_app/main.dart';

void main() {
  testWidgets('App loads control screen', (WidgetTester tester) async {
    await tester.pumpWidget(const IftechUpsApp());
    expect(find.text('IFTECH SAMWOO'), findsOneWidget);
    expect(find.text('연결'), findsOneWidget);
  });
}
