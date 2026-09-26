// Build a standalone patch from a verified private release. Never publish ROMs.
import {readFile, access, mkdir, writeFile} from 'node:fs/promises';
import {createHash} from 'node:crypto';
import {execFileSync} from 'node:child_process';
import path from 'node:path';
const [base, game, out, flips = 'flips'] = process.argv.slice(2);
if (!base || !game || !out) throw Error('Usage: node tools/package-full-patch.mjs original.gba verified-game.gba private-output-directory [flips]');
const hash = (data, algo = 'sha256') => createHash(algo).update(data).digest('hex');
if (hash(await readFile(base), 'sha1') !== 'f3ae088181bf583e55daf962a92bb46f4f1d07b7') throw Error('Wrong original ROM');
const expected = '64d4aed4beb4511483dfe9f655ef9f243fb188d74649a7d4af613789e144a8ad';
if (hash(await readFile(game)) !== expected) throw Error('Wrong game release');
await mkdir(out, {recursive:true});
const patch = path.resolve(out, 'Emerald-Arena-0.10.1-full.bps');
const reconstructed = path.resolve(out, 'verification-private.gba');
for (const p of [patch,reconstructed]) {
  try { await access(p); } catch (e) { if (e.code === 'ENOENT') continue; throw e; }
  throw Error('Refusing to overwrite '+p);
}
execFileSync(flips, ['--create','--bps',base,game,patch], {stdio:'inherit'});
execFileSync(flips, ['--apply',patch,base,reconstructed], {stdio:'inherit'});
if (hash(await readFile(reconstructed)) !== expected) throw Error('Reconstruction mismatch');
await writeFile(path.resolve(out,'SHA256SUMS.txt'),hash(await readFile(patch))+'  '+path.basename(patch)+'\n',{flag:'wx'});
console.log('Verified full patch: '+patch+'; verification-private.gba must stay private.');
