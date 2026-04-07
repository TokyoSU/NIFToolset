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

#ifndef NIDECORATIONLAYERTRANSFORMPROCESSOR_H
#define NIDECORATIONLAYERTRANSFORMPROCESSOR_H

#include "NiDecorationTransformManager.h"
#include "NiDecorationLibType.h"
#include "NiDecorationGenerator.h"

#include <NiAVObject.h>

/**
    Transform processor that uses its owning layer to carry out decoration instance transform
    generation.
    
    @internal
 */
class NIDECORATION_ENTRY NiDecorationLayerTransformProcessor : public NiDecorationTransformProcessor
{
public:

    /**
        Parameterized constructor.

        @param pkLayer The NiDecorationLayer that owns this processor. This layer will be used
            to fulfill any processing demands.
     */
	NiDecorationLayerTransformProcessor(NiAVObject* pkLayer);

    /// Virtual destructor
	virtual ~NiDecorationLayerTransformProcessor();

    // Overridden virtual functions inherit base documentation and thus
    // are not documented here.

    /// NiDecorationTransformProcessor overrides
    //@{
    virtual CellRequestGenerationResult ProcessCellTransforms(NiDecorationCell* pkCell, 
        NiTransform* pkTransformStream,
        NiUInt32 uiTransformCount,
        bool bWriteInvalidTransformOnFail);
    //@}

protected:

    NiAVObject* m_pkLayer;
};

#endif // NIDECORATIONLAYERTRANSFORMPROCESSOR_H
