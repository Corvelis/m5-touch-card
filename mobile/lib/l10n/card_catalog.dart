import 'package:flutter/widgets.dart';

// Domain errors stay locale-independent in storage/protocol code.
String cardNotice(BuildContext context, String message) {
  if (Localizations.localeOf(context).languageCode != 'en') return message;
  final key = message.replaceFirst(
    RegExp(r'^(FormatException|Bad state): '),
    '',
  );
  return _english[key] ?? message;
}

// Translate UI keys only; card data is never passed through this catalog.
String cardNfcError(BuildContext context, String? code, {String? fallback}) {
  final english = Localizations.localeOf(context).languageCode == 'en';
  String text(String ja, String en) => english ? en : ja;
  final normalized = code
      ?.replaceAllMapped(RegExp(r'([a-z])([A-Z])'), (m) => '${m[1]}_${m[2]}')
      .toUpperCase();
  return switch (normalized) {
    'NFC_UNAVAILABLE' => text(
      'この端末ではNFCを利用できません。',
      'NFC is unavailable on this phone.',
    ),
    'NFC_DISABLED' => text('スマホのNFCを有効にしてください。', 'Enable NFC on your phone.'),
    'TAG_LOST' => text(
      '接続が切れました。もう一度本体へ当ててください。',
      'Connection lost. Hold your phone near the device again.',
    ),
    'COMMIT_TIMEOUT' => text(
      '本体の保存確認がタイムアウトしました。',
      'Timed out while waiting for the device to save.',
    ),
    'CRC_MISMATCH' => text(
      '本体でデータのCRCが一致しませんでした。',
      'The data checksum did not match on the device.',
    ),
    'CANCELLED' => text('送信を中止しました。', 'Transfer cancelled.'),
    'TRANSFER_IN_PROGRESS' => text(
      '別のNFC操作が進行中です。先に中止してください。',
      'Another NFC operation is active. Cancel it first.',
    ),
    'INVALID_OFFSET' || 'TRANSFER_STALLED' || 'TRANSFER_ID_MISMATCH' => text(
      '本体の受信位置または転送IDが不正です。再試行してください。',
      'Invalid transfer position or ID. Please retry.',
    ),
    'INCOMPATIBLE_LIMITS' ||
    'UNSAFE_HELLO' ||
    'TIME_SYNC_UNSUPPORTED' ||
    'TRANSCEIVE_LIMIT_TOO_SMALL' => text(
      '本体またはスマホのNFC機能に互換性がありません。',
      'The device or phone has incompatible NFC capabilities.',
    ),
    'NFC_SESSION_UNAVAILABLE' ||
    'NFC_RESTART_FAILED' ||
    'SESSION_INVALIDATED' => text(
      'NFCを開始できませんでした。再試行してください。',
      'Could not start NFC. Please retry.',
    ),
    _ =>
      english
          ? 'Transfer failed${code == null ? '' : ' ($code)'}. Please retry.'
          : (fallback ?? '通信に失敗しました。再試行してください。'),
  };
}

String cardLabel(BuildContext context, String japanese) =>
    Localizations.localeOf(context).languageCode == 'en'
    ? (_english[japanese] ?? japanese)
    : japanese;

const _english = <String, String>{
  "名刺アイコン": "Card icon",
  "日時・カレンダー用画像": "Clock & calendar image",
  "全画面画像": "Full-screen image",
  "名刺アイコンを選んでください": "Choose a card icon",
  "画像を読み込めません": "Could not read the image",
  "元画像が大きすぎます": "The original image is too large",
  "画像を検証できません": "Could not validate the image",
  "画像を転送可能なサイズにできません": "Could not make the image small enough to transfer",
  "送信データの上限または用途が不正です": "Invalid transfer size or target",
  "送信の用途が一致しません": "Transfer targets do not match",
  "送信待ちデータが破損しています": "The pending transfer is damaged",
  "先に画像を選んで保存してください": "Choose and save an image first",
  "画像のデータ量が上限を超えています": "The image exceeds the size limit",
  "保存画像の形式が不正です": "Invalid saved image format",
  "交換用アイコンの形式が不正です": "Invalid exchange icon format",
  "保存データを読み込めません。上書きせず保護しています。":
      "Could not read saved data. It is protected from being overwritten.",
  "保存世代の上限です": "The revision limit has been reached",
  "名刺IDまたは更新番号が不正です": "Invalid card ID or revision",
  "名前を入力してください": "Enter your name",
  "文字数または使用できない文字を確認してください": "Check the text length and unsupported characters",
  "QR用URLはUTF-8で512バイトまでです": "The QR URL must not exceed 512 UTF-8 bytes",
  "QR用URLには http または https のURLを入力してください":
      "Enter an http or https URL for the QR code",
  "このスマホに保存しました": "Saved on this phone",
  "この用途の画像だけを保存しました": "Saved the image for this use only",
  "送信待ちを置き換えますか？": "Replace pending transfer?",
  "キャンセル": "Cancel",
  "置き換える": "Replace",
  "スマホのアイコンを初期画像に戻しました。NFC更新で本体へ反映します。":
      "The default icon is restored on this phone. Update the device via NFC.",
  "交換用アイコンを保存しました。NFCで名刺を更新すると本体に反映されます。":
      "Exchange icon saved. Update your card via NFC to apply it to the device.",
  "交換用アイコン": "Exchange icon",
  "本体表示": "On your device",
  "相手に渡す画像": "Shared image",
  "本体表示はそのままに、交換用だけ160 × 160に縮小できます。":
      "Resize only the shared icon to 160 × 160. The icon on your device stays unchanged.",
  "交換用は160 × 160・6 KB以内です。": "The shared icon is 160 × 160 and under 6 KB.",
  "交換用は160 × 160です。画質を保つため6 KBを超えています。":
      "The shared icon is 160 × 160 and exceeds 6 KB to preserve quality.",
  "交換用を160pxにする": "Resize shared icon to 160 px",
  "あなたの名前": "Your name",
  "アイコンを変更": "Change icon",
  "アイコンを切り抜き直す": "Crop icon again",
  "名前": "Name",
  "追加情報（任意）": "Optional details",
  "アカウント名": "Account",
  "メールアドレス": "Email address",
  "QR用URL": "QR link",
  "一言コメント（40文字まで）": "Short note (up to 40 characters)",
  "変更を保存": "Save changes",
  "スマホに保存": "Save on phone",
  "NFCで名刺を更新": "Update card via NFC",
  "更新する内容": "What to update",
  "内容だけ更新": "Update details only",
  "アイコンだけ更新": "Update icon only",
  "アイコンを初期画像に戻す": "Restore default icon",
  "日時・カレンダー": "Clock & calendar",
  "全画面": "Full screen",
  "画像を送る端末": "Send image to",
  "ｽﾀｯｸﾁｬﾝ": "StackChan",
  "初期画像": "Default image",
  "画像を選んで切り抜く": "Choose and crop image",
  "保存した元画像から切り抜き直す": "Crop again from the original",
  "この画像をNFCで送る": "Send this image via NFC",
  "自分の名刺": "My card",
  "ホーム画像": "Home images",
  "日時を合わせる": "Sync clock",
  "タップして同じ内容で再開": "Tap to resume the same transfer",
  "名刺": "Card",
  "画像": "Images",
  "切り抜き": "Crop",
  "この範囲を保存": "Save crop",
  "切り抜きに失敗しました": "Could not crop the image",
  "本体の「設定 → スマホから更新」を開いてタッチしてください":
      "Open Settings → Phone update on the device, then hold your phone near it.",
  "通信時間を超えました。再試行できます。": "The transfer timed out. You can retry.",
  "本体に保存しました": "Saved on the device",
  "本体に保存しました。スマホの送信履歴の更新に失敗しました。":
      "Saved on the device, but could not update the transfer history on this phone.",
  "保存容量が不足しています。名刺を削除するかSDを確認してください。":
      "Not enough space. Delete cards or check the SD card.",
  "保存先のSDが取り外されました。": "The destination SD card was removed.",
  "名刺の更新世代が競合しています。内容を保存し直してください。":
      "Card revisions conflict. Save your changes again.",
  "本体の「設定 → スマホから更新」を開いてください。": "Open Settings → Phone update on the device.",
  "名刺または画像の形式を確認してください。": "Check the card or image format.",
  "本体への保存に失敗しました。ストレージを確認してください。":
      "Could not save to the device. Check its storage.",
  "通信に失敗しました。再試行してください。": "Transfer failed. Please retry.",
  "送信しています": "Sending",
  "本体への保存を確認しています": "Verifying the saved data",
  "日時を合わせています": "Syncing the clock",
  "同じ内容で再試行": "Retry the same transfer",
  "閉じる": "Close",
  "中止する": "Cancel",
  "閉じる（送信待ちは保持）": "Close (keep pending transfer)",
  "アイコン": "Icon",
  "メール": "Email",
  "一言コメント": "Short note",
  "読み込めません": "Could not load",
};
