import assert from "node:assert/strict";
import { createHash } from "node:crypto";
import { readFile, stat } from "node:fs/promises";
import test from "node:test";

async function render(path) {
  const workerUrl = new URL("../dist/server/index.js", import.meta.url);
  const { default: worker } = await import(workerUrl.href);
  return worker.fetch(
    new Request("https://emerald-arena.example" + path, { headers: { accept: "text/html", host: "emerald-arena.example" } }),
    { ASSETS: { fetch: async () => new Response("Not found", { status: 404 }) } },
    { waitUntil() {}, passThroughOnException() {} },
  );
}

for (const path of ["/", "/arena"]) {
  test(path + " serves the English product, download and real video", async () => {
    const response = await render(path);
    assert.equal(response.status, 200);
    const html = await response.text();
    assert.match(html, /<html lang="en"/);
    assert.match(html, /Pokémon/);
    assert.match(html, /Download game/);
    assert.match(html, /<a class="repo" href="https:\/\/github\.com\/GBurgardt\/pokemon-emerald-arena">GitHub repo/);
    assert.match(html, /The original Pokémon Emerald, modified for real-time battles\./);
    assert.match(html, /releases\/download\/v0\.10\.2\/Emerald-Arena-0\.10\.2\.zip/);
    assert.match(html, /href="\/prepare\.html"/);
    assert.match(html, /<video[^>]*src="\/emerald-arena-combat-update\.mp4"/);
    assert.match(html, /16 seconds: Blastoise versus Eevee/);
    for (const attribute of ["autoPlay", "muted", "loop", "playsInline", "controls"]) {
      assert.match(html, new RegExp("<video[^>]*" + attribute, "i"));
    }
    assert.match(html, /Sound on/);
    assert.match(html, /https:\/\/emerald-arena\.example\/og\.png/);
    assert.match(html, /summary_large_image/);
    assert.doesNotMatch(html, /codex-preview|SkeletonPreview|Your site is taking shape|12 Pokémon|Three things to try|Still your team|You make the moves|Source ↗/);
    const main = html.match(/<main>([\s\S]*?)<\/main>/)?.[1];
    assert.ok(main, "the product is rendered, not only client-side scaffolding");
    assert.equal((main.match(/<h1[ >]/g) ?? []).length, 1);
    assert.doesNotMatch(main, /<h2[ >]/);
    const words = main.replace(/<[^>]+>/g, " ").trim().split(/\s+/);
    assert.ok(words.length <= 40, "keep the landing to essential copy only");
  });
}

test("release assets are present and the installer stays local-only", async () => {
  for (const name of ["emerald-arena-17s.mp4", "emerald-arena-15s.mp4", "arena-poster.png", "og.png", "prepare.html", "fonts/tiny5-regular.ttf", "fonts/OFL-Tiny5.txt"]) {
    assert.ok((await stat(new URL("../public/" + name, import.meta.url))).size > 1000);
  }
  const installer = await readFile(new URL("../public/prepare.html", import.meta.url), "utf8");
  assert.match(installer, /lang="en"/);
  assert.match(installer, /Your file is never uploaded/);
  assert.match(installer, /crypto\.subtle\.digest/);
  assert.match(installer, /download\.download='Emerald-Arena-0\.10\.2\.gba'/);
  assert.match(installer, /\[hidden\]\{display:none!important\}/);
  assert.doesNotMatch(installer, /XMLHttpRequest|FormData|method:[ ]*['"]POST/);
  const video = await readFile(new URL("../public/emerald-arena-combat-update.mp4", import.meta.url));
  assert.equal(createHash("sha256").update(video).digest("hex"), "88f80afe6b6d7daa09363bc01c9b03ba0437db234067b1ceffb80018e3dc542b");
});

test("sharp, locally hosted typography and accessible contrast", async () => {
  const css = await readFile(new URL("../app/globals.css", import.meta.url), "utf8");
  assert.match(css, /font-family: "Tiny5"/);
  assert.match(css, /src: url\("\/fonts\/tiny5-regular\.ttf"\)/);
  assert.doesNotMatch(css, /border-radius:\s*[1-9]|box-shadow|linear-gradient|radial-gradient/);
  assert.match(css, /min-height: 48px/);
  assert.match(css, /focus-visible/);

  function luminance(hex) {
    const values = hex.match(/[\da-f]{2}/g).map(value => {
      const channel = parseInt(value, 16) / 255;
      return channel <= .04045 ? channel / 12.92 : ((channel + .055) / 1.055) ** 2.4;
    });
    return values[0] * .2126 + values[1] * .7152 + values[2] * .0722;
  }
  const color = name => css.match(new RegExp("--" + name + ": #([\\da-f]{6})"))[1];
  const contrast = (a, b) => (Math.max(luminance(a), luminance(b)) + .05) / (Math.min(luminance(a), luminance(b)) + .05);
  assert.ok(contrast(color("ink"), color("paper")) >= 4.5);
  assert.ok(contrast(color("muted"), color("paper")) >= 4.5);
  assert.ok(contrast("ffffff", color("blue")) >= 4.5);
});
