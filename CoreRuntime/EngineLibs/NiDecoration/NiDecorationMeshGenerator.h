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

#ifndef NIDECORATIONMESHGENERATOR_H
#define NIDECORATIONMESHGENERATOR_H

#include "NiDecorationGenerator.h"

NiSmartPointer(NiDecorationMeshGenerator);

/**
    Base generator class which provides base functionality for mesh based decorations. This class 
    supports both instanced and non instanced modes, and performs any required transformation from 
    model to world space.

    The vertex stream elements must be defined by the implementing class, which allows arbitrary 
    meshes to be created.

    Index streams are automatically reduced to 16bit where possible, reverting to 32bit if the mesh 
    contains too many vertices.
 */
class NIDECORATION_ENTRY NiDecorationMeshGenerator : 
    public NiDecorationGenerator
{
    NiDeclareRTTI;

protected:
    enum STREAMREF_ID
    {
        SRI_VERTEX          = 0,
        SRI_INDEX           = 1,
        SRI_VERTEX_STATIC   = 2
    };
public:

    /**
        Parameterized constructor

        @param bUseInstancing Enable GPU based mesh instancing if available.
     */
    NiDecorationMeshGenerator(bool bUseInstancing = true);

    /// Virtual Destructor
    virtual ~NiDecorationMeshGenerator();

    /**
        Create or resize the instance transform stream to fit the given number of cells. It is valid
        to call this while the instance stream is in use by layers, however each layer must have
        first released all of its visible cells.

        @param uiInstancesPerCell The number of instances that each layers cell requires
        @param uiRequiredCells The maximum number of cells that a layer will require
     */ 
    virtual void InitializeTransformStream(NiUInt32 uiInstancesPerCell, NiUInt32 uiRequiredCells);

    /**
        Implementing class should populate the given NiDataStreamElementSet with
        the elements that it requires on the mesh's vertex stream.
     */
    virtual void InitializeVertexElements(NiDataStreamElementSet& kElements) = 0;

    /**
        Specifies the semantic details of the element specified by uiElementIndex in the element set
        that can be acquired through InitializeVertexElements.

        @param uiElementIndex Index in the element set of which to acquire details
        @param kSemantic Semantic of discovered element
        @param uiSemanticIndex Semantic index of discovered element

        @return true if there is an element at the given index
     */
    virtual bool GetVertexElementSemantic(NiUInt32 uiElementIndex,
        NiFixedString& kSemantic, NiUInt32& uiSemanticIndex) = 0;

    // Overridden virtual functions inherit base documentation and thus
    // are not documented here.

    /// NiDecorationGenerator implementation
    //@{
    virtual bool GetUseInstancing(NiAVObject* pkBaseMesh) const;
    virtual NiDecorationMeshInfo* CreateBaseMesh(NiUInt32 uiNumCells, NiUInt32 uiInstancesPerCell);
    //@}

    /**
        Dictates whether a given element of the vertex stream requires 
        transformation by the model to world matrix.

        @note this is only called when mesh instancing is disabled
        @note Only elements of type NiPoint3 can be transformed.

        @param kSemantic Semantic of the element
        @param uiSemanticIndex Index of the semantic of the element
        @param bRequiresTransform True if the element requires transformation,
            false if no action is required

        @return True if the given semantic/index pair exists in the element set,
            false otherwise.
     */
    virtual bool GetVertexElementRequiresTransform(
        const NiFixedString& kSemantic, NiUInt32& uiSemanticIndex,
        bool& bRequiresTransform) = 0;

    /**
        Dictates whether the values of a given element of the vertex stream 
        are normalized. This affects the way they are transformed, if the
        element type is NiPoint3.

        @note this is only called when mesh instancing is disabled

        @param kSemantic Semantic of the element
        @param uiSemanticIndex Index of the semantic of the element
        @param bIsNormalized True if the element requires values are normalized

        @return True if the given semantic/index pair exists in the element set,
            false otherwise.
    */
    virtual bool GetVertexElementIsNormalized(
        const NiFixedString& kSemantic, NiUInt32& uiSemanticIndex,
        bool& bIsNormalized) = 0;

    /**
        Populate the destination memory with default model space values,
        according to the given semantic information.

        @param stTemplateSize Size (in bytes) of the memory to be set
        @param eFormat The Type of the expected data
        @param kSemantic Semantic type of the required data
        @param uiSemanticIndex Semantic index of the required data
        */
    virtual const void* GetVertexElementTemplateData(size_t& stTemplateSize, 
        NiDataStreamElement::Format eFormat, const NiFixedString& kSemantic, 
        NiUInt32 uiSemanticIndex) = 0;

    /**
        Populate the given index stream with 32-bit integer index values.
     */
    virtual const NiUInt32* GetIndexTemplateData(NiUInt32& uiCount) = 0;

    /// Number of index entries required by a single instance
    inline NiUInt32 GetIndicesPerMesh() const;

    /// Number of vertices required by a single instance
    inline NiUInt32 GetVerticesPerMesh() const;

    /// Processes any requests made to the transform manager, then uploads the changes to the GPU

protected:
    inline void SetIndicesPerMesh(NiUInt32 uiNumIndices);
    inline void SetVerticesPerMesh(NiUInt32 uiNumVertices);

    // Overridden virtual functions inherit base documentation and thus
    // are not documented here.

    virtual void SetTransforms(NiDecorationMeshInfo* pkBase, 
        NiTransform* pkTransforms, 
        NiUInt32 uiNumCells, 
        NiUInt32 uiStartCell = 0);
    virtual void SetActiveInstanceCount(NiDecorationMeshInfo* pkBase, 
        NiUInt32 uiInstanceCount);

    /**
        Templated helper function which performs an element-wise typecast from
        one stream to another.
    */
    template <class TFromType, class TToType> inline void CastStream(
        const TFromType* pCastFrom, void* pTo, NiUInt32 uiCount);

    NiUInt32 m_uiInstancesPerCell;

private:

    /// These should be initialized in the implementing classes constructor
    /// via the appropriate Set functions.
    //@{
    NiUInt32 m_uiIndicesPerMesh;
    NiUInt32 m_uiVerticesPerMesh;
    //@}

    /// The shared transform stream. Maintain a smart pointer so that it is not deleted when any
    /// meshes referencing it are destroyed.
    NiDataStreamPtr m_spTransDataStream;

};

#include "NiDecorationMeshGenerator.inl"

#endif // NIDECORATIONMESHGENERATOR_H
