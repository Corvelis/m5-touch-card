import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:touch_card/core/card/card_profile.dart';
import 'package:touch_card/core/card/workspace_store.dart';
import 'package:touch_card/touch_card_app.dart';
import 'package:image/image.dart' as img;

void main() {
  testWidgets('info opens licenses directly without an about dialog', (
    tester,
  ) async {
    await tester.pumpWidget(TouchCardApp(store: _MemoryStore()));
    await tester.pumpAndSettle();
    await tester.tap(find.byIcon(Icons.info_outline));
    await tester.pumpAndSettle();
    expect(find.byType(LicensePage), findsOneWidget);
    expect(find.byType(AboutDialog), findsNothing);
    expect(find.text('0.1.0-dev'), findsNothing);
    expect(find.textContaining('Touch Card対応ファームウェアが必要です'), findsNothing);
    tester.state<NavigatorState>(find.byType(Navigator).first).pop();
    await tester.pumpAndSettle();
    expect(find.text('自分の名刺'), findsOneWidget);
  });
  for (final exchangeSide in [256, 160]) {
    testWidgets('exchange preview fits a narrow phone at $exchangeSide px', (
      tester,
    ) async {
      tester.view.physicalSize = const Size(320, 900);
      tester.view.devicePixelRatio = 1;
      addTearDown(tester.view.resetPhysicalSize);
      addTearDown(tester.view.resetDevicePixelRatio);
      final jpeg = img.encodeJpg(
        img.Image(width: 256, height: 256, numChannels: 3),
      );
      final store = _MemoryStore();
      store.state = CardWorkspace(
        profile: CardProfile.create(),
        images: {
          ImageSlot.avatar: SlotImage(
            source: jpeg,
            jpeg: jpeg,
            width: 256,
            height: 256,
            device: CardDevice.paperMono,
            exchangeJpeg: img.encodeJpg(
              img.Image(
                width: exchangeSide,
                height: exchangeSide,
                numChannels: 3,
              ),
            ),
          ),
        },
      );
      await tester.pumpWidget(TouchCardApp(store: store));
      await tester.pumpAndSettle();
      await tester.ensureVisible(find.text('交換用アイコン'));
      await tester.tap(find.text('交換用アイコン'));
      await tester.pumpAndSettle();
      expect(find.text('本体表示'), findsOneWidget);
      expect(find.text('相手に渡す画像'), findsOneWidget);
      if (exchangeSide == 256) {
        expect(find.textContaining('256 × 256'), findsOneWidget);
        expect(find.text('交換用を160pxにする'), findsOneWidget);
      } else {
        expect(find.textContaining('160 × 160'), findsNWidgets(2));
        expect(find.text('交換用を160pxにする'), findsNothing);
        expect(find.textContaining('2秒での完了を保証'), findsNothing);
      }
      expect(tester.takeException(), isNull);
    });
  }
  for (final width in [320.0, 600.0]) {
    testWidgets('two tabs, local profile save and slot selection at $width', (
      tester,
    ) async {
      final store = _MemoryStore();
      tester.view.physicalSize = Size(width, 900);
      tester.view.devicePixelRatio = 1;
      addTearDown(tester.view.resetPhysicalSize);
      addTearDown(tester.view.resetDevicePixelRatio);
      await tester.pumpWidget(TouchCardApp(store: store));
      await tester.pumpAndSettle();
      expect(find.text('自分の名刺'), findsOneWidget);
      expect(find.text('Web名刺'), findsNothing);
      expect(find.byIcon(Icons.public), findsNothing);
      expect(find.byType(ClipOval), findsOneWidget);
      await tester.enterText(find.byType(TextField).first, 'テスト名刺');
      await tester.pump();
      await tester.ensureVisible(find.text('変更を保存'));
      await tester.tap(find.text('変更を保存'));
      await tester.pumpAndSettle();
      expect(store.state.profile.name, 'テスト名刺');
      await tester.tap(find.text('画像').last);
      await tester.pumpAndSettle();
      expect(find.text('ホーム画像'), findsOneWidget);
      await tester.tap(find.text('全画面'));
      await tester.pumpAndSettle();
      expect(tester.takeException(), isNull);
      await tester.tap(find.text('名刺').last);
      await tester.pumpAndSettle();
      expect(find.text('テスト名刺'), findsWidgets);
      expect(tester.takeException(), isNull);
      await tester.pumpWidget(const SizedBox.shrink());
    });
  }
}

// Filesystem durability is covered separately by card_workspace_test.dart.
class _MemoryStore extends WorkspaceStore {
  CardWorkspace state = CardWorkspace(profile: CardProfile.create());
  @override
  Future<CardWorkspace> load() async => state;
  @override
  Future<CardWorkspace> saveProfile({
    required String name,
    required String account,
    required String email,
    required String url,
    required String comment,
  }) async {
    state = CardWorkspace(
      profile: state.profile.revise(
        name: name,
        account: account,
        email: email,
        url: url,
        comment: comment,
      ),
      images: state.images,
    );
    return state;
  }
}
