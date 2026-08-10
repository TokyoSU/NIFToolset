# Skinned-animation bind-frame fix

The exported hierarchy uses bind transforms reconstructed from
`NiSkinInstance` or `NiSkinningMeshModifier`. These transforms are derived from
the same skin data as the exported bone offset matrices.

KF/KFM evaluators and embedded `NiTransformController` interpolators, however,
return absolute local transforms in the model NIF's original local bind frame.
Writing those values directly causes a bone to switch from the skin-derived
bind frame to the original frame as soon as an animation starts. The static
model can look correct while the animated mesh stretches or collapses.

`SkinBindPose.cpp` now builds one shared `BindPoseOverrideMap` used by both:

- `MeshExtractor::BuildNodeHierarchy`, and
- `AnimationExporter`.

For every animated bone with an exported bind override, each complete sampled
TRS matrix is rebased as:

```text
ExportedAnimatedLocal(t)
    = ExportedBindLocal
    * inverse(OriginalBindLocal)
    * OriginalAnimatedLocal(t)
```

At the source bind pose this evaluates exactly to `ExportedBindLocal`, and the
KF/KFM-authored local animation delta is preserved. Rebasing is performed after
all evaluator and NIF-controller components have been merged, so partial
position/rotation/scale channels are corrected as one transform rather than
independently.

The correction is applied before handedness reflection. Therefore it works for
all axis presets and for both right- and left-handed output.
