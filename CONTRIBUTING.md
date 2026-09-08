# Contributing / 開発への参加

不具合報告や変更提案は、日本語・Englishのどちらでも受け付けています。
対応機種・セットアップは[日本語README](README.md) / [English README](README.en.md)を参照してください。

## 変更と検査 / Changes and checks

- 変更は目的ごとに分け、再現手順・期待する動作・検査結果を添えてください。
- NFC・保存形式を変更する場合は `protocol/` の仕様・共通ベクトルと両端の実装を同時に更新してください。
- 名刺アイコン・日時用・全画面用の画像、保存済み名刺・言語・送信待ちを壊さないようにしてください。
- 日英両方と小さい画面を確認し、機種固有の変更で他の機種を変えないようにしてください。
- 実機未確認の内容は未確認と記載してください。ビルド成功をNFC実機確認の代わりにはしません。

Keep changes focused and include reproduction steps and test results. Protocol/storage changes need matching
specifications, vectors and implementations on both endpoints. Preserve separate images, saved cards, language
and pending transfers. Check both languages and small displays. Clearly distinguish automated checks from hardware tests.

```sh
node scripts/check_public_source.mjs --self-test
node --test scripts/check_public_source.test.mjs
node scripts/check_public_source.mjs
# After selecting/staging the files to commit:
node scripts/check_public_source.mjs --staged
```

Firmware/C++/Flutter検査は[ビルド・テスト](docs/development.ja.md) / [Build and test](docs/development.en.md)を参照してください。ネイティブ通信検査は
`node scripts/check_native_protocol.mjs`、PaperMono描画検査は[描画ツール](scripts/paper_preview/README.md)を参照してください。
CI checks four firmware environments, C++ tests, Flutter analysis/tests, and source hygiene.
Android/iOS builds, native Kotlin/Swift vectors and physical devices still require the documented local checks.

## 個人情報とライセンス / Privacy and licensing

実名の名刺・個人写真・SD内容・フラッシュダンプ・秘密鍵・署名設定をコミットやIssueへ添付しないでください。
再現データは架空の名前・`example.com`のURL等へ置き換え、スクリーンショットも確認してください。
新しい依存・フォント・画像には出典とライセンスを添え、既存の著作権表示を保持してください。

Do not attach real cards, personal photos, SD contents, flash dumps, secrets or signing configuration.
Use synthetic examples and redact screenshots/logs. Preserve existing copyright notices and document the source
and license of added dependencies or artwork. See [security guidance](SECURITY.md) for sensitive reports.
