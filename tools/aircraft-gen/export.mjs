import './node-polyfills.mjs';
import fs from 'node:fs';
import path from 'node:path';
import * as THREE from 'three';
import { GLTFExporter } from 'three/examples/jsm/exporters/GLTFExporter.js';
import { ERAS, MOVABLE, generate, auditJoints } from './aircraft-gen.mjs';

// usage: node export.mjs [--eras=all|war,golden] [--seeds=1,38 | --count=4] [--out=../../assets/models/aircraft]
const args = Object.fromEntries(process.argv.slice(2).map(a => a.replace(/^--/, '').split('=')));
const out = path.resolve(args.out ?? '../../assets/models/aircraft');
const eras = (args.eras ?? 'all') === 'all' ? ERAS.map(e => e.id) : args.eras.split(',');
const seeds = args.seeds ? args.seeds.split(',').map(Number)
  : Array.from({ length: +(args.count ?? 4) }, (_, i) => 1 + i * 37);

// raylib's loader ignores node hierarchy semantics we rely on, so every exported file is a flat
// list of meshes with transforms baked in, non-indexed so normals come out faceted (flatShading).
function bake(root) {
  root.updateMatrixWorld(true);
  const inv = root.matrixWorld.clone().invert();
  const flat = new THREE.Group();
  flat.name = root.name;
  root.traverse(o => {
    if (!o.isMesh || !o.visible) return;
    let g = o.geometry.clone().applyMatrix4(inv.clone().multiply(o.matrixWorld));
    if (g.index) g = g.toNonIndexed();
    g.computeVertexNormals();
    const m = new THREE.Mesh(g, o.material);
    m.name = o.name;
    flat.add(m);
  });
  return flat;
}

function splitRig(group) {
  group.updateMatrixWorld(true);
  const parts = [];
  for (const [name, axis] of MOVABLE) {
    const o = group.getObjectByName(name);
    if (!o) continue;
    const p = new THREE.Vector3(), q = new THREE.Quaternion(), s = new THREE.Vector3();
    o.matrixWorld.decompose(p, q, s);
    o.removeFromParent();
    o.position.set(0, 0, 0); o.quaternion.identity(); o.scale.set(1, 1, 1);
    const root = new THREE.Group();
    root.name = name;
    root.add(o);
    parts.push({ name, axis, pos: p.toArray(), quat: q.toArray(), root });
  }
  return parts;
}

const exporter = new GLTFExporter();
const glb = async obj => Buffer.from(await exporter.parseAsync(obj, { binary: true }));
const f = v => (+v).toFixed(5);

fs.mkdirSync(out, { recursive: true });
const index = [];
let failed = 0;
for (const eraId of eras) for (const seed of seeds) {
  const { id, spec, sim, group } = generate(eraId, seed);
  const a = auditJoints(group);
  if (a.orphans.length || a.sunken.length) {
    console.error('AUDIT FAIL ' + id, JSON.stringify(a));
    failed++;
    continue;
  }
  const dir = path.join(out, id);
  fs.mkdirSync(dir, { recursive: true });
  const parts = splitRig(group);
  fs.writeFileSync(path.join(dir, 'body.glb'), await glb(bake(group)));
  for (const p of parts) fs.writeFileSync(path.join(dir, p.name + '.glb'), await glb(bake(p.root)));
  fs.writeFileSync(path.join(dir, 'rig.txt'),
    '# name px py pz qx qy qz qw axis_x axis_y axis_z  (metres, Y up, nose +Z, rest pose)\n' +
    parts.map(p => [p.name, ...p.pos.map(f), ...p.quat.map(f), ...p.axis].join(' ')).join('\n') + '\n');
  fs.writeFileSync(path.join(dir, 'sim.json'), JSON.stringify(sim, null, 2));
  fs.writeFileSync(path.join(dir, 'spec.json'), JSON.stringify(spec, null, 2));
  index.push({ id, name: spec.name + ' ' + spec.mark, era: spec.eraId, year: spec.year,
    config: sim.config, tris: spec.tris, stats: spec.stats });
  console.log('ok ' + id + '  ' + spec.name + ' ' + spec.mark + '  ' + spec.tris + ' tris');
}
fs.writeFileSync(path.join(out, 'index.json'), JSON.stringify(index, null, 2));
console.log(index.length + ' aircraft -> ' + out + (failed ? '  (' + failed + ' failed audit)' : ''));
process.exit(failed ? 1 : 0);
