# Scalar bone-animation consistency fix

`NiTransform` uses a single uniform scale. The FBX export path now preserves
that scalar exactly through negative-rest hierarchy isolation.

Previous behavior used:

```text
relative = inverse(restMatrix) * animatedMatrix
relative.Decompose(scale, rotation, position)
```

For a matrix with a negative determinant, decomposition is not unique. A
writer may represent the same transform as negative X scale plus one rotation
on one sample, then negative Y scale plus another rotation on a later sample.
That is mathematically equivalent as a matrix but is not stable as separate FBX
translation/rotation/scale curves.

The new path performs uniform-scale TRS algebra directly:

```text
relativePosition = rotate(inverse(restRotation),
                          animatedPosition - restPosition) / restScale
relativeRotation = inverse(restRotation) * animatedRotation
relativeScale    = animatedScale / restScale
```

Additional safeguards:

- Scale-only evaluators remain scale-only.
- Failed evaluator samples no longer fall back to the rest scale in the middle
  of a clip.
- Missing samples are interpolated from valid neighbors.
- X/Y/Z scale equality is validated before and after FBX writing.
- Zero and negative authored scalar scales are preserved and logged.
