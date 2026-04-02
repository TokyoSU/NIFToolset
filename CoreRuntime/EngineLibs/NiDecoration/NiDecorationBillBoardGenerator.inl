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

//------------------------------------------------------------------------------------------------
inline float NiDecorationBillBoardGenerator::GetBillBoardWidth() const
{
    return m_fBillboardWidth;
}

//------------------------------------------------------------------------------------------------
inline float NiDecorationBillBoardGenerator::GetBillBoardHeight() const
{
    return m_fBillboardHeight;
}

//------------------------------------------------------------------------------------------------
inline void NiDecorationBillBoardGenerator::SetBillBoardWidth(float fWidth)
{
    m_fBillboardWidth = fWidth;
}

//------------------------------------------------------------------------------------------------
inline void NiDecorationBillBoardGenerator::SetBillBoardHeight(float fHeight)
{
    m_fBillboardHeight = fHeight;
}

//------------------------------------------------------------------------------------------------
inline void NiDecorationBillBoardGenerator::SetAnimationSwayMultiplier(float fMultiplier)
{
    m_fAnimationSwayMultipier = fMultiplier;
}

//------------------------------------------------------------------------------------------------
inline float NiDecorationBillBoardGenerator::GetAnimationSwayMultiplier() const
{
    return m_fAnimationSwayMultipier;
}

//------------------------------------------------------------------------------------------------
inline void NiDecorationBillBoardGenerator::SetBaseTexture(NiTexture* pkTexture)
{
    m_spBaseTexture = pkTexture;
}

//------------------------------------------------------------------------------------------------
inline NiTexture* NiDecorationBillBoardGenerator::GetBaseTexture()
{
    return m_spBaseTexture;
}

//------------------------------------------------------------------------------------------------
