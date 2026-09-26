import {test} from 'node:test';
import './player-journey.checks.mjs';
import assert from 'node:assert/strict';
import {deflateSync} from 'node:zlib';
import {readFile} from 'node:fs/promises';
import {applyBps,crc32,decodePng,digest,encodeSpriteTiles,packSpriteSet,prepareRom} from '../release/installer.mjs';

const variable=n=>{const out=[];for(;;){let b=n%128;n=Math.floor(n/128);if(!n){out.push(b|128);return out;}out.push(b);n--;}};
const le=n=>[n&255,(n>>>8)&255,(n>>>16)&255,n>>>24];
const makePatch=(source,target,commands)=>{
  const body=Uint8Array.from([66,80,83,49,...variable(source.length),...variable(target.length),128,
    ...commands,...le(crc32(source)),...le(crc32(target))]);
  return Uint8Array.from([...body,...le(crc32(body))]);
};
test('BPS literal roundtrip and checksums',()=>{
  const source=Uint8Array.from([1,2,3]),target=Uint8Array.from([4,5,6,7]);
  const patch=makePatch(source,target,[...variable((target.length-1)*4+1),...target]);
  assert.deepEqual(applyBps(source,patch),target);
  assert.throws(()=>applyBps(Uint8Array.from([9,9,9]),patch));
  const corrupt=patch.slice();corrupt[8]^=1;assert.throws(()=>applyBps(source,corrupt));
});
test('BPS source read, relative source copy and overlapping target copy',()=>{
  const s=Uint8Array.from([1,2,3,4]),t=Uint8Array.from([1,2,3,4,3,4,3,4,3,4]);
  const p=makePatch(s,t,[...variable(3*4),...variable(1*4+2),...variable(2*2),...variable(3*4+3),...variable(4*2)]);
  assert.deepEqual(applyBps(s,p),t);
});
const chunk=(name,data)=>{const type=Buffer.from(name);const raw=Buffer.concat([type,data]);
  const size=Buffer.alloc(4),crc=Buffer.alloc(4);size.writeUInt32BE(data.length);crc.writeUInt32BE(crc32(raw));return Buffer.concat([size,raw,crc]);};
test('PNG exact RGBA and GBA palette/tile packing',async()=>{
  const h=Buffer.alloc(13);h.writeUInt32BE(1);h.writeUInt32BE(8,4);h[8]=8;h[9]=6;
  const raw=Buffer.from(Array.from({length:8},()=>[0,248,0,0,255]).flat());
  const png=Buffer.concat([Buffer.from([137,80,78,71,13,10,26,10]),chunk('IHDR',h),chunk('IDAT',deflateSync(raw)),chunk('IEND',Buffer.alloc(0))]);
  const sheet=await decodePng(png);assert.equal(sheet.rgba.length,32);assert.deepEqual([...sheet.rgba.subarray(0,4)],[248,0,0,255]);
  const packed=packSpriteSet({palette_offset:0,palette_sha256:'test',animations:[{sha256:'red',width:1,height:1,frames:1,offset:32}]},new Map([['red',sheet]]));
  assert.equal(packed[0].bytes.length,16384);assert.equal(packed[0].bytes.reduce((a,b)=>a+(b!==0),0),8);
  assert.equal(packed[1].bytes[2],31);
});
test('release rejects wrong ROM before any network access',async()=>{
  const {manifest,patch}=JSON.parse(await readFile(new URL('../release/payload.json',import.meta.url)));
  let fetched=false;
  await assert.rejects(prepareRom(new Uint8Array(10),manifest,Buffer.from(patch,'base64'),()=>{},async()=>{fetched=true;}));
  assert.equal(fetched,false);
  assert.equal(await digest(Buffer.from(patch,'base64')),manifest.patch_sha256);
  assert.equal(manifest.species.length,150);
  assert.equal(manifest.version,'0.10.0');
  assert.equal(manifest.target_sha256,'0809eb6c377cb848c456162352f2f97bbba5ec8d91807430b7373646718fc511');
  assert.equal(manifest.target_size,33554432);
  assert.ok(manifest.species.every(s=>s.sprite_format==='tile-dictionary-v1'));
  assert.deepEqual(manifest.species.filter(s=>s.animations.some(a=>a.scale===2)).map(s=>s.name).sort(),['ARTICUNO','GYARADOS','HO_OH','LUGIA','RAYQUAZA','SALAMENCE','WAILORD','ZAPDOS']);
});
test('tile dictionary preserves every byte and first-occurrence order',()=>{
  const raw=Uint8Array.from({length:4096},(_,i)=>Math.floor(i/32)%3);
  const packed=encodeSpriteTiles(raw),dv=new DataView(packed.buffer);
  const offset=dv.getUint32(0,true),count=dv.getUint32(4,true);
  assert.equal(count,3);assert.equal(offset,8+128*2);
  const restored=new Uint8Array(raw.length);
  for(let i=0;i<128;i++){
    const index=dv.getUint16(8+i*2,true);assert.equal(index,i%3);
    restored.set(packed.subarray(offset+index*32,offset+(index+1)*32),i*32);
  }
  assert.deepEqual(restored,raw);assert.ok(packed.length<raw.length);
  assert.throws(()=>encodeSpriteTiles(new Uint8Array(2047)));
  assert.throws(()=>encodeSpriteTiles(new Uint8Array()));
});
test('large sprites require explicit reduction and never silent cropping',()=>{
  const rgba=new Uint8Array(128*128*8*4);
  for(let i=0;i<rgba.length;i+=4){rgba[i]=248;rgba[i+3]=255;}
  const sheets=new Map([['large',{width:128,height:128*8,rgba}]]);
  const mon={sprite_format:'tile-dictionary-v1',palette_offset:0,animations:[{sha256:'large',width:128,height:128,frames:1,offset:32,scale:2}]};
  const chunks=packSpriteSet(mon,sheets),dv=new DataView(chunks[0].bytes.buffer);
  assert.equal(dv.getUint32(4,true),1);
  assert.ok(chunks[0].bytes.subarray(dv.getUint32(0,true)).every(b=>b===0x11));
  mon.animations[0].scale=1;assert.throws(()=>packSpriteSet(mon,sheets),/cropping/);
  mon.animations[0].scale=3;assert.throws(()=>packSpriteSet(mon,sheets),/scale/);
});

test('sprite registration preserves opaque pixels and rejects invalid offsets',()=>{
  const rgba=new Uint8Array(80*64*8*4);
  for(let d=0;d<8;d++){const i=((d*64+32)*80+5)*4;rgba[i]=248;rgba[i+3]=255;}
  const sheets=new Map([['shifted',{width:80,height:512,rgba}]]);
  const mon={palette_offset:0,animations:[{sha256:'shifted',width:80,height:64,frames:1,offset:32,frame_offset:[4,0]}]};
  const chunks=packSpriteSet(mon,sheets);
  assert.equal(chunks[0].bytes.filter(x=>x!==0).length,8);
  mon.animations[0].frame_offset=[0,0];assert.throws(()=>packSpriteSet(mon,sheets),/cropping/);
  mon.animations[0].frame_offset=[17,0];assert.throws(()=>packSpriteSet(mon,sheets),/registration/);
  mon.animations[0].frame_offset=[0.5,0];assert.throws(()=>packSpriteSet(mon,sheets),/registration/);
});
