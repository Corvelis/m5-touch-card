import hashlib
import contextlib
import io
import json
from pathlib import Path
import subprocess
import tempfile
import unittest
from flash import FLASH_SIZE, IMAGES, flash_package, load_package, image_arguments


class FlashTests(unittest.TestCase):
    def setUp(self):
        output = contextlib.redirect_stdout(io.StringIO())
        output.__enter__()
        self.addCleanup(output.__exit__, None, None, None)
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        entries = {}
        for name, (offset, _) in IMAGES.items():
            data = bytearray(128)
            if name in ("firmware.bin", "bootloader.bin"):
                data[0], data[12] = 0xE9, 9
            path = self.root / "ja" / name
            path.parent.mkdir(exist_ok=True)
            path.write_bytes(data)
            entries[name] = dict(path=f"ja/{name}", offset=offset, size=len(data), sha256=hashlib.sha256(data).hexdigest())
        self.manifest = dict(schema=1, chip="esp32s3", board="paper-mono", version="0.1.0", flash_size=FLASH_SIZE, languages={"ja": entries})
        self.save()
        self.calls = []

    def save(self):
        (self.root / "manifest.json").write_text(json.dumps(self.manifest))

    def runner(self, cmd, check):
        self.calls.append(cmd)
        if "read_flash" in cmd:
            with Path(cmd[-1]).open("wb") as stream:
                stream.truncate(FLASH_SIZE)
                stream.seek(0x8000)
                stream.write((self.root / "ja/partitions.bin").read_bytes())

    def execute(self, mode="update", runner=None, confirm=None):
        flash_package(self.root, "ja", mode, "EXPLICIT_TEST_PORT", self.root / "private-backups",
                      runner=runner or self.runner, confirm=confirm or (lambda _: "paper-mono"))

    def test_validate_and_update_regions(self):
        _, paths = load_package(self.root, "ja")
        self.assertEqual(image_arguments(paths, "update")[::2], ["0x10000", "0xe000"])
        self.assertEqual(self.calls, [])

    def test_backup_write_verify_and_preserve_backup(self):
        self.execute()
        self.assertEqual(len(self.calls), 3)
        self.assertIn("verify_flash", self.calls[-1])
        self.assertNotIn("erase_flash", str(self.calls))
        backup = next((self.root / "private-backups").rglob("flash-backup.bin"))
        self.assertEqual(backup.stat().st_size, FLASH_SIZE)

    def test_cancel_no_device_access(self):
        with self.assertRaises(ValueError):
            self.execute(confirm=lambda _: "no")
        self.assertEqual(self.calls, [])

    def test_backup_failure_never_writes(self):
        def fail(cmd, check):
            self.calls.append(cmd)
            raise subprocess.CalledProcessError(1, cmd)
        with self.assertRaises(subprocess.CalledProcessError):
            self.execute(runner=fail)
        self.assertEqual(len(self.calls), 1)

    def test_partition_mismatch_never_writes(self):
        def mismatch(cmd, check):
            self.runner(cmd, check)
            Path(cmd[-1]).write_bytes(b"x" * FLASH_SIZE)
        with self.assertRaises(ValueError):
            self.execute(runner=mismatch)
        self.assertEqual(len(self.calls), 1)

    def test_tamper_rejected_before_device_access(self):
        (self.root / "ja/firmware.bin").write_bytes(b"changed")
        with self.assertRaises(ValueError):
            self.execute()
        self.assertEqual(self.calls, [])

    def test_unexpected_region_rejected(self):
        self.manifest["languages"]["ja"]["firmware.bin"]["offset"] = 0x810000
        self.save()
        with self.assertRaises(ValueError):
            self.execute()
        self.assertEqual(self.calls, [])

    def test_install_regions(self):
        self.execute(mode="install")
        args = self.calls[1][self.calls[1].index("16MB") + 1:]
        self.assertEqual(args[::2], ["0x0", "0x8000", "0xe000", "0x10000"])

    def test_incomplete_backup_never_writes(self):
        def incomplete(cmd, check):
            self.calls.append(cmd)
            Path(cmd[-1]).write_bytes(b"partial")
        with self.assertRaises(ValueError):
            self.execute(runner=incomplete)
        self.assertEqual(len(self.calls), 1)

    def test_verify_failure_is_not_success(self):
        def fail_verify(cmd, check):
            self.runner(cmd, check)
            if "verify_flash" in cmd:
                raise subprocess.CalledProcessError(1, cmd)
        with self.assertRaises(subprocess.CalledProcessError):
            self.execute(runner=fail_verify)
        self.assertEqual(len(self.calls), 3)
        self.assertTrue(list((self.root / "private-backups").rglob("flash-backup.bin")))

    def test_unexpected_path_rejected(self):
        self.manifest["languages"]["ja"]["firmware.bin"]["path"] = "../outside.bin"
        self.save()
        with self.assertRaises(ValueError):
            self.execute()
        self.assertEqual(self.calls, [])


if __name__ == "__main__":
    unittest.main()
