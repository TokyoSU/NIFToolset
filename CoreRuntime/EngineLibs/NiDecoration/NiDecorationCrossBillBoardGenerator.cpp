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
#include "NiDecorationCrossBillboardGenerator.h"

NiImplementRTTI(NiDecorationCrossBillBoardGenerator, NiDecorationBillBoardGenerator, NiTypeMask::NiDecorationCrossBillBoardGenerator);

//------------------------------------------------------------------------------------------------
NiFixedString NiDecorationCrossBillBoardGenerator::GENERATOR_NAME = NULL;

//------------------------------------------------------------------------------------------------
NiDecorationCrossBillBoardGenerator::NiDecorationCrossBillBoardGenerator(bool bUseInstancing, 
    float fBillboardWidth, float fBillboardHeight,
    NiUInt32 uiVerticesPerMesh, NiUInt32 uiIndicesPerMesh) :
    NiDecorationBillBoardGenerator(bUseInstancing, 
        fBillboardWidth, fBillboardHeight, uiVerticesPerMesh, uiIndicesPerMesh)
{
}

//------------------------------------------------------------------------------------------------
NiDecorationCrossBillBoardGenerator::~NiDecorationCrossBillBoardGenerator()
{
}

//------------------------------------------------------------------------------------------------
const NiFixedString& NiDecorationCrossBillBoardGenerator::GetGeneratorType()
{
    return GENERATOR_NAME;
}

//------------------------------------------------------------------------------------------------
void NiDecorationCrossBillBoardGenerator::_SDMInit()
{
    GENERATOR_NAME = "Cross Billboard Generator";
}

//------------------------------------------------------------------------------------------------
void NiDecorationCrossBillBoardGenerator::_SDMShutdown()
{
    GENERATOR_NAME = NULL;
}

//------------------------------------------------------------------------------------------------
void NiDecorationCrossBillBoardGenerator::PopulateStreamTemplates()
{
    EE_ASSERT(GetVerticesPerMesh() >= 9);

    float fHalfWidth = GetBillBoardWidth() * 0.5f;
    float fHeight = GetBillBoardHeight();
    m_pkVertexPositionTemplate[0] = NiPoint3(-fHalfWidth, 0.0f, fHeight);
    m_pkVertexPositionTemplate[1] = NiPoint3(0.0f, -fHalfWidth, fHeight);
    m_pkVertexPositionTemplate[2] = NiPoint3(fHalfWidth, 0.0f, fHeight);
    m_pkVertexPositionTemplate[3] = NiPoint3(0.0f, fHalfWidth, fHeight);
    m_pkVertexPositionTemplate[4] = NiPoint3(0.0f, 0.0f, fHeight);
    m_pkVertexPositionTemplate[5] = NiPoint3(-fHalfWidth, 0.0f, 0.0f);
    m_pkVertexPositionTemplate[6] = NiPoint3(0.0f, -fHalfWidth, 0.0f);
    m_pkVertexPositionTemplate[7] = NiPoint3(fHalfWidth, 0.0f, 0.0f);
    m_pkVertexPositionTemplate[8] = NiPoint3(0.0f, fHalfWidth, 0.0f);

    m_pkVertexNormalTemplate[0] = NiPoint3::UNIT_Z;
    m_pkVertexNormalTemplate[1] = NiPoint3::UNIT_Z;
    m_pkVertexNormalTemplate[2] = NiPoint3::UNIT_Z;
    m_pkVertexNormalTemplate[3] = NiPoint3::UNIT_Z;
    m_pkVertexNormalTemplate[4] = NiPoint3::UNIT_Z;
    m_pkVertexNormalTemplate[5] = NiPoint3::UNIT_Z;
    m_pkVertexNormalTemplate[6] = NiPoint3::UNIT_Z;
    m_pkVertexNormalTemplate[7] = NiPoint3::UNIT_Z;
    m_pkVertexNormalTemplate[8] = NiPoint3::UNIT_Z;

    m_pkVertexTexCoordTemplate[0] = NiPoint2(0.0f, 0.0f);
    m_pkVertexTexCoordTemplate[1] = NiPoint2(0.0f, 0.0f);
    m_pkVertexTexCoordTemplate[2] = NiPoint2(1.0f, 0.0f);
    m_pkVertexTexCoordTemplate[3] = NiPoint2(1.0f, 0.0f);
    m_pkVertexTexCoordTemplate[4] = NiPoint2(0.5f, 0.0f);
    m_pkVertexTexCoordTemplate[5] = NiPoint2(0.0f, 1.0f);
    m_pkVertexTexCoordTemplate[6] = NiPoint2(0.0f, 1.0f);
    m_pkVertexTexCoordTemplate[7] = NiPoint2(1.0f, 1.0f);
    m_pkVertexTexCoordTemplate[8] = NiPoint2(1.0f, 1.0f);

    EE_ASSERT(GetIndicesPerMesh() >= 18);
    m_pkIndexTemplate[0] = 0;
    m_pkIndexTemplate[1] = 4;
    m_pkIndexTemplate[2] = 5;
    m_pkIndexTemplate[3] = 2;
    m_pkIndexTemplate[4] = 7;
    m_pkIndexTemplate[5] = 4;
    m_pkIndexTemplate[6] = 4;
    m_pkIndexTemplate[7] = 7;
    m_pkIndexTemplate[8] = 5;
    m_pkIndexTemplate[9] = 1;
    m_pkIndexTemplate[10] = 4;
    m_pkIndexTemplate[11] = 6;
    m_pkIndexTemplate[12] = 4;
    m_pkIndexTemplate[13] = 3;
    m_pkIndexTemplate[14] = 8;
    m_pkIndexTemplate[15] = 4;
    m_pkIndexTemplate[16] = 8;
    m_pkIndexTemplate[17] = 6;
}

//------------------------------------------------------------------------------------------------
void NiDecorationCrossBillBoardGenerator::UpdateAnimation(
    NiDecorationMeshInfo* pkBase, NiUpdateProcess& kProcess)
{
    NiMesh* pkBaseMesh = pkBase->GetMesh();

    // Animation is only enabled when we are using instanced meshes.
    if (!GetUseInstancing(pkBaseMesh) || GetAnimationSwayMultiplier() == 0.0f)
    {
        return;
    }

    EE_ASSERT(NiIsExactKindOf(NiMesh, pkBaseMesh));

    NiDataStreamElementLock kLock((NiMesh*)pkBaseMesh, NiCommonSemantics::POSITION(), 0, 
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
    RotatePoint((fSinTwoPi + fCosFourPi) * 0.5f, 0.0f, kPositions[1]);
    RotatePoint(fCosFourPi, fSinTwoPiOff, kPositions[3]);

    RotatePoint((fCosThreePiOff + fSinTwoPi) * 0.5f, fSinTwoPiOff, kPositions[0]);
    RotatePoint(fCosFourPi, (fSinTwoPi + fCosThreePiOff) * 0.5f, kPositions[2]);

    kLock.Unlock();
}

//------------------------------------------------------------------------------------------------