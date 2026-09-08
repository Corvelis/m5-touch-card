# 再利用元 / Source provenance

コピー元: `paper-mono-nfc-image-transfer`

元リポジトリのHEAD: `17ab4ee38a3d6d3cbaf1822a49d623b5252af766`。
現行アプリの導入・使い方は[README](../README.md)を参照してください。

- `legacy/firmware/`: 旧NFC受信・画面・歩数などを移植参照用に保持。新ファームウェアの
  ビルド対象ではない。17画像・自動削除・マウント失敗時の自動フォーマットは採用しない。
- `legacy/protocol/`: v1の正本とベクトル。新プロトコルの正本とは区別。
- `legacy/mobile/`: 旧画面と説明書の参照。
- `mobile/`には既存の画像/JPEG/CRC処理とNFCネイティブ処理をコピー。旧Dart画面・
  ワークフローと回帰テストは残すが、`main.dart`は新しい`TouchCardApp`だけを起動する。
- RTCを新ファームウェアで再利用し、NVS名前空間を`touch_time`へ変更。
- 初期画像とそのLICENSEをfirmware/mobileそれぞれへコピー。

旧v1と現行v2は通信互換ではありません。参照コードは現行アプリの操作手順には使用しないでください。
