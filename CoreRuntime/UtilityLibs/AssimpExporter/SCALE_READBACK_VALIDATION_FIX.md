# Scalar scale read-back validation

`NiTransform` stores one scalar scale. Before FBX writing, every skeleton
animation scale key is now checked strictly so X, Y, and Z are equal.

Assimp's FBX importer can decompose the same FBX transform into signed or
slightly non-uniform XYZ scale when a reflected/negative-scale ancestor exists.
That round-trip representation is not the raw scale curve written by the FBX
exporter. It must not cause a valid FBX to be deleted.

The post-write verifier therefore:

- still rejects non-finite and excessive scale values;
- reports read-back non-uniformity with the exact node, animation, key, value,
  and maximum magnitude spread;
- treats decomposition drift as a warning because the source curves were
  already verified strictly uniform before writing;
- never rewrites the FBX from the read-back Assimp scene.

Expected warning example:

```
Warning: Assimp FBX read-back decomposed 12 scalar-scale key(s) into
signed/non-uniform XYZ values. The pre-export curves were verified strictly
uniform; this round-trip decomposition is not used to rewrite the FBX.
```
