# M5 Touch Card v0.1.0

[日本語](notes-v0.1.0.ja.md) · [English](notes-v0.1.0.en.md)

PaperMonoとｽﾀｯｸﾁｬﾝで、タッチして名刺を交換。
スマホで編集した名刺をNFCで更新し、受け取った名刺を本体やmicroSDに保存できます。

## 主な機能

- 「交換」「渡す」「受け取る」に対応。完了すると相手の名刺を表示します。
- 丸いアイコンと3種類のデザイン。連絡先・QR用URL・一言は任意で、PaperMonoは縦向き／横向きを選べます。
- 名刺帳の名前順／最新順、詳細表示、個別削除。本体・microSDの使用量と空き容量を確認できます。
- 名刺の内容・アイコン・ホーム写真・時計をスマホからNFCで更新できます。
- 本体・スマホとも日本語／Englishに対応します。
- PaperMonoは消灯中、通信などの処理がないときにライトスリープ。時計・電池は毎分部分更新し、毎時00分に残像消去用の全画面更新を行います。
- StackChanは電源ボタン短押しで画面をオン・オフできます。

## ダウンロード・インストール

[インストール手順](../docs/install.ja.md) · [操作ガイド](../docs/operation.ja.md) · [実機デモ](../README.md#デモ)

[Releases](https://github.com/Corvelis/m5-touch-card/releases)の **Assets** から選んでください。

| ファイル | 内容 |
| --- | --- |
| `touch-card-v0.1.0-paper-mono.zip` | M5 PaperMono C153用ファームウェア |
| `touch-card-v0.1.0-stackchan.zip` | 公式StackChan K151 / K151-R用ファームウェア |
| `touch-card-v0.1.0-android.apk` | NFC対応Android 7.0以上用の署名済みアプリ |
| `touch-card-v0.1.0-firmware-sources.zip` | 対応ソースと再リンク手順 |
| `SHA256SUMS` | ダウンロードファイルの照合用ハッシュ |

本体ZIPには日英の初期言語版と書き込みツールを同梱しています。本体の設定で後から言語を切り替えられます。
既存M5 Touch Cardはインストール手順の「更新」を使ってください。保存領域が一致すれば、名刺・画像・設定を保持して更新します。
iPhoneは[ソースからのビルド](../docs/install.ja.md#iphone-開発者向け)に対応します。IPA・TestFlightでの配布はありません。

## 注意点

- PaperMono Lite、自作StackChan、他のM5コアは対象外です。旧v1画像転送ファームウェアとは通信・データの互換性がありません。
- PaperMono ↔ StackChanの交換は実機確認済みです。同一機種2台での交換は未検証です。
- 電子ペーパーの高速更新では残像が残る場合があります。起動時・毎時の全画面更新では画面が点滅します。
- 名刺は平文保存で、NFCに暗号化・暗号学的な相手認証はありません。書き込みや初期化の前に大切なデータをバックアップしてください。

自作コードは[MIT](../LICENSE)、第三者の部品は各ライセンスに従います。[第三者通知](../THIRD_PARTY_NOTICES.md)と[対応ソース](corresponding-source.md)を参照してください。
不具合報告には機種・バージョン・操作手順・エラー番号を添え、実際の名刺やフラッシュバックアップは添付しないでください。
