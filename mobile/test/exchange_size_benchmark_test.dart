import 'dart:io';
import 'package:flutter_test/flutter_test.dart';
import 'package:image/image.dart' as img;

// Compression experiment only; does not alter saved profiles or claim NFC times.
void main() {
  test('compare exchange-only icon sizes using the bundled example', () async {
    final source = img.decodeJpg(
      await File('assets/default.jpg').readAsBytes(),
    )!;
    for (final size in [256, 192, 160, 128]) {
      final round = img.copyResize(
        source,
        width: size,
        height: size,
        interpolation: img.Interpolation.cubic,
      );
      final c = (size - 1) / 2, radius = size / 2 + 1;
      for (final p in round) {
        final dx = p.x - c, dy = p.y - c;
        if (dx * dx + dy * dy > radius * radius) {
          round.setPixelRgb(p.x, p.y, 255, 255, 255);
        }
      }
      for (final quality in [75, 65, 55]) {
        final jpeg = img.encodeJpg(
          round,
          quality: quality,
          chroma: img.JpegChroma.yuv420,
        );
        // ignore: avoid_print
        print(
          'example-only: size=$size quality=$quality jpeg_bytes=${jpeg.length}',
        );
        expect(img.decodeJpg(jpeg)!.width, size);
      }
    }
  });
}
