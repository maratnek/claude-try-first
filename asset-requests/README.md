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
