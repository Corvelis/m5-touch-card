# Security and privacy / セキュリティと個人情報

M5 Touch Cardは開発版です。名刺を内部ストレージ・microSD・スマホに平文で保存します。
NFCには暗号化・暗号学的な相手認証はありません。「交換」の相手照合はセッション混同を防ぐもので、
身元の証明ではありません。必要な時だけ待機を開き、信頼できる相手と使用してください。
他人の名刺やバックアップを公開しないでください。Webへの名刺公開機能はありません。

This is development software. Cards are plaintext on the device, SD card and phone.
NFC is neither encrypted nor cryptographically authenticated. Exchange session checks prevent accidental session
mix-ups, not impersonation. Open receive/update mode only when needed and use it with trusted peers.
Do not publish other people's cards or backups. There is no built-in Web card publishing feature.

## 問題の報告 / Reporting

名刺データや秘密情報を含む問題は、公開Issueに詳細・ダンプを貼らないでください。
公開リポジトリでGitHubの非公開脆弱性報告が有効なら「Security → Report a vulnerability」を使用してください。
利用できない場合は、秘密情報を含めずに非公開の連絡先を問い合わせてください。
このファイルの追加だけで非公開報告の受付設定が有効になるわけではありません。

Do not put sensitive data, exploit details involving private cards, or dumps in public issues.
If private vulnerability reporting is enabled on GitHub, use **Security → Report a vulnerability**.
Otherwise, ask for a private contact channel without disclosing sensitive details.
Adding this file does not enable GitHub's reporting setting or promise a response deadline.
