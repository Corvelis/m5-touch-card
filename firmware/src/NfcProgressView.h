#pragma once
#include <UiLanguage.h>
#include <M5GFX.h>
#include <TransferProgress.h>
#include <cstdio>

namespace tc {
struct ProgressFonts {
  const lgfx::IFont *label, *number, *detail;
  int numberScale;
};
inline void drawNfcProgress(lgfx::LGFXBase &d, const TransferProgress &p,
                             bool paper, bool sender, bool phone,
                             const ProgressFonts &fonts,
                             const TransferProgress *previous = nullptr,
                             bool continuing = false) {
  const int x = paper ? 32 : 24, y = paper ? 280 : 82;
  const int w = d.width() - 2 * x, h = paper ? 292 : 112;
  // Preserve unchanged labels/hints between DATA acknowledgements. Besides
  // reducing the e-paper dirty area, this avoids rasterizing Japanese text
  // again while the NFC listener is waiting for its next command.
  const bool valuesOnly = previous && previous->stage == TransferStage::Transferring &&
      p.stage == previous->stage && p.total == previous->total;
  if (valuesOnly) {
    d.fillRect(x, y + (paper ? 56 : 24), w, paper ? 110 : 50, 0xFFFFFFu);
    d.fillRect(x, y + (paper ? 220 : 92), w, paper ? 32 : 20, 0xFFFFFFu);
  } else d.fillRect(x, y, w, h, 0xFFFFFFu);
  const char *label = tc::tr("タッチして待機", "Touch to connect");
  switch (p.stage) {
    case TransferStage::Waiting: break;
    case TransferStage::Connecting: label = tc::tr("接続しています", "Connecting"); break;
    case TransferStage::Transferring: label = sender ? tc::tr("送信中", "Sending") : tc::tr("受信中", "Receiving"); break;
    case TransferStage::Reconnecting: label = tc::tr("再接続しています", "Reconnecting"); break;
    case TransferStage::Saving: label = sender ? tc::tr("相手の保存を確認中", "Checking save") : tc::tr("保存しています", "Saving"); break;
    case TransferStage::Complete: label = continuing ? tc::tr("1枚目が完了しました", "First card complete") : sender ? tc::tr("渡しました", "Sent") : phone ? tc::tr("更新しました", "Updated") : tc::tr("受け取りました", "Received"); break;
    case TransferStage::Failed: label = tc::tr("通信エラー", "Transfer error"); break;
  }
  d.setTextColor(0x000000u, 0xFFFFFFu);
  d.setFont(fonts.label); d.setTextSize(1);
  if (!valuesOnly) d.drawString(label, x, y);
  char value[16] = "--";
  if (p.total || p.stage == TransferStage::Complete)
    snprintf(value, sizeof(value), "%u%%", p.percent());
  if (p.stage == TransferStage::Failed) snprintf(value, sizeof(value), "!");
  d.setFont(fonts.number); d.setTextSize(fonts.numberScale);
  d.drawString(value, x, y + (paper ? 56 : 24));
  const int barY = y + (paper ? 190 : 78), barH = paper ? 10 : 6;
  d.drawRect(x, barY, w, barH, 0x000000u);
  d.fillRect(x + 2, barY + 2, w - 4, barH - 4, 0xFFFFFFu);
  const int filled = (w - 4) * p.percent() / 100;
  if (filled > 0) d.fillRect(x + 2, barY + 2, filled, barH - 4, 0x000000u);
  d.setFont(fonts.detail); d.setTextSize(1);
  char detail[48];
  const char *hint = sender ? tc::tr("相手は「受け取る」に", "Peer: select Receive") : phone ? tc::tr("スマホを近づけてください", "Touch with your phone") : tc::tr("相手は「渡す」に", "Peer: select Send");
  if (p.total) {
    snprintf(detail, sizeof(detail), "%lu / %lu KB",
             (unsigned long)((std::min(p.transferred, p.total) + 1023) / 1024),
             (unsigned long)((p.total + 1023) / 1024));
    hint = detail;
  }
  if (p.stage == TransferStage::Complete) hint = continuing ? tc::tr("続けて交換します", "Continuing exchange") : tc::tr("離して大丈夫です", "You can separate now");
  if (p.stage == TransferStage::Failed) hint = tc::tr("もう一度お試しください", "Please try again");
  d.drawString(hint, x, y + (paper ? 220 : 92));
  if (!valuesOnly && paper && p.stage != TransferStage::Waiting && p.stage != TransferStage::Complete && p.stage != TransferStage::Failed)
    d.drawString(tc::tr("完了するまで離さないでください", "Keep touching until complete"), x, y + 260);
}
} // namespace tc
