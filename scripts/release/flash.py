"""Touch Card package installer. Dry-run unless --execute is explicitly given.

Uses esptool 4.9.0 as a separately installed tool; never disables its security
checks and never erases all flash. No device is accessed during dry-run/tests.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys
import tempfile

FLASH_SIZE = 0x1000000
IMAGES = {
    "bootloader.bin": (0x0, 0x8000),
    "partitions.bin": (0x8000, 0x1000),
    "boot_app0.bin": (0xE000, 0x2000),
    "firmware.bin": (0x10000, 0x400000),
}


def load_package(root, language):
    root = Path(root).resolve()
    manifest = json.loads((root / "manifest.json").read_text(encoding="utf-8"))
    if (manifest.get("schema") != 1 or manifest.get("chip") != "esp32s3"
            or manifest.get("flash_size") != FLASH_SIZE
            or manifest.get("board") not in ("paper-mono", "stackchan")
            or not re.fullmatch(r"\d+\.\d+\.\d+", manifest.get("version", ""))):
        raise ValueError("Unsupported package / 対応していないパッケージです")
    entries = manifest["languages"][language]
    if set(entries) != set(IMAGES):
        raise ValueError("Missing or unexpected images / イメージ構成が不正です")
    paths = {}
    for name, (offset, maximum) in IMAGES.items():
        entry = entries[name]
        expected_path = f"{language}/{name}"
        candidate = root / expected_path
        if (entry.get("path") != expected_path or entry.get("offset") != offset
                or candidate.is_symlink() or candidate.resolve().parent != root / language):
            raise ValueError("Unsafe image path or offset / パスまたは書き込み位置が不正です")
        data = candidate.read_bytes()
        if (not 0 < len(data) <= maximum or len(data) != entry.get("size")
                or hashlib.sha256(data).hexdigest() != entry.get("sha256")):
            raise ValueError(f"Image verification failed / 検証に失敗: {expected_path}")
        if name in ("bootloader.bin", "firmware.bin"):
            if len(data) < 24 or data[0] != 0xE9 or int.from_bytes(data[12:14], "little") != 9:
                raise ValueError("Not an ESP32-S3 image / ESP32-S3用ではありません")
        paths[name] = candidate
    return manifest, paths


def image_arguments(paths, mode):
    # Update app first, then select app0. Never write NVS, the card filesystem,
    # the inactive app slot, or the partition table in update mode.
    names = list(IMAGES) if mode == "install" else ["firmware.bin", "boot_app0.bin"]
    return [item for name in names for item in (hex(IMAGES[name][0]), str(paths[name]))]


def check_backup(backup, partitions, mode):
    if backup.stat().st_size != FLASH_SIZE:
        raise ValueError("Incomplete backup; stopped / バックアップ不完全のため中止")
    if mode == "update":
        expected = partitions.read_bytes()
        with backup.open("rb") as stream:
            stream.seek(0x8000)
            actual = stream.read(len(expected))
        if actual != expected:
            raise ValueError("Partition mismatch; no writes / 保存領域が異なるため書き込みを中止")


def flash_package(root, language, mode, port, backup_dir, baud=460800,
                  runner=subprocess.run, confirm=input):
    manifest, paths = load_package(root, language)
    # Do not search or select USB ports automatically: the user must identify it.
    board = manifest["board"]
    print(f"{board} / {manifest['version']} / {language} / {mode} / {port}")
    print("Back up the microSD separately. Install replaces the current firmware and partition layout.")
    print("microSDは別途保存してください。初回導入は既存ファームウェアと保存領域の構成を置き換えます。")
    print("Chip detection cannot distinguish these two boards. Confirm the physical device.")
    if confirm(f"Type {board} to continue / 続けるには {board} と入力: ").strip() != board:
        raise ValueError("Cancelled / キャンセルしました")
    base = [sys.executable, "-m", "esptool", "--chip", "esp32s3", "--port", port, "--baud", str(baud)]
    backup_dir = Path(backup_dir).resolve()
    backup_dir.mkdir(parents=True, exist_ok=True, mode=0o700)
    private = Path(tempfile.mkdtemp(prefix=f"{board}-", dir=backup_dir))
    incomplete = private / "flash-backup.incomplete"
    # A complete backup is mandatory; read failure or mismatch stops before write.
    runner(base + ["--after", "no_reset", "read_flash", "0", hex(FLASH_SIZE), str(incomplete)], check=True)
    check_backup(incomplete, paths["partitions.bin"], mode)
    backup = private / "flash-backup.bin"
    incomplete.rename(backup)
    backup.chmod(0o600)
    digest = hashlib.sha256(backup.read_bytes()).hexdigest()
    (private / "SHA256SUMS").write_text(f"{digest}  flash-backup.bin\n", encoding="utf-8")
    print(f"Private backup (never publish) / 非公開バックアップ: {backup}")
    # Revalidate immediately before writing, after a potentially slow backup.
    _, paths = load_package(root, language)
    args = image_arguments(paths, mode)
    runner(base + ["--after", "no_reset", "write_flash", "--flash_size", "16MB"] + args, check=True)
    runner(base + ["--after", "hard_reset", "verify_flash", "--flash_size", "16MB"] + args, check=True)
    print("Write/verification complete / 書き込み・検証完了")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--package", type=Path, default=Path(__file__).resolve().parent)
    parser.add_argument("--language", choices=["ja", "en"], default="ja")
    parser.add_argument("--mode", choices=["install", "update"], required=True)
    parser.add_argument("--port")
    parser.add_argument("--backup-dir", type=Path, default=Path.cwd() / "private-backups")
    parser.add_argument("--baud", type=int, choices=[115200, 460800], default=460800)
    parser.add_argument("--execute", action="store_true", help="Back up, confirm board and write / 確認・保存後に実機へ書き込む")
    args = parser.parse_args()
    try:
        manifest, paths = load_package(args.package, args.language)
        if not args.execute:
            print(json.dumps({"dry_run": True, "board": manifest["board"], "language": args.language,
                              "mode": args.mode, "images": image_arguments(paths, args.mode)}, ensure_ascii=False, indent=2))
            print("No device access. Add --port PORT --execute to install. / 実機にはアクセスしていません。")
            return
        if not args.port:
            parser.error("--port is required with --execute")
        from importlib.metadata import version, PackageNotFoundError
        try:
            installed = version("esptool")
        except PackageNotFoundError:
            installed = None
        if installed != "4.9.0":
            raise ValueError("Install esptool==4.9.0 / esptool 4.9.0を使用してください")
        flash_package(args.package, args.language, args.mode, args.port, args.backup_dir, args.baud)
    except (OSError, ValueError, KeyError, subprocess.CalledProcessError) as error:
        parser.exit(1, f"Stopped / 中止: {error}\n")


if __name__ == "__main__":
    main()
