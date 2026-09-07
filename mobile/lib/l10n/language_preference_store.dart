import 'dart:io';

import 'package:flutter/widgets.dart';
import 'package:path/path.dart' as p;
import 'package:path_provider/path_provider.dart';

class LanguagePreferenceStore {
  const LanguagePreferenceStore({this.supportDirectory});

  final Future<Directory> Function()? supportDirectory;

  Future<File> _file() async {
    final directory = supportDirectory == null
        ? await getApplicationSupportDirectory()
        : await supportDirectory!();
    return File(p.join(directory.path, 'paper_mono', 'language.txt'));
  }

  Future<Locale> load() async {
    try {
      final file = await _file();
      if (!await file.exists()) return const Locale('ja');
      final code = (await file.readAsString()).trim();
      return Locale(code == 'en' ? 'en' : 'ja');
    } on Object {
      return const Locale('ja');
    }
  }

  Future<void> save(Locale locale) async {
    final file = await _file();
    await file.parent.create(recursive: true);
    await file.writeAsString(
      locale.languageCode == 'en' ? 'en' : 'ja',
      flush: true,
    );
  }

  Future<String> loadCode({String fallback = 'ja'}) async {
    final supportedFallback = fallback == 'en' ? 'en' : 'ja';
    try {
      final file = await _file();
      if (!await file.exists()) return supportedFallback;
      final code = (await file.readAsString()).trim();
      // Migrate the removed phone-language mode deterministically to Japanese.
      if (code == 'system') return 'ja';
      return const ['ja', 'en'].contains(code) ? code : supportedFallback;
    } on Object {
      return supportedFallback;
    }
  }

  Future<void> saveCode(String code) async {
    if (!const ['ja', 'en'].contains(code)) {
      throw ArgumentError.value(code, 'code');
    }
    final file = await _file();
    await file.parent.create(recursive: true);
    final temp = File('${file.path}.tmp');
    await temp.writeAsString(code, flush: true);
    await temp.rename(file.path);
  }
}
