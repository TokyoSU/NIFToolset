#pragma once

#include <NiAVObject.h>
#include <NiTransform.h>

#include <unordered_map>

// Local bind-pose overrides derived from NiSkinInstance and
// NiSkinningMeshModifier data. The NIF skin data is the authoritative source
// for the skeleton bind pose used by aiBone offsets and FBX skin clusters.
using BindPoseOverrideMap = std::unordered_map<NiAVObject*, NiTransform>;

// Collect corrected local bind transforms for every bone referenced by a
// skinned object below pkRoot. The resulting transforms use the original NIF
// parent hierarchy and are shared by both hierarchy and animation export.
void BuildSkinBindPoseOverrides(NiAVObject* pkRoot,
    BindPoseOverrideMap& kOut);
