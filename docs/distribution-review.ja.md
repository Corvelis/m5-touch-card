# 再配布と第三者ライセンス

[日本語](distribution-review.ja.md) · [English](distribution-review.en.md)

M5 Touch Cardの自作コードは[MIT](../LICENSE)です。第三者のライブラリ・フォント・ランタイムには、それぞれのライセンスが適用されます。
このページは収録物と対応資料の案内です。ライセンス全文は各ファイルおよび同梱通知を参照してください。

## 同梱する資料

| 対象 | ライセンス・資料 |
| --- | --- |
| 自作コード・初期画像 | MIT。画像の範囲は各assets内のLICENSEに記載 |
| TouchSansJP・Touch Digits | SIL OFL 1.1。フォントと変換スクリプトを保持 |
| /efont/・標準Font0 | 各フォントのBSD系著作権表示・通知を保持 |
| M5系ライブラリ・ArduinoJson | [第三者通知](../THIRD_PARTY_NOTICES.md)と同梱の原文 |
| Arduino-ESP32 | LGPL-2.1-or-later等。対応ソースと再リンク手順を提供 |
| ESP-IDF・ランタイム | [通知と由来](../release/licenses/runtime/)を同梱 |
| Androidネイティブ依存 | [依存一覧・ハッシュ](../release/android-dependencies.json)と[通知全文](../mobile/licenses/ANDROID-NATIVE-NOTICES.txt) |
| Flutter・Dart | アプリ内のライセンス一覧に収録 |

名刺の数字はOFLのTouch Digitsを使用します。配布ツールはGNU FreeFont由来のFreeSans/FreeMono/FreeSerifがリンクされていないか検査します。
通知集には未リンクの部品の通知も含まれるため、通知の掲載だけでその部品が使用されているとは限りません。

## ファームウェアの対応ソース

`touch-card-v0.1.0-firmware-sources.zip` を、同じ版の本体ZIPと一緒に配布してください。
自作ファームウェア、使用したArduinoのcore/libraries/variants、ESP32-S3のヘッダー・設定・リンカースクリプト、再リンク用の補助を含みます。
Arduinoの固定コミットは `dcc1105b0cf1322a437b354c336f2abf72b7e512` です。
コンパイラーやSDKバイナリは固定版のPlatformIOパッケージから取得します。

各収録ソース・外部SDK入力はSHA-256で記録しています。[対応ソースと再リンク](../release/corresponding-source.md)に従い、
インストール済みSDKを変更せずに改変用コピーを作り、ライブラリを修正して再ビルドできます。

## 再配布時

- 著作権表示、ライセンス全文、NOTICE、フォントの通知を保持してください。
- 改変版は変更内容を明示し、そのバイナリに対応するソース・ビルド手順を揃えてください。
- ファームウェアと対応ソースへ同じ配布場所からアクセスできるようにしてください。
- Androidの通知はアプリ上部のiボタンから閲覧できます。再配布時も表示と同梱内容を保持してください。

配布ファイルの生成は[リリース用ファイルの作成](../release/README.md)を参照してください。
