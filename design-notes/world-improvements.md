# World improvements proposal (lower-priority item)

Status: PROPOSAL, AWAITING OWNER CHOICE. Nothing here is built. The owner picks which ideas to take, if any.

Inputs: PLAYER_FEEDBACK.md is empty (no players yet), so this is designed against the niche (chill/arcade low-poly flight, comps A Short Hike / Sky Rogue). Revisit once itch.io feedback exists.

## What exists today (checked in src/)
- Terrain: one sine heightmap, flat corridor at x=0, grass/dirt/rock vertex colours (`terrainColors`), plus a 50000 m `DrawPlane` backdrop in single-colour `kGrass` (`world.cpp`).
- Scatter: trees and rocks only, outside the corridor, culled at a hard-coded 450 m; clouds culled at 600 m (both literals in `DrawWorldObject`, not settings).
- Weather: rain or snow, alternating per run. Sky: flat `ClearBackground(SKYBLUE)`; no gradient, sun or fog.
- Obstacles are spheres, not pillars. Hard: red `DrawSphere`, centre 6 m above ground, radius 3. Soft: green foliage sphere (radius 1.6-2.5) on a brown trunk cylinder when raised, so they read as trees/bushes. `obstacleWires` adds sphere wires. 5 distance-marker pillars per side (`distanceMarkers`).
- Level: 1 km, rings, finish gate, landing after the gate.

## What Low gets (Web/iOS default, `ApplyGraphicsPreset`)
Low is: `terrainColors` off (flat green terrain), `scatterDensity` 0, `cloudCount` 0, `obstacleWires` off (High only), `distanceMarkers` off, rain 0, snow 0 (snow is High only). Only `blobShadow` is on. So the world on Low is a flat green field with red and green spheres: any idea gated on those toggles is invisible exactly where most players are. Rule used below: an idea that matters for the first impression gets a cheap always-on path on Low or a new bool that Low keeps on; the rest is explicitly "none on Low".

## Ideas (each is cheap; all are draw-only)

### 1. Gradient sky and sun disc
- Sees: sky fades from deep blue to pale at the horizon, plus a low-poly sun disc. Rain/snow runs use a grey gradient.
- Cost: 1 run. One full-screen quad (4 vertices, vertex colours, no texture) before the 3D pass, one small fan. No measurable cost expected; measure on iPhone 11 before claiming.
- Low: gradient yes, always on, no toggle. Sun disc also draws on Low (it is a single fan, independent of `cloudCount`); if it looks odd with no clouds the owner can veto it.

### 2. Distance haze
- Sees: far hills fade toward the sky colour.
- Cost: 1-2 runs. Limits: the 50000 m backdrop is one `kGrass` colour, so baked vertex haze on the 1 km terrain cannot hide the horizon seam; hiding it needs the plane drawn as a gradient (vertex colours blending to the sky horizon colour, 4 vertices) in `DrawWorldObject`. Scatter pop-in comes from the 450 m cull; haze only softens it, it does not remove it. Terrain haze is baked once, so it is height-based, not camera-based.
- Low: only the backdrop-plane gradient (always on, cheap). The terrain-colour haze does nothing on Low because `terrainColors` is off there (flat colours).

### 3. Landmarks: windmill, lighthouse, lone big tree
- Sees: windmill with slowly turning blades at z~250 (x=45), striped lighthouse at z~600 (x=-60), oversized tree at z~850. Orientation on the 1 km and a different silhouette set per level later.
- Cost: 2 runs. Procedural prisms/cones like `AddTree`, about 100 triangles each, 3 in total, behind `AddLandmark(world, kind, x, z)` so a model can replace them. Outside obstacle clearance; decor only unless the owner wants solid.
- Toggle: not `scatterDensity` (0 on Low). Own rule: a new `landmarks` bool, true on all presets because only 3 objects are drawn. Blade spin stops on Low.

### 4. Biome zones along z (meadow / forest / rocky / sand)
- Sees: ground colour and prop mix change along the strip.
- Cost: 2 runs. Biome function of z in `TerrainVertexColor`, per-biome density in scatter. Baked, no runtime cost.
- Low: nothing, since Low has flat terrain colours and no scatter. To give Low anything, flat mode would need a per-biome single colour (4 colour bands in the existing `terrainFlatColors` buffer), which costs +0.5 run and is still free at runtime. Recommended if the idea is taken.

### 5. Water: lake and river
- Sees: shallow lake beside the corridor, river strip crossing at z~400, decor only.
- Cost: 2 runs. Terrain colour below a water height goes blue plus one flat quad at that height.
- Low: with flat colours the lake would be one blue quad only (no shaded shore); draw the quad on Low, skip the colour work. Touching water does nothing.

### 6. Time of day: dawn, noon, dusk per run
- Sees: warmer sky and larger sun at dawn/dusk, tinted terrain and clouds.
- Cost: 2 runs. A `TimeOfDay` value per run feeds the sky gradient (idea 1), a terrain colour multiplier (re-uploading the colour buffer as `ApplyTerrainColors` does) and cloud tint. No dynamic lights. Depends on idea 1.
- Low: sky and sun only (the terrain tint needs `terrainColors`; the flat `kGrass` colour could take the tint as a single multiplier, +0.5 run). Clouds are off on Low.

### 7. Readable obstacles (spheres): bands and ground ring
- Sees: the red hard sphere gets a white equator band or two (a short cylinder/ring or a few triangle strips on the sphere), so it reads as a warning ball and not a generic red ball. A dark ground ring/blob under each hard sphere shows where it stands. Soft obstacles keep their green foliage and trunk (no tint change, so they stay "tree/bush"); they only gain the same ground blob. Today the only "readable" cue is wire overlay, which is High only.
- Cost: 1 run. About 30-60 extra triangles per obstacle, 8 obstacles; reuse the blob-shadow drawing in `DrawBlobShadow`.
- Toggle: none (always on, all presets), since it is a gameplay-clarity fix and wires are High only. Wires stay as the extra High cue.

### 8. Ambient life: birds and a drifting balloon
- Sees: 5-8 V-shaped birds circling over the hills, one balloon near z~700. No collision.
- Cost: 2 runs, about 20-60 triangles total.
- Toggle: new `ambientLife` int (bird count). Low: 0 (nothing); Medium 3; High 8.

## Fit with progression
Landmarks (3), biomes (4), water (5), time of day (6) are the parameters progression.md Loop A levels 2-5 need (coast, canyon, rain mountains, snow dusk), so each becomes a `LevelDef` field later. Ideas 1, 2 and 7 improve every level now and carry over to Loops B/C.

## Recommended order (suggestion only)
1. Idea 7 (readable obstacles): small, works on Low, fixes "I could not see it" crashes.
2. Idea 1 (sky gradient and sun) with idea 2's backdrop gradient: the biggest change for Low players, who otherwise see a flat green field and blue sky.
3. Idea 3 (landmarks): orientation and screenshots for the itch.io page.
4. Idea 4 (with the flat per-biome colours) then 6.
5. Ideas 5 and 8 only if the owner wants more life.
Total for 1+2+3+7: about 5-6 runs.

## Questions for the owner
1. Bright daytime look, or moody variety (idea 6)?
2. Landmarks solid (crash risk) or decor only?
3. Water decor only, or should touching it do something?
4. Are new always-on-Low draws (sky, backdrop gradient, landmarks, obstacle bands) acceptable for the iPhone 11 budget, and a new `landmarks` bool plus `ambientLife` int in `GraphicsSettings`?
5. Should this wait for Loop A's `LevelDef` table so the ideas become per-level data from the start?
