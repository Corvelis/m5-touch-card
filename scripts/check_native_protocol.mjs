// Compile the actual production protocol code, not a hand-copied test encoder.
// Output is disposable generated build data; no phone, NFC or network required.
import {readFileSync, writeFileSync, mkdirSync} from 'node:fs';
import {spawnSync} from 'node:child_process';
import {fileURLToPath} from 'node:url';
import path from 'node:path';
const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');
const output = path.join(root, 'build/protocol');
mkdirSync(output, {recursive: true});
const read = name => readFileSync(path.join(root, name), 'utf8');
function suffix(source, marker) {
  const at = source.indexOf(marker);
  if (at < 0 || at !== source.lastIndexOf(marker)) throw new Error(`Production marker changed: ${marker}`);
  return source.slice(at);
}
function run(command, args) {
  const result = spawnSync(command, args, {stdio: 'inherit', cwd: root});
  if (result.error) throw result.error;
  if (result.status !== 0) process.exit(result.status ?? 1);
}
const android = 'mobile/android/app/src/main/kotlin/io/github/corvelis/paper_mono_image_sender/';
writeFileSync(path.join(output, 'TransferTypes.kt'), 'package io.github.corvelis.touch_card\n' +
  suffix(read(android + 'MainActivity.kt'), 'internal data class PendingTransfer('));
run('kotlinc', [path.join(root, android, 'PaperMonoProtocol.kt'), path.join(output, 'TransferTypes.kt'),
  path.join(root, 'protocol/ProtocolCheck.kt'), '-include-runtime', '-d', path.join(output, 'protocol.jar')]);
run('java', ['-jar', path.join(output, 'protocol.jar'), path.join(root, 'protocol/test_vectors.json')]);
writeFileSync(path.join(output, 'ProtocolCheck.swift'), 'import Foundation\n' +
  suffix(read('mobile/ios/Runner/AppDelegate.swift'), '@available(iOS 13.0, *)\nprivate struct PendingTransfer {') +
  '\n' + read('protocol/ProtocolCheck.swift'));
run('swift', [path.join(output, 'ProtocolCheck.swift'), path.join(root, 'protocol/test_vectors.json')]);
