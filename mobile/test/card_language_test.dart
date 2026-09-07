import 'dart:io';
import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:touch_card/core/card/card_profile.dart';
import 'package:touch_card/core/card/workspace_store.dart';
import 'package:touch_card/l10n/language_preference_store.dart';
import 'package:touch_card/touch_card_app.dart';

class MemoryLanguage extends LanguagePreferenceStore {
  MemoryLanguage(this.code, {this.fail = false});
  String code;
  final bool fail;
  @override
  Future<String> loadCode({String fallback = 'ja'}) async => code;
  @override
  Future<void> saveCode(String next) async {
    if (fail) throw const FileSystemException('Simulated write failure');
    code = next;
  }
}

class MemoryCards extends WorkspaceStore {
  final state = CardWorkspace(
    profile: CardProfile.create().revise(
      name: '名前',
      account: '@test',
      email: '',
      url: '',
      comment: '',
    ),
  );
  @override
  Future<CardWorkspace> load() async => state;
}

void main() {
  test(
    'language persists; saved choice wins; corrupt file falls back',
    () async {
      final dir = await Directory.systemTemp.createTemp('touch-card-language-');
      addTearDown(() => dir.delete(recursive: true));
      final store = LanguagePreferenceStore(supportDirectory: () async => dir);
      expect(await store.loadCode(), 'ja');
      expect(await store.loadCode(fallback: 'en'), 'en');
      await store.saveCode('ja');
      expect(await store.loadCode(fallback: 'en'), 'ja');
      await store.saveCode('en');
      expect(await store.loadCode(), 'en');
      await expectLater(store.saveCode('system'), throwsArgumentError);
      await expectLater(store.saveCode('invalid'), throwsArgumentError);
      expect(await store.loadCode(), 'en');
      await File('${dir.path}/paper_mono/language.txt').writeAsString('system');
      expect(await store.loadCode(fallback: 'en'), 'ja');
      await File(
        '${dir.path}/paper_mono/language.txt',
      ).writeAsString('invalid');
      expect(await store.loadCode(fallback: 'en'), 'en');
    },
  );

  testWidgets('switching preserves unsaved card and fits a 320px phone', (
    tester,
  ) async {
    tester.view.physicalSize = const Size(320, 900);
    tester.view.devicePixelRatio = 1;
    addTearDown(tester.view.resetPhysicalSize);
    addTearDown(tester.view.resetDevicePixelRatio);
    final language = MemoryLanguage('ja');
    final cards = MemoryCards();
    final originalName = cards.state.profile.name;
    await tester.pumpWidget(
      TouchCardApp(store: cards, languageStore: language),
    );
    await tester.pumpAndSettle();
    await tester.enterText(find.byType(TextField).first, '未保存の名前');
    expect(find.byIcon(Icons.settings_outlined), findsNothing);
    expect(find.byIcon(Icons.language), findsOneWidget);
    await tester.tap(find.byTooltip('言語 / Language'));
    await tester.pumpAndSettle();
    expect(find.byType(CheckedPopupMenuItem<String>), findsNWidgets(2));
    expect(
      tester.getTopLeft(find.byType(CheckedPopupMenuItem<String>).first).dy,
      greaterThan(tester.getBottomLeft(find.byIcon(Icons.language)).dy),
    );
    expect(find.text('スマホの設定に合わせる'), findsNothing);
    await tester.tap(
      find.widgetWithText(CheckedPopupMenuItem<String>, 'English'),
    );
    await tester.pumpAndSettle();
    expect(language.code, 'en');
    expect(find.byType(CheckedPopupMenuItem<String>), findsNothing);
    expect(find.byType(BackButton), findsNothing);
    expect(find.text('My card'), findsOneWidget);
    expect(find.text('Web card'), findsNothing);
    expect(find.byIcon(Icons.public), findsNothing);
    expect(find.text('未保存の名前'), findsWidgets);
    expect(cards.state.profile.name, originalName);
    await tester.tap(find.text('Images').last);
    await tester.pumpAndSettle();
    expect(find.text('Home images'), findsOneWidget);
    expect(find.text('Clock & calendar'), findsOneWidget);
    expect(tester.takeException(), isNull);
    await tester.tap(find.byTooltip('言語 / Language'));
    await tester.pumpAndSettle();
    await tester.tap(find.widgetWithText(CheckedPopupMenuItem<String>, '日本語'));
    await tester.pumpAndSettle();
    expect(find.text('ホーム画像'), findsOneWidget);
    expect(language.code, 'ja');
    expect(tester.takeException(), isNull);
  });

  testWidgets('failed save keeps current language and reports an error', (
    tester,
  ) async {
    final language = MemoryLanguage('ja', fail: true);
    await tester.pumpWidget(
      TouchCardApp(store: MemoryCards(), languageStore: language),
    );
    await tester.pumpAndSettle();
    await tester.tap(find.byTooltip('言語 / Language'));
    await tester.pumpAndSettle();
    await tester.tap(
      find.widgetWithText(CheckedPopupMenuItem<String>, 'English'),
    );
    await tester.pumpAndSettle();
    expect(language.code, 'ja');
    expect(find.text('言語を保存できませんでした'), findsOneWidget);
    expect(find.text('自分の名刺'), findsOneWidget);
  });

  testWidgets('old system preference no longer follows phone language', (
    tester,
  ) async {
    tester.platformDispatcher.localeTestValue = const Locale('en');
    addTearDown(tester.platformDispatcher.clearLocaleTestValue);
    await tester.pumpWidget(
      TouchCardApp(
        store: MemoryCards(),
        languageStore: MemoryLanguage('system'),
      ),
    );
    await tester.pumpAndSettle();
    expect(find.text('自分の名刺'), findsOneWidget);
    await tester.tap(find.byTooltip('言語 / Language'));
    await tester.pumpAndSettle();
    expect(find.text('日本語'), findsOneWidget);
    expect(find.text('English'), findsOneWidget);
    expect(find.text('Use phone language'), findsNothing);
  });
}
