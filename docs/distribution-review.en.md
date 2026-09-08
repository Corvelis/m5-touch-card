# Redistribution and third-party licenses

[日本語](distribution-review.ja.md) · [English](distribution-review.en.md)

M5 Touch Card's original code is [MIT-licensed](../LICENSE). Third-party libraries, fonts and runtimes retain their own licenses.
This page identifies bundled materials; consult each file and the full notices for its terms.

## Included materials

| Component | License and reference |
| --- | --- |
| Original code and default artwork | MIT; artwork scope is in each assets directory's LICENSE |
| TouchSansJP and Touch Digits | SIL OFL 1.1; retain fonts and transformation scripts |
| /efont/ and Font0 | Retain each font's BSD-style copyright and notices |
| M5 libraries and ArduinoJson | [Third-party notices](../THIRD_PARTY_NOTICES.md) and bundled original texts |
| Arduino-ESP32 | LGPL-2.1-or-later and component-specific terms; matching source and relinking instructions |
| ESP-IDF and runtimes | Bundled [notices and provenance](../release/licenses/runtime/) |
| Android native dependencies | [Inventory/hashes](../release/android-dependencies.json) and [full notices](../mobile/licenses/ANDROID-NATIVE-NOTICES.txt) |
| Flutter and Dart | Included in the app's license list |

Numeric rendering uses OFL Touch Digits. Packaging checks for linked GNU FreeFont-derived FreeSans/FreeMono/FreeSerif symbols.
The notice collection includes some unlinked components; inclusion of a notice does not imply that component is used.

## Matching firmware source

Distribute `touch-card-v0.1.0-firmware-sources.zip` with the matching board ZIPs.
It includes application firmware, Arduino core/libraries/variants, ESP32-S3 headers/configuration/linker scripts and relinking helpers.
The pinned Arduino commit is `dcc1105b0cf1322a437b354c336f2abf72b7e512`. Compilers and SDK binaries come from pinned PlatformIO packages.

Included sources and external SDK inputs are recorded with SHA-256 hashes.
The [source and relinking guide](../release/corresponding-source.md) explains how to create an editable framework copy,
modify it and rebuild without changing the installed SDK.

## When redistributing

- Preserve copyright, full license texts, NOTICE files and font notices.
- Identify modifications and provide source/build instructions matching the modified binaries.
- Make firmware and corresponding source available from the same distribution location.
- Preserve the app's included notices and license viewer, accessible through the top info button.

See [release packaging](../release/README.md) to generate distribution files.
