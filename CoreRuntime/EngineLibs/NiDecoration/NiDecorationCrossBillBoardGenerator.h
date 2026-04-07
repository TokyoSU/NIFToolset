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

#ifndef NIDECORATIONCROSSBILLBOARDGENERATOR_H
#define NIDECORATIONCROSSBILLBOARDGENERATOR_H

#include "NiDecorationLibType.h"
#include "NiDecorationBillBoardGenerator.h"

/**
    Decoration 
 */
class NIDECORATION_ENTRY NiDecorationCrossBillBoardGenerator : 
    public NiDecorationBillBoardGenerator
{
    NiDeclareRTTI;

public:

    /// Class Name of this generator (used by the factories)
    static NiFixedString GENERATOR_NAME;

    /**
        Parameterized Constructor

        @param bUseInstancing Enable GPU based mesh instancing if available.
        @param fBillboardWidth Width in model space of the billboard mesh
        @param fBillboardHeight Height in model space of the billboard mesh
        @param uiVerticesPerMesh How many entries to allocate in the vertex stream
        @param uiIndicesPerMesh How many entries to allocate in the index stream
     */
    NiDecorationCrossBillBoardGenerator(bool bUseInstancing = true, 
        float fBillboardWidth = 0.35f, float fBillboardHeight = 0.35f,
        NiUInt32 uiVerticesPerMesh = 9, NiUInt32 uiIndicesPerMesh = 18);

    /// Virtual Destructor
    virtual ~NiDecorationCrossBillBoardGenerator();


    // Overridden virtual functions inherit base documentation and thus
    // are not documented here.

    /// NiDecorationMeshGenerator implementation
    // @{
    virtual const NiFixedString& GetGeneratorType();
    virtual void UpdateAnimation(NiDecorationMeshInfo* pkBase, NiUpdateProcess& kProcess);
    // @}

    /// @cond EMERGENT_INTERNAL
    static void _SDMInit();
    static void _SDMShutdown();
    /// @endcond

private:

    // Overridden virtual functions inherit base documentation and thus
    // are not documented here.
    virtual void PopulateStreamTemplates();
};

#endif // NIDECORATIONCROSSBILLBOARDGENERATOR_H