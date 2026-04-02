// EMERGENT GAME TECHNOLOGIES PROPRIETARY INFORMATION
//
// This software is supplied under the terms of a license agreement or
// nondisclosure agreement with Emergent Game Technologies and may not 
// be copied or disclosed except in accordance with the terms of that 
// agreement.
//
//      Copyright (c) 1996-2007 Emergent Game Technologies.
//      All Rights Reserved.
//
// Emergent Game Technologies, Chapel Hill, North Carolina 27517
// http://www.emergent.net

#ifndef NITERRAINDECORATIONFUNCTOREX_H 
#define NITERRAINDECORATIONFUNCTOREX_H

#include "NiTerrainLibType.h"

#include <NiTerrainStreamLocks.h>
#include "NiTerrainCellLeaf.h"

class NiRandomLCG;

class NITERRAIN_ENTRY NiTerrainDecorationFunctor 
{
    // Extra data ordering
    enum EXTRA_DATA_KEYS
    {
        EDK_MATERIAL_META_DATA,
        EDK_OPACITY_THRESHOLD,
        EDK_SCALE,
        EDK_SCALE_VARIATION,
        EDK_QUOTA_MULTIPLIER,
        EDK_TERRAIN_COLOR,
        EDK_TERRAIN_NORMALS,

        // Number of array entries
        EDK_COUNT
    };

    // Index entries for a 2 value array, which represents a 1x2 matrix. Each
    // enum value corresponds to a row in the matrix.
    enum 
    {
        P_TOP, 
        P_BOTTOM 
    };

    // Index entries in a 4 value array, which defines the x,y components of all
    // corners of a voxel
    enum VoxelFactors
    {
        X_PREVIOUS,
        X_CURRENT,
        Y_TOP,
        Y_BOTTOM,

        VOXELFACTOR_MAX
    };

    struct GeneratorInfo
    {
        NiTerrainPositionRandomAccessIterator m_kPositions; 
        NiTerrainNormalRandomAccessIterator m_kNormals;
        NiPoint3 m_kWorldCellCorner;
        NiIndex m_kLeafIndex;
        NiIndex m_kCellOffset;
        NiIndex m_kCellsPerBlock;
        NiIndex m_kSizePerCell;
        NiIndex m_kBottomLeft;
        NiIndex m_kTopRight;
        NiUInt32 m_uiBlockSize;
        NiTerrainCellLeaf* m_pkCellLeaf;
        float m_fVertexSpacing;
        float m_fInstanceScale;
        float m_fInstanceScaleVariation;
        bool m_bUseTerrainNormals;
    };

public:

    // NiDecorationFunctor implementation
    typedef NiTerrain target_type;

    // *** begin Emergent internal use only ***
    static void _SDMInit();
    static void _SDMShutdown();
    // *** end Emergent internal use only ***

    static bool IsValidTargetType(NiObject* pkTarget);

    static void InitializeExtraData(NiTPrimitiveArray<NiExtraData*>& kExtraData);

    static bool Validate(NiTerrain* pkTerrain,
        NiTPrimitiveArray<NiExtraData*>& kExtraData,
        const NiUInt32 auiCellCount[2],
        const NiPoint2& kCellRange,
        const NiTransform& kLayerWorldTransform);

    // Applies the relevant terrain sector color map to the given mesh.
    static void ConfigureMesh(const NiTPrimitiveArray<NiExtraData*>& kExtraData, 
        const NiUInt32 auiCellCount[2], const NiPoint2& kCellRange,
        NiUInt32 uiFieldIndex, const NiTransform& kLayerWorldTransform, NiTerrain* pkTerrain, 
        NiAVObject* pkBase);

    // Returns true if at least 1 transform was successfully applied.
    static bool GenerateTransforms(NiTerrain* pkTerrain,
        const NiTPrimitiveArray<NiExtraData*>& kExtraData,
        NiTransform* pkTransforms, NiUInt32 uiTransformCount,
        const NiUInt32 auiCellCount[2], 
        const NiUInt32 auiCellIndex[2], 
        const NiPoint3& kCellCenter, 
        const NiPoint2& kRange, 
        NiUInt32 uiFieldIndex, 
        const NiTransform& kWorldTransform,
        NiRandomLCG* pkRandom);

protected:
    static void GenerateTransformsUsingMeta(NiTransform*& pkTransformIter, 
        const NiTransform* pkTransformEnd, NiUInt32 uiTransformCount, 
        const NiTransform& kWorldTransform, NiRandomLCG* pkRandom, 
        const GeneratorInfo& kGenInfo, const NiFixedString& kMetaDataKey, 
        float fOpacityThreshold);

    static void GenerateTransformsWithoutMeta(NiTransform*& pkTransformIter, 
        const NiTransform* pkTransformEnd, NiUInt32 uiTransformCount, 
        const NiTransform& kWorldTransform, NiRandomLCG* pkRandom, 
        const GeneratorInfo& kGenInfo);

    /// Gets the value of the requested component of pixel in the
    /// given blend mask's pixel data
    static float GetProbabilityAt(const NiPixelData*& pkPixelData, const NiUInt32& uiPixelIndex,
        const NiTPrimitiveArray<NiUInt32>& kMaskIndexArray,
        const NiTPrimitiveArray<float>& kMaskMetaValues);

    static inline float BiLerp(const NiPoint2& kInterpolant, 
        const NiPoint2& kInvInterpolant, float fBottomLeft,
        float fBottomRight, float fTopLeft, float fTopRight);

    static inline void FactoredBiLerp(const NiPoint2& kInterpolant,
        const float* fFactors, const float* fLeftColumn, 
        const float* fRightColumn, float& fResult, NiPoint2& kVortexPosition);

public:
    static NiFixedString FUNCTOR_NAME;
    static NiFixedString COLOR_EFFECT_NAME;
};

#include "NiTerrainDecorationFunctor.inl"

#endif // NITERRAINDECORATIONFUNCTOREX_H
