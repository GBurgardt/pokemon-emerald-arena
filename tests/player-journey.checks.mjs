import test from 'node:test';
import assert from 'node:assert/strict';
import vm from 'node:vm';
import {execFileSync} from 'node:child_process';
import {readFile,access} from 'node:fs/promises';

const root=new URL('../',import.meta.url);
const template=await readFile(new URL('tools/installer.html',root),'utf8');
test('single-file installer matches its source template and runtime',()=>{
  execFileSync(process.execPath,['tools/build-installer.mjs','--check'],{cwd:root});
});
test('hosted installer and ZIP installer are identical',async()=>{
 assert.equal(await readFile(new URL('site/public/prepare.html',root),'utf8'),await readFile(new URL('release/Prepare-Emerald-Arena.html',root),'utf8'));
});
test('setup cannot show a working button before its script starts',()=>{
 assert.match(template,/<button id="choose" disabled>/);
 assert.match(template,/Loading setup/);
 assert.doesNotMatch(template,/<script type="module">/);
 assert.match(template,/download the setup ZIP/);
});
const handler=template.split('const choose=')[1].split('</script>')[0];
function setup(prepareRom=async()=>new Uint8Array([1,2,3]),supported=true){
  const nodes=Object.fromEntries(['choose','file','status','download','ready','progress'].map(id=>[id,{hidden:['download','ready','progress'].includes(id),disabled:false,dataset:{},removeAttribute(k){delete this[k];},focus(){this.focused=true;}}]));
  const revoked=[];
  const context=vm.createContext({document:{querySelector:s=>nodes[s.slice(1)]},crypto:supported?{subtle:{}}:undefined,DecompressionStream:supported?function(){}:undefined,payload:{manifest:{source_size:3},patch:'AA=='},prepareRom,Uint8Array,Blob,URL:{createObjectURL:()=> 'blob:local-game',revokeObjectURL:u=>revoked.push(u)},atob:s=>Buffer.from(s,'base64').toString('binary')});
  vm.runInContext('const choose='+handler,context);
  return {nodes,revoked,async select(size=3){nodes.file.files=[{size,arrayBuffer:async()=>new Uint8Array(size).buffer}];await nodes.file.onchange();}};
}
test('success reveals the actual download and next steps, then recovers from a wrong file',async()=>{
  let calls=0;const ui=setup(async()=>{calls++;return new Uint8Array(3);});
  assert.equal(ui.nodes.download.hidden,true);
  await ui.select();assert.equal(calls,1);assert.equal(ui.nodes.ready.hidden,false);assert.equal(ui.nodes.download.download,'Emerald-Arena-0.7.0-preview.1.gba');assert.equal(ui.nodes.download.focused,true);
  await ui.select(2);assert.equal(calls,1);assert.equal(ui.nodes.download.hidden,true);assert.equal(ui.nodes.ready.hidden,true);assert.equal(ui.nodes.choose.disabled,false);assert.equal(ui.nodes.progress.hidden,true);assert.equal(ui.revoked.length,1);assert.match(ui.nodes.status.textContent,/not a ZIP/);
  await ui.select();assert.equal(calls,2);assert.equal(ui.nodes.ready.hidden,false);
});
test('download failure does not leave a stale game link or disabled retry',async()=>{
  const ui=setup(async()=>{throw new Error('Asset unavailable');});await ui.select();
  assert.equal(ui.nodes.status.dataset.error,'true');assert.equal(ui.nodes.download.hidden,true);assert.equal(ui.nodes.choose.disabled,false);assert.equal(ui.nodes.file.value,'');
});
test('unsupported browser gets an actionable message',()=>{
  const ui=setup(undefined,false);assert.equal(ui.nodes.choose.disabled,true);assert.match(ui.nodes.status.textContent,/updated Chrome/);
});
test('README puts a direct ZIP before gameplay and never calls a video Play',async()=>{
  const md=await readFile(new URL('README.md',root),'utf8');
  assert.ok(md.indexOf('Download Emerald Arena')<md.indexOf('## Watch gameplay'));
  assert.match(md,/releases\/download\/v0\.6\.0\/Emerald-Arena-0\.6\.0\.zip/);
  assert.doesNotMatch(md,/Play & watch|chatgpt\.site/);
});
test('README shows the current gameplay before setup and links longer footage',async()=>{
  const md=await readFile(new URL('README.md',root),'utf8');
  assert.equal((md.match(/!\[/g)||[]).length,1);
  assert.ok(md.indexOf('media/emerald-arena-combat-update.gif')<md.indexOf('## Start playing'));
  assert.match(md,/Full gameplay · 2:26 with sound/);
  assert.match(md,/New in 0.6.0/);
  assert.match(md,/PLAY.md#controls/);
  assert.match(md,/PLAY.md#current-scope/);
  const guide=await readFile(new URL('PLAY.md',root),'utf8');
  assert.match(guide,/## Controls/);
  assert.match(guide,/## Current scope/);
  assert.match(guide,/### Pokémon/);
});
test('local documentation links resolve and instructions ship in English',async()=>{
  for(const name of ['README.md','PLAY.md','HOW-IT-WORKS.md','RELEASE.md']){
    const md=await readFile(new URL(name,root),'utf8');
    for(const [,target] of md.matchAll(/\]\(([^)]+)\)/g))if(!/^(https?:|#)/.test(target))await access(new URL(target.split('#')[0],root));
  }
  assert.match(template,/<html lang="en">/);assert.match(template,/Your file is never uploaded/);
  assert.match(template,/NEW GAME/);assert.match(template,/SELECT/);assert.match(template,/L \+ R together/);
  assert.doesNotMatch(template,/FormData|XMLHttpRequest|method:\s*['"]POST/);
});
