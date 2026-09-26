// Emerald Arena: local-only ROM preparation. No user file is uploaded.
// MIT, German Burgardt. BPS codec follows the public BPS1 format.
const fail = message => { throw new Error(message); };
export async function digest(bytes, algorithm = 'SHA-256') {
  return Array.from(new Uint8Array(await crypto.subtle.digest(algorithm, bytes)),
    b => b.toString(16).padStart(2, '0')).join('');
}
const crcTable = Uint32Array.from({length:256}, (_, n) => {
  for(let i=0;i<8;i++) n=(n>>>1)^((n&1)?0xedb88320:0);
  return n>>>0;
});
export function crc32(bytes) {
  let c=0xffffffff;
  for(const b of bytes) c=crcTable[(c^b)&255]^(c>>>8);
  return (c^0xffffffff)>>>0;
}
export function applyBps(source, patch) {
  const dv=new DataView(patch.buffer,patch.byteOffset,patch.byteLength);
  if(patch.length<16 || new TextDecoder().decode(patch.subarray(0,4))!=='BPS1') fail('Invalid patch.');
  if(crc32(patch.subarray(0,-4))!==dv.getUint32(patch.length-4,true)) fail('Incomplete patch. Download the package again.');
  if(crc32(source)!==dv.getUint32(patch.length-12,true)) fail('This ROM does not match Emerald (USA/Europe).');
  let p=4;
  function variable() {
    let n=0,shift=1;
    for(let i=0;i<8;i++) {
      if(p>=patch.length-12) fail('Truncated patch.');
      const b=patch[p++]; n+=(b&127)*shift;
      if(b&128) return n;
      shift*=128; n+=shift;
    }
    fail('Invalid patch integer.');
  }
  const originalSize=variable(),size=variable(),metadata=variable();
  if(originalSize!==source.length||size>33554432||p+metadata>patch.length-12) fail('Unsupported size.');
  p+=metadata;
  const out=new Uint8Array(size);let at=0,sourceRelative=0,targetRelative=0;
  while(at<size) {
    const command=variable(),kind=command%4,count=Math.floor(command/4)+1;
    if(count>size-at) fail('Patch is out of bounds.');
    if(kind===0) {
      if(at+count>source.length) fail('Source read is out of bounds.');
      out.set(source.subarray(at,at+count),at);at+=count;
    } else if(kind===1) {
      if(p+count>patch.length-12) fail('Incomplete data.');
      out.set(patch.subarray(p,p+count),at);p+=count;at+=count;
    } else {
      const offset=variable(),delta=Math.floor(offset/2)*(offset%2?-1:1);
      if(kind===2) {
        sourceRelative+=delta;
        if(sourceRelative<0||sourceRelative+count>source.length) fail('Source copy is out of bounds.');
        out.set(source.subarray(sourceRelative,sourceRelative+count),at);sourceRelative+=count;at+=count;
      } else {
        targetRelative+=delta;
        if(targetRelative<0||targetRelative>=at) fail('Target copy is out of bounds.');
        for(let i=0;i<count;i++) out[at++]=out[targetRelative++];
      }
    }
  }
  if(p!==patch.length-12||crc32(out)!==dv.getUint32(patch.length-8,true)) fail('Patch output did not match the expected result.');
  return out;
}

// Deterministic PNG decoding: browser color-management cannot change pixels.
export async function decodePng(bytes) {
  if(bytes.length<33||bytes[0]!==137||new TextDecoder().decode(bytes.subarray(1,4))!=='PNG') fail('Invalid PNG.');
  const dv=new DataView(bytes.buffer,bytes.byteOffset,bytes.byteLength);
  let w,h,depth,type,palette,alpha,parts=[],total=0;
  for(let p=8;p+12<=bytes.length;) {
    const n=dv.getUint32(p),kind=new TextDecoder().decode(bytes.subarray(p+4,p+8));
    if(n>4000000||p+12+n>bytes.length) fail('Truncated PNG.');
    const data=bytes.subarray(p+8,p+8+n);
    if(kind==='IHDR') {
      w=dv.getUint32(p+8);h=dv.getUint32(p+12);depth=data[8];type=data[9];
      if(!w||!h||w*h>2000000||data[10]||data[11]||data[12]) fail('Unsupported PNG.');
    } else if(kind==='PLTE') palette=data;
    else if(kind==='tRNS') alpha=data;
    else if(kind==='IDAT') { parts.push(data);total+=n; }
    p+=n+12;if(kind==='IEND')break;
  }
  const channels={0:1,2:3,3:1,4:2,6:4}[type];
  if(!channels||![1,2,4,8].includes(depth)||(type!==3&&depth!==8)) fail('Unsupported PNG bit depth.');
  const packed=new Uint8Array(total);let at=0;
  for(const part of parts){packed.set(part,at);at+=part.length;}
  const raw=new Uint8Array(await new Response(new Blob([packed]).stream().pipeThrough(new DecompressionStream('deflate'))).arrayBuffer());
  const stride=Math.ceil(w*channels*depth/8),bpp=Math.max(1,Math.ceil(channels*depth/8));
  if(raw.length!==h*(stride+1)) fail('Invalid PNG dimensions.');
  const scan=new Uint8Array(h*stride);
  const paeth=(a,b,c)=>{const p=a+b-c,pa=Math.abs(p-a),pb=Math.abs(p-b),pc=Math.abs(p-c);return pa<=pb&&pa<=pc?a:pb<=pc?b:c;};
  for(let y=0;y<h;y++) {
    const filter=raw[y*(stride+1)];if(filter>4)fail('Invalid PNG filter.');
    for(let x=0;x<stride;x++) {
      const i=y*stride+x,a=x>=bpp?scan[i-bpp]:0,b=y?scan[i-stride]:0,c=y&&x>=bpp?scan[i-stride-bpp]:0;
      scan[i]=(raw[y*(stride+1)+1+x]+[0,a,b,Math.floor((a+b)/2),paeth(a,b,c)][filter])&255;
    }
  }
  const rgba=new Uint8Array(w*h*4);
  for(let y=0;y<h;y++)for(let x=0;x<w;x++) {
    const i=(y*w+x)*4,s=y*stride+x*channels;rgba[i+3]=255;
    if(type===3) {
      const index=(scan[y*stride+Math.floor(x*depth/8)]>>(8-depth-(x*depth)%8))&((1<<depth)-1);
      if(!palette||index*3+2>=palette.length)fail('Invalid palette.');
      rgba.set(palette.subarray(index*3,index*3+3),i);rgba[i+3]=alpha?.[index]??255;
    } else if(type===6){rgba.set(scan.subarray(s,s+4),i);}
    else if(type===2){rgba.set(scan.subarray(s,s+3),i);}
    else {rgba[i]=rgba[i+1]=rgba[i+2]=scan[s];if(type===4)rgba[i+3]=scan[s+1];}
  }
  return {width:w,height:h,rgba};
}

export function encodeSpriteTiles(tiles) {
  if(!tiles.length || tiles.length%2048)fail('Invalid sprite frame data.');
  const seen=new Map(),dictionary=[],indices=[];
  for(let at=0;at<tiles.length;at+=32){
    const tile=tiles.subarray(at,at+32),key=String.fromCharCode(...tile);
    let index=seen.get(key);
    if(index===undefined){
      index=dictionary.length;
      if(index>=65536)fail('Sprite dictionary is too large.');
      seen.set(key,index);dictionary.push(tile);
    }
    indices.push(index);
  }
  const offset=8+indices.length*2,out=new Uint8Array(offset+dictionary.length*32);
  const dv=new DataView(out.buffer);
  dv.setUint32(0,offset,true);dv.setUint32(4,dictionary.length,true);
  indices.forEach((index,i)=>dv.setUint16(8+i*2,index,true));
  dictionary.forEach((tile,i)=>out.set(tile,offset+i*32));
  return out;
}

export function packSpriteSet(species, sheets) {
  const colors=[0],index=(rgba,i)=>{
    if(!rgba[i+3])return 0;
    if(rgba[i+3]!==255)fail('Partial transparency is not supported.');
    const c=(rgba[i]>>3)|((rgba[i+1]>>3)<<5)|((rgba[i+2]>>3)<<10);
    let at=colors.indexOf(c,1);
    if(at<0){if(colors.length===16)fail('Palette is too large.');at=colors.length;colors.push(c);}
    return at;
  };
  for(const a of species.animations) {
    const sheet=sheets.get(a.sha256);
    if(sheet.width!==a.width*a.frames||sheet.height!==a.height*8)fail('Sprite dimensions do not match.');
    for(let i=0;i<sheet.rgba.length;i+=4)index(sheet.rgba,i);
  }
  const chunks=[];
  for(const a of species.animations) {
    if(a.offset===null)continue; // An alias uses an earlier ROM tile array.
    const s=sheets.get(a.sha256),tiles=new Uint8Array(a.frames*8*2048);
    const scale=a.scale??1;
    if(![1,2].includes(scale)||a.width%scale||a.height%scale)fail('Invalid sprite scale.');
    const offset=a.frame_offset??[0,0];
    if(!Array.isArray(offset)||offset.length!==2||!offset.every(v=>Number.isInteger(v)&&Math.abs(v)<=16))fail('Invalid sprite registration.');
    const fit=a.frame_fit;
    if(fit && (!Array.isArray(fit)||fit.length!==2||!fit.every(Number.isInteger)||fit[0]<1||fit[0]>fit[1]||fit[1]>8))fail('Invalid sprite fit ratio.');
    if(fit) {
      const [n,q]=fit;
      for(let d=0;d<8;d++)for(let f=0;f<a.frames;f++) {
        let left=a.width,top=a.height,right=0,bottom=0;
        for(let y=0;y<a.height;y++)for(let x=0;x<a.width;x++)if(s.rgba[((d*a.height+y)*s.width+f*a.width+x)*4+3]) {
          left=Math.min(left,x);top=Math.min(top,y);right=Math.max(right,x+1);bottom=Math.max(bottom,y+1);
        }
        if(right<=left||bottom<=top)continue;
        const w=Math.ceil((right-left)*n/q),h=Math.ceil((bottom-top)*n/q);
        if(w>64||h>64)fail('Fitted sprite cropping is not allowed.');
        for(let y=0;y<h;y++)for(let x=0;x<w;x++) {
          const sx=left+Math.floor(x*q/n),sy=top+Math.floor(y*q/n);
          const ci=index(s.rgba,((d*a.height+sy)*s.width+f*a.width+sx)*4);
          const tx=32-Math.floor(w/2)+x,ty=64-h+y;
          const off=(d*a.frames+f)*2048+(Math.floor(ty/8)*8+Math.floor(tx/8))*32+(ty%8)*4+Math.floor(tx%8/2);
          tiles[off]|=ci<<((tx&1)*4);
        }
      }
    } else {
    for(let d=0;d<8;d++)for(let f=0;f<a.frames;f++)for(let y=0;y<a.height;y++)for(let x=0;x<a.width;x++) {
      const pi=((d*a.height+y)*s.width+f*a.width+x)*4,ci=index(s.rgba,pi);
      if(!ci)continue;
      const tx=Math.floor(x/scale)+32-Math.floor(a.width/scale/2)+offset[0],ty=Math.floor(y/scale)+32-Math.floor(a.height/scale/2)+offset[1];
      if(tx<0||tx>=64||ty<0||ty>=64)fail('Sprite cropping is not allowed.');
      if(x%scale||y%scale)continue;
      const off=(d*a.frames+f)*2048+(Math.floor(ty/8)*8+Math.floor(tx/8))*32+(ty%8)*4+Math.floor(tx%8/2);
      tiles[off]|=ci<<((tx&1)*4);
    }
    }
    if(species.sprite_format && species.sprite_format!=='tile-dictionary-v1')fail('Unsupported sprite format.');
    chunks.push({offset:a.offset,bytes:species.sprite_format?encodeSpriteTiles(tiles):tiles,sha256:a.compiled_sha256});
  }
  const pal=new Uint8Array(32);colors.forEach((v,i)=>{pal[i*2]=v&255;pal[i*2+1]=v>>8;});
  chunks.push({offset:species.palette_offset,bytes:pal,sha256:species.palette_sha256});
  return chunks;
}

export async function prepareRom(source, manifest, patch, report=()=>{}, fetchResource=async url=>{
  const controller=new AbortController();
  const timer=setTimeout(()=>controller.abort(),30000);
  try{
  const response=await fetch(url,{credentials:'omit',signal:controller.signal});
  if(!response.ok)fail('Could not download an asset. Please try again.');
  const bytes=new Uint8Array(await response.arrayBuffer());
  if(bytes.length>4000000)fail('Asset is too large.');
  return bytes;
  }catch(error){
    if(controller.signal.aborted)fail('Animation download timed out. Check your connection and choose your ROM again.');
    throw error;
  }finally{clearTimeout(timer);}
}) {
  report('Checking your Emerald ROM…');
  if(source.length!==manifest.source_size||await digest(source)!==manifest.source_sha256)fail('Choose an unmodified Pokémon Emerald ROM (USA/Europe).');
  if(await digest(patch)!==manifest.patch_sha256)fail('Incomplete installation package.');
  const out=applyBps(source,patch),resources=new Map();
  for(const species of manifest.species)for(const a of species.animations)resources.set(a.sha256,a.url);
  // Keep downloaded PNGs compressed. Decode one species at a time so a
  // larger roster does not retain every animation's RGBA sheet in memory.
  const entries=[...resources],pngs=new Map();let next=0,done=0;
  async function worker(){for(;;){const at=next++;if(at>=entries.length)return;
    const [sha,url]=entries[at],bytes=await fetchResource(url);
    if(await digest(bytes)!==sha)fail('An asset changed. Download rejected.');
    pngs.set(sha,bytes);report(`Preparing animations ${++done}/${entries.length}…`);
  }}
  await Promise.all(Array.from({length:4},worker));
  for(const species of manifest.species) {
    report(`Adding ${species.name}…`);
    const sheets=new Map();
    for(const a of species.animations)if(!sheets.has(a.sha256))sheets.set(a.sha256,await decodePng(pngs.get(a.sha256)));
    for(const chunk of packSpriteSet(species,sheets)) {
      if(chunk.offset<0||chunk.offset+chunk.bytes.length>out.length||await digest(chunk.bytes)!==chunk.sha256)fail('Graphics do not match the verified release.');
      if(out.subarray(chunk.offset,chunk.offset+chunk.bytes.length).some(b=>b!==0))fail('Graphics region is not empty.');
      out.set(chunk.bytes,chunk.offset);
    }
  }
  if(await digest(out)!==manifest.target_sha256)fail('The final ROM does not match the verified release.');
  report('Ready. Download Emerald Arena and open it in your GBA emulator.');
  return out;
}
