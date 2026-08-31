# Unity skeletal-animation export safeguards

The FBX writer now protects animated Gamebryo skeletons in four ways:

1. Sequence components that are identical to the static rest transform are no longer baked for every sample. A component that is constant but differs from rest is reduced to one key.
2. Assimp creates translation, rotation, and scale curves for every `aiNodeAnim`. Missing components therefore receive one local rest key instead of a full sampled track. This avoids old Assimp versions deriving an incorrect local default from a world transform.
3. A node with a negative static scale is exported as:

   ```text
   NIFToolset_StaticRest_<node>   (original static transform)
     <node>                       (identity rest transform, animated)
   ```

   Animation keys on `<node>` are converted to rest-relative transforms. The skin/bone name remains unchanged, but animated scale is positive and normally reduces to a single identity key.
4. The generated FBX is imported again after writing. Export fails when axis metadata, animation count, finite-value validation, or skeleton-scale validation does not survive the FBX writer.

Assimp 6.0.4 or newer is recommended because its FBX exporter reliably writes FBX 7.5 axis metadata. Assimp 6.0.0 is supported: the exporter emits a warning and performs post-write finite-data, animation-count, and skeleton-scale validation. With Assimp older than 6.0.4, use `-unity_axes -left-handed` for Unity because older FBX exporters may ignore custom axis metadata.

## Unity import settings

For a Unity-targeted file, export with:

```text
-unity_axes -left-handed
```

Use the **Generic** rig type while validating the source animation, enable **Import Animation**, and enable **Preserve Hierarchy**. The synthetic `NIFToolset_StaticRest_*` parent nodes are part of the bind hierarchy and must not be stripped.

## Uniform bone-scale preservation

Gamebryo `NiTransform` stores one scalar scale value, not independent X/Y/Z
scale. Negative-rest nodes are therefore converted analytically when their
static rest transform is moved to `NIFToolset_StaticRest_*`:

```text
relativePosition = inverse(restScale) * inverse(restRotation)
                   * (animatedPosition - restPosition)
relativeRotation = inverse(restRotation) * animatedRotation
relativeScale    = animatedScale / restScale
```

The exporter no longer decomposes a reflected matrix for every animation
sample. Matrix decomposition can move a negative determinant to a different
axis and compensate with a 180-degree rotation, producing inconsistent scale
curves in Unity. Scale-only evaluators also remain scale-only; they no longer
force generated position and rotation tracks.

Each evaluator/controller sample records whether its position, rotation, and
scale value was actually produced. Missing samples inside an otherwise valid
track are interpolated between neighboring valid samples; leading/trailing
missing samples use the nearest valid value instead of silently reverting to
the model rest pose.

The export log reports:

```text
repaired samples(T/R/S)=0/0/0
Animated scalar scale: channels=2, samples=84, range=[0.25, 1.5], non-positive=0.
```

The final scene validator rejects non-uniform generated bone scale because NIF
scalar scale must remain equal on X/Y/Z. Authored zero or negative scalar keys
are preserved rather than rejected; they are reported by the `non-positive`
counter for diagnosis.

## Assimp 6.0.x full-TRS compatibility

Do not reduce rest or constant animation components when using the Assimp 6.0.x
FBX exporter. Every animated node is exported with complete sampled local
translation, rotation, and scalar scale arrays. This avoids world-transform
fallback values being interpreted as local FBX curve defaults.
