// EMERGENT GAME TECHNOLOGIES PROPRIETARY INFORMATION
//
// This software is supplied under the terms of a license agreement or
// nondisclosure agreement with Emergent Game Technologies and may not 
// be copied or disclosed except in accordance with the terms of that 
// agreement.
//
//      Copyright (c) 1996-2008 Emergent Game Technologies.
//      All Rights Reserved.
//
// Emergent Game Technologies, Chapel Hill, North Carolina 27517
// http://www.emergent.net

// Precompiled Header
#include <NiParticlePCH.h>

#include "NiPSAlignedQuadTextureKernel.h"
#include <NiUniversalTypes.h>
#include <NiMath.h>
#include <NiPSSimulatorKernelHelpers.h>
#include <NiPoint2.h>

//---------------------------------------------------------------------------
NiSPBeginKernelImpl(NiPSAlignedQuadTextureKernel)
{
    // Get input streams.
    const NiPSAlignedQuadTextureKernelStruct* pkIStruct =
        kWorkload.GetInput<NiPSAlignedQuadTextureKernelStruct>(0);
    // age and lifetime
    const float* pfIAges = kWorkload.GetInput<float>(1);
    const float* pfILifeTimes = kWorkload.GetInput<float>(2);
    const float* pfIVariance = kWorkload.GetInput<float>(3);
    const NiUInt32* pRemap = kWorkload.GetInput<NiUInt32>(4);
    
    // Get output streams.
    // Texture coords 
    NiPoint2* pkOTexCoords = kWorkload.GetOutput<NiPoint2>(0);
    
    // Get block count.
    NiUInt32 uiBlockCount = kWorkload.GetBlockCount();

    // Cache local variables
    bool bPingPong = pkIStruct->m_bPingPong;
    bool bUVScrolling = pkIStruct->m_bUVScrolling;
    float fNumFramesAcross = (float)pkIStruct->m_uiNumFramesAcross;
    float fNormInitialTime = pkIStruct->m_fInitialTime;
    float fNormFinalTime = pkIStruct->m_fFinalTime;
    float fInitialFrame = (float)pkIStruct->m_uiInitialFrame;
    float fInitialFrameVar = pkIStruct->m_fInitialFrameVar;
    float fNumFrames = (float)pkIStruct->m_uiNumFrames;
    float fNumFramesVar = pkIStruct->m_fNumFramesVar;

    // Calculate fDeltaU change in texture coordinate from left to right
    float fDeltaU = 1.0f/fNumFramesAcross;

    // Calculate fDeltaV change in texture coordinate from bottom to top
    float fDeltaV = 1.0f/pkIStruct->m_uiNumFramesDown;

    // The bottom, left texture coordinate
    float fBottom = 0.0f;
    float fLeft = 0.0f;
   
    // Generate texture coords. 
    for (NiUInt32 ui = 0; ui < uiBlockCount; ++ui)
    {
        NiUInt32 index = (pRemap != NULL) ? pRemap[ui] : ui;
        float fAge = pfIAges[index];
        float fLife = pfILifeTimes[index];
        float fVariance = pfIVariance[index];
        float fInitialTime = fNormInitialTime * fLife;
        float fFinalTime = fNormFinalTime * fLife;
        float fNormTime = 0.0f;

        // Given an age, lifetime and loop behavior, get scaled age 
        float fScaledAge;
        if (!bPingPong)
        {
            fScaledAge = 
                (fAge < fInitialTime) ? fInitialTime : (fAge > fFinalTime) ? fFinalTime : fAge;
        }
        else
        {
            float fKeyDiff = fFinalTime - fInitialTime;
            if (fKeyDiff > NIPSKERNEL_EPSILON)
            {
                float fRatio = fAge / fKeyDiff;
                float fWhole = NiFloor(fRatio);
                NiUInt32 uiReverse = (NiUInt32)fWhole % 2;
                if (uiReverse > 0)
                    fScaledAge = fFinalTime - (fAge - (fFinalTime * fWhole));
                else
                    fScaledAge = (fAge - (fFinalTime * fWhole));
            }
            else
            {
                fScaledAge = fAge;
            }
        }

        // Compute normalized time between the keys.
        fNormTime = (fScaledAge - fInitialTime) / (fFinalTime - fInitialTime);

        // Convert linear position to grid (x,y) position  
        // y = floor (linear pos / frames down)
        // x = (remainder of (linear pos / frames across))* frames across 
        if (!bUVScrolling)
        {
            // Calculate the current frame giving the variance
            float fCurrentInitialFrame = fInitialFrame + NiFloor(fVariance * fInitialFrameVar);
            float fCurrentFinalFrame = 
                fCurrentInitialFrame + (fNumFrames + NiFloor(fVariance * fNumFramesVar));

            // Linear interpolation only.
            float fLinearPos = 
                (1.0f - fNormTime) * fCurrentInitialFrame + fNormTime * fCurrentFinalFrame;

            fBottom = NiFloor(fLinearPos * fDeltaU);
            fLeft = (fLinearPos * fDeltaU) - fBottom;
            fLeft *= fNumFramesAcross;
            fLeft = NiFloor(fLeft);
        }
        else
        {
            // Calculate the current frame giving the variance
            float fCurrentInitialFrame = fInitialFrame + (fVariance * fInitialFrameVar);
            float fCurrentFinalFrame = 
                fCurrentInitialFrame + (fNumFrames + (fVariance * fNumFramesVar));

            // Linear interpolation only.
            float fLinearPos = 
                (1.0f - fNormTime) * fCurrentInitialFrame + fNormTime * fCurrentFinalFrame;

            fBottom = NiFloor(fLinearPos * fDeltaU);
            fLeft = (fLinearPos * fDeltaU) - fBottom;
            fLeft *= fNumFramesAcross;
        }

        // Convert to a UV value
        fLeft *= fDeltaU;
        fBottom *= fDeltaV;

        // 1--2
        // |  |
        // 0--3

        // Put the texture coordinates in place

        pkOTexCoords->x = fLeft;
        pkOTexCoords->y = fBottom;
        ++pkOTexCoords;

        pkOTexCoords->x = fLeft;
        pkOTexCoords->y = fBottom + fDeltaV;
        ++pkOTexCoords;

        pkOTexCoords->x = fLeft + fDeltaU;
        pkOTexCoords->y = fBottom + fDeltaV;
        ++pkOTexCoords;

        pkOTexCoords->x = fLeft + fDeltaU; 
        pkOTexCoords->y = fBottom;
        ++pkOTexCoords;

    }
}
NiSPEndKernelImpl(NiPSAlignedQuadTextureKernel)
//---------------------------------------------------------------------------
