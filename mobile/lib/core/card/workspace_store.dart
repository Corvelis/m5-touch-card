import 'dart:convert';
import 'dart:io';
import 'dart:typed_data';
import 'package:path_provider/path_provider.dart';
import '../protocol/checksums.dart';
import '../image/jpeg_inspector.dart';
import 'card_profile.dart';
import '../protocol/card_packet.dart';

enum ImageSlot { avatar, dashboard, fullscreen }

enum CardDevice { paperMono, stackchan }

class SlotImage {
  const SlotImage({
    required this.source,
    required this.jpeg,
    required this.width,
    required this.height,
    required this.device,
    this.exchangeJpeg,
  });
  final Uint8List source, jpeg;
  // Optional transfer derivative. Original/source and device-display JPEG stay intact.
  final Uint8List? exchangeJpeg;
  final int width, height;
  final CardDevice device;
  JpegInfo get exchangeInfo => JpegInspector.inspect(exchangeJpeg ?? jpeg);
  void validate() {
    if (source.isEmpty ||
        source.length > 16 * 1024 * 1024 ||
        jpeg.length > 262144) {
      throw const FormatException('画像のデータ量が上限を超えています');
    }
    final info = JpegInspector.inspect(jpeg);
    if (!info.isPaperMonoV1Compatible ||
        width != info.width ||
        height != info.height ||
        width < 1 ||
        height < 1 ||
        width > 800 ||
        height > 800) {
      throw const FormatException('保存画像の形式が不正です');
    }
    if (exchangeJpeg != null) {
      final exchange = JpegInspector.inspect(exchangeJpeg!);
      if (exchangeJpeg!.length > 262144 ||
          !exchange.isPaperMonoV1Compatible ||
          exchange.width < 1 ||
          exchange.width > 256 ||
          exchange.height < 1 ||
          exchange.height > 256 ||
          width > 256 ||
          height > 256) {
        throw const FormatException('交換用アイコンの形式が不正です');
      }
    }
  }

  Map<String, Object> toJson() => {
    'source': base64Encode(source),
    'jpeg': base64Encode(jpeg),
    'width': width,
    'height': height,
    'device': device.name,
    if (exchangeJpeg != null) 'exchangeJpeg': base64Encode(exchangeJpeg!),
  };
  factory SlotImage.fromJson(Map<String, dynamic> json) => SlotImage(
    source: base64Decode(json['source'] as String),
    jpeg: base64Decode(json['jpeg'] as String),
    width: json['width'] as int,
    height: json['height'] as int,
    device: CardDevice.values.byName(json['device'] as String),
    exchangeJpeg: json['exchangeJpeg'] == null
        ? null
        : base64Decode(json['exchangeJpeg'] as String),
  )..validate();
}

class CardWorkspace {
  CardWorkspace({
    required this.profile,
    Map<ImageSlot, SlotImage> images = const {},
    this.pending,
  }) : images = Map.unmodifiable(images);
  final CardProfile profile;
  final Map<ImageSlot, SlotImage> images;
  final CardPacket? pending;
  Map<String, Object> toJson() => {
    'profile': profile.toJson(),
    'images': images.map((key, value) => MapEntry(key.name, value.toJson())),
    if (pending != null) 'pending': pending!.toJson(),
  };
  factory CardWorkspace.fromJson(Map<String, dynamic> json) => CardWorkspace(
    pending: json['pending'] == null
        ? null
        : CardPacket.fromJson(json['pending'] as Map<String, dynamic>),
    profile: CardProfile.fromJson(json['profile'] as Map<String, dynamic>),
    images: (json['images'] as Map<String, dynamic>).map(
      (key, value) => MapEntry(
        ImageSlot.values.byName(key),
        SlotImage.fromJson(value as Map<String, dynamic>),
      ),
    ),
  );
}

// Two checked snapshots preserve the previous valid workspace on interrupted writes.
// Temporary files are never used as committed data. All mutations are serialized.
class WorkspaceStore {
  WorkspaceStore({Future<Directory> Function()? directory})
    : _directory = directory ?? _defaultDirectory;
  final Future<Directory> Function() _directory;
  Future<void> _tail = Future.value();
  static Future<Directory> _defaultDirectory() async => Directory(
    '${(await getApplicationSupportDirectory()).path}/touch_card/workspace',
  );

  Future<(int, CardWorkspace)> _read(Directory dir) async {
    (int, CardWorkspace)? best;
    var found = false;
    for (var slot = 0; slot < 2; slot++) {
      final file = File('${dir.path}/workspace.$slot.json');
      if (!await file.exists()) continue;
      found = true;
      try {
        if (await file.length() > 80 * 1024 * 1024) continue;
        final envelope =
            jsonDecode(await file.readAsString()) as Map<String, dynamic>;
        if (envelope['version'] != 1) continue;
        final payload = envelope['payload'] as String;
        if (crc32IsoHdlc(Uint8List.fromList(utf8.encode(payload))) !=
            envelope['crc32']) {
          continue;
        }
        final data = jsonDecode(payload) as Map<String, dynamic>;
        final generation = data['generation'] as int;
        if (generation < 0 || generation > 0x1fffffffffffff) continue;
        final workspace = CardWorkspace.fromJson(
          data['workspace'] as Map<String, dynamic>,
        );
        if (best == null || generation > best.$1) {
          best = (generation, workspace);
        }
      } on FormatException {
        continue;
      } on TypeError {
        continue;
      } on ArgumentError {
        continue;
      }
    }
    if (best != null) return best;
    if (found) throw const FormatException('保存データを読み込めません。上書きせず保護しています。');
    return (0, CardWorkspace(profile: CardProfile.create()));
  }

  Future<T> _serial<T>(Future<T> Function() action) {
    final result = _tail.then((_) => action());
    _tail = result.then<void>((_) {}, onError: (Object _, StackTrace __) {});
    return result;
  }

  Future<CardWorkspace> load() =>
      _serial(() async => (await _read(await _directory())).$2);

  Future<CardWorkspace> update(CardWorkspace Function(CardWorkspace) edit) =>
      _serial(() async {
        final dir = await _directory();
        await dir.create(recursive: true);
        final previous = await _read(dir);
        final workspace = edit(previous.$2);
        workspace.profile.validate(draft: true);
        for (final image in workspace.images.values) {
          image.validate();
        }
        final generation = previous.$1 + 1;
        if (generation > 0x1fffffffffffff) {
          throw const FormatException('保存世代の上限です');
        }
        final payload = jsonEncode({
          'generation': generation,
          'workspace': workspace.toJson(),
        });
        final envelope = jsonEncode({
          'version': 1,
          'payload': payload,
          'crc32': crc32IsoHdlc(Uint8List.fromList(utf8.encode(payload))),
        });
        final temporary = File('${dir.path}/workspace.pending');
        await temporary.writeAsString(envelope, flush: true);
        await temporary.rename('${dir.path}/workspace.${generation % 2}.json');
        return workspace;
      });

  Future<CardWorkspace> saveProfile({
    required String name,
    required String account,
    required String email,
    required String url,
    required String comment,
  }) => update(
    (current) => CardWorkspace(
      profile: current.profile.revise(
        name: name,
        account: account,
        email: email,
        url: url,
        comment: comment,
      ),
      images: current.images,
      pending: current.pending,
    ),
  );
  Future<CardWorkspace> saveImage(ImageSlot slot, SlotImage image) => update(
    (current) => CardWorkspace(
      profile: slot == ImageSlot.avatar
          ? current.profile.bumpRevision()
          : current.profile,
      images: {...current.images, slot: image},
      pending: current.pending,
    ),
  );
  Future<CardWorkspace> savePending(CardPacket packet) => update(
    (current) => CardWorkspace(
      profile: current.profile,
      images: current.images,
      pending: packet,
    ),
  );
  Future<CardWorkspace> clearPending(int id) => update(
    (current) => CardWorkspace(
      profile: current.profile,
      images: current.images,
      pending: current.pending?.id == id ? null : current.pending,
    ),
  );
  Future<CardWorkspace> resetAvatar() => update(
    (current) => CardWorkspace(
      profile: current.profile.bumpRevision(),
      images: {...current.images}..remove(ImageSlot.avatar),
      pending: current.pending,
    ),
  );
}
