# M5 Touch Card mobile

FlutterのAndroid/iPhone共通アプリ。入口は `lib/main.dart` → `TouchCardApp`。

配布版の導入は[日本語インストール手順](../docs/install.ja.md) / [English](../docs/install.en.md)を参照。
初回はAndroid APKを配布し、iPhoneはソースビルドのみとします。IPA・TestFlightは初回配布の対象外です。

## 操作

- 上部の地球アイコン: 下に開くメニューから日本語 / Englishを選択。即時反映し、再起動後も保持。
  初期値は日本語。ビルドに `--dart-define=TOUCH_CARD_DEFAULT_LANGUAGE=en` を追加すると初期値が英語。
  保存済みの選択を優先し、本体の言語設定や名刺本文は変更しません。
  OSの権限ダイアログはiOS/Android側の言語設定に従います。

- 「名刺」タブ: 名前だけ必須。アイコンと追加情報は任意。一言は40 Unicode scalarまで。
  「変更を保存」はスマホ内の下書き保存。「NFCで名刺を更新」は本体へ名刺とアイコンを送信。
  追加メニューから本文だけ・アイコンだけの更新と初期アイコンへのリセットができます。
- 「画像」タブ: 日時・カレンダー用 / 全画面用を選び、送り先の機種を指定して切り抜き。
  「画像を送る端末」を変えると、表示中の用途の元画像から新しい寸法へ自動で中央切り抜きして保存します。
  元画像と送信用JPEGは用途別に保持し、他の用途・名刺アイコンは変更しません。構図は「保存した元画像から切り抜き直す」で調整できます。
  用途を切り替えると、その画像の保存済み端末を選択表示します。変換・保存失敗時は前の端末・画像を維持します。
- 上部のiボタン: ライセンス一覧を直接表示します。
- 上部の時計ボタン: NFCでUTC時刻とスマホのUTCオフセットを送信。
- 本体はどの更新でも「設定 → スマホから更新」。受信する項目を本体で選ぶ必要はありません。
- 送信待ちはスマホ再起動後も残り、同じ内容・IDで再試行できます。
  別の内容へ置き換える場合は確認します。100%送信しただけでなく本体の保存通知を待ちます。

名刺アイコン、日時用画像、全画面画像は相互に上書きしません。カラーのBaseline JPEGを
作り、Paper Mono側でグレー表示します。NFCはM5 Touch Card v2専用で、旧画像転送アプリとは別IDです。

## 検査

```sh
flutter analyze
flutter test
flutter build apk --debug
flutter build ios --simulator --no-codesign
```

iOS実機はNFC対応iPhoneと開発者自身のTeam/署名が必要です。シミュレーターではNFC通信できません。
今回の言語変更はテスト・ビルドまでです。新しい版の実機NFC・権限画面の確認は別途必要です。

English instructions: [project README](../README.en.md) and [operation guide](../docs/operation.en.md).

旧Dart画面・v1プロトコルは回帰用に残していますが、新アプリの入口から旧送信画面は呼びません。
Android/iOSのネイティブ通信はv2に更新済みです。再利用元の説明書は
`migration/legacy/mobile/README.md`を参照。
