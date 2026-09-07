import 'dart:convert';
import 'dart:io';

import 'package:flutter_test/flutter_test.dart';

void main() {
  test(
    'native distribution notices retain every audited non-Flutter artifact',
    () {
      final inventory =
          jsonDecode(
                File('../release/android-dependencies.json').readAsStringSync(),
              )
              as Map<String, dynamic>;
      final notices = File(
        'licenses/ANDROID-NATIVE-NOTICES.txt',
      ).readAsStringSync();
      for (final row in inventory['dependencies'] as List<dynamic>) {
        final name = (row as Map<String, dynamic>)['coordinate'] as String;
        if (!name.startsWith('io.flutter:')) {
          expect(notices, contains(name));
        }
      }
      expect(notices, contains('Copyright 2015 KeepSafe Software Inc.'));
      expect(notices, contains('Apache Commons IO'));
      expect(notices, contains('mime / third_party/httpd'));
      expect(
        File('pubspec.yaml').readAsStringSync(),
        contains('- licenses/ANDROID-NATIVE-NOTICES.txt'),
      );
    },
  );
}
