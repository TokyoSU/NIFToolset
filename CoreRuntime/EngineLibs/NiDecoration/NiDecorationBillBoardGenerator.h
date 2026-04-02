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

#ifndef NIDECORATIONBILLBOARDGENERATOR_H
#define NIDECORATIONBILLBOARDGENERATOR_H

#include "NiDecorationMeshGenerator.h"

class NiAVObject;
class NiMesh;

/**
    Implementation of NiDecorationGenerator representing a simple two-sided billboard. Each
    billboard consists of 3 triangles to improve animation.

    The billboards origin is defined as: (0, 0, fBillboardHeight / 2)

    @note This generator automatically sets the material to 
        NiDecorationMaterial and creates the appropriate noise texture for use
        in screen door fading
    @note This generator attaches an NiStencilProperty to the mesh to enable
        double sided triangles.
 */
class NIDECORATION_ENTRY NiDecorationBillBoardGenerator : 
    public NiDecorationMeshGenerator
{
    NiDeclareRTTI;

    enum VERTEX_ELEMENTS
    {
        VE_POSITION,
        VE_NORMAL,
        VE_UV_MODEL,
        VE_TANGENT,
        VE_BINORMAL,

        VE_MAX
    };

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
    NiDecorationBillBoardGenerator(bool bUseInstancing = true, 
        float fBillboardWidth = 1.0f, float fBillboardHeight = 1.0f,
        NiUInt32 uiVerticesPerMesh = 5, NiUInt32 uiIndicesPerMesh = 9);

    /// Virtual Destructor
    virtual ~NiDecorationBillBoardGenerator();

    /// Width of the billboard, in model space.
    //@{
    inline float GetBillBoardWidth() const;
    inline void SetBillBoardWidth(float fWidth);
    //@}

    /// Height of the billboard, in model space.
    //@{
    inline float GetBillBoardHeight() const;
    inline void SetBillBoardHeight(float fHeight);
    //@}

    /// Texture to be used as the generated meshes Base texture.
    //@{
    inline void SetBaseTexture(NiTexture* pkTexture);
    inline NiTexture* GetBaseTexture();
    //@}

    /// @cond EMERGENT_INTERNAL
    static void _SDMInit();
    static void _SDMShutdown();
    /// @endcond

    /**
        Defines how much the billboards will deform throughout the animation 
        period. A value of 0.0 will disable animation.
    */
    //@{
    inline void SetAnimationSwayMultiplier(float fMultiplier = 1.0f);
    inline float GetAnimationSwayMultiplier() const;
    //@}

    // Overridden virtual functions inherit base documentation and thus
    // are not documented here.

    /// NiDecorationMeshGenerator implementation
    //@{
    virtual void InitializeVertexElements(NiDataStreamElementSet& kVertexElements);
    virtual bool GetVertexElementSemantic(NiUInt32 uiElementIndex, 
        NiFixedString& kSemantic, 
        NiUInt32& uiSemanticIndex);
    virtual bool GetVertexElementRequiresTransform(
        const NiFixedString& kSemantic, 
        NiUInt32& uiSemanticIndex,
        bool& bRequiresTransform);
    virtual bool GetVertexElementIsNormalized(
        const NiFixedString& kSemantic, 
        NiUInt32& uiSemanticIndex,
        bool& bIsNormalized);
    virtual const NiFixedString& GetGeneratorType();
    virtual const void* GetVertexElementTemplateData(size_t& stTemplateSize, 
        NiDataStreamElement::Format eFormat, const NiFixedString& kSemantic, 
        NiUInt32 uiSemanticIndex);
    virtual const NiUInt32* GetIndexTemplateData(NiUInt32& uiCount);
    virtual void UpdatePropertyData(NiDecorationMeshInfo* pkBase);
    //@}

    // Overridden virtual functions inherit base documentation and thus
    // are not documented here.

    /// NiDecorationGenerator implementation
    //@{
    virtual void UpdateAnimation(NiDecorationMeshInfo* pkBase, NiUpdateProcess& kProcess);
    virtual void Initialize();
    //@}

protected:

    /// Fill the internal vertex and index stream templates with generated data that represents a
    /// billboard mesh
    virtual void PopulateStreamTemplates();

    /// (Re)allocate the memory used to store the vertex and index stream templates
    virtual void CreateStreamTemplates(NiUInt32 uiNumVertices, NiUInt32 uiNumIndices);

    /// Rotate the given point around the origin (0,0,0) by the given angles
    void RotatePoint(float fAngleX, float fAngleY, NiPoint3& kPoint);

    float m_fBillboardWidth;
    float m_fBillboardHeight;
    float m_fAnimationSwayMultipier;

    NiPoint3* m_pkVertexPositionTemplate;
    NiPoint3* m_pkVertexNormalTemplate;
    NiPoint2* m_pkVertexTexCoordTemplate;
    NiUInt32* m_pkIndexTemplate;

    NiTexturePtr m_spBaseTexture;
};

#include "NiDecorationBillBoardGenerator.inl"

#endif // NIDECORATIONBILLBOARDGENERATOR_H
