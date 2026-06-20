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

#include "NiDecorationPCH.h"
#include "NiDecorationBillBoardGenerator.h"
#include "NiDecorationMaterial.h"
#include "NiDecorationFactories.h"
#include "NiDecorationLayer.h"

#include <NiFloatExtraData.h>
#include <NiMetricsTimer.h>
#include <NiMesh.h>
#include <NiSourceTexture.h>
#include <NiStencilProperty.h>
#include <NiTexturingProperty.h>

NiFixedString NiDecorationBillBoardGenerator::GENERATOR_NAME = NULL;

NiImplementRTTI(NiDecorationBillBoardGenerator, NiDecorationMeshGenerator, NiTypeMask::NiDecorationBillBoardGenerator);

//------------------------------------------------------------------------------------------------
NiDecorationBillBoardGenerator::NiDecorationBillBoardGenerator(bool bUseInstancing, 
    float fBillboardWidth, float fBillboardHeight,
    NiUInt32 uiVerticesPerMesh, NiUInt32 uiIndicesPerMesh) :
    NiDecorationMeshGenerator(bUseInstancing),
    m_fBillboardWidth(fBillboardWidth),
    m_fBillboardHeight(fBillboardHeight),
    m_fAnimationSwayMultipier(1.0f),
    m_pkVertexPositionTemplate(NULL),
    m_pkVertexNormalTemplate(NULL), 
    m_pkVertexTexCoordTemplate(NULL),
    m_pkIndexTemplate(NULL),
    m_spBaseTexture(NULL)
{
    SetVerticesPerMesh(uiVerticesPerMesh);
    SetIndicesPerMesh(uiIndicesPerMesh);
}

//------------------------------------------------------------------------------------------------
NiDecorationBillBoardGenerator::~NiDecorationBillBoardGenerator()
{
    EE_FREE(m_pkVertexPositionTemplate);
    EE_FREE(m_pkVertexNormalTemplate);
    EE_FREE(m_pkVertexTexCoordTemplate);
    EE_FREE(m_pkIndexTemplate);
}

//------------------------------------------------------------------------------------------------
void NiDecorationBillBoardGenerator::Initialize()
{
    CreateStreamTemplates(GetVerticesPerMesh(), GetIndicesPerMesh());
}

//------------------------------------------------------------------------------------------------
const NiFixedString& NiDecorationBillBoardGenerator::GetGeneratorType()
{
    return GENERATOR_NAME;
}

//------------------------------------------------------------------------------------------------
void NiDecorationBillBoardGenerator::_SDMInit()
{
    GENERATOR_NAME = "Billboard Generator";
}

//------------------------------------------------------------------------------------------------
void NiDecorationBillBoardGenerator::_SDMShutdown()
{
    GENERATOR_NAME = NULL;
}

//------------------------------------------------------------------------------------------------
void NiDecorationBillBoardGenerator::InitializeVertexElements(
    NiDataStreamElementSet& kVertexElements)
{
    kVertexElements.RemoveAll();

    // POSITION
    kVertexElements.AddElement(NiDataStreamElement::F_FLOAT32_3);

    // NORMAL
    kVertexElements.AddElement(NiDataStreamElement::F_FLOAT32_3);

    // UV
    kVertexElements.AddElement(NiDataStreamElement::F_FLOAT32_2);
}

//------------------------------------------------------------------------------------------------
bool NiDecorationBillBoardGenerator::GetVertexElementSemantic(
    NiUInt32 uiElementIndex,
    NiFixedString& kSemantic, NiUInt32& uiSemanticIndex)
{
    if (uiElementIndex == VE_POSITION)
    {
        kSemantic = NiCommonSemantics::POSITION();
        uiSemanticIndex = 0;
    }
    else if (uiElementIndex == VE_NORMAL)
    {
        kSemantic = NiCommonSemantics::NORMAL();
        uiSemanticIndex = 0;
    }
    else if (uiElementIndex == VE_UV_MODEL)
    {
        kSemantic = NiCommonSemantics::TEXCOORD();
        uiSemanticIndex = UV_MODEL;
    }
    else
    {
        return false;
    }

    return true;
}

//------------------------------------------------------------------------------------------------
const void* NiDecorationBillBoardGenerator::GetVertexElementTemplateData(
    size_t& stTemplateSize, NiDataStreamElement::Format eFormat,
    const NiFixedString& kSemantic, NiUInt32 uiSemanticIndex)
{
    EE_UNUSED_ARG(eFormat);

    if (kSemantic == NiCommonSemantics::POSITION())
    {
        EE_ASSERT(uiSemanticIndex == 0);
        EE_ASSERT(eFormat == NiDataStreamElement::F_FLOAT32_3);
        stTemplateSize = sizeof(NiPoint3) * GetVerticesPerMesh();
        return m_pkVertexPositionTemplate;
    }
    else if (kSemantic == NiCommonSemantics::NORMAL())
    {
        EE_ASSERT(uiSemanticIndex == 0);
        EE_ASSERT(eFormat == NiDataStreamElement::F_FLOAT32_3);
        stTemplateSize = sizeof(NiPoint3) * GetVerticesPerMesh();
        return m_pkVertexNormalTemplate;
    }
    else if (kSemantic == NiCommonSemantics::TEXCOORD())
    {
        EE_ASSERT(uiSemanticIndex < UV_MAX);
        EE_ASSERT(eFormat == NiDataStreamElement::F_FLOAT32_2);

        if (uiSemanticIndex == UV_MODEL)
        {
            stTemplateSize = sizeof(NiPoint2) * GetVerticesPerMesh();
            return m_pkVertexTexCoordTemplate;
        }
    }

    stTemplateSize = 0;
    return NULL;
}

//------------------------------------------------------------------------------------------------
bool NiDecorationBillBoardGenerator::GetVertexElementRequiresTransform(
    const NiFixedString& kSemantic, NiUInt32& uiSemanticIndex,
    bool& bRequiresTransform)
{
    EE_UNUSED_ARG(uiSemanticIndex);

    if (kSemantic == NiCommonSemantics::POSITION() || kSemantic == NiCommonSemantics::NORMAL())
    {
        EE_ASSERT(uiSemanticIndex == 0);
        bRequiresTransform = true;
    }
    else if (kSemantic == NiCommonSemantics::TEXCOORD())
    {
        EE_ASSERT(uiSemanticIndex < UV_MAX);
        bRequiresTransform = false;
    }
    else
    {
        // Unknown semantic
        return false;
    }

    return true;
}

//------------------------------------------------------------------------------------------------
bool NiDecorationBillBoardGenerator::GetVertexElementIsNormalized(
    const NiFixedString& kSemantic, NiUInt32& uiSemanticIndex,
    bool& bIsNormalized)
{
    EE_UNUSED_ARG(uiSemanticIndex);

    if (kSemantic == NiCommonSemantics::NORMAL())
    {
        EE_ASSERT(uiSemanticIndex == 0);
        bIsNormalized = true;
    }
    else if (kSemantic == NiCommonSemantics::POSITION())
    {
        EE_ASSERT(uiSemanticIndex == 0);
        bIsNormalized = false;
    }
    else if (kSemantic == NiCommonSemantics::TEXCOORD())
    {
        EE_ASSERT(uiSemanticIndex < UV_MAX);
        bIsNormalized = false;
    }
    else
    {
        // Unknown semantic
        return false;
    }

    return true;
}

//------------------------------------------------------------------------------------------------
const NiUInt32* NiDecorationBillBoardGenerator::GetIndexTemplateData(NiUInt32& uiCount)
{
    uiCount = GetIndicesPerMesh();
    return m_pkIndexTemplate;
}

//------------------------------------------------------------------------------------------------
void NiDecorationBillBoardGenerator::CreateStreamTemplates(
    NiUInt32 uiVerticesPerMesh, NiUInt32 uiIndicesPerMesh)
{
    EE_FREE(m_pkVertexPositionTemplate);
    EE_FREE(m_pkVertexNormalTemplate);
    EE_FREE(m_pkVertexTexCoordTemplate);
    EE_FREE(m_pkIndexTemplate);

    m_pkVertexPositionTemplate = EE_ALLOC(NiPoint3, uiVerticesPerMesh);
    m_pkVertexNormalTemplate = EE_ALLOC(NiPoint3, uiVerticesPerMesh);
    m_pkVertexTexCoordTemplate = EE_ALLOC(NiPoint2, uiVerticesPerMesh);
    m_pkIndexTemplate = EE_ALLOC(NiUInt32, uiIndicesPerMesh);

    PopulateStreamTemplates();
}

//------------------------------------------------------------------------------------------------
void NiDecorationBillBoardGenerator::PopulateStreamTemplates()
{
    EE_ASSERT(GetVerticesPerMesh() >= 5);

    float fHalfWidth = GetBillBoardWidth() * 0.5f;
    float fHeight = GetBillBoardHeight();
    m_pkVertexPositionTemplate[0] = NiPoint3(-fHalfWidth, 0.0f, fHeight);
    m_pkVertexPositionTemplate[1] = NiPoint3(fHalfWidth, 0.0f, fHeight);
    m_pkVertexPositionTemplate[2] = NiPoint3(0.0f, 0.0f, fHeight);
    m_pkVertexPositionTemplate[3] = NiPoint3(-fHalfWidth, 0.0f, 0.0f);
    m_pkVertexPositionTemplate[4] = NiPoint3(fHalfWidth, 0.0f, 0.0f);

    m_pkVertexNormalTemplate[0] = NiPoint3::UNIT_Z;
    m_pkVertexNormalTemplate[1] = NiPoint3::UNIT_Z;
    m_pkVertexNormalTemplate[2] = NiPoint3::UNIT_Z;
    m_pkVertexNormalTemplate[3] = NiPoint3::UNIT_Z;
    m_pkVertexNormalTemplate[4] = NiPoint3::UNIT_Z;

    m_pkVertexTexCoordTemplate[0] = NiPoint2(0.0f, 0.0f);
    m_pkVertexTexCoordTemplate[1] = NiPoint2(1.0f, 0.0f);
    m_pkVertexTexCoordTemplate[2] = NiPoint2(0.5f, 0.0f);
    m_pkVertexTexCoordTemplate[3] = NiPoint2(0.0f, 1.0f);
    m_pkVertexTexCoordTemplate[4] = NiPoint2(1.0f, 1.0f);

    EE_ASSERT(GetIndicesPerMesh() >= 9);
    m_pkIndexTemplate[0] = 0;
    m_pkIndexTemplate[1] = 2;
    m_pkIndexTemplate[2] = 3;
    m_pkIndexTemplate[3] = 2;
    m_pkIndexTemplate[4] = 4;
    m_pkIndexTemplate[5] = 3;
    m_pkIndexTemplate[6] = 2;
    m_pkIndexTemplate[7] = 1;
    m_pkIndexTemplate[8] = 4;
}

//------------------------------------------------------------------------------------------------
void NiDecorationBillBoardGenerator::UpdatePropertyData(NiDecorationMeshInfo* pkBase)
{
    NiMesh* pkBaseMesh = pkBase->GetMesh();
    EE_ASSERT(pkBaseMesh);

    // Stencil property to make each triangle double sided.
    NiStencilProperty* pkStencilProperty = 
        (NiStencilProperty*)pkBaseMesh->GetProperty(NiProperty::STENCIL);
    if (!pkStencilProperty)
    {
        pkStencilProperty = NiNew NiStencilProperty();
        pkStencilProperty->SetDrawMode(NiStencilProperty::DRAW_BOTH);
        pkBaseMesh->AttachProperty(pkStencilProperty);
    }

    // If one does not already exist, create and attach a texturing property
    // to display the texture defined in the 'Resource Path'
    NiTexturingProperty* pkTexturingProperty = 
        (NiTexturingProperty*)pkBaseMesh->GetProperty(NiProperty::TEXTURING);
    if (!pkTexturingProperty && m_spBaseTexture)
    {
        pkTexturingProperty = NiNew NiTexturingProperty();
        pkTexturingProperty->SetName(NiDecorationGenerator::DEFAULT_TEXTURING_PROPERTY_NAME);
        pkTexturingProperty->SetBaseTexture(m_spBaseTexture);
        pkTexturingProperty->SetBaseFilterMode(NiTexturingProperty::FILTER_TRILERP);
        pkTexturingProperty->SetBaseClampMode(NiTexturingProperty::CLAMP_S_CLAMP_T);
        pkTexturingProperty->SetBaseTextureIndex(UV_MODEL);
        pkBaseMesh->AttachProperty(pkTexturingProperty);
    }

    // Call the super function after we make sure a texturing property exists.
    NiDecorationGenerator::UpdatePropertyData(pkBase);
}

//------------------------------------------------------------------------------------------------
void NiDecorationBillBoardGenerator::UpdateAnimation(NiDecorationMeshInfo* pkBase, 
    NiUpdateProcess& kProcess)
{
    NiMesh* pkBaseMesh = pkBase->GetMesh();

    // Animation is only enabled when we are using instanced meshes.
    if (!GetUseInstancing(pkBaseMesh) || GetAnimationSwayMultiplier() == 0.0f)
    {
        return;
    }

    NiDataStreamElementLock kLock(pkBaseMesh, NiCommonSemantics::POSITION(), 0, 
        NiDataStreamElement::F_FLOAT32_3);

    // We can't animate the mesh.
    if (!kLock.IsLocked())
        return;

    NiTStridedRandomAccessIterator<NiPoint3> kPositions = kLock.begin<NiPoint3>();

    // Reset all the positions
    size_t stStreamSize;
    NiPoint3* pkPoints = (NiPoint3*)GetVertexElementTemplateData(stStreamSize, 
        NiDataStreamElement::F_FLOAT32_3, NiCommonSemantics::POSITION(), 0);
    NIASSERT(stStreamSize >= kLock.count(0) * sizeof(NiPoint3) && pkPoints != NULL);
    NiUInt32 uiNumVerts = GetVerticesPerMesh();
    for (NiUInt32 ui = 0; ui < uiNumVerts; ++ui)
        kPositions[ui] = pkPoints[ui];
    
    // Now animate some positions
    float fTime = (kProcess.GetTime() + GetAnimationOffsetTime()) / GetAnimationLoopTime();

    float fAngleMultiplier = 0.075f * GetAnimationSwayMultiplier();
    float fSinTwoPi = NiSin(fTime * NI_TWO_PI) * fAngleMultiplier;
    float fSinTwoPiOff = NiSin(fTime * NI_TWO_PI + 0.25f * NI_PI) * fAngleMultiplier;
    float fCosFourPi = NiCos(fTime * 4.0f * NI_PI) * fAngleMultiplier;
    float fCosThreePiOff = NiSin(fTime * 3.0f * NI_PI - 0.75f * NI_PI) * fAngleMultiplier;

    // Shift the upper outlying points. This keeps the bulk of the decoration
    // billboard still, improving the overall look.
    RotatePoint((fSinTwoPi + fCosFourPi) * 0.5f, fCosThreePiOff, kPositions[0]);
    RotatePoint(fCosFourPi, fSinTwoPiOff, kPositions[1]);

    kLock.Unlock();
}

//------------------------------------------------------------------------------------------------
void NiDecorationBillBoardGenerator::RotatePoint(
    float fAngleX, float fAngleY, NiPoint3& kPoint)
{
    NiQuaternion kQuat;
    kQuat.FromAngleAxesXYZ(fAngleX, fAngleY, 0.0f);

    NiMatrix3 kRot;
    kQuat.ToRotation(kRot);

    kPoint = kPoint * kRot;
}

//------------------------------------------------------------------------------------------------
