# Build and test

[日本語](development.ja.md) · [English](development.en.md) · [README](../README.en.md)

To use prebuilt downloads, follow the [installation guide](install.en.md).

## Environment

- PlatformIO Core 6.1.19
- Flutter 3.32.8 / Dart 3.8.1
- CMake, Clang/GCC and Node.js
- Android: JDK 17, Android SDK 36, NDK 27.0.12077973
- iOS: macOS, Xcode, CocoaPods 1.16.2

Dependencies are pinned in `firmware/platformio.ini` and the lockfiles.
Firmware builds support Windows/macOS/Linux; C++ host tests target macOS/Linux.

## Firmware

Run from the repository root:

```sh
# Japanese default
pio run -d firmware -e paper-mono -e stackchan
# English default
pio run -d firmware -e paper-mono-en -e stackchan-en

cmake -S . -B build/native-tests
cmake --build build/native-tests
ctest --test-dir build/native-tests --output-on-failure
```

Japanese is the default. A saved language setting overrides the build default.
Custom environments can define `TOUCH_CARD_DEFAULT_LANGUAGE=0` for Japanese or `1` for English.

CMake uses ArduinoJson installed by the `paper-mono` build. For a different location, pass
`-DARDUINOJSON_DIR=/path/to/ArduinoJson/src`.

Building does not flash a device. See [packaging](../release/README.md) and [flashing instructions](install.en.md).

## Phone app

Run in `mobile/`:

```sh
flutter pub get --enforce-lockfile
flutter analyze
flutter test
flutter build apk --debug
flutter build ios --simulator --no-codesign
```

Add `--dart-define=TOUCH_CARD_DEFAULT_LANGUAGE=en` for an English initial language; otherwise it defaults to Japanese.
Saved language settings are preserved. Physical iPhones need [signing configuration](install.en.md#iphone-for-developers);
the simulator cannot use NFC. See [release packaging](../release/README.md) for Android distribution signing.

## Additional checks

```sh
node scripts/check_native_protocol.mjs
python3 -m unittest discover -s scripts/release -p 'test_*.py'
node scripts/check_public_source.mjs --self-test
node --test scripts/check_public_source.test.mjs
node scripts/check_public_source.mjs
```

Shared protocol-vector checks need Node.js, Kotlin/JDK and Swift. See the [rendering tool](../scripts/paper_preview/README.md)
for PaperMono previews. GitHub Actions checks all four firmware configurations, C++ tests, Flutter analysis/tests,
source hygiene and release scripts.

## Repository map

- `firmware/`: device rendering, NFC, storage and power control
- `mobile/`: Flutter app and Android/iOS NFC implementations
- `protocol/`: protocol specification and shared vectors
- `scripts/`: font generation, rendering checks and release tools
- `migration/legacy/`: reference code and regression fixtures, not the current app entry point

See the [relinking guide](../release/corresponding-source.md) for modifying bundled library sources.
