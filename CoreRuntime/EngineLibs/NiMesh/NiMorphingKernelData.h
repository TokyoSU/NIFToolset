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

#pragma once
#ifndef NIMORPHINGKERNELDATA_H
#define NIMORPHINGKERNELDATA_H

enum EMorphingKernelDataFlags
{
    FLAG_POINTS_16    = 0x0001,
    FLAG_POINTS_32    = 0x0002,
    FLAG_TARGETS_16   = 0x0004,
    FLAG_TARGETS_32   = 0x0008,
    FLAG_NORMALIZE    = 0x0010,
};
// MKDC + Morph weights format + Points format
enum EMorphingKernelDataCombination
{
    MKDC_INVALID       = 0x0000,

    MKDC_16_16 = FLAG_POINTS_16 | FLAG_TARGETS_16,
    MKDC_32_16 = FLAG_POINTS_32 | FLAG_TARGETS_16,
    MKDC_16_32 = FLAG_POINTS_16 | FLAG_TARGETS_32,
    MKDC_32_32 = FLAG_POINTS_32 | FLAG_TARGETS_32,
    MKDC_16_16_N = FLAG_POINTS_16 | FLAG_TARGETS_16 | FLAG_NORMALIZE,
    MKDC_32_16_N = FLAG_POINTS_32 | FLAG_TARGETS_16 | FLAG_NORMALIZE,
    MKDC_16_32_N = FLAG_POINTS_16 | FLAG_TARGETS_32 | FLAG_NORMALIZE,
    MKDC_32_32_N = FLAG_POINTS_32 | FLAG_TARGETS_32 | FLAG_NORMALIZE,
};

/**
    A structure that is used to store the make up of the input streams
    for this morphing modifier.

    This information is needed for the morphing kernel and the deform vertex
    function in this modifier.
*/
class NiMorphingKernelData
{
public:

    inline NiMorphingKernelData()
    {
        m_uiCombination = (NiUInt16)MKDC_INVALID;
        m_uiActiveCount = 0;
    }

    inline ~NiMorphingKernelData()
    {
    }

    inline EMorphingKernelDataCombination GetCombination() const
    {
        return (EMorphingKernelDataCombination)m_uiCombination;
    }

    inline void SetCombination(
        NiUInt32 uiSourceComponentSize,
        NiUInt32 uiTargetComponentSize,
        bool bNormalize)
    {
        NiUInt16 uiCombination = 0;
        uiCombination |= (uiSourceComponentSize == sizeof(float)) ?
            (NiUInt16)FLAG_POINTS_32 : (NiUInt16)FLAG_POINTS_16;

        uiCombination |= (uiTargetComponentSize == sizeof(float)) ?
            (NiUInt16)FLAG_TARGETS_32 : (NiUInt16)FLAG_TARGETS_16;

        if (bNormalize)
            uiCombination |= (NiUInt16)FLAG_NORMALIZE;

        m_uiCombination = uiCombination;
    }

    inline NiUInt16 GetActiveCount() const
    {
        return m_uiActiveCount;
    }

    inline void SetActiveCount(const NiUInt16 uiActiveCount)
    {
        m_uiActiveCount = uiActiveCount;
    }

private:
    /// Data type/stride of the data streams
    NiUInt16 m_uiCombination;

    /// Active weight count
    NiUInt16 m_uiActiveCount;
};

#endif // #ifndef NIMORPHINGKERNELDATA_H
