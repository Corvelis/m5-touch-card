# M5 Touch Card

[日本語](README.md) · [English](README.en.md)

**タッチして、名刺を交換。**

M5 PaperMonoと公式ｽﾀｯｸﾁｬﾝで使える、オフラインのNFC名刺交換アプリです。
スマホで名前やアイコンを編集し、本体にタッチして更新。受け取った名刺は名刺帳に保存できます。
Wi-Fi接続やクラウドアカウントは不要です。

![PaperMonoとスタックチャンに表示した名刺](docs/media/devices.jpg)

## インストール

[Releasesからダウンロード](https://github.com/Corvelis/m5-touch-card/releases) · [書き込み・インストール手順](docs/install.ja.md)

| 機器 | 使用するファイル |
| --- | --- |
| M5 PaperMono C153 | `touch-card-v0.1.0-paper-mono.zip` |
| 公式StackChan K151 / K151-R | `touch-card-v0.1.0-stackchan.zip` |
| NFC対応Android 7.0以上 | `touch-card-v0.1.0-android.apk` |
| iPhone | [ソースからビルド](docs/install.ja.md#iphone-開発者向け) |

本体用ZIPには日本語・英語のファームウェア、書き込みツール、手順を同梱しています。
Windows / macOS / Linuxに対応。既存のM5 Touch Cardを更新する場合は、手順の「更新」を使ってください。
PaperMono Lite、自作StackChan、他のM5コアは対象外です。

## できること

- **名刺を交換**：「交換」「渡す」「受け取る」を選んでタッチ。交換後は相手の名刺を表示します。
- **自分らしい名刺**：名前だけで作成でき、丸いアイコン・アカウント・メール・QR用URL・一言は任意。3種類のデザインと、PaperMonoの縦向き・横向きに対応します。
- **スマホから更新**：名刺の内容・アイコン・ホーム写真・時計をNFCで送信。名刺アイコンと2種類のホーム写真は別々に保存します。
- **名刺帳**：名前順・最新受信順で閲覧し、詳細・QRの表示や個別削除ができます。
- **microSD対応**：本体のみでも使用可能。保存先を選び、使用量・空き容量を確認できます。件数固定の上限や古い名刺の自動削除はありません。
- **選べるホーム**：名刺、画像＋日時・カレンダー、全画面写真。日本語 / Englishは本体とスマホでそれぞれ切り替えられます。
- **PaperMonoの省電力**：Bでライトを切り替え。消灯中は操作をロックし、通信などの処理がないときはライトスリープ。時計・電池は毎分部分更新し、毎時00分に全体更新します。

詳しくは[操作ガイド](docs/operation.ja.md)を参照してください。

## はじめて使う

1. 本体の「メニュー → 設定 → スマホから更新」を開きます。
2. スマホの「名刺」で名前などを保存し、「NFCで名刺を更新」を選んで本体にタッチします。
3. 時計はスマホ上部の時計ボタンから送信します。本体の待機画面はそのままで構いません。
4. 本体同士の交換は、両方で「名刺交換 → 交換」を開き、片方を「先に渡す」、もう片方を「先に受け取る」にします。
5. NFC部分を合わせ、両方向の交換が完了するまで待ちます。受け取った名刺は名刺帳からも閲覧できます。

## デモ

### 名刺交換 · 約25秒

端末を近づけて名刺を交換。受け取った相手の名刺が、両方の画面に表示されます。

<!-- BEGIN VIDEO: card-exchange -->

https://github.com/user-attachments/assets/cd2798ff-c891-4521-8a90-304021eb681b

<!-- END VIDEO: card-exchange -->

### スマホから更新 · 約31秒

<!-- BEGIN VIDEO: phone-update -->

https://github.com/user-attachments/assets/b0046ea4-0d4f-4b6d-bd87-edb3436d9dc6

<!-- END VIDEO: phone-update -->

### デザインを選ぶ · 約31秒

<!-- BEGIN VIDEO: card-designs -->

https://github.com/user-attachments/assets/99213d5d-7417-4696-aaa6-da09b605a35e

<!-- END VIDEO: card-designs -->

### 名刺帳を見る · 約17秒

<!-- BEGIN VIDEO: card-book -->

https://github.com/user-attachments/assets/7bbaaae1-e9f9-4768-aa49-6665adaf12c1

<!-- END VIDEO: card-book -->

実機撮影・ノーカット・等速です。スマホの動画はiPhoneで撮影しています。

## 対応・注意点

- 交換する両方の本体にM5 Touch Cardが必要です。旧v1画像転送ファームウェアとは通信できません。
- PaperMono ↔ StackChanの交換は実機確認済みです。同一機種2台での交換は未検証です。
- 名刺は本体・SDへ平文で保存され、NFC通信に暗号化・暗号学的な相手認証はありません。大切なデータはバックアップしてください。
- QRは自分で設定したURLを表します。Webページを作成・公開する機能はありません。

## 開発・ライセンス

自作コードは[MITライセンス](LICENSE)です。外部ライブラリ・フォントにはそれぞれのライセンスが適用されます。
ファームウェアと同じリリースに、対応ソースと再リンク手順を含むZIPを添付しています。

- [ビルド・テスト](docs/development.ja.md) / [開発への参加・不具合報告](CONTRIBUTING.md)
- [通信仕様](protocol/specification.md) / [セキュリティ](SECURITY.md)
- [第三者通知](THIRD_PARTY_NOTICES.md) / [再配布の案内](docs/distribution-review.ja.md) / [対応ソース・再リンク](release/corresponding-source.md)
