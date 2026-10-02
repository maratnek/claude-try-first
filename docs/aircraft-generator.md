# Procedural aircraft — integration contract

## Source of truth

`tools/aircraft-gen/aircraft-gen.mjs` in this repo. The design project only reviews (reads the repo)
and files findings; it never pushes.

## Two paths, one source

1. **Offline (now).** `tools/aircraft-gen` (Node + three.js) exports GLB parts + rig + sim JSON into
   `assets/models/aircraft/`. The game loads them with `src/objects/aircraft.{h,cpp}`.
   Works on desktop, Web (Emscripten preload) and iOS (bundled) with zero runtime cost.
2. **Runtime (port).** `src/aircraft_gen/` — a C++ port that builds the same mesh + sim config from
   `(era, seed)` inside the game. See "C++ port" below. Until it passes the goldens, path 1 ships.

## Hard rules (from the design project — apply to every change)

- Every part is physically connected. No floating details: ailerons sit on the real trailing edge
  (with taper, sweep, dihedral), struts end on wing surfaces, tail sits on the fuselage, wheels touch
  y = 0, cowls and skins overlap the joints. New parts attach through `wingPt()` or an explicit point
  on the parent structure, never approximate coordinates. `npm run check` enforces this.
- Flight parameters (area, aspect ratio, mass, Cd0, Vmax/Vstall, climb, CG/neutral point, static
  margin, inertia) come from the same geometry — never hand-tuned per model.
- Rig node names are identical across all variants.

## Coordinates

Metres, Y up, nose +Z — identical to `PlaneState` / `GetPlaneForward` (yaw 0 → +Z). The model's
lowest point (wheels) is y = 0; `flight.cpp`'s `kWheelHeight` should become 0 for these models
(or the model is drawn at `position.y - kWheelHeight`).

## rig.txt

```
name px py pz qx qy qz qw axis_x axis_y axis_z
```
Draw a part: translate(p) · rotate(q) · rotate(angle, axis) · DrawModel(part). Positive angle is a
right-hand rotation about the local axis. Mapping stick input → angle sign is the game's decision;
verify visually (stick back → elevator trailing edge up).

## sim.json

Physics is **not** wired into `flight.cpp` yet (owner's call: JSON only for now). When it is, read
`envelope.vStallMs` → liftoff speed, `envelope.vMaxMs` → max speed, `control.rollRateDegS` →
roll rate, `envelope.turnSustainedDegS` → bank-turn rate, `propulsion.staticThrustN / mass.massKg`
→ acceleration. Keep the arcade feel; these replace constants, not the control scheme.

## C++ port (runtime path)

Staged, each stage verified before the next:

1. `aircraft_gen::BuildSpec(eraId, seed)` + `FlightModel` + `SimConfig` → must match
   `tools/aircraft-gen/golden.json` to 1e-3 for every entry. Notes: use `double` everywhere;
   the seed hash is `uint32_t(uint64_t(seed) * 2654435761ull + eraIdLen * 7919)` (JS does this in
   doubles then `>>> 0`; for seeds < 2^20 the products are exact); `mulberry` uses 32-bit
   `Math.imul` semantics → `uint32_t` multiply. Call `r()` in exactly the same order as the JS.
2. Geometry: same parts, same names, built as raylib `Mesh` (GenMeshCube/Cylinder + custom
   vertex warps for taper/sweep/dihedral). Output = body Model + the five MOVABLE parts + pivots,
   i.e. the same `AircraftModel` struct the offline loader fills, so call sites don't change.
3. Port `auditJoints` (AABB touch test + y ≥ -0.04) and run it in a debug build over 5 eras x 40
   seeds; zero orphans.

Per-run scope: one stage per run, per CLAUDE.md.
