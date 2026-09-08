# Operation guide

For release downloads, first-time flashing and APK installation, start with the [installation guide](install.en.md).

[日本語](operation.ja.md) · [English](operation.en.md)

## Setup and phone updates

Back up existing device data before flashing or formatting. M5 Touch Card does not automatically format damaged/uninitialized internal storage.
If initialization is needed, open **Settings → Storage → Internal**, read the warning and explicitly confirm. This erases internal cards/images, not the SD card.

Open **Settings → More → 言語 / Language → English** to switch the device. Use the app's top globe menu to choose **日本語 / English** directly. The former phone-language mode migrates to Japanese.
These settings are independent, persist after restarting, and do not translate or modify card content.

1. Device: **Settings → Phone update**. The same waiting screen accepts every phone update type.
2. Phone: enter a name in **My card**, optionally add an icon, account, email, QR link and note, then save.
3. Select **Update card via NFC** and hold the phone at the device's NFC area until storage is confirmed.
4. The update menu also offers details-only or icon-only updates. Resetting the icon changes the phone draft first; send it via NFC afterward.
5. **Images** stores separate clock/calendar and full-screen photos. Changing **Send image to** automatically center-crops the current purpose's original image to that device's dimensions and saves it. Other purposes and the card icon remain untouched. Use **Crop again from the original** to adjust composition. Switching purposes recalls each saved image's device; conversion/save failures retain the old image and target.
6. Use the top clock icon to send the phone's time and UTC offset, without changing card/image data.
7. The top info icon opens licenses directly.

Pending transfers survive app restart and retry the same content/ID. A newer draft does not silently replace them; confirm replacement to send changed content.
100% transmission is not proof of storage: wait for the saved acknowledgement.

## Exchange and received cards

- Two-way: both devices select **Card exchange → Exchange**. Choose **Send first** on one and **Receive first** on the other. Keep NFC areas together through steps 1/2 and 2/2.
- One-way: choose **Send** and **Receive** on the corresponding devices.
- Completion displays the other person's card and saves it in **Cards**. If the second transfer fails or is cancelled, the first saved card is kept.
- Both endpoints need compatible M5 Touch Card firmware. Same-model pairs use the same protocol but have not yet been tested with two physical units.
- For smaller transfers, expand **Exchange icon** in the phone app and resize the shared image to 160px, then update the device via NFC. Original, on-device and home images remain unchanged. A two-second transfer is not guaranteed.

Open **Cards** for newest/name ordering, page controls and individual cards. Use the upper-right Back button to leave the list.
Open card actions to read full details, enlarge the QR or delete a received card after confirmation.
Name ordering is UTF-8 lexical order, not phonetic Japanese sorting. Newest ordering works even without setting the clock.

## Home and display

**Home screen** offers Card, Photo + clock and Full-screen photo. The photo/clock home opens a month calendar; full-screen photos open the menu when tapped.
PaperMono places a native 128px clock and date above the existing square photo, with a labeled week below.
The month view uses a large month heading, centered dates and a black circle for today.
Ordinary minute updates change only the clock rectangle, leaving photo content intact; a date change refreshes the calendar too. PaperMono additionally cleans the retained full frame once at each local :00 minute, as described below.
The menu has a **Home** action at the bottom. **Settings → Card design** selects Typography, Contrast or Simple.
The lower-right card button always says **Menu** and opens the main menu, including design/orientation previews and received cards, on both devices. In a preview, tapping elsewhere still returns to the selected home screen.
PaperMono additionally offers portrait/landscape orientation for cards and enlarged QR; settings and image homes keep their normal orientation.
Long fields are truncated on the card; card actions provide full text.

PaperMono uses fast differential updates for menus and limited progress updates during NFC. E-paper ghosting remains possible; this is not a promise of flicker-free operation.

## Power and buttons

- PaperMono skips the two intermediate library screen clears at startup. The first prepared home frame still uses one clean full-screen refresh; this waveform can flash internally. `[tc.boot]` logs display initialization, storage initialization, first-frame and total setup duration. Actual startup time requires device measurement.
- PaperMono's side red LED is switched off at app startup. It is a PMIC-default indicator, not used as a charging indicator by this app. Charging and frontlight settings are preserved. Pre-boot and download-mode LED behavior is unchanged.
- A short press moves row selection; holding A returns home/menu (or cancels NFC).
- PaperMono B toggles its light, including during transfers. While dark, touch and A (short/hold) are locked; B still wakes it.
  Waking restores only the footer with a fast partial update, without a full-screen cleaning waveform. Input resumes after the update and release of held inputs.
  Turning the light off hides only the bottom controls/divider, in portrait and landscape; the card, clock and battery remain.
  Controls return on wake. The photo/clock home also hides its footer; full-screen photos are never cropped or blanked.
  NFC and saving continue; toggling does not cancel an exchange. Ordinary minute updates change the clock and battery regions even while dark, leaving photos, cards and hidden controls intact. Full-screen photos remain overlay-free.
  PaperMono runs one full-screen cleaning refresh at each local :00 minute (09:00, 10:00, etc.), both lit and dark. This intentionally flickers once through a cleaning sequence while retaining the frame contents and brightness. Clock/battery changes and lit date changes share one transaction, rather than triggering multiple full refreshes.
  NFC waiting/transfers, saving, catalog scans and drawing defer hourly cleaning until idle. Pending hours coalesce into one cleaning refresh. An unset clock and the overlay-free full-screen photo are excluded. A clean boot render at :00 counts for that hour.
  When dark and idle, PaperMono enters CPU light sleep. NFC waiting/transfers, pending saves, catalog scans and display work defer sleep.
  It wakes at the next minute boundary, updates the clock/battery without turning on the light, then sleeps again. B wakes and lights the screen; its release is consumed so it cannot immediately turn the light off again.
  Touch and A do not wake the CPU. SD insertion/removal is checked after wake, so detection while sleeping can take about one minute.
  CPU light sleep preserves application memory and panel history; it does not reboot or itself force a full-screen refresh. The minute sleep/wake cycle does not switch panel power or power-save mode: those transitions invalidate the driver's differential baseline and would force unwanted whole-screen clock updates every minute. Hourly cleaning is deliberate and separate. No extra panel-controller sleep is applied. Peripheral/SD power rails are not cut.
  Sleep failures leave the device awake, with retries no more often than every five seconds. `[tc.power.sleep]` logs minute/button wakes or errors.
  USB connection does not prevent sleep; serial responsiveness while asleep is not guaranteed. Wake with B before diagnostics.
  The corrected partial updates, hourly cleaning and actual current savings still require hardware verification.
  Busy display updates are retried. Date/calendar changes while dark are deferred until the next lit periodic update, then repainted regionally. Some e-paper ghosting or local flicker remains possible.
- StackChan B confirms the selected row (or cancels NFC).
- StackChan power-button short press toggles screen brightness to zero and back. Touch/A/B input is suppressed while dark and until held inputs are released after waking. NFC, saving and clock updates continue; transfer completion does not automatically light the screen.
- PaperMono disables power-button short-press reset and double-click shutdown at startup, with readback verification and a warning on failure. Download-mode long press is unchanged.
- **Settings → More → Power** provides confirmed power off/restart without erasing cards/settings. Cancel is at the lower left; Confirm is at the lower right.
- Screen off is not deep sleep. Hardware long-press behavior on StackChan is retained. The new power controls still require physical-device verification.

## Storage and privacy

Internal storage works without microSD. **Settings → Storage** shows usage/free space and selects SD-first automatic storage or internal-only storage.
Card limits depend on usable capacity and reserved work space, not a fixed count. No automatic deletion of old cards occurs.
Do not remove SD during saving. Use the storage screen's safe removal/reload actions. M5 Touch Card manages `/tc` only and does not format the whole SD card.

Cards are plaintext and NFC is unauthenticated. Keep backups. Cards are edited locally and transferred via NFC; there is no Web publishing feature.
