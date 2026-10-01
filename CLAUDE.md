# Flight game — project notes for Claude

C++/raylib 3D game: fly gliders/planes through levels. Target platforms:
Web (via Emscripten, itch.io) and iOS App Store, aiming for a mobile launch
within about a month as a step toward running this with real players in
production. The user (project owner) gives final approval before anything
ships to production.

## Build

CMake + Conan via the cmake-conan dependency provider (no manual
`conan install` step — `cmake -S . -B build -DCMAKE_BUILD_TYPE=Release`
handles it). raylib comes from Conan. Currently built/tested on macOS;
Web and iOS build targets are not set up yet and are near-term roadmap
items.

```
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/FlightGame
```

Assets are copied next to the built executable (see CMakeLists.txt
POST_BUILD step) and loaded via `GetApplicationDirectory()`-relative paths
so the game runs regardless of the launcher's working directory.

## Code conventions

- Plain structs + free functions (e.g. `PlaneState`, `WorldState`,
  `LevelState` + functions that operate on them) — no classes, no deep
  abstraction layers. Keep it that simple.
- No comments except for a non-obvious "why" (e.g. a sign-convention note).
  Never comment on "what" the code does.
- Object/world creation stays behind small functions (see
  `src/objects/*.cpp`) so procedural geometry can be swapped for a loaded
  3D model later without changing call sites — see `src/objects/plane.cpp`,
  which already does this (loads `assets/models/biplane-1920.glb`).
- **Git commit messages must never mention Claude or AI attribution, in
  any form.** This is a strict, explicit rule from the project owner.
- Never force-push or rewrite history without the owner's explicit, current
  ask.
- Verify changes by actually building and launching the game (background
  process, check the log, kill it) — not just by reading the diff.

## Current state (roughly newest last)

1. Project skeleton: window + camera + ground grid.
2. Procedural plane/world/characters drawn.
3. Keyboard flight controls (pitch/roll/yaw/throttle) + chase camera.
4. 1km level goal with a finish gate, heightmap terrain (flat flight
   corridor, hills to the sides, blended into a flat horizon backdrop),
   takeoff (start grounded, need speed + nose-up pitch to lift off),
   obstacles that crash the plane on collision (restart with R), and
   checkpoint rings for in-air maneuvering.
5. Procedurally synthesized engine sound, pitch/volume tied to throttle.
6. A real 3D biplane model (1920s style, `assets/models/biplane-1920.glb`,
   generated externally — see "3D assets" below) replaces the procedural
   plane mesh, using the same rotation convention so physics is unaffected.

## Roadmap / known follow-up work

- Landing (takeoff exists; there is no touchdown/landing mechanic yet).
- Customizable/swappable plane parts (tail, etc.) instead of one fixed
  model.
- Deeper flight model beyond the current arcade approximation.
- Mobile port: touch controls, a Web (Emscripten) build, and an iOS build
  — this is the actual priority driving the schedule below, not just
  desktop polish.
- Niche/positioning reference: chill/arcade low-poly flight exploration,
  comps like *A Short Hike* / *Sky Rogue*.

## 3D assets — human-in-the-loop, no generation API

There is no API access to any text-to-3D tool from this project. See
`asset-requests/README.md` for the exact convention: an agent writes a
prompt + expected file path to `asset-requests/pending/<slug>.md`; a human
generates the model externally and drops it at that path; a later run
integrates it and moves the request to `asset-requests/fulfilled/`.

## Subagents (`.claude/agents/`)

- **game-developer** — implements one scoped feature/fix, builds + briefly
  runs the game to confirm it starts cleanly.
- **game-tester** — read-only: builds, runs, checks logs/code for
  regressions against previously completed features. Never edits code.
- **asset-planner** — manages the asset-requests/ workflow above: files
  new requests, integrates fulfilled ones.

## Autonomy policy for the scheduled product-manager routine

The scheduled/cloud routine that drives ongoing work on this game has the
project owner's explicit, standing permission to `git commit`, `push`, and
merge on its own, without waiting for approval first — the owner reviews
the result asynchronously rather than approving each action in advance.
The one non-negotiable condition: every run must end with a full, clear,
specific write-up of what was done and why (in the final summary and in
commit messages), since that explanation is what the owner's review relies
on. This elevated autonomy applies to this scheduled routine specifically,
not to the default behavior of interactive sessions with the owner.
