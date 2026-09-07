# リリース準備 / Release preparation

初回は **v0.1.0の通常Release**。GitHubのPre-releaseにはしません。
PaperMono / StackChanのファームウェアZIPとAndroidの署名済みAPKを対象にします。
iPhoneはソースと開発者向けビルド手順のみ。IPA・TestFlight・招待リンクは初回には含めません。
GitHubの説明は日本語を先に掲載し、英語版へリンクします。

配布候補は作成済みで、利用者によるリリース向け実機確認も完了しています。実機の再確認は行いません。
現在の残項目は、公開先・コミット/タグの確定と、対応するファイルのGitHub公開です。

## 配布予定ファイル

- `touch-card-v0.1.0-paper-mono.zip`: C153、日英初期版、インストール補助、手順・通知
- `touch-card-v0.1.0-stackchan.zip`: K151 / K151-R、同上
- `touch-card-v0.1.0-android.apk`: 配布用署名鍵で署名したAPK
- `SHA256SUMS`: 最終配布ファイルのSHA-256
- `touch-card-v0.1.0-firmware-sources.zip`: 対応する自作ファームウェアとArduinoのソース、再リンク用の資料

`release-status.json` はローカル準備状況の記録です。作成したZIPをそのまま公開済み・公開可能と扱わないでください。
実機ダンプ、内部/SDの名刺、署名鍵、署名用環境ファイル、デバッグAPK、ELF/MAP、個人ログは添付しません。

## 手順

1. [インストール案内](../docs/install.ja.md)と[リリース説明](notes-v0.1.0.ja.md)の内容を確認します。
2. GitHubの所有者・リポジトリと公開用の著者名/メールを確定し、レビューしたソースをコミット・タグ化します。
3. そのソースで本体4環境をビルドします。ビルド時にPCのパスを公開用のパスへ置換します。
4. Androidの配布用署名鍵を用意し、秘密値を環境変数へ設定してreleaseビルドします。
5. ZIP・APK・対応ソース・通知を点検し、最終ファイルのSHA-256を作成します。
6. 実機確認の記録を参照します。現在の候補は利用者から確認完了の報告を受けており、再実施待ちではありません。
7. 確認項目が揃ってから通常のGitHub Releaseを作成します。公開は別途、明示的に実行します。

### ファームウェア候補の作成

リポジトリのルートで実行します。`FRAMEWORK_DIR`はPlatformIOのArduino-ESP32パッケージの実際のディレクトリに置き換えます。

```sh
pio run -d firmware -e paper-mono -e stackchan -e paper-mono-en -e stackchan-en
python3 -m unittest discover -s scripts/release -p 'test_*.py'
python3 scripts/release/package.py --build-root . --framework-dir FRAMEWORK_DIR --output dist/v0.1.0-candidate
python3 scripts/release/package_sources.py --framework-dir FRAMEWORK_DIR --output dist/v0.1.0-candidate/touch-card-v0.1.0-firmware-sources.zip
```

出力先は新しいディレクトリが必要です。既存の候補を無断で上書きしません。
ZIPにはフラッシュ全体を結合したイメージを入れません。書き込み領域は初回と更新で分離します。
同梱ライセンスの収集だけで、LGPL等の対応ソース・再リンク要件の確認が完了したとは扱いません。
ライセンス原文は[Arduino-ESP32 2.0.17](https://github.com/espressif/arduino-esp32/blob/2.0.17/LICENSE.md)由来で、同梱コピーは改変しません。
対応ソースZIPの構成と改変方法は[再リンク用の資料](corresponding-source.md)を参照してください。
実際のArduinoソースを使い、検証用コピーへの変更が完成ELFへ反映されることを確認済みです。
検証用の変更は配布物へ含めません。SDK追加部品の通知・由来の確認結果は[配布条件の確認](../docs/distribution-review.ja.md)を参照してください。
作成後、ソースZIPと署名済みAPKも含めて `SHA256SUMS` を再生成します。

### Android署名

既存の設定は次の4つです。値をドキュメント・ログ・GitHubへ貼らないでください。

- `ANDROID_KEYSTORE_PATH`
- `ANDROID_KEYSTORE_PASSWORD`
- `ANDROID_KEY_ALIAS`
- `ANDROID_KEY_PASSWORD`

同じ鍵を今後の更新にも使うため、鍵と復旧情報をリポジトリ外で安全に保管します。
公開用の鍵を自動で使い捨て生成したり、debug鍵でreleaseを代用したりしません。
不足する場合、releaseビルドは明示的に失敗します。

M5 Touch Card専用の配布鍵を作成済みです。証明書名は **Touch Card Release** で、実名・メールは含めていません。
公開してよい照合情報は[Android署名証明書](android-signing.md)を参照してください。
秘密鍵とパスワードはリポジトリ外で分けて保管し、今後も同じ鍵を使用します。

`mobile/`で:

```sh
flutter pub get --enforce-lockfile
flutter analyze
flutter test
flutter build apk --release --build-name=0.1.0 --build-number=1 --split-debug-info=build/symbols/v0.1.0
```

Android SDKの `apksigner verify --verbose --print-certs` で検証し、debug証明書でないこと・想定した署名者であることを確認します。
APK内部のアプリID・versionName/versionCode、ライセンス表示、NFC操作も確認します。
診断用のsymbolsは配布APKと対応づけて非公開で保管し、Releaseへ添付しません。
Flutterが実行時に使う生成プラグインのURIは残るため、個人名を含まないビルド場所を使います。
バイナリの文字列を後から置換して機能を壊すような処理は行いません。
配布時は検証したAPKを上記のファイル名にし、ZIPとAPKを含む最終 `SHA256SUMS` を作成します。

## 公開前の確認項目

ローカル検査の記録は[公開準備結果](../docs/publication-readiness.md)を参照してください。

- [ ] 公開先・ソースコミット・v0.1.0タグの確定
- [x] 4本体環境とFlutterの検査成功（ローカル、2026-09-07）
- [x] 本体のリリース向け実機受け入れ（利用者の確認完了報告、追加試験なし）
- [x] 専用鍵での署名済みAPK作成・署名と配布内容の検証（ローカル）
- [x] Androidインストール・起動確認、および実機受け入れ（後者は利用者の確認完了報告）
- [x] GNU FreeFont由来の数字をOFLへ置換、4構成のリンク済みフォント検査
- [x] 対応ソースZIP作成・改変Arduinoでの再リンク確認（ローカル、未フラッシュ）
- [x] Androidのネイティブ依存44個の追加通知を署名済みAPK内で検証
- [x] SDK・画像デコーダー等の原文・由来の照合と収録検査（配布条件の確認を参照）
- [x] ローカル候補のファイル名・日英手順・SHA-256の一致（公開時にも対象ファイルを照合）
- [x] 利用者提供の写真とノーカット動画4本を用意し、日英READMEに4本分の掲載欄を準備
- [x] 動画4本をGitHubへ添付し、日英READMEの仮リンクを差し替え、各URLのプレイヤーへの変換を確認
- [ ] ブラウザ上で日英READMEの動画再生操作を確認
- [x] 同一機種ペア・網羅的なSD故障/電源断などの確認範囲外をリリース説明へ明記
- [x] GitHub上の初回CI成功を確認
- [ ] GitHub上の最終配布リンクを確認

## English

The first publication is a normal **v0.1.0 GitHub Release**, not a pre-release.
Ship model-specific firmware ZIPs and a release-signed Android APK. iPhone remains source/developer-build only;
do not attach IPA files or TestFlight links initially. Japanese is the primary documentation/release-note language.

The packaging command above creates local candidates only. It checks firmware source consistency, image hashes,
chip/offset constraints, and developer-path leakage, and bundles installation guides and license notices.
It never signs, flashes, uploads or erases anything. Output directories must be new.

Before publishing, finalize the public source commit/tag, retain the Android signing key securely for future updates,
and publish the matching APK, firmware, notices, source ZIP and checksums together.
Hardware acceptance is complete based on the user's confirmation; no repeat hardware tests are required for this documentation task.
License collection alone does not complete the corresponding-source/relinking review; the checks below record the additional verification performed.
Keep full-flash backups, credentials, debug artifacts and personal card data out of releases.
The dedicated release key has been created; see the [public signing-certificate identity](android-signing.md).
The signed APK was installed and launched successfully on Xiaomi / Android 16. The user confirmed that release hardware checks are complete.
This is user-reported acceptance, not a claim of additional device tests during documentation cleanup.
The source/relinking ZIP is now generated and a modified Arduino library successfully relinks into firmware.
GNU FreeFont-derived digits are replaced with OFL Touch Digits; all four linked-font checks pass.
Native Android notices are verified inside the signed APK. SDK component notice/provenance checks
are recorded in the [distribution review](../docs/distribution-review.en.md). Include the source ZIP in the final checksums.
See [installation](../docs/install.en.md) and [release notes](notes-v0.1.0.en.md).
The bilingual READMEs include the approved photo and four sections for uncut, real-speed videos.
All four videos are uploaded as GitHub attachments and referenced in both READMEs. GitHub's Markdown renderer generates a player for each matching file.
Browser playback verification remains separate from this rendering check.
Videos do not need to be included as Release assets; see the [media publishing steps](../docs/media/README.md).
