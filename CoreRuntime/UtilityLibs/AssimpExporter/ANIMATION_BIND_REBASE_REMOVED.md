# Animation and skin bind-space policy

Animation tracks are exported directly from the sampled NIF/KF/KFM local
position, rotation, and scale values. The exporter does not rebase animation
channels against a reconstructed or exported bind pose.

For skinning, the native stored SkinToBone transforms are authoritative:

- `NiSkinningMeshModifier::GetSkinToBoneTransforms()` for modern `NiMesh` data;
- `NiSkinData::BoneData::m_kSkinToBone` for legacy `NiGeometry` data.

Those transforms are the inverse-bind matrices used by Gamebryo and already map
from the skin/mesh bind space into each bone's bind space. Reconstructing them
from the current scene hierarchy is unsafe because the hierarchy can represent
a rest or controller pose that differs from the authored skin bind pose.
Hierarchy-derived inverse binds are used only as a fallback when a modern file
does not provide a finite stored transform.

Before Assimp writes the FBX, the complete scene is checked for non-finite node
matrices, vertices, normals, bone offsets, weights, and animation keys. Invalid
content is rejected with the exact mesh, bone, node, animation, or key name
instead of writing an FBX that later produces NaN bounds in Unity.
