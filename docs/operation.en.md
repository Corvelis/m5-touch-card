# Operation guide

[日本語](operation.ja.md) · [English](operation.en.md) · [Installation](install.en.md)

## Set up your card

1. On the device, open **Menu → Settings → Phone update**.
2. In the phone's **Card** tab, enter your name. An icon, account, email, QR URL and note are optional.
3. Save, select **Update card via NFC**, and align the phone with the device's NFC area.
4. Wait for storage confirmation. Reaching 100% transmission does not mean saving is complete.
5. Use the phone's top clock button to send the time while the device remains on the same waiting screen.

If the waiting screen times out, open **Phone update** again.
Saving on the phone stores a local draft; send it by NFC to apply it to the device.

### Choose what to update

| Phone action | Device data changed |
| --- | --- |
| Update card via NFC | Your card details and icon |
| Additional menu → Details only | Details only |
| Additional menu → Icon only | Icon of the same profile only |
| Reset icon | Resets the phone draft; send via NFC to apply |
| Images → Clock/calendar | Photo for the clock home |
| Images → Full screen | Full-screen photo |
| Top clock button | Time and UTC offset |

The device accepts all these updates on the same screen. The card icon and two home photos never overwrite one another.
Changing **Send image to** automatically center-crops that purpose's original image to the new device's dimensions.
Use **Crop again from the original** to adjust composition.

Pending transfers survive app restart. Confirm replacement when sending newer content.
Deleting the phone app removes drafts, original images and the profile ID, which can make your next card a new identity.

## Exchange cards

### Two-way exchange

1. On both devices, open **Card exchange → Exchange**.
2. Choose **Send first** on one and **Receive first** on the other.
3. Bring the NFC areas together. Roles switch automatically after step **1/2**.
4. Keep the devices together through **2/2**. Both screens then show the received card.

Received cards are also saved in **Cards**. Cancelling or failing the second transfer does not delete the first saved card.

### Send one way

Choose **Send** on one device and **Receive** on the other. Do not mix one-way modes with **Exchange**.
Both devices need compatible M5 Touch Card firmware.

### Reduce image size

In the phone's **Card → Exchange icon**, resize the exchange image to 160px, then update the device via NFC.
Only the shared image gets smaller; original, on-device and home images are preserved.
Transfer time depends on image size, connection quality and storage.

## Cards and storage

- Open **Menu → Cards**. The bottom-left control selects name/latest ordering; the middle and right controls change pages.
- Tap a row to view a card. Card actions show full text or an enlarged QR. Use the upper-right **Back** to leave the list.
- Name order is UTF-8 lexical order, not phonetic Japanese sorting.
- Choose **Delete this card** and confirm the target. Deletion cannot be undone through normal controls. Copies on a disconnected SD card are not deleted.
- **Settings → Storage** shows internal/SD usage, free space and card counts. Capacity and reserved work space determine the limit, not a fixed count.
- Choose SD-first automatic storage or internal-only storage. Automatic mode uses internal storage if SD is unavailable at the start. Removing SD or a write failure during a transfer does not continue the write on another medium.
- SD-only cards disappear from the list while SD is absent and return when reinserted. Matching IDs on internal/SD storage show the latest version.
- Select safe removal and wait for confirmation before removing SD. Use the reload action to resume.

microSD is optional. Old cards are not deleted automatically, and the app does not format the whole SD card.
Only the `/tc` folder is managed. Back up important data to a PC or other storage.

### Initialize internal storage

If internal storage is unavailable, check **Settings → Storage → Internal**.
**Erase and initialize deletes internal cards/images**, not SD contents. It is not needed for a normal update.
Check connections and storage, and protect existing data before confirming.

## Home and design

- **Menu → Home screen** selects Card, Photo + clock/calendar or Full-screen photo.
- The card's lower-left button opens exchange; the lower-right opens **Menu**. Tap the body for full-text and QR actions.
- The clock home shows a large clock, photo and week calendar. Tap to open the month calendar.
- Full-screen photos have no clock/button overlay. Tap to open the menu.
- **Settings → Card design** selects Typography, Contrast or Simple.
- PaperMono's **Settings → Card orientation** selects portrait/landscape for cards and enlarged QR, without changing settings or photo orientation.
- In a preview, the lower-right button opens **Menu**; tapping elsewhere returns home. The menu also has a **Home** action at the bottom.

Long fields are truncated on the card; use the full-text action to read them. Design and orientation persist after restart.

## Buttons, language and power

| Action | PaperMono | StackChan |
| --- | --- | --- |
| Short A press | Select a row | Select a row |
| Hold A | Return home/menu; cancel during NFC | Same |
| Short B press | Toggle the light | Confirm; cancel during NFC |
| Short power press | Reset disabled | Toggle the screen |

Device language is under **Settings → More → 言語 / Language**; phone language uses the top globe menu.
These choices are stored independently and do not translate or modify card text. The phone's info button opens licenses.
Use **Settings → More → Power** for confirmed shutdown/restart without deleting cards or settings.

### PaperMono light and sleep

While dark, touch/A are locked and bottom controls on card/clock homes are hidden. Press B to turn the light on.
Waking restores only the controls with a partial update and unlocks input.

- The idle CPU enters light sleep when NFC waiting/transfers, saving, drawing and other work are inactive.
- It wakes at the next clock minute boundary, partially updates time/battery and sleeps again without lighting the screen.
- A full cleaning refresh at each local :00 reduces ghosting. NFC and other busy work defer it. Unset clocks and full-screen photos are excluded.
- Date/calendar changes catch up after the light is turned on. SD insertion/removal may take about a minute to detect while asleep.
- USB connection does not prevent sleep. Press B before flashing or serial diagnostics.

Fast e-paper updates may leave ghosting. Startup and hourly cleaning refreshes make the screen flash.
Power-button short-press reset and double-click shutdown are disabled; the side red LED turns off when the app starts.

### StackChan screen off

A short power-button press turns the screen off; press again to restore it. Touch/A/B are locked while dark.
NFC, saving and clock updates keep running. Screen off is not CPU sleep, and transfer completion does not automatically light the screen.
Hardware long-press power behavior is retained.
