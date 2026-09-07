"""Create an editable framework copy; never overwrite the installed package."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def inside(root, name):
    path = Path(name)
    if path.is_absolute() or '..' in path.parts or '\\' in name:
        raise ValueError('Unsafe manifest path')
    result = root / path
    if result.is_symlink() or root.resolve() not in result.resolve().parents:
        raise ValueError('Source path escapes its directory')
    return result


def restore(bundle, installed, output):
    manifest = json.loads((bundle / 'source-manifest.json').read_text())
    if output.exists():
        raise ValueError('Choose a new output directory; nothing will be overwritten')
    if json.loads((installed / 'package.json').read_text())['version'] != manifest['framework_version']:
        raise ValueError('Installed framework version differs from this release')
    for name, expected in manifest['files'].items():
        if sha(inside(bundle, name)) != expected:
            raise ValueError('Source bundle hash mismatch: ' + name)
    for name, expected in manifest['external_sdk_files'].items():
        if sha(inside(installed, name)) != expected:
            raise ValueError('External SDK differs from this release: ' + name)
    shutil.copytree(installed, output, ignore=shutil.ignore_patterns('.piopm', '.git', '__pycache__'))
    for name in manifest['files']:
        if name.startswith('arduino/'):
            target = inside(output, name[len('arduino/'):])
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(inside(bundle, name), target)
    print('Editable Arduino framework created; original installation was not changed.')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--bundle', type=Path, default=Path(__file__).resolve().parent)
    parser.add_argument('--installed', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    restore(args.bundle.resolve(), args.installed.resolve(), args.output.resolve())
