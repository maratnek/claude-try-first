# Asset requests (3D models)

There is no automated text-to-3D generation hooked up to this project. When
the game needs a new 3D model, the `asset-planner` agent writes a request
file here instead of generating the asset itself.

## Workflow

1. `asset-planner` (or anyone) adds `pending/<slug>.md` with:
   - the exact prompt to paste into a text-to-3D tool
   - the expected output path, e.g. `assets/models/<slug>.glb`
2. A human generates the model externally and saves it at that exact path.
3. On its next run, `asset-planner` checks `pending/` requests against
   `assets/models/`. If the expected file now exists, it wires the model
   into the game's code, then moves the request file to `fulfilled/`.

This keeps the review loop human-in-the-loop for the one step nothing here
can do on its own: actually generating the mesh.

## Break data (required for every model that can crash)

Each model request must also include a **break data** section (or the model's
sidecar `<name>.break.txt`): the break groups (which named nodes move as one
unit, for example "left lower wing + its struts + wires + aileron"), each
group's mass in kg and a strength rating (low = prop/gear/tail surfaces, mid =
wings, high = engine), the joint position for each group (where it attaches to
its parent, plane space), the **fuel tank position** (`fuel_tank <x> <y> <z>
<radius>`, where the crash fireball and smoke column start, in the fuselage
behind the engine), and the weakest-first failure order. Use node names
exactly as they appear in the GLB. The file format is documented in
`design-notes/breakup-v2.md`. If a node spans both sides (a one-piece wing),
either split it in the model or mark it `@L`/`@R` in the sidecar.

## Flight config (required for every aircraft)

Each aircraft request must also include its flight parameters as a sidecar
`<name>.flight.txt` next to the model (`key=value` per line, `#` comments,
keys are the `PlaneParams` field names in `src/flight.h`; loaded by
`LoadPlaneParams`, see `assets/models/biplane-1920.flight.txt`). The file
header names the reference real aircraft and quotes only the specs known with
confidence (marked approx): top speed, stall speed, climb rate, roll rate,
glide ratio, mass, wing loading. It also lists which values are pure
game-feel tuning (for example `yawRate`, `maxTurnRate`, `controlSpeed`).
Unknown keys or unparsable values make the loader reject the whole file.
