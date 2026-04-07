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

#ifndef NIDECORATIONLAYER_H 
#define NIDECORATIONLAYER_H

#include "NiDecorationGenerator.h"

#include "NiDecorationLibType.h"

#include <NiNode.h>
#include <NiCamera.h>

class NiDecorationCell;

/**
    An instance of this class represents a square section of an infinitely expanding decoration 
    layer. The section is defined by the scene graph parent of this layer instance, an 
    NiDecorationField.

    A single decoration layer instance manages a set of mesh instances through via a grid of 
    NiDecorationCells, an optional set of NiDecorationFunctorBase derived objects and an 
    NiDecorationGenerator derived object.

    The layer is split into a grid of equally sized 'cells' along the layers XY plane. Not all of 
    these cells are loaded into memory and rendered - only cells that are within a given range of 
    the assigned camera. 
    
    Each visible cell contains a pointer to a portion of the transform stream, which is owned by 
    the generators Transform Manager. This layer shares the instance transform stream with other 
    layer objects.

    All cell are paging requests (in and out), are passed through the transform manager so that it 
    can keep the transform stream contiguous.

    The maximum number of cells that are visible at any one time is calculated from the width of the
    layer in world space, the width of a cell in world space and the maximum range of a cell.
 */
class NIDECORATION_ENTRY NiDecorationLayer : public NiNode
{
    NiDeclareRTTI;

    /// Simple UInt32 (X, Y) pair
    struct UIntPair 
    { 
        NiUInt32 m_x; 
        NiUInt32 m_y;
    };

    /// List of UInt32 pairs
    typedef efd::list<UIntPair> UIntPairList;

    /// List of NiDecorationCell pointers
    typedef efd::list<NiDecorationCell*> CellList;

public:

    /// An STL list of smart pointers to NiDecorationFunctorBase objects
    typedef NiDecorationGenerator::FunctorList FunctorList;

    /**
        Parameterized constructor

        @param pkGenerator Mesh generator that will be used my the layer to create and manage the 
            base mesh and all mesh instances.
     */
    NiDecorationLayer(NiDecorationGenerator* pkGenerator);

    /// Virtual destructor
    virtual ~NiDecorationLayer();
    
    /**
        Initialize the decoration layer according to the given parameters.
        This should not be called directly, call Initialize on the decoration
        field that this layer belongs to.

        @internal
        @note Emergent internal use only
     */
    virtual void Initialize(NiUInt32 uiNumCellsX, NiUInt32 uiNumCellsY,
        NiPoint2 kLayerWidth, float fMaxRange, float fMinRange, 
		float fFarFadeDistance = 0.0f, float fNearFadeDistance = 0.0f);

    /**
        Release all cached cell visibility data so that instance transforms are calculated from 
        scratch during the next update pass.
    */
    void ResetCells();

    // Overridden virtual functions inherit base documentation and thus
    // are not documented here.

    /// NiAVObject overrides
    //@{
    virtual void UpdateDownwardPass(NiUpdateProcess& kUpdate);
    virtual void UpdateSelectedDownwardPass(NiUpdateProcess& kUpdate);
    virtual void UpdateRigidDownwardPass(NiUpdateProcess& kUpdate);
    virtual void UpdateWorldBound();
    //@}

    /**
        Visibility Processing. 
        
        Visibility should only be calculated once per frame, i.e. for only one camera. Visibility 
        information is cached across frames so that small numbers of transforms are calculated per 
        frame.

        A call to both ReleaseInvisibleCells and CreateVisibleCells must be made once per frame.
        Each call will queue requests to the transform manager, which will process them at a later
        time.
     */ 
    // @{
    /**
        Release any cells that are currently marked as visible yet are out of range of the camera
     */
    void ReleaseInvisibleCells();

    /**
        Requests that any newly visible cells are created.
     */
    void CreateVisibleCells();

    /**
        Populates the given transform stream with transforms for the given cell, using the current
        generator. This function is called when a cell creation request is processed by the 
        generators transform manager.

        @param pkCell Cell for which transforms will be calculated
        @param pkTransformStream Destination transform stream. This must have at least 
            uiTransformCount values allocated.
        @param uiTransformCount Number of values in the given transform stream
        @param bWriteInvalidTransformOnFail On true generate the transform anyway and mark the transform as invalid. 
     */
    CellRequestGenerationResult HandleCellRequestResponse(NiDecorationCell* pkCell, 
        NiTransform* pkTransformStream, 
        NiUInt32 uiTransformCount,
        bool bWriteInvalidTransformOnFail);
    // @}

    /**
        Camera that will be used when calculating which cells should be visible.
     */
    //@{
    inline NiCamera* GetCamera() const;
    inline void SetCamera(NiCamera* pkCamera);
    //@}

    /// @return Length of this layer in the local X axis, in cells.
    inline NiUInt32 GetNumCellsX() const;

    /// @return Breadth of this layer in the local Y axis, in cells.
    inline NiUInt32 GetNumCellsY() const;

    /// @return Maximum distance (in model space) at which an instance will be visible.
    inline float GetMaxRange() const;

    /// @return Minimum distance (in model space) at which an instance will be visible.
    inline float GetMinRange() const;

    /// Transition distance (in model space) from opaque to transparent, when an instance is
    ///     nearing the maximum view distance.
    //@{
    inline void SetFarFadeDistance(float fDistance);
    inline float GetFarFadeDistance() const;
    //@}

    /// Transition distance (in model space) from transparent to opaque, when an instance is
    ///     nearing the minimum view distance.
    //@{
    inline void SetNearFadeDistance(float fDistance);
    inline float GetNearFadeDistance() const;
    //@}

    /**
        @return Length and Breadth of this layer in model space.
     */ 
    inline const NiPoint2& GetDimensions() const;

    /**
        The base seed that is used for all pseudo-random calculations within this layer.
     */
    //@{
    inline NiUInt32 GetBaseSeed() const;
    inline void SetBaseSeed(NiUInt32 uiBaseSeed);
    //@}

    /**
        The maximum number of cells that may be generated and uploaded per CreateVisibleCells call. 
        If set to zero, no cells will ever be generated.
     */
    //@{
    inline NiUInt32 GetMaxCellsGeneratedPerFrame() const;
    inline void SetMaxCellsGeneratedPerFrame(NiUInt32 uiMaxCells);
    //@}

    /**
        @return Pointer to the generator that is currently assigned to this layer.
     */
    inline NiDecorationGenerator* GetGenerator() const;

    /**
        @return The base mesh that was created in the Initialize function.
     */
    inline NiDecorationMeshInfo* GetBaseMesh() const;

    /**
        Specify that this layer should gather its instance position and visibility data based upon 
        the given functor list.

        When generating transforms, the generator will proceed though the set until a functor 
        successfully generates transforms for the relevant cell.

        @note The layer keeps smart pointers to functors.
        */
    //@{
    inline void SetFunctorList(const FunctorList& kFunctorSet);
    inline const FunctorList& GetFunctorList() const;
    //@}

    /**
        Runs our mesh (if not NULL) through the currently assigned functors configuration functions
     */
    void ApplyFunctorsToMesh();

    /**
        Removes any functor configuration from our base mesh, if not NULL
     */
    void RemoveFunctorsFromMesh();

    /**
        Updates the state of material shader constants
     */
    void UpdateShaderConstants();

protected:

    /// Helper function to return a layer coordinate space transform from a 
    /// given world space transform
    inline NiTransform WorldToLocal(const NiTransform& kWorldTransform) const;

    /// Helper to assist in assignment of the base mesh. This will deal with instance pool 
    /// allocation and functor configuration
    void SetBaseMesh(NiDecorationMeshInfo* pkMesh);

    /// Calculates the dimensions (in layer model space) of each cell.
    NiPoint2 CalculateCellRange() const;

    /// The list of functors that we can use.
    FunctorList m_kFunctors;

    /// Cells which need to have their transforms generated
    UIntPairList m_pendingTransformCells;

    /// List of all cells that are currently visible
    CellList m_kVisibleCells;

    /// Stores the transform of the camera at the end of the last call to CreateVisibleCells
    NiTransform m_kLastCameraTransform;
    
    /// Stores the transform of the camera at the end of the last call to CreateVisibleCells, in 
    // layer model space
    NiTransform m_kLastLocalCameraTransform;

    /// Stores the inverse of the layers world transform
    NiTransform m_kWorldTransformInv;

    /// Spacing between cells. This is a calculated value, regenerated whenever Initialize is run.
    NiPoint3 m_kCellSpacing;

    NiPoint3 m_kCellLocationOffset;

    /// Dimensions of this layer, in model space
    NiPoint2 m_kDimensions;

    float m_fFarFadeDistance;
	float m_fNearFadeDistance;
    float m_fMinRange;
    float m_fMaxRange;
    float m_fMinRangeClamped;
    float m_fMinRangeClampedSqr;
    float m_fMaxRangeClamped;
    float m_fMaxRangeClampedSqr;

    /// Current active decoration generator
    NiDecorationGeneratorPtr m_spGenerator;

    /// Camera to use in distance calculations.
    NiCameraPtr m_spCamera;

    /// Mesh that contains all of the instances.
    NiPointer<NiDecorationMeshInfo> m_spBaseMesh;

    /// The base random seed used for generating grass positions and rotations
    /// within the cells
    NiUInt32 m_uiBaseSeed;

    /// The maximum number of cells we are allowed to generate transforms for per update call.
    NiUInt32 m_uiMaxCellGenerationsPerCall;

    /// Maximum number of cells in the X and Y direction
    //@{
    NiUInt32 m_auiNumCells[2];
    //@}

    /// Set to true when SetCamera is called to indicate that all old camera position data must be
    /// ignored.
    bool m_bCameraRedefined;
};

NiSmartPointer(NiDecorationLayer);

#include "NiDecorationLayer.inl"

#endif // NIDECORATIONLAYER_H
