import unittest
from pathlib import Path
from package import check_build_paths, write_zip, is_notice_file, verified_runtime_notices
import tempfile
import zipfile
from check_linked_fonts import check_symbols


class PackageTests(unittest.TestCase):
    def test_runtime_notice_inventory(self):
        folder = Path(__file__).resolve().parents[2] / 'release/licenses/runtime'
        notices = verified_runtime_notices(folder)
        self.assertEqual(len(notices), 19)  # 18 texts plus the provenance manifest.
        embedded = notices['SOURCE-EMBEDDED-NOTICES.txt'].decode()
        for text in ('Copyright (C) 2020 Amazon.com', 'Copyright (c) 2019 kikuchan',
                     'Copyright (C) 2019, ChaN', 'This is free and unencumbered software'):
            self.assertIn(text, embedded)

    def test_linked_font_policy(self):
        good = 'D tc::stackDigits\nD tc::stackClockDigits\nD lgfx::v1::fonts::efontJA_16'
        check_symbols(good, True)
        for font in ('FreeSans24pt7b', 'FreeSansBold24pt7bBitmaps', 'FreeMono9pt7b', 'FreeSerif12pt7b'):
            with self.assertRaises(ValueError):
                check_symbols(good + '\nD lgfx::v1::fonts::' + font, True)
        with self.assertRaises(ValueError):
            check_symbols('D lgfx::v1::fonts::Font0', True)

    def test_nested_font_notice_names(self):
        for name in ('COPYRIGHT.txt', 'IPA_Font_License_Agreement_v1.0.txt', 'license.txt', 'NOTICE', 'COPYING.LESSER'):
            self.assertTrue(is_notice_file(Path(name)), name)
        self.assertFalse(is_notice_file(Path('font.cpp')))

    def test_local_path_is_rejected(self):
        with self.assertRaises(ValueError):
            check_build_paths(b"/temporary/private-build/firmware/main.cpp", [Path("/temporary/private-build")])

    def test_unknown_home_path_is_rejected(self):
        for prefix in (b"/Users/", b"/home/"):
            with self.assertRaises(ValueError):
                check_build_paths(prefix + b"example/work/main.cpp\x00", [])

    def test_known_prebuilt_toolchain_diagnostic_only(self):
        prefix = b"/Users/" + b"vendor/.gitlab-runner/builds/qR2TxTby/1/idf/crosstool-NG/.build/xtensa-esp32s3-elf/src/newlib/newlib/libc/stdlib/"
        check_build_paths(prefix + b"dtoa.c\x00" + prefix + b"mprec.c\x00", [])
        with self.assertRaises(ValueError):
            check_build_paths(prefix + b"another.c\x00", [])

    def test_zip_is_reproducible_and_refuses_overwrite(self):
        with tempfile.TemporaryDirectory() as directory:
            first, second = Path(directory) / "one.zip", Path(directory) / "two.zip"
            entries = {"en/a.txt": b"two", "ja/a.txt": b"one"}
            write_zip(first, entries)
            write_zip(second, entries)
            self.assertEqual(first.read_bytes(), second.read_bytes())
            with zipfile.ZipFile(first) as archive:
                self.assertIsNone(archive.testzip())
                self.assertEqual(archive.namelist(), sorted(entries))
            with self.assertRaises(FileExistsError):
                write_zip(first, entries)


if __name__ == "__main__":
    unittest.main()
