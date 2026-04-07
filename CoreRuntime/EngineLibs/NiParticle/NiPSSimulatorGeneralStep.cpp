// EMERGENT GAME TECHNOLOGIES PROPRIETARY INFORMATION
//
// This software is supplied under the terms of a license agreement or
// nondisclosure agreement with Emergent Game Technologies and may not
// be copied or disclosed except in accordance with the terms of that
// agreement.
//
//      Copyright (c) 1996-2009 Emergent Game Technologies.
//      All Rights Reserved.
//
// Emergent Game Technologies, Calabasas, CA 91302
// http://www.emergent.net

// Precompiled Header
#include "NiParticlePCH.h"

#include "NiPSSimulatorGeneralStep.h"
#include "NiPSSimulatorKernelHelpers.h"
#include "NiPSCommonSemantics.h"
#include "NiPSParticleSystem.h"

NiImplementRTTI(NiPSSimulatorGeneralStep, NiPSSimulatorStep);

//--------------------------------------------------------------------------------------------------
NiPSSimulatorGeneralStep::NiPSSimulatorGeneralStep() :
    m_kInputStructIS(&m_kInputStruct, 1),
    m_pkColorIS(NULL),
    m_pkFlagsIS(NULL),
    m_pkAgeIS(NULL),
    m_pkLifeSpanIS(NULL),
    m_pkRotAngleIS(NULL),
    m_pkRotSpeedIS(NULL),
    m_pkSizeOS(NULL),
    m_pkColorOS(NULL),
    m_pkRotAngleOS(NULL),
    m_pkSizeKeys(NULL),
    m_pkColorKeys(NULL),
    m_pkRotationKeys(NULL)
{
    m_kInputStruct.m_ucBGRA = 0;
    m_kInputStruct.m_ucNumSizeKeys = 0;
    m_kInputStruct.m_eSizeLoopBehavior = PSKERNELLOOP_AGESCALE;
    m_kInputStruct.m_ucNumColorKeys = 0;
    m_kInputStruct.m_eColorLoopBehavior = PSKERNELLOOP_AGESCALE;
    m_kInputStruct.m_ucNumRotationKeys = 0;
    m_kInputStruct.m_eRotationLoopBehavior = PSKERNELLOOP_AGESCALE;
    m_kInputStruct.m_fGrowTime = 0.0f;
    m_kInputStruct.m_fShrinkTime = 0.0f;
    m_kInputStruct.m_usGrowGeneration = 0;
    m_kInputStruct.m_usShrinkGeneration = 0;
}

//--------------------------------------------------------------------------------------------------
NiPSSimulatorGeneralStep::~NiPSSimulatorGeneralStep()
{
    NiAlignedFree(m_pkSizeKeys);
    NiAlignedFree(m_pkColorKeys);
    NiAlignedFree(m_pkRotationKeys);
}

//--------------------------------------------------------------------------------------------------
NiSPKernel* NiPSSimulatorGeneralStep::GetKernel()
{
    return &m_kKernel;
}

//--------------------------------------------------------------------------------------------------
NiUInt16 NiPSSimulatorGeneralStep::GetLargestInputStride()
{
    return m_pkColorIS->GetStride();
}

//--------------------------------------------------------------------------------------------------
void NiPSSimulatorGeneralStep::PrepareInputStream(NiPSParticleSystem* pkParticleSystem,
    const NiFixedString& kSemantic, NiSPStream* pkStream)
{
    // Update the stored pointer to the stream.
    // Associate particle data with Floodgate streams.
    if (kSemantic == NiPSCommonSemantics::PARTICLECOLOR())
    {
        if (m_pkColorIS != pkStream)
        {
            NiDelete m_pkColorIS;
            m_pkColorIS = pkStream;
        }
        m_pkColorIS->SetData(pkParticleSystem->GetColors());
    }
    else if (kSemantic == NiPSCommonSemantics::PARTICLEFLAGS())
    {
        if (m_pkFlagsIS != pkStream)
        {
            NiDelete m_pkFlagsIS;
            m_pkFlagsIS = pkStream;
        }
        m_pkFlagsIS->SetData(pkParticleSystem->GetFlags());
    }
    else if (kSemantic == NiPSCommonSemantics::PARTICLEAGE())
    {
        if (m_pkAgeIS != pkStream)
        {
            NiDelete m_pkAgeIS;
            m_pkAgeIS = pkStream;
        }
        m_pkAgeIS->SetData(pkParticleSystem->GetAges());
    }
    else if (kSemantic == NiPSCommonSemantics::PARTICLELIFESPAN())
    {
        if (m_pkLifeSpanIS != pkStream)
        {
            NiDelete m_pkLifeSpanIS;
            m_pkLifeSpanIS = pkStream;
        }
        m_pkLifeSpanIS->SetData(pkParticleSystem->GetLifeSpans());
    }
    else if (kSemantic == NiPSCommonSemantics::PARTICLEROTINITANGLE())
    {
        if (m_pkRotAngleIS != pkStream)
        {
            NiDelete m_pkRotAngleIS;
            m_pkRotAngleIS = pkStream;
        }
        m_pkRotAngleIS->SetData(pkParticleSystem->GetInitialRotationAngles());
    }
    else if (kSemantic == NiPSCommonSemantics::PARTICLEROTSPEED())
    {
        if (m_pkRotSpeedIS != pkStream)
        {
            NiDelete m_pkRotSpeedIS;
            m_pkRotSpeedIS = pkStream;
        }
        m_pkRotSpeedIS->SetData(pkParticleSystem->GetRotationSpeeds());
    }
    else
    {
        EE_FAIL("Unknown semantic type!");
    }
}

//--------------------------------------------------------------------------------------------------
void NiPSSimulatorGeneralStep::PrepareOutputStream(
    NiPSParticleSystem* pkParticleSystem,
    const NiFixedString& kSemantic,
    NiSPStream* pkStream)
{
    EE_UNUSED_ARG(pkStream);
    // Update the stored pointer to the stream.
    if (kSemantic == NiPSCommonSemantics::PARTICLESIZE())
    {
        EE_ASSERT(m_pkSizeOS == pkStream);
        m_pkSizeOS->SetData(pkParticleSystem->GetSizes());
    }
    else if (kSemantic == NiPSCommonSemantics::PARTICLECOLOR())
    {
        EE_ASSERT(m_pkColorOS == pkStream);
        m_pkColorOS->SetData(pkParticleSystem->GetColors());
    }
    else if (kSemantic == NiPSCommonSemantics::PARTICLEROTANGLE())
    {
        EE_ASSERT(m_pkRotAngleOS == pkStream);
        m_pkRotAngleOS->SetData(pkParticleSystem->GetRotationAngles());
    }
    else
    {
        EE_FAIL("Unknown semantic type!");
    }
}

//--------------------------------------------------------------------------------------------------
NiSPTaskPtr NiPSSimulatorGeneralStep::Attach(
    NiPSParticleSystem* pkParticleSystem)
{
    NiSPTaskPtr spTask = NiSPTask::GetNewTask(10, 4);

    // Create input streams.
    SetInputCount(6);
    m_pkColorIS = NiNew NiTSPStream<NiRGBA>();
    AddInput(NiPSCommonSemantics::PARTICLECOLOR(), m_pkColorIS);
    m_pkFlagsIS = NiNew NiTSPStream<NiUInt32>();
    AddInput(NiPSCommonSemantics::PARTICLEFLAGS(), m_pkFlagsIS);
    m_pkAgeIS = NiNew NiTSPStream<float>();
    AddInput(NiPSCommonSemantics::PARTICLEAGE(), m_pkAgeIS);
    m_pkLifeSpanIS = NiNew NiTSPStream<float>();
    AddInput(NiPSCommonSemantics::PARTICLELIFESPAN(), m_pkLifeSpanIS);
    m_pkRotAngleIS = NiNew NiTSPStream<float>();
    AddInput(NiPSCommonSemantics::PARTICLEROTINITANGLE(), m_pkRotAngleIS);
    m_pkRotSpeedIS = NiNew NiTSPStream<float>();
    AddInput(NiPSCommonSemantics::PARTICLEROTSPEED(), m_pkRotSpeedIS);

    // Create output streams.
    SetOutputCount(3);
    m_pkSizeOS = NiNew NiTSPStream<float>();
    AddOutput(NiPSCommonSemantics::PARTICLESIZE(), m_pkSizeOS);
    m_pkColorOS = NiNew NiTSPStream<NiRGBA>();
    AddOutput(NiPSCommonSemantics::PARTICLECOLOR(), m_pkColorOS);
    m_pkRotAngleOS = NiNew NiTSPStream<float>();
    AddOutput(NiPSCommonSemantics::PARTICLEROTANGLE(), m_pkRotAngleOS);

    // Add local streams to the task.
    spTask->AddInput(&m_kInputStructIS);
    spTask->AddInput(&m_kSizeKeyIS);
    spTask->AddInput(&m_kColorKeyIS);
    spTask->AddInput(&m_kRotationKeyIS);

    // Mark the swizzle flag if the color format is set to BGRA
    NiDataStreamRef* pkRef = pkParticleSystem->FindStreamRef(NiCommonSemantics::COLOR());
    if (pkRef)
    {
        m_kInputStruct.m_ucBGRA = 
            (pkRef->GetElementDescAt(0).GetFormat() == NiDataStreamElement::F_NORMUINT8_4_BGRA) ? 1 : 0;
    }

    return spTask;
}

//--------------------------------------------------------------------------------------------------
void NiPSSimulatorGeneralStep::Detach(NiPSParticleSystem* pkParticleSystem)
{
    // Call base class version to ensure that streams and semantics arrays
    // are properly cleared.
    NiPSSimulatorStep::Detach(pkParticleSystem);

    // Clear out input stream pointers.
    m_pkColorIS = NULL;
    m_pkFlagsIS = NULL;
    m_pkAgeIS = NULL;
    m_pkLifeSpanIS = NULL;
    m_pkRotAngleIS = NULL;
    m_pkRotSpeedIS = NULL;

    // Clear out output stream pointers.
    m_pkSizeOS = NULL;
    m_pkColorOS = NULL;
    m_pkRotAngleOS = NULL;
}

//--------------------------------------------------------------------------------------------------
bool NiPSSimulatorGeneralStep::Update(
    NiPSParticleSystem* pkParticleSystem,
    float fTime)
{
    // Update block count for Floodgate streams to the number of active
    // particles.
    const NiUInt32 uiNumParticles = pkParticleSystem->GetNumParticles();
    m_pkFlagsIS->SetBlockCount(uiNumParticles);
    m_pkAgeIS->SetBlockCount(uiNumParticles);
    m_pkLifeSpanIS->SetBlockCount(uiNumParticles);
    if (pkParticleSystem->HasRotations())
    {
        m_pkRotAngleIS->SetBlockCount(uiNumParticles);
        m_pkRotAngleOS->SetBlockCount(uiNumParticles);
        m_pkRotSpeedIS->SetBlockCount(uiNumParticles);

        if (m_kInputStruct.m_ucNumRotationKeys > 0)
        {
            // Set up color key stream.
            m_kRotationKeyIS.SetData(m_pkRotationKeys);
            m_kRotationKeyIS.SetBlockCount(m_kInputStruct.m_ucNumRotationKeys);
        }
    }
    m_pkSizeOS->SetBlockCount(uiNumParticles);
    if (pkParticleSystem->HasColors())
    {
        m_pkColorIS->SetBlockCount(uiNumParticles);
        m_pkColorOS->SetBlockCount(uiNumParticles);
    }

    if (pkParticleSystem->HasColors() && m_kInputStruct.m_ucNumColorKeys > 0)
    {
        // Set up color key stream.
        m_kColorKeyIS.SetData(m_pkColorKeys);
        m_kColorKeyIS.SetBlockCount(m_kInputStruct.m_ucNumColorKeys);
    }

    if (m_kInputStruct.m_ucNumSizeKeys > 0)
    {
        // Set up color key stream.
        m_kSizeKeyIS.SetData(m_pkSizeKeys);
        m_kSizeKeyIS.SetBlockCount(m_kInputStruct.m_ucNumSizeKeys);
    }

    // Set up input struct.
    m_kInputStruct.m_fCurrentTime = fTime;

    return (
        GetGrowTime() > 0.0f ||
        GetShrinkTime() > 0.0f ||
        m_kInputStruct.m_ucNumSizeKeys > 0 ||
        (pkParticleSystem->HasColors() && m_kInputStruct.m_ucNumColorKeys > 0) ||
        pkParticleSystem->HasRotations());
}

//--------------------------------------------------------------------------------------------------
void NiPSSimulatorGeneralStep::InitializeNewParticle(NiPSParticleSystem* pkParticleSystem,
    NiUInt32 uiIndex)
{
    NiUInt16 usGeneration = NiPSFlagsHelpers::GetGeneration(pkParticleSystem->GetFlags()[uiIndex]);

    // Update grow/shrink size.
    NiPSSimulatorKernelHelpers::UpdateParticleSize(pkParticleSystem->GetSizes()[uiIndex],
        usGeneration, pkParticleSystem->GetAges()[uiIndex],
        pkParticleSystem->GetLifeSpans()[uiIndex], m_kInputStruct.m_fGrowTime,
        m_kInputStruct.m_fShrinkTime, m_kInputStruct.m_usGrowGeneration,
        m_kInputStruct.m_usShrinkGeneration,
        m_kInputStruct.m_ucNumSizeKeys, m_pkSizeKeys, m_kInputStruct.m_eSizeLoopBehavior);

    if (pkParticleSystem->HasColors() && m_kInputStruct.m_ucNumColorKeys > 0)
    {
        // Update color animation.
        NiPSSimulatorKernelHelpers::UpdateParticleColor(pkParticleSystem->GetColors()[uiIndex],
            pkParticleSystem->GetColors()[uiIndex],
            pkParticleSystem->GetAges()[uiIndex], pkParticleSystem->GetLifeSpans()[uiIndex],
            m_kInputStruct.m_ucNumColorKeys,
            m_pkColorKeys, m_kInputStruct.m_eColorLoopBehavior,
            m_kInputStruct.m_ucBGRA);
    }

    if (pkParticleSystem->HasRotations())
    {
        // Update rotation angle.
        NiPSSimulatorKernelHelpers::UpdateParticleRotation(
            pkParticleSystem->GetRotationAngles()[uiIndex],
            pkParticleSystem->GetInitialRotationAngles()[uiIndex],
            pkParticleSystem->GetRotationSpeeds() ?
            pkParticleSystem->GetRotationSpeeds()[uiIndex] : 0.0f,
            pkParticleSystem->GetAges()[uiIndex], pkParticleSystem->GetLifeSpans()[uiIndex],
            m_kInputStruct.m_ucNumRotationKeys,
            m_pkRotationKeys, m_kInputStruct.m_eRotationLoopBehavior);
    }
}

//--------------------------------------------------------------------------------------------------
void NiPSSimulatorGeneralStep::CopySizeKeys(const NiPSKernelFloatKey* pkSizeKeys,
    const NiUInt8 ucNumKeys)
{
    EE_ASSERT((!pkSizeKeys && ucNumKeys == 0) || (pkSizeKeys && ucNumKeys > 0));

    if (ucNumKeys != m_kInputStruct.m_ucNumSizeKeys)
    {
        NiAlignedFree(m_pkSizeKeys);
        m_pkSizeKeys = NiAlignedAlloc(NiPSKernelFloatKey, ucNumKeys, NIPSKERNEL_ALIGNMENT);
        m_kInputStruct.m_ucNumSizeKeys = ucNumKeys;
    }
    NiMemcpy(m_pkSizeKeys, sizeof(NiPSKernelFloatKey) * ucNumKeys,
        pkSizeKeys, sizeof(NiPSKernelFloatKey) * ucNumKeys);
}

//--------------------------------------------------------------------------------------------------
void NiPSSimulatorGeneralStep::CopyColorKeys(
    const NiPSKernelColorKey* pkColorKeys, const NiUInt8 ucNumKeys)
{
    EE_ASSERT((!pkColorKeys && ucNumKeys == 0) || (pkColorKeys && ucNumKeys > 0));

    if (ucNumKeys != m_kInputStruct.m_ucNumColorKeys)
    {
        NiAlignedFree(m_pkColorKeys);
        m_pkColorKeys = NiAlignedAlloc(NiPSKernelColorKey, ucNumKeys, NIPSKERNEL_ALIGNMENT);
        m_kInputStruct.m_ucNumColorKeys = ucNumKeys;
    }
    NiMemcpy(m_pkColorKeys, sizeof(NiPSKernelColorKey) * ucNumKeys,
        pkColorKeys, sizeof(NiPSKernelColorKey) * ucNumKeys);
}

//--------------------------------------------------------------------------------------------------
void NiPSSimulatorGeneralStep::CopyRotationKeys(
    const NiPSKernelFloatKey* pkRotationKeys, const NiUInt8 ucNumKeys)
{
    EE_ASSERT((!pkRotationKeys && ucNumKeys == 0) || (pkRotationKeys && ucNumKeys > 0));

    if (ucNumKeys != m_kInputStruct.m_ucNumRotationKeys)
    {
        NiAlignedFree(m_pkRotationKeys);
        m_pkRotationKeys = NiAlignedAlloc(NiPSKernelFloatKey, ucNumKeys, NIPSKERNEL_ALIGNMENT);
        m_kInputStruct.m_ucNumRotationKeys = ucNumKeys;
    }
    NiMemcpy(m_pkRotationKeys, sizeof(NiPSKernelColorKey) * ucNumKeys,
        pkRotationKeys, sizeof(NiPSKernelColorKey) * ucNumKeys);
}

//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// Cloning
//--------------------------------------------------------------------------------------------------
NiImplementCreateClone(NiPSSimulatorGeneralStep);

//--------------------------------------------------------------------------------------------------
void NiPSSimulatorGeneralStep::CopyMembers(
    NiPSSimulatorGeneralStep* pkDest,
    NiCloningProcess& kCloning)
{
    NiPSSimulatorStep::CopyMembers(pkDest, kCloning);

    pkDest->m_kInputStruct.m_ucNumSizeKeys = m_kInputStruct.m_ucNumSizeKeys;
    pkDest->m_kInputStruct.m_eSizeLoopBehavior = m_kInputStruct.m_eSizeLoopBehavior;
    pkDest->m_kInputStruct.m_ucNumColorKeys = m_kInputStruct.m_ucNumColorKeys;
    pkDest->m_kInputStruct.m_eColorLoopBehavior = m_kInputStruct.m_eColorLoopBehavior;
    pkDest->m_kInputStruct.m_ucNumRotationKeys = m_kInputStruct.m_ucNumRotationKeys;
    pkDest->m_kInputStruct.m_eRotationLoopBehavior = m_kInputStruct.m_eRotationLoopBehavior;

    pkDest->m_pkSizeKeys =
        NiAlignedAlloc(NiPSKernelFloatKey, m_kInputStruct.m_ucNumSizeKeys, NIPSKERNEL_ALIGNMENT);
    for (NiUInt32 ui = 0; ui < m_kInputStruct.m_ucNumSizeKeys; ++ui)
    {
        pkDest->m_pkSizeKeys[ui].m_fValue = m_pkSizeKeys[ui].m_fValue;
        pkDest->m_pkSizeKeys[ui].m_fTime = m_pkSizeKeys[ui].m_fTime;
    }

    pkDest->m_pkColorKeys =
        NiAlignedAlloc(NiPSKernelColorKey, m_kInputStruct.m_ucNumColorKeys, NIPSKERNEL_ALIGNMENT);
    for (NiUInt32 ui = 0; ui < m_kInputStruct.m_ucNumColorKeys; ++ui)
    {
        pkDest->m_pkColorKeys[ui].m_fTime = m_pkColorKeys[ui].m_fTime;
        pkDest->m_pkColorKeys[ui].m_kColor = m_pkColorKeys[ui].m_kColor;
    }

    pkDest->m_pkRotationKeys = NiAlignedAlloc(NiPSKernelFloatKey,
        m_kInputStruct.m_ucNumRotationKeys, NIPSKERNEL_ALIGNMENT);
    for (NiUInt32 ui = 0; ui < m_kInputStruct.m_ucNumRotationKeys; ++ui)
    {
        pkDest->m_pkRotationKeys[ui].m_fTime = m_pkRotationKeys[ui].m_fTime;
        pkDest->m_pkRotationKeys[ui].m_fValue = m_pkRotationKeys[ui].m_fValue;
    }

    pkDest->SetGrowTime(GetGrowTime());
    pkDest->SetShrinkTime(GetShrinkTime());
    pkDest->SetGrowGeneration(GetGrowGeneration());
    pkDest->SetShrinkGeneration(GetShrinkGeneration());
}

//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// Streaming
//--------------------------------------------------------------------------------------------------
NiImplementCreateObject(NiPSSimulatorGeneralStep);

//--------------------------------------------------------------------------------------------------
void NiPSSimulatorGeneralStep::LoadBinary(NiStream& kStream)
{
    NiPSSimulatorStep::LoadBinary(kStream);

    if (kStream.GetFileVersion() < NiStream::GetVersion(20, 6, 1, 0))
    {
        m_kInputStruct.m_ucNumSizeKeys = 0;
        m_kInputStruct.m_eSizeLoopBehavior = PSKERNELLOOP_CLAMP_BIRTH;
        m_kInputStruct.m_ucNumRotationKeys = 0;
        m_kInputStruct.m_eRotationLoopBehavior = PSKERNELLOOP_CLAMP_BIRTH;

        NiStreamLoadBinary(kStream, m_kInputStruct.m_ucNumColorKeys);
        if (m_kInputStruct.m_ucNumColorKeys > 0)
        {
            m_pkColorKeys = NiAlignedAlloc(NiPSKernelColorKey, m_kInputStruct.m_ucNumColorKeys,
                NIPSKERNEL_ALIGNMENT);
            for (NiUInt8 uc = 0; uc < m_kInputStruct.m_ucNumColorKeys; ++uc)
            {
                NiStreamLoadBinary(kStream, m_pkColorKeys[uc].m_fTime);
                m_pkColorKeys[uc].m_kColor.LoadBinary(kStream);
            }
        }
        m_kInputStruct.m_eColorLoopBehavior = PSKERNELLOOP_AGESCALE;
    }
    else
    {
        NiStreamLoadBinary(kStream, m_kInputStruct.m_ucNumSizeKeys);
        m_pkSizeKeys = NiAlignedAlloc(NiPSKernelFloatKey, m_kInputStruct.m_ucNumSizeKeys,
            NIPSKERNEL_ALIGNMENT);
        for (NiUInt8 uc = 0; uc < m_kInputStruct.m_ucNumSizeKeys; ++uc)
        {
            NiStreamLoadBinary(kStream, m_pkSizeKeys[uc].m_fValue);
            NiStreamLoadBinary(kStream, m_pkSizeKeys[uc].m_fTime);
        }
        NiStreamLoadEnum(kStream, m_kInputStruct.m_eSizeLoopBehavior);

        NiStreamLoadBinary(kStream, m_kInputStruct.m_ucNumColorKeys);
        m_pkColorKeys = NiAlignedAlloc(NiPSKernelColorKey, m_kInputStruct.m_ucNumColorKeys,
            NIPSKERNEL_ALIGNMENT);
        for (NiUInt8 uc = 0; uc < m_kInputStruct.m_ucNumColorKeys; ++uc)
        {
            m_pkColorKeys[uc].m_kColor.LoadBinary(kStream);
            NiStreamLoadBinary(kStream, m_pkColorKeys[uc].m_fTime);
        }
        NiStreamLoadEnum(kStream, m_kInputStruct.m_eColorLoopBehavior);


        NiStreamLoadBinary(kStream, m_kInputStruct.m_ucNumRotationKeys);
        m_pkRotationKeys = NiAlignedAlloc(NiPSKernelFloatKey, m_kInputStruct.m_ucNumRotationKeys,
            NIPSKERNEL_ALIGNMENT);
        for (NiUInt8 uc = 0; uc < m_kInputStruct.m_ucNumRotationKeys; ++uc)
        {
            NiStreamLoadBinary(kStream, m_pkRotationKeys[uc].m_fValue);
            NiStreamLoadBinary(kStream, m_pkRotationKeys[uc].m_fTime);
        }
        NiStreamLoadEnum(kStream, m_kInputStruct.m_eRotationLoopBehavior);
    }

    float fValue;
    NiStreamLoadBinary(kStream, fValue);
    SetGrowTime(fValue);
    NiStreamLoadBinary(kStream, fValue);
    SetShrinkTime(fValue);
    NiUInt16 usValue;
    NiStreamLoadBinary(kStream, usValue);
    SetGrowGeneration(usValue);
    NiStreamLoadBinary(kStream, usValue);
    SetShrinkGeneration(usValue);
}

//--------------------------------------------------------------------------------------------------
void NiPSSimulatorGeneralStep::LinkObject(NiStream& kStream)
{
    NiPSSimulatorStep::LinkObject(kStream);
}

//--------------------------------------------------------------------------------------------------
bool NiPSSimulatorGeneralStep::RegisterStreamables(NiStream& kStream)
{
    return NiPSSimulatorStep::RegisterStreamables(kStream);
}

//--------------------------------------------------------------------------------------------------
void NiPSSimulatorGeneralStep::SaveBinary(NiStream& kStream)
{
    NiPSSimulatorStep::SaveBinary(kStream);

    NiStreamSaveBinary(kStream, m_kInputStruct.m_ucNumSizeKeys);
    for (NiUInt8 uc = 0; uc < m_kInputStruct.m_ucNumSizeKeys; ++uc)
    {
        NiStreamSaveBinary(kStream, m_pkSizeKeys[uc].m_fValue);
        NiStreamSaveBinary(kStream, m_pkSizeKeys[uc].m_fTime);
    }
    NiStreamSaveEnum(kStream, m_kInputStruct.m_eSizeLoopBehavior);

    NiStreamSaveBinary(kStream, m_kInputStruct.m_ucNumColorKeys);
    for (NiUInt8 uc = 0; uc < m_kInputStruct.m_ucNumColorKeys; ++uc)
    {
        m_pkColorKeys[uc].m_kColor.SaveBinary(kStream);
        NiStreamSaveBinary(kStream, m_pkColorKeys[uc].m_fTime);
    }
    NiStreamSaveEnum(kStream, m_kInputStruct.m_eColorLoopBehavior);

    NiStreamSaveBinary(kStream, m_kInputStruct.m_ucNumRotationKeys);
    for (NiUInt8 uc = 0; uc < m_kInputStruct.m_ucNumRotationKeys; ++uc)
    {
        NiStreamSaveBinary(kStream, m_pkRotationKeys[uc].m_fValue);
        NiStreamSaveBinary(kStream, m_pkRotationKeys[uc].m_fTime);
    }
    NiStreamSaveEnum(kStream, m_kInputStruct.m_eRotationLoopBehavior);

    NiStreamSaveBinary(kStream, GetGrowTime());
    NiStreamSaveBinary(kStream, GetShrinkTime());
    NiStreamSaveBinary(kStream, GetGrowGeneration());
    NiStreamSaveBinary(kStream, GetShrinkGeneration());
}

//--------------------------------------------------------------------------------------------------
bool NiPSSimulatorGeneralStep::IsEqual(NiObject* pkObject)
{
    if (!NiPSSimulatorStep::IsEqual(pkObject))
    {
        return false;
    }

    NiPSSimulatorGeneralStep* pkDest = (NiPSSimulatorGeneralStep*) pkObject;

    if (pkDest->m_kInputStruct.m_eSizeLoopBehavior != m_kInputStruct.m_eSizeLoopBehavior ||
        pkDest->m_kInputStruct.m_eColorLoopBehavior != m_kInputStruct.m_eColorLoopBehavior ||
        pkDest->m_kInputStruct.m_eRotationLoopBehavior !=
        m_kInputStruct.m_eRotationLoopBehavior ||
        pkDest->m_kInputStruct.m_ucNumSizeKeys != m_kInputStruct.m_ucNumSizeKeys ||
        pkDest->m_kInputStruct.m_ucNumColorKeys != m_kInputStruct.m_ucNumColorKeys ||
        pkDest->m_kInputStruct.m_ucNumRotationKeys != m_kInputStruct.m_ucNumRotationKeys)
    {
        return false;
    }

    for (NiUInt8 uc = 0; uc < m_kInputStruct.m_ucNumSizeKeys; ++uc)
    {
        if (pkDest->m_pkSizeKeys[uc].m_fValue != m_pkSizeKeys[uc].m_fValue ||
            pkDest->m_pkSizeKeys[uc].m_fTime != m_pkSizeKeys[uc].m_fTime)
        {
            return false;
        }
    }

    for (NiUInt8 uc = 0; uc < m_kInputStruct.m_ucNumColorKeys; ++uc)
    {
        if (pkDest->m_pkColorKeys[uc].m_kColor != m_pkColorKeys[uc].m_kColor ||
            pkDest->m_pkColorKeys[uc].m_fTime != m_pkColorKeys[uc].m_fTime)
        {
            return false;
        }
    }

    for (NiUInt8 uc = 0; uc < m_kInputStruct.m_ucNumRotationKeys; ++uc)
    {
        if (pkDest->m_pkRotationKeys[uc].m_fValue != m_pkRotationKeys[uc].m_fValue ||
            pkDest->m_pkRotationKeys[uc].m_fTime != m_pkRotationKeys[uc].m_fTime)
        {
            return false;
        }
    }

    if (pkDest->GetGrowTime() != GetGrowTime() ||
        pkDest->GetShrinkTime() != GetShrinkTime() ||
        pkDest->GetGrowGeneration() != GetGrowGeneration() ||
        pkDest->GetShrinkGeneration() != GetShrinkGeneration())
    {
        return false;
    }

    return true;
}

//--------------------------------------------------------------------------------------------------
// Viewer strings
//--------------------------------------------------------------------------------------------------
void NiPSSimulatorGeneralStep::GetViewerStrings(
    NiViewerStringsArray* pkStrings)
{
    NiPSSimulatorStep::GetViewerStrings(pkStrings);

    pkStrings->Add(NiGetViewerString(NiPSSimulatorGeneralStep::ms_RTTI
        .GetName()));

    NiUInt8 ucNumSizeKeys;
    NiPSKernelFloatKey* pkSizeKeys = GetSizeKeys(ucNumSizeKeys);
    pkStrings->Add(NiGetViewerString("Num Size Keys", ucNumSizeKeys));
    for (NiUInt8 uc = 0; uc < ucNumSizeKeys; uc++)
    {
        pkStrings->Add(NiGetViewerString("Size Key Time", pkSizeKeys[uc].m_fTime));
        pkStrings->Add(NiGetViewerString("Size Key Value", pkSizeKeys[uc].m_fValue));
    }
    pkStrings->Add(NiGetViewerString("Size Loop Behavior", (NiUInt8)GetSizeLoopBehavior()));

    NiUInt8 ucNumColorKeys;
    NiPSKernelColorKey* pkColorKeys = GetColorKeys(ucNumColorKeys);
    pkStrings->Add(NiGetViewerString("Num Color Keys", ucNumColorKeys));
    for (NiUInt8 uc = 0; uc < ucNumColorKeys; uc++)
    {
        pkStrings->Add(NiGetViewerString("Color Key Time", pkColorKeys[uc].m_fTime));
        pkStrings->Add(pkColorKeys[uc].m_kColor.GetViewerString("Color Key Value"));
    }
    pkStrings->Add(NiGetViewerString("Color Loop Behavior", (NiUInt8)GetColorLoopBehavior()));

    NiUInt8 ucNumRotationKeys;
    NiPSKernelFloatKey* pkRotationKeys = GetRotationKeys(ucNumRotationKeys);
    pkStrings->Add(NiGetViewerString("Num Rotation Keys", ucNumRotationKeys));
    for (NiUInt8 uc = 0; uc < ucNumRotationKeys; uc++)
    {
        pkStrings->Add(NiGetViewerString("Rotation Key Time", pkRotationKeys[uc].m_fTime));
        pkStrings->Add(NiGetViewerString("Rotation Key Value", pkRotationKeys[uc].m_fValue));
    }
    pkStrings->Add(NiGetViewerString("Rotation Loop Behavior",
        (NiUInt8)GetRotationLoopBehavior()));

    pkStrings->Add(NiGetViewerString("GrowTime", GetGrowTime()));
    pkStrings->Add(NiGetViewerString("ShrinkTime", GetShrinkTime()));
    pkStrings->Add(NiGetViewerString("GrowGeneration", GetGrowGeneration()));
    pkStrings->Add(NiGetViewerString("ShrinkGeneration", GetShrinkGeneration()));
}

//--------------------------------------------------------------------------------------------------
