"""Prepare local firmware release candidates from verified build outputs.

No upload, signing, flashing, device dumps, or complete-flash merged images.
Output directories must be new. Publication is a separate reviewed step.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import zipfile
from flash import IMAGES, FLASH_SIZE, load_package
import tempfile
from check_linked_fonts import check_elf


def digest(data):
    return hashlib.sha256(data).hexdigest()


def is_notice_file(path):
    # Copyright-only and prefixed font license filenames are common upstream.
    return bool(re.search(r"license|copying|notice|copyright", path.name, re.IGNORECASE))


def verified_runtime_notices(folder):
    records = json.loads((folder / 'provenance.json').read_text())
    result = {'provenance.json': (folder / 'provenance.json').read_bytes()}
    for record in records:
        name = record['file']
        if Path(name).name != name or '/' in name or '\\' in name or name in result:
            raise ValueError('Invalid runtime notice name')
        data = (folder / name).read_bytes()
        if digest(data) != record['sha256']:
            raise ValueError('Runtime notice hash mismatch: ' + name)
        result[name] = data
    for name in ('GCC-RUNTIME-EXCEPTION.txt', 'newlib-COPYING.txt', 'SOURCE-EMBEDDED-NOTICES.txt',
                 'littlefs-BSD-3-Clause.txt', 'TLSF-BSD-3-Clause.txt', 'wpa-supplicant-BSD.txt'):
        if name not in result:
            raise ValueError('Required runtime notice missing: ' + name)
    return result


def check_build_paths(data, private_roots):
    for root in private_roots:
        if str(root).encode() + b"/" in data:
            raise ValueError("Local build path found; rebuild with public path mapping")
    # These two diagnostics are already embedded in Espressif's precompiled
    # GCC/newlib archives, not produced on the machine packaging Touch Card.
    # Do not patch compiled code or silently allow any other home-directory path.
    vendor = rb"/(?:Users|home)/[^/\x00]+/\.gitlab-runner/builds/qR2TxTby/1/idf/crosstool-NG/\.build/xtensa-esp32s3-elf/src/newlib/newlib/libc/stdlib/(?:dtoa|mprec)\.c"
    for path in re.findall(rb"/(?:Users|home)/[^\x00\n\r]+", data):
        if not re.fullmatch(vendor, path):
            raise ValueError("Unexpected home-directory path found in firmware")


def source_files(root, base):
    return {p.relative_to(root).as_posix(): digest(p.read_bytes())
            for p in (root / base).rglob("*")
            if p.is_file() and not any(part.startswith(".") or part == "__pycache__"
                                      for part in p.relative_to(root / base).parts)}


def write_zip(path, entries):
    with zipfile.ZipFile(path, "x", compression=zipfile.ZIP_DEFLATED, compresslevel=9) as archive:
        for name, data in sorted(entries.items()):
            if name.startswith("/") or ".." in Path(name).parts:
                raise ValueError("Unsafe archive path")
            info = zipfile.ZipInfo(name, date_time=(2026, 1, 1, 0, 0, 0))
            info.compress_type = zipfile.ZIP_DEFLATED
            info.external_attr = 0o100644 << 16
            archive.writestr(info, data)


def collect_licenses(build_root, framework):
    result = {}
    deps = build_root / "firmware/.pio/libdeps/paper-mono"
    for name, expected_version in {"ArduinoJson": "7.4.3", "M5Utility": "0.2.0", "M5HAL": "0.1.2",
                                   "M5UnitUnified": "0.5.0", "M5Unit-NFC": "0.1.0",
                                   "M5GFX": "0.2.27", "M5Unified": "0.2.20",
                                   "M5PM1": "1.0.7", "M5IOE1": "1.0.9"}.items():
        folders = []
        for folder in deps.glob(f"{name}*"):
            metadata = folder / "library.json"
            if metadata.exists() and str(json.loads(metadata.read_text()).get("version")) == expected_version:
                folders.append(folder)
        if not folders:
            raise ValueError(f"Missing dependency metadata: {name} {expected_version}")
        folder = sorted(folders)[0]
        notices = [p for p in folder.rglob("*") if p.is_file()
                   and is_notice_file(p)
                   and ".git" not in p.parts]
        if not notices:
            raise ValueError(f"Missing dependency license: {name}")
        for notice in notices:
            result[f"licenses/{name}/{notice.relative_to(folder).as_posix()}"] = notice.read_bytes()
    # Framework and linked SDK license texts are kept, but this is not a claim
    # that collecting licenses alone satisfies corresponding-source obligations.
    for notice in framework.rglob("*"):
        if notice.is_file() and is_notice_file(notice):
            result[f"licenses/arduino-esp32/{notice.relative_to(framework).as_posix()}"] = notice.read_bytes()
    if not any(name.startswith("licenses/arduino-esp32/") for name in result):
        raise ValueError("Framework license texts not found")
    return result


def prepare(root, build_root, framework, output, version, nm=None):
    for base in ("firmware", "protocol"):
        if source_files(root, base) != source_files(build_root, base):
            raise ValueError(f"Build source differs from release source: {base}")
    pubspec = (root / "mobile/pubspec.yaml").read_text()
    if not re.search(rf"^version: {re.escape(version)}\+\d+$", pubspec, re.MULTILINE):
        raise ValueError("Mobile/project release version mismatch")
    framework_meta = json.loads((framework / "package.json").read_text())
    if not str(framework_meta.get("version", "")).startswith("3.20017.241212"):
        raise ValueError("Unexpected Arduino framework version")
    licenses = collect_licenses(build_root, framework)
    for name, data in verified_runtime_notices(root / 'release/licenses/runtime').items():
        licenses['licenses/runtime/' + name] = data
    if nm is None:
        nm = framework.parent / "toolchain-xtensa-esp32s3/bin/xtensa-esp32s3-elf-nm"
        if not nm.is_file():
            nm = nm.with_suffix('.exe')
    for board in ('paper-mono', 'stackchan', 'paper-mono-en', 'stackchan-en'):
        check_elf(nm, build_root / 'firmware/.pio/build' / board / 'firmware.elf', board.startswith('stackchan'))
    output.mkdir(parents=True, exist_ok=False)
    source_manifest = source_files(root, "firmware")
    fingerprint = digest(json.dumps(source_manifest, sort_keys=True).encode())
    results = []
    for board in ("paper-mono", "stackchan"):
        entries = dict(licenses)
        for source, target in {
            "scripts/release/flash.py": "flash.py", "LICENSE": "LICENSE",
            "THIRD_PARTY_NOTICES.md": "THIRD_PARTY_NOTICES.md",
            "firmware/assets/TouchSansJP-OFL.txt": "licenses/TouchSansJP-OFL.txt",
            "release/licenses/arduino-esp32-LGPL-2.1.md": "licenses/arduino-esp32-LGPL-2.1.md",
            "firmware/assets/LICENSE.md": "licenses/default-artwork.md",
            "firmware/lib/PaperUI/src/vendor/stb_truetype.h": "licenses/stb_truetype.h",
            "docs/install.ja.md": "docs/install.ja.md", "docs/install.en.md": "docs/install.en.md",
            "docs/operation.ja.md": "docs/operation.ja.md", "docs/operation.en.md": "docs/operation.en.md",
            "release/corresponding-source.md": "docs/corresponding-source.md",
        }.items():
            entries[target] = (root / source).read_bytes()
        entries["README.md"] = (f"# M5 Touch Card {version} — {board}\n\n"
                                 "[インストール（日本語）](docs/install.ja.md) · [Installation (English)](docs/install.en.md)\n\n"
                                 "`ja/`: 日本語初期 / Japanese default. `en/`: English default.\n"
                                 "対応ソース / Matching source: `touch-card-v" + version + "-firmware-sources.zip` (same Release).\n"
                                 "Read the guide before running flash.py. It is a dry-run without --execute.\n"
                                 "実行前に手順を確認してください。--executeなしでは実機にアクセスしません。\n").encode()
        manifest = dict(schema=1, board=board, chip="esp32s3", flash_size=FLASH_SIZE,
                        version=version, firmware_source_sha256=fingerprint, languages={})
        for language in ("ja", "en"):
            env = board if language == "ja" else f"{board}-en"
            build = build_root / "firmware/.pio/build" / env
            values = {}
            for name, (offset, _) in IMAGES.items():
                source = framework / "tools/partitions/boot_app0.bin" if name == "boot_app0.bin" else build / name
                data = source.read_bytes()
                check_build_paths(data, (Path.home(), root, build_root, framework))
                entry_path = f"{language}/{name}"
                entries[entry_path] = data
                values[name] = dict(path=entry_path, offset=offset, size=len(data), sha256=digest(data))
            manifest["languages"][language] = values
        entries["manifest.json"] = (json.dumps(manifest, indent=2) + "\n").encode()
        entries["SHA256SUMS"] = "".join(f"{digest(data)}  {name}\n" for name, data in sorted(entries.items())).encode()
        # Exercise the same validator shipped to end users, on both variants.
        with tempfile.TemporaryDirectory(prefix="touch-card-package-check-") as temp:
            for name, data in entries.items():
                dest = Path(temp) / name
                dest.parent.mkdir(parents=True, exist_ok=True)
                dest.write_bytes(data)
            for language in ("ja", "en"):
                load_package(temp, language)
        filename = f"touch-card-v{version}-{board}.zip"
        write_zip(output / filename, entries)
        results.append(filename)
    (output / "SHA256SUMS").write_text("".join(f"{digest((output / name).read_bytes())}  {name}\n" for name in results))
    status = dict(version=version, release_kind="normal", published=False, publish_ready=False,
                  firmware_packages=results, android="pending release signing and verification",
                  ios="source only; no IPA or TestFlight in initial release",
                  remaining=["public source commit/tag", "signed Android APK and install/update smoke test",
                             "firmware installer hardware smoke test", "attach and verify matching firmware source ZIP",
                             "publish notices and matching sources alongside binaries"])
    (output / "release-status.json").write_text(json.dumps(status, indent=2) + "\n")
    print(json.dumps(status, indent=2))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--project", type=Path, default=Path(__file__).resolve().parents[2])
    parser.add_argument("--build-root", type=Path, required=True)
    parser.add_argument("--framework-dir", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--version", default="0.1.0")
    parser.add_argument("--nm-tool", type=Path, help="Matching ESP32-S3 nm executable, if not next to the framework package")
    args = parser.parse_args()
    if not re.fullmatch(r"\d+\.\d+\.\d+", args.version):
        parser.error("Use a normal semantic release version")
    prepare(args.project.resolve(), args.build_root.resolve(), args.framework_dir.resolve(), args.output.resolve(), args.version, args.nm_tool)


if __name__ == "__main__":
    main()
