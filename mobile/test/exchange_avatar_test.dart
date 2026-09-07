import 'dart:convert';
import 'dart:io';
import 'dart:math';
import 'dart:typed_data';
import 'package:flutter_test/flutter_test.dart';
import 'package:image/image.dart' as img;
import 'package:touch_card/core/card/card_profile.dart';
import 'package:touch_card/core/card/workspace_store.dart';
import 'package:touch_card/core/image/card_image_processor.dart';
import 'package:touch_card/core/image/jpeg_inspector.dart';
import 'package:touch_card/core/protocol/card_packet.dart';

void main() {
  final processor = CardImageProcessor();
  Future<SlotImage> prepareDefault() async {
    final source = await File('assets/default.jpg').readAsBytes();
    return processor.prepare(
      source,
      source,
      ImageSlot.avatar,
      CardDevice.paperMono,
    );
  }

  test(
    'display stays 256px, exchange becomes 160px; originals survive',
    () async {
      final image = await prepareDefault();
      expect(image.source, await File('assets/default.jpg').readAsBytes());
      expect(image.exchangeJpeg!.length, lessThan(image.jpeg.length));
      final info = JpegInspector.inspect(image.exchangeJpeg!);
      expect((image.width, image.height), (256, 256));
      expect((info.width, info.height), (160, 160));
      expect(image.exchangeJpeg!.length, lessThanOrEqualTo(6144));
      expect(info.isPaperMonoV1Compatible, isTrue);
      final roundTrip = SlotImage.fromJson(image.toJson());
      expect(roundTrip.jpeg, image.jpeg);
      expect(roundTrip.exchangeJpeg, image.exchangeJpeg);
      // A repeatable compression measurement, NOT an NFC timing measurement.
      // ignore: avoid_print
      print(
        'default avatar: display=${image.jpeg.length} bytes, exchange=${image.exchangeJpeg!.length} bytes',
      );
      const reviewDirectory = String.fromEnvironment('EXCHANGE_REVIEW_DIR');
      if (reviewDirectory.isNotEmpty) {
        await Directory(reviewDirectory).create(recursive: true);
        await File('$reviewDirectory/display.jpg').writeAsBytes(image.jpeg);
        await File(
          '$reviewDirectory/exchange.jpg',
        ).writeAsBytes(image.exchangeJpeg!);
      }
    },
  );
  test(
    'older workspaces opt in without recropping or replacing display JPEG',
    () async {
      final image = await prepareDefault();
      final legacy = image.toJson()..remove('exchangeJpeg');
      final before = SlotImage.fromJson(legacy);
      expect(before.exchangeJpeg, isNull);
      final after = await processor.prepareExchange(before);
      expect(after.source, before.source);
      expect(after.jpeg, before.jpeg);
      expect((after.width, after.height), (before.width, before.height));
      expect(after.exchangeJpeg, isNotNull);
    },
  );
  test(
    'complex pictures keep the 160px floor even when exceeding budget',
    () async {
      final noise = img.Image(width: 256, height: 256, numChannels: 3);
      final random = Random(42);
      for (final pixel in noise) {
        noise.setPixelRgb(
          pixel.x,
          pixel.y,
          random.nextInt(256),
          random.nextInt(256),
          random.nextInt(256),
        );
      }
      final source = img.encodePng(noise);
      final image = await processor.prepare(
        source,
        source,
        ImageSlot.avatar,
        CardDevice.stackchan,
      );
      expect((image.width, image.height), (256, 256));
      expect(image.source, source);
      expect((image.exchangeInfo.width, image.exchangeInfo.height), (160, 160));
      expect(image.exchangeJpeg!.length, lessThan(image.jpeg.length));
    },
  );
  test(
    'even small 256px JPEG gets 160px derivative; other slots are unchanged',
    () async {
      final source = img.encodePng(
        img.Image(width: 8, height: 8, numChannels: 3),
      );
      final image = await processor.prepare(
        source,
        source,
        ImageSlot.avatar,
        CardDevice.paperMono,
      );
      expect((image.exchangeInfo.width, image.exchangeInfo.height), (160, 160));
      expect((image.width, image.height), (256, 256));
      for (final slot in [ImageSlot.dashboard, ImageSlot.fullscreen]) {
        final home = await processor.prepare(
          source,
          source,
          slot,
          CardDevice.stackchan,
        );
        expect(home.exchangeJpeg, isNull);
      }
    },
  );
  test(
    'NFC update carries both versions; text-only preserves both on device',
    () async {
      final avatar = await prepareDefault();
      final state = CardWorkspace(
        profile: CardProfile.create().revise(
          name: 'Yuma',
          account: '',
          email: '',
          url: '',
          comment: '',
        ),
        images: {ImageSlot.avatar: avatar},
      );
      for (final target in [1, 2]) {
        final payload =
            jsonDecode(utf8.decode(CardPacket.create(state, target).bytes))
                as Map;
        expect(base64Decode(payload['avatar']['jpeg'] as String), avatar.jpeg);
        expect(payload['avatar']['width'], 256);
        expect(payload['avatar']['height'], 256);
        expect(payload['exchangeAvatar']['width'], 160);
        expect(payload['exchangeAvatar']['height'], 160);
        expect(
          base64Decode(payload['exchangeAvatar']['jpeg'] as String),
          avatar.exchangeJpeg,
        );
      }
      final text =
          jsonDecode(
                utf8.decode(CardPacket.create(state, 1, textOnly: true).bytes),
              )
              as Map;
      expect(text.containsKey('avatar'), isFalse);
      expect(text.containsKey('exchangeAvatar'), isFalse);
    },
  );
  test(
    'saving the derivative advances revision without altering other slots or pending snapshot',
    () async {
      final dir = await Directory.systemTemp.createTemp('exchange-avatar-');
      addTearDown(() => dir.delete(recursive: true));
      final store = WorkspaceStore(directory: () async => dir);
      await store.saveProfile(
        name: 'Owner',
        account: '',
        email: '',
        url: '',
        comment: '',
      );
      final avatar = await prepareDefault();
      final before = await store.saveImage(ImageSlot.avatar, avatar);
      final pending = CardPacket.create(before, 1);
      await store.savePending(pending);
      await store.saveImage(
        ImageSlot.dashboard,
        SlotImage(
          source: avatar.source,
          jpeg: avatar.jpeg,
          width: avatar.width,
          height: avatar.height,
          device: avatar.device,
        ),
      );
      final current = await store.load();
      await store.saveImage(
        ImageSlot.avatar,
        await processor.prepareExchange(avatar),
      );
      final restored = await WorkspaceStore(directory: () async => dir).load();
      expect(restored.profile.revision, current.profile.revision + 1);
      expect(restored.images[ImageSlot.avatar]!.jpeg, avatar.jpeg);
      expect(
        restored.images[ImageSlot.dashboard]!.jpeg,
        current.images[ImageSlot.dashboard]!.jpeg,
      );
      expect(restored.pending!.bytes, pending.bytes);
      expect(restored.pending!.id, pending.id);
    },
  );
  test('oversized derivative dimensions are rejected', () {
    final original = Uint8List.fromList(
      img.encodeJpg(img.Image(width: 256, height: 256, numChannels: 3)),
    );
    final different = Uint8List.fromList(
      img.encodeJpg(img.Image(width: 257, height: 160, numChannels: 3)),
    );
    expect(
      () => SlotImage(
        source: original,
        jpeg: original,
        width: 256,
        height: 256,
        device: CardDevice.paperMono,
        exchangeJpeg: different,
      ).validate(),
      throwsFormatException,
    );
  });
  test(
    'existing 256px derivative loads and can be regenerated to 160px',
    () async {
      final image = await prepareDefault();
      final legacy = SlotImage(
        source: image.source,
        jpeg: image.jpeg,
        width: image.width,
        height: image.height,
        device: image.device,
        exchangeJpeg: image.jpeg,
      );
      final loaded = SlotImage.fromJson(legacy.toJson());
      expect(loaded.exchangeInfo.width, 256);
      final resized = await processor.prepareExchange(loaded);
      expect(
        (resized.exchangeInfo.width, resized.exchangeInfo.height),
        (160, 160),
      );
      expect(resized.source, loaded.source);
      expect(resized.jpeg, loaded.jpeg);
      expect(
        SlotImage.fromJson(resized.toJson()).exchangeJpeg,
        resized.exchangeJpeg,
      );
    },
  );
}
