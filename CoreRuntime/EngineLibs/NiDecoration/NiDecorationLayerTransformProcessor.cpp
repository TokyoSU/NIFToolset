// EMERGENT GAME TECHNOLOGIES PROPRIETARY INFORMATION
//
// This software is supplied under the terms of a license agreement or
// nondisclosure agreement with Emergent Game Technologies and may not
// be copied or disclosed except in accordance with the terms of that
// agreement.
//
//      Copyright (c) 1996-2010 Emergent Game Technologies.
//      All Rights Reserved.
//
// Emergent Game Technologies, Calabasas, CA 91302
// http://www.emergent.net

#include "NiDecorationPCH.h"
#include "NiDecorationLayerTransformProcessor.h"

#include "NiDecorationLayer.h"

//------------------------------------------------------------------------------------------------
NiDecorationLayerTransformProcessor::NiDecorationLayerTransformProcessor(NiAVObject* pkLayer)
    : m_pkLayer(pkLayer)
{
}

//------------------------------------------------------------------------------------------------
NiDecorationLayerTransformProcessor::~NiDecorationLayerTransformProcessor()
{
}

//------------------------------------------------------------------------------------------------
CellRequestGenerationResult NiDecorationLayerTransformProcessor::ProcessCellTransforms(
    NiDecorationCell* pkCell,
    NiTransform* pkTransformStream, 
    NiUInt32 uiTransformCount,
    bool bWriteInvalidTransformOnFail)
{
    NiDecorationLayer* pkLayer = NiVerifyStaticCast(NiDecorationLayer, m_pkLayer);

    return pkLayer->HandleCellRequestResponse(pkCell, pkTransformStream, uiTransformCount,
        bWriteInvalidTransformOnFail);
}

//------------------------------------------------------------------------------------------------
