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

#ifndef NIDECORATIONGENERATOR_H
#define NIDECORATIONGENERATOR_H

#include <NiTexture.h>
#include <NiNoiseTexture.h>
#include <NiRandomLCG.h>
#include <NiMesh.h>
#include <NiMain.h>
#include <NiTPtrSet.h>

#include "NiDecorationLibType.h"
#include "NiDecorationFunctor.h"
#include "NiDecorationTransformManager.h"
#include "NiDecorationMeshInfo.h"

/// Forward declaration
class NiDecorationLayer;

/**
    Abstract base class for all decoration generators. A decoration generator is a factory that is 
    used by a layer to create and maintain the mesh instances. A generator is responsible for 
    creating the actual mesh geometry, a number of instances for that mesh, updating the 
    transformations of the instances and uploading transformation data to the GPU.

    A decoration generator will use mesh instancing via NiInstancingUtils if available, falling back
    to a large number of submeshes otherwise. This fallback technique is not recommended, as it will
    use vast amounts of memory.
*/
class NIDECORATION_ENTRY NiDecorationGenerator : public NiRefObject
{
    NiDeclareRootRTTI(NiDecorationGenerator);

public:
    
    /// An STL list of smart pointers to NiDecorationFunctorBase objects
    typedef efd::list<NiDecorationFunctorBasePtr> FunctorList;

    /// Semantic indices of recognized TEXCOORD streams
    enum UV_SET
    {
        /// Regular texture UV's
        UV_MODEL = 0,

        UV_MAX
    };

    /**
        When an NiTexturingProperty is created by the generator, it is given this name. This is
        used to determine whether or not the texturing property applied to a mesh was created by
        the generator, or applied by the user.
     */ 
    static NiFixedString DEFAULT_TEXTURING_PROPERTY_NAME;

    /**
        Parameterized Constructor

        @param bUseInstancing Enable GPU based mesh instancing if available.
    */
    NiDecorationGenerator(bool bUseInstancing = true);

    /// Virtual Destructor
    virtual ~NiDecorationGenerator();
    
    /**
        Performs initialization of the object; should be called immediately after the constructor.
     */
    virtual void Initialize() = 0;

    /**
        Creates a new transform stream and manager to handle the pooling of transform stream 
        regions. This function can be used to grow or shrink an existing instance stream.
     */
    virtual void InitializeTransformStream(NiUInt32 uiInstancesPerCell, NiUInt32 uiRequiredCells) 
        = 0;

    /// @return Name of this generator implementation
    virtual const NiFixedString& GetGeneratorType() = 0;

    /// @cond EMERGENT_INTERNAL
    static void _SDMInit();
    static void _SDMShutdown();
    // @endcond

    /**
        Toggle GPU based mesh instancing if available on current hardware.

        @note This value will only affect meshes.
    */
    //@{
    virtual bool GetUseInstancing() const;
    virtual void SetUseInstancing(bool bUseInstancing);
    //@}

    /**
        Creates the mesh to use as the base for instancing and sets the maximum number of instances 
        that will be available.

        @return pointer to the created base mesh, or NULL if creation failed.
    */
    virtual NiDecorationMeshInfo* CreateBaseMesh(NiUInt32 uiNumCells, NiUInt32 uiInstancesPerCell) 
        = 0;

    /**
        Creates or updates any shader constant extra data objects on the given base that are 
            required by NiDecorationMaterial.

        @param pkBase The base object to which the extra data will be attached
        @param pkLayer The decoration layer that is used to calculate the values of the shader 
            constants
    */
    virtual void UpdateShaderConstants(NiDecorationMeshInfo* pkBase, 
        const NiDecorationLayer* pkLayer);

    /**
        Attempt to attach the default mesh properties to the given base mesh.

        @note The given base mesh must be non-null.
    */
    virtual void UpdatePropertyData(NiDecorationMeshInfo* pkBase);

    /**
        The length of time, in seconds, of a complete animation cycle.
    */
    //@{
    void SetAnimationLoopTime(float fTime);
    float GetAnimationLoopTime() const;
    //@}

    /**
        Time offset into the animation cycle. This allows different generators to start at different
        points in the animation loop.
    */
    //@{
    void SetAnimationOffsetTime(float fTime);
    float GetAnimationOffsetTime() const;
    //@}

    /// Scaler multiplier applied to the base texture shader sampler. For example, a value of 2.0 
    /// will double the color value of each channel
    //@{
    void SetBaseTextureSaturation(float fSaturation);
    float GetBaseTextureSaturation() const;
    //@}

    /**
        Progress the animation of the given mesh according to time.
    */
    virtual void UpdateAnimation(NiDecorationMeshInfo* pkBase, NiUpdateProcess& kProcess) = 0;

    /**
        This function will calculate the specified number of transforms for the given cell and write
        them to the given transform stream. It will first try to create the transforms using the 
        given functors; if no compatible functors exist and the fallback transform method is enabled
        then all instances are placed evenly through the cell with zeroed Z translation.

        It is possible that a compatible functor exists, yet no valid transformations could be
        calculated due to functor target settings. In this case, no transforms will be assigned
        to the given transform stream and the function will return false. If false, it is safe (and
        desirable) to skip rendering the cell.
 
        @note This function is stable, i.e. it will always produce the same set of transformations
            for identical function inputs.

        @param kFunctorSet Set of functors to use for transform calculation.
        @param auiCellCount Number of cells along the X and Y edges (respectively) of the cells 
            owning layer.
        @param pkCell The cell for which transforms will be calculated.
        @param pkTransforms Destination transform stream. Must have at least uiTransformCount 
            values.
        @param uiTransformCount Number of values in the given pkTransforms stream.
        @param uiSeed Random number generator seed.
        @param kRange The dimensions of the owning layer, in model space.
        @param uiFieldIndex User specified value given to the owning NiDecorationField.
        @param kWorldTransform

        @return true if the given cell generated at least 1 valid transform, false if no transforms
            were generated.
    */
    virtual bool GenerateTransforms(
        const FunctorList& kFunctorSet,
        NiUInt32 auiCellCount[2], NiDecorationCell* pkCell, 
        NiTransform* pkTransforms, NiUInt32 uiTransformCount, 
        NiUInt32 uiSeed, const NiPoint2& kRange, NiUInt32 uiFieldIndex,
        const NiTransform& kWorldTransform);

    /**
        Populates the given transform stream with invalid transforms; ie their scale is set to zero.

        @param pkTransforms Transform stream to invalidate
        @param uiTransformCount Number of entries in the given transform stream
        @return The number of transforms that were invalidated
     */
    virtual NiUInt32 GenerateInvalidTransforms(NiTransform* pkTransforms, 
        NiUInt32 uiTransformCount);

    /// If no valid functors are available when generating transforms, this the fallback transform
    /// method will place all instances on a flat plane
    //@{
    bool GetFallbackTransformsEnabled() const;
    void SetFallbackTransformsEnabled(bool bFallbacksEnabled);
    //@}

    /**
        Process any queued cell releases or requests on the transform manager.
     */
    virtual void ProcessChangedCells();
    
    /**
        Active instance transform manager. All cell release and requests should come through this
        object.
     */
    NiDecorationTransformManager* GetTransformManager() const;

protected:

    /**
        Set the number of decoration instances that should be rendered on the given mesh.

        When GPU mesh instancing is disabled, this function will clone the base mesh to create the 
        number of required instances if they do not exist; however will not delete any surplus 
        instances.

        @note uiInstanceCount MUST be less than or equal to the maximum number of instances that the
            transform manager has allocated.
    */
    virtual void SetActiveInstanceCount(NiDecorationMeshInfo* pkBase, NiUInt32 uiInstanceCount) = 0;

    /**
        Copy 'uiNumCells' entries from the given transforms stream to the relevant mesh instances, 
        starting at the instance at index 'uiStartCell'

        @param pkBase The mesh who's instances should be set
        @param pkTransforms Array containing the source transforms
        @param uiNumCells Number of cells to copy
        @param uiStartCell Index of the first destination cell instance
    */
    virtual void SetTransforms(NiDecorationMeshInfo* pkBase, 
        NiTransform* pkTransforms, 
        NiUInt32 uiNumCells, 
        NiUInt32 uiStartCell = 0) = 0;

    /**
        This function will generate the specified number of transforms for the base mesh instances 
        and assign them to the transforms array given.

        This simple function will not use any functors, and simply set the Z component of the 
        transforms to 0.0

        @note This function is stable, i.e. it will always produce the same set of transformations
        for identical function inputs.

        @return true if the given cell has at least 1 valid transform. If false, no transforms are 
            valid and it is safe to skip rendering the cell.
    */
    virtual bool GenerateTransformsSimple(NiDecorationCell* pkCell, 
        NiTransform* pkTransforms, NiUInt32 uiTransformCount, NiUInt32 uiSeed, 
        const NiPoint2& kRange, const NiTransform& kWorldTransform);

protected:

    NiDecorationTransformManagerPtr m_spTransformStreamManager;

    NiNoiseTexturePtr m_spNoiseTexture;

    NiRandomLCG* m_pkRandom;

    float m_fAnimationLoopTime;
    float m_fAnimationOffsetTime;
    float m_fBaseTextureSaturation;

    bool m_bUseInstancing;
    bool m_bFallbackTransformsEnabled;
};

NiSmartPointer(NiDecorationGenerator);

#endif // NIDECORATIONGENERATOR_H
