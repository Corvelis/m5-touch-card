# M5 Touch Card v0.1.0

[日本語](notes-v0.1.0.ja.md) · [English](notes-v0.1.0.en.md)

Tap to exchange business cards on PaperMono and StackChan.
Edit on a phone, update via NFC, and store received cards internally or on microSD.

## Features

- Exchange both cards, or send/receive one way. The received card appears on screen.
- Round icons and three designs. Contacts, QR URL and note are optional; PaperMono supports portrait/landscape.
- Browse by name/latest receipt, view details and delete individual cards. Check internal/microSD usage and free space.
- Update card details, icons, home photos and time from a phone via NFC.
- Japanese/English UI on devices and phone.
- PaperMono enters light sleep while dark and idle. Time/battery update partially each minute, with a full cleaning refresh at each local :00.
- StackChan's short power-button press toggles the screen.

## Download and install

[Installation](../docs/install.en.md) · [Operation guide](../docs/operation.en.md) · [Hardware demos](../README.en.md#demos)

Choose from **Assets** in [Releases](https://github.com/Corvelis/m5-touch-card/releases).

| File | Contents |
| --- | --- |
| `touch-card-v0.1.0-paper-mono.zip` | M5 PaperMono C153 firmware |
| `touch-card-v0.1.0-stackchan.zip` | Official StackChan K151 / K151-R firmware |
| `touch-card-v0.1.0-android.apk` | Signed app for NFC-capable Android 7.0 or later |
| `touch-card-v0.1.0-firmware-sources.zip` | Corresponding source and relinking instructions |
| `SHA256SUMS` | Download checksums |

Firmware ZIPs include Japanese/English defaults and the flashing tool. Language can also be changed in Settings.
For existing M5 Touch Card installations, follow the guide's update procedure. Matching storage layouts allow cards, images and settings to be preserved.
iPhone supports [source builds](../docs/install.en.md#iphone-for-developers); no IPA or TestFlight distribution is available.

## Notes

- PaperMono Lite, DIY StackChans and other M5 cores are not supported. Legacy v1 image-transfer firmware is not protocol/data compatible.
- PaperMono ↔ StackChan exchange has been tested on hardware. Same-model pairs remain untested.
- Fast e-paper updates may leave ghosting. Startup and hourly cleaning refreshes make the screen flash.
- Cards are plaintext and NFC has no encryption or cryptographic peer authentication. Back up important data before flashing or initializing storage.

Original code is [MIT-licensed](../LICENSE); third-party components retain their own terms. See [notices](../THIRD_PARTY_NOTICES.md) and [corresponding source](corresponding-source.md).
Report issues with the model, version, steps and error number. Do not attach real cards or flash backups.
