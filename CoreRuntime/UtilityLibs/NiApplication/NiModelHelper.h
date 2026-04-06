#pragma once
#ifndef NIMODELLOADER_H
#define NIMODELLOADER_H

#include <NiStream.h>
#include <NiNode.h>
#include <NiAVObject.h>
#include <NiActorManager.h>
#include <NiTexturingProperty.h>
#include <NiSourceTexture.h>
#include <NiCloningProcess.h>
#include <NiLODNode.h>
#include <NiStringExtraData.h>

#include <cfloat>
#include <cstdlib>
#include <cstring>

// ---------------------------------------------------------------------------
// NiModelHelper
//
// Inline helpers for loading, swapping, and attaching NIF/KFM models.
//
// Loading:
//   auto spMesh  = NiModelHelper::LoadStatic("Data\\Rock.nif");
//   auto spActor = NiModelHelper::LoadAnimated("Data\\Player.kfm");
//
// Full model swap (keeps the same scene-graph parent):
//   NiModelHelper::SwapStatic(spMesh, "Data\\NewRock.nif");
//   NiModelHelper::SwapAnimated(spActor, "Data\\NewPlayer.kfm");
//
//   — Capture old for later restore:
//   NiPointer<NiAVObject> spOld;
//   NiModelHelper::SwapStatic(spMesh, "Data\\NewRock.nif", 0.0f, &spOld);
//   NiModelHelper::SwapStatic(spMesh, "Data\\NewRock.nif", 0.0f, &spOld); // restore: SwapStatic(spMesh, ..., &spOld)
//
// Per-mesh swap (equipment piece replacement):
//   NiPointer<NiAVObject> spOldMesh;
//   NiModelHelper::SwapMesh(spActor, "Armor_Body", "Data\\NewArmor.nif", nullptr, 0.0f, &spOldMesh);
//
// Per-mesh texture swap:
//   NiPointer<NiTexture> spOldTex;
//   NiModelHelper::SwapBaseTexture(spActor, "Armor_Body", "Data\\armor_d.dds", &spOldTex);
//
// Bone / node attachment:
//   NiModelHelper::AttachToBone(spActor, "Bip01 R Hand", spSword);
//   NiModelHelper::DetachFromBone(spActor, "Bip01 R Hand", spSword);
//   NiModelHelper::AttachToNode(spMesh, "AttachPoint", spDecal);
//   NiModelHelper::DetachFromNode(spMesh, "AttachPoint", spDecal);
// 
// Hide/show a single object
//   NiModelHelper::Hide(pkSwordMesh);
//   NiModelHelper::Show(pkSwordMesh);
//   NiModelHelper::ToggleVisible(pkSwordMesh);
//
// Hide the whole scene, then selectively re-show one part
//   NiModelHelper::HideSubtree(g_spScene);
//   NiModelHelper::ShowByName(g_spScene, "Bip01 R Hand");
//
// Hide every NiMesh leaf in the scene
//   NiModelHelper::HideByType(g_spScene, NiGetRTTI(NiMesh));
//
// Query
//   if (!NiModelHelper::IsVisible(pkObj))
//     SDL_Log("Object is hidden.");
// ---------------------------------------------------------------------------
namespace NiModelHelper
{
    // -----------------------------------------------------------------------
    // Texture slots
    // -----------------------------------------------------------------------

    enum class TextureSlot
    {
        Base,
        Dark,
        Detail,
        Gloss,
        Glow,
        Normal,
        Bump,
        Parallax,
    };

    namespace Detail
    {
        inline bool TryParseKeepLODDistance(const NiAVObject* pkObject, float& fDistanceOut)
        {
            if (!pkObject)
                return false;

            const char* pcPrefix = "NiOptimizeKeepLODDistance = ";
            const size_t stPrefixLen = std::strlen(pcPrefix);

            const unsigned short usCount = pkObject->GetExtraDataSize();
            for (unsigned short us = 0; us < usCount; ++us)
            {
                NiStringExtraData* pkStringData =
                    NiDynamicCast(NiStringExtraData, pkObject->GetExtraDataAt(us));
                if (!pkStringData)
                    continue;

                const char* pcValue = pkStringData->GetValue();
                if (!pcValue)
                    continue;

                if (std::strncmp(pcValue, pcPrefix, stPrefixLen) != 0)
                    continue;

                char* pcEnd = nullptr;
                const float fParsed = std::strtof(pcValue + stPrefixLen, &pcEnd);
                if (pcEnd == pcValue + stPrefixLen)
                    continue;

                fDistanceOut = fParsed;
                return true;
            }

            return false;
        }

        inline int FindHighestDetailLODChildIndex(NiLODNode* pkLODRoot)
        {
            if (!pkLODRoot)
                return -1;

            int iBestIndex = -1;
            float fBestDistance = FLT_MAX;

            for (unsigned int ui = 0; ui < pkLODRoot->GetChildCount(); ++ui)
            {
                NiAVObject* pkChild = pkLODRoot->GetAt(ui);
                if (!pkChild)
                    continue;

                float fKeepDistance = 0.0f;
                if (TryParseKeepLODDistance(pkChild, fKeepDistance))
                {
                    if (iBestIndex < 0 || fKeepDistance < fBestDistance)
                    {
                        iBestIndex = static_cast<int>(ui);
                        fBestDistance = fKeepDistance;
                    }
                }
            }

            if (iBestIndex >= 0)
                return iBestIndex;

            for (unsigned int ui = 0; ui < pkLODRoot->GetChildCount(); ++ui)
            {
                NiAVObject* pkChild = pkLODRoot->GetAt(ui);
                if (pkChild && pkChild->GetName() == "lodobj0")
                    return static_cast<int>(ui);
            }

            return -1;
        }

        inline NiPointer<NiAVObject> ExtractHighestDetailLODChild(NiLODNode* pkLODNode)
        {
            if (!pkLODNode)
                return nullptr;

            const int iLODChild = FindHighestDetailLODChildIndex(pkLODNode);
            if (iLODChild < 0)
                return pkLODNode;

            NiAVObjectPtr spLODChild = pkLODNode->DetachChildAt(iLODChild);
            if (!spLODChild)
                return pkLODNode;

            const NiTransform kMerged =
                pkLODNode->GetLocalTransform() * spLODChild->GetLocalTransform();
            spLODChild->SetLocalTransform(kMerged);

            if (!spLODChild->GetName() && pkLODNode->GetName())
                spLODChild->SetName(pkLODNode->GetName());

            return spLODChild;
        }

        inline NiPointer<NiAVObject> ExtractHighestDetailLODRoot(NiAVObject* pkRoot)
        {
            if (!pkRoot)
                return nullptr;

            if (NiLODNode* pkLODRoot = NiDynamicCast(NiLODNode, pkRoot))
                return ExtractHighestDetailLODChild(pkLODRoot);

            NiNode* pkNode = NiDynamicCast(NiNode, pkRoot);
            if (!pkNode)
                return nullptr;

            for (unsigned int ui = 0; ui < pkNode->GetChildCount(); ++ui)
            {
                NiLODNode* pkLODRoot = NiDynamicCast(NiLODNode, pkNode->GetAt(ui));
                if (pkLODRoot)
                    return ExtractHighestDetailLODChild(pkLODRoot);
            }

            return nullptr;
        }

        inline NiPointer<NiAVObject> StripLODRecursive(NiAVObject* pkObject)
        {
            if (!pkObject)
                return nullptr;

            if (NiLODNode* pkLODNode = NiDynamicCast(NiLODNode, pkObject))
            {
                NiPointer<NiAVObject> spExtracted =
                    ExtractHighestDetailLODChild(pkLODNode);
                NiAVObject* pkExtracted = spExtracted;
                if (!pkExtracted || pkExtracted == pkLODNode)
                    return pkLODNode;

                return StripLODRecursive(pkExtracted);
            }

            NiNode* pkNode = NiDynamicCast(NiNode, pkObject);
            if (!pkNode)
                return pkObject;

            for (unsigned int ui = 0; ui < pkNode->GetArrayCount(); ++ui)
            {
                NiAVObject* pkChild = pkNode->GetAt(ui);
                if (!pkChild)
                    continue;

                NiPointer<NiAVObject> spNormalizedChild = StripLODRecursive(pkChild);
                NiAVObject* pkNormalizedChild = spNormalizedChild;
                if (pkNormalizedChild && pkNormalizedChild != pkChild)
                    pkNode->SetAt(ui, pkNormalizedChild);
            }

            return pkNode;
        }
    }

    // -----------------------------------------------------------------------
    // Loading
    // -----------------------------------------------------------------------

    // Loads a static .nif file.
    // Returns nullptr when the file is missing, empty, or the root is not
    // an NiAVObject.
    // If the root is an NiLODNode, the highest-detail child is extracted and
    // returned directly so runtime LOD switching is removed.
    inline NiPointer<NiAVObject> LoadStatic(const char* pcNIFPath)
    {
        if (!pcNIFPath)
            return nullptr;

        NiStream kStream;
        if (!kStream.Load(pcNIFPath))
            return nullptr;

        if (kStream.GetObjectCount() == 0)
            return nullptr;

        NiPointer<NiAVObject> spRoot =
            NiDynamicCast(NiAVObject, kStream.GetObjectAt(0));
        if (!spRoot)
            return nullptr;

        NiPointer<NiAVObject> spExtractedRoot = Detail::ExtractHighestDetailLODRoot(spRoot);
        if (spExtractedRoot)
            return Detail::StripLODRecursive(spExtractedRoot);

        return Detail::StripLODRecursive(spRoot);
    }

    // Loads an animated .kfm file (which internally loads the .nif and all
    // .kf sequences referenced by the .kfm).
    // Returns nullptr when any part of the load fails.
    // Set bCumulativeAnimations = true for root-motion animations.
    inline NiPointer<NiActorManager> LoadAnimated(const char* pcKFMPath,
        bool bCumulativeAnimations = false)
    {
        if (!pcKFMPath)
            return nullptr;

        return NiActorManager::Create(pcKFMPath, bCumulativeAnimations);
    }

    // -----------------------------------------------------------------------
    // Node / bone attachment (works on any NiAVObject hierarchy)
    // -----------------------------------------------------------------------

    // Finds a node named pcNodeName inside pkRoot's hierarchy and attaches
    // pkObject to it. Returns the NiNode on success, nullptr otherwise.
    // The scene-graph is updated at fUpdateTime after attachment.
    inline NiNode* AttachToNode(NiAVObject* pkRoot, const char* pcNodeName,
        NiAVObject* pkObject, float fUpdateTime = 0.0f)
    {
        if (!pkRoot || !pcNodeName || !pkObject)
            return nullptr;

        NiNode* pkNode = NiDynamicCast(NiNode, pkRoot->GetObjectByName(pcNodeName));
        if (!pkNode)
            return nullptr;

        pkNode->AttachChild(pkObject);
        pkNode->Update(fUpdateTime);
        return pkNode;
    }

    // Finds a node named pcNodeName inside pkRoot's hierarchy and detaches
    // pkObject from it. Returns true if the node was found and the child
    // was detached.
    inline bool DetachFromNode(NiAVObject* pkRoot, const char* pcNodeName,
        NiAVObject* pkObject)
    {
        if (!pkRoot || !pcNodeName || !pkObject)
            return false;

        NiNode* pkNode = NiDynamicCast(NiNode, pkRoot->GetObjectByName(pcNodeName));
        if (!pkNode)
            return false;

        return pkNode->DetachChild(pkObject) != nullptr;
    }

    // -----------------------------------------------------------------------
    // Bone attachment (actor shorthand — wraps AttachToNode/DetachFromNode)
    // -----------------------------------------------------------------------

    // Attaches pkObject to the named bone in the actor's NIF skeleton.
    // Returns the bone NiNode on success, nullptr otherwise.
    inline NiNode* AttachToBone(NiActorManager* pkActor, const char* pcBoneName,
        NiAVObject* pkObject, float fUpdateTime = 0.0f)
    {
        if (!pkActor)
            return nullptr;

        return AttachToNode(pkActor->GetNIFRoot(), pcBoneName, pkObject, fUpdateTime);
    }

    // Detaches pkObject from the named bone in the actor's NIF skeleton.
    // Returns true if the bone was found and the child was detached.
    inline bool DetachFromBone(NiActorManager* pkActor, const char* pcBoneName,
        NiAVObject* pkObject)
    {
        if (!pkActor)
            return false;

        return DetachFromNode(pkActor->GetNIFRoot(), pcBoneName, pkObject);
    }

    // -----------------------------------------------------------------------
    // Full model swap (preserves the existing scene-graph parent)
    // -----------------------------------------------------------------------

    // Detaches spModel from its current parent, loads pcNewNIFPath, and
    // re-attaches the new model to the same parent.
    // spModel is updated in-place. Returns false if the new NIF fails to load
    // (the old model is left untouched in that case).
    // Pass pspOldOut to receive the old model so it can be restored later.
    inline bool SwapStatic(NiPointer<NiAVObject>& spModel,
        const char* pcNewNIFPath, float fUpdateTime = 0.0f,
        NiPointer<NiAVObject>* pspOldOut = nullptr)
    {
        if (!pcNewNIFPath)
            return false;

        // Load first — do not disturb the old model if it fails.
        NiPointer<NiAVObject> spNew = LoadStatic(pcNewNIFPath);
        if (!spNew)
            return false;

        // Capture old before any modification.
        if (pspOldOut)
            *pspOldOut = spModel;

        if (spModel)
        {
            NiNode* pkParent = spModel->GetParent();
            if (pkParent)
            {
                pkParent->DetachChild(spModel);
                pkParent->AttachChild(spNew);
                pkParent->Update(fUpdateTime);
            }
        }

        spModel = spNew;
        return true;
    }

    // Detaches the old actor's NIF root from its parent, creates a new actor
    // from pcNewKFMPath, and re-attaches the new NIF root to the same parent.
    // spActor is updated in-place. Returns false if the new KFM fails to load
    // (the old actor is left untouched in that case).
    // Pass pspOldOut to receive the old actor so it can be restored later.
    inline bool SwapAnimated(NiPointer<NiActorManager>& spActor,
        const char* pcNewKFMPath, bool bCumulativeAnimations = false,
        float fUpdateTime = 0.0f,
        NiPointer<NiActorManager>* pspOldOut = nullptr)
    {
        if (!pcNewKFMPath)
            return false;

        // Load first — do not disturb the old actor if it fails.
        NiPointer<NiActorManager> spNew = LoadAnimated(pcNewKFMPath, bCumulativeAnimations);
        if (!spNew)
            return false;

        // Capture old before any modification.
        if (pspOldOut)
            *pspOldOut = spActor;

        if (spActor)
        {
            NiAVObject* pkOldRoot = spActor->GetNIFRoot();
            if (pkOldRoot)
            {
                NiNode* pkParent = pkOldRoot->GetParent();
                if (pkParent)
                {
                    pkParent->DetachChild(pkOldRoot);

                    NiAVObject* pkNewRoot = spNew->GetNIFRoot();
                    if (pkNewRoot)
                    {
                        pkParent->AttachChild(pkNewRoot);
                        pkParent->Update(fUpdateTime);
                    }
                }
            }
        }

        spActor = spNew;
        return true;
    }

    // -----------------------------------------------------------------------
    // Per-mesh swap — replaces a single named mesh within a hierarchy
    // -----------------------------------------------------------------------

    // Finds pcMeshName inside pkRoot's hierarchy, loads pcNewNIFPath, and
    // replaces the old mesh with either:
    //   - the root of the new NIF      (pcReplacementName == nullptr), or
    //   - a named child in the new NIF (pcReplacementName != nullptr).
    //
    // The replacement inherits the old mesh's name and local transform so
    // existing lookups and positioning continue to work.
    // Pass pspOldOut to receive the detached mesh so it can be restored later.
    // Returns the newly attached NiAVObject on success, nullptr otherwise.
    // The scene is NOT touched if the new NIF fails to load.
    inline NiAVObject* SwapMesh(NiAVObject* pkRoot, const char* pcMeshName,
        const char* pcNewNIFPath, const char* pcReplacementName = nullptr,
        float fUpdateTime = 0.0f,
        NiPointer<NiAVObject>* pspOldOut = nullptr)
    {
        if (!pkRoot || !pcMeshName || !pcNewNIFPath)
            return nullptr;

        NiAVObject* pkOld = pkRoot->GetObjectByName(pcMeshName);
        if (!pkOld)
            return nullptr;

        NiNode* pkParent = pkOld->GetParent();
        if (!pkParent)
            return nullptr;

        // Load new NIF — bail before touching the scene on failure.
        NiStream kStream;
        if (!kStream.Load(pcNewNIFPath) || kStream.GetObjectCount() == 0)
            return nullptr;

        NiPointer<NiAVObject> spNewRoot =
            NiDynamicCast(NiAVObject, kStream.GetObjectAt(0));
        if (!spNewRoot)
            return nullptr;

        // Pick the replacement from within the loaded hierarchy.
        NiPointer<NiAVObject> spNew;
        if (pcReplacementName)
        {
            NiAVObject* pkFound = spNewRoot->GetObjectByName(pcReplacementName);
            if (!pkFound)
                return nullptr;

            // Detach from its loaded parent; hold via smart pointer so the
            // ref count doesn't drop to zero mid-swap.
            NiNode* pkFoundParent = pkFound->GetParent();
            if (pkFoundParent)
                spNew = pkFoundParent->DetachChild(pkFound);
            else
                spNew = pkFound;
        }
        else
        {
            spNew = spNewRoot;
        }

        if (!spNew)
            return nullptr;

        // Capture old before detaching — smart pointer keeps it alive.
        if (pspOldOut)
            *pspOldOut = pkOld;

        // Preserve the old mesh's name and local transform.
        spNew->SetName(pkOld->GetName());
        spNew->SetTranslate(pkOld->GetTranslate());
        spNew->SetRotate(pkOld->GetRotate());
        spNew->SetScale(pkOld->GetScale());

        pkParent->DetachChild(pkOld);
        pkParent->AttachChild(spNew);
        pkParent->Update(fUpdateTime);
        return spNew;
    }

    // Shorthand for animated actors.
    inline NiAVObject* SwapMesh(NiActorManager* pkActor, const char* pcMeshName,
        const char* pcNewNIFPath, const char* pcReplacementName = nullptr,
        float fUpdateTime = 0.0f,
        NiPointer<NiAVObject>* pspOldOut = nullptr)
    {
        if (!pkActor)
            return nullptr;

        return SwapMesh(pkActor->GetNIFRoot(), pcMeshName,
            pcNewNIFPath, pcReplacementName, fUpdateTime, pspOldOut);
    }

    // -----------------------------------------------------------------------
    // Per-mesh texture swap — replaces one texture slot on a named mesh
    // -----------------------------------------------------------------------

    // Finds pcMeshName inside pkRoot, retrieves its NiTexturingProperty, and
    // replaces the texture in eSlot. Creates the map entry if it is missing.
    // Pass pspOldTexOut to receive the old NiTexture so it can be restored later.
    // Returns true on success.
    inline bool SwapTexture(NiAVObject* pkRoot, const char* pcMeshName,
        TextureSlot eSlot, const char* pcNewTexturePath,
        NiPointer<NiTexture>* pspOldTexOut = nullptr)
    {
        if (!pkRoot || !pcMeshName || !pcNewTexturePath)
            return false;

        NiAVObject* pkMesh = pkRoot->GetObjectByName(pcMeshName);
        if (!pkMesh)
            return false;

        NiTexturingProperty* pkTP = NiDynamicCast(NiTexturingProperty,
            pkMesh->GetProperty(NiTexturingProperty::GetType()));
        if (!pkTP)
            return false;

        NiTexture* pkTex = NiSourceTexture::Create(pcNewTexturePath);
        if (!pkTex)
            return false;

        switch (eSlot)
        {
            case TextureSlot::Base:
            {
                NiTexturingProperty::Map* pkMap = pkTP->GetBaseMap();
                if (pspOldTexOut) *pspOldTexOut = pkMap ? pkMap->GetTexture() : nullptr;
                if (pkMap) pkMap->SetTexture(pkTex);
                else pkTP->SetBaseMap(NiNew NiTexturingProperty::Map(pkTex, 0));
                break;
            }
            case TextureSlot::Dark:
            {
                NiTexturingProperty::Map* pkMap = pkTP->GetDarkMap();
                if (pspOldTexOut) *pspOldTexOut = pkMap ? pkMap->GetTexture() : nullptr;
                if (pkMap) pkMap->SetTexture(pkTex);
                else pkTP->SetDarkMap(NiNew NiTexturingProperty::Map(pkTex, 0));
                break;
            }
            case TextureSlot::Detail:
            {
                NiTexturingProperty::Map* pkMap = pkTP->GetDetailMap();
                if (pspOldTexOut) *pspOldTexOut = pkMap ? pkMap->GetTexture() : nullptr;
                if (pkMap) pkMap->SetTexture(pkTex);
                else pkTP->SetDetailMap(NiNew NiTexturingProperty::Map(pkTex, 0));
                break;
            }
            case TextureSlot::Gloss:
            {
                NiTexturingProperty::Map* pkMap = pkTP->GetGlossMap();
                if (pspOldTexOut) *pspOldTexOut = pkMap ? pkMap->GetTexture() : nullptr;
                if (pkMap) pkMap->SetTexture(pkTex);
                else pkTP->SetGlossMap(NiNew NiTexturingProperty::Map(pkTex, 0));
                break;
            }
            case TextureSlot::Glow:
            {
                NiTexturingProperty::Map* pkMap = pkTP->GetGlowMap();
                if (pspOldTexOut) *pspOldTexOut = pkMap ? pkMap->GetTexture() : nullptr;
                if (pkMap) pkMap->SetTexture(pkTex);
                else pkTP->SetGlowMap(NiNew NiTexturingProperty::Map(pkTex, 0));
                break;
            }
            case TextureSlot::Normal:
            {
                NiTexturingProperty::Map* pkMap = pkTP->GetNormalMap();
                if (pspOldTexOut) *pspOldTexOut = pkMap ? pkMap->GetTexture() : nullptr;
                if (pkMap) pkMap->SetTexture(pkTex);
                else pkTP->SetNormalMap(NiNew NiTexturingProperty::Map(pkTex, 0));
                break;
            }
            case TextureSlot::Bump:
            {
                NiTexturingProperty::BumpMap* pkMap = pkTP->GetBumpMap();
                if (pspOldTexOut) *pspOldTexOut = pkMap ? pkMap->GetTexture() : nullptr;
                if (pkMap) pkMap->SetTexture(pkTex);
                else pkTP->SetBumpMap(NiNew NiTexturingProperty::BumpMap(pkTex, 0));
                break;
            }
            case TextureSlot::Parallax:
            {
                NiTexturingProperty::ParallaxMap* pkMap = pkTP->GetParallaxMap();
                if (pspOldTexOut) *pspOldTexOut = pkMap ? pkMap->GetTexture() : nullptr;
                if (pkMap) pkMap->SetTexture(pkTex);
                else pkTP->SetParallaxMap(NiNew NiTexturingProperty::ParallaxMap(pkTex, 0));
                break;
            }
            default:
                return false;
        }

        return true;
    }

    // Shorthand for animated actors.
    inline bool SwapTexture(NiActorManager* pkActor, const char* pcMeshName,
        TextureSlot eSlot, const char* pcNewTexturePath,
        NiPointer<NiTexture>* pspOldTexOut = nullptr)
    {
        if (!pkActor)
            return false;

        return SwapTexture(pkActor->GetNIFRoot(), pcMeshName,
            eSlot, pcNewTexturePath, pspOldTexOut);
    }

    // Convenience shorthands for the most common slots.
    inline bool SwapBaseTexture(NiAVObject* pkRoot, const char* pcMeshName,
        const char* pcPath, NiPointer<NiTexture>* pspOldTexOut = nullptr)
    { return SwapTexture(pkRoot, pcMeshName, TextureSlot::Base, pcPath, pspOldTexOut); }

    inline bool SwapNormalTexture(NiAVObject* pkRoot, const char* pcMeshName,
        const char* pcPath, NiPointer<NiTexture>* pspOldTexOut = nullptr)
    { return SwapTexture(pkRoot, pcMeshName, TextureSlot::Normal, pcPath, pspOldTexOut); }

    inline bool SwapGlowTexture(NiAVObject* pkRoot, const char* pcMeshName,
        const char* pcPath, NiPointer<NiTexture>* pspOldTexOut = nullptr)
    { return SwapTexture(pkRoot, pcMeshName, TextureSlot::Glow, pcPath, pspOldTexOut); }

    inline bool SwapBaseTexture(NiActorManager* pkActor, const char* pcMeshName,
        const char* pcPath, NiPointer<NiTexture>* pspOldTexOut = nullptr)
    { return SwapTexture(pkActor, pcMeshName, TextureSlot::Base, pcPath, pspOldTexOut); }

    inline bool SwapNormalTexture(NiActorManager* pkActor, const char* pcMeshName,
        const char* pcPath, NiPointer<NiTexture>* pspOldTexOut = nullptr)
    { return SwapTexture(pkActor, pcMeshName, TextureSlot::Normal, pcPath, pspOldTexOut); }

    inline bool SwapGlowTexture(NiActorManager* pkActor, const char* pcMeshName,
        const char* pcPath, NiPointer<NiTexture>* pspOldTexOut = nullptr)
    { return SwapTexture(pkActor, pcMeshName, TextureSlot::Glow, pcPath, pspOldTexOut); }

    // -------------------------------------------------------------------
    // Single-object visibility
    // -------------------------------------------------------------------

    /// Hide pkObject (SetAppCulled(true)). Its subtree is implicitly skipped
    /// by the culler when its root is hidden.
    inline void Hide(NiAVObject* pkObject)
    {
        if (pkObject)
            pkObject->SetAppCulled(true);
    }

    /// Show pkObject (SetAppCulled(false)).
    inline void Show(NiAVObject* pkObject)
    {
        if (pkObject)
            pkObject->SetAppCulled(false);
    }

    /// Set visibility of pkObject directly.
    inline void SetVisible(NiAVObject* pkObject, bool bVisible)
    {
        if (pkObject)
            pkObject->SetAppCulled(!bVisible);
    }

    /// Returns true when the object is NOT app-culled (i.e. potentially visible).
    /// Note: an object may still be frustum-culled at render time.
    inline bool IsVisible(const NiAVObject* pkObject)
    {
        return pkObject && !pkObject->GetAppCulled();
    }

    /// Toggle the current visibility state.
    inline void ToggleVisible(NiAVObject* pkObject)
    {
        if (pkObject)
            pkObject->SetAppCulled(!pkObject->GetAppCulled());
    }

    // -------------------------------------------------------------------
    // Subtree visibility — walks every descendant of pkRoot
    //
    // Note: hiding a NiNode already skips its entire subtree during culling.
    // Use HideSubtree when you need each child's AppCulled flag set
    // individually (e.g. so that re-showing individual children works later).
    // -------------------------------------------------------------------

    namespace Detail
    {
        inline void SetSubtreeCulled(NiNode* pkNode, bool bCulled)
        {
            if (!pkNode)
                return;
            pkNode->SetAppCulled(bCulled);
            for (unsigned int i = 0; i < pkNode->GetArrayCount(); ++i)
            {
                NiAVObject* pkChild = pkNode->GetAt(i);
                if (!pkChild)
                    continue;
                NiNode* pkChildNode = NiDynamicCast(NiNode, pkChild);
                if (pkChildNode)
                    SetSubtreeCulled(pkChildNode, bCulled); // recurse
                else
                    pkChild->SetAppCulled(bCulled);
            }
        }
    }

    /// Hide pkRoot and every object in its subtree.
    inline void HideSubtree(NiNode* pkRoot)
    {
        Detail::SetSubtreeCulled(pkRoot, true);
    }

    /// Show pkRoot and every object in its subtree.
    inline void ShowSubtree(NiNode* pkRoot)
    {
        Detail::SetSubtreeCulled(pkRoot, false);
    }

    /// Set visibility of pkRoot and every object in its subtree.
    inline void SetSubtreeVisible(NiNode* pkRoot, bool bVisible)
    {
        Detail::SetSubtreeCulled(pkRoot, !bVisible);
    }

    // -------------------------------------------------------------------
    // Name-based visibility — searches the subtree of pkRoot
    // Uses NiAVObject::GetObjectByName which performs a depth-first search.
    // -------------------------------------------------------------------

    /// Find the first object named kName under pkRoot and hide it.
    /// Returns the found object, or nullptr if not found.
    inline NiAVObject* HideByName(NiNode* pkRoot, const NiFixedString& kName)
    {
        if (!pkRoot)
            return nullptr;
        NiAVObject* pkObj = pkRoot->GetObjectByName(kName);
        if (pkObj)
            pkObj->SetAppCulled(true);
        return pkObj;
    }

    /// Find the first object named kName under pkRoot and show it.
    /// Returns the found object, or nullptr if not found.
    inline NiAVObject* ShowByName(NiNode* pkRoot, const NiFixedString& kName)
    {
        if (!pkRoot)
            return nullptr;
        NiAVObject* pkObj = pkRoot->GetObjectByName(kName);
        if (pkObj)
            pkObj->SetAppCulled(false);
        return pkObj;
    }

    /// Find the first object named kName under pkRoot and set its visibility.
    /// Returns the found object, or nullptr if not found.
    inline NiAVObject* SetVisibleByName(NiNode* pkRoot,
        const NiFixedString& kName, bool bVisible)
    {
        if (!pkRoot)
            return nullptr;
        NiAVObject* pkObj = pkRoot->GetObjectByName(kName);
        if (pkObj)
            pkObj->SetAppCulled(!bVisible);
        return pkObj;
    }

    // -------------------------------------------------------------------
    // Type-based visibility — hides/shows all objects of a given RTTI type
    // -------------------------------------------------------------------

    /// Hide all objects of type pkRTTI in the subtree of pkRoot.
    inline void HideByType(NiNode* pkRoot, const NiRTTI* pkRTTI)
    {
        if (!pkRoot || !pkRTTI)
            return;
        NiTPointerList<NiAVObject*> kObjects;
        pkRoot->GetObjectsByType(pkRTTI, kObjects);
        NiTListIterator kIter = kObjects.GetHeadPos();
        while (kIter)
            kObjects.GetNext(kIter)->SetAppCulled(true);
    }

    /// Show all objects of type pkRTTI in the subtree of pkRoot.
    inline void ShowByType(NiNode* pkRoot, const NiRTTI* pkRTTI)
    {
        if (!pkRoot || !pkRTTI)
            return;
        NiTPointerList<NiAVObject*> kObjects;
        pkRoot->GetObjectsByType(pkRTTI, kObjects);
        NiTListIterator kIter = kObjects.GetHeadPos();
        while (kIter)
            kObjects.GetNext(kIter)->SetAppCulled(false);
    }

    // -----------------------------------------------------------------------
    // Instancing — cheap scene-graph copies sharing geometry/texture data
    // -----------------------------------------------------------------------

    // Clones a previously loaded NIF root using NiObject::Clone().
    // The clone shares all geometry and texture data with the source
    // (COPY_EXACT semantics) — no re-streaming, no GPU resource duplication.
    //
    // Typical usage:
    //   auto spMaster   = NiModelHelper::LoadStatic("Data\\Rock.nif");
    //   auto spInst1    = NiModelHelper::CloneStatic(spMaster, NiPoint3(10, 0, 0));
    //   auto spInst2    = NiModelHelper::CloneStatic(spMaster, NiPoint3(20, 0, 0));
    //   pkScene->AttachChild(spInst1); pkScene->AttachChild(spInst2);
    //
    // Pass COPY_UNIQUE as eCopyType to give each clone unique node names
    // (needed when you search by name inside the cloned hierarchy).
    inline NiPointer<NiAVObject> CloneStatic(
        NiAVObject* pkSource,
        const NiPoint3& kTranslate = NiPoint3::ZERO,
		const NiPoint3& kRotate = NiPoint3::ZERO,
        float fScale = 1.0f,
        NiObjectNET::CopyType eCopyType = NiObjectNET::COPY_EXACT)
    {
        if (!pkSource)
            return nullptr;

        NiCloningProcess kCloning;
        kCloning.m_eCopyType = eCopyType;

        NiPointer<NiAVObject> spClone = NiDynamicCast(NiAVObject, pkSource->Clone(kCloning));
        if (!spClone)
            return nullptr;

		NiTransform kTransform;
        kTransform.MakeIdentity();
        NiMatrix3 kRot;
        kRot.FromEulerAnglesXYZ(kRotate.x, kRotate.y, kRotate.z);
		kTransform.m_Translate = kTranslate;
		kTransform.m_Rotate = kRot;
		kTransform.m_fScale = fScale;
        spClone->SetLocalTransform(kTransform);

        return spClone;
    }

    // Clones a previously loaded NIF root using NiObject::Clone().
    // The clone shares all geometry and texture data with the source
    // (COPY_EXACT semantics) — no re-streaming, no GPU resource duplication.
    //
    // Typical usage:
    //   auto spMaster   = NiModelHelper::LoadStatic("Data\\Rock.nif");
    //   auto spInst1    = NiModelHelper::CloneStatic(spMaster, NiPoint3(10, 0, 0));
    //   auto spInst2    = NiModelHelper::CloneStatic(spMaster, NiPoint3(20, 0, 0));
    //   pkScene->AttachChild(spInst1); pkScene->AttachChild(spInst2);
    //
    // Pass COPY_UNIQUE as eCopyType to give each clone unique node names
    // (needed when you search by name inside the cloned hierarchy).
    inline NiPointer<NiAVObject> CloneStatic(
        NiAVObject* pkSource,
		const NiTransform& kTransform,
        NiObjectNET::CopyType eCopyType = NiObjectNET::COPY_EXACT)
    {
        if (!pkSource)
            return nullptr;

        NiCloningProcess kCloning;
        kCloning.m_eCopyType = eCopyType;

        NiPointer<NiAVObject> spClone = NiDynamicCast(NiAVObject, pkSource->Clone(kCloning));
        if (!spClone)
            return nullptr;

        spClone->SetLocalTransform(kTransform);
        return spClone;
    }

    // Shorthand for animated actors — clones the NIF hierarchy inside the actor.
    // Note: the returned clone is a raw NIF scene-graph node, not a full actor.
    // Use LoadAnimated() + CloneStatic(spActor->GetNIFRoot()) for animated instances.
    inline NiPointer<NiAVObject> CloneAnimated(
        NiActorManager* pkActor,
        const NiPoint3& kTranslate = NiPoint3::ZERO,
        const NiPoint3& kRotate = NiPoint3::ZERO,
        float fScale = 1.0f,
        NiObjectNET::CopyType eCopyType = NiObjectNET::COPY_EXACT)
    {
        if (!pkActor)
            return nullptr;

        return CloneStatic(pkActor->GetNIFRoot(), kTranslate, kRotate, fScale, eCopyType);
    }

    // Shorthand for animated actors — clones the NIF hierarchy inside the actor.
    // Note: the returned clone is a raw NIF scene-graph node, not a full actor.
    // Use LoadAnimated() + CloneStatic(spActor->GetNIFRoot()) for animated instances.
    inline NiPointer<NiAVObject> CloneAnimated(
        NiActorManager* pkActor,
		const NiTransform& kTransform,
        NiObjectNET::CopyType eCopyType = NiObjectNET::COPY_EXACT)
    {
        if (!pkActor)
            return nullptr;

        return CloneStatic(pkActor->GetNIFRoot(), kTransform, eCopyType);
    }
}

#endif // NIMODELLOADER_H