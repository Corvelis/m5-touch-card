#include <TransferProgress.h>
#include <cassert>
#include <iostream>
using namespace tc;
int main() {
  TransferProgress p;
  assert(p.stage == TransferStage::Waiting && p.percent() == 0);
  p = {TransferStage::Transferring, 1481, 14815};
  assert(p.percent() == 9);
  p.transferred = 14815;
  assert(p.percent() == 100 && p.stage != TransferStage::Complete);
  p = {TransferStage::Saving, 14815, 14815};
  assert(p.percent() == 100 && p.stage != TransferStage::Complete);
  p = {TransferStage::Transferring, UINT32_MAX, UINT32_MAX};
  assert(p.percent() == 100);
  p.total = 1;
  assert(p.percent() == 100);
  p = {TransferStage::Complete, 0, 0};
  assert(p.percent() == 100);

  ProgressRefresh gate;
  TransferProgress idle;
  assert(gate.needed(idle, 0, true, false));
  gate.record(idle, 0);
  assert(!gate.needed(idle, 10000, true, false)); // no idle animation
  TransferProgress moving{TransferStage::Transferring, 1, 100};
  assert(gate.needed(moving, 1, true, false)); // start promptly
  gate.record(moving, 1);
  moving.transferred = 19;
  assert(!gate.needed(moving, 1000, true, false));
  moving.transferred = 21;
  assert(!gate.needed(moving, 200, true, false));
  assert(gate.needed(moving, 501, true, false));
  assert(!gate.needed(moving, 501, true, true)); // never wait for panel
  gate.record(moving, 501);
  TransferProgress saving{TransferStage::Saving, 100, 100};
  assert(gate.needed(saving, 502, true, false));
  gate.record(saving, 502);
  assert(!gate.completionVisible(5000)); // 100% is not durable success
  assert(gate.needed(p, 503, true, false));
  gate.record(p, 503);
  assert(!gate.completionVisible(1102));
  assert(gate.completionVisible(1103));
  gate.reset();
  assert(!gate.completionVisible(9999));
  gate.record({TransferStage::Transferring, 0, 100}, 0);
  assert(gate.needed({TransferStage::Transferring, 5, 100}, 120, false, false));
  assert(!gate.needed({TransferStage::Transferring, 5, 100}, 119, false, false));
  gate.record(p, UINT32_MAX - 100);
  assert(gate.completionVisible(500)); // millis wrap
  std::cout << "transfer percentage, throttle, busy panel and completion hold: PASS\n";
}
