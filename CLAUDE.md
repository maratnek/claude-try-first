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
Web (Emscripten) builds in CI; an iOS Simulator build (raylib SDL backend,
SDL2 static, entry via SDL2main) builds in the non-blocking `ios-sim` CI job
but has never been run in a simulator or on a device.

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
10. A1 graphics presets, A2 per-plane physics params + airspeed-scaled
    controls/drag/stall/ground handling, A3 soft obstacles + damage +
    crash screen, A3b main menu (Play/Exit) + procedural crash sound.

## Current priority: v0.1 release blockers, then level 2 (overrides everything below)

`release` was cut from `dev` on 2026-10-05 (commit 9543bba). The owner
played it and found the issues below. Fix them on `dev` in this order,
one per run; mark each PROGRESS_LOG entry "release-relevant" so
release-manager cherry-picks the fix into `release`.

1. **Throttle is a brake, and a plane with no speed hangs in mid-air.**
   Repro: fly, hold S → the propeller stops and the plane stands still in
   the air. Cause: `UpdatePlaneControls` does
   `plane.speed += throttle * accel * dt`, so S actively decelerates at
   16 m/s², speed clamps to 0, and the plane only ever moves along its nose
   (`forward * speed`) — there is no gravity term, so zero speed = frozen.
   Fix: make throttle a persistent engine power level (W/S raise/lower it,
   it stays where it was left; thrust = power × accel); never brake in the
   air from throttle; add gravity so an unpowered plane glides (shallow
   descent at a sane glide ratio) and below stall speed sinks/falls
   instead of hovering. Propeller RPM follows engine power, windmilling
   slowly when power is off but the plane still moves. Keep W/S on touch
   working the same way. Keep PlaneParams-driven, no biplane constants in
   code.
2. **Flat spin-on-the-spot turns.** Repro: arrow + D → the plane rotates
   almost in place like only the tail turned, with no altitude loss. Cause:
   rudder input sets `yaw` directly at up to 40°/s, plus `roll × 0.6`
   (up to 45°/s more) — a ~17 m turn radius at 25 m/s, and banking never
   costs lift. Fix: turn rate comes mainly from bank angle and airspeed
   (coordinated-turn style, rate ∝ g·tan(bank)/v), so slow flight turns
   wide and fast flight turns tight only with steep bank; rudder alone gives
   only a small yaw/sideslip; banking reduces vertical lift so the plane
   sinks in a turn unless pitched up. No turning in place at any speed.
   Verify with a headless sim (turn radius at 20/35/50 m/s, altitude loss
   in a 45° bank) and report the numbers.
3. **Visible damage from soft obstacles.** Clipping a bush/branch should
   show on the plane, not only in the HUD: e.g. a darkened/scorched part,
   a bent or missing aileron/strut piece, plus the existing smoke. Must
   stay switchable per A1 where it is decorative.
4. **Progression design proposal (proposal only, owner approves).** The
   owner is not sure a plain "level select + level 2" is the right loop.
   He wants anticipation: the player should feel that new things are
   coming and unlock over time. game-designer writes
   `design-notes/progression.md` with 2–3 alternative progression loops
   (e.g. teased locked content, unlocks by stars/skills, a world that
   opens up), how level 2+ fits in, what is persisted, how it supports the
   Track D future modes and the aerobatics idea below, and what it costs.
   Do not build until the owner picks one.
5. **release-manager: rollout plan.** Write `design-notes/rollout-plan.md`
   (see the release-manager agent and "Release success criteria" below)
   and cherry-pick merged blocker fixes into `release`.

Lower priority — only after 1–5, and never ahead of a bug:
- **World improvement proposals (owner picks).** game-designer writes
  `design-notes/world-improvements.md`: 5–8 concrete, cheap ideas to make
  the world more interesting (landmarks, varied biomes, time of day,
  readable obstacles, ...), each with cost and the A1 toggle it would use.
  The owner chooses; nothing is built until he does.
- **Monetization options (owner decides, undecided for now).** A short
  `design-notes/monetization.md`: options that fit a free, chill game and
  App Store rules (e.g. free + optional cosmetic plane skins, a one-time
  "supporter" unlock, no ads at first), with pros/cons. Nothing is built.

## Release success criteria

- **Web (itch.io), free:** the first test is about fun, not money. Goal:
  at least 60% of playtesters react positively (a like / "would play
  again"), and nobody should feel pushed to pay — the game is free.
  release-manager defines how this is measured (itch.io rating/comments,
  a one-question survey) in the rollout plan.
- **App Store:** only once the game is a complete game — enough content
  and progression that Apple's guideline 4.2 (minimum functionality) is
  not a risk — and the Web criterion above is met. The owner enrols in
  the Apple Developer Program later; do not plan around it being done.

## Weekend sprint: Web v0.1 on itch.io (done — kept for reference)

The owner plans to put a first Web build in front of 5–10 real players
this weekend (Sat 3 – Sun 4 Oct 2026), before any App Store work. Until
that ships, pick the NEXT unchecked item from this list, in this order,
instead of following Track A/B/C. Each item = one run.

1. **Web build actually runs in a browser.** It has never been opened —
   CI only compiles it. Verify it loads (model, terrain, menu, audio after
   the first tap) with a headless browser if your sandbox has one
   (e.g. build with emsdk, serve the output, load it in headless
   Chromium/Playwright, screenshot, check the console for errors); fix
   what breaks. If you cannot run a browser, say so plainly — the
   interactive session will test it on the owner's machine.
2. **A4-min results screen:** time, checkpoints hit, damaged or clean, 1–3
   stars, best time kept in memory for the session; shown at the finish
   gate with Restart and Menu.
3. **A5-min sounds:** checkpoint chime, wind rising with speed, touchdown
   thump. Procedural, no files.
4. **Web page shell:** a custom shell instead of raylib's minshell —
   no page scroll/zoom/pull-to-refresh on phones (`touch-action: none`),
   canvas fills the window, fullscreen button, landscape hint on portrait
   phones.
4b. **C++20 / C++23 (owner-requested, do next as its own task/PR).** Move
   the project off C++17: set `CMAKE_CXX_STANDARD` to 23 if every
   toolchain we ship with accepts it — macOS Apple clang locally, CI's
   macos-14 runner, and the Emscripten web build — otherwise 20, and say
   which and why in the PR. Check that the Conan profile/cmake-conan
   still resolve raylib with the new standard (raylib is C, so a
   `compiler.cppstd` change must not trigger a rebuild failure). All three
   builds must pass in CI. Do not rewrite existing code into new-standard
   idioms in this PR — the standard bump alone, plus only the fixes needed
   to compile cleanly; using new features is for later tasks. Update
   README.md and docs/ARCHITECTURE.md (they state C++17).
5. **itch.io package:** a CI job that uploads a zip artifact with
   `index.html` at the zip root (plus the .js/.wasm/.data files), built
   from the web target.
6. **Blob shadow under the plane** (B1's shadow only, switchable per A1).

The release ships the **original biplane** (`assets/models/biplane-1920.glb`)
— the owner tried the procedural `war-1` aircraft and prefers the
biplane. The aircraft generator stays on the `design` branch and is not
merged into `dev` until the owner says so. After the sprint, return to
the Track order below (A4 full, A5 full, then Track B).

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
  **Owner-reported issues, part of A3:** (1) a crash has no proper sound —
  today the engine drone keeps playing after a crash, because
  `UpdateEngineAudio` is fed `plane.speed`, which simply freezes when
  `level.crashed` is set. On crash: the engine must cut out / wind down,
  and a one-shot crash sound plays once (impact + crunch, procedural like
  the engine — no audio files). Pull this piece of A5 forward. (2) After a
  crash the game must go to a menu, not just offer restart-in-place — see
  A3b.
- **A3b. Minimal main menu (pulled forward from Track C).** Title screen
  with Play and Exit (Exit hidden on Web/iOS where quitting makes no
  sense), shown at startup and returned to from the crash screen. Works
  with keyboard and touch. Keep it a small game-state switch (menu /
  playing / crashed) in main.cpp-level code, no UI framework — level
  select, upgrades, plane shop and settings screen stay in Track C, but
  design the state switch so they can be added as more menu entries.
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

### Aerobatics for advanced players (idea, feeds the progression design)

Over time, skilled players learn aerobatic manoeuvres — barrel roll, loop,
Immelmann, split-S and the like — recognised by the game (e.g. scored,
unlocking content). Needs the reworked flight model from blocker 2 first.
Not scheduled yet; game-designer should consider it in
`design-notes/progression.md`.

### Track D — future game modes (only after the first revenue from the simple game)

Owner's long-term idea: the game grows into a family of "things that fly
or fall", each one unlocking further levels. Do NOT work on these yet —
they are recorded so the current architecture doesn't block them (e.g.
keep `PlaneParams`/controls per vehicle type, keep the screen/level switch
open for new modes):
- Gliding and parachuting: free fall, opening a parachute, steering the
  canopy to a landing target.
- Helicopter / copter control with dedicated missions.
- Space: an orbital-station docking mission.

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

**Release** — owns all rollout:
- **release-manager** — plans where, when and how each build ships
  (`design-notes/rollout-plan.md`), keeps `release` current by
  cherry-picking merged release-relevant fixes, packages builds (local
  packages only in the gitignored `dist/` folder inside the repo), writes
  release notes. Stops before anything goes public — see Branches.

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
Nothing here changes the existing rule that only the owner cuts `release`
and promotes it to `main`.

**Player feedback**: see `PLAYER_FEEDBACK.md`. It's empty until there are
real players (nothing is shipped yet), but game-designer must check it
every run and weigh real entries over invented ideas once any exist.

## Branches

Flow: `agents/<task>` or `session/<topic>` → `dev` → `release` → `main`.

**Owner's rule: nothing lands on `dev` (or `release`) directly.** Every
change — by an agent run or an interactive session — is committed on its
own intermediate branch first, and that branch is kept on GitHub after it
is integrated, so each task's history can be reviewed later. Integrate
with **rebase, not merge**: rebase the branch onto the latest target and
fast-forward the target (for PRs: `gh pr merge --rebase`). No merge
commits, no deleting task branches after integration.

- **main** — production: exactly what has shipped. Only the project owner
  promotes `release` into `main`, after testing the candidate, and tags
  the version (`v0.1`, `v1.0`, ...). This is the "I reviewed and approved
  this for production" gate.
- **release** — the current release candidate (cut 2026-10-05 from `dev`).
  It only gets stabilization fixes; test builds (itch.io, TestFlight) are
  made from it. Bugs are fixed on `dev` first; the **release-manager**
  agent then brings the merged, CI-green, release-relevant fixes into
  `release` via its own branch `release-fix/<slug>` cut from `release`
  (cherry-pick there, PR into `release`, rebase-merge; never force-push).
  Cutting a whole new candidate from `dev` needs the owner's ask.
- **dev** — integration branch for ongoing work.
- **agents/<YYYY-MM-DD-HHMM>-<slug>** — one fresh branch per agent task,
  cut from the latest `origin/dev`, e.g. `agents/2026-10-05-1400-flat-turns`.
  The run commits there, opens a PR into `dev`, and rebase-merges it
  itself. The branch is never deleted, never reused for another task, and
  never force-pushed after the PR is opened. Agents must never touch
  `main`, and only the release-manager role writes to `release` (as
  above). If a bug looks release-relevant, fix it on `dev` as usual and
  mark it "release-relevant" in the run's PROGRESS_LOG.md entry so
  release-manager picks it up. (The old shared `agents` branch is retired;
  leave it as-is.)
- **session/<topic>** — interactive sessions with the owner use the same
  pattern: commit on `session/<topic>`, push, rebase onto `dev`,
  fast-forward `dev`, keep the branch.
- **Going public is the owner's call:** making the itch.io page public,
  promoting `release` into `main`, tagging a version, and any App Store /
  external TestFlight submission happen only on the owner's explicit go.
  release-manager prepares all of it and lists what needs approval.
- **start-with-raylib** — the original branch used before this structure
  existed; left as-is, not part of the new workflow.

## Autonomy policy for the scheduled product-manager routine

The scheduled/cloud routine that drives ongoing work on this game has the
project owner's explicit, standing permission to `git commit`, `push`, and
rebase-merge its `agents/<task>` branch into `dev` on its own, without waiting for
approval first — the owner reviews the result asynchronously rather than
approving each action in advance. It must never push to or merge into
`main`; it writes to `release` only through release-manager cherry-picks
(see Branches), and going public / promoting to production is the
owner's call alone. The one non-negotiable
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

## Integrating: one branch + one PR per task, rebase-merged

Push the task branch and open a pull request into `dev`
(`gh pr create --base dev --head agents/<task> --title "..." --body-file ...`),
then rebase-merge it yourself (`gh pr merge --rebase`, never `--merge`,
never `--delete-branch`) — you still have full autonomy to do this without
waiting for a human approval. The point of the PR is a visible, diffable
record on GitHub, not a gate. Write the PR body as the same clear
explanation described above.

**Overlap guard for scheduled runs:** another run is in progress if there
is an open PR from any `agents/*` branch into `dev`, or an `agents/*`
branch with commits not in `origin/dev` whose last commit is less than 2
hours old. An unmerged `agents/*` branch older than that is an abandoned
or blocked task: leave it untouched (it is history), mention it in the
PROGRESS_LOG entry, and carry on.

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

## Project docs (owner-requested: the repo must record how it was built)

Besides `PROGRESS_LOG.md`, keep these up to date in the same PR as the
change that makes them stale (written in Russian, like the existing text):

- `docs/DEVELOPMENT_HISTORY.md` — when a roadmap item is finished, add one
  row (what, PR/commit) to the current stage's table, plus a line under
  that stage's decisions if the work involved a real decision or a
  surprise worth remembering. Start a new stage when the milestone changes.
- `docs/ARCHITECTURE.md` — when a module, screen, build target or asset
  path is added, removed or changes responsibility: update the module
  table, the mermaid diagrams and the relevant section.
- `docs/PROCESS.md` — when the workflow itself changes (agents, branches,
  schedule, rules).
- `README.md` — build/run commands and controls, when they change.

code-reviewer treats a PR that changes structure without updating
ARCHITECTURE.md, or finishes a roadmap item without a history row, as a
(non-blocking) finding to fix in the same run.

## Safety: stop on repeated failure

Before starting new work, check whether the last 2 merges into `dev`
failed CI or were reported as broken in `PROGRESS_LOG.md`. If so, do not
push further changes on top — stop, investigate and describe the problem
in your write-up, and leave it for the project owner or the next run to
decide how to proceed, rather than continuing to build on a known-broken
base.
