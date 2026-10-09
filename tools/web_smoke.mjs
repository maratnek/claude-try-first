import http from 'node:http';
import fs from 'node:fs';
import path from 'node:path';
import { chromium } from 'playwright';

const [dist, outDir = 'web-smoke-out'] = process.argv.slice(2);
if (!dist) {
  console.error('usage: node web_smoke.mjs <web-dist dir> [out dir]');
  process.exit(2);
}
fs.mkdirSync(outDir, { recursive: true });

const types = { '.html': 'text/html', '.js': 'text/javascript', '.wasm': 'application/wasm', '.data': 'application/octet-stream' };
const server = http.createServer((req, res) => {
  const name = req.url.split('?')[0] === '/' ? 'index.html' : path.normalize(req.url.split('?')[0]).replace(/^(\.\.[/\\])+/, '');
  const file = path.join(dist, name);
  if (!file.startsWith(path.resolve(dist)) && !file.startsWith(dist)) { res.writeHead(403).end(); return; }
  fs.readFile(file, (err, body) => {
    if (err) { res.writeHead(404).end(); return; }
    res.writeHead(200, { 'Content-Type': types[path.extname(file)] || 'application/octet-stream' }).end(body);
  });
});
await new Promise((r) => server.listen(0, '127.0.0.1', r));
const url = `http://127.0.0.1:${server.address().port}/`;

const failures = [];
const browser = await chromium.launch({
  args: ['--use-angle=swiftshader', '--enable-unsafe-swiftshader', '--ignore-gpu-blocklist'],
});
const page = await browser.newPage({ viewport: { width: 1280, height: 720 } });
const log = [];
page.on('console', (m) => {
  log.push(`[${m.type()}] ${m.text()}`);
  if (m.type() === 'error') failures.push(`console error: ${m.text()}`);
});
page.on('pageerror', (e) => failures.push(`page error: ${e.message}`));
page.on('requestfailed', (r) => failures.push(`request failed: ${r.url()}`));
page.on('response', (r) => { if (r.status() >= 400) failures.push(`HTTP ${r.status()}: ${r.url()}`); });

await page.goto(url);
try {
  await page.waitForFunction(() => document.getElementById('loading').hidden, null, { timeout: 90000 });
} catch {
  failures.push('loading overlay never went away within 90 s');
}
await page.waitForTimeout(3000);

const loadingText = await page.textContent('#loading-text');
if (/Failed to load/.test(loadingText)) failures.push(`loading shows: ${loadingText}`);

const shot = path.join(outDir, 'menu.png');
await page.screenshot({ path: shot });
// The menu draws sky, ground and text, so a drawn frame has many distinct colours; a black or flat canvas has few.
const colours = await page.evaluate(async (dataUrl) => {
  const img = new Image();
  img.src = dataUrl;
  await img.decode();
  const c = document.createElement('canvas');
  c.width = 160; c.height = 90;
  const ctx = c.getContext('2d');
  ctx.drawImage(img, 0, 0, 160, 90);
  const d = ctx.getImageData(0, 0, 160, 90).data;
  const seen = new Set();
  for (let i = 0; i < d.length; i += 4) seen.add((d[i] >> 4) << 8 | (d[i + 1] >> 4) << 4 | (d[i + 2] >> 4));
  return seen.size;
}, 'data:image/png;base64,' + fs.readFileSync(shot).toString('base64'));
console.log(`distinct colours in menu frame: ${colours}`);
if (colours < 12) failures.push(`menu frame looks blank (${colours} distinct colours)`);

fs.writeFileSync(path.join(outDir, 'console.txt'), log.join('\n') + '\n');
await browser.close();
server.close();

if (failures.length) {
  for (const f of failures) console.error(`SMOKE FAIL: ${f}`);
  process.exit(1);
}
console.log('SMOKE PASS');
