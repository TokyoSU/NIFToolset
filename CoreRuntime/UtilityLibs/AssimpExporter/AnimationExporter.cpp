#include "AnimationExporter.h"
#include "ExportNaming.h"
#include "AxisConversion.h"

#include <NiNode.h>
#include <NiTimeController.h>
#include <NiTransformController.h>
#include <NiTransformInterpolator.h>
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

        const char* pcName = pkObject->GetName().c_str();
        if (pcName && pcName[0] != '\0')
            kOut.emplace(pcName, pkObject);

        if (NiIsKindOf(NiNode, pkObject))
        {
            NiNode* pkNode = NiStaticCast(NiNode, pkObject);
            for (unsigned int i = 0; i < pkNode->GetArrayCount(); ++i)
                CollectNodesByName(pkNode->GetAt(i), kOut);
        }
    }

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
        NiTransformInterpolator* interpolator = nullptr;
        float beginTime = 0.0f;
        float endTime = 0.0f;
        float frequency = 1.0f;
        float durationSeconds = 0.0f;
    };

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

                if (pkInterpolator &&
                    NiIsKindOf(NiTransformInterpolator, pkInterpolator))
                {
                    NiTransformInterpolator* pkTransformInterpolator =
                        NiStaticCast(NiTransformInterpolator, pkInterpolator);

                    float fBegin = pkTransformController->GetBeginKeyTime();
                    float fEnd = pkTransformController->GetEndKeyTime();
                    if (!std::isfinite(fBegin) || !std::isfinite(fEnd) || fEnd <= fBegin)
                        pkTransformInterpolator->GetActiveTimeRange(fBegin, fEnd);

                    const float fFrequency =
                        std::isfinite(pkTransformController->GetFrequency()) &&
                        std::abs(pkTransformController->GetFrequency()) > MIN_POSITIVE
                        ? std::abs(pkTransformController->GetFrequency()) : 1.0f;

                    if (std::isfinite(fBegin) && std::isfinite(fEnd) && fEnd > fBegin)
                    {
                        NifControllerEntry kEntry;
                        kEntry.name = GetExportNodeName(pkObject);
                        kEntry.controller = pkTransformController;
                        kEntry.object = pkObject;
                        kEntry.interpolator = pkTransformInterpolator;
                        kEntry.beginTime = fBegin;
                        kEntry.endTime = fEnd;
                        kEntry.frequency = fFrequency;
                        kEntry.durationSeconds = (fEnd - fBegin) / fFrequency;
                        kOut.push_back(kEntry);
                    }
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
}

//--------------------------------------------------------------------------------------------------
std::vector<aiAnimation*> AnimationExporter::BuildFromSequenceDatas(
    const std::vector<NiSequenceDataPtr>& kSequenceDatas,
    NiAVObject* pkNifRoot,
    float fUnitScale,
    float fSampleRate,
    bool bConvertToUnrealAxes)
{
    std::vector<aiAnimation*> kResult;
    const float fSafeUnitScale = SanitizeUnitScale(fUnitScale);
    const float fSafeSampleRate = SanitizeSampleRate(fSampleRate);

    NodeByNameMap kNodesByName;
    CollectNodesByName(pkNifRoot, kNodesByName);

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

        for (unsigned int e = 0; e < pkSequence->GetNumEvaluators(); ++e)
        {
            NiEvaluator* pkEvaluator = pkSequence->GetEvaluatorAt(e);
            if (!pkEvaluator)
                continue;

            const char* pcNodeName = pkEvaluator->GetAVObjectName();
            if (!pcNodeName || pcNodeName[0] == '\0')
                continue;

            if (!IsBakeableTransformEvaluator(pkEvaluator))
            {
                // Non-transform evaluators can drive material, visibility,
                // morph, look-at, or path-controller data. Do not reinterpret
                // those channels as skeletal transforms.
                ++uiUnsupportedEvaluators;
                continue;
            }

            const std::string kNodeName(pcNodeName);
            NiAVObject* pkRestObject = nullptr;
            auto kRestNode = kNodesByName.find(kNodeName);
            if (kRestNode != kNodesByName.end())
                pkRestObject = kRestNode->second;
            else
                ++uiMissingRestNodes;

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

    for (NifControllerEntry& kEntry : kControllers)
    {
        BakedNodeTrack& kTrack = GetOrCreateTrack(kEntry.name,
            kEntry.object, kGrid, fSafeUnitScale,
            bConvertToUnrealAxes, kTracks, kTrackByName);

        for (size_t i = 0; i < kGrid.localTimes.size(); ++i)
        {
            const float fControllerSeconds = std::min(
                kGrid.localTimes[i], kEntry.durationSeconds);
            const float fLocalTime = std::min(
                kEntry.beginTime + fControllerSeconds * kEntry.frequency,
                kEntry.endTime);

            NiQuatTransform kValue;
            kEntry.interpolator->Update(fLocalTime, kEntry.object, kValue);

            if (kValue.IsTranslateValid())
            {
                const NiPoint3& kPosition = kValue.GetTranslate();
                const aiVector3D kSourcePosition(
                    kPosition.x * fSafeUnitScale,
                    kPosition.y * fSafeUnitScale,
                    kPosition.z * fSafeUnitScale);
                kTrack.positions[i].mValue = AxisConversion::ToUnrealVector(
                    kSourcePosition, bConvertToUnrealAxes);
                kTrack.hasPositionSource = true;
                kTrack.hasAnimationData = true;
            }
            if (kValue.IsRotateValid())
            {
                kTrack.rotations[i].mValue = NormalizeQuaternion(
                    AxisConversion::ToUnrealQuaternion(
                        ToAiQuat(kValue.GetRotate()), bConvertToUnrealAxes));
                kTrack.hasRotationSource = true;
                kTrack.hasAnimationData = true;
            }
            if (kValue.IsScaleValid())
            {
                const float fScale = kValue.GetScale();
                kTrack.scales[i].mValue = aiVector3D(fScale, fScale, fScale);
                kTrack.hasScaleSource = true;
                kTrack.hasAnimationData = true;
            }
        }
    }

    aiAnimation* pkAnimation = BuildAiAnimation("NifAnimation", kGrid, kTracks);
    if (!pkAnimation)
        return {};

    std::cout << "  Baked NIF controller animation: "
        << pkAnimation->mNumChannels << " node channels, "
        << kGrid.ticks.size() << " samples at "
        << kGrid.sampleRate << " fps" << std::endl;
    return {pkAnimation};
}
