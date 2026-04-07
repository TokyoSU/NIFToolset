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

#ifndef NIDECORATIONSIMPLEMESHGENERATOR_H
#define NIDECORATIONSIMPLEMESHGENERATOR_H

#include "NiDecorationGenerator.h"
#include <efd/utf8string.h>

class NiAVObject;
class NiMesh;

/**
    Implementation of NiDecorationGenerator that uses a source NIF file as the source of the mesh
    instance data. GPU Mesh instancing must be used with this generator.

    When the NIF file contains multiple meshes and the user has not specified a source mesh by name,
    the first compatible mesh (with IsAppCulled set to FALSE) will be used.
 */
class NIDECORATION_ENTRY NiDecorationSimpleMeshGenerator : 
    public NiDecorationGenerator
{
    NiDeclareRTTI;

protected:

public:

    /// Class Name of this generator (used by the factories)
    static NiFixedString GENERATOR_NAME;

    /**
        Default Constructor
    */
    NiDecorationSimpleMeshGenerator();

    /// Virtual Destructor
    virtual ~NiDecorationSimpleMeshGenerator();

    /// @cond EMERGENT_INTERNAL
    static void _SDMInit();
    static void _SDMShutdown();
    /// @endcond

    /**
        Determines whether or not the given mesh is a compatible source for instance mesh data.

        @param pkMesh Mesh to be tested
        @param kErrorMessage If the mesh is not compatible, set to the incompatibility reason

        @return true if the given mesh is compatible, false otherwise
     */
    bool IsMeshCompatible(const NiMesh* pkMesh, NiFixedString& kErrorMessage);

    /// Absolute path of the NIF file containing our simple mesh
    //@{
    inline void SetMeshPath(const efd::utf8string& kMeshPath);
    inline const efd::utf8string& GetMeshPath() const;
    //@}

    /// Scaler multiplier applied to the base texture shader sampler
    //@{
    inline void SetBaseTextureSaturation(float fSaturation);
    inline float GetBaseTextureSaturation() const;
    //@}

    /// Optional name of a sub object within the loaded scene graph to use as the base mesh
    //@{
    inline void SetObjectName(const efd::utf8string& kObjectName);
    inline const efd::utf8string& GetObjectName() const;
    //@}

    // Overridden virtual functions inherit base documentation and thus
    // are not documented here.

    /// NiDecorationGenerator implementation
    //@{
    virtual void Initialize();
    virtual void InitializeTransformStream(NiUInt32 uiInstancesPerCell, NiUInt32 uiRequiredCells);
    virtual const NiFixedString& GetGeneratorType();
    virtual NiDecorationMeshInfo* CreateBaseMesh(NiUInt32 uiNumCells, NiUInt32 uiInstancesPerCell);
    virtual void UpdateAnimation(NiDecorationMeshInfo* pkBase, NiUpdateProcess& kProcess);
    virtual void UpdatePropertyData(NiDecorationMeshInfo* pkBase);
    virtual void SetTransforms(NiDecorationMeshInfo* pkBase, NiTransform* pkTransforms, 
        NiUInt32 uiNumCells, NiUInt32 uiStartCell = 0);
    virtual void SetActiveInstanceCount(NiDecorationMeshInfo* pkBase, NiUInt32 uiInstanceCount);
    //@}

protected:

    virtual bool GetUseInstancing(NiDecorationMeshInfo* pkBase) const;

    /// Recursively find and return the first NiMesh derived object contained in
    /// the given scene
    NiMesh* FindNiMesh(NiAVObject* pkObject);

    /// Absolute path of the NIF file containing our simple mesh
    efd::utf8string m_kMeshPath;

    /// Optional name of a sub object within the loaded scene graph to use as the base mesh
    efd::utf8string m_kObjectName;

    /// The shared transform stream. Maintain a smart pointer so that it is not deleted when any
    /// meshes referencing it are destroyed.
    NiDataStreamPtr m_spTransDataStream;

    /// Error messages returned by IsMeshCompatible, if the mesh is not compatible.
    //@{
    static NiFixedString ms_kErrorSubmeshCount;
    static NiFixedString ms_kErrorSkinnedMesh;
    //@}
};

#include "NiDecorationSimpleMeshGenerator.inl"

#endif // NIDECORATIONSIMPLEMESHGENERATOR_H
