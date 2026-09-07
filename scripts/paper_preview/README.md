# PaperMono firmware-rendered review

This builds the pinned M5GFX and the **same PaperFont, PaperCard, PaperHome and list row
code used by the firmware**, into an off-screen canvas. It does not reimplement
the cards in HTML/CSS. It cannot reproduce physical panel contrast or ghosting.
Card previews quantize to four grays; menu text uses the same monochrome mode
as differential updates. Photo dithering on the physical panel may differ.

Requirements: CMake, C++17, pkg-config, SDL2, and installed firmware dependencies.

```sh
cmake -S scripts/paper_preview -B build/paper-preview
cmake --build build/paper-preview -j 8
build/paper-preview/paper_preview firmware/assets/TouchSansJP.ttf firmware/assets/default.jpg build/paper-preview/screens
build/paper-preview/paper_preview firmware/assets/TouchSansJP.ttf firmware/assets/default.jpg build/paper-preview/screens-en en
```

Outputs: six cards, six Japanese/long-field variants, menu, settings, card book.
Also outputs `home.png`, `month.png`, clock-unset states, leap February and a
six-row month. The home clock uses native 128px glyphs, not enlarged bitmaps.
Tests compare minute updates with a clean clock render and assert that every
pixel outside the clock rectangle (including the photo) stays unchanged.
All twelve months are checked for status/footer overlap in Japanese and English.
`light-off-portrait-*.png`, `light-off-landscape-*.png` and `home-light-off.png`
show the controls hidden while dark. Pixel tests verify that only the bottom
44px band changes, that lit views are untouched, and that full-screen photos
retain their entire image. The same clear function is used by firmware.
Footer cache tests also restore only the controls, retain sleeping clock/battery
updates, invalidate stale full-screen-photo/rotation caches and replace the cache
after background screen changes. Status tests cover both orientations, midnight,
100/99/9/0/unknown battery values and unknown clock state. Pixel comparisons check
regional date/calendar catch-up without modifying the photo, clock or footer.
Also renders seven NFC progress states for PaperMono and StackChan using the
same NfcProgressView renderer as firmware (waiting through completion/error).
The optional final argument `en` selects English chrome/progress labels and
also verifies partial/full progress pixel equivalence in that language.
All cards keep text in explicit rectangles, wrap Latin words at spaces where
possible, and truncate overflowing content with an ellipsis. Full text remains
available in the card actions screen.

The font is embedded in app flash, not the user's filesystem. No filesystem
upload/format or partition change is needed. It supports 7,689 Unicode characters;
rare characters outside CP932 and emoji may show a missing-glyph box.

Upstream font: https://github.com/google/fonts/tree/main/ofl/notosansjp
Source SHA256: c2f3b4d463500a2ddcd3849cded1fceeb9fd6d1c32e6cbecd568453ba50fc68f
Embedded font SHA256: 22fcd3ad3eaac37ec0d8677f5e3c0caa96611c2d8b18e7b9fda64eb12d22abed
stb_truetype SHA256: ecd30b05e0dd4fea3a13c26810dd9e1992dc379049482c393d5a19e6b5090aab
