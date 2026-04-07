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

//---------------------------------------------------------------------------
inline float NiTerrainDecorationFunctor::BiLerp(
    const NiPoint2& kInterpolant, const NiPoint2& kInvInterpolant, 
    float fBottomLeft, float fBottomRight, float fTopLeft, float fTopRight)
{
    return
        (fBottomLeft * kInvInterpolant.x * kInvInterpolant.y) +
        (fBottomRight * kInterpolant.x * kInvInterpolant.y) +
        (fTopLeft * kInvInterpolant.x * kInterpolant.y) +
        (fTopRight * kInterpolant.x * kInterpolant.y);
}
//---------------------------------------------------------------------------
inline void NiTerrainDecorationFunctor::FactoredBiLerp(
    const NiPoint2& kInterpolant, const float* fFactors, 
    const float* fLeftColumn, const float* fRightColumn,
    float& fResult, NiPoint2& kVortexPosition)
{
    NiPoint2 kRange;

    // A value of 0.0 actually equates to 1.0 on the previous
    // voxel.
    if (fFactors[X_CURRENT] == 0.0f)
        kRange.x = 1.0f - fFactors[X_PREVIOUS];
    else
        kRange.x = fFactors[X_CURRENT] - fFactors[X_PREVIOUS];

    if (fFactors[Y_TOP] == 0.0f)
        kRange.y = 1.0f - fFactors[Y_BOTTOM];
    else
        kRange.y = fFactors[Y_TOP] - fFactors[Y_BOTTOM];

    kVortexPosition.x = NiLerp(kInterpolant.x, fFactors[X_PREVIOUS], 
        fFactors[X_PREVIOUS] + kRange.x);
    kVortexPosition.y = NiLerp(kInterpolant.y, fFactors[Y_BOTTOM], 
        fFactors[Y_BOTTOM] + kRange.y);

    NiPoint2 kInvVortexPosition(
        1.0f - kVortexPosition.x,
        1.0f - kVortexPosition.y);

    fResult = BiLerp(
        kVortexPosition, kInvVortexPosition,
        fLeftColumn[P_BOTTOM], fRightColumn[P_BOTTOM],
        fLeftColumn[P_TOP], fRightColumn[P_TOP]);
}
//---------------------------------------------------------------------------