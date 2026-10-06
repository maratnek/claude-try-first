# World improvements proposal (lower-priority item)

Status: PROPOSAL, AWAITING OWNER CHOICE. Nothing here is built. The owner picks which ideas to take, if any.

Inputs: PLAYER_FEEDBACK.md is empty (no players yet), so this is designed against the niche (chill/arcade low-poly flight, comps A Short Hike / Sky Rogue). Revisit once itch.io feedback exists.

## What exists today (checked in src/)
- Terrain: one sine-based heightmap, flat corridor at x=0, grass/dirt/rock vertex colours by height and slope (`terrainColors`), flat backdrop beyond the patch (`world.cpp`).
- Scatter: only two prop kinds, tree (cone) and rock, placed at random outside the corridor, scaled by `scatterDensity` (`scatter.cpp`).
- Clouds: ellipsoid puffs, count by `cloudCount`. Weather: rain or snow, alternating per run (`AdvanceWeather`).
- Sky: a flat `SKYBLUE` clear colour, no gradient, no sun, no fog. Lighting is baked into vertex colours.
- Obstacles: 4 hard pillars (6 m tall, radius 3) and 4 soft ones, drawn as wires when `obstacleWires`; distance-marker pillars (`distanceMarkers`).
- Level: 1 km, rings, finish gate, landing after the gate.
Nothing in the world is a landmark, and nothing tells the player where they are on the 1 km besides the markers.

## Ideas (each is cheap; all are draw-only unless stated)

### 1. Gradient sky and sun disc
- Sees: sky fades from deep blue at the top to pale near the horizon, with a low-poly sun disc. Rain/snow runs get a grey gradient.
- Cost: 1 run. One full-screen quad drawn before the 3D pass (4 vertices, no texture) plus one small fan. Zero perf impact.
- Toggle: always on (cheaper than the clear colour path in practice); sun disc hides when `cloudCount` is 0 only if it looks wrong, otherwise no new toggle.

### 2. Distance fog / horizon haze
- Sees: hills and far props fade into the sky colour, which hides the backdrop-plane seam and pop-in of scatter.
- Cost: 1-2 runs. Simplest is blending vertex colours toward the sky colour by distance at mesh build (terrain is static) and tinting scatter by distance in `DrawScatter`. A shader fog is not needed. Risk: the terrain tint is baked once, so view-dependent fog is only approximate (use a height-based haze, not camera-based).
- Toggle: `terrainColors` (flat colours skip it). Allows shrinking `drawDistance` on Low.

### 3. Landmarks: windmill, lighthouse, lone big tree (3 fixed ones along the strip)
- Sees: a windmill with slowly spinning blades at z~250 beside the corridor (x=45), a tall striped lighthouse at z~600 (x=-60), a lone oversized tree at z~850. Seen from the takeoff roll as goals, and they give "I passed the windmill" orientation. Distinct silhouettes also make levels 2+ (progression.md Loop A) feel different by swapping the set.
- Cost: 2 runs. Procedural prisms/cones like `AddTree`; about 100 triangles each; 3 objects total. Behind a `AddLandmark(world, kind, x, z)` function so a loaded model can replace it later (CLAUDE.md convention). Must be placed outside obstacle clearance and not collide (decorative, or optionally solid hard obstacle).
- Toggle: `scatterDensity` > 0 (draw), skipped at 0. On Low, still drawn since only 3.

### 4. Biome zones along z (meadow / forest / rocky / sand)
- Sees: the strip changes colour as you fly: meadow green, a dense dark-green forest stretch, a grey rocky stretch, and a sandy dry stretch near the finish. Gives a sense of travel.
- Cost: 2 runs. Add a biome function of z (and the colour palette per biome) to `TerrainVertexColor`; scatter picks tree/rock/cactus-like (cone-less) kinds by biome and density multiplier per biome. Static, baked in the existing vertex colour buffer, so no runtime cost.
- Toggle: `terrainColors` for the colours, `scatterDensity` for the prop mix.

### 5. Water: a lake or river with a flat blue surface
- Sees: a shallow lake low in a hollow beside the corridor, and a river strip crossing at z~400. Sun-like shine is faked by a lighter blue band.
- Cost: 2 runs. Terrain colour below a water height becomes blue (no extra mesh) plus one flat translucent-free quad at that height. Flying low over water gives the chill feel. Question: what does the plane do touching water (nothing, crash, splash)? Default: nothing (decor only).
- Toggle: `terrainColors`; flat-colour mode keeps all green (water omitted).

### 6. Time of day: dawn, noon, dusk presets per run
- Sees: warmer light and orange-pink sky at dawn/dusk, with long-looking shading and a bigger sun. Pairs with progression.md Loop A level 5 (snow night/dusk) and gives replays a different mood with no new level.
- Cost: 2 runs. A `TimeOfDay` value chosen per run (like `AdvanceWeather`) feeds the sky gradient (idea 1), a terrain colour multiplier (done by re-uploading the colour buffer, as `ApplyTerrainColors` already does), and the cloud tint. No dynamic lights.
- Toggle: `terrainColors` (flat mode ignores the tint, only the sky changes). Depends on idea 1.

### 7. Readable obstacles: colour banding and shadow/ground markers
- Sees: hard pillars get red/white stripes, soft ones a warm yellow tint, so "this will break you" vs "this will scratch you" is obvious at speed. A bright ground ring under each obstacle shows where it stands against the terrain.
- Cost: 1 run. Obstacle drawing only (a few extra vertices per pillar). Directly serves the crash/soft-damage design already in the game and removes "I could not see it" crashes. Most likely player complaint once feedback exists.
- Toggle: `obstacleWires` (stripes replace/accompany the wires, ground ring only when on). Cheap on Low.

### 8. Ambient life: birds and drifting balloon
- Sees: a flock of 5-8 small V-shaped birds that circle over the hills, one hot air balloon floating near z~700, slowly drifting. Adds movement to a still world.
- Cost: 2 runs. Birds are 2 triangles each on a sine path, no collision (or optional soft collision later). Balloon is a lathe/cone sphere. About 20-60 draw triangles in total.
- Toggle: new `ambientLife` bool, off on Low (or capped at 3 birds). Skip entirely if time is short.

## Fit with progression
Landmarks (3), biomes (4), water (5) and time of day (6) are the parameters Loop A levels 2-5 need (coast, canyon, rain mountains, snow dusk), so they are not wasted: each becomes a `LevelDef` field later. Ideas 1, 2 and 7 improve every level immediately and carry over to Loop B/C unchanged.

## Recommended order (suggestion only)
1. Idea 7 (readable obstacles): small and addresses a real gameplay clarity risk.
2. Idea 1 (sky gradient and sun) then 2 (haze): about 2-3 runs together, biggest visual jump for the cost, hides the horizon seam.
3. Idea 3 (landmarks): orientation and screenshots for the itch.io page.
4. Idea 4 (biomes), then 6 (time of day) as the first step toward level variety.
5. Ideas 5 and 8 only if the owner wants more life; lowest priority.
Total for 1+2+3+7: about 5-6 runs.

## Questions for the owner
1. Is the target look "bright and cheerful" (stay daytime) or "moody variety" (time of day, idea 6)?
2. Should landmarks be solid (a crash risk, extra challenge) or decor only?
3. Is water decor-only acceptable, or must touching it do something (splash, crash)?
4. Is a new `ambientLife` toggle acceptable, or should the Low preset stay at the current toggle list?
5. Should any of this wait until Loop A's `LevelDef` table exists, so the ideas become per-level data from the start?
