// Procedural aircraft generator — single source for the viewer, the exporter and the checks.
import * as THREE from 'three';

/* ============================= seeded random ============================= */
function mulberry(seed) {
  let a = seed >>> 0;
  return () => {
    a = (a + 0x6D2B79F5) >>> 0;
    let t = Math.imul(a ^ (a >>> 15), 1 | a);
    t = (t + Math.imul(t ^ (t >>> 7), 61 | t)) ^ t;
    return ((t ^ (t >>> 14)) >>> 0) / 4294967296;
  };
}
const rnd = (r, a, b) => a + r() * (b - a);
const pick = (r, list) => list[Math.floor(r() * list.length) % list.length];
const chance = (r, p) => r() < p;

/* ================================= eras ================================= */
const ERAS = [
  {
    id: 'pioneer', name: 'Pioneer', years: '1909–1915', year: [1909, 1915],
    wings: [[2, .75], [1, .25]], engines: ['rotary'], gears: ['skid'], cockpit: ['open'],
    span: [9.0, 10.6], fuse: [4.6, 5.4], power: [.13, .24], propBlades: [2], chordMul: 1.45,
    palette: [['#e4d8bd', '#8a6a3f'], ['#d9cbb0', '#6f5738'], ['#efe6cf', '#9c3b2c']],
    designations: ['Type', 'No.', 'Model'],
  },
  {
    id: 'war', name: 'Great War', years: '1916–1922', year: [1916, 1922],
    wings: [[2, .72], [3, .22], [1, .06]], engines: ['rotary', 'inline'], gears: ['spoked'],
    cockpit: ['open'], span: [7.6, 9.2], fuse: [5.0, 6.0], power: [.34, .55], propBlades: [2],
    chordMul: 1.2,
    palette: [['#e9dfc6', '#b23a30'], ['#c8c2a6', '#3f5d46'], ['#e3d7b8', '#2f4a63'], ['#d8c9a4', '#7a3f6d']],
    designations: ['Scout', 'D.', 'F.', 'Type'],
  },
  {
    id: 'golden', name: 'Golden Age', years: '1923–1931', year: [1923, 1931],
    wings: [[2, .55], [1, .45]], engines: ['radial', 'inline'], gears: ['spoked', 'spatted'],
    cockpit: ['open'], span: [7.2, 10.0], fuse: [5.2, 6.4], power: [.5, .72], propBlades: [2],
    chordMul: 1.15,
    palette: [['#f2e7cd', '#c9452c'], ['#eadfc4', '#1f6b8f'], ['#f5efdd', '#d99a24'], ['#e8ddc0', '#2f7a58']],
    designations: ['Racer', 'Model', 'Mk'],
  },
  {
    id: 'streamline', name: 'Streamline', years: '1932–1938', year: [1932, 1938],
    wings: [[1, .85], [2, .15]], engines: ['radial', 'inline'], gears: ['spatted', 'retract'],
    cockpit: ['canopy', 'open'], span: [9.0, 11.4], fuse: [6.0, 7.4], power: [.68, .86],
    propBlades: [2, 3], chordMul: 1.15,
    palette: [['#d6d3cb', '#b0382c'], ['#c9cdd2', '#26445f'], ['#e2ddd0', '#8c7a2e']],
    designations: ['Mk', 'Model', 'Type'],
  },
  {
    id: 'warbird', name: 'Warbird', years: '1939–1945', year: [1939, 1945],
    wings: [[1, 1]], engines: ['inline', 'radial'], gears: ['retract'], cockpit: ['canopy'],
    span: [9.6, 12.2], fuse: [6.6, 8.2], power: [.84, 1.0], propBlades: [3, 4], chordMul: 1.15,
    palette: [['#6f7a62', '#3d4535'], ['#8d939a', '#2d3742'], ['#7c7566', '#4a3f2e'], ['#9aa3a8', '#7a2b24']],
    designations: ['Mk', 'F-', 'Type', 'A-'],
  },
];

const NAME_A = ['Kestrel', 'Falcon', 'Swift', 'Heron', 'Hornet', 'Lark', 'Vireo', 'Osprey',
  'Marlin', 'Sabre', 'Comet', 'Gannet', 'Shrike', 'Tercel', 'Petrel', 'Wyvern', 'Merlin', 'Tanager'];

function weighted(r, pairs) {
  const total = pairs.reduce((s, p) => s + p[1], 0);
  let x = r() * total;
  for (const [v, w] of pairs) { if ((x -= w) <= 0) return v; }
  return pairs[0][0];
}

/* ======================= flight model (real units) ======================= */
const RHO = 1.225, G = 9.81;
function flightModel(s) {
  const tm = (1 + s.taper) / 2;                      // mean / root chord
  let S = 0, acNum = 0;
  s.levels.forEach((lv, i) => {
    const a = lv.span * lv.chord * tm * (i > 0 ? .94 : 1);   // mutual interference
    S += a; acNum += a * (lv.z + lv.chord * .25);
  });
  const b = Math.max(...s.levels.map(l => l.span));
  const AR = b * b / S;                     // on total area; biplane loss goes into e
  const MAC = s.chord * tm;
  const acZ = acNum / S;

  const powerKW = 20 + 1120 * Math.pow(s.power, 2.6);
  const cd0 = (s.year < 1923 ? .017 : s.year < 1932 ? .016 : s.year < 1939 ? .014 : .012) +
    (s.wings - 1) * .012 +
    (s.gear === 'retract' ? .002 : s.gear === 'spatted' ? .007 : s.gear === 'skid' ? .012 : .010) +
    (s.cockpit === 'open' ? .005 : .001) + s.fuseR * (s.year > 1938 ? .018 : .022) +
    (s.wings > 1 ? .006 : 0);
  const e = .80 - (s.wings - 1) * .10;
  const clMax = 1.15 + (s.wings > 1 ? .04 : .10) + (s.year > 1932 ? .16 : 0);

  const massEmpty = 90 + S * (s.year < 1923 ? 5.5 : s.year < 1939 ? 8.5 : 11) +
    powerKW * (s.year < 1923 ? 3.0 : s.year < 1939 ? 2.8 : 3.1) + (s.wings - 1) * 45;
  const fuel = massEmpty * 1.32 * (s.year < 1923 ? .055 : s.year < 1939 ? .085 : .105);
  const mass = massEmpty + fuel + 95;                // + crew, oil, guns
  const W = mass * G;

  const vStall = Math.sqrt(2 * W / (RHO * S * clMax));
  const etaP = .72 + (s.propBlades - 2) * .02 + (s.year > 1938 ? .08 : s.year > 1932 ? .06 : 0);
  const pAvail = powerKW * 1000 * etaP;
  const vMax = Math.cbrt(2 * pAvail / (RHO * S * cd0));
  // best rate of climb: scan the speed range instead of guessing one speed
  let roc = -1e9, vy = vStall * 1.2;
  for (let k = 1.02; k <= 2.4; k += .04) {
    const v = vStall * k;
    const pr = .5 * RHO * v ** 3 * S * cd0 + 2 * W * W / (RHO * v * S * Math.PI * AR * e);
    const rc = (pAvail - pr) / W;
    if (rc > roc) { roc = rc; vy = v; }
  }
  roc = Math.max(.1, roc);
  const tipR = Math.min(s.BASE - .18, .52 + s.power * .52);
  const thrust = Math.cbrt(2 * RHO * Math.PI * tipR * tipR * Math.pow(pAvail, 2));

  const nLimit = 3.0 + (s.year - 1909) * .055 + (s.wings - 1) * .35;
  /* turn performance: instantaneous (lift / structure) and sustained (power) */
  let turnInst = 0, turnSus = 0, nInstBest = 1, nSusBest = 1, vCorner = vStall;
  for (let v = vStall * 1.05; v <= vMax * .98; v += .5) {
    const nLift = .5 * RHO * v * v * clMax / (W / S);
    const kInd = 2 * W * W / (RHO * v * v * S * Math.PI * AR * e);
    const nPow = Math.sqrt(Math.max(0, (pAvail / v - .5 * RHO * v * v * S * cd0) / kInd));
    const nI = Math.min(nLift, nLimit), nS = Math.min(nI, nPow);
    const rI = 57.3 * G * Math.sqrt(Math.max(0, nI * nI - 1)) / v;
    const rS = 57.3 * G * Math.sqrt(Math.max(0, nS * nS - 1)) / v;
    if (rI > turnInst) { turnInst = rI; nInstBest = nI; vCorner = v; }
    if (rS > turnSus) { turnSus = rS; nSusBest = nS; }
  }
  const rollRate = 57.3 * .16 * (vMax * .8) / b;

  const thermal = .14 + (s.year - 1909) * .0035;
  const flow = (.45 * powerKW * 1000) / (thermal * 43e6);  // kg/s
  const endurance = fuel / flow / 3600;
  const range = vMax * .75 * 3.6 * endurance;
  const ceiling = 11500 * (1 - Math.exp(-roc / 7.5));

  /* balance along the fuselage, +z forward */
  const noseZ = s.fuseLen / 2 + .28, pitZ = -s.fuseLen * .13, tailZ = -s.fuseLen * .66;
  const mEng = 25 + powerKW * .55, mTail = mass * .07, mCrew = 95;
  const mWing = S * 9.5 * Math.pow(s.wings, .4);
  const mBody = Math.max(40, mass - mEng - mTail - mCrew - mWing - fuel);
  const fixed = [[mEng, noseZ - s.fuseLen * .10], [mCrew, pitZ],
    [mBody, -s.fuseLen * .04], [mTail, tailZ]];
  const Wo = fixed.reduce((a, i) => a + i[0], 0);
  const Mo = fixed.reduce((a, i) => a + i[0] * i[1], 0);
  const mw = mWing + fuel;
  const Sh = s.stabSpan * s.fuseLen * .175;
  const Vh = Sh * (acZ - tailZ) / (S * MAC);
  const npZ = acZ - MAC * (.03 + .32 * Vh);
  const dNP = acZ - npZ;                 // a.c. -> neutral point distance
  /* place the wing so the CG lands a healthy 12% MAC ahead of the neutral
     point, whatever the tail volume of this particular airframe is */
  const Wt = Wo + mw;
  const zwTarget = (Mo + Wt * (dNP - .12 * MAC)) / Wo;
  const cgZ = (Mo + mw * acZ) / Wt;

  const r2 = (v, n = 2) => Math.round(v * 10 ** n) / 10 ** n;
  const clAlpha = 2 * Math.PI * AR / (AR + 2);          // 3-D lift slope, per rad
  const sm = (cgZ - npZ) / MAC;
  return {
    wingAreaM2: r2(S), spanM: r2(b), aspectRatio: r2(AR), macM: r2(MAC),
    massEmptyKg: Math.round(massEmpty), fuelKg: Math.round(fuel), massKg: Math.round(mass),
    wingLoading: r2(mass / S, 1), powerKW: Math.round(powerKW), powerHp: Math.round(powerKW * 1.341),
    powerLoading: r2(mass / powerKW, 2), staticThrustN: Math.round(thrust), propEff: r2(etaP),
    cd0: r2(cd0, 4), oswald: r2(e), clMax: r2(clMax),
    vStallMs: r2(vStall, 1), vStallKmh: Math.round(vStall * 3.6),
    vMaxMs: r2(vMax, 1), vMaxKmh: Math.round(vMax * 3.6),
    vCruiseKmh: Math.round(vMax * .75 * 3.6), vCornerKmh: Math.round(vCorner * 3.6),
    rocMs: r2(roc, 2), vBestClimbKmh: Math.round(vy * 3.6), ceilingM: Math.round(ceiling),
    enduranceH: r2(endurance, 2), rangeKm: Math.round(range),
    nLimit: r2(nLimit, 1), nInstant: r2(nInstBest, 2), nSustained: r2(nSusBest, 2),
    turnRateDeg: r2(turnInst, 1), turnSustainedDeg: r2(turnSus, 1),
    rollRateDeg: r2(rollRate, 1), wingShift: r2(zwTarget - acZ, 3),
    cgZ: r2(cgZ, 3), acZ: r2(acZ, 3), npZ: r2(npZ, 3),
    cgPctMAC: r2((acZ + MAC * .25 - cgZ) / MAC, 3),
    staticMargin: r2(sm, 3), tailVolume: r2(Vh),
    clAlphaPerRad: r2(clAlpha), inducedK: r2(1 / (Math.PI * AR * e), 4),
    cmAlphaPerRad: r2(-sm * clAlpha, 3), vNeKmh: Math.round(vMax * 1.35 * 3.6),
    ixx: Math.round(mass * Math.pow(b * .215, 2)),
    iyy: Math.round(mass * Math.pow(s.fuseLen * .30, 2)),
    izz: Math.round(mass * (Math.pow(b * .215, 2) + Math.pow(s.fuseLen * .30, 2) * .82)),
  };
}

/* ============================ spec generation ============================ */
function buildSpec(era, seed) {
  const r = mulberry(seed * 2654435761 + era.id.length * 7919);
  const year = Math.round(rnd(r, ...era.year));
  /* triplanes existed only c. 1916-1918 (Sopwith Triplane, Fokker Dr.I) */
  let wings = weighted(r, era.wings);
  if (wings === 3 && year > 1918) wings = 2;
  const span = rnd(r, ...era.span) * (wings === 1 ? 1.06 : wings === 3 ? 0.92 : 1);
  const [body, accent] = pick(r, era.palette);
  const engine = pick(r, era.engines);
  const gear = pick(r, era.gears);
  const cockpit = pick(r, era.cockpit);
  const power = rnd(r, ...era.power);
  const chord = rnd(r, 1.15, 1.5) * (wings === 3 ? 0.84 : wings === 1 ? 1.24 : 1) * (era.chordMul || 1);

  const spec = {
    year,
    era: era.name,
    eraId: era.id,
    seed,
    name: pick(r, NAME_A),
    mark: (d => d + (d.endsWith('-') ? '' : ' '))(pick(r, era.designations)) + (1 + Math.floor(r() * 9)) +
      (chance(r, .35) ? String.fromCharCode(65 + Math.floor(r() * 4)) : ''),
    wings, span, chord,
    fuseLen: rnd(r, ...era.fuse),
    fuseR: rnd(r, 0.38, 0.52) * (era.id === 'warbird' ? 1.1 : 1),
    stagger: wings > 1 ? rnd(r, 0.25, 0.55) : 0,
    dihedral: rnd(r, 0.0, 0.07),
    taper: rnd(r, 0.72, 1.0),
    sweep: era.id === 'warbird' || era.id === 'streamline' ? rnd(r, 0.04, 0.16) : rnd(r, 0, 0.05),
    engine, gear, cockpit, power,
    propBlades: pick(r, era.propBlades),
    finHeight: rnd(r, 0.8, 1.15),
    stabSpan: span * rnd(r, .30, .38),
    roundels: chance(r, era.id === 'golden' ? .3 : .8),
    colors: { body, accent },
  };
  spec.guns = era.id === 'war' ? 'cowl' : era.id === 'warbird' ? 'wing'
    : era.id === 'streamline' && chance(r, .5) ? 'cowl' : 'none';

  /* wing levels — one source of truth for geometry AND aerodynamics */
  const R = spec.fuseR;
  /* stance: every era gets real leg length, so the gear reads as legs and not
     as splayed-out wires. Retract eras stand taller for the big props. */
  const BASE = R + (era.id === 'warbird' ? 1.05 : era.id === 'streamline' ? .95 : .88);
  spec.BASE = BASE;
  const lowWing = era.id === 'warbird' || era.id === 'streamline';
  if (wings === 1) {
    spec.levels = [{ y: lowWing ? BASE - R * .45 : BASE + R + .40, z: .10,
      span, chord, mount: lowWing ? 'low' : 'parasol' }];
  } else if (wings === 2) {
    spec.levels = [
      { y: BASE - R * .5, z: -spec.stagger / 2, span: span * .9, chord, mount: 'low' },
      { y: BASE + R + .5, z: spec.stagger / 2, span, chord, mount: 'upper' },
    ];
  } else {
    spec.levels = [
      { y: BASE - R * .55, z: -spec.stagger * .6, span: span * .82, chord, mount: 'low' },
      { y: BASE + R * .55, z: 0, span: span * .95, chord, mount: 'mid' },
      { y: BASE + R + .9, z: spec.stagger * .6, span, chord, mount: 'upper' },
    ];
  }

  /* stats are a read-out of the flight model, so looks and numbers agree */
  spec.flight = flightModel(spec);
  /* shift the wing so the aircraft actually balances, then re-solve */
  const shift = Math.max(-.9, Math.min(.9, spec.flight.wingShift));
  spec.wingShift = Math.round(shift * 1000) / 1000;
  spec.levels.forEach(l => { l.z += shift; });
  spec.flight = flightModel(spec);
  const F = spec.flight;
  const map = (v, a, b, k = .72) =>
    Math.max(1, Math.min(99, Math.round(1 + 98 * Math.pow(Math.max(0, (v - a) / (b - a)), k))));
  spec.stats = {
    speed: map(F.vMaxKmh, 60, 700, .5),
    climb: map(F.rocMs, .3, 22, .6),
    agility: map(F.turnSustainedDeg * .5 + F.turnRateDeg * .3 + F.rollRateDeg * .2, 6, 48),
    strength: map(F.nLimit, 2.6, 7),
    range: map(F.rangeKm, 100, 1600, .8),
  };
  spec.tris = 0;
  return spec;
}

/* ============================== geometry =============================== */
function buildPlane(spec) {
  const C = spec.colors;
  const M = {
    body:  new THREE.MeshStandardMaterial({ name: 'body', color: C.body, roughness: .74, metalness: spec.year > 1932 ? .3 : .06, flatShading: true }),
    accent:new THREE.MeshStandardMaterial({ name: 'accent', color: C.accent, roughness: .56, metalness: .1, flatShading: true }),
    wood:  new THREE.MeshStandardMaterial({ name: 'wood', color: 0xa5713c, roughness: .6, flatShading: true }),
    metal: new THREE.MeshStandardMaterial({ name: 'metal', color: 0x8e9299, roughness: .34, metalness: .42, flatShading: true }),
    dark:  new THREE.MeshStandardMaterial({ name: 'dark', color: 0x2a2724, roughness: .85, flatShading: true }),
    leather: new THREE.MeshStandardMaterial({ name: 'leather', color: 0x6b4a2f, roughness: .75, flatShading: true }),
    skin:  new THREE.MeshStandardMaterial({ name: 'skin', color: 0xd9a97f, roughness: .8, flatShading: true }),
    glass: new THREE.MeshStandardMaterial({ name: 'glass', color: 0xa8c4cc, roughness: .18, metalness: .1, transparent: true, opacity: .42, flatShading: true }),
    blur:  new THREE.MeshStandardMaterial({ name: 'prop_blur', color: 0x8a7350, roughness: .9, transparent: true, opacity: .2, side: THREE.DoubleSide, depthWrite: false }),
  };

  const g = new THREE.Group();
  g.name = 'aircraft';
  const add = (m, name, parent) => { m.name = name; (parent || g).add(m); return m; };
  const box = (w, h, d, mat) => new THREE.Mesh(new THREE.BoxGeometry(w, h, d), mat);
  const cyl = (rt, rb, h, mat, seg = 8) => new THREE.Mesh(new THREE.CylinderGeometry(rt, rb, h, seg), mat);
  function strut(a, b, rad, mat, name, seg = 6, parent) {
    const A = new THREE.Vector3(...a), B = new THREE.Vector3(...b);
    const m = new THREE.Mesh(new THREE.CylinderGeometry(rad, rad, A.distanceTo(B), seg), mat);
    m.position.copy(A).add(B).multiplyScalar(.5);
    m.quaternion.setFromUnitVectors(new THREE.Vector3(0, 1, 0), B.clone().sub(A).normalize());
    return add(m, name, parent);
  }

  const L = spec.fuseLen, R = spec.fuseR;
  const BASE = spec.BASE;            // fuselage centreline height
  const NOSE = L / 2 + 0.28;

  /* fuselage */
  const fus = cyl(R, R * .62, L, M.body, spec.year > 1932 ? 12 : 8);
  fus.geometry.rotateY(Math.PI / 8);
  fus.rotation.x = Math.PI / 2;
  fus.position.set(0, BASE, -0.1);
  add(fus, 'fuselage');
  // cone runs far enough aft that the whole tail group sits on solid structure
  const tailCone = cyl(R * .62, R * .22, L * .28, M.body, spec.year > 1932 ? 12 : 8);
  tailCone.geometry.rotateY(Math.PI / 8);
  tailCone.rotation.x = Math.PI / 2;
  tailCone.position.set(0, BASE, -L * .64 - 0.1);
  add(tailCone, 'tail_cone');
  const coneR = z => {                      // cone radius at a station
    const t = Math.min(1, Math.max(0, (-z - (L * .5 + .1)) / (L * .28)));
    return R * (.62 + (.22 - .62) * t);
  };
  /* engine + propeller */
  const cowlSeg = spec.engine === 'inline' ? 10 : 14;
  if (spec.engine === 'inline') {
    const nose = cyl(R * .74, R * .5, L * .22, M.body, cowlSeg);
    nose.rotation.x = Math.PI / 2;
    nose.position.set(0, BASE, NOSE - L * .1);
    add(nose, 'engine_nose');
    const spinner = cyl(0.04, R * .42, 0.42, M.accent, cowlSeg);
    spinner.rotation.x = -Math.PI / 2;
    spinner.position.set(0, BASE, NOSE + .18);
    add(spinner, 'spinner');
  } else {
    const cowl = cyl(R * .92, R * 1.04, L * .1, M.metal, cowlSeg);
    cowl.rotation.x = Math.PI / 2;
    cowl.position.set(0, BASE, NOSE - L * .05);
    add(cowl, 'engine_cowl');
    const ring = new THREE.Mesh(new THREE.TorusGeometry(R * 1.02, .055, 6, 16), M.accent);
    ring.position.set(0, BASE, NOSE);
    add(ring, 'cowl_ring');
    if (spec.engine === 'radial') {
      const n = 7;
      for (let i = 0; i < n; i++) {
        const a = (i / n) * Math.PI * 2;
        const cylr = cyl(.09, .1, .3, M.dark, 6);
        cylr.rotation.x = Math.PI / 2;
        cylr.position.set(Math.cos(a) * R * .62, BASE + Math.sin(a) * R * .62, NOSE - .06);
        add(cylr, 'cylinder_' + i);
      }
    }
  }

  const propGroup = new THREE.Group();
  propGroup.name = 'propeller';
  propGroup.position.set(0, BASE, NOSE + (spec.engine === 'inline' ? .4 : .18));
  g.add(propGroup);
  const hub = cyl(.11, .13, .2, M.metal, 10);
  hub.rotation.x = Math.PI / 2;
  add(hub, 'prop_hub', propGroup);
  // tipR is the true blade-tip radius: it must clear the ground under the hub
  const tipR = Math.min(BASE - .14, .52 + spec.power * .52);
  const bladeLen = tipR / 1.695;
  for (let i = 0; i < spec.propBlades; i++) {
    const bg = new THREE.BoxGeometry(.2, bladeLen * 1.55, .07);
    const p = bg.attributes.position;
    for (let k = 0; k < p.count; k++) {
      const t = (p.getY(k) + bladeLen * .775) / (bladeLen * 1.55);
      const s = 1 - .45 * t;
      p.setX(k, p.getX(k) * s); p.setZ(k, p.getZ(k) * s);
    }
    bg.computeVertexNormals();
    bg.rotateY(.3);
    bg.translate(0, bladeLen * .92, 0);
    bg.rotateZ((i / spec.propBlades) * Math.PI * 2);
    add(new THREE.Mesh(bg, spec.year > 1932 ? M.metal : M.wood), 'prop_blade_' + i, propGroup);
  }

  /* wing stack — shared with the aerodynamic model */
  const levels = spec.levels;

  const TAPER = spec.taper, SWEEP = spec.sweep, DIH = spec.dihedral;
  /* the one planform transform: every part that touches a wing uses it */
  const wingPt = (lv, x, zFrac, dy = 0) => {
    const t = Math.min(1, Math.abs(x) / (lv.span / 2));
    return [x, lv.y + DIH * Math.abs(x) + dy,
      lv.z + zFrac * lv.chord * (1 - (1 - TAPER) * t) - SWEEP * t * lv.chord];
  };
  const warp = (geom, lv, zOff = 0) => {
    const p = geom.attributes.position;
    for (let k = 0; k < p.count; k++) {
      const x = p.getX(k), t = Math.min(1, Math.abs(x) / (lv.span / 2));
      p.setZ(k, (p.getZ(k) + zOff) * (1 - (1 - TAPER) * t) - SWEEP * t * lv.chord);
      p.setY(k, p.getY(k) + DIH * Math.abs(x));
    }
    geom.computeVertexNormals();
  };
  const localChord = (lv, x) =>
    lv.chord * (1 - (1 - TAPER) * Math.min(1, Math.abs(x) / (lv.span / 2)));

  /* fuselage skin: local radius and a point ON the skin, so every fitting lands
     on real structure instead of hanging in the air next to it */
  const fuseRAt = z => {
    const t = Math.min(1, Math.max(0, (L / 2 - .1 - z) / L));
    return R * (1 - .38 * t);
  };
  const fuseSurf = (x, z, up) => {
    const rr = fuseRAt(z);
    const xr = Math.sign(x) * Math.min(Math.abs(x), rr * .9);
    const dy = Math.sqrt(Math.max(.01, rr * rr - xr * xr)) * .94;
    return [xr, BASE + (up ? dy : -dy), z];
  };

  levels.forEach((lv, i) => {
    // segmented along the span, otherwise taper/sweep/dihedral would just
    // interpolate tip-to-tip and the real surface would miss every fitting
    const w = new THREE.Mesh(new THREE.BoxGeometry(lv.span, .13, lv.chord, 16, 1, 1), M.body);
    warp(w.geometry, lv);
    w.position.set(0, lv.y, lv.z);
    add(w, 'wing_' + i);
    const le = new THREE.Mesh(new THREE.BoxGeometry(lv.span, .15, .12, 16, 1, 1), M.accent);
    warp(le.geometry, lv, lv.chord / 2 - .055);
    le.position.set(0, lv.y, lv.z);
    add(le, 'wing_' + i + '_leading_edge');
    if (spec.roundels) {
      for (const s of [1, -1]) {
        const rd = cyl(lv.chord * .2, lv.chord * .2, .05, M.accent, 14);
        rd.position.set(...wingPt(lv, s * lv.span * .30, 0, .045));
        rd.rotation.z = s * Math.atan(DIH);
        add(rd, 'roundel_' + i + '_' + (s > 0 ? 'r' : 'l'));
      }
    }
  });

  /* interplane struts + rigging */
  for (let i = 0; i < levels.length - 1; i++) {
    const lo = levels[i], up = levels[i + 1];
    const bay = Math.min(lo.span, up.span) * .29;
    for (const s of [1, -1]) {
      const tag = (s > 0 ? 'r' : 'l') + i;
      const out = Math.min(lo.span, up.span) * .46;
      strut(wingPt(lo, s * bay, -.30, .055), wingPt(up, s * bay, -.30, -.055), .05, M.wood, 'strut_' + tag + '_rear');
      strut(wingPt(lo, s * bay, .30, .055), wingPt(up, s * bay, .30, -.055), .05, M.wood, 'strut_' + tag + '_front');
      strut(wingPt(lo, s * bay, 0, .05), wingPt(up, s * out, 0, -.05), .012, M.metal, 'wire_' + tag + '_a', 4);
      strut(wingPt(lo, s * out, 0, .05), wingPt(up, s * bay, 0, -.05), .012, M.metal, 'wire_' + tag + '_b', 4);
    }
  }

  const top = levels[levels.length - 1];
  /* cabanes: carry any wing that sits clear above the fuselage */
  if (top.y > BASE + R * .3) {
    for (const s of [1, -1]) {
      const tag = s > 0 ? 'r' : 'l';
      strut(fuseSurf(s * R * .55, .34, true), wingPt(top, s * R * .85, .30, -.055), .05, M.wood, 'cabane_' + tag + '_front');
      strut(fuseSurf(s * R * .55, -.34, true), wingPt(top, s * R * .85, -.30, -.055), .05, M.wood, 'cabane_' + tag + '_rear');
    }
    if (spec.wings === 1) {          // parasol: lift struts to the lower fuselage
      for (const s of [1, -1]) {
        strut(fuseSurf(s * R * .55, .12, false), wingPt(top, s * top.span * .22, 0, -.055),
          .055, M.metal, 'lift_strut_' + (s > 0 ? 'r' : 'l'));
      }
    }
  } else if (spec.wings === 1) {     // low wing: fillet the root join
    const fair = box(R * 2.15, R * .85, top.chord * 1.1, M.body);
    fair.position.set(0, top.y + R * .2, top.z);
    add(fair, 'wing_root_fairing');
  }

  /* ailerons: hinged on the real trailing edge, aligned to the planform */
  for (const s of [1, -1]) {
    const xm = top.span * .33, ch = localChord(top, xm);
    const ap = new THREE.Group();
    ap.name = s > 0 ? 'aileron_right_pivot' : 'aileron_left_pivot';
    ap.position.set(...wingPt(top, s * xm, -.5, 0));
    ap.rotation.z = s * Math.atan(DIH);
    ap.rotation.y = s * Math.atan(SWEEP * top.chord * 2 / top.span);
    g.add(ap);
    const ail = box(top.span * .28, .1, ch * .26, M.accent);
    ail.position.set(0, 0, -ch * .11);          // overlaps the wing by ~2 cm
    add(ail, s > 0 ? 'aileron_right' : 'aileron_left', ap);
  }

  /* cockpit */
  const pitZ = -L * .13;
  /* canopy geometry kept in variables: the pilot's headroom is computed from it */
  const cvRad = R * .85, cvScaleY = 1.3, cvScaleZ = 1.75, cvY = BASE + R * .45;
  if (spec.cockpit === 'canopy') {
    const canopy = new THREE.Mesh(new THREE.SphereGeometry(cvRad, 12, 7, 0, Math.PI * 2, 0, Math.PI / 2), M.glass);
    canopy.scale.set(.9, cvScaleY, cvScaleZ);
    canopy.position.set(0, cvY, pitZ);        // base well inside the skin
    add(canopy, 'canopy');
    const wsc = box(R * 1.06, R * .5, .06, M.dark);
    wsc.position.set(0, BASE + R * .68, pitZ + R * 1.3);
    wsc.rotation.x = -.5;
    add(wsc, 'windscreen');
    const frame = box(R * 1.3, .05, .05, M.dark);
    frame.position.set(0, BASE + R * .5, pitZ);
    add(frame, 'canopy_frame');
    /* turtledeck fairs the bubble into the fin instead of leaving it a blob */
    const sz0 = pitZ - R * .9, sz1 = -L * .66;
    const spine = cyl(R * .58, R * .18, Math.abs(sz1 - sz0), M.body, 7);
    spine.rotation.x = Math.PI / 2;
    spine.position.set(0, BASE + R * .28, (sz0 + sz1) / 2);
    add(spine, 'turtledeck');
  } else {
    const rim = new THREE.Mesh(new THREE.TorusGeometry(R * .62, .055, 6, 14), M.wood);
    rim.rotation.x = Math.PI / 2;
    rim.position.set(0, BASE + R * .78, pitZ);
    add(rim, 'cockpit_rim');
    const hole = cyl(R * .56, R * .46, .3, M.dark, 12);
    hole.position.set(0, BASE + R * .62, pitZ);
    add(hole, 'cockpit_interior');
    const screen = box(R * .8, .14, .04, M.dark);
    screen.position.set(0, BASE + R * .9, pitZ + R * .7);
    screen.rotation.x = -.25;
    add(screen, 'windscreen');
    const headrest = box(R * .78, .22, .26, M.body);
    headrest.position.set(0, BASE + R * .72, pitZ - R * 1.05);
    add(headrest, 'headrest');
  }

  /* pilot: under a canopy the seat height comes from the dome, not by eye —
     the helmet must clear the glass at its own station by 6 cm */
  const HELMET_TOP = .615;                       // local y of the helmet crown
  let pilotY = BASE + R * .3;
  if (spec.cockpit === 'canopy') {
    const dz = .17;                              // helmet station + half depth
    const domeH = cvY + cvRad * cvScaleY * Math.sqrt(Math.max(0, 1 - (dz / (cvRad * cvScaleZ)) ** 2));
    pilotY = Math.min(BASE + R * .05, domeH - HELMET_TOP - .06);
  }
  const pilot = new THREE.Group();
  pilot.name = 'pilot';
  pilot.position.set(0, pilotY, pitZ - .04);
  g.add(pilot);
  const torso = box(.34, .4, .28, M.leather);
  torso.position.set(0, .18, 0);
  add(torso, 'pilot_torso', pilot);
  const headPivot = new THREE.Group();
  headPivot.name = 'pilot_head_pivot';
  headPivot.position.set(0, .46, .02);
  pilot.add(headPivot);
  add(box(.22, .24, .22, M.skin), 'pilot_head', headPivot);
  const helmet = box(.25, .15, .25, M.leather);
  helmet.position.set(0, .08, 0);
  add(helmet, 'pilot_helmet', headPivot);
  const goggles = box(.26, .07, .05, M.dark);
  goggles.position.set(0, .03, .12);
  add(goggles, 'pilot_goggles', headPivot);
  if (spec.cockpit === 'open') {
    const scarf = box(.3, .09, .26, M.accent);
    scarf.position.set(0, .36, 0);
    add(scarf, 'pilot_scarf', pilot);
    const tail = box(.14, .5, .08, M.accent);
    tail.position.set(.12, .18, -.24);
    add(tail, 'pilot_scarf_tail', pilot);
  }
  const armPivot = new THREE.Group();
  armPivot.name = 'pilot_arm_pivot';
  armPivot.position.set(.17, .3, .04);
  pilot.add(armPivot);
  const arm = box(.11, .34, .11, M.leather);
  arm.position.set(0, -.17, .05);
  arm.rotation.x = -.35;
  add(arm, 'pilot_arm', armPivot);

  /* tail */
  const TAILZ = -L * .66;
  const fin = box(.1, spec.finHeight, L * .17, M.body);
  {
    const p = fin.geometry.attributes.position;
    for (let k = 0; k < p.count; k++) {
      const t = (p.getY(k) + spec.finHeight / 2) / spec.finHeight;
      p.setZ(k, p.getZ(k) * (1 - .5 * t) - L * .035 * t);
    }
    fin.geometry.computeVertexNormals();
  }
  fin.position.set(0, BASE - coneR(TAILZ) * .5 + spec.finHeight / 2, TAILZ);
  add(fin, 'vertical_fin');

  const stab = box(spec.stabSpan, .1, L * .12, M.body);
  stab.position.set(0, BASE - R * .05, TAILZ - .05);
  add(stab, 'horizontal_stabilizer');
  const fairing = box(.34, coneR(TAILZ) * 1.5, L * .14, M.body);
  fairing.position.set(0, BASE - R * .02, TAILZ - .05);
  add(fairing, 'tailplane_fairing');

  const elevPivot = new THREE.Group();
  elevPivot.name = 'elevator_pivot';
  elevPivot.position.set(0, BASE - R * .05, TAILZ - .05 - L * .058);   // stab trailing edge
  g.add(elevPivot);
  const elev = box(spec.stabSpan, .1, L * .055, M.accent);
  elev.position.set(0, 0, -L * .024);
  add(elev, 'elevator', elevPivot);

  const rudderPivot = new THREE.Group();
  rudderPivot.name = 'rudder_pivot';
  rudderPivot.position.set(0, fin.position.y, TAILZ - L * .080);        // fin trailing edge
  g.add(rudderPivot);
  const rudder = box(.11, spec.finHeight * .92, L * .075, M.accent);
  rudder.position.set(0, 0, -L * .034);
  add(rudder, 'rudder', rudderPivot);
  add(cyl(.055, .055, spec.finHeight * .92, M.accent, 8), 'rudder_hinge', rudderPivot);

  /* landing gear */
  const suspension = new THREE.Group();
  suspension.name = 'gear_suspension';
  g.add(suspension);
  const wheelR = spec.gear === 'retract' ? .3 : .34;
  const lg = levels[0];
  // tyre bottom lands exactly on y=0, so nothing sinks through the ground
  const axleY = wheelR + .08;
  const track = R + (spec.gear === 'retract' ? .62 : .40);
  const axleZ = spec.gear === 'retract' ? lg.z + lg.chord * .5 : L * .20;
  /* eras 1-4: legs hang from the LOWER WING when it is at gear height,
     otherwise from the fuselage longeron — a real V of two struts plus a
     spreader, all meeting at the axle. */
  const legTop = (s, dz) => {
    if (spec.gear !== 'retract' && lg.y < BASE + R * .1 && Math.abs(lg.z - axleZ) < lg.chord * 1.4
        && Math.abs(track) < lg.span * .45)
      return wingPt(lg, s * Math.min(track, lg.span * .42), dz > 0 ? .34 : -.34, -.06);
    return fuseSurf(s * R * .45, axleZ + dz, false);
  };
  if (spec.gear === 'retract') {
    for (const s of [1, -1]) {
      const tag = s > 0 ? 'r' : 'l';
      const topPt = wingPt(lg, s * track, .05, -.05);   // leg hangs from the real wing
      strut(topPt, [s * track, axleY, axleZ], .075, M.metal, 'gear_leg_' + tag, 8, suspension);
      const door = box(.09, .44, lg.chord * .42, M.body);
      door.position.set(s * (track + .1), topPt[1] - .2, topPt[2] - lg.chord * .06);
      add(door, 'gear_door_' + tag, suspension);
    }
  } else if (spec.gear === 'skid') {
    for (const s of [1, -1]) {
      const tag = s > 0 ? 'r' : 'l';
      const sx = s * (track + .21);                    // runner sits beside the tyre
      const skid = box(.12, .1, L * .5, M.wood);
      skid.position.set(sx, .05, axleZ - L * .1);
      add(skid, 'skid_' + tag, suspension);
      strut(legTop(s, .46), [sx, .1, axleZ + L * .1], .05, M.wood, 'skid_strut_' + tag + '_f', 6, suspension);
      strut(legTop(s, -.52), [sx, .1, axleZ - L * .22], .05, M.wood, 'skid_strut_' + tag + '_r', 6, suspension);
      strut([sx, .1, axleZ], [s * track, axleY, axleZ], .05, M.wood, 'skid_axle_' + tag, 6, suspension);
      strut([sx, .1, axleZ + L * .1], [s * track, axleY, axleZ], .04, M.wood, 'skid_brace_' + tag, 4, suspension);
    }
  } else {
    for (const s of [1, -1]) {
      const tag = s > 0 ? 'r' : 'l';
      const fwd = legTop(s, .46), aft = legTop(s, -.52);
      strut(fwd, [s * track, axleY, axleZ], .055, M.metal, 'gear_' + tag + '_front', 6, suspension);
      strut(aft, [s * track, axleY, axleZ], .055, M.metal, 'gear_' + tag + '_rear', 6, suspension);
      strut(aft, fwd, .04, M.metal, 'gear_' + tag + '_top', 6, suspension);
      strut([s * track, axleY, axleZ], [s * R * .2, axleY + .06, axleZ], .028, M.metal, 'gear_' + tag + '_spreader', 4, suspension);
      const bungee = cyl(.035, .035, .2, M.leather, 6);
      bungee.position.set(s * (track - .2), axleY + .13, axleZ);
      add(bungee, 'bungee_' + tag, suspension);
    }
    const axle = cyl(.045, .045, track * 2, M.metal, 6);
    axle.rotation.z = Math.PI / 2;
    axle.position.set(0, axleY, axleZ);
    add(axle, 'axle', suspension);
  }

  for (const s of [1, -1]) {
    const wheel = new THREE.Group();
    wheel.name = s > 0 ? 'wheel_right' : 'wheel_left';
    wheel.position.set(s * track, axleY, axleZ);
    wheel.rotation.y = Math.PI / 2;
    suspension.add(wheel);
    add(new THREE.Mesh(new THREE.TorusGeometry(wheelR, .09, 8, 18), M.dark), 'tire', wheel);
    const rim = cyl(wheelR * .8, wheelR * .8, .06, spec.gear === 'skid' ? M.wood : M.accent, 18);
    rim.geometry.rotateX(Math.PI / 2);
    add(rim, 'rim', wheel);
    const whub = cyl(.07, .07, .13, M.metal, 8);
    whub.geometry.rotateX(Math.PI / 2);
    add(whub, 'wheel_hub', wheel);
    if (spec.gear === 'spoked' || spec.gear === 'skid') {
      for (let i = 0; i < 8; i++) {
        const sp = cyl(.016, .016, wheelR * 1.66, M.metal, 4);
        sp.rotation.z = (i / 8) * Math.PI;
        add(sp, 'spoke_' + i, wheel);
      }
    } else if (spec.gear === 'spatted') {
      const spat = new THREE.Mesh(new THREE.SphereGeometry(wheelR * 1.15, 8, 6), M.body);
      spat.scale.set(.55, 1, 1.35);
      spat.rotation.y = Math.PI / 2;
      add(spat, 'wheel_spat', wheel);
    }
  }

  /* tail skid or wheel */
  if (spec.gear === 'retract') {
    const tw = new THREE.Mesh(new THREE.TorusGeometry(.13, .06, 6, 12), M.dark);
    tw.rotation.y = Math.PI / 2;
    tw.position.set(0, .182, TAILZ + .2);
    add(tw, 'tail_wheel');
    strut([0, BASE - R * .5, TAILZ + .2], [0, .182, TAILZ + .2], .045, M.metal, 'tail_wheel_leg');
  } else {
    strut([0, BASE - coneR(TAILZ) * .8, TAILZ], [0, .1, TAILZ + .35], .05, M.wood, 'tail_skid');
    const shoe = box(.1, .1, .42, M.dark);
    shoe.position.set(0, .042, TAILZ + .35);   // mains and skid both touch y=0
    add(shoe, 'tail_skid_shoe');
  }

  /* era hardware — every piece bolted to existing structure */
  if (spec.engine === 'inline' && spec.year > 1922) {
    for (const s of [1, -1]) for (let i = 0; i < 4; i++) {
      const ex = cyl(.045, .05, .18, M.dark, 6);
      ex.rotation.z = Math.PI / 2;
      ex.position.set(s * R * .52, BASE + R * .22, NOSE - L * .07 - i * .21);
      add(ex, 'exhaust_' + (s > 0 ? 'r' : 'l') + '_' + i);
    }
    if (spec.eraId === 'warbird') {
      const scoop = box(R * 1.05, R * .5, L * .2, M.body);
      scoop.position.set(0, BASE - R * .82, -L * .06);
      add(scoop, 'radiator_scoop');
    }
  }
  if (spec.guns === 'cowl') {
    for (const s of (spec.eraId === 'war' ? [0] : [.5, -.5])) {
      const gun = box(.09, .09, L * .26, M.dark);
      gun.position.set(s * R, BASE + R * .99, pitZ + L * .26);
      add(gun, 'gun_cowl_' + (s > 0 ? 'r' : s < 0 ? 'l' : 'c'));
    }
  } else if (spec.guns === 'wing') {
    for (const s of [1, -1]) for (let i = 0; i < 2; i++) {
      const bar = cyl(.035, .035, .5, M.dark, 6);
      bar.rotation.x = Math.PI / 2;
      const [x, y, z] = wingPt(top, s * top.span * (.24 + i * .09), .5, -.01);
      bar.position.set(x, y, z + .16);
      add(bar, 'gun_wing_' + (s > 0 ? 'r' : 'l') + i);
    }
  }

  /* ground the model exactly */
  const bb = new THREE.Box3().setFromObject(g);
  g.position.y -= bb.min.y;

  /* blur disc added after grounding so it never affects the bounding box */
  const blurDisc = new THREE.Mesh(new THREE.CircleGeometry(tipR, 24), M.blur);
  blurDisc.name = 'prop_blur_disc';
  blurDisc.position.set(0, 0, .02);
  blurDisc.visible = false;

  let tris = 0;
  g.traverse(o => { if (o.isMesh) tris += o.geometry.index ? o.geometry.index.count / 3 : o.geometry.attributes.position.count / 3; });
  spec.tris = Math.round(tris);

  return { group: g, propGroup, blurDisc, suspension, headPivot, armPivot, rudderPivot, elevPivot };
}

/* ====================== simulation config (per aircraft) ======================
   Everything a 6-DOF or arcade flight model needs, in SI units. Coefficients,
   not just results — so two aircraft fly differently from the same equations. */
function simConfig(s) {
  const F = s.flight, b = F.spanM, S = F.wingAreaM2, AR = F.aspectRatio, MAC = F.macM;
  const r3 = v => Math.round(v * 1000) / 1000;
  const clAlpha = 2 * Math.PI * AR / (AR + 2) * (s.wings > 1 ? .92 : 1);   // per rad
  const tailArm = F.acZ + s.fuseLen * .66;
  const Sh = s.stabSpan * s.fuseLen * .175, Sv = s.finHeight * s.fuseLen * .16;
  return {
    id: s.eraId + '-' + s.seed, name: s.name, era: s.era, year: s.year,
    seed: s.seed, config: s.wings === 1 ? 'monoplane' : s.wings === 2 ? 'biplane' : 'triplane',

    geometry: { wingAreaM2: S, spanM: b, macM: MAC, aspectRatio: AR,
      taper: r3(s.taper), sweepDeg: r3(Math.atan(s.sweep) * 57.3),
      dihedralDeg: r3(Math.atan(s.dihedral) * 57.3),
      fuseLenM: r3(s.fuseLen), fuseRadiusM: r3(s.fuseR),
      wheelTrackM: r3((s.fuseR + (s.gear === 'retract' ? .62 : .40)) * 2),
      hStabAreaM2: r3(Sh), vStabAreaM2: r3(Sv), tailArmM: r3(tailArm) },

    mass: { massKg: F.massKg, emptyKg: F.massEmptyKg, fuelKg: F.fuelKg,
      cgZ: F.cgZ, cgPctMAC: F.cgPctMAC, aeroCentreZ: F.acZ, neutralPointZ: F.npZ,
      staticMarginMAC: F.staticMargin,
      inertiaKgM2: { ixx: F.ixx, iyy: F.iyy, izz: F.izz, ixz: Math.round(F.ixx * .04) } },

    aero: { cd0: F.cd0, oswaldE: F.oswald, k: r3(1 / (Math.PI * AR * F.oswald)),
      clMax: F.clMax, clAlphaPerRad: r3(clAlpha), cl0: r3(.08 + s.wings * .02),
      alphaStallDeg: r3(F.clMax / clAlpha * 57.3),
      cm0: r3(-.04 - s.wings * .01), cmAlphaPerRad: r3(-clAlpha * F.staticMargin),
      cmq: r3(-8 * F.tailVolume), clq: r3(3.2 * F.tailVolume),
      cyBeta: r3(-.28 - Sv / S * 1.9), cnBeta: r3(.055 + Sv * tailArm / (S * b) * 1.6),
      clBeta: r3(-.06 - s.dihedral * .9 - (s.wings - 1) * .01),
      clp: r3(-.42 - AR * .012), cnr: r3(-.06 - Sv * tailArm / (S * b) * .9) },

    control: { elevatorCmDelta: r3(-1.05 * F.tailVolume * 2.4), elevatorMaxDeg: 25,
      aileronClDelta: r3(.16 + s.span / 40), aileronMaxDeg: 20,
      rudderCnDelta: r3(.07 + Sv * tailArm / (S * b) * 1.1), rudderMaxDeg: 25,
      rollRateDegS: F.rollRateDeg, trimAlphaDeg: r3(2.4 + s.wings * .3) },

    propulsion: { type: s.engine, powerKW: F.powerKW, powerHp: F.powerHp,
      propBlades: s.propBlades, propDiameterM: r3(Math.min(s.BASE - .18, .52 + s.power * .52) * 2),
      propEfficiency: F.propEff, staticThrustN: F.staticThrustN,
      fuelBurnKgPerH: r3(F.fuelKg / F.enduranceH),
      gyroTorqueNm: s.engine === 'rotary' ? 180 : 40 },

    gear: { type: s.gear, retractable: s.gear === 'retract',
      tailSkid: s.gear !== 'retract', cdRetractGain: s.gear === 'retract' ? .012 : 0,
      groundFrictionRoll: .035, groundFrictionBrake: .38 },

    envelope: { vStallMs: F.vStallMs, vMaxMs: F.vMaxMs, vNeMs: r3(F.vMaxMs * 1.22),
      vCruiseMs: r3(F.vMaxKmh * .75 / 3.6), vBestClimbMs: r3(F.vBestClimbKmh / 3.6),
      nLimit: F.nLimit, nLimitNeg: r3(-F.nLimit * .45),
      ceilingM: F.ceilingM, rocMs: F.rocMs, rangeKm: F.rangeKm, enduranceH: F.enduranceH,
      turnInstantDegS: F.turnRateDeg, turnSustainedDegS: F.turnSustainedDeg },

    gameStats: s.stats, rig: RIG_NODES,
  };
}

/* ===================== joint audit (dev instrument) =====================
   every mesh must touch at least one other mesh, and nothing may sit below
   the ground plane. window.__audit(n) sweeps all eras x n seeds.          */
function auditJoints(group) {
  group.updateMatrixWorld(true);
  const parts = [];
  group.traverse(o => {
    if (!o.isMesh || o.name === 'prop_blur_disc') return;
    parts.push({ name: o.name, bb: new THREE.Box3().setFromObject(o).expandByScalar(.025) });
  });
  const orphans = [], sunken = [];
  parts.forEach((p, i) => {
    if (p.bb.min.y < -.04) sunken.push(p.name + ' y=' + p.bb.min.y.toFixed(2));
    let ok = false;
    for (let j = 0; j < parts.length && !ok; j++) if (j !== i && p.bb.intersectsBox(parts[j].bb)) ok = true;
    if (!ok) orphans.push(p.name);
  });
  return { orphans, sunken };
}
const RIG_NODES = ['propeller', 'prop_blur_disc', 'wheel_left', 'wheel_right', 'gear_suspension',
  'rudder_pivot', 'elevator_pivot', 'aileron_left_pivot', 'aileron_right_pivot',
  'pilot_head_pivot', 'pilot_arm_pivot'];

// [node name, local hinge axis] — parts the game animates; everything else is static body.
export const MOVABLE = [
  ['propeller', [0, 0, 1]],
  ['rudder_pivot', [0, 1, 0]],
  ['elevator_pivot', [1, 0, 0]],
  ['aileron_left_pivot', [1, 0, 0]],
  ['aileron_right_pivot', [1, 0, 0]],
];

export { ERAS, RIG_NODES, buildSpec, buildPlane, simConfig, flightModel, auditJoints };

export function generate(eraId, seed) {
  const era = ERAS.find(e => e.id === eraId);
  if (!era) throw new Error('unknown era ' + eraId + ' (have: ' + ERAS.map(e => e.id).join(', ') + ')');
  const spec = buildSpec(era, seed);
  const rig = buildPlane(spec);
  rig.propGroup.add(rig.blurDisc);
  rig.group.name = 'aircraft';
  return { id: eraId + '-' + seed, spec, sim: simConfig(spec), group: rig.group, rig };
}
