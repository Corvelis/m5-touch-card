# M5 Touch Card

[日本語](README.md) · [English](README.en.md)

**Tap to exchange business cards.**

Offline NFC business cards for M5 PaperMono and official StackChan.
Edit your name and icon on a phone, tap to update your device, and keep received cards in a card book.

![Business cards on PaperMono and StackChan](docs/media/devices.jpg)

## Installation

[Download from Releases](https://github.com/Corvelis/m5-touch-card/releases) · [Flashing and installation guide](docs/install.en.md)

| Device | File |
| --- | --- |
| M5 PaperMono C153 | `touch-card-v0.1.0-paper-mono.zip` |
| Official StackChan K151 / K151-R | `touch-card-v0.1.0-stackchan.zip` |
| NFC-capable Android 7.0 or later | `touch-card-v0.1.0-android.apk` |
| iPhone | [Build from source](docs/install.en.md#iphone-for-developers) |

Each firmware ZIP includes Japanese/English variants, the flashing tool and instructions for Windows, macOS and Linux.
For an existing M5 Touch Card installation, follow the guide's update procedure.
PaperMono Lite, DIY StackChans and other M5 cores are not supported.

## Features

- **Exchange cards**: choose Exchange, Send or Receive and bring the devices together. The received card appears on screen.
- **Make it yours**: only a name is required. Add a round icon, account, email, QR URL and short note. Choose from three designs, with portrait and landscape layouts on PaperMono.
- **Update from a phone**: send card details, icons, home photos and time via NFC. The card icon and two home photos are stored independently.
- **Card book**: browse by name or latest receipt, view details and QR codes, and delete individual cards.
- **Optional microSD**: works with internal storage alone. Choose a storage destination and check usage/free space. There is no fixed card-count limit or automatic deletion of old cards.
- **Choose your home**: a card, photo with clock/calendar, or full-screen photo. Select Japanese or English independently on the device and phone.
- **PaperMono power saving**: B toggles the light. While dark, controls lock and the idle CPU enters light sleep. Time/battery update partially each minute, with a full cleaning refresh at each local :00.

See the [operation guide](docs/operation.en.md) for details.

## Quick start

1. On the device, open **Menu → Settings → Phone update**.
2. In the phone app's **Card** tab, save your name/details, choose **Update card via NFC**, and tap the device.
3. Use the phone's top clock button to send the time. Keep the device on the same waiting screen.
4. On both devices, open **Card exchange → Exchange**. Choose **Send first** on one and **Receive first** on the other.
5. Keep the NFC areas together until both transfers finish. Received cards are also available in **Cards**.

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

Real hardware, uncut and at original speed. The phone demo was recorded on an iPhone.

## Compatibility and notes

- Both devices must run M5 Touch Card.
- PaperMono ↔ StackChan exchange has been tested on hardware. Same-model pairs remain untested.
- QR codes contain the URL you choose.

## Development and licenses

Original code is [MIT-licensed](LICENSE). Third-party libraries and fonts retain their own licenses.
Each firmware release includes a matching source ZIP with relinking instructions.

- [Build and test](docs/development.en.md)
- [Protocol](protocol/specification.md) / [Security](SECURITY.md)
- [Third-party notices](THIRD_PARTY_NOTICES.md) / [Redistribution](docs/distribution-review.en.md)
