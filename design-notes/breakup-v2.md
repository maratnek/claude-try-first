# Breakup v2: the model defines how it breaks, fuel drives the explosion

Status: design, for code-reviewer review before any code. Feedback source: `PLAYER_FEEDBACK.md` is empty (no players yet), so this follows CLAUDE.md priority item 6 and the chill/arcade niche. A crash must be readable and a little funny, never gory, and must stay cheap on Web/iOS (Low preset).

## What exists today
- `src/objects/debris.cpp` throws 7 hardcoded `PlanePart`s (`kSpawnOrder`: prop, 2 wheels, rudder, elevator, 2 ailerons). Each is a single rig node with a pivot. All spawn at once with random kick. Wings, struts, fuselage and tail never come apart. Debris is drawn via `DrawPlanePart`, which draws meshes by `meshPart` (a mesh belongs to the rig part found by walking ancestors).
- `plane.cpp` reads the glTF node tree (`LoadGlbNodes`) because raylib drops node names. Mesh to node mapping is by walking `GlbNode::parent`. Break groups reuse exactly this mechanism.
- Crash trigger: `main.cpp` (Hard obstacle hit or `LandingResult::Hard`) calls `PlayCrashSound` and `SpawnDebris(..., g.gfx.debrisPieces)`. The only impact info available is `plane.speed`, forward vector, and (for obstacles) the plane position.
- A1: `GraphicsSettings::debrisPieces` (`ApplyGraphicsPreset` in `src/settings.cpp` today: Low 0, Medium 3, High 7; 0 means no breakup and `UpdateDebris(pieceCount<=0)` clears the debris), `smokePuffs` (64 max, `kMaxSmokePuffs`), `damageVisuals`.
- Biplane GLB node names (real): `fuselage, fuselage_band, engine_cowl, cowl_ring, cockpit_rim, cockpit_interior, headrest, windscreen, tail_cone, tailplane_fairing, horizontal_stabilizer, vertical_fin, elevator(_pivot), rudder(_pivot), rudder_hinge, wing_lower, wing_lower_leading_edge, wing_upper, wing_upper_leading_edge, wing_upper_marking, aileron_left(_pivot), aileron_right(_pivot), strut_{l,r}_{front,rear}, cabane_{l,r}_{front,rear}, wire_{l,r}_{c,d}, gear_{l,r}_{front,rear}, gear_suspension (parent of axle, bungee_r, bungee_l, wheel_right, wheel_left, wheel_hub), tail_skid, tail_skid_shoe, propeller, prop_hub, prop_blur_disc, pilot (+ pilot_* children)`. Sidecar groups may name either the pivot or the mesh node; descendants always follow.

## 1. Sidecar format: `assets/models/biplane-1920.break.txt`
Same style as the generator's `rig.txt` (see `origin/design`: `assets/models/aircraft/*/rig.txt`): `#` comments, whitespace-separated, one record per line, units metres/kg, plane space (Y up, nose +Z; same axes as rig.txt, so +X is the right wing). Loaded next to the GLB (path derived by swapping `.glb` for `.break.txt`). Missing file means fallback to today's behaviour (the 7-part list), so nothing regresses.

Records:
- `group <name> <mass_kg> <strength> <node> [<node>...]` : every listed node and all its descendants belong to the group, except descendants that are explicitly listed in another group (most specific listing wins: a node explicitly listed anywhere is excluded from any ancestor's group). Unlisted meshes go to `hull`. Listing the same node in two groups is an error. Optional suffix `@L` / `@R` on a node (only triangles with plane-space X < 0 / X > 0) is NOT used in iteration 1 (see the splitting step below).
- `joint <child_group> <parent_group> <x> <y> <z> <strength_scale>` : the attachment point (plane space) and a multiplier on the child's strength. Parent chain must end at `hull`, which never detaches (it is the "pilot stays in" rule; pilot head stays on the hull as today).
- `order <group> <group> ...` : tie-break order when two joints are equally overloaded (weakest-first list, optional).
- `fuel_tank <x> <y> <z> <radius>` : where the fuel burst/fireball originates (in the fuselage behind the engine).

Proposed full content:

```
# group name mass strength nodes...   (strength = impact "energy units", see section 2)
group hull        180  -   fuselage fuselage_band cockpit_rim cockpit_interior headrest windscreen tail_cone tailplane_fairing gear_suspension tail_skid tail_skid_shoe
group engine       90  60  engine_cowl cowl_ring
group prop          6  12  propeller prop_hub prop_blur_disc
group tail_fin      8  18  vertical_fin rudder_pivot rudder_hinge
group tail_plane   10  18  horizontal_stabilizer elevator_pivot
group wing_up      44  30  wing_upper wing_upper_leading_edge wing_upper_marking aileron_left_pivot aileron_right_pivot
group wing_lo      40  34  wing_lower wing_lower_leading_edge strut_l_front strut_l_rear strut_r_front strut_r_rear wire_l_c wire_l_d wire_r_c wire_r_d
group cabane        6  22  cabane_l_front cabane_l_rear cabane_r_front cabane_r_rear
group gear_l       14  16  gear_l_front gear_l_rear axle bungee_l wheel_left
group gear_r       14  16  gear_r_front gear_r_rear bungee_r wheel_right
# joint child parent x y z strength_scale
joint engine    hull    0.0  1.3  2.2  1.5
joint prop      engine  0.0  1.3  3.1  0.5
joint tail_fin  hull    0.0  1.7 -3.9  1.0
joint tail_plane hull   0.0  1.3 -3.8  1.0
joint wing_lo   hull    0.0  0.9  0.2  1.4
joint cabane    hull    0.0  2.0  0.3  1.0
joint wing_up   cabane  0.0  2.4  0.3  1.0
joint gear_l    hull   -0.6  0.4  0.4  0.8
joint gear_r    hull    0.6  0.4  0.4  0.8
order prop gear_l gear_r tail_plane tail_fin wing_up wing_lo cabane engine
fuel_tank 0.0 1.1 1.4 0.5
```

Coordinates are first guesses (wings are whole-wing groups in iteration 1); game-developer must read the real pivot positions from the GLB node tree and correct them in iteration 1. Rule of thumb for the strength numbers: weakest = prop, gear, tail surfaces (they go in any hard hit); wings need a real hit; engine and hull never fly off from speed alone. Struts and wires of both sides travel with the lower wing so a wing never leaves a dangling strut. `tail_skid`/`tail_skid_shoe` are deliberately left in `hull` (the tail skid is part of the tail cone). Pilot nodes need no entry (they fall into `hull` as unlisted meshes). The wheels are listed in the gear groups explicitly so `gear_suspension` in `hull` does not swallow them.

Loader validation (log warnings, fall back to the 7-part list, never crash): unknown node name, node listed in two groups, cycle in joints, group without a joint, mass or strength <= 0. The loader must also log each group's mesh count; a group with 0 meshes is a warning (catches the wheel-swallowing mistake).

## 2. Impact speed and direction choose the failing joints
New struct `ImpactInfo { Vector3 point; Vector3 velocity; float speed; }` passed by `main.cpp` at crash time. `point` = plane position at hit, `velocity` = forward * speed (+ fallSpeed down). Obstacle hits keep using the plane position; ground hits use the lowest of the plane.

Algorithm in `SpawnDebris` (free function, runs once, deterministic given a seed so screenshots are repeatable):
1. Impact energy `E = 0.5 * speed^2` (m/s based, mass-free units matching the strength numbers: 25 m/s cruise = 312, so a landing-limit hard thud at ~15 m/s = 112, full speed 50 m/s = 1250). A hit that is only slightly hard must still pop at least the weakest group so the crash is visible.
2. Impact direction `d` = normalised impact velocity (forward * speed plus the downward `fallSpeed`; `speed` is its length, and `ImpactInfo.ground` is true for a hard landing, an addition to the struct above). The hull centre used below is the centre of the `hull` group's mesh bounds (plane space). The dot product is evaluated in world space (joint offset rotated by the plane pose). For each group `g`, exposure `e_g = clamp(0.35 + 0.65 * dot(normalize(joint_g - centre_of_hull), d), 0.2, 1.0)`: groups on the leading side take the blow (head-on: prop, engine, upper wing; ground, nose down: gear, lower wings; tail first when a tail strike). Ground impacts add +0.3 to groups whose joint y is lowest.
3. Joint `g` fails if `E * e_g >= strength_g * strength_scale_g`. Process joints from the leaves toward the hull. Per-group strength gets +-15% seeded jitter so identical crashes are not identical.
4. If a parent group detaches, all its children go with it as one body (the upper wing goes if the cabane fails; if only the wing joint fails, the cabane stays). A child's own joint test only matters while its parent stays attached.
5. Cap order: first decide failures (steps 3-4) with the full set; then, if more than `cap` roots detached, first keep one member of each sidecar `keep` line (highest overload among those that detached, counted through its root; skipped when none of the line detached), then fill with the highest-overload roots (a parent counts with its whole subtree as one detachment), and merge the rest back into the hull (they stay attached, children of a kept parent still go with it and do not count). Minimum 1 detached group on any crash, including Low: if nothing failed, the group with the highest overload goes. The jitter seed is a hash of a fixed constant, the impact point (to 0.1 m) and the speed, so the same crash always gives the same result.
6. Leave the hull in place on a crash with a settled hull pose: the hull stops where it hit (as today) and does not tumble in this feature; only groups fly.

## 3. Debris physics (plain structs and free functions)
Replace `DebrisPiece.part` (and `kMaxDebrisPieces = PART_PILOT_HEAD`, `kSpawnOrder`, both `static_assert`s: all removed; capacity becomes the sidecar's group count, max 16, and the fixed list survives only inside the fallback path, which keeps its own 7-part constants and asserts) with `group` and extend (struct stays plain):
```
struct DebrisPiece { int group; Vector3 position, velocity, angularVel; Quaternion orientation; Vector3 comLocal; float mass, restHeight, bounds[3]; bool settled; };
```
- Spawn: centre of mass `comLocal` = mass-weighted mean of the group's mesh vertex bounds (precomputed once at load, no per-frame work). Position/orientation start at the group's pose in the crashed plane, so nothing visibly jumps. Orientation is a quaternion built from yaw/pitch/roll, not Euler adds, to avoid the gimbal-like drift in today's draw code.
- Momentum: `velocity = planeVelocity * 0.8` (some energy is lost in the crash) plus impulse `J / mass`, where `J = k * E_excess * away_dir` and `away_dir` = from hull centre through the joint, biased up by `+0.4 Y`. Light parts fly farther (prop 6 kg flies, 90 kg engine drops). Clamp the final speed to `maxDebrisSpeed = 30 m/s`.
- Tumble: `angularVel = cross(r, J) / inertia` with `inertia ~= mass * radius^2 * 0.4`, `r` = joint offset from COM, clamped to 540 deg/s as today, plus seeded random.
- Per step (fixed 1/60 sub-step, like `UpdateDebris` today): gravity, linear drag 0.1/s, integrate position and orientation (`QuaternionMultiply` with the half-angle update). Ground test uses the group's lowest world-space bounding point (the 8 corners of the precomputed local AABB, rotated), not just the centre, so a wing lies flat on the terrain instead of sinking into it: `ground = GetGroundHeight(...)` under the lowest corner; if below, lift the group by the penetration and bounce `v.y *= -0.35`, friction 0.6 on x/z and on angular velocity.
- Settle: when speed < 1.2 m/s and the last bounce was small, settle by slerping orientation to the nearest stable rest pose over 0.3 s (lowest of the group's 3 axis-aligned faces down), then set the group so its lowest corner is exactly `restHeight` (0.05 m; this changes `kRestHeight` from today's 0.15) above ground and freeze (`settled = true`). No floating: after settle, assert lowest corner height within 0.1 m of ground, in a headless self-check.
- No interpenetration between groups: debris vs debris is skipped (cheap, covered by spawn spread), but groups that settle within 0.5 m of each other are nudged apart along x/z by the overlap of their bounds (one pass, at settle time only). Debris vs hull: on settle, if a group's AABB overlaps the hull's AABB, push it out along the shortest axis. Debris vs obstacles/trees is ignored (documented limitation).
- Drawing: `DrawDebris` draws by group (`meshGroup[meshIndex]` replaces `meshPart` for debris) with the group's quaternion; `DrawPlaneObject` takes a `detachedGroups` bitmask (replacing `detachedParts`) so the hull draws without them. Parts still animate correctly if the sidecar is absent (fallback).
- Cost: <= 12 groups, a few dozen flops each per frame; no allocation per frame.

## 4. Fuel and explosion
Flight data:
- `PlaneParams`: `float fuelCapacity;` (biplane: 100 units, 1 unit ~ 1 l). `PlaneState`: `float fuel = 0` (set to `fuelCapacity` at start/restart, in the same place the plane is reset). Consumption by engine power is a follow-up (not part of this feature, `fuel` just stays full on every run until then). Add `float FuelFraction(const PlaneState&, const PlaneParams&)`.
- A low-fuel test hook is required for verification: env var `FLIGHT_FUEL=0..1` read at startup, since nothing burns fuel yet. It, `--crash-test` and the other `FLIGHT_*` variables exist only under `#ifdef FLIGHT_DEBUG` (defined by a CMake option `FLIGHT_DEBUG`, off by default and never set for Web/iOS or release CI builds), so they cannot ship.

Explosion scale `s = clamp(fuelFraction, 0, 1)`; shaped for readability:
| fuel | result |
|---|---|
| < 0.05 | breakup only: no flash, no fireball, 3-5 dust puffs, existing crash sound at normal volume |
| 0.05 - 0.3 | small flash, small orange puff, a thin smoke column that lasts ~3 s |
| 0.3 - 0.7 | medium fireball (radius ~3 m), smoke column 5 s |
| 0.7 - 1 | big fireball (radius ~6 m), thick smoke column 8 s, debris gets an extra outward impulse `x(1 + 0.5 s)` |
- Flash: a full-screen white overlay of `0.35 * s` alpha fading over 0.15 s (skipped on Low/Medium; no separate reduced-motion setting is added). Fireball: 2-3 billboard spheres/quads (orange then dark) at `fuel_tank` position, grow over 0.4 s with `radius * s`, fade 1.2 s. Smoke: reuse `SmokeState` with a new `ExplosionSmoke` emission mode (rising, longer life, `s`-scaled puff count); no new particle system.
- Sound: `PlayCrashSound(audio, s)`: today's thud plus a low boom layer whose gain and length scale with `s` (synthesised, like the engine sound, no asset); at `s < 0.05` boom is silent.
- A1 presets (extend `ApplyGraphicsPreset`; fields `bool explosionFire`, `int explosionSmoke`). Existing values CHANGE: `debrisPieces` goes from Low 0 / Medium 3 / High 7 to Low 4 / Medium 6 / High 12; `smokePuffs` stays 24 (Medium) / 64 (High), and Low's 0 stays for the idle damage smoke while the crash column gets its own 12 puffs. `debrisPieces == 0` no longer means no breakup: the minimum is 1 group on any preset, so `UpdateDebris`'s `pieceCount <= 0` clear is removed and debris is cleared only by `ClearDebris` (restart/menu). Saved preset choices keep working because the enum is unchanged. The fallback path (no sidecar) uses `min(debrisPieces, 7)` with the old physics. Per-preset list:
  - High: everything above, up to 12 groups, 64 smoke puffs.
  - Medium: no screen flash, fireball is 1 billboard, 6 groups, 32 puffs.
  - Low (Web/iOS default): no flash, no fireball geometry; instead a single orange tint ring sprite for 0.3 s, so the fire still reads; at most 4 groups (the 4 highest-overload ones), 12 smoke puffs, the boom sound still plays. Readability rule for Low: at least the prop, one wing and one gear must visibly leave the plane on any crash above the landing-limit speed, because the break pattern is the main read.
  - No shake is added.

## 5. Future models and the `design` branch
Text to add to `asset-requests/README.md` under the request template:
> Each model request must also include a **break data** section (or the model's sidecar `<name>.break.txt`): the break groups (which named nodes move as one unit, for example "left lower wing + its struts + wires + aileron"), each group's mass in kg and a strength rating (low = prop/gear/tail surfaces, mid = wings, high = engine), the joint position for each group (where it attaches to its parent, plane space), the fuel tank position, and the weakest-first failure order. Use node names exactly as they appear in the GLB. The file format is documented in `design-notes/breakup-v2.md`. If a node spans both sides (a one-piece wing), either split it in the model or mark it `@L`/`@R` in the sidecar.

Note for the `design` branch (aircraft-generator agent, `tools/aircraft-gen`): alongside `rig.txt`, export `break.txt` in the section 1 format. The generator already knows the group structure (wing halves, struts, ailerons, gear legs, tail surfaces) and the part masses/positions, so groups should be derived from its own part tree rather than guessed: one `group` per wing half including its struts/wires/aileron, per gear leg, per tail surface, plus engine, prop and `hull`. Emit separate `wing_*_l`/`_r` nodes so `@L`/`@R` is unnecessary. Strength defaults: prop 12, gear 16, tail 18, wings 30-34, engine 60 (scaled by aircraft size class), `fuel_tank` at the fuselage centre behind the engine. This is a note only: no work on `tools/aircraft-gen` happens on this branch.

## 6. Iteration tasks (one run each, game-developer + verifier)
Headless verification setup: add a `--crash-test` debug flag (no UI) that starts at a given speed/heading/height and drives into the ground or a hard obstacle, takes screenshots with `TakeScreenshot` at t = 0.1, 1, 3, 6 s after impact, and exits. Variables: `FLIGHT_SPEED`, `FLIGHT_FUEL`, `FLIGHT_PRESET`. Screenshots attached to the PR. game-tester also runs the debris self-check (no piece with its lowest point more than 0.1 m above or below the ground once settled; no NaN).

1. **Sidecar + groups, no physics change.** Add `src/objects/break_data.{h,cpp}` (parse `.break.txt`, validate, build `meshGroup[]` using ancestors, per-group COM and AABB). Commit `assets/models/biplane-1920.break.txt` with real pivot coordinates. Debris uses groups with the OLD spawn physics; `main.cpp` unchanged except type renames. Verify: build, launch, crash at 3 speeds (15, 25, 45 m/s); log lists all groups, mesh counts and no warnings; with the file deleted, today's 7-piece breakup still works. Screenshots: all groups flying off as units (wing with struts and wires attached).
2. **Joint failure from impact.** `ImpactInfo`, the section 2 algorithm, seeded RNG, group cap from `debrisPieces`. Verify: screenshots head-on at 15/25/45 m/s, ground impact nose down, tail strike: 15 m/s drops prop/gear only, 25 m/s adds tail/ailerons, 45 m/s sheds wings; cap respected on Low (4) and High (12); same seed gives the same result.
3. **Debris physics. DONE (iteration 3, see "Iteration 3 result" below).** Quaternion orientation, mass/inertia impulse, AABB ground contact, settle pose, nudge-apart. Verify: screenshots at 1, 3, 6 s; self-check for floating/sinking on flat ground and on a slope (hills beside the corridor); log settle times (<= 6 s each); frame time with 12 groups unchanged within noise.
4. **Fuel, explosion, A1.** `fuel` fields, `FLIGHT_FUEL` hook, flash/fireball/smoke/sound scaling, A1 fields and Low behaviour, CLAUDE.md + `asset-requests/README.md` text. Verify: 3 fuel levels (0, 0.3, 1.0) x 2 speeds x Low/High = 12 screenshots at t = 0.3 s and 3 s; at fuel 0 no fire on any preset; Low is readable on its own (prop, wing, gear visible, orange ring, smoke); the crash sound at s = 1 is audibly bigger (log gain values); no CI regression, Web build compile check.

Out of scope: fuel burn by engine power, fire spreading/burning wreck on the ground after settle, pilot ejection, debris hitting trees, hull tumbling, damage from soft obstacles (item 3 is separate).

## Risks for reviewers to probe
- `wing_upper`/`wing_lower` as single nodes: iteration 1 uses whole-wing groups (one upper, one lower, both sides leave together) as the default. Optional later step, only if one-sided wing loss is wanted: splitting needs NEW raylib Mesh objects, because one Mesh exists per primitive with node transforms baked in and `meshPart` maps one Mesh to one part. The step copies each wing Mesh twice (triangle centroid X < 0 and X >= 0), `UploadMesh`es both, extends `meshPart`/`meshGroup`/`meshIsBlur`, frees the original, and enables the `@L`/`@R` sidecar suffix. The cut at X = 0 lies inside the fuselage/cabane, so it should be invisible.
- Strength units are tuned by eye; iteration 2 should expose them only through the sidecar so tuning needs no rebuild.
- Existing `PlanePart` rig (animation) is kept unchanged; groups are an overlay for debris only.

## Iteration 3 result (debris physics)

Implemented in `src/objects/debris.cpp` as designed, with these deviations:
- Ground contact uses the group's real vertices, not the 8 AABB corners: 26 extreme vertices (cube directions) plus a voxel-thinned sample (up to 128) of the group's other vertices, plus the propeller blade cube. A long strut or leading edge can otherwise hide between two extremes when the terrain height steps under it (found in testing: a wing standing on its edge sank 0.14 m on a terrain step).
- Contact is a single-point impulse on the deepest vertex with scalar inertia `m * R^2 * 0.4` (not a full contact manifold). Bounce 0.35 only above 2 m/s contact speed; per-step friction on `v.xz` and the spin.
- Settle: when speed < 1.2 m/s and spin < 2 rad/s, or after 4.5 s, velocity is zeroed and the group slerps over 0.3 s to the nearest axis-aligned rest pose (the local face pointing roughly down with the lowest centre-of-mass height), is lowered to `kRestHeight = 0.05 m`, nudged apart once from already settled groups (AABB overlap along x or z), and then frozen (no further updates).
- Hull overlap push-out and debris vs trees/obstacles are NOT done: an engine or other heavy group that drops almost in place can still overlap the hull.
- The 7-part fallback uses the same physics (mass 5 kg, old-style random kick, seeded) so there is one code path.
- Randomness is now seeded by the impact hash (same crash, same debris); `GetRandomValue` is no longer used.
- `GetGroundHeight` is a nearest-sample heightmap (22 m cells) while the rendered terrain is interpolated, so on hills a piece can look slightly above or below the visible surface even though it rests exactly on the physics height. Not changed here.

Headless check (ASan/UBSan, Xvfb, raylib 5.5): 48 crash cases (3 speeds x cap 4/12 x 4 impact types x group/fallback path), 256 pieces: all settled within 5.4 s, true lowest vertex of every settled piece between 0.024 and 0.052 m above terrain, no NaN, no piece moving after settling.

## Open items found in review (resolve in the named iteration)

- RESOLVED in iteration 1/2 (strengths rescaled in the sidecar, ailerons split into their own groups): the example strengths are far below the impact energy scale in section 2 (E = 0.5·speed² gives 112 / 312 / 1250 at 15 / 25 / 45 m/s; the engine strength is about 90, wings 30–48), so at 25 m/s nearly everything fails. Rescale strengths (roughly 3–10×) or the energy formula so the stated outcomes (15 m/s prop/gear only, 25 m/s adds tail/ailerons, 45 m/s sheds wings) hold; tune by the headless crash test.
- RESOLVED in iteration 1: the hull record has strength `-`; special-case it in the loader validation (it never detaches).
- RESOLVED in iteration 1: `axle` is one node spanning both wheels but sits in `gear_l`; put it in the hull or accept the right wheel losing its axle. Check aileron parentage (upper or lower wing) against real node transforms.
- RESOLVED in iteration 2 (`keep` records in the sidecar, hull centre = hull mesh bounds centre, the E x 2 sentence removed): the cap keeps the highest-overload roots, which can drop the prop on Low; add a forced-include priority (prop, one wing, one gear) so the Low readability rule holds. Delete or clarify the "second test at E×2" sentence for surviving children of a detached parent, and define the hull centre used in the exposure formula.
- Iteration 4: Medium smoke is stated as 24 idle puffs and 32 crash-column puffs; confirm the crash column is a separate field. With this model only 9 detachable groups exist, so the High cap of 12 is never reached.
- Known cosmetic limits: struts and wires may dangle if only one wing leaves; cabane stubs may remain if the upper wing leaves.
