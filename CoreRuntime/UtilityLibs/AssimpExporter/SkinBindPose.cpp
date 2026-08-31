#include "SkinBindPose.h"

#include <NiGeometry.h>
#include <NiMesh.h>
#include <NiMeshModifier.h>
#include <NiNode.h>
#include <NiSkinData.h>
#include <NiSkinInstance.h>
#include <NiSkinningMeshModifier.h>

#include <iostream>
#include <unordered_map>

namespace
{
    //----------------------------------------------------------------------------------------------
    NiTransform MakeIdentityTransform()
    {
        NiTransform kTransform;
        kTransform.MakeIdentity();
        return kTransform;
    }

    //----------------------------------------------------------------------------------------------
    NiTransform ComputeWorldTransformFromParents(const NiAVObject* pkObject)
    {
        if (!pkObject)
            return MakeIdentityTransform();

        const NiNode* pkParent = pkObject->GetParent();
        if (!pkParent)
            return pkObject->GetLocalTransform();

        return ComputeWorldTransformFromParents(pkParent) *
            pkObject->GetLocalTransform();
    }

    //----------------------------------------------------------------------------------------------
    void StoreLocalBoneBindTransforms(
        const std::unordered_map<NiAVObject*, NiTransform>& kBoneWorldBind,
        BindPoseOverrideMap& kOut)
    {
        for (const auto& kEntry : kBoneWorldBind)
        {
            NiAVObject* pkBone = kEntry.first;
            const NiTransform& kBoneWorld = kEntry.second;
            if (!pkBone)
                continue;

            NiTransform kParentWorld = MakeIdentityTransform();
            NiNode* pkParent = pkBone->GetParent();
            if (pkParent)
            {
                const auto kParentBind = kBoneWorldBind.find(pkParent);
                if (kParentBind != kBoneWorldBind.end())
                    kParentWorld = kParentBind->second;
                else
                    kParentWorld = ComputeWorldTransformFromParents(pkParent);
            }

            NiTransform kWorldToParent;
            kParentWorld.Invert(kWorldToParent);
            kOut[pkBone] = kWorldToParent * kBoneWorld;
        }
    }

    //----------------------------------------------------------------------------------------------
    void CollectLegacySkinBindPose(NiGeometry* pkGeom,
        BindPoseOverrideMap& kOut)
    {
        if (!pkGeom)
            return;

        NiSkinInstance* pkSkin = pkGeom->GetSkinInstance();
        if (!pkSkin || !pkSkin->GetSkinData() || !pkSkin->GetRootParent())
            return;

        NiSkinData* pkSkinData = pkSkin->GetSkinData();
        const unsigned int uiBoneCount = pkSkinData->GetBoneCount();
        NiSkinData::BoneData* pkBoneData = pkSkinData->GetBoneData();
        NiAVObject* const* ppkBones = pkSkin->GetBones();
        if (uiBoneCount == 0 || !pkBoneData || !ppkBones)
            return;

        NiTransform kSkinToRootParent;
        pkSkinData->GetRootParentToSkin().Invert(kSkinToRootParent);
        const NiTransform kSkinWorld =
            ComputeWorldTransformFromParents(pkSkin->GetRootParent()) *
            kSkinToRootParent;

        std::unordered_map<NiAVObject*, NiTransform> kBoneWorldBind;
        kBoneWorldBind.reserve(uiBoneCount);

        for (unsigned int b = 0; b < uiBoneCount; ++b)
        {
            NiAVObject* pkBone = ppkBones[b];
            if (!pkBone)
                continue;

            NiTransform kBoneToSkin;
            pkBoneData[b].m_kSkinToBone.Invert(kBoneToSkin);
            kBoneWorldBind[pkBone] = kSkinWorld * kBoneToSkin;
        }

        const size_t stBefore = kOut.size();
        StoreLocalBoneBindTransforms(kBoneWorldBind, kOut);
        const size_t stAdded = kOut.size() - stBefore;
        if (stAdded > 0)
        {
            std::cerr << "    Built " << stAdded
                << " bind-pose bone transform override(s) from NiSkinInstance for mesh '"
                << (pkGeom->GetName().c_str() ? pkGeom->GetName().c_str() : "<unnamed>")
                << "'." << std::endl;
        }
    }

    //----------------------------------------------------------------------------------------------
    void CollectModernSkinBindPose(NiMesh* pkMesh,
        BindPoseOverrideMap& kOut)
    {
        if (!pkMesh)
            return;

        for (NiUInt32 m = 0; m < pkMesh->GetModifierCount(); ++m)
        {
            NiMeshModifier* pkModifier = pkMesh->GetModifierAt(m);
            if (!pkModifier || !NiIsKindOf(NiSkinningMeshModifier, pkModifier))
                continue;

            NiSkinningMeshModifier* pkSkin =
                NiStaticCast(NiSkinningMeshModifier, pkModifier);
            const NiUInt32 uiBoneCount = pkSkin->GetBoneCount();
            NiAVObject** ppkBones = pkSkin->GetBones();
            NiTransform* pkSkinToBone = pkSkin->GetSkinToBoneTransforms();
            NiAVObject* pkRootParent = pkSkin->GetRootBoneParent();
            if (uiBoneCount == 0 || !ppkBones || !pkSkinToBone || !pkRootParent)
                continue;

            NiTransform kSkinToRootParent;
            pkSkin->GetRootBoneParentToSkinTransform().Invert(kSkinToRootParent);
            const NiTransform kSkinWorld =
                ComputeWorldTransformFromParents(pkRootParent) *
                kSkinToRootParent;

            std::unordered_map<NiAVObject*, NiTransform> kBoneWorldBind;
            kBoneWorldBind.reserve(uiBoneCount);

            for (NiUInt32 b = 0; b < uiBoneCount; ++b)
            {
                NiAVObject* pkBone = ppkBones[b];
                if (!pkBone)
                    continue;

                NiTransform kBoneToSkin;
                pkSkinToBone[b].Invert(kBoneToSkin);
                kBoneWorldBind[pkBone] = kSkinWorld * kBoneToSkin;
            }

            const size_t stBefore = kOut.size();
            StoreLocalBoneBindTransforms(kBoneWorldBind, kOut);
            const size_t stAdded = kOut.size() - stBefore;
            if (stAdded > 0)
            {
                std::cerr << "    Built " << stAdded
                    << " bind-pose bone transform override(s) from NiSkinningMeshModifier for mesh '"
                    << (pkMesh->GetName().c_str() ? pkMesh->GetName().c_str() : "<unnamed>")
                    << "'." << std::endl;
            }
        }
    }
}

//--------------------------------------------------------------------------------------------------
void BuildSkinBindPoseOverrides(NiAVObject* pkRoot,
    BindPoseOverrideMap& kOut)
{
    if (!pkRoot)
        return;

    if (NiIsKindOf(NiGeometry, pkRoot))
        CollectLegacySkinBindPose(NiStaticCast(NiGeometry, pkRoot), kOut);
    else if (NiIsKindOf(NiMesh, pkRoot))
        CollectModernSkinBindPose(NiStaticCast(NiMesh, pkRoot), kOut);

    if (NiIsKindOf(NiNode, pkRoot))
    {
        NiNode* pkNode = NiStaticCast(NiNode, pkRoot);
        for (unsigned int i = 0; i < pkNode->GetArrayCount(); ++i)
            BuildSkinBindPoseOverrides(pkNode->GetAt(i), kOut);
    }
}
