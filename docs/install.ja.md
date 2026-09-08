# インストール

[日本語](install.ja.md) · [English](install.en.md) · [概要](https://github.com/Corvelis/m5-touch-card)

[Releases](https://github.com/Corvelis/m5-touch-card/releases)の **v0.1.0 → Assets** から、使用する機器のファイルをダウンロードしてください。
本体はUSB接続のPCから書き込み、AndroidアプリはAPKを開いてインストールします。
iPhoneは[ソースからのビルド](#iphone-開発者向け)に対応します。

## 1. ダウンロードするもの

| 使用する機器 | ファイル |
| --- | --- |
| M5 PaperMono C153 | `touch-card-v0.1.0-paper-mono.zip` |
| 公式StackChan K151 / K151-R | `touch-card-v0.1.0-stackchan.zip` |
| NFC対応Androidスマホ | `touch-card-v0.1.0-android.apk`（署名済み配布版） |
| ダウンロード内容の照合 | `SHA256SUMS` |

PaperMono Lite、自作StackChan、他のM5コア向けではありません。同じESP32-S3でも別機種のZIPを使わないでください。
ZIPには `ja/`（日本語初期）と `en/`（英語初期）があり、どちらも後から設定で言語変更できます。
保存済みの言語がある場合は、その選択が初期言語より優先されます。
GitHubが表示する「Source code」は開発者用で、本体へそのまま書き込むファイルではありません。

## 2. 本体を書き込む前に

- PC（Windows/macOS/Linux）、Python 3.9以上、データ通信対応USBケーブルを用意してください。
- 書き込む本体を1台だけ接続し、シリアルモニターや他の書き込みアプリを閉じます。
- 本体の既存ファームウェアは置き換わります。旧画像転送製品などからの名刺・画像・設定の自動移行はありません。
- ZIPを展開し、そのフォルダーをターミナルで開きます。ZIP内のまま実行しないでください。
- 同梱の `flash.py` は書き込み前に内部フラッシュ全体をバックアップします。microSDは対象外なのでPCで別途コピーしてください。
  バックアップには個人情報が含まれます。`private-backups/` はGitHub・Issue・SNSへ添付しないでください。
- 全消去や保護機能を無視するオプションは使いません。失敗した場合も、データ領域の初期化を最初の対処にしないでください。

まずReleasesに掲載されたSHA-256とダウンロードしたZIP/APKを照合します。
Windowsは `Get-FileHash ファイル名 -Algorithm SHA256`、macOSは `shasum -a 256 ファイル名`、
Linuxは `sha256sum ファイル名` を使えます。ZIP内部の各イメージは `flash.py` でも書き込み前に検査します。

### 書き込みツールの準備

Windows（PowerShell）:

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

以降の `python` は、Windowsなら `.\.venv\Scripts\python.exe`、macOS/Linuxなら `.venv/bin/python` に置き換えて実行してください。
`PORT` は確認したポート（例: Windowsの `COM5`、macOSの `/dev/cu.usbmodem...`、Linuxの `/dev/ttyACM0`）へ置き換えます。
機器の抜き差しで増減するポートを確認し、他の機器のポートを選ばないでください。

## 3. 初めてM5 Touch Cardを入れる

先に、機器へアクセスしない確認を実行します。

```sh
python flash.py --mode install --language ja
```

表示された機種・言語・対象ファイルを確認してから書き込みます。

```sh
python flash.py --mode install --language ja --port PORT --execute
```

確認欄に対象機種の `paper-mono` または `stackchan` を入力すると、内部バックアップ → 書き込み → 検証 → 再起動を行います。
バックアップに時間がかかる場合があります。完了するまでUSBを抜かないでください。
英語初期版は `--language en` を指定します。チップ検出だけでは2機種を区別できないため、機種確認は省略しないでください。

初回導入はブートローダー・パーティション表・起動先情報・アプリを書き込みます。
内部データ領域を全消去する処理はしませんが、以前と保存領域の構成が違えば、旧データを読めなくなる可能性があります。
バックアップを保管し、移行元の環境へ戻す場合はその環境に対応した復元手順を使ってください。

## 4. 既存M5 Touch Cardを更新する

同じ機種のM5 Touch Cardが既に入っており、リリース説明で更新対象とされている場合だけ使います。

```sh
python flash.py --mode update --language ja
python flash.py --mode update --language ja --port PORT --execute
```

バックアップ内のパーティション表が配布版と一致する場合に限り、アプリと起動先情報を更新します。
ブートローダー・パーティション表・NVS設定・名刺/画像領域は書き込みません。内部/SDの名刺は自動削除しません。
「保存領域が異なる」と出たら書き込みは行いません。`install`へ安易に切り替えず、バックアップとリリース説明を確認してください。
以前の開発版・別製品からのデータ互換や、電源断時の復旧を保証するものではありません。

## 5. Android APKを入れる・更新する

1. NFC搭載Android 7.0以上のスマホで、Releasesの配布用APKをダウンロードします。端末のNFCを有効にします。
2. ダウンロードしたAPKを開きます。許可を求められたら、そのブラウザー/ファイルアプリの「この提供元を許可」を有効にしてインストールします。
3. 完了後、必要がなければその提供元の許可を戻します。Play Protect等の保護機能を無効にする必要はありません。
4. Touch Cardを開きます。アプリ上部の地球メニューで日本語 / Englishを選べます。

更新は同じ提供元・同じ署名鍵の新しいAPKを上書きインストールします。
署名が違う開発版が既に入っていると更新できない場合があります。すぐにアンインストール/データ削除をせず確認してください。
アプリを削除すると下書き・元画像・送信待ちやプロフィールIDが失われるため、新しい名刺として扱われる場合があります。
現時点ではアプリ内バックアップ/復元UIはありません。
OSや管理端末の方針によって画面・制限が異なります。[Android公式案内](https://support.google.com/android/answer/9457058?hl=ja)も参照してください。

## 6. 最初の名刺と時計を設定する

1. 本体の「メニュー → 設定 → スマホから更新」を開きます。
2. スマホの「名刺」で名前を保存し、「NFCで名刺を更新」を選びます。
3. スマホと本体のNFC部分を合わせ、「保存完了」まで待ちます。本体で項目の受信先を選ぶ必要はありません。
4. 同じ本体待機画面で、スマホ上部の時計ボタンから日時を合わせます。
5. ホーム写真は「画像」で用途と送る端末を選び、切り抜いてNFC送信します。

初回に内部保存のエラーが出た場合は、配線・SD・バックアップを確認してから[操作ガイド](operation.ja.md)の初回セットアップを参照してください。
「消去して初期化」は内部の名刺/画像を消す操作です。更新作業の通常手順には含めません。
PaperMonoはライト消灯中にタッチできません。Bボタンで点灯してから操作します。

## 7. 困ったとき

- ポートが出ない: データ対応ケーブル、USB接続先、本体の電源を確認します。
- 接続失敗: シリアルモニターを閉じ、他の本体を外します。本体をメーカー指定のダウンロードモードにして再試行します。
  [PaperMono公式](https://docs.m5stack.com/en/core/PaperMono) / [StackChan公式](https://docs.m5stack.com/en/StackChan)
- PaperMonoが消灯中: Bボタンでライトを点けてから書き込んでください。ライトスリープ中はUSB接続に応答しない場合があります。
- 通信が途切れる: `--baud 115200` を追加して再試行します。バックアップが失敗した場合は書き込みへ進みません。
- ハッシュ検証失敗: 書き込まずに配布ファイルを再ダウンロードしてください。
- 起動しない: 機種の取り違え、電源、書き込み完了/検証結果を確認します。バックアップを消さずに問い合わせてください。
- NFCが反応しない: 本体を「スマホから更新」にし、スマホのNFC設定・アンテナ位置・ケースを確認します。

書き込みの基礎とオプションは[esptool 4系の公式資料](https://docs.espressif.com/projects/esptool/en/release-v4/esp32/esptool/basic-commands.html)を参照してください。
本製品の書き込み先はESP32-S3用で、同資料の汎用例のアドレスをそのまま使わないでください。

## iPhone: 開発者向け

macOS/Xcode、Flutter、CocoaPodsと、NFCに対応する署名・プロビジョニングが必要です。IPA・TestFlightでの配布はありません。

1. リリースと同じソースを取得し、`mobile/`で `flutter pub get --enforce-lockfile` を実行します。
2. `mobile/ios/`で `pod install --deployment` を実行し、固定された依存を用意します（検証済みCocoaPods: 1.16.2）。
3. `mobile/ios/Runner.xcworkspace` をXcodeで開き、RunnerのSigning & Capabilitiesで自分のTeamと利用可能なBundle Identifierを設定します。
4. NFC Tag Readingの権限・プロビジョニングを確認し、接続したNFC対応iPhoneを選んでビルド/実行します。
5. 署名設定・証明書・端末識別情報はGitHubへコミットしないでください。シミュレーターではNFC通信できません。
