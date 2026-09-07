# 掲載用メディア / README media

日本語・英語READMEで共用する、利用者提供の実機写真と動画です。

| 内容 / Content | 動画 / Video | 長さ / Duration |
| --- | --- | --- |
| 名刺交換 / Card exchange | [01-card-exchange.mp4](01-card-exchange.mp4) | 24.6秒 / s |
| スマホ更新 / Phone update | [02-phone-update.mp4](02-phone-update.mp4) | 31.0秒 / s |
| デザイン変更 / Card designs | [03-card-designs.mp4](03-card-designs.mp4) | 30.7秒 / s |
| 名刺帳 / Card book | [04-card-book.mp4](04-card-book.mp4) | 17.3秒 / s |

動画はノーカット・等速で、固定した画角調整と日英字幕のみを加えています。
元動画の全フレームと再生時刻が一致することを検査済みです。音声と撮影メタデータは除去し、H.264 MP4として軽量化しました。
写真 `devices.jpg` は撮影メタデータのみ除去済みで、画素・構図・色は変更していません。
`*-poster.jpg` は動画から取り出したサムネイルです。

**4本ともREADME内で埋め込み再生**する構成です。日本語・英語とも、動画ごとの見出しにGitHubの添付URLを配置しています。
添付先は[README動画の保管用Issue](https://github.com/Corvelis/m5-touch-card/issues/1)です。GitHubのMarkdown描画で、各URLが対応するファイル名の動画プレイヤーになることを確認しました。
動画4本は合計約21MBです。元の撮影ファイル・前回の短縮版・編集スクリプト・作業用ファイルは含めません。
動画本体と既存のサムネイルは保持しており、今回のREADME整理では削除・再編集していません。

## 動画を差し替える場合

1. 公開先のリポジトリと公開範囲を確定してから作業します。添付は選択した時点でアップロードされるため、READMEの保存前でも外部送信になります。
2. GitHub上のREADME編集欄に上表のMP4を1本ずつドラッグ＆ドロップし、アップロード完了を待ちます。
   ファイル一覧の「Upload files」やReleaseの配布ファイル欄への追加とは別の操作です。
3. 挿入された添付URLを、その動画の `BEGIN VIDEO: …` と `END VIDEO: …` コメントの間へ置き、仮の相対リンクを置き換えます。
   URLは前後に空行を入れて単独の行にし、画像記法・通常のリンク記法・コードブロックで囲みません。
   コメントの外へURLが重複して挿入されていれば取り除きます。
4. 英語READMEの同じ動画欄にも同じ添付URLを使います。8回アップロードする必要はありません。
   編集欄で発行されたURLをそのまま使い、動画の配信先へ転送された後の一時URLはコピーしません。
5. 古い動画URLや仮リンクが残っていないか確認します。現在のREADMEに公開待ち案内はありません。
6. GitHubのプレビューと保存後のREADMEで、日英それぞれ4本のプレイヤー表示・再生を確認します。
   Public公開後はログアウト状態でも確認し、単なるダウンロードリンクや存在しないURLになっていないことを確認します。
7. GitHub上でREADMEを編集した場合は、その変更をローカルにも取り込んでから次のpushを行います。
   実際の再生確認後に[公開準備結果](../publication-readiness.md)と[リリースのチェック項目](../../release/README.md)を完了へ更新します。

動画のRelease添付は埋め込み再生の前提にしません。Releaseの配布物はAPK・ファームウェア・対応ソース・チェックサムを中心にします。
添付URLの発行とGitHubのMarkdown描画によるプレイヤー生成は確認済みです。ブラウザでの再生操作確認は、この描画確認と区別して記録します。
仕様: [READMEへの動画添付](https://github.blog/changelog/2021-05-13-video-uploads-now-generally-available/) / [添付方法・容量・形式](https://docs.github.com/en/get-started/writing-on-github/working-with-advanced-formatting/attaching-files)。

スマホの動画はiPhone開発版で撮影しています。初回リリースはiPhoneのIPA・TestFlight配布を含みません。
撮影は2026年9月6日・7日の開発版で、赤いLEDなど一部の外観がリリース版と異なる場合があります。
ソフトウェアのMITライセンスが、画面内の第三者のアイコンや商標にまで適用されることを意味しません。

## English

These user-provided photos and hardware recordings are shared by both READMEs.
The four videos retain every original frame and presentation timestamp, with a fixed crop and bilingual captions; there are no cuts, speed changes or added freeze frames.
Audio and capture metadata were removed. Each H.264 MP4 is below 10MB; the four videos total about 21MB.
The hero photo retains its original pixels and only has capture metadata removed. Posters are actual video frames.

All four videos are embedded in both READMEs using GitHub attachment URLs, each under its own heading rather than in a thumbnail table.
The [media archive issue](https://github.com/Corvelis/m5-touch-card/issues/1) retains the attachments. GitHub's Markdown renderer generates a video player with the matching filename for each URL.
The exports and existing posters are retained unchanged. Original recordings, previous shortened edits and private editing files are not included.

To replace a video, confirm the destination and visibility, then drop the new MP4 into GitHub's README editor and wait for its attachment URL.
Attaching a file uploads it immediately, even before saving the README. This is different from adding files to the repository or Release assets.
Replace the fallback between the matching `BEGIN VIDEO: …` / `END VIDEO: …` comments with that URL on its own line, with blank lines around it.
Do not wrap it in image/link syntax or a code block, and remove any duplicate copy inserted elsewhere by the editor.
Reuse the same four URLs in the English README. Keep the editor-generated attachment URLs, not temporary redirected media URLs.
Check for obsolete URLs or temporary links after updating both READMEs. The upload-pending notices have already been removed.
Check all four players in each language in GitHub's preview and saved README, including logged-out access after public publication.
Sync web edits back locally before pushing again, then update the readiness record and release checklist.
Release attachments are not required. Uploads and GitHub player rendering are confirmed; browser playback checks are recorded separately from rendering checks.

The iPhone demo uses a development build; initial iOS distribution is source-build only.
Footage was recorded on September 6–7, 2026; some visual details, such as the red LED, may differ from the release version.
The software's MIT license does not automatically license third-party avatars or trademarks visible in the footage.
