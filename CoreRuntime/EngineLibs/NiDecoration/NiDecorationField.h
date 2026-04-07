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

#ifndef NIDECORATIONFIELD_H
#define NIDECORATIONFIELD_H

#include "NiDecorationLibType.h"
#include "NiDecorationLayer.h"
#include "NiDecorationFunctor.h"
#include <NiTSet.h>
#include <NiMemObject.h>

/**
    A Field represents a vertically portioned region within a decoration plane. It contains a
    slice for each layer definition on the plane in the form of an NiDecorationLayer instance.

    In other words, it contains one NiDecorationLayer instance for each of the parent planes 
    layer definitions, each covering a XY region which is a subset of the parent planes total 
    region.
 */
class NIDECORATION_ENTRY NiDecorationField : public NiNode
{
    NiDeclareRTTI;

public:

    /**
        Parameterized constructor

        @param kWidth Length and breadth (X and Y) of the field, in model 
            space.
     */
    NiDecorationField(NiPoint2 kWidth);

    /// Virtual Destructor
    virtual ~NiDecorationField();

    /**
        Uniquely adds the given layer to this field. If the layer is already present within the 
        field, and assertion is thrown and the layer is not added.

        @param kLayerName The key (a unique name) of the new layer
        @param pkLayer The layer instance to add
     */
    void AddLayer(const efd::utf8string& kLayerName, NiDecorationLayer* pkLayer);

    /**
        Attempt to remove the given layer from the field. If the layer does not exist in the field, 
        no operation is performed
     */
    //@{
    void RemoveLayer(NiDecorationLayer* pkLayer);
    void RemoveLayer(const efd::utf8string& kLayerName);
    //@}

    /**
        Appends the given set with a list of all layer instances present within this field.
     */
    void GetLayers(NiTPrimitiveSet<NiDecorationLayer*>& kLayers) const;

    /**
        @return the number of layer instances contained within this field
     */
    NiUInt32 GetNumLayers() const;

    /**
        @return Layer found at the given key (unique layer name)
     */
    NiDecorationLayer* GetLayerAt(const efd::utf8string& kLayerName) const;

    /**
        Identifier that can be used by the owner of this field. This has no direct effect on the
        field itself. This is passed to all functors when configuring the mesh.
     */
    // @{
    void SetFieldIndex(NiUInt32 uiFieldIndex);
    NiUInt32 GetFieldIndex() const;
    // @}

    /**
        Release all cached cell visibility data and recreate instance transforms from scratch during
        the next update.
     */
    void ResetCells();

    /**
        Process animation on the underlying instance meshes, via the contained layer Generators. 
        The provided generators will ignore animation if GPU mesh instancing is disabled.

        @param kUpdate Update state object
     */
    void UpdateAnimation(NiUpdateProcess& kUpdate);

    /**
        Causes each contained layer instance to refresh and set the cached shader constant values.
     */
    void UpdateShaderConstants();


    // Overridden virtual functions inherit base documentation and thus
    // are not documented here.

    /// NiNode implementation
    //@{
    virtual void UpdateDownwardPass(NiUpdateProcess& kUpdate);
    virtual void UpdateSelectedDownwardPass(NiUpdateProcess& kUpdate);
    virtual void UpdateRigidDownwardPass(NiUpdateProcess& kUpdate);
    //@}

protected:

    void RebuildLayerTransforms();

    typedef efd::map<efd::utf8string, NiDecorationLayerPtr> LayerMap;
    LayerMap m_kLayers;

    NiPoint2 m_kWidth;

    NiUInt32 m_uiFieldIndex;
};

NiSmartPointer(NiDecorationField);

#endif // NIDECORATIONFIELD_H
