#include <cassert>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>

// Source-contract regression, not a hardware simulation: the driver owns mode
// configuration inside begin(). An extra explicit configure call caused the
// reader's second initial_field_on() to fail with TX already enabled.
int main(int argc, char **argv) {
  assert(argc == 3);
  std::ifstream file(argv[1]);
  assert(file);
  std::string source((std::istreambuf_iterator<char>(file)), {});
  auto start = source.find("bool initialize(bool target)");
  auto end = source.find("void sendTick()", start);
  assert(start != std::string::npos && end != std::string::npos);
  auto body = source.substr(start, end - start);
  auto role = body.find("config.emulation = target;");
  auto config = body.find("unit.config(config);");
  auto first = body.find("units.begin()");
  auto again = body.find("unit.begin()");
  assert(role < config && config < first && config < again);
  assert(first != std::string::npos && again != std::string::npos);
  assert(body.find("units.begin()", first + 1) == std::string::npos);
  assert(body.find("unit.begin()", again + 1) == std::string::npos);
  assert(body.find("configureNFCMode(") == std::string::npos);
  assert(body.find("configureEmulationMode(") == std::string::npos);
  assert(body.find("writeExternalFieldDetectorActivationThreshold(0x11)") != std::string::npos);
  assert(body.find("writeExternalFieldDetectorDeactivationThreshold(0)") != std::string::npos);
  assert(source.find("reader.detect(found, 10U)") != std::string::npos);
  assert(source.find("reader.detect(found)") == std::string::npos);
  assert(source.find("recoverField(\"command_retries\")") != std::string::npos);
  assert(source.find("recoverField(\"discovery_timeout\")") != std::string::npos);
  assert(source.find("recoverField(\"reactivate_failed\")") != std::string::npos);
  assert(source.find("tc::cardResponseTimeout(sendPhase)") != std::string::npos);
  assert(source.find("tc::waitForCardSave(millis() - verifyingSince)") != std::string::npos);
  assert(source.find("sendPhase == 4 && (status == tc::Receiving || status == tc::BadOffset)") != std::string::npos);
  auto recovery = source.substr(source.find("if (fieldRecovery == 1)"),
                                source.find("if (millis() - lastCommand") -
                                source.find("if (fieldRecovery == 1)"));
  assert(recovery.find("unit.enableField()") != std::string::npos);
  assert(recovery.find("delay(") == std::string::npos);
  std::ifstream mainFile(argv[2]);
  assert(mainFile);
  std::string mainSource((std::istreambuf_iterator<char>(mainFile)), {});
  assert(mainSource.find("rows({tc::tr(\"交換\", \"Exchange\"), tc::tr(\"渡す\", \"Send\"), tc::tr(\"受け取る\", \"Receive\"), tc::tr(\"戻る\", \"Back\")})") != std::string::npos);
  assert(mainSource.find("heading(tc::tr(\"交換\", \"Exchange\"))") != std::string::npos);
  assert(mainSource.find("相互交換") == std::string::npos);
  assert(source.find("相互交換") == std::string::npos);
  auto loop = mainSource.substr(mainSource.find("void loop()"));
  const auto setupStart = mainSource.find("void setup()");
  assert(setupStart != std::string::npos);
  const auto setup = mainSource.substr(setupStart, mainSource.find("void loop()") - setupStart);
  const auto noClear = setup.find("config.clear_display = false;");
  assert(noClear != std::string::npos && noClear < setup.find("M5.begin(config);"));
  const auto paperGuard = setup.rfind("#if TOUCH_CARD_PAPER_MONO", noClear);
  assert(paperGuard != std::string::npos);
  assert(setup.substr(paperGuard, noClear - paperGuard).find("#endif") == std::string::npos);
  assert(setup.find("config.clear_display = true") == std::string::npos);
  assert(setup.find(".clear(") == std::string::npos);
  assert(setup.find(".fillScreen(") == std::string::npos);
  const auto firstRender = setup.find("render();");
  assert(firstRender != std::string::npos && firstRender > setup.find("storage.begin();"));
  assert(setup.find("render();", firstRender + 1) == std::string::npos);
  assert(setup.find("M5.Display.waitDisplay();") > firstRender);
  assert(setup.find("[tc.boot] first_frame_ms=%lu setup_ms=%lu") != std::string::npos);
  // Do not bypass the clean first frame just to hide initialization flicker.
  assert(setup.find("displayInitialized = true") == std::string::npos);
  assert(mainSource.find("config.pmic_button = true;") != std::string::npos);
  assert(mainSource.find("tc::protectPaperPowerButton(") != std::string::npos);
  const auto redLed = mainSource.find("const bool redLedOff = tc::turnOffPaperRedLed(");
  assert(redLed > mainSource.find("M5.begin(config);"));
  assert(redLed < mainSource.find("tc::protectPaperPowerButton("));
  assert(mainSource.substr(mainSource.rfind("#if TOUCH_CARD_PAPER_MONO", redLed),
                           redLed - mainSource.rfind("#if TOUCH_CARD_PAPER_MONO", redLed)).find("#endif") == std::string::npos);
  assert(mainSource.find("settings.getUChar(\"language\", 255)") != std::string::npos);
  assert(mainSource.find("settings.putUChar(\"language\", row)") < mainSource.find("tc::uiLanguage = tc::resolveLanguage(row)"));
  auto toggle = loop.substr(loop.find("bool displayToggled"), loop.find("bool scanning") - loop.find("bool displayToggled"));
  assert(toggle.find("M5.BtnPWR.wasClicked()") != std::string::npos);
  assert(toggle.find("screenBlanker.toggle") != std::string::npos);
  assert(toggle.find("paper() ? M5.BtnB.wasClicked() : M5.BtnPWR.wasClicked()") != std::string::npos);
  assert(toggle.find("paper() && !screenBlanker.off") < toggle.find("paperFooterCache.restore(M5.Display)"));
  assert(toggle.find("render(") == std::string::npos);
  assert(toggle.find("epd_quality") == std::string::npos);
  assert(toggle.find("paper() || screenBlanker.acceptsInput") == std::string::npos);
  assert(toggle.find("const bool allowInput = screenBlanker.acceptsInput(") != std::string::npos);
  assert(toggle.find("screenBlanker.waitForRelease && M5.Display.displayBusy()") != std::string::npos);
  assert(toggle.find("M5.Display.setBrightness(brightness)") < toggle.find("paperFooterCache.restore(M5.Display)"));
  assert(toggle.find("tc::hidePaperFooter(true, screenBlanker.off, screen, home)") != std::string::npos);
  assert(toggle.find("tc::clearSleepingPaperFooter(M5.Display, true, screen, home)") != std::string::npos);
  assert(toggle.find("M5.Display.setEpdMode(epd_mode_t::epd_fastest)") != std::string::npos);
  assert(mainSource.find("tc::clearSleepingPaperFooter(d, screenBlanker.off, screen, home)") != std::string::npos);
  assert(mainSource.find("paperFooterCache.capture(d, screen, home)") < mainSource.find("tc::clearSleepingPaperFooter(d, screenBlanker.off, screen, home)"));
  assert(mainSource.find("tc::paperRefresh(displayInitialized, screen)") != std::string::npos);
  assert(loop.find("if ((paper() || !nfc.active) && millis() - lastClock >= 1000)") != std::string::npos);
  assert(loop.find("tc::minuteStatusDue(") != std::string::npos);
  assert(loop.find("if (!paper() && (screen == Screen::Month") != std::string::npos);
  assert(loop.find("if (day != lastDay && !screenBlanker.off)") != std::string::npos);
  assert(toggle.find("return") == std::string::npos);
  assert(toggle.find("navigate(") == std::string::npos);
  assert(toggle.find("nfc.stop") == std::string::npos);
  assert(loop.find("if (allowInput && M5.Touch.getCount()") != std::string::npos);
  assert(loop.find("tap(x, y)") < loop.find("nfc.tick();"));
  const auto buttonStart = loop.find("if (!paper() && allowInput && M5.BtnB.wasClicked())");
  assert(buttonStart != std::string::npos && buttonStart < loop.find("nfc.tick();"));
  const auto stackButton = loop.substr(buttonStart, loop.find("nfc.tick();") - buttonStart);
  assert(stackButton.find("cancelNfc()") != std::string::npos);
  assert(stackButton.find("activate(focus)") != std::string::npos);
  assert(stackButton.find("setBrightness") == std::string::npos);
  assert(mainSource.find("footer(tc::tr(\"ホームへ戻る\", \"Home\"), \"\")") != std::string::npos);
  const auto tapStart = mainSource.find("void tap(int x, int y)");
  const auto previewTap = mainSource.find("if (screen == Screen::Preview)", tapStart);
  const auto cardTap = mainSource.find("if (screen == Screen::Card)", previewTap);
  assert(previewTap != std::string::npos && cardTap != std::string::npos);
  const auto previewAction = mainSource.substr(previewTap, cardTap - previewTap);
  assert(previewAction.find("navigate(bottom && !left ? Screen::Menu : Screen::Home)") != std::string::npos);
  assert(previewAction.find("Screen::Settings") == std::string::npos);
  assert(mainSource.find("screen == Screen::Preview ? tc::tr(\"設定\", \"Settings\")") == std::string::npos);
  assert(mainSource.find("footer(\"\", tc::tr(\"メニュー\", \"Menu\"))") != std::string::npos);
  const auto menuHome = mainSource.find("tc::menuHomeRect(w, h).contains(x, y)", tapStart);
  assert(menuHome != std::string::npos);
  assert(menuHome < mainSource.find("tc::menuRowAt(", tapStart));
  assert(mainSource.substr(menuHome, 105).find("navigate(Screen::Home)") != std::string::npos);
  assert(loop.find("nfc.tick();") < loop.find("transferProgress();"));
  assert(loop.find("nfcProgressRefresh.completionVisible(millis())") != std::string::npos);
  assert(loop.find("mutual.completeLeg(millis())") != std::string::npos);
  assert(loop.find("mutual.readyForNext(millis())") != std::string::npos);
  assert(mainSource.find("if (!mutualMode || mutual.leg == 1) cards.lastReceivedId = \"\";") != std::string::npos);
  assert(mainSource.find("受け取った名刺は名刺帳に保存しました") == std::string::npos);
  const auto firstCard = loop.find("showReceivedCard(id);");
  const auto secondCard = loop.find("showReceivedCard(id);", firstCard + 1);
  assert(firstCard != std::string::npos && secondCard != std::string::npos);
  assert(loop.find("mutual.completeLeg(millis())") < firstCard);
  assert(loop.find("nfc.stop();") < firstCard);
  const auto showStart = mainSource.find("void showReceivedCard(");
  const auto showEnd = mainSource.find("void startBook(", showStart);
  assert(showStart != std::string::npos && showEnd != std::string::npos);
  const auto show = mainSource.substr(showStart, showEnd - showStart);
  assert(show.find("storage.loadCard(id.c_str(), data)") != std::string::npos);
  assert(show.find("tc::profileValid(shown[\"profile\"])") != std::string::npos);
  assert(show.find("selectedId = id;") < show.find("navigate(Screen::Card)"));
  assert(mainSource.find("if (!nfc.prepareOwn())") != std::string::npos);
  assert(source.find("sendPhase = mutualLeg ? 6 : 1") != std::string::npos);
  const auto frameDecode = source.find("tc::nfcAPayloadSize(response, size)");
  const auto pairValidation = source.find("size != 22");
  assert(frameDecode != std::string::npos && pairValidation != std::string::npos);
  assert(frameDecode < pairValidation);
  assert(source.find("if (!pendingBefore && (receiver.commitPending || receiver.clockPending)) return;") != std::string::npos);
  auto callbackStart = source.find("State receive_callback");
  auto callbackEnd = source.find("} emulation;", callbackStart);
  auto callback = source.substr(callbackStart, callbackEnd - callbackStart);
  assert(callback.find("drawNfcProgress") == std::string::npos);
  assert(callback.find("M5.Display") == std::string::npos);
  auto progress = source.substr(source.find("tc::TransferProgress progress"),
                                source.find("bool readyToClose()") - source.find("tc::TransferProgress progress"));
  assert(progress.find("tc::exchangeUiReady") != std::string::npos);
  std::cout << "NFC initialization single-configuration source contract: PASS\n";
}
