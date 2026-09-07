# Mutual exchange extension (v2 compatible)

Ordinary one-way and phone sessions retain the existing HELLO (26 bytes, capabilities 31).
Mutual receive sessions add capability bit5 (32), append leg 1 or 2 at response byte26 (length27), and require PAIR before BEGIN. Mutual senders reject ordinary sessions and mismatched legs before sending card content; new one-way senders likewise reject mutual receivers. FINISH support is required.

PAIR: command9, exactly13 bytes: TC / version2 / opcode9 (4 bytes), nonzero little-endian uint64 session token (8 bytes), leg (1 byte). First sender generates token using ESP random. First receiver locks the first accepted token; repeated same PAIR is idempotent. Reverse-direction receiver is initialized with the same token, and rejects other tokens or legs. This is accidental peer/session binding, NOT cryptographic authentication against an active attacker on the NFC link.

PAIR success reply: standard13-byte response header, token at13..20, leg at21 (22 bytes). Rejection: standard13-byte header with BadLength, WrongSession, InvalidData or Conflict. No filesystem operation in PAIR. Existing CRC, transaction ID, offsets, COMMIT, durable STORED acknowledgement and FINISH semantics are unchanged.

These are application payload lengths, excluding the two NFC-A CRC bytes.
The pinned ST25R3916 reader API returns raw FIFO bytes, so a successful PAIR
arrives as 24 bytes. The sender validates CRC_A (seed 0x6363, reflected polynomial
0x8408, low byte first) and removes the trailer from the reported length before
parsing any reply. Corrupt/truncated frames follow the existing retry path;
PAIR still requires exactly 22 payload bytes and the expected token and leg.
FINISH remains best-effort after durable STORED confirmation.

After leg1 reaches the existing FINISH/grace condition and its completion indication has been visible for600ms, the old reader disables RF and changes to listener. Old listener waits1200ms non-blockingly before becoming the reader. The new reader's existing discovery/recovery handles further listener startup delay. Both reopen independent transfer IDs with the retained pair token and leg2. No third automatic leg is launched.

UI does not say it is safe to separate after leg1. Cancellation stops the pending role change; errors/partial success never roll back completed journals. Both owners are preflighted locally before entering mutual mode so a missing own card is caught before the first leg.

After both legs complete, stop NFC and open the received card on both devices
instead of a storage-completion message. Retain the first leg's received card ID
while sending the second leg, and clear it when starting a new exchange. One-way
receive uses the same viewer. If loading the saved card fails (e.g. removed SD),
show a display/storage-access warning, not a stale card or transfer-failure claim.

Host tests cover opt-in gating, ordinary/phone compatibility, length checks, token and leg mismatches, duplicate PAIR, reverse binding, missing reverse token, cancel reset, and role-switch workflow including millis rollover. Real RF reversal still requires hardware verification.
