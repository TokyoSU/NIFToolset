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
#include "NiD3D10RendererPCH.h"

#include "NiD3D10Utility.h"

#include <NiTransform.h>

#if defined(_WIN64) || defined(WIN64)
#include <xmmintrin.h>
#endif

//--------------------------------------------------------------------------------------------------
void NiD3D10Utility::GetMatrixFromNi(NiBgfxMath::Mat4& kMatrix, const NiTransform& kNi)
{
    GetMatrixFromNi(kMatrix, kNi.m_Rotate, kNi.m_Translate, kNi.m_fScale);
}

//--------------------------------------------------------------------------------------------------
void NiD3D10Utility::GetMatrixFromNi(NiBgfxMath::Mat4& kMatrix, const NiMatrix3& kNiRot,
    const NiPoint3& kNiTrans, float fNiScale)
{
    kMatrix[0] = kNiRot.GetEntry(0, 0) * fNiScale;
    kMatrix[1] = kNiRot.GetEntry(1, 0) * fNiScale;
    kMatrix[2] = kNiRot.GetEntry(2, 0) * fNiScale;
    kMatrix[3] = 0.0f;
    kMatrix[4] = kNiRot.GetEntry(0, 1) * fNiScale;
    kMatrix[5] = kNiRot.GetEntry(1, 1) * fNiScale;
    kMatrix[6] = kNiRot.GetEntry(2, 1) * fNiScale;
    kMatrix[7] = 0.0f;
    kMatrix[8] = kNiRot.GetEntry(0, 2) * fNiScale;
    kMatrix[9] = kNiRot.GetEntry(1, 2) * fNiScale;
    kMatrix[10] = kNiRot.GetEntry(2, 2) * fNiScale;
    kMatrix[11] = 0.0f;
    kMatrix[12] = kNiTrans.x;
    kMatrix[13] = kNiTrans.y;
    kMatrix[14] = kNiTrans.z;
    kMatrix[15] = 1.0f;
}

//--------------------------------------------------------------------------------------------------
void NiD3D10Utility::GetTransposeMatrixFromNi(NiBgfxMath::Mat4& kMatrix,
    const NiTransform& kNi)
{
    GetTransposeMatrixFromNi(kMatrix, kNi.m_Rotate, kNi.m_Translate, kNi.m_fScale);
}

//--------------------------------------------------------------------------------------------------
void NiD3D10Utility::GetTransposeMatrixFromNi(NiBgfxMath::Mat4& kMatrix,
    const NiMatrix3& kNiRot, const NiPoint3& kNiTrans, float fNiScale)
{
    kMatrix[0] = kNiRot.GetEntry(0, 0) * fNiScale;
    kMatrix[1] = kNiRot.GetEntry(0, 1) * fNiScale;
    kMatrix[2] = kNiRot.GetEntry(0, 2) * fNiScale;
    kMatrix[3] = kNiTrans.x;
    kMatrix[4] = kNiRot.GetEntry(1, 0) * fNiScale;
    kMatrix[5] = kNiRot.GetEntry(1, 1) * fNiScale;
    kMatrix[6] = kNiRot.GetEntry(1, 2) * fNiScale;
    kMatrix[7] = kNiTrans.y;
    kMatrix[8] = kNiRot.GetEntry(2, 0) * fNiScale;
    kMatrix[9] = kNiRot.GetEntry(2, 1) * fNiScale;
    kMatrix[10] = kNiRot.GetEntry(2, 2) * fNiScale;
    kMatrix[11] = kNiTrans.z;
    kMatrix[12] = 0.0f;
    kMatrix[13] = 0.0f;
    kMatrix[14] = 0.0f;
    kMatrix[15] = 1.0f;
}

//--------------------------------------------------------------------------------------------------
int NiD3D10Utility::FastFloatToInt(float fValue)
{
#if defined(_WIN64) || defined(WIN64)
    return _mm_cvt_ss2si(_mm_set_ss(fValue));
#else
    int iValue;

    _asm fld fValue
    _asm fistp iValue

    return iValue;
#endif
}

//--------------------------------------------------------------------------------------------------
