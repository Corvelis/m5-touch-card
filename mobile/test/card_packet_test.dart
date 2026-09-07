import 'dart:convert';
import 'dart:io';
import 'dart:typed_data';
import 'package:flutter_test/flutter_test.dart';
import 'package:touch_card/core/card/workspace_store.dart';
import 'package:touch_card/core/protocol/card_packet.dart';

void main() {
  late Directory directory;
  late WorkspaceStore store;
  setUp(() async {
    directory = await Directory.systemTemp.createTemp('card-packet-');
    store = WorkspaceStore(directory: () async => directory);
    await store.saveProfile(
      name: 'Yuma',
      account: '',
      email: '',
      url: '',
      comment: '',
    );
  });
  tearDown(() async => directory.delete(recursive: true));

  test(
    'full, text-only and avatar-only explicitly distinguish reset and preserve',
    () async {
      final workspace = await store.load();
      final full =
          jsonDecode(utf8.decode(CardPacket.create(workspace, 1).bytes)) as Map;
      final text =
          jsonDecode(
                utf8.decode(
                  CardPacket.create(workspace, 1, textOnly: true).bytes,
                ),
              )
              as Map;
      final icon =
          jsonDecode(utf8.decode(CardPacket.create(workspace, 2).bytes)) as Map;
      expect(full.containsKey('avatar'), isTrue);
      expect(full['avatar'], isNull);
      expect(text.containsKey('avatar'), isFalse);
      expect(icon['profileId'], workspace.profile.id);
      expect(icon['revision'], workspace.profile.revision);
      expect(icon.containsKey('profile'), isFalse);
      expect(() => CardPacket.create(workspace, 3), throwsFormatException);
      expect(() => CardPacket.create(workspace, 4), throwsFormatException);
    },
  );
  test(
    'pending retains immutable content and ID across edits and restart',
    () async {
      final pending = CardPacket.create(await store.load(), 1);
      await store.savePending(pending);
      await store.saveProfile(
        name: 'Changed',
        account: '',
        email: '',
        url: '',
        comment: 'new',
      );
      await store.resetAvatar();
      final restored = await WorkspaceStore(
        directory: () async => directory,
      ).load();
      expect(restored.profile.name, 'Changed');
      expect(restored.pending!.id, pending.id);
      expect(restored.pending!.bytes, pending.bytes);
      await store.clearPending(pending.id == 1 ? 2 : 1);
      expect((await store.load()).pending, isNotNull);
      await store.clearPending(pending.id);
      expect((await store.load()).pending, isNull);
    },
  );
  test(
    'pending rejects corrupt CRC, mismatched target and invalid ID',
    () async {
      final packet = CardPacket.create(await store.load(), 1);
      final json = packet.toJson();
      json['crc32'] = packet.checksum ^ 1;
      expect(() => CardPacket.fromJson(json), throwsFormatException);
      expect(
        () => CardPacket(target: 2, bytes: packet.bytes, id: packet.id),
        throwsFormatException,
      );
      expect(
        () => CardPacket(target: 1, bytes: packet.bytes, id: 0),
        throwsFormatException,
      );
      expect(
        () => CardPacket(target: 1, bytes: Uint8List(524289), id: 1),
        throwsFormatException,
      );
    },
  );
  test(
    'shared payload vector survives Dart serialization with matching CRC',
    () {
      final vectors =
          jsonDecode(File('../protocol/test_vectors.json').readAsStringSync())
              as Map<String, dynamic>;
      Uint8List bytes(String key) {
        final hex = vectors[key] as String;
        return Uint8List.fromList([
          for (var i = 0; i < hex.length; i += 2)
            int.parse(hex.substring(i, i + 2), radix: 16),
        ]);
      }

      final packet = CardPacket(
        target: 1,
        bytes: bytes('payload'),
        id: 0x12345678,
      );
      expect(
        packet.checksum,
        ByteData.sublistView(bytes('commit')).getUint32(8, Endian.little),
      );
      expect(CardPacket.fromJson(packet.toJson()).bytes, packet.bytes);
      final data =
          jsonDecode(utf8.decode(packet.bytes)) as Map<String, dynamic>;
      expect(data['profile']['name'], 'Yuma');
    },
  );
}
