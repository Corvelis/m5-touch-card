# 公開準備結果 / Publication readiness

更新日: 2026-09-07。対象は独立した `touch-card` リポジトリです。

## 現在の状態 / Current status

- ソース整理、日英ドキュメント、配布候補、写真と動画4本の掲載準備は完了しています。動画の埋め込みはGitHubへの添付と実URLへの差し替え待ちです。
- Androidの署名済みAPKはXiaomi / Android 16へインストール・起動確認済みです。以前の端末側インストール制限は解消しています。
- **利用者から実機確認は完了済みと報告されています。** Android・本体のリリース向け実機受け入れは完了として扱い、再実施を求めません。
- 今回は文書と掲載素材・確認記録を更新しました。新たな実機試験、書き込み、アプリのインストールは行っていません。
- GitHubリポジトリの作成、コミット、push、タグ、Releaseの公開は未実施です。

実機受け入れの根拠は利用者の完了報告です。既存の自動テスト、インストール結果、起動ログと区別して記録しています。
同一機種ペアや網羅的なSD故障・電源断試験まで新たに検証済みとするものではありません。
既知の範囲は[実装状況](implementation_status.ja.md)と[リリース説明](../release/notes-v0.1.0.ja.md)に残しています。

Source/docs/media preparation and user-confirmed release hardware acceptance are complete. Inline videos still need GitHub uploads and attachment URLs.
The signed Android APK was installed and launched successfully. The earlier device-side installation restriction is resolved.
This documentation update did not rerun hardware tests, flash devices or reinstall apps.
Publication, commit/tag and hosted CI verification remain outstanding. User-confirmed acceptance does not imply exhaustive same-model or SD-failure testing.

## 整理済みの内容 / Prepared materials

- 日本語をデフォルトにした日英README、操作・導入・更新ガイド、リリース説明。
- `docs/media/` に写真・ノーカット動画4本・サムネイルを保持。日英READMEは動画ごとの4見出しへ整理し、添付URLへの差し替え欄を準備しています。現在の動画リンクは仮の相対リンクです。
- デバイスのホーム画像・個人名刺・バックアップ・鍵・署名設定・作業用ファイルは公開対象外。掲載用に利用者が提供・確認したメディアだけを含めます。
- `.gitignore`、公開候補チェック、開発・不具合報告・セキュリティ案内、CIの読み取り権限と依存バージョンの固定。
- 自作コードのMIT、第三者フォント・ランタイムの原文・著作権表示、対応ソースと再リンク用資料。
- 実装範囲と過去の設計資料の区別。旧通信の参照・回帰検査に使う `migration/legacy` は維持。

The READMEs share a hero photo and four separate video sections prepared for inline playback. Relative video links remain as temporary fallbacks until upload.
Only the approved publication media are included; raw recordings and editing files remain excluded.

## 自動検査の記録 / Automated verification

以下は既存のビルド・試験記録です。今回の文書整理でファームウェアやAPKを再ビルドしたという意味ではありません。
公開候補だけの作業用コピーを使い、SDK・ダウンロードキャッシュは既存環境を利用しています。
完全に新しいOS・空のキャッシュや、GitHub上での実行を意味しません。

| 検査 / Check | 結果 / Result |
| --- | --- |
| PaperMono / StackChan × 日英初期値 | 4構成ビルド成功 / four builds passed |
| リンク済みフォント・数字幅・部分更新 | 4構成・ホスト描画検査成功 / passed |
| Flutter解析・テスト | 問題なし、59件成功 / 59 tests passed |
| CMake / CTest | 14件成功 / 14 tests passed |
| 配布・インストール補助の自動検査 | 21件成功、機器アクセスは模擬 / 21 tests passed with simulated device access |
| 公開候補チェックの回帰検査 | 4件成功 / four tests passed |
| Kotlin / Swift通信の共通ベクトル | 両方成功 / both passed |
| PaperMono日英描画・部分更新の画素検査 | 成功 / passed |
| Android debug・iOS署名なしシミュレーター版 | ビルド成功 / builds passed |
| CI YAML・文書のローカルリンク | 構文・リンク先を確認 / checked |

公開候補の検査とその回帰検査、文書・メディアのリンク、配布候補のSHA-256は文書整理後にも確認します。
これらは限定的な検査であり、あらゆる秘密情報・個人情報の不存在を保証しません。
公開前に選択したファイルとコミット予定の内容を確認し、staged検査も実施します。

検証環境: macOS、Flutter 3.32.8 / Dart 3.8.1、PlatformIO Core 6.1.19、Clang/CMake、Node.js。
Flutter/Dart・CocoaPodsのlockfileはこの文書整理では変更していません。

## 配布候補 / Release candidates

初回は通常の **v0.1.0 Release** で、次のファイルを同時に配布する方針です。
iPhoneはソースからのビルドのみで、IPA・TestFlightは初回に含めません。

- `touch-card-v0.1.0-paper-mono.zip`
- `touch-card-v0.1.0-stackchan.zip`
- `touch-card-v0.1.0-android.apk`
- `touch-card-v0.1.0-firmware-sources.zip`
- `SHA256SUMS`

本体ZIPは初回と更新の書き込み領域を分離し、診断用の個人PCパスを除去した4構成を収録しています。
Androidは専用RSA-3072鍵で署名済みです。署名証明書、ID・版番号、非debug設定、
バックアップ無効、NFC必須、Internet権限なし、16 KiB ZIPアラインメント、同梱通知を確認済みです。
公開してよい照合情報は[Android署名](../release/android-signing.md)にあります。
鍵とパスワードはリポジトリ外に分けて保管しています。今後の更新に必要なので、別媒体への安全なバックアップも保管してください。

自作コードはMIT、外部部品は元の条件を維持します。StackChanのGNU FreeFont由来の数字はOFLのTouch Digitsへ置換済みです。
Androidのネイティブ依存44個、追加ランタイム通知18ファイル、ソース内通知925種類と由来を照合しました。
対応ソースZIPを展開し、検証用のArduino変更が完成ELFへ反映される再リンク試験も成功しています。
検証専用の変更は配布物へ含めず、その改変版は実機へ書き込んでいません。
詳細は[配布条件の確認](distribution-review.ja.md)と[対応ソース・再リンク](../release/corresponding-source.md)を参照してください。

The signed APK, model-specific firmware ZIPs, matching source/relinking ZIP and checksums are prepared.
The dedicated signing key must be retained securely for future updates. Third-party notices and original license terms remain intact.
The source/relink test modified only a verification copy; that modified image was neither distributed nor flashed.

## メディア / Media

[READMEのデモ](../README.md#デモ)には、2台の写真と以下の4本を掲載しています。

- 名刺交換: 約25秒
- スマホから更新: 約31秒
- デザイン変更: 約31秒
- 名刺帳: 約17秒

4本とも全フレーム・再生時刻を元動画と照合済みで、カット・倍速・末尾の静止延長はありません。
固定した表示範囲の調整と日英字幕を加え、音声・撮影メタデータは除去しています。
軽量化済みMP4は各10MB未満、4本合計約21MBです。元の撮影動画や編集作業用ファイルは含めません。
スマホ動画はiPhone開発版で撮影したことを明記しています。
利用者の希望により、交換だけでなく4本すべてをREADME内で埋め込み再生する方針です。
[添付・差し替え手順](media/README.md)を準備しました。動画の外部アップロード、添付URLの取得、GitHub上でのプレイヤー確認は未実施です。
動画をReleaseの配布ファイルに添付することは前提にしていません。

## 公開工程の残り / Publication steps remaining

1. GitHubの所有者・リポジトリ名・公開範囲、公開用の作者名・メールを確定する。
2. [公開前チェック](publishing.md)に沿ってファイルを選択し、staged検査・コミット・push・v0.1.0タグを実施する。
3. 確定ソースと対応するZIP・APK・通知・対応ソース・チェックサムを同じReleaseへ添付する。
4. 動画4本をGitHubに添付し、日英READMEの仮リンクを同じ4つの添付URLへ差し替え、公開待ち案内を削除する。
5. GitHub上の初回CI、READMEの画像と両言語の動画4本の埋め込み再生、ダウンロードリンクと公開設定を確認する。

ローカルの `publish_ready: false` は、公開先・ソースコミット/タグと実際の公開が未確定であることを表します。
実機の再確認待ちではありません。署名鍵・Git設定・GitHubアカウント設定は、この文書整理では変更していません。

The remaining work is repository/author selection, staging and commit/tag, publishing matching release files, uploading four README videos,
reusing their attachment URLs in both languages, and checking hosted CI, inline playback and links.
No additional hardware verification is requested. Local preparation does not itself publish the project.
