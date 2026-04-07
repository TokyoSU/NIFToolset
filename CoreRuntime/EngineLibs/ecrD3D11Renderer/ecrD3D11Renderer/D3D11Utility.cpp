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
#include "ecrD3D11RendererPCH.h"

#include "D3D11Utility.h"

#include <NiTransform.h>

using namespace ecr;

//------------------------------------------------------------------------------------------------
void D3D11Utility::GetD3DFromNi(XMMATRIX& xnaMatrix, const NiTransform& niTranform)
{
    GetD3DFromNi(xnaMatrix, niTranform.m_Rotate, niTranform.m_Translate, niTranform.m_fScale);
}

//------------------------------------------------------------------------------------------------
void D3D11Utility::GetD3DFromNi(
    XMMATRIX& xnaMatrix, 
    const efd::Matrix3& niRotation,
    const efd::Point3& niTranslation, 
    float niScale)
{
    XMFLOAT4X4 m;
    m._11 = niRotation.GetEntry(0, 0) * niScale;
    m._12 = niRotation.GetEntry(1, 0) * niScale;
    m._13 = niRotation.GetEntry(2, 0) * niScale;
    m._14 = 0.0f;
    m._21 = niRotation.GetEntry(0, 1) * niScale;
    m._22 = niRotation.GetEntry(1, 1) * niScale;
    m._23 = niRotation.GetEntry(2, 1) * niScale;
    m._24 = 0.0f;
    m._31 = niRotation.GetEntry(0, 2) * niScale;
    m._32 = niRotation.GetEntry(1, 2) * niScale;
    m._33 = niRotation.GetEntry(2, 2) * niScale;
    m._34 = 0.0f;
    m._41 = niTranslation.x;
    m._42 = niTranslation.y;
    m._43 = niTranslation.z;
    m._44 = 1.0f;
    xnaMatrix = XMLoadFloat4x4(&m);
}

//------------------------------------------------------------------------------------------------
void D3D11Utility::GetD3DTransposeFromNi(
    XMMATRIX& xnaMatrix,
    const NiTransform& niTranform)
{
    GetD3DTransposeFromNi(
        xnaMatrix, 
        niTranform.m_Rotate, 
        niTranform.m_Translate, 
        niTranform.m_fScale);
}

//------------------------------------------------------------------------------------------------
void D3D11Utility::GetD3DTransposeFromNi(
    XMMATRIX& xnaMatrix,
    const efd::Matrix3& niRotation, 
    const efd::Point3& niTranslation, 
    float niScale)
{
    XMFLOAT4X4 m;
    m._11 = niRotation.GetEntry(0, 0) * niScale;
    m._12 = niRotation.GetEntry(0, 1) * niScale;
    m._13 = niRotation.GetEntry(0, 2) * niScale;
    m._14 = niTranslation.x;
    m._21 = niRotation.GetEntry(1, 0) * niScale;
    m._22 = niRotation.GetEntry(1, 1) * niScale;
    m._23 = niRotation.GetEntry(1, 2) * niScale;
    m._24 = niTranslation.y;
    m._31 = niRotation.GetEntry(2, 0) * niScale;
    m._32 = niRotation.GetEntry(2, 1) * niScale;
    m._33 = niRotation.GetEntry(2, 2) * niScale;
    m._34 = niTranslation.z;
    m._41 = 0.0f;
    m._42 = 0.0f;
    m._43 = 0.0f;
    m._44 = 1.0f;
    xnaMatrix = XMLoadFloat4x4(&m);
}

//------------------------------------------------------------------------------------------------
