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

#include "NiEnvironmentPCH.h"
#include "NiSkyDome.h"

#include <NiShaderFactory.h>
#include <NiSingleShaderMaterial.h>
#include <NiStencilProperty.h>
#include <NiZBufferProperty.h>

//---------------------------------------------------------------------------
NiImplementRTTI(NiSkyDome, NiSky);

//---------------------------------------------------------------------------
NiSkyDome::NiSkyDome()
{  
    // Initialise the settings to their defaults:
    LoadDefaultConfiguration();
}

//---------------------------------------------------------------------------
NiSkyDome::~NiSkyDome()
{    
}

//---------------------------------------------------------------------------
float NiSkyDome::HorizonBiasFunction(float fValue, float fBias)
{
    float fResult = 1.0f - NiPow((1.0f - fValue), 3);
    return NiLerp(fBias, fValue, fResult);
}

//---------------------------------------------------------------------------
NiMesh* NiSkyDome::GenerateSkyDome(float fDomeRadius, float fDetail, 
    float fAxisDetailBias, float fHorizonBias)
{
    // Default parameters
    NiUInt32 uiDefaultNumSegments = 10;

    // Make sure parameters are valid:
    EE_ASSERT(fDetail >= 0);

    // Calculated variables
    uiDefaultNumSegments *= NiUInt32(2.0f * fDetail);
    NiUInt32 uiNumSegmentsX = NiUInt32(uiDefaultNumSegments * (1.0f - fAxisDetailBias));
    NiUInt32 uiNumSegmentsY = NiUInt32(uiDefaultNumSegments * fAxisDetailBias);
    
    // Make sure calculations are within limits
    uiNumSegmentsX = efd::Max<NiUInt32>(uiNumSegmentsX, 4);
    uiNumSegmentsY = efd::Max<NiUInt32>(uiNumSegmentsY, 1);
    NiUInt32 uiNumVerts = uiNumSegmentsX * uiNumSegmentsY * 4;

    // Generate Mesh
    NiMesh* pkMesh = NiNew NiMesh();
    pkMesh->SetPrimitiveType(NiPrimitiveType::PRIMITIVE_TRISTRIPS);
    pkMesh->SetSubmeshCount(1);
    pkMesh->SetTranslate(0.0f, 0.0f, 0.0f);

    NiDataStreamElementLock kPositions = pkMesh->AddStreamGetLock(
        NiCommonSemantics::POSITION(), 0, NiDataStreamElement::F_FLOAT32_3, 
        uiNumVerts, NiDataStream::ACCESS_GPU_READ | 
        NiDataStream::ACCESS_CPU_READ | NiDataStream::ACCESS_CPU_WRITE_STATIC, 
        NiDataStream::USAGE_VERTEX);
    NiDataStreamElementLock kNormals = pkMesh->AddStreamGetLock(
        NiCommonSemantics::NORMAL(), 0, NiDataStreamElement::F_FLOAT32_3, 
        uiNumVerts, NiDataStream::ACCESS_GPU_READ | 
        NiDataStream::ACCESS_CPU_READ | NiDataStream::ACCESS_CPU_WRITE_STATIC, 
        NiDataStream::USAGE_VERTEX);

    NiTStridedRandomAccessIterator<NiPoint3> kPosIter = 
        kPositions.begin<NiPoint3>();
    NiTStridedRandomAccessIterator<NiPoint3> kNormIter = 
        kNormals.begin<NiPoint3>();

    // Generate the sky dome such that rendering the streams will produce a 
    // continuous triangle strip.
    NiUInt32 uiCurrVertex = 0;
    for (NiUInt32 uiSegmentY = 0; uiSegmentY < uiNumSegmentsY; ++uiSegmentY)
    {
        float fPhiRads = NiLerp(
            HorizonBiasFunction(
                float(uiSegmentY) / float(uiNumSegmentsY), fHorizonBias), 
            0, NI_PI / 2.0f);
        float fDeltaPhiRads = NiLerp(
            HorizonBiasFunction(
                float(uiSegmentY + 1) / float(uiNumSegmentsY), fHorizonBias), 
            0, NI_PI / 2.0f);

        for (NiUInt32 uiSegmentX = 0; uiSegmentX < uiNumSegmentsX; ++uiSegmentX)
        {
            float fThetaRads = NiLerp(
                float(uiSegmentX) / float(uiNumSegmentsX), 0, NI_PI * 2.0f);
            float fDeltaThetaRads = NiLerp(
                float(uiSegmentX + 1) / float(uiNumSegmentsX), 0, NI_PI * 2.0f);

            float fSinTheta;
            float fCosTheta;
            float fSinPhi;
            float fCosPhi;  

            float fSinDeltaTheta;
            float fCosDeltaTheta;
            float fSinDeltaPhi;
            float fCosDeltaPhi;

            NiSinCos(fThetaRads, fSinTheta, fCosTheta);
            NiSinCos(fPhiRads, fSinPhi, fCosPhi);
            NiSinCos(fDeltaThetaRads, fSinDeltaTheta, fCosDeltaTheta);
            NiSinCos(fDeltaPhiRads, fSinDeltaPhi, fCosDeltaPhi);
            
            NiPoint3 kVertex;
            NiPoint2 kTexCoord;
            NiPoint2 kScatterValues;
            
            // ------------ VERT 1 --------------
            kVertex =  NiPoint3(
                fSinPhi * fCosTheta,
                fSinPhi * fSinTheta, 
                fCosPhi);
            kPosIter[uiCurrVertex] = kVertex * fDomeRadius;            
            kNormIter[uiCurrVertex] = -kVertex;

            uiCurrVertex++;
            
            // ------------ VERT 2 --------------
            kVertex = NiPoint3(
                fSinDeltaPhi * fCosTheta,
                fSinDeltaPhi * fSinTheta,
                fCosDeltaPhi);
            kPosIter[uiCurrVertex] = kVertex * fDomeRadius;
            kNormIter[uiCurrVertex] = -kVertex;

            uiCurrVertex++;

            // ------------ VERT 3 --------------
            kVertex = NiPoint3(
                fSinPhi * fCosDeltaTheta,
                fSinPhi * fSinDeltaTheta,
                fCosPhi);
            kPosIter[uiCurrVertex] = kVertex * fDomeRadius;
            kNormIter[uiCurrVertex] = -kVertex;

            uiCurrVertex++;

            // ------------ VERT 4 --------------
            kVertex = NiPoint3(
                fSinDeltaPhi * fCosDeltaTheta,
                fSinDeltaPhi * fSinDeltaTheta,
                fCosDeltaPhi);
            kPosIter[uiCurrVertex] = kVertex * fDomeRadius;
            kNormIter[uiCurrVertex] = -kVertex;

            uiCurrVertex++;
        }
    }
   
    pkMesh->SetName("SkyDome");
    pkMesh->RecomputeBounds();

    // --- Attach the appropriate properties:
    NiStencilProperty* pkStencilProp = NiNew NiStencilProperty();
    pkStencilProp->SetDrawMode(NiStencilProperty::DRAW_CW);
    pkMesh->AttachProperty(pkStencilProp);

    NiZBufferProperty* pkZProp = NiNew NiZBufferProperty();
    pkZProp->SetZBufferWrite(false);    
    pkZProp->SetZBufferTest(false);    
    pkZProp->SetTestFunction(NiZBufferProperty::TEST_LESSEQUAL);
    pkMesh->AttachProperty(pkZProp);

    pkMesh->UpdateProperties();

    return pkMesh;
}

//---------------------------------------------------------------------------
void NiSkyDome::LoadDefaultConfiguration()
{
    // Initialise NiSky object
    NiSky::LoadDefaultConfiguration();
    m_bGeometrySettingsChanged = true;

    // How far away from the camera the horizon is. Recommend a large
    // number to represent infinity. NOTE: the default skydome material
    // destroys the depth information of the dome so far clipping and
    // depth culling are not an issue.
    m_fDomeRadius = 50000.0f;

    // Directly adjusts the number of verticies that are used in the 
    // generation of the skydome. Higher numbers increase detail, lower
    // numbers decrease detail. A value of 1 is a general compromise between
    // quality and speed.
    m_fDomeDetail = 1.0f;

    // Adjust the distribution of verticies on the Rotational and Pitch axis. 
    // a value closer to 0 causes the number used on the rotational axis to
    // be increased and the pitch axis to be decreased. a value closer to 1 
    // is the opposite, and a value of 0.5 is an even distribution.
    m_fDomeDetailAxisBias = 0.6f;

    // Adjust the distribution of verticies on the pitch axis between a
    // linear distribution (a value of 0) and a power of 3 distribution
    // concentrated at the horixon (value of 1). Higher detail at the horizon
    // is prefered during sunsets and when the camera never looks directly up.
    m_fDomeDetailHorizonBias = 0.9f;
}

//---------------------------------------------------------------------------
void NiSkyDome::UpdateGeometry()
{
    // Regenerate Skydome 
    if (m_spGeometry)
        DetachChild(m_spGeometry);

    // Generate a new skydome for use
    m_spGeometry = GenerateSkyDome(m_fDomeRadius, m_fDomeDetail, 
        m_fDomeDetailAxisBias, m_fDomeDetailHorizonBias);
    EE_ASSERT(m_spGeometry);

    AttachExtraData(m_spGeometry);
    
    m_spGeometry->ApplyAndSetActiveMaterial(m_spSkyMaterial);
    
    // Attach the skydome and update it appropriately  
    AttachChild(m_spGeometry);
    m_spGeometry->UpdateProperties();
    m_spGeometry->UpdateEffects();
    m_spGeometry->Update(0.0f);
}

//---------------------------------------------------------------------------
