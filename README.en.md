# M5 Touch Card

[日本語](README.md) · [English](README.en.md)

**Tap to exchange business cards.**

Offline NFC business cards for M5 PaperMono C153 and official StackChan K151 / K151-R,
with an Android/iPhone editor.

![Business cards on PaperMono and StackChan](docs/media/devices.jpg)

## Demos

### Exchange cards · About 25 seconds

Bring the devices together to exchange cards. Each screen then shows the card it received.

<!-- BEGIN VIDEO: card-exchange -->

https://github.com/user-attachments/assets/cd2798ff-c891-4521-8a90-304021eb681b

<!-- END VIDEO: card-exchange -->

### Update from your phone · About 31 seconds

<!-- BEGIN VIDEO: phone-update -->

https://github.com/user-attachments/assets/b0046ea4-0d4f-4b6d-bd87-edb3436d9dc6

<!-- END VIDEO: phone-update -->

### Choose a design · About 31 seconds

<!-- BEGIN VIDEO: card-designs -->

https://github.com/user-attachments/assets/99213d5d-7417-4696-aaa6-da09b605a35e

<!-- END VIDEO: card-designs -->

### Browse received cards · About 17 seconds

<!-- BEGIN VIDEO: card-book -->

https://github.com/user-attachments/assets/7bbaaae1-e9f9-4768-aa49-6665adaf12c1

<!-- END VIDEO: card-book -->

All four videos show real hardware, uncut and at original speed.
The phone demo uses an iPhone development build. iOS is source-build only for the initial release.

## Installation

[Installation guide](docs/install.en.md) · [日本語](docs/install.ja.md)

The planned first publication is a normal `v0.1.0` release with model-specific firmware ZIPs and a release-signed Android APK.
The guide covers first installation, updates and initial card setup. Downloads will be available in GitHub Releases after publication.
iPhone is source-build only initially; no IPA or TestFlight distribution. Release candidates are prepared and checked; GitHub publication is pending.

## Features

- A name is the only required field. Add a round icon, account, email, QR URL and short note if wanted.
- Typography, Contrast and Simple designs. PaperMono supports portrait and landscape; StackChan has a compact landscape layout.
- Exchange both cards automatically, or send/receive one way. The received card appears on screen and is saved in Cards.
- Independent card icon, clock/calendar photo and full-screen photo. A 160px exchange icon keeps the original and on-device image intact.
- Browse received cards by name or latest receipt, view full details/QR and confirm before deletion.
- Internal storage works without an SD card. Optional microSD support, capacity/usage display and SD-first or internal-only storage.
- Japanese/English settings, persistent language choice and selectable build defaults.
- PaperMono accidental power-button reset/shutdown protection; B toggles the light.
  Touch/A are locked while dark and bottom controls are hidden. Waking with B restores only the controls using a partial update.
  Time/battery normally update regionally each minute while dark, with one full-frame cleaning refresh at each local :00 (deferred during NFC). Unset clocks and full-screen photos are excluded from hourly cleaning. Date/calendar changes catch up after waking.
  While dark and idle, PaperMono enters light sleep until the next minute update or B press; NFC, saving, drawing and catalog scans defer sleep.
  StackChan power-button short press toggles the screen only; NFC and storage keep running while dark.

PaperMono ↔ StackChan exchange has been tested on hardware. The user has confirmed completion of release hardware checks;
no additional device tests were run during documentation cleanup. Same-model pairs and exhaustive SD-failure/power-loss scenarios remain outside the verified scope.
See the [verification record](docs/publication-readiness.md) for the distinction between automated checks and user-confirmed hardware acceptance.

## Quick start

1. On the device, open **Settings → Phone update** (Japanese: 設定 → スマホから更新).
2. In the phone app, save your name and optional details, then choose **Update card via NFC**.
3. For two-way exchange, both devices open **Card exchange → Exchange**. Choose **Send first** on one and **Receive first** on the other. Keep them together until both transfers finish.
4. Device language: **Settings → More → 言語 / Language**. Phone language: the globe menu at the top.
5. Power off/restart: **Settings → More → Power**, then confirm.

See the [operation guide](docs/operation.en.md), [Japanese detailed guide](docs/operation.ja.md),
and [publication checklist](docs/publishing.md).

## Build and test

Tested with PlatformIO Core 6.1.19, Flutter 3.32.8 / Dart 3.8.1, CMake and Node.js 23.10.0.
Dependencies are recorded in the PlatformIO config and lockfiles.
Android builds need JDK 17, Android SDK 36 and NDK 27.0.12077973. iOS builds require macOS/Xcode.
Use the tested SDK versions first; dependency upgrades are a separate change from publication cleanup.

```sh
# Japanese default
pio run -d firmware -e paper-mono -e stackchan
# English default
pio run -d firmware -e paper-mono-en -e stackchan-en

cmake -S . -B build/native-tests
cmake --build build/native-tests
ctest --test-dir build/native-tests --output-on-failure
node scripts/check_native_protocol.mjs
```

Native protocol checks additionally require Kotlin/JDK and Swift. C++ checks target Clang/GCC on macOS/Linux. CMake uses ArduinoJson installed by the `paper-mono` build;
override `ARDUINOJSON_DIR` if necessary. Custom firmware environments can define `TOUCH_CARD_DEFAULT_LANGUAGE=0` for Japanese or `1` for English.
**Saved language settings override build defaults**, including after reflashing.

In `mobile/`:

```sh
flutter pub get --enforce-lockfile
flutter analyze
flutter test
flutter build apk --debug
flutter build ios --simulator --no-codesign
```

For an English initial app language, add `--dart-define=TOUCH_CARD_DEFAULT_LANGUAGE=en` to the build command.
Omitting it defaults to Japanese. The globe menu offers **日本語 / English**. The former phone-language mode migrates to Japanese. iOS device installation needs your own signing team and an NFC-capable iPhone.

Builds do not flash or install anything. Back up and positively identify devices before flashing, changing partitions or formatting.
Both endpoints must use M5 Touch Card v2; legacy v1 image-transfer firmware is incompatible.
The included CI checks builds/tests only; it does not deploy, publish releases or upload device backups.

## Privacy and licenses

Cards are stored as local plaintext. NFC is not encrypted or cryptographically authenticated; mutual-exchange pairing prevents accidental session mixing, not impersonation.
SD management is limited to `/tc`. Keep separate backups; software cannot guarantee survival of media faults or power loss.
There is no Web publishing feature. QR codes can still contain any existing URL you choose.
Previously saved URLs are preserved. To remove one, clear Optional details → QR link, save, and update the device via NFC.

See [LICENSE](LICENSE), [third-party notices](THIRD_PARTY_NOTICES.md), [migration sources](migration/README.md)
and the [publication checklist](docs/publishing.md). Review dependency redistribution requirements before distributing binaries.

## Repository map and contributing

M5 Touch Card's original code is released under the [MIT license](LICENSE).
Third-party components retain their original terms, including OFL/BSD fonts and the LGPL Arduino runtime;
they are not relicensed as MIT. Preserve the bundled notices and distribute the matching source/relinking ZIP
alongside firmware binaries. See the [distribution review](docs/distribution-review.en.md) and
[source/relinking guide](release/corresponding-source.md).

`firmware/` contains device code, `mobile/` the phone app, and `protocol/` the current wire specification and shared vectors.
`migration/legacy/` retains reference code and regression fixtures; it is not the current app entry point.
Generated builds, device backups, signing credentials and real cards are excluded from source publication.
See [contributing and bug reports](CONTRIBUTING.md) and [security/privacy](SECURITY.md).
