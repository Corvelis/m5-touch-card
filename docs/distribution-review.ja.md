# 配布条件の確認 — v0.1.0

[日本語](distribution-review.ja.md) · [English](distribution-review.en.md)

2026-09-07。対象は通常ReleaseのAndroid APKとPaperMono / StackChanファームウェアです。
iPhoneはソースのみで、IPA・TestFlightは対象外です。これは実装・配布物の技術的な照合記録で、法的な適合性の保証ではありません。

## 確認結果

| 対象 | 確認と対応 | 公開前に残るもの |
| --- | --- | --- |
| Androidのネイティブ依存 | 実際のrelease classpathの44個を固定版POM・AAR/JARと照合。Apache-2.0全文、収録NOTICE、ReLinkerの著作権表示を既存ライセンス画面へ追加。新APK内の収録・署名を確認 | 同梱通知を公開物で維持（実機確認は利用者報告で完了） |
| Flutter / Dart | 固定したFlutter/engineのSky Engine LICENSEと自動生成NOTICESを維持。mimeのHTTPD通知も新APK内で確認 | 同梱通知を公開物で維持（実機確認は利用者報告で完了） |
| 自作画像・TouchSansJP | MITの初期画像、OFL 1.1のフォント全文と変換スクリプトを維持 | ソース公開と一緒に保持 |
| /efont/日本語フォント | `efontJA_16` を維持。BSD系の `COPYRIGHT.txt` の収集漏れを修正。標準Font0のBSD通知も追加 | 同梱を公開物で維持 |
| FreeSans英数字フォント | 通常/太字をOFLのTouch Digitsに置換。4構成でGNU FreeFont由来のリンク済みシンボルがないことを検査 | 対応するソース・フォント通知を公開物で維持（実機確認は利用者報告で完了） |
| Arduino-ESP32 | 実使用版 `3.20017.241212+sha.dcc1105b` を明示固定。LGPL本文、実使用のライブラリと自作部分のソース・再リンク用の資料をZIP化 | 最終ファームウェアと同じReleaseへ添付 |
| ESP-IDF・コンパイラーのランタイム | IDF 4.4.7、newlib・GCC例外、LittleFS・TLSF、画像デコーダー等の追加通知18ファイルを確認。mbedTLSはApache-2.0、wpa_supplicantはBSD側を使用 | 同梱した原文・由来・ハッシュを公開物で維持 |

Androidの一覧とアーカイブのSHA-256は[依存の照合情報](../release/android-dependencies.json)、
実際の通知全文は[同梱ライセンス](../mobile/licenses/ANDROID-NATIVE-NOTICES.txt)にあります。
ライブラリ名・版・ハッシュだけの一覧は、ライセンス全文やNOTICEの代用ではありません。
SDK/画像デコーダー等は固定版の原文に加え、使用SDKとM5GFX/NFC/M5Utilityのソース内の
著作権・ライセンスコメント925種類も全文収録しています。未使用のものを含む通知の集合で、
全ての部品が完成ファームウェアにリンクされているという意味ではありません。
LittleFSラッパーはSDKヘッダーの1.14.1と上流固定版のファイルがバイト単位で一致し、
そこが指定するlittlefsコミット `f53a0cc961a8acac85f868b431d2f3e58e447ba3` のBSD全文を保持しました。
通知の欠落・ハッシュ不一致があると配布ZIP作成は停止します。

## MITで公開する範囲

利用者の指定により、**M5 Touch Cardの自作コードはMIT**で公開する方針です。
第三者のフォント、Arduino等をMITへ変更する意味ではありません。
OFL/BSD/Apache/LGPL等の原文は残し、対応ソースの提供を含む各条件を満たす形で配布します。

## フォントの置き換え

M5GFXのライセンスはMITですが、組み込む個別フォントの元の条件までMITになるわけではありません。
[Adafruitの公式説明](https://learn.adafruit.com/adafruit-gfx-graphics-library/using-fonts)はFreeSans等の元をGNU FreeFontとしています。
[GNU FreeFontの原文](https://www.gnu.org/software/freefont/license.html)ではGPL-3.0-or-laterと文書への埋め込み例外が示されています。
名刺を描くアプリのファームウェアそのものを「文書」と扱ってよいとは、ここでは判断しません。

承認を得て、時計と進行率の数字を既存TouchSansJP由来のTouch Digitsへ置き換えました。
48px、必要な数字・記号だけの通常/1px太字ビットマップで、ファームウェアへの追加は約3 KiBです。
名刺の配置、日本語フォント、PaperMonoの描画方式は変えていません。
`scripts/build_stack_digits.cpp` で再生成でき、OFL全文と著作権表示を保持します。
時計の幅、0〜100%とエラー記号、部分更新後の残像をホスト描画で検査しました。
配布作成時にELFを検査し、FreeSans/FreeMono/FreeSerifが再びリンクされた場合は停止します。

## LGPLの対応ソースと再リンク

実使用のArduinoコミットは `dcc1105b0cf1322a437b354c336f2abf72b7e512` です。
[その版のライセンス](https://github.com/espressif/arduino-esp32/blob/dcc1105b0cf1322a437b354c336f2abf72b7e512/LICENSE.md)に沿い、
ライブラリの対応ソースと、改変したライブラリで再リンクできるM5 Touch Card側のソース・ビルド手順を提供する方式を使います。
独自に3年間の書面提供義務を約束する方式や、改変禁止・逆解析禁止の追加制限は設けません。

`touch-card-v0.1.0-firmware-sources.zip` に、自作ファームウェア全体、実際に使用したArduinoの
core/libraries/variants、ESP32-S3のヘッダー・設定・リンカースクリプト、修正スクリプトと導入手順を収録します。
SDKバイナリ・コンパイラー等は固定版PlatformIOパッケージから取得する構成です。
全ツール同梱の完全オフラインビルドではありません。
各ソース・外部SDK入力をSHA-256で対応づけ、改変用の新しいコピーを作る補助を同梱します。
詳細は[対応ソース・再リンク手順](../release/corresponding-source.md)を参照してください。
このZIPをファームウェアと同じReleaseへ添付し、アプリの公開コミット/タグも確定します。
上流のリンクやライセンス全文だけを置いて「対応ソース提供済み」とは扱いません。
ソース一式からの再ビルド/再リンクを検査し、配布ZIPのバイナリと対応づけます。
ローカルではZIPを展開し、復元したArduinoの `millis()` に検証用の命令を追加してStackChanを再ビルドしました。
完成ELFの逆アセンブルでもその変更を確認しています。検証用の変更は配布ソース・配布バイナリには含めず、実機にも書き込んでいません。

## 実機と公開の状態

Xiaomi / Android 16への署名済みAPKのインストールと起動は2026-09-07に成功しています。
以前の端末側インストール制限は解消済みで、アプリやデータの削除は行っていません。
利用者からリリースに向けた実機確認は完了済みと報告されているため、Android・本体の実機項目を再実施待ちにはしません。
これは利用者の確認報告による受け入れ完了であり、今回の文書整理で新しく実機試験や書き込みを実行したという意味ではありません。
同一機種ペアや網羅的なSD故障・電源断試験は、既知の確認範囲外として残します。

公開先・コミット・タグと、ソースZIPを含めた対応する配布物の同時公開は未実施です。
このため候補は `publish_ready: false` のままです。残項目はGitHubでの公開工程であり、実機の再確認待ちではありません。
