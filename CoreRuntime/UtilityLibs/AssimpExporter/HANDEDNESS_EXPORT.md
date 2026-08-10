# FBX handedness

AssimpExporter writes right-handed FBX data by default.

```powershell
AssimpExporter -nif_folder "C:\Game\models" -output "C:\Export" -all
```

Use either spelling to request left-handed output:

```text
-left-handed
-left_handed
```

For example, Unity's +Z-forward convention is:

```powershell
AssimpExporter -nif_folder "C:\Game\models" -output "C:\Export\Unity" -all -unity_axes -left-handed
```

The left-handed conversion is target-axis-aware:

- Unreal axes reflect Y, preserving +X forward and +Z up;
- Unity axes reflect Z, changing -Z forward to +Z forward while preserving +Y up;
- native axes reflect X, preserving +Y forward and +Z up.

This avoids the old unconditional `aiProcess_ConvertToLeftHanded` behavior,
which always reflected Z and could invert a Z-up export. The exporter now
reflects the completed scene itself, then asks Assimp only to reverse triangle
winding. UV V coordinates are flipped in both handedness modes.
