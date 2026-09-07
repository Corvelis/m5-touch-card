# 対応ソースと再リンク / Corresponding source and relinking

このZIPは、同じv0.1.0の本体ZIPと一緒に配布します。M5 Touch Cardの自作部分はMIT、
Arduino部分はLGPL-2.1-or-later等、フォント等は各ファイルの元のライセンスのままです。
MITの表示で第三者の条件を置き換えません。改変や、その改変のデバッグのための逆解析を禁止しません。

## 内容

- `touch-card/`: 本体の全自作ソース、組み込み画像・フォント、通信仕様、ビルド時のNFC修正スクリプト、書き込み手順。
- `arduino/`: 実際にビルドしたArduino-ESP32パッケージのcore・libraries・variants、ビルドスクリプト、ESP32-S3のヘッダー/リンカースクリプト/設定。LGPL本文とファイル内の原文を保持。
- `source-manifest.json`: 各収録ファイル、外部SDK入力、対応する本体ソースのSHA-256。
- `restore_framework.py`: 検証済みSDKに収録ソースを合わせ、改変用の新しいコピーを作る補助。インストール済みSDKを変更しません。

Arduinoの固定版は `3.20017.241212+sha.dcc1105b`（コミット `dcc1105b0cf1322a437b354c336f2abf72b7e512`）。
PlatformIO Core 6.1.19、Espressif32 6.12.0、Xtensa ESP32-S3 GCC 8.4.0+2021r2-patch5を使用します。
SDKのビルド済みバイナリ、コンパイラー、PlatformIO本体はこの**ソースZIPには含めず**、
固定した通常のPlatformIOパッケージから取得します。完全オフラインの全ツール一式ではありません。
MIT系のM5ライブラリ等も `platformio.ini` に固定した版から取得します。NFCのローカル修正はビルド時に再適用します。

## 再ビルド

1. 新しいフォルダーにZIPを展開し、PythonとPlatformIO Core 6.1.19を用意します。
2. 展開先の `touch-card/` で通常ビルドします。必要な固定版パッケージを取得します。

   ```sh
   pio run -d firmware -e paper-mono -e stackchan -e paper-mono-en -e stackchan-en
   ```

3. Arduinoを改変する場合は、ZIP展開先で下記を実行します。
   `INSTALLED_FRAMEWORK` はPlatformIOの `framework-arduinoespressif32` パッケージのパスに置き換えます。
   `editable-arduino` はまだ存在しないフォルダーを使います。

   ```sh
   python3 restore_framework.py --installed INSTALLED_FRAMEWORK --output editable-arduino
   ```

4. `editable-arduino/cores` や `libraries` を改変します。`touch-card/firmware/platformio.ini` の
   `[env]` の `platform_packages` を、作ったフォルダーの**絶対パス**を使って
   `platformio/framework-arduinoespressif32 @ symlink://ABSOLUTE_EDITABLE_ARDUINO_PATH` に置き換えます。
   PlatformIOの[カスタムパッケージ指定](https://docs.platformio.org/en/latest/projectconf/sections/env/options/platform/platform_packages.html)を使用します。
5. 手順2のビルドを再実行します。Arduinoのソースから再コンパイルし、M5 Touch Cardと再リンクします。
   `firmware/.pio/build/環境名/` の生成物と、同梱のインストール手順を使って導入できます。
   改変前後でバイナリのSHA-256が変わるのは正常です。

書き込みは別操作です。機種・バックアップ・書き込み領域を確認し、既存データを保護してください。
この補助はビルドやダウンロードの時点で実機を変更しません。

## English

Distribute this source ZIP alongside the matching v0.1.0 firmware ZIPs. M5 Touch Card's original code is MIT;
Arduino and third-party components retain their original licenses. Modification and reverse engineering
for debugging such modifications are not prohibited. This uses source distribution, not a three-year written offer.

The archive includes all application firmware sources/assets/protocol/build patches, the exact Arduino
core/libraries/variants and build scripts, ESP32-S3 headers/linker scripts/configuration, original notices,
and hashes linking the bundle to the firmware. The precompiled SDK, toolchains and other pinned dependencies
are external PlatformIO downloads, not bundled here; this is not a fully offline toolchain distribution.

Extract to a new directory. Use PlatformIO Core 6.1.19 and run the four-environment command above inside
`touch-card/`. The config pins Espressif32 6.12.0 and Arduino 3.20017.241212+sha.dcc1105b.
To change Arduino, run `restore_framework.py` from the extraction directory with the actual installed
framework path and a new output directory. It validates sources and SDK hashes, copies the installed SDK,
and overlays the supplied library sources without changing the installed package.

Edit that copy, replace `[env] platform_packages` in the application's config with
`platformio/framework-arduinoespressif32 @ symlink://ABSOLUTE_EDITABLE_ARDUINO_PATH`, then rebuild.
This recompiles the modified Arduino library and relinks the application. Follow the bundled installation
guide separately for flashing; preserve user data/backups. No device access occurs during source restoration.
