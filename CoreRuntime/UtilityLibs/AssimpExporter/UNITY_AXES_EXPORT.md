# Unity axis export

Use `-unity_axes` to export model, skeleton, bind-pose, animation, and terrain
data with Y as the up axis.

The exporter is right-handed by default. In that mode the Unity-style basis is:

- positive X points right;
- positive Y points up;
- negative Z points forward.

Use `-left-handed` when the FBX must match Unity's runtime convention directly:

- positive X points right;
- positive Y points up;
- positive Z points forward.

Example for Unity's left-handed convention:

```powershell
AssimpExporter `
  -nif_folder "C:\Game\models" `
  -texture_folder "C:\Game\textures" `
  -output "C:\Export\Unity" `
  -all `
  -unity_axes `
  -left-handed
```

Axis options are mutually exclusive. When more than one is supplied, the last
axis option on the command line wins:

- `-unreal_axes`: +X forward and +Z up; this remains the default axis preset.
- `-unity_axes`: Y up, with -Z forward in right-handed mode or +Z forward in left-handed mode.
- `-native_axes`: retain the source +Y-forward and +Z-up orientation.
- `-no_unreal_axes`: compatibility alias for `-native_axes`.

Handedness options are also last-one-wins:

- right-handed output is the default;
- `-left-handed` or `-left_handed` selects left-handed output;
- `-right-handed` or `-right_handed` explicitly restores right-handed output.

The conversion is applied to all coordinate-bearing data rather than only mesh
positions. This includes normals, node transforms, skin-to-bone matrices, rest
poses, position keys, rotation keys, cameras, and lights. Triangle winding is
reversed only for left-handed output. UV V coordinates are flipped in both
modes to preserve the exporter's existing FBX texture orientation.
