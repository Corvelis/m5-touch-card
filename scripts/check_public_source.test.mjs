import test from 'node:test';
import assert from 'node:assert/strict';
import {mkdtempSync, writeFileSync, rmSync, symlinkSync} from 'node:fs';
import {tmpdir} from 'node:os';
import {join} from 'node:path';
import {fileURLToPath} from 'node:url';
import {execFileSync} from 'node:child_process';
import {inspect, scanRepository} from './check_public_source.mjs';

test('repository ignore rules exclude private files but retain required assets and lockfiles', () => {
  const cwd = fileURLToPath(new URL('..', import.meta.url));
  const ignored = ['backups/state.json', 'private-backups/state.json', '.venv/pyvenv.cfg', 'cards.db', 'contacts.vcf', '.env',
    'mobile/ios/ExportOptions.plist', 'firmware/.pio/build/paper-mono/firmware.bin',
    'mobile/ios/Flutter/Generated.xcconfig', 'mobile/android/local.properties'];
  const kept = ['firmware/assets/default.jpg', 'firmware/assets/TouchSansJP.ttf',
    'mobile/pubspec.lock', 'mobile/ios/Podfile.lock', '.env.example'];
  const result = execFileSync('git', ['check-ignore', '--no-index', '--stdin'], {
    cwd, input: [...ignored, ...kept].join('\n') + '\n', encoding: 'utf8',
  }).trim().split('\n');
  assert.deepEqual(result.sort(), ignored.sort());
});

test('guard detects private/generated files but allows source assets', () => {
  for (const name of ['.env', 'backup.P12', 'cards.sqlite', 'cards.db-wal', 'contacts.vcf',
    'backups/state.json', 'private-backups/state.json', '.venv/pyvenv.cfg', 'tc/profile.json', '.codex/state.json', 'mobile/ios/ExportOptions.plist']) {
    assert.ok(inspect(name).length, name);
  }
  for (const name of ['.env.example', 'protocol/test_vectors.json', 'firmware/assets/TouchSansJP.ttf',
    'mobile/assets/default.jpg', 'mobile/android/gradle/wrapper/gradle-wrapper.jar']) {
    assert.deepEqual(inspect(name), [], name);
  }
});

test('guard detects sensitive text without including values in findings', () => {
  const values = [
    ['-----BEGIN ', 'PRIVATE KEY-----'].join(''),
    'gh' + 'p_' + 'x'.repeat(30),
    'AK' + 'IA' + 'X'.repeat(16),
    'AI' + 'za' + 'x'.repeat(35),
    'https://' + 'user:secret@host.invalid',
    '/Users/' + 'test/project/', '/home/' + 'test/project/',
    'C:\\Users\\' + 'test\\project', 'DEVELOPMENT_TEAM = ' + 'X'.repeat(10),
  ];
  for (const value of values) {
    const findings = inspect('example.txt', value);
    assert.ok(findings.length);
    assert.ok(!findings.join().includes(value));
  }
});

test('staged scan reads index bytes, includes force-added ignored files and rejects links', t => {
  const cwd = mkdtempSync(join(tmpdir(), 'touch-card-guard-test-'));
  t.after(() => rmSync(cwd, {recursive: true, force: true}));
  const git = (...args) => execFileSync('git', args, {cwd, stdio: 'pipe'});
  git('init', '--quiet');
  writeFileSync(join(cwd, '.gitignore'), '*.db\n');
  writeFileSync(join(cwd, 'note.txt'), 'gh' + 'p_' + 'x'.repeat(30));
  git('add', '.gitignore', 'note.txt');
  writeFileSync(join(cwd, 'note.txt'), 'sanitized\n');
  assert.equal(scanRepository({cwd}).findings.length, 0);
  assert.equal(scanRepository({cwd, staged: true}).findings.length, 1);
  writeFileSync(join(cwd, 'private.db'), 'placeholder');
  git('add', '-f', 'private.db');
  symlinkSync('note.txt', join(cwd, 'link.txt'));
  git('add', 'link.txt');
  const {findings} = scanRepository({cwd, staged: true});
  assert.equal(findings.length, 3);
  assert.deepEqual(findings.map(f => f.name).sort(), ['link.txt', 'note.txt', 'private.db']);
});
