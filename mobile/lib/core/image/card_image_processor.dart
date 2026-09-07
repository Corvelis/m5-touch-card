import 'dart:isolate';
import 'dart:typed_data';
import 'package:image/image.dart' as img;
import '../card/workspace_store.dart';

// Adapted from ImageProcessor: retain baseline JPEG validation/compression,
// but keep exchange icons in colour and never stretch a different aspect ratio.
class CardImageProcessor {
  static const exchangeSide = 160;
  static const exchangeBudgetBytes = 6 * 1024;
  // Only the exchange derivative is resized. Never go below quality 55.
  // Complex photographs may exceed the budget; the preview reports that honestly.
  static Uint8List _exchange(img.Image rgb, Uint8List original) {
    if (rgb.width == exchangeSide &&
        rgb.height == exchangeSide &&
        original.length <= exchangeBudgetBytes) {
      return original;
    }
    final side = rgb.width < rgb.height ? rgb.width : rgb.height;
    final square = img.copyCrop(
      rgb,
      x: (rgb.width - side) ~/ 2,
      y: (rgb.height - side) ~/ 2,
      width: side,
      height: side,
    );
    final round = img.copyResize(
      square,
      width: exchangeSide,
      height: exchangeSide,
      interpolation: img.Interpolation.cubic,
    );
    final cx = (round.width - 1) / 2, cy = (round.height - 1) / 2;
    final radius = round.width / 2;
    for (final p in round) {
      final dx = p.x - cx, dy = p.y - cy;
      // Only corners outside every circular avatar crop are removed.
      if (dx * dx + dy * dy > (radius + 1) * (radius + 1)) {
        round.setPixelRgb(p.x, p.y, 255, 255, 255);
      }
    }
    Uint8List? best;
    for (final quality in [85, 80, 75, 70, 65, 60, 55]) {
      final jpeg = img.encodeJpg(
        round,
        quality: quality,
        chroma: img.JpegChroma.yuv420,
      );
      if (best == null || jpeg.length < best.length) best = jpeg;
      if (jpeg.length <= exchangeBudgetBytes) return jpeg;
    }
    return best!;
  }

  Future<SlotImage> prepareExchange(SlotImage image) => Isolate.run(() {
    image.validate();
    if (image.width > 256 || image.height > 256) {
      throw const FormatException('名刺アイコンを選んでください');
    }
    final rgb = img.decodeJpg(image.jpeg);
    if (rgb == null) throw const FormatException('画像を読み込めません');
    return SlotImage(
      source: image.source,
      jpeg: image.jpeg,
      width: image.width,
      height: image.height,
      device: image.device,
      exchangeJpeg: _exchange(rgb, image.jpeg),
    )..validate();
  });
  static (int, int) dimensions(ImageSlot slot, CardDevice device) =>
      switch (slot) {
        ImageSlot.avatar => (256, 256),
        ImageSlot.dashboard =>
          device == CardDevice.paperMono ? (386, 386) : (144, 144),
        ImageSlot.fullscreen =>
          device == CardDevice.paperMono ? (480, 800) : (320, 240),
      };
  Future<SlotImage> prepare(
    Uint8List source,
    Uint8List cropped,
    ImageSlot slot,
    CardDevice device,
  ) => Isolate.run(() {
    final (width, height) = dimensions(slot, device);
    if (source.length > 16 * 1024 * 1024) {
      throw const FormatException('元画像が大きすぎます');
    }
    final decoded = img.decodeImage(cropped);
    if (decoded == null) throw const FormatException('画像を読み込めません');
    // Cover crop provides a safe aspect-correct fallback for rounding in crop UI.
    final targetRatio = width / height;
    final cropWidth = (decoded.height * targetRatio).round().clamp(
      1,
      decoded.width,
    );
    final cropHeight = (decoded.width / targetRatio).round().clamp(
      1,
      decoded.height,
    );
    final cut = img.copyCrop(
      decoded,
      x: (decoded.width - cropWidth) ~/ 2,
      y: (decoded.height - cropHeight) ~/ 2,
      width: cropWidth,
      height: cropHeight,
    );
    final resized = img.copyResize(
      cut,
      width: width,
      height: height,
      interpolation: img.Interpolation.cubic,
    );
    final rgb = img.Image(width: width, height: height, numChannels: 3);
    for (final pixel in resized) {
      final alpha = pixel.aNormalized;
      rgb.setPixelRgb(
        pixel.x,
        pixel.y,
        (pixel.r * alpha + 255 * (1 - alpha)).round(),
        (pixel.g * alpha + 255 * (1 - alpha)).round(),
        (pixel.b * alpha + 255 * (1 - alpha)).round(),
      );
    }
    for (final quality in [80, 75, 70, 65, 60, 55, 50, 45, 40, 35]) {
      final jpeg = img.encodeJpg(
        rgb,
        quality: quality,
        chroma: img.JpegChroma.yuv444,
      );
      if (jpeg.length > 262144) continue;
      final verified = img.decodeJpg(jpeg);
      if (verified == null ||
          verified.width != width ||
          verified.height != height) {
        throw const FormatException('画像を検証できません');
      }
      return SlotImage(
        source: source,
        jpeg: jpeg,
        width: width,
        height: height,
        device: device,
        exchangeJpeg: slot == ImageSlot.avatar ? _exchange(rgb, jpeg) : null,
      )..validate();
    }
    throw const FormatException('画像を転送可能なサイズにできません');
  });
}
