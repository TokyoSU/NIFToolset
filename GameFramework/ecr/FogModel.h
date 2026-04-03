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

#pragma once
#ifndef EE_FOGMODEL_H
#define EE_FOGMODEL_H

#include "ecrLibType.h"

#include <NiFogProperty.h>
#include <ecr/ecrClassIDs.h>
#include <egf/StandardModelLibraryPropertyIDs.h>
#include <egf/StandardModelLibraryFlatModelIDs.h>
#include <egf/BuiltinModelHelper.h>
#include <ecr/CoreRuntimeMessages.h>

namespace ecr
{

/**
    The FogModel class provides the built-in model functionality for the Fog model in the 
    StandardModelLibrary.

    The FogModel is designed to be placed into a scene to apply a global fog across all the 
    entities within the scene. 

    The FogModel class holds an internal NiFogProperty that is applied to all the scene graphs
    of entities that have the "IsFogApplied" property of the "Renderable" model set to "true". 
    The values of this model directly reflect the values stored in the NiFogProperty and thus
    have the same effects.
*/
class EE_ECR_ENTRY FogModel
    : public egf::IBuiltinModelImpl
    , public egf::IPropertyCallback
{
    /// @cond EMERGENT_INTERNAL

    EE_DECLARE_CLASS1(
        FogModel,
        efd::kCLASSID_FogModel,
        egf::IBuiltinModelImpl);
    EE_DECLARE_CONCRETE_REFCOUNT;

    // Define the properties that are built in
    EE_DECLARE_BUILTINMODEL_PROPERTIES
       
        EE_BUILTINMODELPROPERTY_ACCESSOR(
            FogModel,
            egf::kPropertyID_StandardModelLibrary_FogEnabled,
            bool,
            FogModel,
            GetEnabled,
            SetEnabled)

        EE_BUILTINMODELPROPERTY_ACCESSOR(
            FogModel,
            egf::kPropertyID_StandardModelLibrary_FogFunction,
            efd::utf8string,
            FogModel,
            GetFunction,
            SetFunction)

        EE_BUILTINMODELPROPERTY_ACCESSOR(
            FogModel,
            egf::kPropertyID_StandardModelLibrary_FogDepth,
            efd::Float32,
            FogModel,
            GetDepth,
            SetDepth)

        EE_BUILTINMODELPROPERTY_ACCESSOR(
            FogModel,
            egf::kPropertyID_StandardModelLibrary_FogColor,
            efd::Color,
            FogModel,
            GetColor,
            SetColor)
       
    EE_END_BUILTINMODEL_PROPERTIES;

    /// @endcond

public:

    /// Constructor sets all the default properties
    FogModel();
    
    /// Virtual destructor.
    virtual ~FogModel();

    /**
        The factory creation function for the Fog built-in model.

        It is public to allow the ecr::SceneGraphService to register the factory in its
        PreInit method.
    */
    static egf::IBuiltinModel* FogModelFactory();

    /**
        @name Built-in Model Functionality
    */
    // @{
    
    /**
        Registers the entity with the ecr::SceneGraphService.
    */
    virtual void OnAdded();

    /**
        Removes the entity from the ecr::SceneGraphService.
    */
    virtual void OnRemoved();

    /**
        Checks for equality of all properties.
    */
    virtual bool operator==(const IProperty& other) const;

    // @}

    /// @name Property Accessors

    // @{

    /**
        Get the FogEnabled property.
    */
    bool GetEnabled() const;

    /**
        Set the FogEnabled property.
    */
    void SetEnabled(const bool& bEnabled);

    /**
        Get the FogFunction property.
    */
    efd::utf8string GetFunction() const;

    /**
        Set the FogFunction property.
    */
    void SetFunction(const efd::utf8string& fogFunction);

    /**
        Get the FogDepth property.
    */
    efd::Float32 GetDepth() const;

    /**
        Set the FogDepth property.
    */
    void SetDepth(const efd::Float32& depth);

    /**
        Get the FogColor property.
    */
    efd::Color GetColor() const;

    /**
        Set the FogColor property.
    */
    void SetColor(const efd::Color& color);

    // @}

    /**
        Callback for built-in model property changes.

        The builtin registers itself to get property update callbacks
        from the RenderableModel. This function is invoked to apply the property change.
    */
    virtual void OnPropertyUpdate(
        const egf::FlatModelID& modelID,
        egf::Entity* pEntity,
        const egf::PropertyID& propertyID,
        const egf::IProperty* pProperty,
        const efd::UInt32 tags);

    /**
        Retrieve the fog property that this fog model is manipulating.
    */
    NiFogProperty* GetFogProperty();

    /// @cond EMERGENT_INTERNAL

    /// -- INTERNAL MESSAGE HANDLING -- 
    void HandleSceneGraphAddedMessage(const ecr::SceneGraphAddedMessage* pMessage,
        efd::Category targetChannel);
    void HandleSceneGraphRemovedMessage(const ecr::SceneGraphRemovedMessage* pMessage,
        efd::Category targetChannel);

    /// @endcond

protected:
    
    /**
        An entity scene graph functor used to iterate over each scene graph and
        attach or detach the fog property of the given FogModel object.
    */
    class AttachFogFunctor : public ecr::SceneGraphService::EntitySceneGraphFunctor
    {
    public:

        /**
            Constructor

            @param pFogModel The Fog model to apply to the scene graph
            @param bDetach True if the model needs to be detached from the scene graphs
        */
        AttachFogFunctor(FogModel* pFogModel, bool bDetach);

        /**
            Execution operator for the scene graph functor to perform it's work upon each 
            scenegraph in the scene. 

            @param pEntity The entity being manipulated
            @param objects The objects the entity is responsible for
        */
        efd::Bool operator()(const egf::Entity* pEntity, const efd::vector<NiObjectPtr>& objects);

    private:

        /// The fog model to apply
        FogModel* m_pFogModel;
        /// Should the model be detached or attached
        bool m_detach;
    };

    /// Find all the scene graphs to be fogged and attach to them
    void AttachAllSceneGraphs();
    /// Detach the fog property from all scene graphs
    void DetachAllSceneGraphs();
    /// Attach the fogging property to the scene graph
    void AttachSceneGraph(const egf::Entity* pEntity, NiAVObject* pNode);
    /// Detach the fogging property from the scene graph
    void DetachSceneGraph(const egf::Entity* pEntity, NiAVObject* pNode);
    /// Is this entity affected by fog?
    bool IsAffectedByFog(const egf::Entity* pEntity);

    /// The fog property this model is responsible for
    NiFogPropertyPtr m_spFogProperty;
};

}; // namespace ecr

#endif // EE_FOGMODEL_H
