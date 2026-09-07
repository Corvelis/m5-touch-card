// Read-only preflight for tracked files and non-ignored untracked candidates.
// This is a narrow guard, not a substitute for reviewing the staged diff/history.
import assert from 'node:assert/strict';
import { execFileSync } from 'node:child_process';
import { readFileSync, lstatSync } from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

export function inspect(name, text = '') {
  const problems = [];
  if (/(^|\/)(\.env(\..*)?|\.dev\.vars(\..*)?|key\.properties|local\.properties)$/.test(name)
      && !name.endsWith('.example')) problems.push('local environment or credential file');
  if (/\.(bin|elf|map|apk|aab|ipa|p12|pfx|pem|key|jks|keystore|mobileprovision|cer|der|csr|log|sqlite3?|db(?:-wal|-shm)?|vcf)$/i.test(name)
      || /(^|\/)(flash-backups|backups|private-backups|tc|xcuserdata|\.pio|node_modules|\.wrangler|build|\.dart_tool|\.venv|Pods|\.codex|\.agents|\.ssh)\//i.test(name)
      || /(^|\/)(ExportOptions\.plist|Generated\.xcconfig|flutter_export_environment\.sh)$/i.test(name))
    problems.push('private/generated artifact');
  if (/-----BEGIN (?:RSA |EC |OPENSSH )?PRIVATE KEY-----/.test(text)) problems.push('private key material');
  if (/\b(?:gh[pousr]_[A-Za-z0-9]{25,}|github_pat_[A-Za-z0-9_]{40,})\b/.test(text)) problems.push('possible GitHub token');
  if (/\b(?:AKIA|ASIA)[A-Z0-9]{16}\b/.test(text) || /\bAIza[A-Za-z0-9_-]{35}\b/.test(text)) problems.push('possible cloud credential');
  if (/https?:\/\/[^\s/:]+:[^\s/@]+@/.test(text)) problems.push('credential in URL');
  if (/(?:\/(?:Users|home)\/[A-Za-z0-9._-]+\/|[A-Za-z]:\\Users\\[^\\\r\n]+\\)/.test(text)) problems.push('personal absolute path');
  if (/DEVELOPMENT_TEAM\s*=\s*"?[A-Z0-9]{10}\b/.test(text)) problems.push('personal signing team');
  return problems;
}

export function scanRepository({cwd = process.cwd(), staged = false} = {}) {
  const git = args => execFileSync('git', args, {cwd, maxBuffer: 32 * 1024 * 1024});
  const entries = staged
    ? git(['ls-files', '--stage', '-z']).toString().split('\0').filter(Boolean).map(line => {
      const tab = line.indexOf('\t');
      const [mode, , stage] = line.slice(0, tab).split(' ');
      return {name: line.slice(tab + 1), mode, stage};
    })
    : [...new Set(git(['ls-files', '-c', '-o', '--exclude-standard', '-z']).toString().split('\0').filter(Boolean))].map(name => ({name}));
  const findings = [];
  for (const {name, mode, stage} of entries) {
    let data;
    if (staged) {
      if (stage !== '0' || !['100644', '100755'].includes(mode)) {
        findings.push({name, issues: ['review non-regular or conflicted index entry']});
        continue;
      }
      // Inspect bytes actually about to be committed, not a possibly different
      // or already sanitized working-tree copy.
      data = git(['cat-file', 'blob', `:0:${name}`]);
    } else {
      let stat;
      try { stat = lstatSync(path.join(cwd, name)); } catch (error) {
        if (error.code === 'ENOENT') continue;
        throw error;
      }
      if (!stat.isFile()) {
        findings.push({name, issues: ['review non-regular file before publishing']});
        continue;
      }
      data = readFileSync(path.join(cwd, name));
    }
    const issues = inspect(name, data.includes(0) ? '' : data.toString('utf8'));
    if (data.length > 10 * 1024 * 1024) issues.push('file exceeds 10 MiB: review generated/private content');
    if (issues.length) findings.push({name, issues});
  }
  return {count: entries.length, findings};
}

function main() {
  const args = process.argv.slice(2);
  if (args.length > 1 || args.some(arg => !['--self-test', '--staged'].includes(arg))) {
    console.error('Usage: node scripts/check_public_source.mjs [--self-test | --staged]');
    process.exitCode = 2;
    return;
  }
  if (args.includes('--self-test')) {
    for (const name of ['build/backup.bin', '.env.local', 'mobile/ios/team.p12',
      'build/state/db.sqlite', 'mobile/ios/xcuserdata/state']) {
      assert.ok(inspect(name).length, name);
    }
    assert.deepEqual(inspect('.env.example', ''), []);
    assert.deepEqual(inspect('firmware/assets/default.jpg'), []);
    assert.ok(inspect('secret.txt', ['-----BEGIN ', 'PRIVATE KEY-----'].join('')).length);
    console.log('Public-source guard self-test: PASS');
  } else {
    const {count, findings} = scanRepository({staged: args.includes('--staged')});
    for (const {name, issues} of findings) {
      console.error(`${JSON.stringify(name)}: ${issues.join(', ')}`); // Never print secret values.
    }
    console.log(`Public-source preflight: ${count} ${args.includes('--staged') ? 'index entries' : 'candidates'}, ${findings.length} flagged files`);
    if (findings.length) process.exitCode = 1;
  }
}

if (process.argv[1] && path.resolve(process.argv[1]) === fileURLToPath(import.meta.url)) main();
