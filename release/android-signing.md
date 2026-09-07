# Android配布版の署名 / Android release signing

初回v0.1.0用のM5 Touch Card専用署名証明書です。ここには秘密鍵・パスワードを記載しません。

| 項目 / Field | 値 / Value |
| --- | --- |
| アプリID / Application ID | `io.github.corvelis.touch_card` |
| 証明書名 / Certificate | `CN=Touch Card Release, O=Touch Card` |
| 公開鍵 / Public key | RSA 3072-bit |
| 有効期限 / Valid until | 2054-01-23 UTC |
| SHA-256 | `E0:B6:E4:D6:54:9B:1E:ED:E8:0E:F3:7A:D4:60:A3:27:73:05:1F:0A:F7:40:EF:FA:B8:77:7D:1B:B0:D0:BD:37` |

通常の更新は同じ鍵で署名します。GitHubへ添付するのは署名済みAPKで、鍵ファイルではありません。
開発用debug鍵とは別なので、既存のdebug版へはそのまま上書きできない場合があります。
アンインストールすると保存データを失うため、先に[インストール・更新手順](../docs/install.ja.md)を確認してください。

APKのファイルSHA-256はReleaseの `SHA256SUMS`、署名証明書はAndroid SDKの次の読み取り専用検査で確認できます。

```sh
apksigner verify --verbose --print-certs touch-card-v0.1.0-android.apk
```

検証に成功し、証明書SHA-256が上記と一致することを確認します。照合には信頼できる配布元の情報を使用してください。
署名検査の成功だけで、アプリの安全性・NFCの実機動作を保証するものではありません。
詳細は[Android公式の署名検証](https://developer.android.com/tools/apksigner)を参照してください。

## English

This is the dedicated signing identity for M5 Touch Card v0.1.0. No private key or password is published here.
Keep using the same key for ordinary updates. Verify the APK with the command above and compare the certificate SHA-256
against this trusted record; compare the APK file hash separately against the release's `SHA256SUMS`.
Debug builds use a different key and may not support an in-place update. Do not uninstall blindly, as local data would be lost.
See the [installation guide](../docs/install.en.md). Signature validation does not replace hardware or security testing.
