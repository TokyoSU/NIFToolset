#pragma once
#ifndef NIINSTANCINGHELPER_H
#define NIINSTANCINGHELPER_H

#include "NiModelHelper.h"

#include <NiActorManager.h>
#include <NiAVObject.h>
#include <NiBillboardNode.h>
#include <NiCloningProcess.h>
#include <NiMesh.h>
#include <NiMeshHWInstance.h>
#include <NiInstancingUtilities.h>
#include <NiNode.h>
#include <NiRenderer.h>
#include <NiSwitchNode.h>
#include <NiTransform.h>

#include <vector>

// ---------------------------------------------------------------------------
// NiInstancingHelper
//
// Header-only helpers for efficient model reuse in NiApplication.
//
// - Static models use hardware instancing when supported by the renderer and
//   the source hierarchy can be represented by nodes + NiMesh leaves.
// - Static models that contain switching hierarchies such as NiSwitchNode or
//   NiLODNode fall back to exact cloning so their child-selection behavior is
//   preserved.
// - Animated models are cloned from a cached NiActorManager template so they
//   avoid repeated disk loads, but they are not hardware-instanced.
//
// Typical usage:
//   NiInstancingHelper::StaticTemplate kTrees;
//   NiInstancingHelper::StaticDesc kTreeDesc;
//   kTreeDesc.m_uiMaxInstances = 512;
//   NiInstancingHelper::LoadStaticTemplate("Data\\Tree.nif", kTrees, kTreeDesc);
//   auto spTree = NiInstancingHelper::CreateAttachedStaticInstance(kTrees, g_spScene);
//
//   NiInstancingHelper::AnimatedTemplate kNPCs;
//   NiInstancingHelper::LoadAnimatedTemplate("Data\\NPC.kfm", kNPCs);
//   auto spNPC = NiInstancingHelper::CreateAttachedAnimatedInstance(kNPCs, g_spScene);
// ---------------------------------------------------------------------------
namespace NiInstancingHelper
{
    struct StaticDesc
    {
        NiUInt32 m_uiMaxInstances = 256;
        NiUInt32 m_uiMaxIndicesPerSubmesh = 65536;
        bool     m_bCullPerInstance = true;
        bool     m_bStaticBounds = true;
        bool     m_bCPURead = true;
    };

    struct StaticBranch
    {
        NiFixedString                m_kName;
        NiTransform                  m_kLocal;
        NiPointer<NiMesh>            m_spMasterMesh;
        std::vector<StaticBranch>    m_kChildren;
    };

    struct StaticTemplate
    {
        NiPointer<NiAVObject> m_spMasterRoot;
        StaticBranch          m_kRoot;
        NiUInt32              m_uiMaxInstances = 0;
        bool                  m_bUsesHardwareInstancing = false;
    };

    struct AnimatedTemplate
    {
        NiPointer<NiActorManager> m_spMasterActor;
    };

    namespace Detail
    {
        inline bool SupportsHardwareInstancing()
        {
            NiRenderer* pkRenderer = NiRenderer::GetRenderer();
            return pkRenderer &&
                (pkRenderer->GetFlags() & NiRenderer::CAPS_HARDWAREINSTANCING) != 0;
        }

        inline NiPointer<NiAVObject> CloneExact(NiAVObject* pkRoot)
        {
            if (!pkRoot)
                return nullptr;

            NiCloningProcess kCloning;
            kCloning.m_eCopyType = NiObjectNET::COPY_EXACT;
            return NiDynamicCast(NiAVObject, pkRoot->Clone(kCloning));
        }

        inline void ResetTemplate(StaticTemplate& kTemplate)
        {
            kTemplate = StaticTemplate{};
        }

        inline void DisableMeshes(const std::vector<NiMesh*>& kMeshes)
        {
            for (NiMesh* pkMesh : kMeshes)
                NiInstancingUtilities::DisableMeshInstancing(pkMesh);
        }

        inline bool ContainsUnsupportedSwitching(NiAVObject* pkObject)
        {
            if (!pkObject)
                return false;

            if (NiDynamicCast(NiSwitchNode, pkObject) ||
                NiDynamicCast(NiBillboardNode, pkObject))
            {
                return true;
            }

            NiNode* pkNode = NiDynamicCast(NiNode, pkObject);
            if (!pkNode)
                return false;

            for (unsigned int ui = 0; ui < pkNode->GetChildCount(); ++ui)
            {
                NiAVObject* pkChild = pkNode->GetAt(ui);
                if (pkChild && ContainsUnsupportedSwitching(pkChild))
                    return true;
            }

            return false;
        }

        inline bool BuildStaticBranch(
            NiAVObject* pkObject,
            const StaticDesc& kDesc,
            StaticBranch& kBranch,
            std::vector<NiMesh*>& kEnabledMeshes,
            NiUInt32& uiTemplateCapacity,
            bool& bFoundMesh)
        {
            if (!pkObject)
                return false;

            kBranch.m_kName = pkObject->GetName();
            kBranch.m_kLocal = pkObject->GetLocalTransform();
            kBranch.m_kChildren.clear();
            kBranch.m_spMasterMesh = nullptr;

            if (NiMesh* pkMesh = NiDynamicCast(NiMesh, pkObject))
            {
                bFoundMesh = true;

                if (!pkMesh->GetInstanced())
                {
                    if (!NiInstancingUtilities::EnableMeshInstancing(
                        pkMesh,
                        kDesc.m_uiMaxInstances,
                        nullptr,
                        0,
                        kDesc.m_uiMaxIndicesPerSubmesh,
                        kDesc.m_bCullPerInstance,
                        kDesc.m_bStaticBounds,
                        kDesc.m_bCPURead))
                    {
                        return false;
                    }

                    kEnabledMeshes.push_back(pkMesh);
                }

                const NiUInt32 uiMeshCapacity =
                    NiInstancingUtilities::GetMaxInstanceCount(pkMesh);
                if (uiMeshCapacity == 0)
                    return false;

                uiTemplateCapacity = uiTemplateCapacity == 0 ?
                    uiMeshCapacity : NiMin(uiTemplateCapacity, uiMeshCapacity);
                kBranch.m_spMasterMesh = pkMesh;
                return true;
            }

            NiNode* pkNode = NiDynamicCast(NiNode, pkObject);
            if (!pkNode)
                return false;

            kBranch.m_kChildren.reserve(pkNode->GetChildCount());
            for (unsigned int ui = 0; ui < pkNode->GetChildCount(); ++ui)
            {
                NiAVObject* pkChild = pkNode->GetAt(ui);
                if (!pkChild)
                    continue;

                StaticBranch kChildBranch;
                if (!BuildStaticBranch(
                    pkChild,
                    kDesc,
                    kChildBranch,
                    kEnabledMeshes,
                    uiTemplateCapacity,
                    bFoundMesh))
                {
                    return false;
                }

                kBranch.m_kChildren.push_back(kChildBranch);
            }

            return true;
        }

        inline bool CanCreateStaticInstance(const StaticBranch& kBranch)
        {
            if (kBranch.m_spMasterMesh)
            {
                return NiInstancingUtilities::GetActiveInstanceCount(
                    kBranch.m_spMasterMesh) <
                    NiInstancingUtilities::GetMaxInstanceCount(
                        kBranch.m_spMasterMesh);
            }

            for (const StaticBranch& kChild : kBranch.m_kChildren)
            {
                if (!CanCreateStaticInstance(kChild))
                    return false;
            }

            return true;
        }

        inline NiPointer<NiAVObject> CreateStaticBranchInstance(
            const StaticBranch& kBranch)
        {
            if (kBranch.m_spMasterMesh)
            {
                NiMeshHWInstance* pkInstance =
                    NiNew NiMeshHWInstance(kBranch.m_spMasterMesh);
                NiPointer<NiAVObject> spResult = pkInstance;

                pkInstance->SetName(kBranch.m_kName);
                pkInstance->SetLocalTransform(kBranch.m_kLocal);
                NiInstancingUtilities::AddMeshInstance(
                    kBranch.m_spMasterMesh, pkInstance);

                if (pkInstance->GetMesh() != kBranch.m_spMasterMesh)
                    return nullptr;

                return spResult;
            }

            NiNode* pkNode =
                NiNew NiNode(static_cast<unsigned int>(kBranch.m_kChildren.size()));
            NiPointer<NiAVObject> spResult = pkNode;

            pkNode->SetName(kBranch.m_kName);
            pkNode->SetLocalTransform(kBranch.m_kLocal);

            for (const StaticBranch& kChild : kBranch.m_kChildren)
            {
                NiPointer<NiAVObject> spChild =
                    CreateStaticBranchInstance(kChild);
                if (!spChild)
                    return nullptr;

                pkNode->AttachChild(spChild);
            }

            return spResult;
        }

        inline void AttachIfRequested(
            NiAVObject* pkObject,
            NiNode* pkParent,
            float fUpdateTime)
        {
            if (!pkObject || !pkParent)
                return;

            pkParent->AttachChild(pkObject);
            pkParent->Update(fUpdateTime);
        }
    }

    [[nodiscard]] inline bool PrepareStaticTemplate(
        NiAVObject* pkStaticModel,
        StaticTemplate& kTemplate,
        const StaticDesc& kDesc = StaticDesc{})
    {
        Detail::ResetTemplate(kTemplate);
        if (!pkStaticModel)
            return false;

        kTemplate.m_spMasterRoot = pkStaticModel;

        if (Detail::ContainsUnsupportedSwitching(pkStaticModel))
            return true;

        if (!Detail::SupportsHardwareInstancing())
            return true;

        std::vector<NiMesh*> kEnabledMeshes;
        NiUInt32 uiCapacity = 0;
        bool bFoundMesh = false;

        if (!Detail::BuildStaticBranch(
            pkStaticModel,
            kDesc,
            kTemplate.m_kRoot,
            kEnabledMeshes,
            uiCapacity,
            bFoundMesh))
        {
            Detail::DisableMeshes(kEnabledMeshes);
            return true;
        }

        if (!bFoundMesh)
            return true;

        kTemplate.m_uiMaxInstances = uiCapacity;
        kTemplate.m_bUsesHardwareInstancing = uiCapacity > 0;
        return true;
    }

    [[nodiscard]] inline bool LoadStaticTemplate(
        const char* pcNIFPath,
        StaticTemplate& kTemplate,
        const StaticDesc& kDesc = StaticDesc{})
    {
        NiPointer<NiAVObject> spModel = NiModelHelper::LoadStatic(pcNIFPath);
        if (!spModel)
            return false;

        return PrepareStaticTemplate(spModel, kTemplate, kDesc);
    }

    [[nodiscard]] inline NiPointer<NiAVObject> CreateStaticInstance(
        const StaticTemplate& kTemplate)
    {
        if (!kTemplate.m_spMasterRoot)
            return nullptr;

        if (!kTemplate.m_bUsesHardwareInstancing)
            return Detail::CloneExact(kTemplate.m_spMasterRoot);

        if (!Detail::CanCreateStaticInstance(kTemplate.m_kRoot))
            return nullptr;

        return Detail::CreateStaticBranchInstance(kTemplate.m_kRoot);
    }

    [[nodiscard]] inline NiPointer<NiAVObject> CreateAttachedStaticInstance(
        const StaticTemplate& kTemplate,
        NiNode* pkParent,
        float fUpdateTime = 0.0f)
    {
        NiPointer<NiAVObject> spInstance = CreateStaticInstance(kTemplate);
        Detail::AttachIfRequested(spInstance, pkParent, fUpdateTime);
        return spInstance;
    }

    [[nodiscard]] inline bool PrepareAnimatedTemplate(
        NiActorManager* pkActor,
        AnimatedTemplate& kTemplate)
    {
        kTemplate = AnimatedTemplate{};
        if (!pkActor)
            return false;

        kTemplate.m_spMasterActor = pkActor;
        return true;
    }

    [[nodiscard]] inline bool LoadAnimatedTemplate(
        const char* pcKFMPath,
        AnimatedTemplate& kTemplate,
        bool bCumulativeAnimations = false)
    {
        NiPointer<NiActorManager> spActor =
            NiModelHelper::LoadAnimated(pcKFMPath, bCumulativeAnimations);
        if (!spActor)
            return false;

        return PrepareAnimatedTemplate(spActor, kTemplate);
    }

    [[nodiscard]] inline NiPointer<NiActorManager> CreateAnimatedInstance(
        const AnimatedTemplate& kTemplate)
    {
        if (!kTemplate.m_spMasterActor)
            return nullptr;

        return kTemplate.m_spMasterActor->Clone();
    }

    [[nodiscard]] inline NiPointer<NiActorManager> CreateAttachedAnimatedInstance(
        const AnimatedTemplate& kTemplate,
        NiNode* pkParent,
        float fUpdateTime = 0.0f)
    {
        NiPointer<NiActorManager> spActor = CreateAnimatedInstance(kTemplate);
        if (spActor)
            Detail::AttachIfRequested(spActor->GetNIFRoot(), pkParent, fUpdateTime);
        return spActor;
    }
}

#endif // NIINSTANCINGHELPER_H
