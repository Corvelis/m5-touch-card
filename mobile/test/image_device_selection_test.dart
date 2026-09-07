import 'dart:convert';
import 'package:flutter/material.dart';
import 'package:flutter_localizations/flutter_localizations.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:image/image.dart' as img;
import 'package:touch_card/core/card/card_profile.dart';
import 'package:touch_card/core/card/workspace_store.dart';
import 'package:touch_card/core/image/card_image_processor.dart';
import 'package:touch_card/core/protocol/card_packet.dart';
import 'package:touch_card/touch_card_app.dart';

class ImageStore extends WorkspaceStore {
  ImageStore(this.state);
  CardWorkspace state;
  bool fail = false;
  int attempts = 0;
  @override
  Future<CardWorkspace> load() async => state;
  @override
  Future<CardWorkspace> saveImage(ImageSlot slot, SlotImage image) async {
    attempts++;
    if (fail) throw const FormatException('画像を保存できません');
    image.validate();
    state = CardWorkspace(
      profile: state.profile,
      images: {...state.images, slot: image},
      pending: state.pending,
    );
    return state;
  }
}

SlotImage fixture(ImageSlot slot) {
  final (w, h) = CardImageProcessor.dimensions(slot, CardDevice.paperMono);
  final original = img.Image(width: 200, height: 160, numChannels: 3);
  for (final p in original) {
    original.setPixelRgb(p.x, p.y, p.x, p.y, 80);
  }
  return SlotImage(
    source: img.encodePng(original),
    jpeg: img.encodeJpg(img.Image(width: w, height: h, numChannels: 3)),
    width: w,
    height: h,
    device: CardDevice.paperMono,
  );
}

Future<void> choose(
  WidgetTester tester,
  ImageStore store,
  CardDevice target,
) async {
  final previous = store.attempts;
  await tester.runAsync(() async {
    await tester.ensureVisible(
      find.byType(DropdownButtonFormField<CardDevice>),
    );
    await tester.tap(find.byType(DropdownButtonFormField<CardDevice>));
    await tester.pumpAndSettle();
    await tester.tap(
      find.text(target == CardDevice.paperMono ? 'Paper Mono' : 'ｽﾀｯｸﾁｬﾝ').last,
    );
    // Let the dropdown finish closing so its selection callback starts in the
    // real async zone before waiting for the image-processing isolate.
    await tester.pump();
    await tester.pump(const Duration(seconds: 1));
    for (int i = 0; i < 500 && store.attempts == previous; ++i) {
      await Future<void>.delayed(const Duration(milliseconds: 20));
    }
    await Future<void>.delayed(const Duration(milliseconds: 20));
  });
  await tester.pumpAndSettle();
  expect(store.attempts, previous + 1);
}

Widget app(ImageStore store) => MaterialApp(
  locale: const Locale('ja'),
  supportedLocales: const [Locale('ja'), Locale('en')],
  localizationsDelegates: GlobalMaterialLocalizations.delegates,
  home: WorkspaceScreen(store: store),
);

void main() {
  for (final slot in [ImageSlot.dashboard, ImageSlot.fullscreen]) {
    testWidgets(
      'device selection automatically crops $slot and preserves other data',
      (tester) async {
        tester.view.physicalSize = const Size(320, 1000);
        tester.view.devicePixelRatio = 1;
        addTearDown(tester.view.resetPhysicalSize);
        addTearDown(tester.view.resetDevicePixelRatio);
        final images = {for (final s in ImageSlot.values) s: fixture(s)};
        final originalState = CardWorkspace(
          profile: CardProfile.create(),
          images: images,
        );
        final pending = CardPacket.create(originalState, 3);
        final store = ImageStore(
          CardWorkspace(
            profile: originalState.profile,
            images: images,
            pending: pending,
          ),
        );
        await tester.pumpWidget(app(store));
        await tester.pumpAndSettle();
        await tester.enterText(find.byType(TextField).first, '未保存');
        await tester.tap(find.text('画像').last);
        await tester.pumpAndSettle();
        if (slot == ImageSlot.fullscreen) {
          await tester.tap(find.text('全画面'));
          await tester.pumpAndSettle();
        }
        expect(find.text('画像を送る端末'), findsOneWidget);
        expect(find.textContaining('端末の選択だけ'), findsNothing);
        await choose(tester, store, CardDevice.stackchan);
        final converted = store.state.images[slot]!;
        final (w, h) = CardImageProcessor.dimensions(
          slot,
          CardDevice.stackchan,
        );
        expect((converted.width, converted.height), (w, h));
        expect(converted.source, images[slot]!.source);
        final jpeg = img.decodeJpg(converted.jpeg)!;
        expect((jpeg.width, jpeg.height), (w, h));
        expect(
          tester.widget<AspectRatio>(find.byType(AspectRatio)).aspectRatio,
          w / h,
        );
        final payload =
            jsonDecode(
                  utf8.decode(
                    CardPacket.create(
                      store.state,
                      slot == ImageSlot.dashboard ? 3 : 4,
                    ).bytes,
                  ),
                )
                as Map;
        expect(payload['image']['width'], w);
        expect(payload['image']['height'], h);
        for (final s in ImageSlot.values.where((s) => s != slot)) {
          expect(identical(store.state.images[s], images[s]), isTrue);
        }
        expect(identical(store.state.pending, pending), isTrue);
        expect(identical(store.state.profile, originalState.profile), isTrue);
        await tester.ensureVisible(find.byType(SegmentedButton<ImageSlot>));
        await tester.tap(
          find.text(slot == ImageSlot.dashboard ? '全画面' : '日時・カレンダー'),
        );
        await tester.pumpAndSettle();
        expect(
          tester
              .widget<DropdownButtonFormField<CardDevice>>(
                find.byType(DropdownButtonFormField<CardDevice>),
              )
              .initialValue,
          CardDevice.paperMono,
        );
        await tester.tap(
          find.text(slot == ImageSlot.dashboard ? '日時・カレンダー' : '全画面'),
        );
        await tester.pumpAndSettle();
        expect(
          tester
              .widget<DropdownButtonFormField<CardDevice>>(
                find.byType(DropdownButtonFormField<CardDevice>),
              )
              .initialValue,
          CardDevice.stackchan,
        );
        // Return to PaperMono without upscaling/recompressing the smaller JPEG.
        final expected = await tester.runAsync(
          () => CardImageProcessor().prepare(
            images[slot]!.source,
            images[slot]!.source,
            slot,
            CardDevice.paperMono,
          ),
        );
        await choose(tester, store, CardDevice.paperMono);
        expect(store.state.images[slot]!.jpeg, expected!.jpeg);
        await tester.tap(find.text('名刺').last);
        await tester.pumpAndSettle();
        expect(find.text('未保存'), findsWidgets);
        expect(tester.takeException(), isNull);
      },
    );
  }
  testWidgets('save failure restores target and original image', (
    tester,
  ) async {
    final original = fixture(ImageSlot.fullscreen);
    final store = ImageStore(
      CardWorkspace(
        profile: CardProfile.create(),
        images: {ImageSlot.fullscreen: original},
      ),
    )..fail = true;
    await tester.pumpWidget(app(store));
    await tester.pumpAndSettle();
    await tester.tap(find.text('画像').last);
    await tester.pumpAndSettle();
    await tester.tap(find.text('全画面'));
    await tester.pumpAndSettle();
    await choose(tester, store, CardDevice.stackchan);
    expect(
      identical(store.state.images[ImageSlot.fullscreen], original),
      isTrue,
    );
    expect(
      tester
          .widget<DropdownButtonFormField<CardDevice>>(
            find.byType(DropdownButtonFormField<CardDevice>),
          )
          .initialValue,
      CardDevice.paperMono,
    );
    expect(find.textContaining('画像を保存できません'), findsOneWidget);
    expect(tester.takeException(), isNull);
  });
  testWidgets('empty image changes preview aspect without writing an image', (
    tester,
  ) async {
    final store = ImageStore(CardWorkspace(profile: CardProfile.create()));
    await tester.pumpWidget(app(store));
    await tester.pumpAndSettle();
    await tester.tap(find.text('画像').last);
    await tester.pumpAndSettle();
    await tester.tap(find.text('全画面'));
    await tester.pumpAndSettle();
    await tester.tap(find.byType(DropdownButtonFormField<CardDevice>));
    await tester.pumpAndSettle();
    await tester.tap(find.text('ｽﾀｯｸﾁｬﾝ').last);
    await tester.pumpAndSettle();
    expect(
      tester.widget<AspectRatio>(find.byType(AspectRatio)).aspectRatio,
      320 / 240,
    );
    expect(store.attempts, 0);
    await tester.tap(find.text('日時・カレンダー'));
    await tester.pumpAndSettle();
    expect(tester.widget<AspectRatio>(find.byType(AspectRatio)).aspectRatio, 1);
  });
}
