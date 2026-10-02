import fs from 'node:fs';
import { ERAS, buildSpec, buildPlane, simConfig, auditJoints } from './aircraft-gen.mjs';

// node check.mjs [--count=40] [--max-tris=N] [--update-golden]
const args = Object.fromEntries(process.argv.slice(2).map(a => a.replace(/^--/, '').split('=')));
const count = +(args.count ?? 40);
const maxTris = args['max-tris'] ? +args['max-tris'] : Infinity;
let bad = 0, trisMax = 0, trisSum = 0;

for (const e of ERAS) for (let i = 0; i < count; i++) {
  const sd = 1 + i * 37;
  const spec = buildSpec(e, sd);
  const a = auditJoints(buildPlane(spec).group);
  if (a.orphans.length || a.sunken.length) { bad++; console.error('joint ' + e.id + '-' + sd, JSON.stringify(a)); }
  trisMax = Math.max(trisMax, spec.tris); trisSum += spec.tris;
  if (spec.tris > maxTris) { bad++; console.error('tris ' + e.id + '-' + sd + ' ' + spec.tris + ' > ' + maxTris); }
}
console.log('tris: avg ' + Math.round(trisSum / (ERAS.length * count)) + ', max ' + trisMax);

const goldenPath = new URL('./golden.json', import.meta.url);
const golden = JSON.parse(fs.readFileSync(goldenPath, 'utf8'));
const fresh = {};
for (const key of Object.keys(golden)) {
  const [eraId, sd] = key.split('-');
  const s = buildSpec(ERAS.find(e => e.id === eraId), +sd);
  fresh[key] = { spec: s, sim: simConfig(s) };
}
if (args['update-golden'] !== undefined) {
  fs.writeFileSync(goldenPath, JSON.stringify(fresh, null, 1));
  console.log('golden.json updated');
} else {
  for (const k of Object.keys(golden))
    if (JSON.stringify(golden[k]) !== JSON.stringify(fresh[k])) { bad++; console.error('golden drift ' + k); }
}

for (const k of Object.keys(fresh)) {
  const m = fresh[k].sim.mass, v = fresh[k].sim.envelope;
  if (!(v.vMaxMs > v.vStallMs * 1.3)) { bad++; console.error('envelope ' + k, v.vStallMs, v.vMaxMs); }
  if (!(m.staticMarginMAC > 0)) { bad++; console.error('unstable ' + k, m.staticMarginMAC); }
}

console.log(bad ? bad + ' problem(s)' : 'all checks passed (' + ERAS.length * count + ' airframes, ' + Object.keys(golden).length + ' goldens)');
process.exit(bad ? 1 : 0);
