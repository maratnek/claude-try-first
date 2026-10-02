# tools/aircraft-gen

Procedural 1909–1945 aircraft generator. **This folder is the source of truth** for plane
geometry, rig and flight parameters. `aircraft-gen.mjs` is shared by the viewer, the exporter
and the checks — edit it here.

```
cd tools/aircraft-gen
npm ci            # once
npm run view      # http://localhost:5174 — 3D viewer: eras, seeds, stats, sim config
npm run check     # joint audit (5 eras x 40 seeds) + golden drift + stability sanity
npm run export -- --eras=all --count=4          # -> assets/models/aircraft/<era>-<seed>/
npm run export -- --eras=war --seeds=904,1337
npm run find -- --era=war --where="stats.agility>=60,config=triplane" --sort=-stats.agility --max-tris=9000
```

`find` scans seeds and returns the best matches for a brief. Conditions use paths into
{ stats, config, era, year, name } + sim.json (e.g. `envelope.vStallMs<=20`, `mass.massKg>=900`).
Exit code 2 = nothing matched (loosen the brief, don't fake numbers).

Per aircraft (`assets/models/aircraft/<era>-<seed>/`):

| file | what |
|---|---|
| body.glb | every static mesh, transforms baked, faceted normals |
| propeller.glb, rudder_pivot.glb, elevator_pivot.glb, aileron_left_pivot.glb, aileron_right_pivot.glb | movable parts, modelled around their own hinge (origin = hinge) |
| rig.txt | one line per movable part: name, hinge position, rest rotation quaternion, local hinge axis |
| sim.json | flight-sim config in SI units (geometry, mass/CG/inertia, aero coefficients, control, propulsion, gear, envelope, gameStats) |
| spec.json | raw generator spec (wing levels, colours, era, etc.) |

Plus `assets/models/aircraft/index.json` listing everything exported.

Conventions match the game: metres, Y up, nose toward +Z, wheels touch y = 0, origin on the centreline.
Node names are identical for every variant (see `RIG_NODES` / `MOVABLE` in aircraft-gen.mjs).

`golden.json` = buildSpec + simConfig output for 5 eras x seeds 1, 38, 904, 31337. Any change to the
generator that moves these numbers fails `npm run check`; if the change is intentional run
`npm run update-golden` and say so in the commit message. The C++ port must reproduce these values.
