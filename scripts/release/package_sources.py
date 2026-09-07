"""Bundle the application and matching Arduino library sources for relinking.

Does not publish, download dependencies, use user data, or redistribute SDK
precompiled archives/toolchains. Those exact external inputs are inventoried.
"""
import argparse
import json
from pathlib import Path
from package import digest, source_files, write_zip, verified_runtime_notices

FRAMEWORK_VERSION = '3.20017.241212+sha.dcc1105b'


def framework_source(name):
    parts = Path(name).parts
    if any(p.startswith('.') or p == '__pycache__' for p in parts):
        return False
    if parts[0] in ('cores', 'libraries', 'variants'):
        return Path(name).suffix.lower() not in ('.a', '.o', '.bin', '.exe')
    if len(parts) == 1:
        return name in ('package.json', 'CMakeLists.txt', 'Kconfig.projbuild', 'boards.txt', 'programmers.txt')
    if parts[:2] == ('tools', 'sdk'):
        return len(parts) > 4 and parts[2] == 'esp32s3' and (
            parts[3] in ('include', 'ld') or Path(name).name in ('sdkconfig', 'sdkconfig.h'))
    return parts[0] == 'tools' and Path(name).suffix in ('.py', '.csv')


def prepare_sources(root, framework, output, version):
    if json.loads((framework / 'package.json').read_text())['version'] != FRAMEWORK_VERSION:
        raise ValueError('Unexpected framework version')
    entries = {}
    external = {}
    for path in sorted(framework.rglob('*')):
        name = path.relative_to(framework).as_posix()
        if framework_source(name) and path.is_file():
            if path.is_symlink():
                raise ValueError('Source symlinks are not allowed')
            entries['arduino/' + name] = path.read_bytes()
        elif path.is_file() and name.startswith('tools/sdk/esp32s3/'):
            external[name] = digest(path.read_bytes())
    for required in ('cores/esp32/main.cpp', 'libraries/FS/src/FS.cpp', 'tools/platformio-build.py'):
        if 'arduino/' + required not in entries:
            raise ValueError('Incomplete Arduino source: ' + required)
    for base in ('firmware', 'protocol'):
        for name in source_files(root, base):
            entries['touch-card/' + name] = (root / name).read_bytes()
    for name in ('LICENSE', 'THIRD_PARTY_NOTICES.md', 'docs/install.ja.md', 'docs/install.en.md',
                 'docs/operation.ja.md', 'docs/operation.en.md',
                 'scripts/build_stack_digits.cpp', 'scripts/build_paper_font.py',
                 'scripts/release/flash.py', 'scripts/release/check_linked_fonts.py'):
        entries['touch-card/' + name] = (root / name).read_bytes()
    for name, data in verified_runtime_notices(root / 'release/licenses/runtime').items():
        entries['licenses/runtime/' + name] = data
    entries['arduino/LICENSE.md'] = (root / 'release/licenses/arduino-esp32-LGPL-2.1.md').read_bytes()
    entries['README.md'] = (root / 'release/corresponding-source.md').read_bytes()
    entries['restore_framework.py'] = (root / 'scripts/release/restore_framework.py').read_bytes()
    manifest = dict(schema=1, version=version, framework_version=FRAMEWORK_VERSION,
                    framework_commit='dcc1105b0cf1322a437b354c336f2abf72b7e512',
                    firmware_source_sha256=digest(json.dumps(source_files(root, 'firmware'), sort_keys=True).encode()),
                    files={name: digest(data) for name, data in sorted(entries.items())},
                    external_sdk_files=external,
                    note='Source files retain their own licenses. SDK binaries and toolchains are external pinned inputs.')
    entries['source-manifest.json'] = (json.dumps(manifest, indent=2) + '\n').encode()
    write_zip(output, entries)
    print(json.dumps(dict(file=output.name, sha256=digest(output.read_bytes()), files=len(entries),
                          size_bytes=output.stat().st_size)))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--project', type=Path, default=Path(__file__).resolve().parents[2])
    parser.add_argument('--framework-dir', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--version', default='0.1.0')
    args = parser.parse_args()
    prepare_sources(args.project.resolve(), args.framework_dir.resolve(), args.output.resolve(), args.version)
