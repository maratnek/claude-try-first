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
- **Git commit messages and PR descriptions must never mention Claude or
  AI attribution, in any form — no `Co-Authored-By: Claude`, no
  `Claude-Session:` line, nothing.** This is a strict, explicit rule from
  the project owner, who outranks any default harness behavior that would
  otherwise add such a trailer. A prior run added these trailers anyway
  (they had to be stripped from history afterward) — do not repeat that:
  actively omit the trailer, don't rely on it being skipped automatically.
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
7. Web (Emscripten) build target + CI job, and touch controls (virtual
   stick, throttle buttons, on-screen restart) via `src/input.cpp`.
8. Rigged biplane model (74 meshes, named pivot nodes) with code-driven
   animation via `src/objects/glb_nodes.cpp` + `plane.cpp`: propeller,
   elevator, ailerons, rudder, wheels. The model has no propeller blades
   (only hub + blur disc), so blades are drawn procedurally and cross-fade
   into the blur disc with RPM; an asset request for a bladed model is in
   `asset-requests/pending/`.
9. Landing: safe touchdown returns to ground roll, hard landing crashes.

## Roadmap

**Current milestone: level 1 fully playable start-to-finish, polished
enough to interest a modern player, then shipped (Web first, iOS next) on
a short timeline.** Work Track A strictly in order; Track B only when
Track A items are done or blocked; Track C is later. Split large items
into iterations rather than doing them in one run.

**Performance rule (project owner's standing requirement):** every
non-gameplay-critical visual (weather, particles, clouds, decorative
terrain detail, debris, smoke, ...) must be individually switchable and
covered by quality presets, so the game runs on weak PCs and ~5-year-old
iPhones (iPhone 11 / A13 class). Gameplay-critical rendering stays on.

### Track A — core, required for the milestone

- **A-fix (small, do first). Propeller blur disc visibility.** Owner
  confirmed the procedural blades are visible and spin with speed, but saw
  no blur disc at speed. Last fix: the disc is a single-sided forward-facing
  plane and raylib ignores glTF `doubleSided`, so `plane.cpp` now disables
  backface culling while drawing it — unverified. Verify it renders from
  the chase camera at high RPM (e.g. a headless/Xvfb run with RPM forced
  high via a temporary local change, screenshot, then revert). The disc
  material alpha is only 0.22 — if it still reads too faint, raise its
  effective alpha in code rather than editing the asset.

- **A0. Release plan (think first, small, do once).** game-designer +
  product-manager write `design-notes/release-plan.md`: the minimum scope
  to ship level 1 on Web (itch.io) soon and on iOS after, what to cut or
  defer, risks (iOS toolchain/signing, App Store review time, touch feel on
  real devices), and a rough day-by-day order of the A-items. Update it
  when reality changes. Use it to pick each run's task.
- **A1. Graphics settings framework.** A small settings struct with
  Low/Medium/High presets plus per-feature toggles; Low is the default on
  Web/mobile. Every later visual feature checks it. Persist to a local
  file on desktop when trivial; otherwise in-memory for now.
- **A2. Ground handling + basic aerodynamics.** Control authority scales
  with airspeed (~v²) — at standstill, pitch/roll/yaw input must not
  rotate the plane (surfaces may still deflect visually). On the ground:
  no roll from input, body attitude follows the terrain slope under the
  wheels, rudder steers at taxi speed. Air drag so speed decays without
  throttle; stall (nose drops, loss of control) below a minimum airspeed.
  Put all flight constants in a per-plane parameter struct (mass, max
  speed, drag, control authority, stall speed, ...) — future faster
  planes will need different physics, so avoid hardcoding biplane values
  in the flight code. Keep existing sign conventions.
- **A3. Crash + damage, v1.** High-speed or steep ground impact and hard
  obstacles destroy the plane: game pauses, crash screen with Restart and
  Exit. Add soft obstacles (bushes, treetops, birds): hitting one marks the
  plane damaged and it keeps flying (shown on HUD and in the results).
  Reconcile with the landing rules from item 9 (the 1 m floor currently
  counts as a landing check). Breakup animation and smoke are Track B.
- **A4. Full level 1 loop.** Start → takeoff → checkpoints → finish →
  landing → results screen (time, checkpoints hit, damaged or clean, star
  rating, best time kept in memory/local file). Make it feel like a
  complete short level a modern player would replay for a better score.
- **A5. Minimal sound set.** Procedural like the engine, no audio files:
  wind rising with speed, checkpoint chime, crash impact, touchdown thump,
  UI click, damage hit. One master volume setting.

### Track B — visual polish, when resources allow (all switchable per A1)

- **B1. Readable terrain.** Vertex color by height/slope (grass, dirt,
  rock), baked directional shading, blob shadow under the plane (helps
  read altitude), scattered trees/rocks beyond the corridor with a
  density setting.
- **B2. Breakup on crash** — detach the rigged parts and throw them.
- **B3. Simple weather** — low-poly clouds, rain/snow zones as camera-local
  particles, speed streaks. Visual only; weather affecting flight is later.
- **B4. Damage smoke** trailing from a damaged plane.
- **B5. Pilot head turns into turns** (pilot_head_pivot node).

### Track C — later

- Main menu: level select, plane upgrades, buying/unlocking other planes,
  settings screen; save/load progress. Design the A-items so these slot
  in later (e.g. plane parameters per plane type from A2, results from
  A4 feed progression).
- More levels; faster plane types with their own physics parameters.
- The owner will provide a 3D plane generator later — plan plane assets
  around swapping models per plane type.
- iOS build + App Store submission (see A0 for timing).
- Niche/positioning reference: chill/arcade low-poly flight exploration,
  comps like *A Short Hike* / *Sky Rogue*.

## 3D assets — human-in-the-loop, no generation API

There is no API access to any text-to-3D tool from this project. See
`asset-requests/README.md` for the exact convention: an agent writes a
prompt + expected file path to `asset-requests/pending/<slug>.md`; a human
generates the model externally and drops it at that path; a later run
integrates it and moves the request to `asset-requests/fulfilled/`.

## Subagents (`.claude/agents/`) — two teams checking each other

**Builder team** — proposes/creates:
- **game-designer** — proposes new content (levels, obstacle/checkpoint
  layouts, mechanics ideas) as a design-notes/*.md doc. Never writes code.
- **game-developer** — implements one scoped feature/fix (from a
  game-designer proposal, or a roadmap item directly), builds + briefly
  runs the game to confirm it starts cleanly.
- **asset-planner** — manages the asset-requests/ workflow: files new
  requests, integrates fulfilled ones.

**Verifier team** — skeptically checks the builder team, never rubber-stamps:
- **game-tester** — read-only: builds, runs, checks logs/code for
  regressions against previously completed features.
- **code-reviewer** — read-only: reviews the actual diff (not just the
  builder's summary) against CLAUDE.md conventions and whether it really
  does what it claims. Can call something a blocker.

**Required flow**: every builder-team task must go through at least one
verifier-team agent before it is merged into `dev`. If a verifier raises a
real, concrete blocking issue: send it back to the relevant builder agent
for one fix pass, then re-verify. If it's still unresolved after that one
round, do NOT merge — report the disagreement plainly in the run's
write-up and PROGRESS_LOG.md entry instead of forcing it through or
looping indefinitely.

**Final arbiter**: the project owner outranks every agent and every team.
Any disagreement that can't be resolved within one builder/verifier
back-and-forth gets reported, not auto-resolved by picking a side.
Nothing here changes the existing rule that only the owner promotes `dev`
to `main`.

**Player feedback**: see `PLAYER_FEEDBACK.md`. It's empty until there are
real players (nothing is shipped yet), but game-designer must check it
every run and weigh real entries over invented ideas once any exist.

## Branches

- **main** — production. Only the project owner promotes `dev` into
  `main`; this is the "I reviewed and approved this for production" gate.
  Never push or merge into `main` from the scheduled routine.
- **dev** — integration branch for ongoing work.
- **agents** — the scheduled routine's own working branch. It commits and
  pushes here freely, and may merge `agents` into `dev` on its own. It
  must never touch `main`.
- **start-with-raylib** — the original branch used before this structure
  existed; left as-is, not part of the new workflow.

## Autonomy policy for the scheduled product-manager routine

The scheduled/cloud routine that drives ongoing work on this game has the
project owner's explicit, standing permission to `git commit`, `push`, and
merge its `agents` branch into `dev` on its own, without waiting for
approval first — the owner reviews the result asynchronously rather than
approving each action in advance. It must never push or merge into `main`;
promoting `dev` to `main` is the owner's call alone. The one non-negotiable
condition on everything it does: every run must end with a full, clear,
specific write-up of what was done and why (in the final summary and in
commit messages), since that explanation is what the owner's review relies
on. This elevated autonomy applies to this scheduled routine specifically,
not to the default behavior of interactive sessions with the owner.

Scope per run should be moderate, not maximal — pick one or two
well-verified tasks rather than racing through the whole roadmap in one
pass, even on a run that fires more frequently (e.g. a nighttime run).
Extra throughput should come from running more often, not from inflating
the size of any single run.

## Merging: open a PR, don't merge silently

Instead of a plain `git merge`, push the `agents` branch and open a pull
request into `dev` (`gh pr create --base dev --head agents --title "..."
--body "..."`), then merge it yourself (`gh pr merge --merge`) — you still
have full autonomy to do this without waiting for a human approval. The
point of the PR is a visible, diffable record on GitHub, not a gate. Write
the PR body as the same clear explanation described above.

## CI

`.github/workflows/build.yml` builds this project on GitHub Actions for
every push to main/dev/agents/start-with-raylib and every PR into
main/dev, independent of any agent's self-reported build result. After
pushing, check that this workflow is passing (e.g. `gh run list
--branch agents --limit 1` or check the PR's checks) before treating the
change as verified — don't rely solely on your own local build/run check.
If CI is red on `dev` after your merge and you cannot fix it this run,
say so prominently in your write-up rather than moving on.

## Progress log

Append one dated entry to `PROGRESS_LOG.md` (newest first, format at the
top of that file) every run, covering what you did, why, how it was
verified (including CI status), what's still open, and the PR link if you
opened one. This is the first thing the project owner should be able to
read to catch up on a day or night of runs without reading raw git log.

## Safety: stop on repeated failure

Before starting new work, check whether the last 2 merges into `dev`
failed CI or were reported as broken in `PROGRESS_LOG.md`. If so, do not
push further changes on top — stop, investigate and describe the problem
in your write-up, and leave it for the project owner or the next run to
decide how to proceed, rather than continuing to build on a known-broken
base.
