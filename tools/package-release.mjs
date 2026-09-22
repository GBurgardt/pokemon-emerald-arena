// Explicit public allowlist: never include ROMs, saves or development artifacts.
import {mkdir,readFile,writeFile,access} from 'node:fs/promises';
import {execFileSync} from 'node:child_process';
import {createHash} from 'node:crypto';
import path from 'node:path';
import {fileURLToPath} from 'node:url';
const root=fileURLToPath(new URL('../',import.meta.url));
if(!process.argv[2])throw new Error('Usage: node tools/package-release.mjs /private/output/directory');
const out=path.resolve(process.argv[2]);
if(out===root||out.startsWith(root+path.sep))throw new Error('Keep generated archives outside the source tree.');
await mkdir(out,{recursive:true});
const name='Emerald-Arena-0.7.0-preview.1.zip',zip=path.join(out,name);
try{await access(zip);throw new Error('Archive exists. Use a new output directory.');}catch(e){if(e.code!=='ENOENT')throw e;}
execFileSync(process.execPath,['tools/build-installer.mjs','--check'],{cwd:root,stdio:'inherit'});
execFileSync('zip',['-X','-q',zip,'Prepare-Emerald-Arena.html','installer.mjs','install.mjs','payload.json','README.md'],{cwd:path.join(root,'release'),stdio:'inherit'});
const sha=createHash('sha256').update(await readFile(zip)).digest('hex');
await writeFile(path.join(out,'SHA256SUMS.txt'),sha+'  '+name+'\n',{flag:'wx'});
console.log(JSON.stringify({zip,sha256:sha}));
