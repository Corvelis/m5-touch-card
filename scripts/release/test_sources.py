import hashlib
import json
from pathlib import Path
import tempfile
import unittest
from package_sources import framework_source, FRAMEWORK_VERSION
from restore_framework import restore, inside


class SourceTests(unittest.TestCase):
    def test_source_selection(self):
        for name in ('cores/esp32/main.cpp', 'libraries/FS/src/FS.cpp', 'tools/platformio-build.py',
                     'tools/sdk/esp32s3/qio_opi/include/sdkconfig.h', 'tools/sdk/esp32s3/ld/memory.ld'):
            self.assertTrue(framework_source(name), name)
        for name in ('.piopm', 'tools/sdk/esp32/lib/libc.a', 'tools/sdk/esp32s3/lib/libc.a', 'tools/espota.exe'):
            self.assertFalse(framework_source(name), name)

    def test_restore_validates_and_does_not_overwrite(self):
        with tempfile.TemporaryDirectory() as directory:
            base = Path(directory)
            bundle, installed, output = base / 'bundle', base / 'installed', base / 'editable'
            (bundle / 'arduino/cores').mkdir(parents=True)
            installed.mkdir()
            (installed / 'package.json').write_text(json.dumps({'version': FRAMEWORK_VERSION}))
            source = b'// library source\n'
            (bundle / 'arduino/cores/main.cpp').write_bytes(source)
            (installed / 'sdk.a').write_bytes(b'sdk')
            manifest = dict(framework_version=FRAMEWORK_VERSION,
                            files={'arduino/cores/main.cpp': hashlib.sha256(source).hexdigest()},
                            external_sdk_files={'sdk.a': hashlib.sha256(b'sdk').hexdigest()})
            (bundle / 'source-manifest.json').write_text(json.dumps(manifest))
            restore(bundle, installed, output)
            self.assertEqual((output / 'cores/main.cpp').read_bytes(), source)
            self.assertFalse((installed / 'cores').exists())
            with self.assertRaises(ValueError):
                restore(bundle, installed, output)
            (installed / 'sdk.a').write_bytes(b'wrong SDK')
            with self.assertRaises(ValueError):
                restore(bundle, installed, base / 'bad')
            self.assertFalse((base / 'bad').exists())

    def test_manifest_traversal_is_rejected(self):
        with tempfile.TemporaryDirectory() as directory:
            for name in ('../outside', '/absolute', 'a/../../outside', 'a\\outside'):
                with self.assertRaises(ValueError):
                    inside(Path(directory), name)


if __name__ == '__main__':
    unittest.main()
