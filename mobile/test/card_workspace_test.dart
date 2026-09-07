import 'dart:io';
import 'dart:typed_data';
import 'package:flutter_test/flutter_test.dart';
import 'package:image/image.dart' as img;
import 'package:touch_card/core/card/card_profile.dart';
import 'package:touch_card/core/card/workspace_store.dart';
import 'package:touch_card/core/image/card_image_processor.dart';

void main() {
  late Directory directory;
  late WorkspaceStore store;
  setUp(() async {
    directory = await Directory.systemTemp.createTemp('touch-card-test-');
    store = WorkspaceStore(directory: () async => directory);
  });
  tearDown(() async => directory.delete(recursive: true));
  Future<CardWorkspace> save(String name, {String comment = ''}) =>
      store.saveProfile(
        name: name,
        account: '',
        email: '',
        url: '',
        comment: comment,
      );
  SlotImage picture(int red) {
    final source = img.Image(width: 8, height: 8);
    img.fill(source, color: img.ColorRgb8(red, 20, 30));
    final data = img.encodeJpg(source);
    return SlotImage(
      source: data,
      jpeg: data,
      width: 8,
      height: 8,
      device: CardDevice.paperMono,
    );
  }

  test(
    'name only persists; revision and stable ID survive a new store',
    () async {
      final first = await save('  ゆうま  ');
      final second = await save('ゆうま', comment: 'よろしく');
      final restored = await WorkspaceStore(
        directory: () async => directory,
      ).load();
      expect(restored.profile.id, first.profile.id);
      expect(second.profile.revision, first.profile.revision + 1);
      expect(restored.profile.name, 'ゆうま');
      expect(restored.profile.comment, 'よろしく');
      expect((await save('ゆうま')).profile.comment, isEmpty);
    },
  );
  test(
    'invalid input leaves previous profile intact and queue usable',
    () async {
      await save('old');
      await expectLater(save(' '), throwsFormatException);
      await expectLater(save('new', comment: 'あ' * 41), throwsFormatException);
      expect((await store.load()).profile.name, 'old');
      expect(
        (await save('new', comment: '😀' * 40)).profile.comment.runes.length,
        40,
      );
    },
  );
  test(
    'QR URL survives reopening and can be cleared without losing images',
    () async {
      await store.saveProfile(
        name: 'owner',
        account: '@test',
        email: '',
        url: 'https://example.com/c/existing-card',
        comment: 'hello',
      );
      await store.saveImage(ImageSlot.avatar, picture(120));
      final reopened = WorkspaceStore(directory: () async => directory);
      final before = await reopened.load();
      expect(before.profile.url, 'https://example.com/c/existing-card');
      final after = await reopened.saveProfile(
        name: before.profile.name,
        account: before.profile.account,
        email: before.profile.email,
        url: '',
        comment: before.profile.comment,
      );
      expect(after.profile.id, before.profile.id);
      expect(after.profile.url, isEmpty);
      expect(after.profile.comment, 'hello');
      expect(
        after.images[ImageSlot.avatar]!.jpeg,
        before.images[ImageSlot.avatar]!.jpeg,
      );
      expect((await reopened.load()).profile.url, isEmpty);
    },
  );
  test('field limits are independent when two fields have identical text', () {
    final profile = CardProfile.create();
    expect(
      () => profile.revise(
        name: 'a' * 81,
        account: '',
        email: 'a' * 81,
        url: '',
        comment: '',
      ),
      throwsFormatException,
    );
    expect(
      () => profile.revise(
        name: 'valid',
        account: '',
        email: '',
        url: 'javascript:alert(1)',
        comment: '',
      ),
      throwsFormatException,
    );
  });
  test(
    'three slots survive restart and updating one does not mutate others',
    () async {
      await save('owner');
      for (final slot in ImageSlot.values) {
        await store.saveImage(slot, picture(40 + slot.index * 40));
      }
      final before = await store.load();
      await store.saveImage(ImageSlot.fullscreen, picture(220));
      final after = await WorkspaceStore(
        directory: () async => directory,
      ).load();
      expect(after.images.length, 3);
      expect(
        after.images[ImageSlot.avatar]!.jpeg,
        before.images[ImageSlot.avatar]!.jpeg,
      );
      expect(
        after.images[ImageSlot.dashboard]!.jpeg,
        before.images[ImageSlot.dashboard]!.jpeg,
      );
      expect(
        after.images[ImageSlot.fullscreen]!.jpeg,
        isNot(before.images[ImageSlot.fullscreen]!.jpeg),
      );
      expect(after.profile.revision, before.profile.revision);
      expect(
        (await store.saveImage(
          ImageSlot.avatar,
          picture(240),
        )).profile.revision,
        before.profile.revision + 1,
      );
    },
  );
  test(
    'concurrent image updates merge against latest committed workspace',
    () async {
      await save('owner');
      await Future.wait(
        ImageSlot.values.map((slot) => store.saveImage(slot, picture(100))),
      );
      expect((await store.load()).images.length, 3);
    },
  );
  test(
    'interrupted latest snapshot recovers previous, pending file ignored',
    () async {
      await save('first'); // generation 1
      await save('second'); // generation 2
      await File('${directory.path}/workspace.0.json').writeAsString('{broken');
      await File(
        '${directory.path}/workspace.pending',
      ).writeAsString('{pending');
      expect((await store.load()).profile.name, 'first');
      expect((await save('third')).profile.name, 'third');
    },
  );
  test(
    'both corrupt snapshots are protected, never reset or overwritten',
    () async {
      await save('first');
      await File('${directory.path}/workspace.1.json').writeAsString('broken');
      await expectLater(store.load(), throwsFormatException);
      await expectLater(save('replacement'), throwsFormatException);
      expect(
        await File('${directory.path}/workspace.1.json').readAsString(),
        'broken',
      );
    },
  );
  test(
    'failed snapshot publication retains old workspace and allows retry',
    () async {
      await save('first');
      final obstruction = Directory('${directory.path}/workspace.pending');
      await obstruction.create();
      await expectLater(save('second'), throwsA(isA<FileSystemException>()));
      expect((await store.load()).profile.name, 'first');
      await obstruction.delete();
      expect((await save('third')).profile.name, 'third');
    },
  );
  test(
    'processor preserves colour, dimensions and aspect ratio for all slots',
    () async {
      final pixels = img.Image(width: 100, height: 50);
      img.fill(pixels, color: img.ColorRgb8(240, 30, 10));
      final source = Uint8List.fromList(img.encodePng(pixels));
      for (final device in CardDevice.values) {
        for (final slot in ImageSlot.values) {
          final result = await CardImageProcessor().prepare(
            source,
            source,
            slot,
            device,
          );
          final (width, height) = CardImageProcessor.dimensions(slot, device);
          expect((result.width, result.height), (width, height));
          expect(result.source, source);
          final pixel = img.decodeJpg(result.jpeg)!.getPixel(0, 0);
          expect(pixel.r, greaterThan(pixel.g * 2));
          expect(result.jpeg.length, lessThanOrEqualTo(262144));
        }
      }
    },
  );
}
