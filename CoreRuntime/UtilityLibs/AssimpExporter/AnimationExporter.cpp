#include "AnimationExporter.h"
#include "ExportNaming.h"
#include "AxisConversion.h"

#include <NiNode.h>
#include <NiTimeController.h>
#include <NiControllerManager.h>
#ifndef EE_REMOVE_BACK_COMPAT_STREAMING
#include <NiMultiTargetTransformController.h>
#endif
#include <NiTransformController.h>
#include <NiTransformInterpolator.h>
#include <NiInterpolator.h>
#include <NiTransformEvaluator.h>
#include <NiConstTransformEvaluator.h>
#include <NiBSplineTransformEvaluator.h>
#include <NiSequenceData.h>
#include <NiScratchPad.h>
#include <NiEvaluatorSPData.h>
#include <NiQuatTransform.h>
#include <NiQuaternion.h>

#include <assimp/scene.h>
#include <assimp/anim.h>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace
{
    constexpr float DEFAULT_SAMPLE_RATE = 30.0f;
    constexpr float MIN_POSITIVE = 0.000001f;
    constexpr unsigned int MAX_BAKED_SAMPLES = 1000000u;

    using NodeByNameMap = std::unordered_map<std::string, NiAVObject*>;

    struct SampleGrid
    {
        float sampleRate = DEFAULT_SAMPLE_RATE;
        float durationSeconds = 0.0f;
        std::vector<float> localTimes;
        std::vector<double> ticks;
    };

    struct BakedNodeTrack
    {
        std::string name;
        NiAVObject* restObject = nullptr;
        std::vector<aiVectorKey> positions;
        std::vector<aiQuatKey> rotations;
        std::vector<aiVectorKey> scales;
        bool hasAnimationData = false;
        bool hasPositionSource = false;
        bool hasRotationSource = false;
        bool hasScaleSource = false;
    };

    struct TransformComponentMask
    {
        bool position = false;
        bool rotation = false;
        bool scale = false;
    };

    struct ControllerBakeStats
    {
        unsigned int controllersBaked = 0;
        unsigned int controllerSamplesFailed = 0;
        unsigned int skippedAccumulationRoots = 0;
        unsigned int protectedPositionComponents = 0;
        unsigned int protectedRotationComponents = 0;
        unsigned int protectedScaleComponents = 0;
    };

    //----------------------------------------------------------------------------------------------
    aiQuaternion ToAiQuat(const NiQuaternion& kQuat)
    {
        return aiQuaternion(kQuat.GetW(), kQuat.GetX(), kQuat.GetY(), kQuat.GetZ());
    }

    //----------------------------------------------------------------------------------------------
    aiQuaternion NormalizeQuaternion(aiQuaternion kQuat)
    {
        const double dLengthSq =
            static_cast<double>(kQuat.w) * kQuat.w +
            static_cast<double>(kQuat.x) * kQuat.x +
            static_cast<double>(kQuat.y) * kQuat.y +
            static_cast<double>(kQuat.z) * kQuat.z;

        if (dLengthSq <= 1.0e-20)
            return aiQuaternion(1.0f, 0.0f, 0.0f, 0.0f);

        const float fInvLength = static_cast<float>(1.0 / std::sqrt(dLengthSq));
        kQuat.w *= fInvLength;
        kQuat.x *= fInvLength;
        kQuat.y *= fInvLength;
        kQuat.z *= fInvLength;
        return kQuat;
    }

    //----------------------------------------------------------------------------------------------
    void MakeQuaternionTrackContinuous(std::vector<aiQuatKey>& kKeys)
    {
        if (kKeys.empty())
            return;

        kKeys[0].mValue = NormalizeQuaternion(kKeys[0].mValue);
        for (size_t i = 1; i < kKeys.size(); ++i)
        {
            aiQuaternion kCurrent = NormalizeQuaternion(kKeys[i].mValue);
            const aiQuaternion& kPrevious = kKeys[i - 1].mValue;
            const float fDot =
                kPrevious.w * kCurrent.w +
                kPrevious.x * kCurrent.x +
                kPrevious.y * kCurrent.y +
                kPrevious.z * kCurrent.z;

            // q and -q represent the same rotation. Keep neighboring samples
            // in the same hemisphere so the FBX exporter's quaternion-to-Euler
            // conversion does not create avoidable 360-degree jumps.
            if (fDot < 0.0f)
            {
                kCurrent.w = -kCurrent.w;
                kCurrent.x = -kCurrent.x;
                kCurrent.y = -kCurrent.y;
                kCurrent.z = -kCurrent.z;
            }
            kKeys[i].mValue = kCurrent;
        }
    }

    //----------------------------------------------------------------------------------------------
    void CollectNodesByName(NiAVObject* pkObject, NodeByNameMap& kOut)
    {
        if (!pkObject)
            return;

        kOut.emplace(GetExportNodeName(pkObject), pkObject);

        if (NiIsKindOf(NiNode, pkObject))
        {
            NiNode* pkNode = NiStaticCast(NiNode, pkObject);
            for (unsigned int i = 0; i < pkNode->GetArrayCount(); ++i)
                CollectNodesByName(pkNode->GetAt(i), kOut);
        }
    }

    //----------------------------------------------------------------------------------------------
    struct ControllerManagerEntry
    {
        NiControllerManager* manager = nullptr;
        NiAVObject* target = nullptr;
    };

#ifndef EE_REMOVE_BACK_COMPAT_STREAMING
    struct LegacyMultiTargetEntry
    {
        NiAVObject* rootTarget = nullptr;
        std::vector<NiAVObject*> extraTargets;
        std::unordered_map<std::string, NiAVObject*> uniqueTargetsByName;
        std::unordered_set<std::string> duplicateTargetNames;
    };

    struct LegacyTargetSelection
    {
        const LegacyMultiTargetEntry* controller = nullptr;
        std::vector<NiAVObject*> ordinalTargets;
        bool ambiguousController = false;
    };

    using SequenceOwnerMap =
        std::unordered_map<const NiSequenceData*, NiAVObject*>;
#endif

    //----------------------------------------------------------------------------------------------
    void CollectControllerManagers(NiAVObject* pkObject,
        std::vector<ControllerManagerEntry>& kOut,
        std::unordered_set<NiControllerManager*>& kSeenManagers)
    {
        if (!pkObject)
            return;

        for (NiTimeController* pkController = pkObject->GetControllers();
            pkController; pkController = pkController->GetNext())
        {
            if (!NiIsKindOf(NiControllerManager, pkController))
                continue;

            NiControllerManager* pkManager =
                NiStaticCast(NiControllerManager, pkController);
            if (kSeenManagers.insert(pkManager).second)
                kOut.push_back({pkManager, pkObject});
        }

        if (NiIsKindOf(NiNode, pkObject))
        {
            NiNode* pkNode = NiStaticCast(NiNode, pkObject);
            for (unsigned int i = 0; i < pkNode->GetArrayCount(); ++i)
            {
                CollectControllerManagers(pkNode->GetAt(i), kOut,
                    kSeenManagers);
            }
        }
    }

#ifndef EE_REMOVE_BACK_COMPAT_STREAMING
    //----------------------------------------------------------------------------------------------
    void AddLegacyTargetByName(LegacyMultiTargetEntry& kEntry,
        NiAVObject* pkTarget)
    {
        if (!pkTarget)
            return;

        const std::string kName = GetExportNodeName(pkTarget);
        if (kName.empty() || kEntry.duplicateTargetNames.count(kName) != 0)
            return;

        auto kExisting = kEntry.uniqueTargetsByName.find(kName);
        if (kExisting == kEntry.uniqueTargetsByName.end())
        {
            kEntry.uniqueTargetsByName.emplace(kName, pkTarget);
            return;
        }

        if (kExisting->second != pkTarget)
        {
            kEntry.uniqueTargetsByName.erase(kExisting);
            kEntry.duplicateTargetNames.insert(kName);
        }
    }

    //----------------------------------------------------------------------------------------------
    void CollectSequenceOwners(NiAVObject* pkRoot, SequenceOwnerMap& kOut)
    {
        std::vector<ControllerManagerEntry> kManagers;
        std::unordered_set<NiControllerManager*> kSeenManagers;
        CollectControllerManagers(pkRoot, kManagers, kSeenManagers);

        for (const ControllerManagerEntry& kEntry : kManagers)
        {
            if (!kEntry.manager)
                continue;

            const unsigned int uiSequenceCount =
                kEntry.manager->GetSequenceDataCount();
            for (unsigned int i = 0; i < uiSequenceCount; ++i)
            {
                NiSequenceData* pkSequence =
                    kEntry.manager->GetSequenceDataAt(i);
                if (pkSequence)
                    kOut.emplace(pkSequence, kEntry.target);
            }
        }
    }

    //----------------------------------------------------------------------------------------------
    void CollectLegacyMultiTargetControllers(const NiStream* pkStream,
        std::vector<LegacyMultiTargetEntry>& kOut)
    {
        if (!pkStream)
            return;

        std::unordered_set<NiMultiTargetTransformController*> kSeen;
        for (unsigned int i = 0; i < pkStream->GetObjectCount(); ++i)
        {
            NiMultiTargetTransformController* pkController =
                NiDynamicCast(NiMultiTargetTransformController,
                    pkStream->GetObjectAt(i));
            if (!pkController || !kSeen.insert(pkController).second)
                continue;

            LegacyMultiTargetEntry kEntry;
            kEntry.rootTarget = NiDynamicCast(NiAVObject,
                pkController->GetTarget());
            AddLegacyTargetByName(kEntry, kEntry.rootTarget);

            const unsigned short usExtraTargetCount =
                pkController->GetLegacyExtraTargetCount();
            kEntry.extraTargets.reserve(usExtraTargetCount);
            for (unsigned short us = 0; us < usExtraTargetCount; ++us)
            {
                NiAVObject* pkTarget =
                    pkController->GetLegacyExtraTargetAt(us);
                kEntry.extraTargets.push_back(pkTarget);
                AddLegacyTargetByName(kEntry, pkTarget);
            }

            kOut.push_back(std::move(kEntry));
        }
    }

    //----------------------------------------------------------------------------------------------
    unsigned int CountBakeableTransformEvaluators(
        const NiSequenceData* pkSequence)
    {
        if (!pkSequence)
            return 0;

        unsigned int uiCount = 0;
        for (unsigned int i = 0; i < pkSequence->GetNumEvaluators(); ++i)
        {
            NiEvaluator* pkEvaluator = pkSequence->GetEvaluatorAt(i);
            if (pkEvaluator &&
                (NiIsKindOf(NiTransformEvaluator, pkEvaluator) ||
                 NiIsKindOf(NiConstTransformEvaluator, pkEvaluator) ||
                 NiIsKindOf(NiBSplineTransformEvaluator, pkEvaluator)))
            {
                ++uiCount;
            }
        }
        return uiCount;
    }

    //----------------------------------------------------------------------------------------------
    LegacyTargetSelection SelectLegacyTargetHints(
        const NiSequenceData* pkSequence,
        unsigned int uiTransformEvaluatorCount,
        const SequenceOwnerMap& kSequenceOwners,
        const std::vector<LegacyMultiTargetEntry>& kControllers)
    {
        LegacyTargetSelection kSelection;
        if (!pkSequence || kControllers.empty())
            return kSelection;

        NiAVObject* pkExpectedRoot = nullptr;
        auto kOwner = kSequenceOwners.find(pkSequence);
        if (kOwner != kSequenceOwners.end())
            pkExpectedRoot = kOwner->second;

        for (const LegacyMultiTargetEntry& kEntry : kControllers)
        {
            if (pkExpectedRoot && kEntry.rootTarget != pkExpectedRoot)
                continue;

            if (kSelection.controller)
            {
                // More than one controller can target the same root. Do not
                // guess which target list belongs to this sequence.
                kSelection.controller = nullptr;
                kSelection.ambiguousController = true;
                return kSelection;
            }
            kSelection.controller = &kEntry;
        }

        if (!pkExpectedRoot && kControllers.size() != 1)
        {
            // External KF/KFM data does not identify its owning manager. It is
            // safe to use target hints only when the model has one controller.
            kSelection.controller = nullptr;
            kSelection.ambiguousController = true;
            return kSelection;
        }

        if (!kSelection.controller || uiTransformEvaluatorCount == 0)
            return kSelection;

        const LegacyMultiTargetEntry& kEntry = *kSelection.controller;
        if (uiTransformEvaluatorCount == kEntry.extraTargets.size())
        {
            // Some exporters store the manager/root target separately and put
            // only the remaining controlled nodes in Extra Targets.
            kSelection.ordinalTargets = kEntry.extraTargets;
        }
        else if (kEntry.rootTarget &&
            uiTransformEvaluatorCount == kEntry.extraTargets.size() + 1)
        {
            // Other exporters emit one controlled block for the inherited
            // Target followed by one block for each Extra Target.
            kSelection.ordinalTargets.reserve(uiTransformEvaluatorCount);
            kSelection.ordinalTargets.push_back(kEntry.rootTarget);
            kSelection.ordinalTargets.insert(
                kSelection.ordinalTargets.end(),
                kEntry.extraTargets.begin(), kEntry.extraTargets.end());
        }

        return kSelection;
    }

    //----------------------------------------------------------------------------------------------
    NiAVObject* FindUniqueLegacyTargetByName(
        const LegacyMultiTargetEntry* pkController,
        const std::string& kNodeName)
    {
        if (!pkController || kNodeName.empty())
            return nullptr;

        auto kTarget = pkController->uniqueTargetsByName.find(kNodeName);
        return kTarget != pkController->uniqueTargetsByName.end()
            ? kTarget->second : nullptr;
    }
#endif

    //----------------------------------------------------------------------------------------------
    bool IsLikelyAccumulationRoot(NiAVObject* pkObject,
        const std::string& kNodeName)
    {
        if (!pkObject || kNodeName.empty() || !NiIsKindOf(NiNode, pkObject))
            return false;

        NiNode* pkNode = NiStaticCast(NiNode, pkObject);
        const std::string kExpectedNonAccum = kNodeName + " NonAccum";
        for (unsigned int i = 0; i < pkNode->GetArrayCount(); ++i)
        {
            NiAVObject* pkChild = pkNode->GetAt(i);
            if (!pkChild || !pkChild->GetName().c_str())
                continue;

            const std::string kChildName(pkChild->GetName().c_str());
            if (kChildName == kExpectedNonAccum ||
                kChildName.find("NonAccum") != std::string::npos)
            {
                return true;
            }
        }

        return false;
    }

    //----------------------------------------------------------------------------------------------
    float SanitizeUnitScale(float fUnitScale)
    {
        return std::isfinite(fUnitScale) && fUnitScale > 0.0f
            ? fUnitScale : 1.0f;
    }

    //----------------------------------------------------------------------------------------------
    float SanitizeSampleRate(float fSampleRate)
    {
        return std::isfinite(fSampleRate) && fSampleRate > 0.0f
            ? fSampleRate : DEFAULT_SAMPLE_RATE;
    }

    //----------------------------------------------------------------------------------------------
    SampleGrid BuildSequenceSampleGrid(float fDuration, float fFrequency,
        float fSampleRate)
    {
        SampleGrid kGrid;
        kGrid.sampleRate = SanitizeSampleRate(fSampleRate);

        const float fSafeFrequency =
            std::isfinite(fFrequency) && std::abs(fFrequency) > MIN_POSITIVE
            ? std::abs(fFrequency) : 1.0f;
        const float fSafeDuration =
            std::isfinite(fDuration) && fDuration > 0.0f ? fDuration : 0.0f;

        kGrid.durationSeconds = fSafeDuration / fSafeFrequency;
        if (kGrid.durationSeconds <= 0.0f)
            return kGrid;

        double dRequestedSamples =
            std::ceil(static_cast<double>(kGrid.durationSeconds) * kGrid.sampleRate) + 1.0;
        unsigned int uiSampleCount = static_cast<unsigned int>(
            std::clamp(dRequestedSamples, 2.0,
                static_cast<double>(MAX_BAKED_SAMPLES)));

        if (dRequestedSamples > MAX_BAKED_SAMPLES)
        {
            std::cerr << "  Warning: animation requested "
                << static_cast<unsigned long long>(dRequestedSamples)
                << " samples; capped to " << MAX_BAKED_SAMPLES << "." << std::endl;
        }

        kGrid.localTimes.resize(uiSampleCount);
        kGrid.ticks.resize(uiSampleCount);
        for (unsigned int i = 0; i < uiSampleCount; ++i)
        {
            const float fPlaybackSeconds = (i + 1u == uiSampleCount)
                ? kGrid.durationSeconds
                : std::min(static_cast<float>(i) / kGrid.sampleRate,
                    kGrid.durationSeconds);

            kGrid.localTimes[i] = std::min(
                fPlaybackSeconds * fSafeFrequency, fSafeDuration);
            kGrid.ticks[i] = static_cast<double>(fPlaybackSeconds) * kGrid.sampleRate;
        }

        return kGrid;
    }

    //----------------------------------------------------------------------------------------------
    void GetRestTransform(NiAVObject* pkObject, float fUnitScale,
        bool bConvertToUnrealAxes, aiVector3D& kPosition,
        aiQuaternion& kRotation, aiVector3D& kScale)
    {
        if (!pkObject)
        {
            kPosition = aiVector3D(0.0f, 0.0f, 0.0f);
            kRotation = aiQuaternion(1.0f, 0.0f, 0.0f, 0.0f);
            kScale = aiVector3D(1.0f, 1.0f, 1.0f);
            return;
        }

        const NiPoint3& kTranslate = pkObject->GetTranslate();
        NiQuaternion kRotate(1.0f, 0.0f, 0.0f, 0.0f);
        pkObject->GetRotate(kRotate);
        const float fScale = pkObject->GetScale();

        const aiVector3D kSourcePosition(
            kTranslate.x * fUnitScale,
            kTranslate.y * fUnitScale,
            kTranslate.z * fUnitScale);
        kPosition = AxisConversion::ToUnrealVector(kSourcePosition,
            bConvertToUnrealAxes);
        kRotation = NormalizeQuaternion(AxisConversion::ToUnrealQuaternion(
            ToAiQuat(kRotate), bConvertToUnrealAxes));
        kScale = aiVector3D(fScale, fScale, fScale);
    }

    //----------------------------------------------------------------------------------------------
    BakedNodeTrack& GetOrCreateTrack(
        const std::string& kNodeName,
        NiAVObject* pkRestObject,
        const SampleGrid& kGrid,
        float fUnitScale,
        bool bConvertToUnrealAxes,
        std::vector<BakedNodeTrack>& kTracks,
        std::unordered_map<std::string, size_t>& kTrackByName)
    {
        auto kFound = kTrackByName.find(kNodeName);
        if (kFound != kTrackByName.end())
        {
            BakedNodeTrack& kTrack = kTracks[kFound->second];
            if (!kTrack.restObject && pkRestObject)
                kTrack.restObject = pkRestObject;
            return kTrack;
        }

        aiVector3D kRestPosition;
        aiQuaternion kRestRotation;
        aiVector3D kRestScale;
        GetRestTransform(pkRestObject, fUnitScale,
            bConvertToUnrealAxes, kRestPosition, kRestRotation, kRestScale);

        BakedNodeTrack kTrack;
        kTrack.name = kNodeName;
        kTrack.restObject = pkRestObject;
        kTrack.positions.resize(kGrid.ticks.size());
        kTrack.rotations.resize(kGrid.ticks.size());
        kTrack.scales.resize(kGrid.ticks.size());

        for (size_t i = 0; i < kGrid.ticks.size(); ++i)
        {
            kTrack.positions[i] = aiVectorKey(kGrid.ticks[i], kRestPosition);
            kTrack.rotations[i] = aiQuatKey(kGrid.ticks[i], kRestRotation);
            kTrack.scales[i] = aiVectorKey(kGrid.ticks[i], kRestScale);
        }

        const size_t stIndex = kTracks.size();
        kTracks.push_back(std::move(kTrack));
        kTrackByName.emplace(kNodeName, stIndex);
        return kTracks.back();
    }

    //----------------------------------------------------------------------------------------------
    NiEvaluatorSPData* FindScratchPadChannel(NiScratchPad& kScratch,
        unsigned int uiChannel)
    {
        NiEvaluatorSPData* pkEntries = static_cast<NiEvaluatorSPData*>(
            kScratch.GetDataBlock(SPBEVALUATORSPDATA));
        const unsigned int uiEntryCount =
            kScratch.GetNumBlockItems(SPBEVALUATORSPDATA);

        for (unsigned int i = 0; i < uiEntryCount; ++i)
        {
            if (static_cast<unsigned int>(pkEntries[i].GetEvalChannelIndex()) == uiChannel)
                return &pkEntries[i];
        }
        return nullptr;
    }

    //----------------------------------------------------------------------------------------------
    bool IsBakeableTransformEvaluator(NiEvaluator* pkEvaluator)
    {
        return pkEvaluator &&
            (NiIsKindOf(NiTransformEvaluator, pkEvaluator) ||
             NiIsKindOf(NiConstTransformEvaluator, pkEvaluator) ||
             NiIsKindOf(NiBSplineTransformEvaluator, pkEvaluator));
    }

    //----------------------------------------------------------------------------------------------
    bool BakeEvaluatorIntoTrack(NiEvaluator* pkEvaluator,
        const SampleGrid& kGrid, float fUnitScale,
        bool bConvertToUnrealAxes, BakedNodeTrack& kTrack)
    {
        if (!IsBakeableTransformEvaluator(pkEvaluator) || kGrid.ticks.empty())
            return false;

        // A transform evaluator can expose a channel in two different ways:
        //
        //  1. Animated channel: evaluated through NiScratchPad/UpdateChannel.
        //  2. Posed channel: a constant value stored directly in the evaluator.
        //
        // Posed channels commonly do not allocate scratch-pad data. Falling
        // back to the model NIF rest transform for such a channel is incorrect:
        // the pose embedded in the KF/KFM sequence is authoritative and can be
        // different for each sequence. Mixing animated components with model
        // rest components causes bones to detach or stretch in affected clips.
        NiScratchPad kScratch(pkEvaluator);

        NiEvaluatorSPData* pkPosSP = nullptr;
        NiEvaluatorSPData* pkRotSP = nullptr;
        NiEvaluatorSPData* pkScaleSP = nullptr;

        if (!pkEvaluator->IsEvalChannelInvalid(NiEvaluator::EVALPOSINDEX))
            pkPosSP = FindScratchPadChannel(kScratch, NiEvaluator::EVALPOSINDEX);
        if (!pkEvaluator->IsEvalChannelInvalid(NiEvaluator::EVALROTINDEX))
            pkRotSP = FindScratchPadChannel(kScratch, NiEvaluator::EVALROTINDEX);
        if (!pkEvaluator->IsEvalChannelInvalid(NiEvaluator::EVALSCALEINDEX))
            pkScaleSP = FindScratchPadChannel(kScratch, NiEvaluator::EVALSCALEINDEX);

        NiPoint3 kPosedPosition;
        NiQuaternion kPosedRotation;
        float fPosedScale = 1.0f;

        const bool bPosPosed =
            !pkEvaluator->IsEvalChannelInvalid(NiEvaluator::EVALPOSINDEX) &&
            pkEvaluator->GetChannelPosedValue(
                NiEvaluator::EVALPOSINDEX, &kPosedPosition);
        const bool bRotPosed =
            !pkEvaluator->IsEvalChannelInvalid(NiEvaluator::EVALROTINDEX) &&
            pkEvaluator->GetChannelPosedValue(
                NiEvaluator::EVALROTINDEX, &kPosedRotation);
        const bool bScalePosed =
            !pkEvaluator->IsEvalChannelInvalid(NiEvaluator::EVALSCALEINDEX) &&
            pkEvaluator->GetChannelPosedValue(
                NiEvaluator::EVALSCALEINDEX, &fPosedScale);

        bool bAnySuccess = false;
        for (size_t i = 0; i < kGrid.localTimes.size(); ++i)
        {
            const float fLocalTime = kGrid.localTimes[i];

            if (bPosPosed)
            {
                const aiVector3D kSourcePosition(
                    kPosedPosition.x * fUnitScale,
                    kPosedPosition.y * fUnitScale,
                    kPosedPosition.z * fUnitScale);
                kTrack.positions[i].mValue = AxisConversion::ToUnrealVector(
                    kSourcePosition, bConvertToUnrealAxes);
                kTrack.hasPositionSource = true;
                bAnySuccess = true;
            }
            else if (pkPosSP)
            {
                NiPoint3 kPosition;
                if (pkEvaluator->UpdateChannel(fLocalTime,
                    NiEvaluator::EVALPOSINDEX, pkPosSP, &kPosition))
                {
                    const aiVector3D kSourcePosition(
                        kPosition.x * fUnitScale,
                        kPosition.y * fUnitScale,
                        kPosition.z * fUnitScale);
                    kTrack.positions[i].mValue = AxisConversion::ToUnrealVector(
                        kSourcePosition, bConvertToUnrealAxes);
                    kTrack.hasPositionSource = true;
                    bAnySuccess = true;
                }
            }

            if (bRotPosed)
            {
                kTrack.rotations[i].mValue = NormalizeQuaternion(
                    AxisConversion::ToUnrealQuaternion(ToAiQuat(kPosedRotation),
                        bConvertToUnrealAxes));
                kTrack.hasRotationSource = true;
                bAnySuccess = true;
            }
            else if (pkRotSP)
            {
                NiQuaternion kRotation;
                if (pkEvaluator->UpdateChannel(fLocalTime,
                    NiEvaluator::EVALROTINDEX, pkRotSP, &kRotation))
                {
                    kTrack.rotations[i].mValue = NormalizeQuaternion(
                        AxisConversion::ToUnrealQuaternion(ToAiQuat(kRotation),
                            bConvertToUnrealAxes));
                    kTrack.hasRotationSource = true;
                    bAnySuccess = true;
                }
            }

            if (bScalePosed)
            {
                kTrack.scales[i].mValue = aiVector3D(
                    fPosedScale, fPosedScale, fPosedScale);
                kTrack.hasScaleSource = true;
                bAnySuccess = true;
            }
            else if (pkScaleSP)
            {
                float fScale = 1.0f;
                if (pkEvaluator->UpdateChannel(fLocalTime,
                    NiEvaluator::EVALSCALEINDEX, pkScaleSP, &fScale))
                {
                    kTrack.scales[i].mValue = aiVector3D(fScale, fScale, fScale);
                    kTrack.hasScaleSource = true;
                    bAnySuccess = true;
                }
            }
        }

        kTrack.hasAnimationData |= bAnySuccess;
        return bAnySuccess;
    }

    //----------------------------------------------------------------------------------------------
    aiNodeAnim* BuildAiNodeAnim(BakedNodeTrack& kTrack)
    {
        if (!kTrack.hasAnimationData || kTrack.positions.empty())
            return nullptr;

        MakeQuaternionTrackContinuous(kTrack.rotations);

        aiNodeAnim* pkChannel = new aiNodeAnim();
        pkChannel->mNodeName = kTrack.name.c_str();

        pkChannel->mNumPositionKeys = static_cast<unsigned int>(kTrack.positions.size());
        pkChannel->mPositionKeys = new aiVectorKey[pkChannel->mNumPositionKeys];
        std::copy(kTrack.positions.begin(), kTrack.positions.end(),
            pkChannel->mPositionKeys);

        pkChannel->mNumRotationKeys = static_cast<unsigned int>(kTrack.rotations.size());
        pkChannel->mRotationKeys = new aiQuatKey[pkChannel->mNumRotationKeys];
        std::copy(kTrack.rotations.begin(), kTrack.rotations.end(),
            pkChannel->mRotationKeys);

        pkChannel->mNumScalingKeys = static_cast<unsigned int>(kTrack.scales.size());
        pkChannel->mScalingKeys = new aiVectorKey[pkChannel->mNumScalingKeys];
        std::copy(kTrack.scales.begin(), kTrack.scales.end(),
            pkChannel->mScalingKeys);

        return pkChannel;
    }

    //----------------------------------------------------------------------------------------------
    aiAnimation* BuildAiAnimation(const std::string& kName,
        const SampleGrid& kGrid, std::vector<BakedNodeTrack>& kTracks)
    {
        std::vector<aiNodeAnim*> kChannels;
        kChannels.reserve(kTracks.size());
        for (BakedNodeTrack& kTrack : kTracks)
        {
            aiNodeAnim* pkChannel = BuildAiNodeAnim(kTrack);
            if (pkChannel)
                kChannels.push_back(pkChannel);
        }

        if (kChannels.empty())
            return nullptr;

        aiAnimation* pkAnimation = new aiAnimation();
        pkAnimation->mName = kName.c_str();
        pkAnimation->mTicksPerSecond = kGrid.sampleRate;
        pkAnimation->mDuration =
            static_cast<double>(kGrid.durationSeconds) * kGrid.sampleRate;
        pkAnimation->mNumChannels = static_cast<unsigned int>(kChannels.size());
        pkAnimation->mChannels = new aiNodeAnim*[kChannels.size()];
        for (size_t i = 0; i < kChannels.size(); ++i)
            pkAnimation->mChannels[i] = kChannels[i];

        pkAnimation->mNumMeshChannels = 0;
        pkAnimation->mMeshChannels = nullptr;
        pkAnimation->mNumMorphMeshChannels = 0;
        pkAnimation->mMorphMeshChannels = nullptr;
        return pkAnimation;
    }

    //----------------------------------------------------------------------------------------------
    struct NifControllerEntry
    {
        std::string name;
        NiTransformController* controller = nullptr;
        NiAVObject* object = nullptr;
        NiInterpolator* interpolator = nullptr;
        float beginTime = 0.0f;
        float endTime = 0.0f;
        float frequency = 1.0f;
        float phase = 0.0f;
        float durationSeconds = 0.0f;
        NiTimeController::CycleType cycleType = NiTimeController::CLAMP;
        bool playBackwards = false;
        bool active = true;
    };

    //----------------------------------------------------------------------------------------------
    const char* GetCycleTypeName(NiTimeController::CycleType eCycleType)
    {
        switch (eCycleType)
        {
        case NiTimeController::LOOP:
            return "LOOP";
        case NiTimeController::REVERSE:
            return "REVERSE";
        case NiTimeController::CLAMP:
            return "CLAMP";
        default:
            return "UNKNOWN";
        }
    }

    //----------------------------------------------------------------------------------------------
    float ComputeControllerKeyTime(const NifControllerEntry& kEntry,
        float fPlaybackSeconds)
    {
        const float fLo = kEntry.beginTime;
        const float fHi = kEntry.endTime;
        const float fSpan = fHi - fLo;
        if (!std::isfinite(fSpan) || fSpan <= 0.0f)
            return fLo;

        float fScaledTime =
            fPlaybackSeconds * kEntry.frequency + kEntry.phase;

        switch (kEntry.cycleType)
        {
        case NiTimeController::LOOP:
        {
            float fRelative = std::fmod(fScaledTime - fLo, fSpan);
            if (fRelative < 0.0f)
                fRelative += fSpan;
            fScaledTime = fLo + fRelative;
            break;
        }
        case NiTimeController::REVERSE:
        {
            // Match NiTimeController::ComputeScaledTime exactly: unlike LOOP,
            // REVERSE folds the unshifted scaled time over a double span and
            // adds the low key time after the fold.
            const float fDoubleSpan = 2.0f * fSpan;
            float fRelative = std::fmod(fScaledTime, fDoubleSpan);
            if (fRelative < 0.0f)
                fRelative += fDoubleSpan;
            if (fRelative > fSpan)
                fRelative = fDoubleSpan - fRelative;
            fScaledTime = fLo + fRelative;
            break;
        }
        case NiTimeController::CLAMP:
        default:
            fScaledTime = std::clamp(fScaledTime, fLo, fHi);
            break;
        }

        fScaledTime = std::clamp(fScaledTime, fLo, fHi);
        if (kEntry.playBackwards)
            fScaledTime = fHi - (fScaledTime - fLo);

        return std::clamp(fScaledTime, fLo, fHi);
    }

    //----------------------------------------------------------------------------------------------
    float GetPlaybackSeconds(const SampleGrid& kGrid, size_t stSampleIndex)
    {
        if (stSampleIndex >= kGrid.ticks.size() ||
            !std::isfinite(kGrid.sampleRate) ||
            kGrid.sampleRate <= MIN_POSITIVE)
        {
            return 0.0f;
        }

        return static_cast<float>(
            kGrid.ticks[stSampleIndex] / kGrid.sampleRate);
    }

    //----------------------------------------------------------------------------------------------
    void CollectTransformControllers(NiAVObject* pkObject,
        std::vector<NifControllerEntry>& kOut)
    {
        if (!pkObject)
            return;

        NiTimeController* pkController = pkObject->GetControllers();
        while (pkController)
        {
            if (NiIsKindOf(NiTransformController, pkController))
            {
                NiTransformController* pkTransformController =
                    NiStaticCast(NiTransformController, pkController);
                NiInterpolator* pkInterpolator =
                    pkTransformController->GetInterpolator(0);

                // NiTransformController can use NiTransformInterpolator,
                // NiBSplineTransformInterpolator, compressed B-spline
                // interpolators, or another transform-valued interpolator.
                // Restricting this to NiTransformInterpolator silently omitted
                // animated effect nodes in models such as M329.
                if (pkInterpolator && pkInterpolator->IsTransformValueSupported())
                {
                    float fBegin = pkTransformController->GetBeginKeyTime();
                    float fEnd = pkTransformController->GetEndKeyTime();
                    if (!std::isfinite(fBegin) || !std::isfinite(fEnd) ||
                        fEnd <= fBegin)
                    {
                        pkInterpolator->GetActiveTimeRange(fBegin, fEnd);
                    }

                    float fFrequency = pkTransformController->GetFrequency();
                    if (!std::isfinite(fFrequency))
                        fFrequency = 1.0f;

                    float fPhase = pkTransformController->GetPhase();
                    if (!std::isfinite(fPhase))
                        fPhase = 0.0f;

                    if (std::isfinite(fBegin) && std::isfinite(fEnd) &&
                        fEnd > fBegin)
                    {
                        NifControllerEntry kEntry;
                        kEntry.name = GetExportNodeName(pkObject);
                        kEntry.controller = pkTransformController;
                        kEntry.object = pkObject;
                        kEntry.interpolator = pkInterpolator;
                        kEntry.beginTime = fBegin;
                        kEntry.endTime = fEnd;
                        kEntry.frequency = fFrequency;
                        kEntry.phase = fPhase;
                        const float fAbsFrequency = std::abs(fFrequency);
                        kEntry.durationSeconds =
                            fAbsFrequency > MIN_POSITIVE
                            ? (fEnd - fBegin) / fAbsFrequency
                            : 0.0f;
                        kEntry.cycleType = pkTransformController->GetCycleType();
                        kEntry.playBackwards =
                            pkTransformController->GetPlayBackwards();
                        kEntry.active = pkTransformController->GetActive();
                        kOut.push_back(kEntry);
                    }
                    else
                    {
                        std::cerr << "  Warning: NiTransformController on node '"
                            << GetExportNodeName(pkObject)
                            << "' has no usable active time range; interpolator="
                            << pkInterpolator->GetRTTI()->GetName() << "."
                            << std::endl;
                    }
                }
                else
                {
                    std::cerr << "  Warning: NiTransformController on node '"
                        << GetExportNodeName(pkObject)
                        << "' has no transform-valued interpolator."
                        << std::endl;
                }
            }
            pkController = pkController->GetNext();
        }

        if (NiIsKindOf(NiNode, pkObject))
        {
            NiNode* pkNode = NiStaticCast(NiNode, pkObject);
            for (unsigned int i = 0; i < pkNode->GetArrayCount(); ++i)
                CollectTransformControllers(pkNode->GetAt(i), kOut);
        }
    }

    //----------------------------------------------------------------------------------------------
    ControllerBakeStats BakeNifControllersIntoTracks(
        const std::vector<NifControllerEntry>& kControllers,
        const SampleGrid& kGrid,
        float fUnitScale,
        bool bConvertToUnrealAxes,
        const std::unordered_map<std::string, TransformComponentMask>&
            kProtectedComponents,
        std::vector<BakedNodeTrack>& kTracks,
        std::unordered_map<std::string, size_t>& kTrackByName)
    {
        ControllerBakeStats kStats;

        for (const NifControllerEntry& kEntry : kControllers)
        {
            if (!kEntry.object || !kEntry.interpolator)
                continue;

            // Preserve the same accumulation-root rule used for KF/KFM
            // evaluators. Effect children such as Bip01 NonAccum are not
            // filtered; only the actual accumulation node is skipped.
            if (IsLikelyAccumulationRoot(kEntry.object, kEntry.name))
            {
                ++kStats.skippedAccumulationRoots;
                continue;
            }

            TransformComponentMask kProtected;
            auto kProtectedIt = kProtectedComponents.find(kEntry.name);
            if (kProtectedIt != kProtectedComponents.end())
                kProtected = kProtectedIt->second;

            BakedNodeTrack& kTrack = GetOrCreateTrack(kEntry.name,
                kEntry.object, kGrid, fUnitScale,
                bConvertToUnrealAxes, kTracks, kTrackByName);

            bool bWroteController = false;
            bool bSawPosition = false;
            bool bSawRotation = false;
            bool bSawScale = false;

            // Interpolators cache their last sample time. Force the first
            // sample when the same NIF controller is baked into several KF
            // clips, each of which starts again at time zero.
            kEntry.interpolator->ForceNextUpdate();

            for (size_t i = 0; i < kGrid.ticks.size(); ++i)
            {
                const float fPlaybackSeconds = GetPlaybackSeconds(kGrid, i);
                const float fControllerTime =
                    ComputeControllerKeyTime(kEntry, fPlaybackSeconds);

                NiQuatTransform kValue;
                if (!kEntry.interpolator->Update(
                    fControllerTime, kEntry.object, kValue))
                {
                    ++kStats.controllerSamplesFailed;
                    continue;
                }

                if (kValue.IsTranslateValid())
                {
                    bSawPosition = true;
                    if (!kProtected.position)
                    {
                        const NiPoint3& kPosition = kValue.GetTranslate();
                        const aiVector3D kSourcePosition(
                            kPosition.x * fUnitScale,
                            kPosition.y * fUnitScale,
                            kPosition.z * fUnitScale);
                        kTrack.positions[i].mValue =
                            AxisConversion::ToUnrealVector(
                                kSourcePosition, bConvertToUnrealAxes);
                        kTrack.hasPositionSource = true;
                        kTrack.hasAnimationData = true;
                        bWroteController = true;
                    }
                }

                if (kValue.IsRotateValid())
                {
                    bSawRotation = true;
                    if (!kProtected.rotation)
                    {
                        kTrack.rotations[i].mValue = NormalizeQuaternion(
                            AxisConversion::ToUnrealQuaternion(
                                ToAiQuat(kValue.GetRotate()),
                                bConvertToUnrealAxes));
                        kTrack.hasRotationSource = true;
                        kTrack.hasAnimationData = true;
                        bWroteController = true;
                    }
                }

                if (kValue.IsScaleValid())
                {
                    bSawScale = true;
                    if (!kProtected.scale)
                    {
                        const float fScale = kValue.GetScale();
                        kTrack.scales[i].mValue = aiVector3D(
                            fScale, fScale, fScale);
                        kTrack.hasScaleSource = true;
                        kTrack.hasAnimationData = true;
                        bWroteController = true;
                    }
                }
            }

            if (bSawPosition && kProtected.position)
                ++kStats.protectedPositionComponents;
            if (bSawRotation && kProtected.rotation)
                ++kStats.protectedRotationComponents;
            if (bSawScale && kProtected.scale)
                ++kStats.protectedScaleComponents;
            if (bWroteController)
                ++kStats.controllersBaked;
        }

        return kStats;
    }

}

//--------------------------------------------------------------------------------------------------
unsigned int AnimationExporter::AppendFromControllerManagers(
    NiAVObject* pkRoot,
    std::vector<NiSequenceDataPtr>& kSequenceDatas)
{
    if (!pkRoot)
        return 0;

    // External KFM/KF sequences are already present in kSequenceDatas and
    // intentionally win name collisions. This matters for older assets that
    // embed a mostly duplicate sequence set but also contain one or more
    // model-specific clips (for example, a weapon/effect animation).
    std::unordered_set<const NiSequenceData*> kKnownPointers;
    std::unordered_set<std::string> kKnownNames;
    for (NiSequenceData* pkSequence : kSequenceDatas)
    {
        if (!pkSequence)
            continue;

        kKnownPointers.insert(pkSequence);
        const char* pcName = pkSequence->GetName().c_str();
        if (pcName && pcName[0] != '\0')
            kKnownNames.emplace(pcName);
    }

    std::vector<ControllerManagerEntry> kManagers;
    std::unordered_set<NiControllerManager*> kSeenManagers;
    CollectControllerManagers(pkRoot, kManagers, kSeenManagers);

    unsigned int uiAdded = 0;
    unsigned int uiDuplicateNames = 0;
    unsigned int uiDuplicatePointers = 0;

    for (const ControllerManagerEntry& kEntry : kManagers)
    {
        NiControllerManager* pkManager = kEntry.manager;
        if (!pkManager)
            continue;

        const std::string kTargetName = kEntry.target
            ? GetExportNodeName(kEntry.target) : std::string("<unknown>");
        const unsigned int uiSequenceCount =
            pkManager->GetSequenceDataCount();

        std::cout << "  Found NiControllerManager on node '"
            << kTargetName << "' with " << uiSequenceCount
            << " embedded sequence(s)." << std::endl;

        for (unsigned int i = 0; i < uiSequenceCount; ++i)
        {
            NiSequenceData* pkSequence = pkManager->GetSequenceDataAt(i);
            if (!pkSequence)
                continue;

            if (!kKnownPointers.insert(pkSequence).second)
            {
                ++uiDuplicatePointers;
                continue;
            }

            const char* pcName = pkSequence->GetName().c_str();
            const std::string kName =
                (pcName && pcName[0] != '\0') ? pcName : std::string();

            if (!kName.empty() && !kKnownNames.insert(kName).second)
            {
                // Keep the external/first copy. Legacy embedded sequences
                // commonly duplicate the normal KFM set.
                ++uiDuplicateNames;
                continue;
            }

            kSequenceDatas.push_back(pkSequence);
            ++uiAdded;

            std::cout << "    Added embedded sequence '"
                << (kName.empty() ? "<unnamed>" : kName) << "' ("
                << pkSequence->GetNumEvaluators() << " evaluator(s))."
                << std::endl;
        }
    }

    if (!kManagers.empty())
    {
        std::cout << "  Embedded controller-manager sequences added: "
            << uiAdded;
        if (uiDuplicateNames > 0 || uiDuplicatePointers > 0)
        {
            std::cout << " (skipped " << uiDuplicateNames
                << " duplicate name(s) and " << uiDuplicatePointers
                << " duplicate object reference(s))";
        }
        std::cout << "." << std::endl;
    }

    return uiAdded;
}

//--------------------------------------------------------------------------------------------------
std::vector<aiAnimation*> AnimationExporter::BuildFromSequenceDatas(
    const std::vector<NiSequenceDataPtr>& kSequenceDatas,
    NiAVObject* pkNifRoot,
    float fUnitScale,
    float fSampleRate,
    bool bConvertToUnrealAxes,
    const NiStream* pkNifStream)
{
    std::vector<aiAnimation*> kResult;
    const float fSafeUnitScale = SanitizeUnitScale(fUnitScale);
    const float fSafeSampleRate = SanitizeSampleRate(fSampleRate);

    NodeByNameMap kNodesByName;
    CollectNodesByName(pkNifRoot, kNodesByName);

#ifndef EE_REMOVE_BACK_COMPAT_STREAMING
    SequenceOwnerMap kSequenceOwners;
    CollectSequenceOwners(pkNifRoot, kSequenceOwners);

    std::vector<LegacyMultiTargetEntry> kLegacyMultiTargets;
    CollectLegacyMultiTargetControllers(pkNifStream, kLegacyMultiTargets);
    if (!kLegacyMultiTargets.empty())
    {
        std::cout << "  Found " << kLegacyMultiTargets.size()
            << " legacy NiMultiTargetTransformController(s)." << std::endl;
        for (const LegacyMultiTargetEntry& kEntry : kLegacyMultiTargets)
        {
            unsigned int uiResolvedExtraTargets = 0;
            for (NiAVObject* pkTarget : kEntry.extraTargets)
                uiResolvedExtraTargets += pkTarget ? 1u : 0u;

            std::cout << "    Multi-target root='"
                << (kEntry.rootTarget
                    ? GetExportNodeName(kEntry.rootTarget)
                    : std::string("<null>"))
                << "' extraTargets=" << kEntry.extraTargets.size()
                << " (resolved=" << uiResolvedExtraTargets << ", uniqueNames="
                << kEntry.uniqueTargetsByName.size() << ")." << std::endl;
        }
    }
#endif

    // NIF-level NiTransformControllers continue to run while a KFM/KF
    // sequence drives the skeleton in Gamebryo. Collect them once and bake
    // them into every exported sequence so animated effects and auxiliary
    // meshes are not lost merely because skeletal animations are present.
    std::vector<NifControllerEntry> kNifControllers;
    CollectTransformControllers(pkNifRoot, kNifControllers);
    if (!kNifControllers.empty())
    {
        std::cout << "  Found " << kNifControllers.size()
            << " NIF NiTransformController(s); merging them into each "
            << "KF/KFM animation." << std::endl;

        for (const NifControllerEntry& kEntry : kNifControllers)
        {
            std::cout << "    Controller node='" << kEntry.name
                << "' interpolator="
                << kEntry.interpolator->GetRTTI()->GetName()
                << " range=[" << kEntry.beginTime << ", "
                << kEntry.endTime << "] frequency=" << kEntry.frequency
                << " phase=" << kEntry.phase
                << " cycle=" << GetCycleTypeName(kEntry.cycleType)
                << " backwards=" << (kEntry.playBackwards ? "yes" : "no")
                << " active=" << (kEntry.active ? "yes" : "no")
                << std::endl;
        }
    }

    for (size_t s = 0; s < kSequenceDatas.size(); ++s)
    {
        NiSequenceData* pkSequence = kSequenceDatas[s];
        if (!pkSequence || pkSequence->GetNumEvaluators() == 0)
            continue;

        const SampleGrid kGrid = BuildSequenceSampleGrid(
            pkSequence->GetDuration(), pkSequence->GetFrequency(),
            fSafeSampleRate);
        if (kGrid.ticks.empty())
            continue;

        std::vector<BakedNodeTrack> kTracks;
        std::unordered_map<std::string, size_t> kTrackByName;
        unsigned int uiUnsupportedEvaluators = 0;
        unsigned int uiMissingRestNodes = 0;
        unsigned int uiDuplicateComponentSources = 0;
        unsigned int uiSkippedAccumulationRoots = 0;

#ifndef EE_REMOVE_BACK_COMPAT_STREAMING
        const unsigned int uiTransformEvaluatorCount =
            CountBakeableTransformEvaluators(pkSequence);
        const LegacyTargetSelection kLegacySelection =
            SelectLegacyTargetHints(pkSequence, uiTransformEvaluatorCount,
                kSequenceOwners, kLegacyMultiTargets);
        unsigned int uiLegacyTransformOrdinal = 0;
        unsigned int uiLegacyUniqueNameMatches = 0;
        unsigned int uiLegacyOrdinalNameMatches = 0;
        unsigned int uiLegacyUnnamedFallbacks = 0;
        unsigned int uiLegacyNameConflicts = 0;
        unsigned int uiLegacyNullOrdinalTargets = 0;
#endif
        unsigned int uiUnnamedTransformEvaluators = 0;

        for (unsigned int e = 0; e < pkSequence->GetNumEvaluators(); ++e)
        {
            NiEvaluator* pkEvaluator = pkSequence->GetEvaluatorAt(e);
            if (!pkEvaluator)
                continue;

            if (!IsBakeableTransformEvaluator(pkEvaluator))
            {
                // Non-transform evaluators can drive material, visibility,
                // morph, look-at, or path-controller data. Do not reinterpret
                // those channels as skeletal transforms.
                ++uiUnsupportedEvaluators;
                continue;
            }

            const char* pcNodeName = pkEvaluator->GetAVObjectName();
            std::string kNodeName =
                (pcNodeName && pcNodeName[0] != '\0')
                ? std::string(pcNodeName) : std::string();
            NiAVObject* pkRestObject = nullptr;

#ifndef EE_REMOVE_BACK_COMPAT_STREAMING
            NiAVObject* pkOrdinalTarget = nullptr;
            if (uiLegacyTransformOrdinal <
                kLegacySelection.ordinalTargets.size())
            {
                pkOrdinalTarget = kLegacySelection.ordinalTargets[
                    uiLegacyTransformOrdinal];
            }
            ++uiLegacyTransformOrdinal;

            if (!kNodeName.empty())
            {
                // The sequence ID tag remains authoritative. Use the legacy
                // controller's direct pointer when its target name is unique,
                // which also avoids choosing the wrong node when the scene has
                // duplicate names.
                pkRestObject = FindUniqueLegacyTargetByName(
                    kLegacySelection.controller, kNodeName);
                if (pkRestObject)
                {
                    ++uiLegacyUniqueNameMatches;
                }
                else if (pkOrdinalTarget)
                {
                    const std::string kOrdinalName =
                        GetExportNodeName(pkOrdinalTarget);
                    if (kOrdinalName == kNodeName)
                    {
                        pkRestObject = pkOrdinalTarget;
                        ++uiLegacyOrdinalNameMatches;
                    }
                    else
                    {
                        // A valid sequence name must not be replaced merely
                        // because a target array uses a different ordering.
                        ++uiLegacyNameConflicts;
                    }
                }
            }
            else if (pkOrdinalTarget)
            {
                // Only use ordinal recovery when the number of transform
                // evaluators exactly matches either Extra Targets or
                // Target+Extra Targets. This avoids silently assigning an
                // unnamed channel to the wrong bone.
                kNodeName = GetExportNodeName(pkOrdinalTarget);
                pkRestObject = pkOrdinalTarget;
                ++uiLegacyUnnamedFallbacks;
            }
            else if (!kLegacySelection.ordinalTargets.empty())
            {
                ++uiLegacyNullOrdinalTargets;
            }
#endif

            if (kNodeName.empty())
            {
                ++uiUnnamedTransformEvaluators;
                continue;
            }

            if (!pkRestObject)
            {
                auto kRestNode = kNodesByName.find(kNodeName);
                if (kRestNode != kNodesByName.end())
                    pkRestObject = kRestNode->second;
                else
                    ++uiMissingRestNodes;
            }

            // KFM/Gamebryo often uses a node such as "Bip01" as the
            // accumulation/root-motion node and keeps the actual skeleton under
            // "Bip01 NonAccum". Exporting the accumulation node as a normal
            // bone animation overwrites the model bind transform with identity
            // or root-motion values, which makes skinned limbs stretch in FBX.
            // Keep the accumulation node in its NIF bind transform and export
            // the NonAccum/skeletal children instead.
            if (IsLikelyAccumulationRoot(pkRestObject, kNodeName))
            {
                ++uiSkippedAccumulationRoots;
                continue;
            }

            BakedNodeTrack& kTrack = GetOrCreateTrack(kNodeName,
                pkRestObject, kGrid, fSafeUnitScale,
                bConvertToUnrealAxes, kTracks, kTrackByName);

            const bool bHadPosition = kTrack.hasPositionSource;
            const bool bHadRotation = kTrack.hasRotationSource;
            const bool bHadScale = kTrack.hasScaleSource;

            BakeEvaluatorIntoTrack(pkEvaluator, kGrid,
                fSafeUnitScale, bConvertToUnrealAxes, kTrack);

            if ((bHadPosition && kTrack.hasPositionSource &&
                    !pkEvaluator->IsEvalChannelInvalid(NiEvaluator::EVALPOSINDEX)) ||
                (bHadRotation && kTrack.hasRotationSource &&
                    !pkEvaluator->IsEvalChannelInvalid(NiEvaluator::EVALROTINDEX)) ||
                (bHadScale && kTrack.hasScaleSource &&
                    !pkEvaluator->IsEvalChannelInvalid(NiEvaluator::EVALSCALEINDEX)))
            {
                ++uiDuplicateComponentSources;
            }
        }

        // Preserve KF/KFM-authored components when a NIF controller targets
        // the same node. The embedded controller fills only components not
        // supplied by the sequence; on effect-only nodes it supplies the full
        // transform. Multiple NIF controllers still follow list order and the
        // later one may replace an earlier NIF-controller component.
        std::unordered_map<std::string, TransformComponentMask>
            kSequenceProtectedComponents;
        for (const BakedNodeTrack& kTrack : kTracks)
        {
            TransformComponentMask kMask;
            kMask.position = kTrack.hasPositionSource;
            kMask.rotation = kTrack.hasRotationSource;
            kMask.scale = kTrack.hasScaleSource;
            kSequenceProtectedComponents.emplace(kTrack.name, kMask);
        }

        const ControllerBakeStats kControllerStats =
            BakeNifControllersIntoTracks(kNifControllers, kGrid,
                fSafeUnitScale, bConvertToUnrealAxes,
                kSequenceProtectedComponents, kTracks, kTrackByName);

        const char* pcSequenceName = pkSequence->GetName().c_str();
        const std::string kAnimationName =
            (pcSequenceName && pcSequenceName[0] != '\0')
            ? pcSequenceName : ("anim_" + std::to_string(s));

        aiAnimation* pkAnimation = BuildAiAnimation(
            kAnimationName, kGrid, kTracks);
        if (!pkAnimation)
            continue;

        std::cout << "  Baked animation '" << kAnimationName << "': "
            << pkAnimation->mNumChannels << " node channels, "
            << kGrid.ticks.size() << " samples at "
            << kGrid.sampleRate << " fps" << std::endl;

#ifndef EE_REMOVE_BACK_COMPAT_STREAMING
        if (kLegacySelection.controller)
        {
            std::cout << "    NiMultiTargetTransformController resolved "
                << uiLegacyUniqueNameMatches
                << " channel target(s) by unique direct name";
            if (uiLegacyOrdinalNameMatches > 0)
            {
                std::cout << " and " << uiLegacyOrdinalNameMatches
                    << " duplicate-name target(s) by validated order";
            }
            std::cout << "." << std::endl;

            if (!kLegacySelection.ordinalTargets.empty())
            {
                std::cout << "    Validated legacy target ordering for "
                    << uiTransformEvaluatorCount
                    << " transform evaluator(s)." << std::endl;
            }
        }
        else if (kLegacySelection.ambiguousController)
        {
            std::cerr << "  Warning: animation '" << kAnimationName
                << "' has more than one possible "
                << "NiMultiTargetTransformController; direct target hints "
                << "were not used." << std::endl;
        }
        if (uiLegacyUnnamedFallbacks > 0)
        {
            std::cout << "    Recovered " << uiLegacyUnnamedFallbacks
                << " unnamed transform target(s) from the validated legacy "
                << "target order." << std::endl;
        }
        if (uiLegacyNameConflicts > 0)
        {
            std::cerr << "  Warning: animation '" << kAnimationName
                << "' had " << uiLegacyNameConflicts
                << " sequence-name/multi-target-order conflict(s); sequence "
                << "AVObject names were kept." << std::endl;
        }
        if (uiLegacyNullOrdinalTargets > 0)
        {
            std::cerr << "  Warning: animation '" << kAnimationName
                << "' matched a validated legacy target order containing "
                << uiLegacyNullOrdinalTargets << " unresolved/null target(s)."
                << std::endl;
        }
#endif

        if (uiUnnamedTransformEvaluators > 0)
        {
            std::cerr << "  Warning: animation '" << kAnimationName
                << "' skipped " << uiUnnamedTransformEvaluators
                << " unnamed transform evaluator(s) because no unambiguous "
                << "legacy target mapping was available." << std::endl;
        }

        if (!kNifControllers.empty())
        {
            std::cout << "    Merged " << kControllerStats.controllersBaked
                << "/" << kNifControllers.size()
                << " NIF transform controller(s)." << std::endl;

            const unsigned int uiProtectedComponents =
                kControllerStats.protectedPositionComponents +
                kControllerStats.protectedRotationComponents +
                kControllerStats.protectedScaleComponents;
            if (uiProtectedComponents > 0)
            {
                std::cout << "    KF/KFM precedence protected "
                    << kControllerStats.protectedPositionComponents
                    << " position, "
                    << kControllerStats.protectedRotationComponents
                    << " rotation, and "
                    << kControllerStats.protectedScaleComponents
                    << " scale controller component(s)." << std::endl;
            }
            if (kControllerStats.skippedAccumulationRoots > 0)
            {
                std::cout << "    Skipped "
                    << kControllerStats.skippedAccumulationRoots
                    << " NIF accumulation-root controller(s)." << std::endl;
            }
            if (kControllerStats.controllerSamplesFailed > 0)
            {
                std::cerr << "  Warning: animation '" << kAnimationName
                    << "' had " << kControllerStats.controllerSamplesFailed
                    << " failed NIF controller sample(s)." << std::endl;
            }
        }

        if (uiUnsupportedEvaluators > 0)
        {
            std::cerr << "  Warning: animation '" << kAnimationName
                << "' skipped " << uiUnsupportedEvaluators
                << " non-transform evaluator(s)." << std::endl;
        }
        if (uiMissingRestNodes > 0)
        {
            std::cerr << "  Warning: animation '" << kAnimationName
                << "' referenced " << uiMissingRestNodes
                << " node name(s) absent from the model NIF; identity was used "
                << "for missing components." << std::endl;
        }
        if (uiDuplicateComponentSources > 0)
        {
            std::cerr << "  Warning: animation '" << kAnimationName
                << "' had " << uiDuplicateComponentSources
                << " duplicate transform component source(s); the later evaluator "
                << "was merged into the same node channel." << std::endl;
        }
        if (uiSkippedAccumulationRoots > 0)
        {
            std::cerr << "  Animation '" << kAnimationName
                << "' skipped " << uiSkippedAccumulationRoots
                << " accumulation root channel(s) and kept their NIF bind pose."
                << std::endl;
        }

        kResult.push_back(pkAnimation);
    }

    return kResult;
}

//--------------------------------------------------------------------------------------------------
std::vector<aiAnimation*> AnimationExporter::BuildFromNifControllers(
    NiAVObject* pkRoot,
    float fUnitScale,
    float fSampleRate,
    bool bConvertToUnrealAxes)
{
    if (!pkRoot)
        return {};

    std::vector<NifControllerEntry> kControllers;
    CollectTransformControllers(pkRoot, kControllers);
    if (kControllers.empty())
        return {};

    const float fSafeUnitScale = SanitizeUnitScale(fUnitScale);
    const float fSafeSampleRate = SanitizeSampleRate(fSampleRate);

    float fMaxDurationSeconds = 0.0f;
    for (const NifControllerEntry& kEntry : kControllers)
        fMaxDurationSeconds = std::max(fMaxDurationSeconds, kEntry.durationSeconds);
    if (fMaxDurationSeconds <= 0.0f)
        return {};

    SampleGrid kGrid;
    kGrid.sampleRate = fSafeSampleRate;
    kGrid.durationSeconds = fMaxDurationSeconds;
    const unsigned int uiSampleCount = static_cast<unsigned int>(std::clamp(
        std::ceil(static_cast<double>(fMaxDurationSeconds) * fSafeSampleRate) + 1.0,
        2.0, static_cast<double>(MAX_BAKED_SAMPLES)));
    kGrid.ticks.resize(uiSampleCount);
    kGrid.localTimes.resize(uiSampleCount);
    for (unsigned int i = 0; i < uiSampleCount; ++i)
    {
        const float fSeconds = (i + 1u == uiSampleCount)
            ? fMaxDurationSeconds
            : std::min(static_cast<float>(i) / fSafeSampleRate,
                fMaxDurationSeconds);
        kGrid.localTimes[i] = fSeconds;
        kGrid.ticks[i] = static_cast<double>(fSeconds) * fSafeSampleRate;
    }

    std::vector<BakedNodeTrack> kTracks;
    std::unordered_map<std::string, size_t> kTrackByName;
    const std::unordered_map<std::string, TransformComponentMask>
        kNoProtectedComponents;

    const ControllerBakeStats kControllerStats =
        BakeNifControllersIntoTracks(kControllers, kGrid,
            fSafeUnitScale, bConvertToUnrealAxes,
            kNoProtectedComponents, kTracks, kTrackByName);

    aiAnimation* pkAnimation = BuildAiAnimation("NifAnimation", kGrid, kTracks);
    if (!pkAnimation)
        return {};

    std::cout << "  Baked NIF controller animation: "
        << pkAnimation->mNumChannels << " node channels, "
        << kGrid.ticks.size() << " samples at "
        << kGrid.sampleRate << " fps; controllers="
        << kControllerStats.controllersBaked << "/"
        << kControllers.size() << std::endl;

    if (kControllerStats.skippedAccumulationRoots > 0)
    {
        std::cout << "    Skipped "
            << kControllerStats.skippedAccumulationRoots
            << " NIF accumulation-root controller(s)." << std::endl;
    }
    if (kControllerStats.controllerSamplesFailed > 0)
    {
        std::cerr << "  Warning: NIF controller animation had "
            << kControllerStats.controllerSamplesFailed
            << " failed sample(s)." << std::endl;
    }

    return {pkAnimation};
}
