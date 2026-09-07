# Filename compatibility (2026-09-06)

The connected StackChan's existing LittleFS volume was formatted with a
32-byte filename limit. The old 32-hex-digit ID plus `.a` (34 bytes), `.ia`
(35 bytes), or `.delete` (39 bytes) exceeds that limit. NFC reception reached
COMMIT, then record creation failed with status 16 despite 6,836,224 free bytes.

New received-card records use the full 128-bit ID encoded as 26 lower-case
RFC 4648 base32 characters without padding. Payload and metadata suffixes remain
`.a`, `.b`, `.ia`, `.ib`; tombstones use `.del`. Longest filename is 30 bytes.
Metadata and wire IDs remain the original 32 hex digits. No hash truncation,
case-sensitive distinction, filesystem format, or phone protocol change.

Existing long-name records are read and updated in place on volumes that already
contain them. No automatic migration. If both layouts exist, short artifacts are
authoritative (including corrupt records and tombstones); old data must not be
resurrected. Deletion retains a short tombstone until both layouts are removed.
Catalog enumeration recognizes both layouts and counts a card only once.

`card_core_test` covers the 32-byte failure/success boundary, update/load/catalog,
interrupted deletion, legacy read/update, mixed-layout deduplication, and 10,000
full-ID round trips. MemoryIO simulates the filename limit, not the physical
LittleFS driver; on-device end-to-end verification remains necessary.
