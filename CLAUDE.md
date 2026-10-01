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
