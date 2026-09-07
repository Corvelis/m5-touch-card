import 'dart:convert';
import 'dart:math';
import 'dart:typed_data';
import '../card/workspace_store.dart';
import 'checksums.dart';

class CardPacket {
  CardPacket({required this.target, required Uint8List bytes, required this.id})
    : bytes = Uint8List.fromList(bytes),
      checksum = crc32IsoHdlc(bytes) {
    if (target < 1 ||
        target > 4 ||
        id < 1 ||
        id > 0xffffffff ||
        bytes.isEmpty ||
        bytes.length > 524288) {
      throw const FormatException('送信データの上限または用途が不正です');
    }
    final data = jsonDecode(utf8.decode(bytes)) as Map<String, dynamic>;
    if (data['target'] != target) throw const FormatException('送信の用途が一致しません');
  }
  final int target, id, checksum;
  final Uint8List bytes;
  String get label => ['', '名刺', '名刺アイコン', '日時・カレンダー用画像', '全画面画像'][target];
  Map<String, Object> toJson() => {
    'target': target,
    'id': id,
    'bytes': base64Encode(bytes),
    'crc32': checksum,
  };
  factory CardPacket.fromJson(Map<String, dynamic> value) {
    final packet = CardPacket(
      target: value['target'] as int,
      id: value['id'] as int,
      bytes: base64Decode(value['bytes'] as String),
    );
    if (packet.checksum != value['crc32']) {
      throw const FormatException('送信待ちデータが破損しています');
    }
    return packet;
  }
  static Map<String, Object>? image(SlotImage? value) => value == null
      ? null
      : {
          'jpeg': base64Encode(value.jpeg),
          'width': value.width,
          'height': value.height,
        };
  static CardPacket create(
    CardWorkspace workspace,
    int target, {
    bool textOnly = false,
  }) {
    final payload = <String, Object?>{'target': target};
    if (target == 1 || target == 2) workspace.profile.validate();
    if (target == 1) {
      payload['profile'] = workspace.profile.toJson();
      if (!textOnly) {
        payload['avatar'] = image(workspace.images[ImageSlot.avatar]);
        _addExchange(payload, workspace.images[ImageSlot.avatar]);
      }
    } else if (target == 2) {
      payload['profileId'] = workspace.profile.id;
      payload['revision'] = workspace.profile.revision;
      payload['avatar'] = image(workspace.images[ImageSlot.avatar]);
      _addExchange(payload, workspace.images[ImageSlot.avatar]);
    } else if (target == 3 || target == 4) {
      final selected = workspace
          .images[target == 3 ? ImageSlot.dashboard : ImageSlot.fullscreen];
      if (selected == null) throw const FormatException('先に画像を選んで保存してください');
      payload['image'] = image(selected);
    }
    final random = Random.secure();
    var id = (random.nextInt(65536) << 16) | random.nextInt(65536);
    if (id == 0) id = 1;
    return CardPacket(
      target: target,
      bytes: Uint8List.fromList(utf8.encode(jsonEncode(payload))),
      id: id,
    );
  }

  static void _addExchange(Map<String, Object?> payload, SlotImage? avatar) {
    if (avatar?.exchangeJpeg == null) return;
    avatar!.validate();
    final info = avatar.exchangeInfo;
    payload['exchangeAvatar'] = {
      'jpeg': base64Encode(avatar.exchangeJpeg!),
      'width': info.width,
      'height': info.height,
    };
  }
}
