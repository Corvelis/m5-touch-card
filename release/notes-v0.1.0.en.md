# M5 Touch Card v0.1.0

[日本語](notes-v0.1.0.ja.md) · [English](notes-v0.1.0.en.md)

Release-note draft for a normal release. Add the final commit/tag and actual download/corresponding-source links
before publication. Creating this file does not publish a release.

Offline NFC business cards for PaperMono and StackChan, with an Android editor.

[Watch the exchange (about 25 seconds)](../docs/media/01-card-exchange.mp4) · [Four hardware demos](../README.en.md#demos)

## Features

- Exchange both cards or send/receive one way. Completion displays the other person's card.
- Round icons, optional contacts/QR link/note, three designs, and portrait/landscape cards on PaperMono.
- Browse/sort/delete received cards; internal/microSD storage with usage and free-space information.
- Independently update the card icon, clock/calendar photo and full-screen photo by NFC.
- Japanese/English UI on devices and phone.

## Install

Follow the [installation guide](../docs/install.en.md) and select your board's ZIP and the Android APK from Assets.
Each ZIP includes Japanese/English initial-language variants. iPhone is source-build only; no IPA or TestFlight distribution is included.
Data from the older image-transfer product is not automatically migrated. Keep the pre-install backup.

## Compatibility and verification

- Supported: PaperMono C153, official StackChan K151 / K151-R and NFC-capable Android phones.
- PaperMono ↔ StackChan exchange has been tested on hardware.
- Same-model pairs and every SD-failure/power-loss recovery scenario remain unverified.
- Signed Android APK installation and launch are verified. Release hardware acceptance is complete based on the user's confirmation.
- The [readiness record](../docs/publication-readiness.md) distinguishes automated verification from user-confirmed hardware acceptance.
- Cards are plaintext; NFC does not provide cryptographic peer authentication or encryption.

Report the model, version, steps and error number. Never attach real cards, personal photos or flash backups.
