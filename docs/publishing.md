# GitHub公開前 / Before publishing

## 日本語

この段階では公開・コミット・pushは実行していません。初回はv0.1.0の通常のGitHub Releaseを予定しています。
公開ソースに加え、機種別ファームウェアZIPとAndroidの署名済みAPKを配布します。
iPhoneはソースと開発者向けの手順のみで、IPA・TestFlightは初回に含めません。
[インストール案内](install.ja.md)と[リリース準備](../release/README.md)を併せて確認してください。

1. `node scripts/check_public_source.mjs --self-test` と `node scripts/check_public_source.mjs` を実行。
   後者はGit管理済み＋未追跡の公開候補を読むだけです。実データや鍵は出力しません。
   限定的なパターン検査なので、個人情報やあらゆる秘密値の検出を保証しません。
   検査器自体は `node --test scripts/check_public_source.test.mjs` で確認します。
   公開するファイルを選んでステージした後は、`node scripts/check_public_source.mjs --staged` も実行。
   ステージ済みの内容そのものを検査するため、作業ファイルだけ直してもコミットに秘密が残るケースを検出できます。
2. `git status --short`、公開予定ファイル一覧、staged diffを目視確認。
   既存履歴がある場合は履歴も確認。Gitの著者名・メールはコミットに残るので公開用設定を確認。
3. バックアップ、実機ダンプ、名刺データ、個人写真、ログ、署名証明書を含めない。
   README掲載用として利用者が提供・確認した `docs/media/` の写真と軽量化済み動画だけを例外として含めます。
   元動画・旧短縮版・編集用ファイルは `dist/` 等の公開対象外に残します。
   `.gitignore` はビルド成果物・署名・ローカル環境値を除外します。
   既に追跡したファイルにはignoreが遡及しないため、一覧の確認を省略しないでください。
4. 日本語/英語README、操作ガイド、LICENSE、第三者通知、元実装の履歴・フォント/画像ライセンスを保持。
   `migration/legacy` は旧通信の回帰検査でも参照しているため、現時点では削除しません。
   動画4本は日英README内で埋め込み再生する方針です。[メディアの公開手順](media/README.md)に沿って
   GitHubの添付URLへ差し替え、公開待ち案内を削除し、両言語の4プレイヤーを確認します。
   現在の相対MP4リンクは仮置きで、Releaseへの動画添付は必須にしません。
5. `.github/workflows/ci.yml` は4本体環境、C++コア、Flutter、公開候補チェックを実行します。
   権限はcontents読取のみ。自動デプロイ・リリース公開・署名・実機書き込みは行いません。
   GitHub上での動作はpush後に確認してください。
   Actionsは公式リポジトリの固定コミットを指定し、Flutterも検証済みのリビジョンを照合します。
   更新時は[GitHubの安全な利用ガイド](https://docs.github.com/en/actions/reference/security/secure-use)に沿って変更先を確認してください。
6. バイナリ配布は別工程。自分の署名設定・依存ライセンス・対応ソース/再リンク要件・
   実機チェック結果を確認し、ボードと言語が区別できるファイル名とハッシュを付けてください。
   フルフラッシュバックアップをリリースファイルに使わないでください。

リリース向け実機確認は利用者から完了報告を受けています。追加の実機確認は今回の作業には含めません。
ソース公開後も、同一機種ペアや網羅的なSD故障・電源断など、既知の確認範囲外は明記します。
不具合報告ではボード・言語・バージョン・再現手順のみをまず共有し、名刺本文・トークン・USB識別情報入りのログを公開しないでください。

### 公開時に決めること

- GitHubの所有者、リポジトリ名、Public/Private。アプリIDを所有者名に合わせて変える必要はありません。
- コミットの公開用名義とメール。設定値は勝手に変更しません。メールを公開したくない場合はGitHub側のnoreply設定を確認。
- 公開ソースと通常Releaseの配布候補を準備します。LICENSEは既存のMITと著作権表示を保持し、フォント・第三者コードは個別の通知を保持します。
- README等をGitHub側で自動生成せず空のリポジトリを作成し、ローカル内容との不要な競合を避けてください。
- 公開後にCI初回実行、デフォルトブランチ、非公開脆弱性報告の設定を確認。これらは今回まだ実行しません。

署名・データ領域を含むバイナリの自動添付、リリース作成、コミット・pushはこの整理作業の対象外です。
公開用ファイルの検査記録は[公開準備結果](publication-readiness.md)を参照してください。

## English

No commit, push or publication has been performed at this stage. Plan a normal v0.1.0 GitHub Release
with public source, model-specific firmware ZIPs and a release-signed Android APK.
iPhone remains source/developer-build only; no IPA or TestFlight initially.
See [installation](install.en.md) and [release preparation](../release/README.md).

1. Run `node scripts/check_public_source.mjs --self-test` and `node scripts/check_public_source.mjs`.
   This read-only guard scans tracked and non-ignored candidate files. It prints paths/reasons, not secret values.
   It is a limited heuristic, not a complete secrets or personal-data audit.
   Run its regression tests with `node --test scripts/check_public_source.test.mjs`.
   After staging the intended files, run `node scripts/check_public_source.mjs --staged` to inspect actual index bytes, not just working files.
2. Review `git status --short`, the candidate list and staged diff, plus any existing history.
   Check the public author name/email before committing. Ignore rules do not remove already tracked files.
3. Exclude flash dumps/backups, real cards, personal photos, logs and signing credentials.
   The user-provided and approved publication media in `docs/media/` are the explicit exception.
   Original recordings, previous shortened edits and private editing files remain outside source publication.
4. Preserve the bilingual guides, LICENSE, third-party notices and artwork/font licenses.
   Keep `migration/legacy`: current regression checks still use parts of it.
   Finalize all four inline videos in both READMEs using [GitHub attachment URLs](media/README.md), remove the upload-pending notice,
   and verify every player in both languages. Current relative MP4 links are temporary; Release video assets are not required.
5. The CI workflow only checks builds/tests with read-only repository permissions; it does not deploy, release, sign or flash.
   Verify its first GitHub run after pushing. Local checks do not prove a hosted workflow has run.
   Official Actions use full commit SHAs; the tested Flutter revision is checked after download.
6. Treat binary redistribution as a separate release step: review signing, dependency notices and corresponding-source/relinking requirements,
   retain the user-confirmed hardware acceptance record, label board/language variants and publish hashes. Never distribute full device backups.

Keep hardware limitations visible. Do not put card contents, editing keys or device identifiers in public bug reports.
Choose repository owner/name/visibility and public commit author/email before the first commit/push.
Start with an empty GitHub repository; preserve MIT attribution and separate font/dependency notices in source and release materials.
Repository creation, account settings, signing, releases and pushing are not part of local preparation.
See the [readiness record](publication-readiness.md) and [GitHub's secure-use guidance](https://docs.github.com/en/actions/reference/security/secure-use).
