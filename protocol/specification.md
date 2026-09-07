# M5 Touch Card NFC v2

This is the wire contract used by this repository. Numeric fields are little-endian.
Transport: NFC-A raw commands; sender is the reader, receiver emulates a MIFARE
Ultralight EV1 target. This is not NDEF business-card sharing, Android Beam or
phone card emulation. Both devices need compatible firmware. Phone updates use
Android NFC-A reader mode / iOS Core NFC MiFare commands.

## Framing

Every command starts `54 43 02 <opcode>` (ASCII TC, version 2). RF CRC-A is handled
by the transport. The application CRC is CRC-32/ISO-HDLC over exact payload bytes
(polynomial 0xEDB88320, initial/final XOR 0xFFFFFFFF).

Response common header, 13 bytes:

| Offset | Size | Meaning |
| --- | --- | --- |
| 0 | 3 | 54 43 02 |
| 3 | 1 | request opcode OR 0x80 |
| 4 | 1 | status |
| 5 | 4 | current transfer ID, zero before BEGIN |
| 9 | 4 | next expected payload offset |

Commands:

| Opcode | Total bytes | Fields after the 4-byte header |
| --- | --- | --- |
| 01 HELLO | 4 | none |
| 02 BEGIN | 23 | ID:u32, replace:u8, target:u8, encoding:u8, width:u16, height:u16, length:u32, CRC:u32 |
| 03 DATA | 13+n | ID:u32, offset:u32, n:u8, bytes[n] |
| 04 STATUS | 8 | ID:u32 |
| 05 COMMIT | 12 | ID:u32, CRC:u32 |
| 06 ABORT | 8 | ID:u32 |
| 07 SET_TIME | 15 | Unix seconds:u64, UTC offset minutes:i16, reserved:u8=0 |
| 08 FINISH | 8 | ID:u32; optional device-exchange acknowledgement after STORED |

BEGIN encoding=1 means UTF-8 JSON. Encoding=2 is the compact target5-only card
described below; send it only after HELLO advertises capability bit3.
Width and height in the BEGIN header MUST be
zero (actual image dimensions are inside JSON). ID is nonzero and stable for a
retry of identical bytes. replace=1 may replace an incomplete different transfer;
same-ID requests must have identical target/encoding/length/CRC. No replacement during commit.
Maximum application command length is 253, DATA n is 1..240, JSON is 1..524288 bytes.
Device senders use up to240-byte DATA chunks with compact-capable peers, bounded
by the peer's advertised limits. Two consecutive DATA failures reduce240→128→64
without changing payload/ID/offset. Older peers start at128. Phone transport is
unchanged (128, fallback64). The active firmware loop yields1ms, idle loop5ms.

HELLO appends 13 bytes:

| Response offset | Size | Meaning |
| --- | --- | --- |
| 13 | 2 | maximum RF frame=255 |
| 15 | 2 | maximum application command=253 |
| 17 | 2 | maximum DATA payload=240 |
| 19 | 4 | maximum JSON payload=524288 |
| 23 | 1 | capabilities: bit0 JSON, bit1 SET_TIME, bit2 typed updates, bit3 compact target5, bit4 target5 FINISH (currently31; older firmware7) |
| 24 | 1 | device: 1 Paper Mono, 2 official StackChan |
| 25 | 1 | session: 1 phone update, 2 receive another device's card |

HELLO capabilities are additive; ignore unknown bits.
Mutual receive sessions additionally advertise bit5 and a leg byte; see
[mutual-exchange.md](mutual-exchange.md) for the opt-in PAIR and role-reversal extension.

Image dimensions below are the per-device
contract, not an inferred destination. Phone senders reject session 2, device
senders reject session 1. Legacy PM/version1 is not compatible.

## Target and JSON schemas

| Target | Session | Envelope fields | Storage |
| --- | --- | --- | --- |
| 1 | phone | target, profile, optional avatar | internal own card |
| 2 | phone | target, profileId, revision, avatar | internal own avatar only |
| 3 | phone | target, image | internal dashboard image |
| 4 | phone | target, image | internal fullscreen image |
| 5 | receive | target, profile, avatar | received-card book, selected medium |

Profile:

```json
{"id":"00112233445566778899aabbccddeeff","revision":1,"name":"Yuma","account":"","email":"","url":"","comment":""}
```

All fields above are supplied; unused optional strings are empty. Only name is
required. Limits in Unicode scalar values: name80, account80, email254, URL512,
comment40. URL additionally has a 512 UTF-8-byte maximum and must be empty or an
http/https URL with a nonempty host. Invalid UTF-8, NUL and control characters are
rejected. ID is exactly 32 lowercase hex digits; revision is unsigned32.
Device font coverage is separate from valid Unicode input.

Image object: `{"jpeg":"<base64>","width":256,"height":256}`.
JPEG must be baseline, 8-bit, three components, at most262144 decoded bytes.
Avatar is at most256x256; the phone generates256x256. Preserve colour during
exchange, render grey on Paper Mono. Missing/default avatar is rendered from the
embedded initial asset, not copied from a currently selected home image.

| Image | Paper Mono | StackChan |
| --- | --- | --- |
| Dashboard | 386x386 | 144x144 |
| Fullscreen | 480x800 | 320x240 |

Target1 with omitted avatar preserves the current avatar for the same profile ID.
Explicit null resets it to the default. New profile IDs cannot inherit another
person's avatar. Target2 requires a matching existing own profile ID and carries
the new revision. Targets3/4 do not change the profile revision or other slots.
Device exchange always sends the full card. Its look-and-feel setting is not sent.

### Optional exchange avatar and compact transport

Phone targets1/2 may additionally supply `exchangeAvatar`, using the same image
schema as `avatar`. It is a separately stored transfer derivative (<=256x256),
not a replacement for the owner's display JPEG. Target1 text-only updates preserve
both images for the same profile ID. Replacing/resetting the avatar without this
field clears the derivative; a new profile cannot inherit it. Other image slots
are unchanged. Old firmware ignores the field and uses the original avatar.

The app generates a256x256 colour baseline JPEG, targeting8192 bytes with4:2:0
chroma, quality85 down to55 and white corners outside the circular crop. It keeps
original dimensions and permits an over-budget result rather than reducing quality
further. The original source and display JPEG stay unchanged. Existing phone
images are optimized only via the explicit preview action; saving a derivative
advances the profile revision. Existing immutable pending packets are not rewritten.

The sender selects `exchangeAvatar` when present, otherwise `avatar`, BEFORE
negotiation. The selected image becomes the received card's normal `avatar`.
Legacy JSON and compact transmission contain identical profile/avatar content;
transport changes never change content at an unchanged revision. Downgrading an
owner to old firmware can send a different avatar at that revision and correctly
produce CONFLICT; resave/update with an advanced revision, do not bypass checks.

Compact payload (encoding2, only target5 / device receive session):

| Bytes | Content |
| --- | --- |
| 0..3 | ASCII `TCJ1` |
| 4..7 | JSON metadata length:u32,1..8192 |
| 8..11 | raw JPEG length:u32,0..262144 |
| next | JSON `{target:5,profile:...,avatar:{width:256,height:256}}` |
| remaining | exact JPEG bytes, not Base64 |

No JPEG uses `avatar:null` and length0. The exact lengths must match the payload;
no trailing bytes. CRC covers the complete compact bytes. After verification,
outside the RF callback, firmware reconstructs ordinary JSON and applies the
existing JPEG validation, revision checks and durable journal save. Storage
selection budgets up to4/3 of compact bytes plus4096 for JSON expansion. On-disk
format and received-card semantics are unchanged; full photos and fonts are never
added to device exchange.

Same profile ID: a lower revision is rejected; equal revision with different
normalized profile/avatar content is a conflict. Equal revision and identical
content is safe to retry. A fresh target5 transfer of identical content updates
the receive-order counter, without adding a second list entry. A repeated RF
COMMIT/BEGIN for the same already stored transfer does not save a second time.
Cross-volume duplicates display the highest revision, then newest receive
sequence; exact ties prefer internal. Name order is UTF-8 lexical order + ID.

## Transfer lifecycle and durability

HELLO → BEGIN → DATA until all bytes → COMMIT → STATUS until STORED.
DATA at the expected offset advances it; repeated identical earlier chunks are
idempotent. Different bytes at an acknowledged offset are rejected. COMMIT must
match the BEGIN CRC and requires all bytes. It queues verification/persistence
and returns VERIFYING. **Only STORED confirms durable save.** Received byte count
and VERIFYING are not success. Native UI events include transferId; Dart ignores
success belonging to a different queued card.

The RF callback performs bounds checks and memory copies, never SD scanning,
capacity queries, JSON/JPEG parsing or display. Saving occurs in the main loop;
senders tolerate interrupted RF and retry the same transaction. Receiver keeps RF
available for four seconds after save/failure so the sender can poll. With bit4,
a device sender that has validated STORED sends one best-effort FINISH. It then
reports success without the former four-second UI delay; a lost FINISH response
does not undo the already-confirmed save. Receiver exits its grace period20ms
after successfully launching the FINISH response. FINISH before STORED, for a
different ID, or in phone mode cannot shorten the grace period. Without FINISH
(including unchanged phone clients), the receiver retains the four-second window.
Initial
wait timeout is60s, incomplete transfer inactivity timeout120s; device sender
and phone screen also have120s limits. Actual timing requires hardware validation.

Incomplete bytes live in RAM. Closing a wait session or rebooting discards them.
Phone pending snapshots keep the target, immutable payload, CRC and ID, so a new
session can send from zero; an existing session resumes its returned offset.
ABORT drops the receive offset, not saved data. SET_TIME accepts2023..2099 and
offset -840..840 minutes, queues RTC/NVS save and initially replies BUSY; retry
the identical timestamp until OK. This command is phone-session-only.

Receiving target5 chooses SD-preferred or internal-only at BEGIN using cached
capacity/readiness, and pins that medium + mount generation. Capacity and SD
volume identity are checked again before persistence. No mid-transfer fallback
to another medium. Existing duplicates do not change that placement policy.
Internal reserve1MiB, SD reserve64KiB, conservative per-write management4096 bytes.
No automatic eviction and no fixed maximum number of received cards.

## On-disk records

`/tc/own`, `/tc/dashboard`, `/tc/fullscreen`, or `/tc/<profile-id>` are key prefixes,
not directories. Each has `.a/.b` data generations and `.ia/.ib` small sidecars.
Data begins with16 bytes: `TCR2`, metadata length:u32, payload length:u32,
CRC:u32 over all following bytes. Following are metadata JSON (<=2048 bytes)
and the normalized payload JSON. Metadata is a `{body:<JSON string>,crc:<u32>}`
envelope; body contains key, generation, size, crc, name, account, revision,
sequence. Its CRC checks the exact UTF-8 body string; the body CRC field checks
the payload. It is local storage, not the NFC envelope.

Write the inactive generation, read it back, then publish its sidecar. The old
valid generation remains available. An absent/corrupt sidecar is recoverable
from a fully checksummed self-describing record, including on read-only media.
A fully written record may be recovered after power loss before its sender
received an acknowledgement; retransmission is safe. Both broken records are
not silently reset/overwritten. Filesystem/SD power-loss guarantees are limited.

Delete writes `<id>.delete` before removing either generation or sidecar. A
remaining marker masks all copies on that medium and cleanup retries during
the next scan. Only currently connected media are in scope, and multi-medium
deletion can partially fail. No offline deletion queue. `/tc/.volume` holds a
random16-byte SD identity; writes verify it and the mount generation. Unrelated
SD paths and formats are not changed. A cloned SD can share that app identity.

## Status codes (hex)

00 OK, 01 ACCEPTED, 02 BUSY, 03 CONFLICT, 04 BAD_MAGIC, 05 BAD_VERSION,
06 UNKNOWN_COMMAND, 07 BAD_LENGTH, 08 TOO_LARGE, 09 reserved legacy image-size,
0A BAD_OFFSET, 0B DATA_MISMATCH, 0C BAD_CRC, 0D INVALID_DATA, 0E UNSUPPORTED,
0F NOT_FOUND, 10 STORAGE_ERROR, 11 RECEIVING, 12 VERIFYING, 13 STORED,
14/15 reserved legacy display states, 16 NO_SPACE, 17 MEDIA_LOST, 18 WRONG_SESSION.
v2 firmware does not emit14/15, but native transport accepts them as saved for
forward display-notification compatibility. Unknown/errors never imply success.

No cryptographic authentication or encryption is provided. Only an explicit
phone-update wait accepts own-card/image/clock changes. Another-device receive
mode cannot update the owner's data. CRC is corruption detection, not authenticity.
