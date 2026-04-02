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

#ifndef NIDECORATIONFUNCTOR_H
#define NIDECORATIONFUNCTOR_H


#include "NiDecorationLibType.h"
#include "NiDecorationCell.h"
#include <NiRandomLCG.h>

#include <NiObject.h>
#include <NiPoint2.h>
#include <NiTransform.h>

#include <NiExtraData.h>
#include <NiAVObject.h>

/**
    Virtual base class that acts as a link between a decoration layer and an
    object that acts as a data source for decoration instance positions.

    This is an abstract class that is purely used by the decoration layers to 
    remove inter-library dependencies.
*/
class NIDECORATION_ENTRY NiDecorationFunctorBase : public NiObject
{
public:

    /// Default constructor
    NiDecorationFunctorBase();
    virtual ~NiDecorationFunctorBase();

    /// @see NiTDecorationFunctor::SetTarget
    virtual void SetTarget(NiObject* pkTarget) = 0;

    /// @see NiTDecorationFunctor::GetTarget
    virtual NiObject* GetTarget() const = 0;

    /// @see NiTDecorationFunctor::GetExtraData
    virtual NiTPrimitiveArray<NiExtraData*>& GetExtraData() = 0;

    /// @see NiTDecorationFunctor::Validate
    virtual bool Validate(
        const NiUInt32 auiCellCount[2], 
        const NiPoint2& kCellRange,
        const NiTransform& kLayerWorldTransform) = 0;

    /// @see NiTDecorationFunctor::ConfigureMesh
    virtual void ConfigureMesh(const NiUInt32 auiCellCount[2], 
        const NiPoint2& kCellRange,
        NiUInt32 uiFieldIndex, 
        const NiTransform& kLayerWorldTransform, 
        NiAVObject* pkBase) const = 0;

    /// @see NiTDecorationFunctor::GenerateTransforms
    virtual bool GenerateTransforms( 
        const NiUInt32 auiCellIndex[2], 
        NiDecorationCell* pkCell, 
        NiTransform* pkTransforms, 
        NiUInt32 uiTransformCount, 
        const NiPoint2& kRange, 
        NiUInt32 uiFieldIndex,
        const NiTransform& kWorldTransform, 
        NiRandomLCG* pkRandom) = 0;
};

NiSmartPointer(NiDecorationFunctorBase);

/**
    Templated functor that is used to link an arbitrary instance position data
    source to the decoration generator. The template parameter T corresponds to
    a class containing static functions, as described in "Decoration Functors"
*/
template <class T>
class NiTDecorationFunctor : public NiDecorationFunctorBase
{
    NiDeclareClone(NiTDecorationFunctor<T>);

public:

    /// Default constructor
    NiTDecorationFunctor();

    /**
        Parameterized Constructor

        @param pkTarget Data source that the functor implementation class T
        will reference when creating instance positions.
    */
    NiTDecorationFunctor(typename T::target_type* pkTarget);
    ~NiTDecorationFunctor();

    /**
        Simple check to see if the given NiObject can be cast to the type
        required by the implementation class T.

        The required type is T::target_type
    */
    static bool IsValidTargetType(NiObject* pkTarget);

    /**
        Data source for this functor that will be referenced when generating transforms

        @param pkTarget Data source that the functor implementation class T
            will reference when creating instance positions.
    */
    // @{
    virtual void SetTarget(NiObject* pkTarget);
    virtual NiObject* GetTarget() const;
    // @}

    /**
        Thorough check to ensure that the data source provided upon this 
        functors initialization is compatible with the given layer parameters.

        @param auiCellCount Number of cells along the X and Y edges (respectively) of the owning
            layer
        @param kCellRange The length and breadth (x and y) in layer coordinate space of all cells 
            within the layer
        @param kLayerWorldTransform The transformation in world space of the layer

        @return True if all relevant checks pass, false otherwise.
    */
    virtual bool Validate(
        const NiUInt32 auiCellCount[2],
        const NiPoint2& kCellRange,
        const NiTransform& kLayerWorldTransform);

    /**
        Apply any additional properties or effects to the base mesh. This is called once, after the
        mesh is created.

        @param auiCellCount Number of cells along the X and Y edges (respectively) of the owning
            layer
        @param kCellRange The length and breadth (x and y) in layer coordinate 
            space of all cells within the layer
        @param uiFieldIndex The user specified 'field index' of the owning layers parent field.
        @param kLayerWorldTransform The transformation in world space of the layer
        @param pkBase Mesh to configure
     */
    virtual void ConfigureMesh(const NiUInt32 auiCellCount[2], 
        const NiPoint2& kCellRange,
        NiUInt32 uiFieldIndex, 
        const NiTransform& kLayerWorldTransform, 
        NiAVObject* pkBase) const;

    /**
        Return a read only array containing all current extra data
    */
    virtual NiTPrimitiveArray<NiExtraData*>& GetExtraData();

    /**
        Use the functor implementation class T to populate the given cells transform stream with 
        instance transform data, using this functors data source.

        @param auiCellCount Number of cells along the X and Y edges (respectively) of the owning
            layer
        @param pkCell Cell containing the transform stream to be updated.
        @param pkTransforms Destination transform stream. Must have at least uiTransformCount 
            values.
        @param uiTransformCount Number of values in the given pkTransforms stream.
        @param kRange The length and breadth (x and y) in layer coordinate space of all cells within
            the layer.
        @param uiFieldIndex User specified value given to the owning NiDecorationField.
        @param kWorldTransform World transform of the layer the given cell is contained within.
        @param pkRandom Seeded random number generator to use for transformation generation. Must be
            initialized with the seed to be used.

        @return true if all instances were successfully placed, false otherwise
    */
    virtual bool GenerateTransforms(const NiUInt32 auiCellCount[2],
        NiDecorationCell* pkCell, 
        NiTransform* pkTransforms, 
        NiUInt32 uiTransformCount,
        const NiPoint2& kRange, 
        NiUInt32 uiFieldIndex,
        const NiTransform& kWorldTransform,
        NiRandomLCG* pkRandom);

protected:
    NiTPrimitiveArray<NiExtraData*> m_kExtraData;
    typename T::target_type* m_pkTarget;
};

#include "NiDecorationFunctor.inl"

#endif // NIDECORATIONFUNCTOR
