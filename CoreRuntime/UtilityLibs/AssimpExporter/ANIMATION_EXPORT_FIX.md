# AssimpExporter animation export fix

The FBX animation path deliberately uses **seconds** for every Assimp key time:

```cpp
animation->mTicksPerSecond = 1.0;
animation->mDuration = durationSeconds;
```

The configured sample rate still controls how many baked samples are generated, but it no longer changes the external FBX time unit. This avoids timing differences between Assimp FBX exporter versions.

The exporter also no longer applies `aiProcessPreset_TargetRealtime_Quality` to the already-built `aiScene`. That preset is intended for imported runtime assets and contains cleanup/optimization operations that can alter baked animation tracks. Export now uses only:

```cpp
aiProcess_FlipUVs | aiProcess_ValidateDataStructure
```

For left-handed output, `aiProcess_FlipWindingOrder` is added after the explicit scene reflection.

These changes are independent of `-unity_axes`, `-unreal_axes`, `-native_axes`, `-left-handed`, and `-right-handed`.
