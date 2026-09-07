# Distribution review — v0.1.0

[日本語](distribution-review.ja.md) · [English](distribution-review.en.md)

Reviewed 2026-09-07 for normal-release Android APK and PaperMono/StackChan firmware distribution.
iPhone remains source-only. This is an engineering/artifact review, not a guarantee of legal compliance.

## Android

The 44 non-Flutter artifacts in the actual release package classpath were checked against fixed-version
Google Maven/Maven Central metadata and cached AAR/JAR files. They use Apache-2.0; full text, packaged NOTICE
entries and ReLinker's copyright-bearing license are added to the existing Flutter license page.
Flutter engine/Dart notices are preserved, and mime's nested HTTPD license is added.
See the [artifact inventory](../release/android-dependencies.json) and [full additional notices](../mobile/licenses/ANDROID-NATIVE-NOTICES.txt).
The revised APK passes signature and embedded-notice checks for all 44 native artifacts and mime's notice.
Installation and launch of the signed APK succeeded on the NFC-capable Xiaomi running Android 16 on September 7, 2026.
The earlier device-side installation restriction is resolved; the app and its data were not deleted.
The user has confirmed completion of release hardware checks, so those checks are no longer listed as pending.
This records user-confirmed acceptance, not new device tests performed during documentation cleanup.

## MIT publication policy and firmware

M5 Touch Card's original code remains MIT at the owner's request. Third-party fonts/libraries retain their own
terms; this does not relicense OFL/BSD/Apache/LGPL components as MIT.
The previous StackChan ELF contained /efont/ Japanese plus FreeSans24pt7b and FreeSansBold24pt7b.
The packager previously missed /efont/'s BSD-style COPYRIGHT file; collection now includes copyright
and prefixed font-license filenames. M5GFX's MIT license and its GFX wrapper's BSD license alone do not
establish the terms of the original font designs.

[Adafruit's official guide](https://learn.adafruit.com/adafruit-gfx-graphics-library/using-fonts) identifies these fonts as GNU FreeFont derivatives.
[GNU FreeFont's terms](https://www.gnu.org/software/freefont/license.html) specify GPL-3.0-or-later with a document-embedding exception.
This review does not assume that an executable firmware rendering cards qualifies as such a document.
With the owner's approval, the clock and progress digits now use Touch Digits, a 48px OFL-1.1 bitmap
subset of the existing TouchSansJP font, with regular and one-pixel-emboldened variants (~3 KiB total).
Japanese typography, card layouts and PaperMono rendering remain unchanged. The converter is included.
All four firmware builds pass the linked-font check; packaging now rejects GNU FreeFont symbols.
Host rendering checks clock width, actual glyph coverage and partial progress redraws.

Arduino-ESP32's exact package is 3.20017.241212+sha.dcc1105b, corresponding to commit
`dcc1105b0cf1322a437b354c336f2abf72b7e512`. Its LGPL-2.1-or-later terms require more than a license text:
provide the matching library source and the application source/build material needed to relink a modified library,
with equivalent access from the release location. Preserve local modifications and validate rebuilding/relinking.
An upstream link alone is not treated as completing this requirement, and no three-year written offer or
additional reverse-engineering restriction is introduced.

The matching `touch-card-v0.1.0-firmware-sources.zip` includes application firmware/assets/protocol/build
patches, Arduino core/libraries/variants/build scripts, ESP32-S3 headers/config/linker scripts, and
restoration/relinking instructions with source and SDK input hashes. SDK binaries/toolchains remain pinned
external PlatformIO downloads; this is not a fully offline toolchain bundle. Attach it beside firmware in the
same Release. See the [relinking guide](../release/corresponding-source.md).
Local verification extracted the ZIP, restored an editable framework, modified Arduino's `millis()` and
rebuilt StackChan. Disassembly confirms the modified instruction in the final ELF. This test-only change
is excluded from release sources/binaries and was never flashed to a device.

Eighteen additional runtime-notice files are checked, including newlib, the GCC runtime exception,
LittleFS/TLSF and decoder notices. A superset of 925 original source-embedded notice blocks is retained;
this does not imply that every listed component is linked. The SDK's esp_littlefs 1.14.1 header exactly
matches its fixed upstream version; its littlefs submodule BSD notice is retained. Apache-2.0 is selected
for mbedTLS and BSD for wpa_supplicant. Missing or changed notice hashes stop packaging.
Project MIT artwork and TouchSansJP's OFL text remain intact. Hardware acceptance is complete based on the user's report;
no additional device flashing or testing was performed during this documentation update.
Same-model pairs and exhaustive SD-failure/power-loss scenarios remain outside the verified scope.
The public repository/commit/tag and publication of matching binaries, notices, sources and checksums remain outstanding,
so `publish_ready` stays false. These are publication steps, not a request to repeat hardware checks.
