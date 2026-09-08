# リリース用ファイルの作成 / Release packaging

インストールは[日本語](../docs/install.ja.md) / [English](../docs/install.en.md)を参照してください。
ここではメンテナー向けに配布ファイルの作成方法を説明します。

## 配布ファイル

- `touch-card-v0.1.0-paper-mono.zip`: PaperMono C153用。日英版、書き込み補助、手順、通知を同梱
- `touch-card-v0.1.0-stackchan.zip`: 公式StackChan K151 / K151-R用。同上
- `touch-card-v0.1.0-android.apk`: リリース鍵で署名したAndroidアプリ
- `touch-card-v0.1.0-firmware-sources.zip`: 対応する本体・Arduinoソースと再リンク手順
- `SHA256SUMS`: 上記4ファイルのSHA-256

## ファームウェアと対応ソース

リポジトリのルートで実行します。`FRAMEWORK_DIR` は使用するPlatformIOのArduino-ESP32パッケージのパスに置き換えてください。
出力先には新しいディレクトリを指定します。

```sh
pio run -d firmware -e paper-mono -e stackchan -e paper-mono-en -e stackchan-en
python3 -m unittest discover -s scripts/release -p 'test_*.py'
python3 scripts/release/package.py --build-root . --framework-dir FRAMEWORK_DIR --output dist/v0.1.0-candidate
python3 scripts/release/package_sources.py --framework-dir FRAMEWORK_DIR --output dist/v0.1.0-candidate/touch-card-v0.1.0-firmware-sources.zip
```

作成ツールはソースとの一致、機種・書き込み領域、ハッシュ、ローカルPCのパス混入、リンク済みフォント、同梱通知を検査します。
本体への書き込みは行いません。`release-status.json` は作成ツールの出力記録で、配布ファイルには含めません。
対応ソースと再リンク方法は[こちら](corresponding-source.md)、通知の構成は[再配布の案内](../docs/distribution-review.ja.md)を参照してください。

## Android APK

リリース署名には次の環境変数を使用します。鍵とパスワードはリポジトリへ保存しないでください。

- `ANDROID_KEYSTORE_PATH`
- `ANDROID_KEYSTORE_PASSWORD`
- `ANDROID_KEY_ALIAS`
- `ANDROID_KEY_PASSWORD`

`mobile/` で実行します。

```sh
flutter pub get --enforce-lockfile
flutter analyze
flutter test
flutter build apk --release --build-name=0.1.0 --build-number=1 --split-debug-info=build/symbols/v0.1.0
```

更新用APKは同じリリース鍵で署名してください。[署名証明書の照合情報](android-signing.md)で確認できます。
Android SDKの `apksigner verify --verbose --print-certs` で署名を検査し、アプリID・バージョン・非debug設定・同梱ライセンスを確認します。
検証したAPKを `touch-card-v0.1.0-android.apk` として配布ディレクトリへ配置します。
symbolsは診断用に非公開で保管し、APKやReleaseへ追加しないでください。

## 配布時の確認

1. ビルドに使ったソースコミットとバージョンを揃えます。[日英リリース説明](notes-v0.1.0.ja.md)も更新します。
2. 機種別ZIP・署名済みAPK・対応ソースZIPを揃え、4ファイルのSHA-256を `SHA256SUMS` に記録します。
3. 通知・対応ソース・再リンク手順をバイナリと同じReleaseに添付します。
4. アップロードしたファイルとハッシュ、日英の手順リンクを照合します。

実機ダンプ、名刺・個人写真、署名鍵、ローカル設定、デバッグAPK、ELF/MAP、ログは添付しません。
READMEの動画はGitHubの添付URLを使用します。動画をReleaseの配布ファイルに追加する必要はありません。

## English

Use the commands above to build four firmware variants and package both board ZIPs plus the matching source ZIP.
Replace `FRAMEWORK_DIR` with the installed Arduino-ESP32 package path and use a new output directory.
The packager checks source consistency, chip/offsets, hashes, private build paths, linked fonts and notices. It does not flash hardware.
`release-status.json` is a local tool report, not a release asset.

Build Android from `mobile/` with the four signing environment variables listed above. Retain the same signing key for updates;
check it against the [certificate identity](android-signing.md). Verify the APK signature, app ID/version, non-debug settings and bundled licenses.
Keep symbols and signing credentials private.

For each release, align the source commit/version, update both release-note languages, and attach the two board ZIPs, signed APK,
matching source ZIP and checksums together. Verify uploaded hashes and documentation links.
Do not attach device dumps, personal cards/photos, keys, local settings, debug APKs, ELF/MAP files or logs.
See [redistribution](../docs/distribution-review.en.md) and [relinking](corresponding-source.md) for source and notice requirements.
