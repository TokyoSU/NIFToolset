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

#ifndef NIDECORATIONPLANE_H
#define NIDECORATIONPLANE_H

#include "NiDecorationField.h"
#include "NiDecorationGenerator.h"
#include "NiDecorationSimpleMeshGenerator.h"

/**
    A decoration plane is the 'top level' scene graph item in the decoration system. It creates and
    manages a set of NiDecorationFields according to properties set by the user.

    NiDecorationField instances should not be attached using traditional methods as they are
    created and maintained as required, according to the internal list of registered field ID's. A
    FieldID contains information about the location of the field.

    Changes to any properties, such as registered FieldID's, are processed in bulk on a scene graph
    update.
 */
class NIDECORATION_ENTRY NiDecorationPlane : public NiNode
{
    NiDeclareRTTI;

public:

    /// ID type of a managed NiDecorationField
    typedef NiUInt32 FieldID;

    /// Key/Value map used for functor and generator settings
    typedef efd::map<efd::utf8string, efd::utf8string> SettingsMap;

    /// Default constructor
    NiDecorationPlane();

    /// Virtual destructor
    virtual ~NiDecorationPlane();

    /// @cond EMERGENT_INTERNAL
    static void _SDMInit();
    static void _SDMShutdown();
    /// @endcond

    /**
        Override the update function of nodes so that those are called
        appropriately when updating the decoration.
    */
    //@{
    virtual void UpdateDownwardPass(NiUpdateProcess& kUpdate);
    virtual void UpdateSelectedDownwardPass(NiUpdateProcess& kUpdate);
    virtual void UpdateRigidDownwardPass(NiUpdateProcess& kUpdate);    
    //@}
    
    /**
        Sets / Gets the camera to be used with the decoration layers
    */   
    //{@ 
    inline NiCamera* GetCamera() const;
    inline void SetCamera(NiCamera* pkCamera);
    //}@

    /**
        Sets / Gets the number of layers to be used with the decoration fields
    */ 
    //{@
    inline NiUInt32 GetNumLayers() const;
    //}@

    void AddLayer(const efd::utf8string& kLayerName);
    void RemoveLayer(const efd::utf8string& kLayerName);

    /**
        Sets / Gets the dimensions of the decoration fields (pre world transform)
    */ 
    //{@
    inline void SetDimension(const NiPoint2& kDimensions);
    inline const NiPoint2& GetDimension() const;
    //}@

    /**
        Sets / Gets the number of cells along X to be used with the decoration fields
    */ 
    //{@
    inline void SetNumCellsX(const efd::utf8string& kLayerName, NiUInt32 uiNumCells);
    inline NiUInt32 GetNumCellsX(const efd::utf8string& kLayerName) const;
    //}@

    /**
        Sets / Gets the number of cells along Y to be used with the decoration fields
    */ 
    //{@
    inline void SetNumCellsY(const efd::utf8string& kLayerName, NiUInt32 uiNumCells);
    inline NiUInt32 GetNumCellsY(const efd::utf8string& kLayerName) const;
    //}@

    /**
        Sets / Gets the number of instances per cell on a given layer for the decoration fields
    */ 
    //{@
    inline void SetInstancesPerField(const efd::utf8string& kLayerName, NiUInt32 uiNumInstances);
    inline NiUInt32 GetInstancesPerField(const efd::utf8string& kLayerName) const;
    //}@    

    /**
        Sets / Gets the maximum visible range of mesh instances
    */
    //{@
    inline void SetMaxRange(const efd::utf8string& kLayerName, float fMaxRange);
    inline float GetMaxRange(const efd::utf8string& kLayerName) const;
    //}@

    /**
        Sets / Gets the minimum visible range of mesh instances
    */ 
    //{@
    inline void SetMinRange(const efd::utf8string& kLayerName, float fMinRange);
	inline float GetMinRange(const efd::utf8string& kLayerName) const;
    //}@

    /**
        Sets / Gets the near fading distance of a mesh instance
    */ 
    //{@
    inline void SetNearFade(const efd::utf8string& kLayerName, float fNearFade);
	inline float GetNearFade(const efd::utf8string& kLayerName) const;
    //}@

    /**
        Far fade distance of a mesh instance
    */ 
    //{@
    inline void SetFarFade(const efd::utf8string& kLayerName, float fFarFade);
    inline float GetFarFade(const efd::utf8string& kLayerName) const;
    //}@

    /**
        Decoration instance generator that is responsible for creating the specified layers mesh,
        as well as managing the instance transforms. 
        
        @note Setting this property will trigger the recreation of this layer

        @param kLayerName Identifier of the layer
        @param pkGenerator Generator instance
     */
    //{@
    inline void SetGenerator(const efd::utf8string& kLayerName, NiDecorationGenerator* pkGenerator);
    inline NiDecorationGenerator* GetGenerator(const efd::utf8string& kLayerName) const;
    //@}

    /**
        Sets / Gets the random seed to be used for all internal pseudo-random calculations
    */ 
    //{@
    inline void SetRandomSeed(const efd::utf8string& kLayerName, NiUInt32 uiSeed);
    inline NiUInt32 GetRandomSeed(const efd::utf8string& kLayerName) const;
    //}@

    /**
        The maximum number of cells that may be generated and uploaded per Update call. If set to
        zero, no cells will ever be generated.
     */
    //@{
    inline NiUInt32 GetMaxCellsGeneratedPerFrame(const efd::utf8string& kLayerName) const;
    inline void SetMaxCellsGeneratedPerFrame(const efd::utf8string& kLayerName, 
        NiUInt32 uiMaxCells);
    //@}

    /**
        Scene graph object that the decoration functors will reference when attempting to set the 
        instance transforms.
     */
    //@{
    inline void SetFunctorTarget(NiAVObject* pkTarget);
    inline NiAVObject* GetFunctorTarget() const;
    //@}

    /**
        Settings that all functors will read when creating instance transforms
     */
    //@{
    inline void SetFunctorTargetSettings(const efd::utf8string& kLayerName, 
        const SettingsMap& kSettings);
    inline const SettingsMap& GetFunctorTargetSettings(const efd::utf8string& kLayerName);
    //@}

    /**
        Marks all functor settings as dirty so that they are re-applied to all layers
     */
    void ReapplyFunctorConfiguration();

    /**
        Forces the recalculation of all instance positions
     */
    inline void InvalidateAllTransforms();

    /**
        Creates a decoration field and adds it to the field map at the given index.

        @param uiKey map index where the field should be added
        @return returns the decoration field if created
    */
    NiDecorationField* CreateDecorationFieldAt(FieldID uiKey);

    /**
        Returns the field at the given index

        @param uiKey map index of the field to return
        @return returns the decoration field if found
    */
    NiDecorationField* GetDecorationFieldAt(FieldID uiKey);

    /**
        Populates the given set with the keys of all active decoration fields
     */
    void GetDecorationFieldKeys(NiTPrimitiveSet<FieldID>& kKeys);

    /**
        Remove the decoration field at the given index.

        @param uiKey index of the field to be removed
    */
    void RemoveDecorationFieldAt(FieldID uiKey);

    /**
        Generate a FieldID from a fields coordinates

        @param sIndexX The fields X coordinate
        @param sIndexY The fields Y coordinate
        @param[out] kFieldID the generated FieldID
    */
    static void GenerateFieldID(NiInt16 sIndexX, NiInt16 sIndexY, FieldID& kFieldID);

    /**
        Extract the fields coordinates from a FieldID

        @param kFieldID The FieldID to extract the data from
        @param[out] sIndexX The fields X coordinate
        @param[out] sIndexY The fields Y coordinate
        
    */
    static void GenerateFieldIndex(const FieldID& kFieldID, NiInt16& sIndexX, NiInt16& sIndexY);

protected:

    /**
        Ensures that the radius of our bound is not zero so that the Lightspeed Light service 
        doesn't fail, since it does not support zero radius objects.
     */
    void EnforceValidBound();

    /**
        Create the decoration field that this component manages. This function 
        does not add any layers or initialize the field, which occurs in the 
        Update function.
    */
    NiDecorationField* CreateField();

    /**
        Helper function to create an NiDecorationLayer object to represent a field in the given 
        layer.
     */
    NiDecorationLayer* CreateLayerForField(const efd::utf8string& kLayerName, 
        NiDecorationField* pkField);

    // if pkLayer == null, functors are created for layers in all fields.
    void AttachFunctorsToLayer(const efd::utf8string& kLayerName,
        NiDecorationLayer* pkLayer = NULL);

    /**
        Re-initializes all the functors for the given layer.
     */
    void ReconfigureFunctors(const efd::utf8string& kLayerName);

    /**
        Performs all update tasks:
        - Application of settings
        - Initialization of fields, layers, functors and generators as required
        - Progression of animation
        - Update of instance visibility

        @param fTime current application time. This is used for the mesh animations
    */
    void DoUpdate(float fTime);

    /**
        Component initialization
     */
    //@{
    void InitializeFunctors();
    void InitializeFields();
    bool InitializeLayers();
    bool InitializeCells();
    //@}

    /**
        Update cell visibility for all layers
     */
    void UpdateLayers();

    class NiDecorationLayerInfo
    {
    public:
        NiDecorationLayerInfo(NiDecorationGenerator* pkGenerator = NULL,
            NiUInt32 uiNumCellsX = 1, NiUInt32 uiNumCellsY = 1, NiUInt32 uiInstancesPerField = 0,
            float fMaxRange = 0.0f, float fMinRange = 0.0f, 
            float fFarFadeDistance = 0.0f, float fNearFadeDistance = 0.0f,
            NiUInt32 uiMaxCellsGenerated = UINT_MAX, NiUInt32 uiRandomSeed = 0);

        // Functors
        typedef efd::list<NiDecorationFunctorBasePtr> FunctorList;
        FunctorList m_kFunctors;
        SettingsMap m_functorSettings;

        NiDecorationGeneratorPtr m_spGenerator;

        NiUInt32 m_uiNumCellsX;
        NiUInt32 m_uiNumCellsY;
        NiUInt32 m_uiInstancesPerField;
        NiUInt32 m_uiMaxCellsGenerated;
        NiUInt32 m_uiRandomSeed;
        float m_fMaxRange;
        float m_fMinRange;
        float m_fNearFadeDistance;
        float m_fFarFadeDistance;

        bool m_bRequiresFunctorCreation;
        bool m_bRequiresFunctorReinitialization;
        bool m_bRequiresInitialization;
        bool m_bRequiresReset;
    };

    /**
        Helper function to retrieve the NiDecorationLayerInfo instance for a given layer
     */
    //@{
    const NiDecorationLayerInfo& GetLayerInfo(const efd::utf8string& kLayerName) const;
    NiDecorationLayerInfo& GetLayerInfo(const efd::utf8string& kLayerName);
    //@}

    // Layers
    typedef efd::map<efd::utf8string, NiDecorationLayerInfo> LayerInfoMap;
    LayerInfoMap m_kLayers;

    // Map of field index => decoration field.
    NiTPointerMap<FieldID, NiDecorationField*> m_kFieldMap;

    // Transformation as it was at the end of the last update call
    NiTransform m_kLastLocalTransform;

    // Property values
    NiPoint2 m_kDimension;
    NiAVObject* m_pFunctorTarget;

    // Pointer to the camera that is used for distance calculations in the 
    // layers
    NiCamera* m_pCamera;

    // If true, all fields need to be destroyed and re-created.
    bool m_bRequiresRecreation;

    // Name given to all child field objects
    static NiFixedString ms_kChildFieldName;
};

NiSmartPointer(NiDecorationPlane);

#include "NiDecorationPlane.inl"

#endif // NIDECORATIONPLANE_H
