# ビルド・テスト

[日本語](development.ja.md) · [English](development.en.md) · [README](../README.md)

配布ファイルを使う場合は[インストール手順](install.ja.md)を参照してください。

## 開発環境

- PlatformIO Core 6.1.19
- Flutter 3.32.8 / Dart 3.8.1
- CMake、Clang/GCC、Node.js
- Android: JDK 17、Android SDK 36、NDK 27.0.12077973
- iOS: macOS、Xcode、CocoaPods 1.16.2

依存バージョンは `firmware/platformio.ini` と各lockfileに固定しています。
本体のビルドはWindows / macOS / Linux、C++のホストテストはmacOS / Linux向けです。

## 本体

リポジトリのルートで実行します。

```sh
# 日本語を初期言語にする
pio run -d firmware -e paper-mono -e stackchan
# 英語を初期言語にする
pio run -d firmware -e paper-mono-en -e stackchan-en

cmake -S . -B build/native-tests
cmake --build build/native-tests
ctest --test-dir build/native-tests --output-on-failure
```

初期言語は日本語です。保存済みの言語設定があれば、ビルドの初期値より優先します。
独自環境では `TOUCH_CARD_DEFAULT_LANGUAGE=0`（日本語）/ `1`（英語）を指定できます。

CMakeは `paper-mono` のビルドで取得したArduinoJsonを使います。別の配置では
`-DARDUINOJSON_DIR=/path/to/ArduinoJson/src` を指定してください。

ビルドだけでは本体を書き換えません。[配布用ZIPの作成](../release/README.md)と[書き込み手順](install.ja.md)を参照してください。

## スマホアプリ

`mobile/` で実行します。

```sh
flutter pub get --enforce-lockfile
flutter analyze
flutter test
flutter build apk --debug
flutter build ios --simulator --no-codesign
```

英語を初期言語にする場合は、ビルドに `--dart-define=TOUCH_CARD_DEFAULT_LANGUAGE=en` を追加します。
指定しない場合は日本語です。保存済みの言語設定は維持します。
iPhone実機への導入には[署名設定](install.ja.md#iphone-開発者向け)が必要です。シミュレーターではNFCを利用できません。
配布用Android APKの署名は[リリース手順](../release/README.md)を参照してください。

## 追加の検査

```sh
node scripts/check_native_protocol.mjs
python3 -m unittest discover -s scripts/release -p 'test_*.py'
node scripts/check_public_source.mjs --self-test
node --test scripts/check_public_source.test.mjs
node scripts/check_public_source.mjs
```

共通ベクトル検査にはNode.js、Kotlin/JDK、Swiftが必要です。
PaperMonoの描画確認は[描画ツール](../scripts/paper_preview/README.md)を参照してください。
GitHub Actionsでは本体4構成、C++テスト、Flutter解析・テスト、ソースと配布スクリプトの検査を実行します。

## ソース構成

- `firmware/`: 本体の表示・NFC・ストレージ・電源制御
- `mobile/`: FlutterアプリとAndroid/iOSのNFC実装
- `protocol/`: 通信仕様と共通テストベクトル
- `scripts/`: フォント生成・描画確認・配布ツール
- `migration/legacy/`: 移植元の参照コードと回帰検査用データ。現行アプリの入口ではありません。

変更提案と不具合報告は[CONTRIBUTING](../CONTRIBUTING.md)、対応ソースの再リンクは[こちら](../release/corresponding-source.md)を参照してください。
