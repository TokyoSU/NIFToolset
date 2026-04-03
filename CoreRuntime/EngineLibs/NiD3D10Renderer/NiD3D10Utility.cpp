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
void NiD3D10Utility::GetD3DFromNi(D3DXMATRIX& kD3D, const NiTransform& kNi)
{
    GetD3DFromNi(kD3D, kNi.m_Rotate, kNi.m_Translate, kNi.m_fScale);
}

//--------------------------------------------------------------------------------------------------
void NiD3D10Utility::GetD3DFromNi(D3DXMATRIX& kD3D, const NiMatrix3& kNiRot,
    const NiPoint3& kNiTrans, float fNiScale)
{
    kD3D._11 = kNiRot.GetEntry(0, 0) * fNiScale;
    kD3D._12 = kNiRot.GetEntry(1, 0) * fNiScale;
    kD3D._13 = kNiRot.GetEntry(2, 0) * fNiScale;
    kD3D._14 = 0.0f;
    kD3D._21 = kNiRot.GetEntry(0, 1) * fNiScale;
    kD3D._22 = kNiRot.GetEntry(1, 1) * fNiScale;
    kD3D._23 = kNiRot.GetEntry(2, 1) * fNiScale;
    kD3D._24 = 0.0f;
    kD3D._31 = kNiRot.GetEntry(0, 2) * fNiScale;
    kD3D._32 = kNiRot.GetEntry(1, 2) * fNiScale;
    kD3D._33 = kNiRot.GetEntry(2, 2) * fNiScale;
    kD3D._34 = 0.0f;
    kD3D._41 = kNiTrans.x;
    kD3D._42 = kNiTrans.y;
    kD3D._43 = kNiTrans.z;
    kD3D._44 = 1.0f;
}

//--------------------------------------------------------------------------------------------------
void NiD3D10Utility::GetD3DTransposeFromNi(D3DXMATRIX& kD3D,
    const NiTransform& kNi)
{
    GetD3DTransposeFromNi(kD3D, kNi.m_Rotate, kNi.m_Translate, kNi.m_fScale);
}

//--------------------------------------------------------------------------------------------------
void NiD3D10Utility::GetD3DTransposeFromNi(D3DXMATRIX& kD3D,
    const NiMatrix3& kNiRot, const NiPoint3& kNiTrans, float fNiScale)
{
    kD3D._11 = kNiRot.GetEntry(0, 0) * fNiScale;
    kD3D._12 = kNiRot.GetEntry(0, 1) * fNiScale;
    kD3D._13 = kNiRot.GetEntry(0, 2) * fNiScale;
    kD3D._14 = kNiTrans.x;
    kD3D._21 = kNiRot.GetEntry(1, 0) * fNiScale;
    kD3D._22 = kNiRot.GetEntry(1, 1) * fNiScale;
    kD3D._23 = kNiRot.GetEntry(1, 2) * fNiScale;
    kD3D._24 = kNiTrans.y;
    kD3D._31 = kNiRot.GetEntry(2, 0) * fNiScale;
    kD3D._32 = kNiRot.GetEntry(2, 1) * fNiScale;
    kD3D._33 = kNiRot.GetEntry(2, 2) * fNiScale;
    kD3D._34 = kNiTrans.z;
    kD3D._41 = 0.0f;
    kD3D._42 = 0.0f;
    kD3D._43 = 0.0f;
    kD3D._44 = 1.0f;
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
