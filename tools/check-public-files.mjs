// Only project-created effect tiles explicitly listed by hash may ship in Git.
import {readFileSync} from 'node:fs';
import {execFileSync} from 'node:child_process';
import {createHash} from 'node:crypto';
const allowed=JSON.parse(readFileSync(new URL('../game/effect-assets.json',import.meta.url)));
for(const file of execFileSync('git',['ls-files','-z'],{encoding:'utf8'}).split('\0').filter(Boolean)) {
  if(/\.(gba|gbc|gb|sav|state|ss\d+)$/i.test(file))throw Error('Private file: '+file);
  if(/\.(4bpp|gbapal)$/i.test(file)) {
    if(!allowed[file])throw Error('Unlisted graphics: '+file);
    if(createHash('sha256').update(readFileSync(file)).digest('hex')!==allowed[file])throw Error('Effect asset changed: '+file);
  }
}
for(const [file,hash] of Object.entries(allowed)) {
  if(!/^game\/overlay\/graphics\/arena\/(psychic|sendout|elements|flame|double-team|warp-toss|dig-fly|cover-smoke|barriers|signatures150|signatures150-extra)\/[^/]+\.(4bpp|gbapal)$/.test(file))throw Error('Invalid effect path');
  if(createHash('sha256').update(readFileSync(file)).digest('hex')!==hash)throw Error('Missing or changed effect: '+file);
}
console.log('Public file allowlist passed. No ROMs or saves.');
