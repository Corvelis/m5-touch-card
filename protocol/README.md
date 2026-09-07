# M5 Touch Card NFC protocol v2

The firmware, Android app and iPhone app use the explicit `TC 02` protocol.
Targets identify profile/avatar/dashboard/fullscreen/received-card data; dimensions
never select a storage slot. Phone-update and received-card sessions are disjoint.

- [Normative framing, schemas and persistence semantics](specification.md)
- [Shared vectors](test_vectors.json): C++ receiver, Dart payload/CRC, actual Kotlin
  and Swift encoders/parsers.
- Run `node scripts/check_native_protocol.mjs` from the repository root.
  It compiles the production native protocol code into disposable host test programs.

The imported v1 transport under `migration/legacy/protocol/` is regression material
only. It is not interoperable with v2. Internal legacy class names in the native
source files do not imply wire compatibility.
