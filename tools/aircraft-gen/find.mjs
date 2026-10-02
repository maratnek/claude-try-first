import { ERAS, buildSpec, buildPlane, simConfig } from './aircraft-gen.mjs';

// Seed search: turn a gameplay brief into concrete aircraft.
// node find.mjs --era=war --where="stats.speed>=60,stats.agility>=50,config=biplane" --sort=-stats.agility --top=5 --scan=3000 --max-tris=9000
// Paths resolve against { stats, config, era, year, name, ...sim } — e.g. envelope.vStallMs, mass.massKg, aero.cd0.
const args = Object.fromEntries(process.argv.slice(2).map(a => { const i = a.indexOf('='); return [a.slice(2, i < 0 ? undefined : i), i < 0 ? '' : a.slice(i + 1)]; }));
const eras = !args.era || args.era === 'all' ? ERAS : ERAS.filter(e => args.era.split(',').includes(e.id));
const scan = +(args.scan ?? 2000), top = +(args.top ?? 5), maxTris = args['max-tris'] ? +args['max-tris'] : Infinity;
const get = (o, p) => p.split('.').reduce((v, k) => v?.[k], o);
const conds = (args.where ?? '').split(',').filter(Boolean).map(c => {
  const m = c.match(/^([\w.]+)\s*(>=|<=|!=|=|>|<)\s*(.+)$/);
  if (!m) throw new Error('bad condition: ' + c);
  const [, path, op, raw] = m, val = isNaN(+raw) ? raw : +raw;
  return o => { const v = get(o, path);
    return op === '>=' ? v >= val : op === '<=' ? v <= val : op === '>' ? v > val : op === '<' ? v < val : op === '!=' ? v != val : v == val; };
});
const sortKey = args.sort ?? '-stats.speed', desc = sortKey.startsWith('-'), sk = sortKey.replace(/^-/, '');

const hits = [];
for (const e of eras) for (let sd = 1; sd <= scan; sd++) {
  const spec = buildSpec(e, sd), sim = simConfig(spec);
  const row = { ...sim, stats: spec.stats, name: spec.name + ' ' + spec.mark };
  if (conds.every(c => c(row))) hits.push({ e, sd, row });
}
hits.sort((a, b) => (get(a.row, sk) - get(b.row, sk)) * (desc ? -1 : 1));
const out = [];
for (const h of hits) {
  if (out.length >= top) break;
  const spec = buildSpec(h.e, h.sd); buildPlane(spec);
  if (spec.tris > maxTris) continue;
  out.push({ id: h.row.id, name: h.row.name, config: h.row.config, year: h.row.year, tris: spec.tris,
    stats: h.row.stats, vStallKmh: Math.round(h.row.envelope.vStallMs * 3.6), vMaxKmh: Math.round(h.row.envelope.vMaxMs * 3.6) });
}
console.log(JSON.stringify({ matched: hits.length, scanned: eras.length * scan, results: out }, null, 2));
process.exit(out.length ? 0 : 2);
