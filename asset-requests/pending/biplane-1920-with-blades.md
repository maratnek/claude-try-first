# Biplane 1920 — rigged model with propeller blades

**Expected file:** `assets/models/biplane-1920.glb` (replace the current one)

**Why:** the current rigged export has a `propeller` node with only
`prop_hub` and `prop_blur_disc` children — no blades, so the propeller is
invisible at low RPM. The game currently draws placeholder blades in code
(`src/objects/plane.cpp`, `DrawPropellerBlades`); remove that once this
model lands.

**Prompt / change request for the generator:** same 1920s low-poly biplane
and the same node hierarchy and names as the current file (all pivot nodes
must keep their names and hinge positions), plus two wooden propeller
blades as a child of the `propeller` node, named `prop_blade_upper` and
`prop_blade_lower`, lying in the propeller's local XY plane, about 0.85 m
long each from the hub (the blur disc radius is 0.87), walnut wood
material, slight twist optional.
