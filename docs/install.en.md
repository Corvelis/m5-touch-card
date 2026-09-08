# Installation

[日本語](install.ja.md) · [English](install.en.md) · [Overview](https://github.com/Corvelis/m5-touch-card)

Download your device's files from **v0.1.0 → Assets** in [Releases](https://github.com/Corvelis/m5-touch-card/releases).
Flash device firmware from a USB-connected PC, and install the Android app by opening the APK.
For iPhone, follow the [source-build instructions](#iphone-for-developers).

## 1. Choose the correct download

| Device | File |
| --- | --- |
| M5 PaperMono C153 | `touch-card-v0.1.0-paper-mono.zip` |
| Official StackChan K151 / K151-R | `touch-card-v0.1.0-stackchan.zip` |
| NFC-capable Android phone | `touch-card-v0.1.0-android.apk` (release-signed) |
| Download verification | `SHA256SUMS` |

These packages are not for PaperMono Lite, DIY StackChans or other M5 cores. Sharing an ESP32-S3 chip does not make boards interchangeable.
Each firmware ZIP includes `ja/` (Japanese default) and `en/` (English default). Both support changing language in Settings;
an existing saved language overrides the build default. GitHub's **Source code** archives are for developers, not directly flashable images.

## 2. Before flashing

- Use a Windows/macOS/Linux PC, Python 3.9+ and a USB data cable.
- Connect only the intended device and close serial monitors and other flashing programs.
- The current firmware will be replaced. Data from other products, including the old image-transfer firmware, is not automatically migrated.
- Extract the ZIP and open a terminal in the extracted directory.
- `flash.py` always backs up the complete internal flash before writing. Copy microSD files separately; they are not included in this backup.
  Keep `private-backups/` private: it may contain personal cards and settings. Never attach it to GitHub, issues or social posts.
- The helper never requests a full flash erase or overrides chip security protections. Do not format storage as the first troubleshooting step.

Compare the ZIP/APK's SHA-256 with the release's `SHA256SUMS`.
Use `Get-FileHash FILENAME -Algorithm SHA256` on Windows, `shasum -a 256 FILENAME` on macOS, or `sha256sum FILENAME` on Linux.
The helper also verifies every image against the package manifest before writing.

### Install the flashing tool

Windows PowerShell:

```powershell
py -3 -m venv .venv
.\.venv\Scripts\python.exe -m pip install esptool==4.9.0
.\.venv\Scripts\python.exe -m serial.tools.list_ports
```

macOS / Linux:

```sh
python3 -m venv .venv
.venv/bin/python -m pip install esptool==4.9.0
.venv/bin/python -m serial.tools.list_ports
```

In the following commands, replace `python` with `.\.venv\Scripts\python.exe` on Windows or `.venv/bin/python` on macOS/Linux.
Replace `PORT` with the port you identified (for example `COM5`, `/dev/cu.usbmodem...` or `/dev/ttyACM0`).
Check which port appears/disappears when reconnecting your device; do not select another device's port.

## 3. First-time installation

Validate the package and display the plan without accessing any device:

```sh
python flash.py --mode install --language ja
```

After checking the model, language and files, write it:

```sh
python flash.py --mode install --language ja --port PORT --execute
```

Type `paper-mono` or `stackchan` when prompted to confirm the physical model. The helper backs up internal flash, writes, verifies and resets.
Backups may take several minutes; do not unplug during the operation. Select `--language en` for an English initial language.
Chip detection cannot distinguish these two boards, so the physical model confirmation matters.

First-time installation writes the bootloader, partition table, boot selection data and application.
It does not erase all internal storage, but changing the old partition layout can make previous data unreadable.
Keep the backup. Restoring another product requires the appropriate recovery process for that product.

## 4. Updating an existing M5 Touch Card installation

Use this only on the same board already running M5 Touch Card, when the release notes identify it as an eligible update.

```sh
python flash.py --mode update --language ja
python flash.py --mode update --language ja --port PORT --execute
```

The helper requires the backed-up partition table to exactly match the package, then updates the app and boot selection data only.
It does not write the bootloader, partition table, NVS settings or card/image filesystem. Internal and SD cards are not automatically deleted.
On a partition mismatch, nothing is written. Do not simply switch to `install`: review the backup and release notes first.
Compatibility with earlier development builds/other products and recovery from power loss are not guaranteed.

## 5. Install or update Android

1. Download the release APK on an NFC-capable Android 7.0+ phone and enable NFC.
2. Open the APK. If prompted, allow **Install unknown apps / Allow from this source** for the browser or file manager you used, then install.
3. Disable that source's install permission again if no longer needed. Do not disable Play Protect or other security protections.
4. Open Touch Card. Its globe menu selects Japanese or English.

Update in place with a newer APK from the same publisher and signing key. An existing development build signed with a different key may block an update.
Do not immediately uninstall or clear app data: doing so removes drafts, originals, pending transfers and the profile ID, potentially creating a new card identity.
There is currently no in-app backup/restore UI. Screens and restrictions vary by OS/device management policy; see [Android's official guide](https://support.google.com/android/answer/9457058?hl=en).

## 6. Set up your first card and clock

1. On the device, open **Menu → Settings → Phone update**.
2. In the phone app's **Card** tab, save your name and select **Update card via NFC**.
3. Align NFC antennas and wait for confirmed storage. No per-field receiving mode is needed on the device.
4. Use the phone app's top clock button to set the time while the device is in the same update mode.
5. In **Images**, select the purpose and destination device, crop, and send the photo by NFC.

If initial internal storage fails, check connections, media and backups before following the [operation guide](operation.en.md).
**Erase and initialize** deletes internal cards/images; it is not a routine update step. On PaperMono, press B to turn the light on before using touch controls.

## 7. Troubleshooting

- No port: check the USB data cable, connection and device power.
- Connection failure: close serial monitors, disconnect other boards and follow the manufacturer's download-mode procedure.
  [PaperMono](https://docs.m5stack.com/en/core/PaperMono) / [StackChan](https://docs.m5stack.com/en/StackChan)
- PaperMono with the light off: press B to wake it before flashing. USB may not respond during light sleep.
- Interrupted transfer: retry with `--baud 115200`. Failed backups stop before any write.
- Hash mismatch: do not flash; download the release again.
- No boot: check the board/package match, power and write/verification result. Keep the backup when asking for help.
- No NFC response: open **Phone update** on the device; check NFC settings, antenna alignment and the phone case.

See [esptool 4.x documentation](https://docs.espressif.com/projects/esptool/en/release-v4/esp32/esptool/basic-commands.html) for command background.
Do not copy generic ESP32 example offsets: this product uses the ESP32-S3 addresses encoded in its package.

## iPhone: for developers

Source builds require macOS/Xcode, Flutter, CocoaPods and signing/provisioning that supports NFC. No IPA or TestFlight distribution is available.

1. Get the same release source and run `flutter pub get --enforce-lockfile` in `mobile/`.
2. Run `pod install --deployment` in `mobile/ios/` to install the locked dependencies (tested with CocoaPods 1.16.2).
3. Open `mobile/ios/Runner.xcworkspace` in Xcode. In Runner's Signing & Capabilities, select your Team and an available Bundle Identifier.
4. Check NFC Tag Reading entitlements/provisioning, select a connected NFC-capable iPhone and build/run.
5. Do not commit signing settings, certificates or device identifiers. The simulator cannot test NFC communication.
