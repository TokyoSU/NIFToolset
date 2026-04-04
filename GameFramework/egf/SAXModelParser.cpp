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

#include "egfPCH.h"

#include <efd/File.h>
#include <egf/SAXModelParser.h>
#include <egf/FlatModelManager.h>
#include <egf/PrimitiveProperties.h>
#include <egf/UtilityProperties.h>
#include <efd/ILogger.h>
#include <egf/egfLogIDs.h>

using namespace efd;
using namespace egf;

//------------------------------------------------------------------------------------------------
struct egf::XmlModelStringConstants : public efd::MemObject
{
    EE_TIXML_STRING kTagModelGroup;

    EE_TIXML_STRING kTagModel;
    EE_TIXML_STRING kAttrModelID;
    EE_TIXML_STRING kAttrModelName;

    EE_TIXML_STRING kTagProperty;
    EE_TIXML_STRING kAttrPropertyID;
    EE_TIXML_STRING kAttrPropertyName;
    EE_TIXML_STRING kAttrPropertyType;
    EE_TIXML_STRING kAttrPropertySource;

    EE_TIXML_STRING kTagSet;
    EE_TIXML_STRING kAttrSetIndex;
    EE_TIXML_STRING kAttrSetValue;

    EE_TIXML_STRING kTagTrait;
    EE_TIXML_STRING kAttrTraitValue;

    EE_TIXML_STRING kTagBehavior;
    EE_TIXML_STRING kAttrBehaviorID;
    EE_TIXML_STRING kAttrBehaviorName;
    EE_TIXML_STRING kAttrBehaviorSource;
    EE_TIXML_STRING kAttrBehaviorType;

    EE_TIXML_STRING kTagMixin;
    EE_TIXML_STRING kAttrMixinID;
    EE_TIXML_STRING kAttrMixinName;
    EE_TIXML_STRING kAttrMixinType;

    EE_TIXML_STRING kTagInvocationOrder;

    EE_TIXML_STRING kTagInvocationOrderModel;
    EE_TIXML_STRING kAttrInvocationOrderModelName;

    EE_TIXML_STRING kTagExtraData;
    EE_TIXML_STRING kAttrExtraDataType;
    EE_TIXML_STRING kAttrExtraDataName;
    EE_TIXML_STRING kTagExtraDataEntry;
    EE_TIXML_STRING kAttrExtraDataEntryType;
    EE_TIXML_STRING kAttrExtraDataEntryKey;
    EE_TIXML_STRING kAttrExtraDataEntryValue;

    XmlModelStringConstants() :
        kTagModelGroup("modelGroup"),

        kTagModel("model"),
        kAttrModelID("id"),
        kAttrModelName("name"),

        kTagProperty("property"),
        kAttrPropertyID("id"),
        kAttrPropertyName("name"),
        kAttrPropertyType("type"),
        kAttrPropertySource("source"),

        kTagSet("set"),
        kAttrSetIndex("index"),
        kAttrSetValue("value"),

        kTagTrait("trait"),
        kAttrTraitValue("value"),

        kTagBehavior("behavior"),
        kAttrBehaviorID("id"),
        kAttrBehaviorName("name"),
        kAttrBehaviorSource("source"),
        kAttrBehaviorType("type"),

        kTagMixin("mixin"),
        kAttrMixinID("id"),
        kAttrMixinName("name"),
        kAttrMixinType("type"),

        kTagInvocationOrder("invocation-order"),

        kTagInvocationOrderModel("model"),
        kAttrInvocationOrderModelName("name"),

        kTagExtraData("extradata"),
        kAttrExtraDataType("type"),
        kAttrExtraDataName("name"),
        kTagExtraDataEntry("entry"),
        kAttrExtraDataEntryType("type"),
        kAttrExtraDataEntryKey("key"),
        kAttrExtraDataEntryValue("value")
    {
    }
};

/*static*/ XmlModelStringConstants* SAXModelParser::ms_constants;

//------------------------------------------------------------------------------------------------
/*static*/ void SAXModelParser::_SDMInit()
{
    ms_constants = EE_NEW XmlModelStringConstants();
}

//------------------------------------------------------------------------------------------------
/*static*/ void SAXModelParser::_SDMShutdown()
{
    EE_DELETE ms_constants;
}

//------------------------------------------------------------------------------------------------
SAXModelParser::SAXModelParser(FlatModelManager& pModelManager)
: m_modelFile("")
, m_errors(0)
, m_tempPropertyDescriptor(NULL)
, m_fTraitsFinalized(false)
, m_propIsEntityRef(false)
{
    m_pFlatModelManager = &pModelManager;
    m_parserState = NotParsing;

    // DT32401 We should support user defined extensions to the valid trait names.  Currently
    // you must edit this file to add custom traits.

    m_modelTraitMap["Active"] = ModelTrait_Active;
    m_modelTraitMap["BuiltinModel"] = ModelTrait_BuiltinModel;
    m_modelTraitMap["ReplicaBuiltinModel"] = ModelTrait_ReplicaBuiltinModel;
    m_modelTraitMap["ServerOnlyReplicaBuiltinModel"] = ModelTrait_ServerOnlyReplicaBuiltinModel;
    m_modelTraitMap["ClientOnlyReplicaBuiltinModel"] = ModelTrait_ClientOnlyReplicaBuiltinModel;
    m_modelTraitMap["ServerOnlyBuiltinModel"] = ModelTrait_ServerOnlyBuiltinModel;
    m_modelTraitMap["ClientOnlyBuiltinModel"] = ModelTrait_ClientOnlyBuiltinModel;
    m_modelTraitMap["ToolBuiltinModel"] = ModelTrait_ToolBuiltinModel;
    // The "Abstract" trait is only for Toolbench use and has no runtime meaning:
    m_modelTraitMap["Abstract"] = ModelTrait_None;

    m_propertyTraitMap["ReadOnly"] = PropertyTrait_ReadOnly ;
    m_propertyTraitMap["Persisted"] = PropertyTrait_Persisted;
    m_propertyTraitMap["Private"] = PropertyTrait_Private;
    m_propertyTraitMap["BuiltinModel"] = PropertyTrait_FromBuiltinModel;
    m_propertyTraitMap["FromBuiltinModel"] = PropertyTrait_FromBuiltinModel;
    m_propertyTraitMap["FromReplicaBuiltinModel"] = PropertyTrait_FromReplicaBuiltinModel;
    m_propertyTraitMap["Mutable"] = PropertyTrait_Mutable;
    m_propertyTraitMap["ClientOnly"] = PropertyTrait_ClientOnly;
    m_propertyTraitMap["ServerOnly"] = PropertyTrait_ServerOnly;

    m_propertyReplicationGroupMap["ReplicationGroup00"] = 0;
    m_propertyReplicationGroupMap["ReplicationGroup01"] = 1;
    m_propertyReplicationGroupMap["ReplicationGroup02"] = 2;
    m_propertyReplicationGroupMap["ReplicationGroup03"] = 3;
    m_propertyReplicationGroupMap["ReplicationGroup04"] = 4;
    m_propertyReplicationGroupMap["ReplicationGroup05"] = 5;
    m_propertyReplicationGroupMap["ReplicationGroup06"] = 6;
    m_propertyReplicationGroupMap["ReplicationGroup07"] = 7;
    m_propertyReplicationGroupMap["ReplicationGroup08"] = 8;
    m_propertyReplicationGroupMap["ReplicationGroup09"] = 9;
    m_propertyReplicationGroupMap["ReplicationGroup10"] = 10;
    m_propertyReplicationGroupMap["ReplicationGroup11"] = 11;
    m_propertyReplicationGroupMap["ReplicationGroup12"] = 12;
    m_propertyReplicationGroupMap["ReplicationGroup13"] = 13;
    m_propertyReplicationGroupMap["ReplicationGroup14"] = 14;
    m_propertyReplicationGroupMap["ReplicationGroup15"] = 15;
    m_propertyReplicationGroupMap["ReplicationGroup16"] = 16;
    m_propertyReplicationGroupMap["ReplicationGroup17"] = 17;
    m_propertyReplicationGroupMap["ReplicationGroup18"] = 18;
    m_propertyReplicationGroupMap["ReplicationGroup19"] = 19;
    m_propertyReplicationGroupMap["ReplicationGroup20"] = 20;
    m_propertyReplicationGroupMap["ReplicationGroup21"] = 21;
    m_propertyReplicationGroupMap["ReplicationGroup22"] = 22;
    m_propertyReplicationGroupMap["ReplicationGroup23"] = 23;
    m_propertyReplicationGroupMap["ReplicationGroup24"] = 24;
    m_propertyReplicationGroupMap["ReplicationGroup25"] = 25;
    m_propertyReplicationGroupMap["ReplicationGroup26"] = 26;
    m_propertyReplicationGroupMap["ReplicationGroup27"] = 27;
    m_propertyReplicationGroupMap["ReplicationGroup28"] = 28;
    m_propertyReplicationGroupMap["ReplicationGroup29"] = 29;
    m_propertyReplicationGroupMap["ReplicationGroup30"] = 30;
    m_propertyReplicationGroupMap["ReplicationGroup31"] = 31;

    m_behaviorTraitMap["Private"] = BehaviorTrait_Private;
    m_behaviorTraitMap["ViewOnly"] = BehaviorTrait_ViewOnly;
    m_behaviorTraitMap["NoBlock"] = BehaviorTrait_NoBlock;
    m_behaviorTraitMap["ServerOnly"] = BehaviorTrait_ServerExecOnly;
    m_behaviorTraitMap["ServerExecOnly"] = BehaviorTrait_ServerExecOnly;
    m_behaviorTraitMap["ClientOnly"] = BehaviorTrait_ClientExecOnly;
    m_behaviorTraitMap["ClientExecOnly"] = BehaviorTrait_ClientExecOnly;
    m_behaviorTraitMap["RemotelyVisible"] = BehaviorTrait_RemotelyVisible;
    m_behaviorTraitMap["Extends"] = BehaviorTrait_Extends;
    // NOTE: as far as the run-time is concerned, reverse extends is identical to extends.  The
    // flat model already lists the invocation order backwards for reverse extends so nothing
    // else is different.
    m_behaviorTraitMap["ReverseExtends"] = BehaviorTrait_Extends;
    m_behaviorTraitMap["Immediate"] = BehaviorTrait_Immediate;
    m_behaviorTraitMap["InWorldOnly"] = BehaviorTrait_InWorldOnly;

    // NOTE: The "Init" trait is not a valid trait to read from the data file, its for internal
    // use only.  So we do not want parsing for that trait here.

    m_behaviorTypeMap["C"] = BehaviorType_C;
    m_behaviorTypeMap["Cpp"] = BehaviorType_Cpp;
    m_behaviorTypeMap["Python"] = BehaviorType_Python;
    m_behaviorTypeMap["Builtin"] = BehaviorType_Builtin;
    m_behaviorTypeMap["Remote"] = BehaviorType_Remote;
    m_behaviorTypeMap["Virtual"] = BehaviorType_Virtual;
    m_behaviorTypeMap["Lua"] = BehaviorType_Lua;
    m_behaviorTypeMap["Abstract"] = BehaviorType_Abstract;

    // These traits where technically supported in LightSpeed 3.0 even though ToolBench never
    // displayed them. They were renamed in LightSpeed 3.1 when they become supported. We still
    // parse the old names just in case.
    m_modelTraitMap["ViewBuiltinModel"] = ModelTrait_ReplicaBuiltinModel;
    m_modelTraitMap["ServerOnlyViewBuiltinModel"] = ModelTrait_ServerOnlyReplicaBuiltinModel;
    m_modelTraitMap["ClientOnlyViewBuiltinModel"] = ModelTrait_ClientOnlyReplicaBuiltinModel;
    // The Controller trait was been removed, not renamed
    m_modelTraitMap["Controller"] = ModelTrait_None;

    m_propertyTraitMap["FromViewBuiltinModel"] = PropertyTrait_FromReplicaBuiltinModel;
}


//------------------------------------------------------------------------------------------------
SAXModelParser::~SAXModelParser()
{
    // nothing to release -- everything is owned by other classes.
}


//------------------------------------------------------------------------------------------------
FlatModelPtr SAXModelParser::Parse(const efd::utf8string& modelFile)
{
    bool parseOK = false;
    // Remember the file name for use in log statements
    m_modelFile = modelFile;

    // create a SAX parser
    TiXmlDocument document(modelFile.c_str());

    File* pkFile = efd::File::GetFile(document.Value(), File::READ_ONLY);
    if (pkFile)
    {
        unsigned int uiSize = pkFile->GetFileSize();
        char* pBuffer = EE_ALLOC(char, uiSize + 1);
        pkFile->Read(pBuffer, uiSize);
        pBuffer[uiSize] = '\0';
        parseOK = document.SAXParseBuffer(pBuffer, this);
        EE_FREE(pBuffer);
        EE_DELETE(pkFile);
    }
    else
    {
        EE_LOG(efd::kEntity, efd::ILogger::kERR0,
            ("Failed to open model file: %s", modelFile.c_str()));
    }

    if (!parseOK)
    {
        EE_LOG(efd::kEntity, efd::ILogger::kERR0,
            ("Model file parse failure. File: \"%s\" Error: \"%s\"",
            m_modelFile.c_str(), document.ErrorDesc()));
        m_spFlatModel = NULL;
    }

    return m_spFlatModel;
}

#if defined(WIN32)
//------------------------------------------------------------------------------------------------
FlatModelPtr SAXModelParser::ParseBuffer(
    const efd::utf8string& virtualModelFile,
    const char* pBuffer)
{
    bool parseOK = false;
    // Remember the file name for use in log statements
    m_modelFile = virtualModelFile;

    // create a SAX parser
    TiXmlDocument kDocument (virtualModelFile.c_str());

    parseOK = kDocument.SAXParseBuffer(pBuffer, this);

    if (!parseOK)
    {
        m_spFlatModel = NULL;
    }
    return m_spFlatModel;
}
#endif

//------------------------------------------------------------------------------------------------
void SAXModelParser::startElement(const EE_TIXML_STRING& localname,
                                   const TiXmlAttributeSet& attrs)
{
    // switch on the current state of the parser
    switch (m_parserState)
    {
    case NotParsing:
        // First tag must be model.  If something else comes first, then that's an error.
        if (localname == ms_constants->kTagModel)
        {
            m_parserState = InModel;
            ParseModel(attrs);
        }
        else
        {
            EE_LOG(efd::kEntity, efd::ILogger::kERR0,
                ("Error parsing flat model file: '%s'.  Expected a 'model' tag.",
                m_modelFile.c_str()));

            ++m_errors;
        }
        break;

    case InModel:
        // look for either a property, behavior, trait, or mixin tag.  if something else
        // comes first, then that's an error
        if (localname == ms_constants->kTagTrait)
        {
            m_parserState = InModelTrait;
            ParseModelTrait(attrs);
        }
        else if (localname == ms_constants->kTagMixin)
        {
            m_parserState = InMixin;
            ParseMixin(attrs);
        }
        else if (localname == ms_constants->kTagProperty)
        {
            m_parserState = InProperty;
            ParseProperty(attrs);
        }
        else if (localname == ms_constants->kTagBehavior)
        {
            m_parserState = InBehavior;
            ParseBehavior(attrs);
        }
        else if (localname == ms_constants->kTagExtraData)
        {
            m_parserState = InExtraData;
            ParseExtraData(attrs);
        }
        else
        {
            EE_LOG(efd::kEntity, efd::ILogger::kERR0,
                ("Error parsing flat model file: '%s'.  "
                "Expected a 'behavior', 'mixin', 'property', or 'trait' tag.",
                m_modelFile.c_str()));

            ++m_errors;
        }
        break;

    case InProperty:
        // look for a set or trait tag.  if something else comes first,
        // then that's an error
        if (localname == ms_constants->kTagSet)
        {
            m_parserState = InPropertySet;
            ParsePropertyValue(attrs);
        }
        else if (localname == ms_constants->kTagTrait)
        {
            m_parserState = InPropertyTrait;
            ParsePropertyTrait(attrs);
        }
        else
        {
            EE_LOG(efd::kEntity, efd::ILogger::kERR0,
                ("Error parsing flat model file: '%s'.  Expected a 'set' or 'trait' tag.",
                m_modelFile.c_str()));

            ++m_errors;
        }
        break;

    case InPropertySet:
        // should never get here, no sub-tags within a set value
        EE_LOG(efd::kEntity, efd::ILogger::kERR0,
            ("Error parsing flat model file: '%s'.  Found a tag within a 'set' tag.",
            m_modelFile.c_str()));

        ++m_errors;
        break;

    case InBehavior:
        if (localname == ms_constants->kTagTrait)
        {
            m_parserState = InBehaviorTrait;
            ParseBehaviorTrait(attrs);
        }
        else if (localname == ms_constants->kTagInvocationOrder)
        {
            m_parserState = InBehaviorInvocationOrder;
            ParseBehaviorInvocationOrder(attrs);
        }
        else
        {
            // Should never get here, no sub-tags within a behavior
            EE_LOG(efd::kEntity, efd::ILogger::kERR0,
                ("Error parsing flat model file: '%s'.  "
                "Expected a 'trait' or 'invocation-order' tag.",
                m_modelFile.c_str()));

            ++m_errors;
        }
        break;

    case InModelTrait:
    case InPropertyTrait:
    case InBehaviorTrait:
        // Should never get here, no sub-tags within a trait
        EE_LOG(efd::kEntity, efd::ILogger::kERR0,
            ("Error parsing flat model file: '%s'.  Found a tag within a 'trait' tag.",
            m_modelFile.c_str()));

        ++m_errors;
        break;

    case InBehaviorInvocationOrder:
        if (localname == ms_constants->kTagInvocationOrderModel)
        {
            m_parserState = InBehaviorInvocationOrderModel;
            ParseBehaviorInvocationOrderModel(attrs);
        }
        break;

    case InMixin:
        // should never get here, no sub-tags within a mixin value
        EE_LOG(efd::kEntity, efd::ILogger::kERR0,
            ("Error parsing flat model file: '%s'.  Found a tag within a 'mixin' tag.",
            m_modelFile.c_str()));

        ++m_errors;
        break;

    case InExtraData:
        if (localname == ms_constants->kTagExtraDataEntry)
        {
            m_parserState = InExtraDataEntry;
            ParseExtraDataElement(attrs);
        }
        else
        {
            EE_LOG(efd::kEntity, efd::ILogger::kERR0,
                ("Error parsing flat model file: '%s'.   Expected a 'pair' tag.",
                m_modelFile.c_str()));

            ++m_errors;
        }
        break;

    default:
        // This should be unreachable
        EE_LOG(efd::kEntity, efd::ILogger::kERR0,
            ("Error parsing entity set file: '%s'.  Invalid state %d.",
            m_modelFile.c_str(), m_parserState));
        EE_ASSERT(false);
        break;
    }
}


//------------------------------------------------------------------------------------------------
void SAXModelParser::endElement(const EE_TIXML_STRING& localname)
{
    switch (m_parserState)
    {
    case InModel:
        // should only get here at the end of the model
        if (localname == ms_constants->kTagModel)
        {
            FinishedModel();
            m_parserState = NotParsing;
        }
        else
        {
            EE_LOG(efd::kEntity, efd::ILogger::kERR0,
                ("Error parsing flat model file: '%s'.  Expected an ending 'model' tag.",
                m_modelFile.c_str()));

            ++m_errors;
        }
        break;

    case InModelTrait:
        // should only get here at the end of a trait
        if (localname == ms_constants->kTagTrait)
        {
            m_parserState = InModel;
        }
        else
        {
            EE_LOG(efd::kEntity, efd::ILogger::kERR0,
                ("Error parsing flat model file: '%s'.  Expected an ending 'trait' tag.",
                m_modelFile.c_str()));

            ++m_errors;
        }
        break;

    case InProperty:
        // should only get here at the end of a property
        if (localname == ms_constants->kTagProperty)
        {
            FinishedProperty();
            m_parserState = InModel;
        }
        else
        {
            EE_LOG(efd::kEntity, efd::ILogger::kERR0,
                ("Error parsing flat model file: '%s'.  Expected an ending 'property' tag.",
                m_modelFile.c_str()));

            ++m_errors;
        }
        break;

    case InPropertySet:
        // should only get here at the end of a set value tag
        if (localname == ms_constants->kTagSet)
        {
            m_parserState = InProperty;
        }
        else
        {
            EE_LOG(efd::kEntity, efd::ILogger::kERR0,
                ("Error parsing flat model file: '%s'.  Expected an ending 'set' tag.",
                m_modelFile.c_str()));

            ++m_errors;
        }
        break;

    case InPropertyTrait:
        // should only get here at the end of a trait
        if (localname == ms_constants->kTagTrait)
        {
            m_parserState = InProperty;
        }
        else
        {
            EE_LOG(efd::kEntity, efd::ILogger::kERR0,
                ("Error parsing flat model file: '%s'.  Expected an ending 'trait' tag.",
                m_modelFile.c_str()));

            ++m_errors;
        }
        break;

    case InBehavior:
        // should only get here at the end of a behavior
        if (localname == ms_constants->kTagBehavior)
        {
            FinishedBehavior();
            m_parserState = InModel;
        }
        else
        {
            EE_LOG(efd::kEntity, efd::ILogger::kERR0,
                ("Error parsing flat model file: '%s'.  Expected an ending 'behavior' tag.",
                m_modelFile.c_str()));

            ++m_errors;
        }
        break;

    case InBehaviorTrait:
        // should only get here at the end of a trait
        if (localname == ms_constants->kTagTrait)
        {
            m_parserState = InBehavior;
        }
        else
        {
            EE_LOG(efd::kEntity, efd::ILogger::kERR0,
                ("Error parsing flat model file: '%s'.  Expected an ending 'trait' tag.",
                m_modelFile.c_str()));

            ++m_errors;
        }
        break;

    case InBehaviorInvocationOrder:
        // should only get here at the end of an invocation order element
        if (localname == ms_constants->kTagInvocationOrder)
        {
            m_parserState = InBehavior;
        }
        else
        {
            EE_LOG(efd::kEntity, efd::ILogger::kERR0,
                ("Error parsing flat model file: '%s'.  Expected an ending 'invocation-order' tag.",
                m_modelFile.c_str()));

            ++m_errors;
        }
        break;

    case InBehaviorInvocationOrderModel:
        // should only get here at the end of an invocation order model element
        if (localname == ms_constants->kTagInvocationOrderModel)
        {
            m_parserState = InBehaviorInvocationOrder;
        }
        else
        {
            EE_LOG(efd::kEntity, efd::ILogger::kERR0,
                ("Error parsing flat model file: '%s'.  Expected an ending 'model' tag.",
                m_modelFile.c_str()));

            ++m_errors;
        }
        break;

    case InMixin:
        // should only get here at the end of a trait
        if (localname == ms_constants->kTagMixin)
        {
            m_parserState = InModel;
        }
        else
        {
            EE_LOG(efd::kEntity, efd::ILogger::kERR0,
                ("Error parsing flat model file: '%s'.  Expected an ending 'mixin' tag.",
                m_modelFile.c_str()));

            ++m_errors;
        }
        break;

    case InExtraData:
        if (localname == ms_constants->kTagExtraData)
        {
            m_spFlatModel->AddExtraData(m_spExtraData);
            m_parserState = InModel;
        }
        else
        {
            EE_LOG(efd::kEntity, efd::ILogger::kERR0,
                ("Error parsing flat model file: '%s'.  Expected an ending 'extradata' tag.",
                m_modelFile.c_str(), localname.c_str()));

            ++m_errors;
        }
        break;
    case InExtraDataEntry:
        if (localname == ms_constants->kTagExtraDataEntry)
        {
            m_parserState = InExtraData;
        }
        else
        {
            EE_LOG(efd::kEntity, efd::ILogger::kERR0,
                ("Error parsing flat model file: '%s'.  Expected an ending 'entry' tag.",
                m_modelFile.c_str(), localname.c_str()));

            ++m_errors;
        }
        break;
    default:
        // This should be unreachable
        EE_LOG(efd::kEntity, efd::ILogger::kERR0,
            ("Error parsing entity set file: '%s'.  Invalid state %d.",
            m_modelFile.c_str(), m_parserState));
        EE_ASSERT(false);
        break;
    }
}


//------------------------------------------------------------------------------------------------
void SAXModelParser::characters(const EE_TIXML_STRING& chars)
{
    // called for any characters that aren't tags.  This even, apparently, includes whitespace at
    // the document level.  That means if you need to do any meaningful parsing here you MUST
    // pay close attention to the current parser state.

    EE_UNUSED_ARG(chars);
}


//------------------------------------------------------------------------------------------------
void SAXModelParser::ParseModel(const TiXmlAttributeSet& attrs)
{
    // get the model id
    FlatModelID flatModelID = 0;
    if (!XMLUtils::GetAttribute(attrs, ms_constants->kAttrModelID, flatModelID))
    {
        EE_LOG(efd::kEntity, efd::ILogger::kERR0,
            ("Error parsing flat model file: '%s'.  Missing attribute 'id' in 'model' tag.",
            m_modelFile.c_str()));
        ++m_errors;
    }

    utf8string modelName;
    if (!XMLUtils::GetAttribute(attrs, ms_constants->kAttrModelName, modelName))
    {
        EE_LOG(efd::kEntity, efd::ILogger::kERR0,
            ("Error parsing flat model file: '%s'.  Missing attribute 'name' in 'model' tag.",
            m_modelFile.c_str()));
        ++m_errors;
    }

    m_spFlatModel = EE_NEW FlatModel(m_pFlatModelManager);
    m_spFlatModel->SetID(flatModelID);
    m_spFlatModel->SetName(modelName);
}


//------------------------------------------------------------------------------------------------
void SAXModelParser::ParseModelTrait(const TiXmlAttributeSet& attrs)
{
    EE_ASSERT(m_spFlatModel);

    utf8string traitName;
    if (XMLUtils::GetAttribute(attrs, ms_constants->kAttrTraitValue, traitName))
    {
        ModelTraits trait;
        if (FlatModelTraitFromName(traitName, trait))
        {
            m_spFlatModel->SetTrait(trait, true);
        }
        else
        {
            EE_LOG(efd::kEntity, efd::ILogger::kERR0,
                ("Error parsing flat model file: '%s'.  Invalid model trait '%s'.",
                m_modelFile.c_str(), traitName.c_str()));
            ++m_errors;
        }
    }
    else
    {
        EE_LOG(efd::kEntity, efd::ILogger::kERR0,
            ("Error parsing flat model file: '%s'.  Missing attribute 'value' in 'trait' tag.",
            m_modelFile.c_str()));
        ++m_errors;
    }
}


//------------------------------------------------------------------------------------------------
void SAXModelParser::ParseProperty(const TiXmlAttributeSet& attrs)
{
    // get the property ID
    PropertyID propertyID = 0;
    if (!XMLUtils::GetAttribute(attrs, ms_constants->kAttrPropertyID, propertyID))
    {
        EE_LOG(efd::kEntity, efd::ILogger::kERR0,
            ("Error parsing flat model file: '%s'.  Missing attribute 'id' in 'property' tag.",
            m_modelFile.c_str()));
        ++m_errors;
    }

    // get the property name
    utf8string propertyName;
    if (!XMLUtils::GetAttribute(attrs, ms_constants->kAttrPropertyName, propertyName))
    {
        EE_LOG(efd::kEntity, efd::ILogger::kERR0,
            ("Error parsing flat model file: '%s'.  Missing attribute 'name' in 'property' tag.",
            m_modelFile.c_str()));
        ++m_errors;
    }

    // get the property type
    utf8string propertyType;
    if (!XMLUtils::GetAttribute(attrs, ms_constants->kAttrPropertyType, propertyType))
    {
        EE_LOG(efd::kEntity, efd::ILogger::kERR0,
            ("Error parsing flat model file: '%s'.  Missing attribute 'type' in 'property' tag.",
            m_modelFile.c_str()));
        ++m_errors;
    }

    // determine the metatype
    PropertyDescriptor::PropertyMetaType metatype;
    if (0 == propertyType.find("Assoc"))
    {
        metatype = PropertyDescriptor::PropertyMetaType_Assocative;
    }
    else
    {
        metatype = PropertyDescriptor::PropertyMetaType_Scalar;
    }


    // ensure the property type is found in the map
    bool isProxyProperty = false;
    efd::ClassID propertyClassID = PropertyTypeFromName(propertyType);
    if (0 == propertyClassID)
    {
        ServiceManager* pServiceManager = m_pFlatModelManager->GetServiceManager();
        if (!pServiceManager)
        {
            ++m_errors;
            return;
        }
        bool isTool = pServiceManager->IsProgramType(ServiceManager::kProgType_Tool);
        if (isTool)
        {
            // make a proxy property
            isProxyProperty = true;
            EE_LOG(efd::kEntity, efd::ILogger::kLVL1,
                ("Using proxy property for '%s' in flat model file '%s'.",
                propertyType.c_str(), m_modelFile.c_str()));

            if (metatype == PropertyDescriptor::PropertyMetaType_Assocative)
            {
                propertyClassID = StringAssocProperty::CLASS_ID;
            }
            else
            {
                propertyClassID = StringScalarProperty::CLASS_ID;
            }
        }
        else
        {
            EE_LOG(efd::kEntity, efd::ILogger::kERR0,
                ("Error parsing flat model file: '%s'.  Invalid property type '%s'.",
                m_modelFile.c_str(), propertyType.c_str()));
            ++m_errors;
        }
    }

    // create a new property descriptor
    m_tempPropertyDescriptor = EE_NEW PropertyDescriptor();
    m_tempPropertyDescriptor->SetName(propertyName);
    m_tempPropertyDescriptor->SetPropertyID(propertyID);
    m_tempPropertyDescriptor->SetPropertyClassID(propertyClassID);
    m_tempPropertyDescriptor->SetPropertyMetaType(metatype);

    if (isProxyProperty)
    {
        m_tempPropertyDescriptor->SetSemanticType(propertyType);
    }

    // create a new default property
    m_tempDefaultProperty = m_pFlatModelManager->FactoryProperty(propertyClassID);
    if (m_tempDefaultProperty)
    {
        efd::ClassID dataClassID = m_tempDefaultProperty->GetDataType(propertyID);
        m_propIsEntityRef = (dataClassID == CLASS_ID_ENTITYID);
        m_tempPropertyDescriptor->SetDataClassID(dataClassID);

        // Set the default value if this EntityRef. Toolbench sets the proper default but by setting
        // it here we could potentially eliminate it from model files. That is, if an EntityRef
        // property doesn't have 'set value=' it will still get set to its default.
        if (m_propIsEntityRef)
        {
            EntityID defaultEID = kENTITY_INVALID;
            m_tempDefaultProperty->SetValue(propertyID, &defaultEID);
        }
    }
    else
    {
        EE_LOG(efd::kEntity, efd::ILogger::kERR0,
            ("Error parsing flat model file: '%s'.  Cannot create property of type '0x%08X'.",
            m_modelFile.c_str(), propertyClassID));
        ++m_errors;
    }

    // Source is optional and is only valid for FromBuiltinModel properties (which is verified
    // both when that trait is encounter and also in PropDesc::IsValid()).
    utf8string source;
    if (XMLUtils::GetAttribute(attrs, ms_constants->kAttrPropertySource, source))
    {
        m_tempPropertyDescriptor->SetSource(source);
    }
}


//------------------------------------------------------------------------------------------------
void SAXModelParser::ParsePropertyTrait(const TiXmlAttributeSet& attrs)
{
    EE_ASSERT(m_tempPropertyDescriptor);

    utf8string traitName;
    if (XMLUtils::GetAttribute(attrs, ms_constants->kAttrTraitValue, traitName))
    {
        PropertyTraits trait = PropertyTraitFromName(traitName);
        if (trait)
        {
            if (PropertyTrait_FromBuiltinModel == trait &&
                 m_tempPropertyDescriptor->GetSource().empty())
            {
                EE_LOG(efd::kEntity, efd::ILogger::kERR0,
                    ("Error parsing flat model file: '%s'. FromBuiltinModel trait on property "
                    "'%s' requires that the source be specified.",
                    m_modelFile.c_str(), m_tempPropertyDescriptor->GetName().c_str()));
                ++m_errors;
            }
            if (PropertyTrait_FromReplicaBuiltinModel == trait &&
                 m_tempPropertyDescriptor->GetSource().empty())
            {
                EE_LOG(efd::kEntity, efd::ILogger::kERR0,
                    ("Error parsing flat model file: '%s'. FromReplicaBuiltinModel trait on "
                    "property '%s' requires that the source be specified.",
                    m_modelFile.c_str(), m_tempPropertyDescriptor->GetName().c_str()));
                ++m_errors;
            }

            m_tempPropertyDescriptor->SetTrait(trait, true);
        }
        else
        {
            UInt32 replicationGroup = ReplicationGroupFromName(traitName);
            if (0xFFFFFFFF != replicationGroup)
            {
                m_tempPropertyDescriptor->SetUpdateGroup(replicationGroup, true);
            }
            else
            {
                EE_LOG(efd::kEntity, efd::ILogger::kERR0,
                    ("Error parsing flat model file: '%s'.  Invalid property trait '%s' tag.",
                    m_modelFile.c_str(), traitName.c_str()));
                ++m_errors;
            }
        }
    }
    else
    {
        EE_LOG(efd::kEntity, efd::ILogger::kERR0,
            ("Error parsing flat model file: '%s'.  Missing attribute 'value' in 'trait' tag.",
            m_modelFile.c_str()));
        ++m_errors;
    }
}


//------------------------------------------------------------------------------------------------
void SAXModelParser::ParseBehavior(const TiXmlAttributeSet& attrs)
{
    utf8string behaviorName;
    if (!XMLUtils::GetAttribute(attrs, ms_constants->kAttrBehaviorName, behaviorName))
    {
        EE_LOG(efd::kEntity, efd::ILogger::kERR0,
            ("Error parsing flat model file: '%s'.  Missing attribute 'name' in 'behavior' tag.",
            m_modelFile.c_str()));
        ++m_errors;
    }

    BehaviorID behaviorID = 0;
    if (!XMLUtils::GetAttribute(attrs, ms_constants->kAttrBehaviorID, behaviorID))
    {
        EE_LOG(efd::kEntity, efd::ILogger::kERR0,
            ("Error parsing flat model file: '%s'.  Missing attribute 'id' in 'behavior' tag.",
            m_modelFile.c_str()));
        ++m_errors;
    }

    // Who should know how to convert a behavior type name into the type id?  It can be nice if
    // error logging code can use name<->id mapping which implies BehaviorDescriptor should know
    // how to parse these strings into their values.
    utf8string behaviorType;
    if (!XMLUtils::GetAttribute(attrs, ms_constants->kAttrBehaviorType, behaviorType))
    {
        EE_LOG(efd::kEntity, efd::ILogger::kERR0,
            ("Error parsing flat model file: '%s'.  Missing attribute 'type' in 'behavior' tag.",
            m_modelFile.c_str()));
        ++m_errors;
    }

    BehaviorTypes type = BehaviorTypeFromName(behaviorType);
    if (type == BehaviorType_Invalid)
    {
        EE_LOG(efd::kEntity, efd::ILogger::kERR0,
            ("Error parsing flat model file: '%s'.  Invalid 'type' value '%s' in 'behavior' tag.",
            m_modelFile.c_str(),
            behaviorType.c_str()));
        ++m_errors;
    }

    utf8string source;
    if (type != BehaviorType_Virtual &&
        !XMLUtils::GetAttribute(attrs, ms_constants->kAttrBehaviorSource, source))
    {
        EE_LOG(efd::kEntity, efd::ILogger::kERR0,
            ("Error parsing flat model file: '%s'.  Missing attribute 'source' in 'behavior' tag.",
            m_modelFile.c_str()));
        ++m_errors;
    }

    m_spBehaviorDesc = EE_NEW BehaviorDescriptor(behaviorID, behaviorName, source, type);
    EE_ASSERT(m_spBehaviorDesc);
}


//------------------------------------------------------------------------------------------------
void SAXModelParser::ParseBehaviorTrait(const TiXmlAttributeSet& attrs)
{
    EE_ASSERT(m_spBehaviorDesc);

    // get the trait
    utf8string traitName;
    if (XMLUtils::GetAttribute(attrs, ms_constants->kAttrTraitValue, traitName))
    {
        BehaviorTraits trait = BehaviorTraitFromName(traitName);
        if (trait)
        {
            m_spBehaviorDesc->SetTrait(trait, true);
        }
        else
        {
            EE_LOG(efd::kEntity, efd::ILogger::kERR0,
                ("Error parsing flat model file: '%s'.  Invalid behavior trait '%s' tag.",
                m_modelFile.c_str(), traitName.c_str()));
            ++m_errors;
        }
    }
    else
    {
        EE_LOG(efd::kEntity, efd::ILogger::kERR0,
            ("Error parsing flat model file: '%s'.  Missing attribute 'value' in 'trait' tag.",
            m_modelFile.c_str()));
        ++m_errors;
    }
}


//------------------------------------------------------------------------------------------------
void SAXModelParser::ParseBehaviorInvocationOrder(const TiXmlAttributeSet& attrs)
{
    EE_UNUSED_ARG(attrs);
}


//------------------------------------------------------------------------------------------------
void SAXModelParser::ParseBehaviorInvocationOrderModel(const TiXmlAttributeSet& attrs)
{
    EE_ASSERT(m_spBehaviorDesc);

    // get the name
    utf8string modelName;
    if (XMLUtils::GetAttribute(attrs, ms_constants->kAttrInvocationOrderModelName, modelName))
    {
        if (modelName.length() > 0)
        {
            m_spBehaviorDesc->AddInvocationOrderedModelName(modelName);
        }
        else
        {
            EE_LOG(efd::kEntity, efd::ILogger::kERR0,
                ("Error parsing flat model file: '%s'.  "
                "Invalid behavior invocation order model name (null).",
                m_modelFile.c_str()));
            ++m_errors;
        }
    }
    else
    {
        EE_LOG(efd::kEntity, efd::ILogger::kERR0,
            ("Error parsing flat model file: '%s'.  "
            "Missing attribute 'name' in 'invocation-order' tag.",
            m_modelFile.c_str()));
        ++m_errors;
    }
}


//------------------------------------------------------------------------------------------------
void SAXModelParser::ParsePropertyValue(const TiXmlAttributeSet& attrs)
{
    EE_ASSERT(m_tempPropertyDescriptor);
    if (!m_tempDefaultProperty)
    {
        // This will have already been counted as an error, but lets not crash.
        EE_LOG(efd::kEntity, efd::ILogger::kERR0,
            ("Error parsing flat model file: '%s'.  No IProperty, skipping property value set.",
            m_modelFile.c_str()));
        return;
    }

    // index is required for Assoc properties.  Why don't we check the property any enforce
    // that this is set when its needed?  Would need to add a way to check for scalar vs assoc.
    utf8string index;
    XMLUtils::GetAttribute(attrs, ms_constants->kAttrSetIndex, index);

    utf8string value;
    if (XMLUtils::GetAttribute(attrs, ms_constants->kAttrSetValue, value))
    {
        PropertyResult result;
        if (m_propIsEntityRef)
        {
            // EntityRef has a special requirement that it can only be set to 0 in the model files
            // as entity ids have only meaning when the entities are instantiated (so, it's okay
            // to have entity ids set in the block file).
            // The latest model files have entityref ids set to the invalid entity id in the GUID
            // form: "00000000-0000-0000-0000-000000000000". Old files may still have "0" instead.
            // Here we handle both cases. Note that we won't actually set it to the invalid
            // entity id value as it's been already set when the property was identified as
            // the entityref type.
            ID128 dest;
            EntityID eid;
            if (ParseHelper<ID128>::FromString(value, dest))
            {
                if (dest.IsValid())
                {
                    EE_LOG(efd::kEntity, efd::ILogger::kERR0,
                        ("Error parsing flat model file: '%s'.  "
                         "Failed to set property '%s' to value '%s', "
                         "EntityRef can only be set to 00000000-0000-0000-0000-000000000000",
                        m_modelFile.c_str(), m_tempPropertyDescriptor->GetName().c_str(),
                        value.c_str()));
                    ++m_errors;
                }
            }
            else if (ParseHelper<EntityID>::FromString(value, eid))
            {
                if (eid.IsValid())
                {
                    EE_LOG(efd::kEntity, efd::ILogger::kERR0,
                        ("Error parsing flat model file: '%s'.  "
                         "Failed to set property '%s' to value '%s', "
                         "EntityRef can only be set to 0",
                        m_modelFile.c_str(), m_tempPropertyDescriptor->GetName().c_str(),
                        value.c_str()));
                    ++m_errors;
                }
            }
            else
            {
                EE_LOG(efd::kEntity, efd::ILogger::kERR0,
                    ("Error parsing flat model file: '%s'.  "
                     "Failed to set property '%s' to value '%s'.",
                    m_modelFile.c_str(), m_tempPropertyDescriptor->GetName().c_str(),
                    value.c_str()));
                ++m_errors;
            }
        }
        else if (index.empty())
        {
            result = m_tempDefaultProperty->SetValueByString(
                m_tempPropertyDescriptor->GetPropertyID(), value);
            if (PropertyResult_OK != result)
            {
                EE_LOG(efd::kEntity, efd::ILogger::kERR0,
                    ("Error parsing flat model file: '%s'.  "
                    "Failed to set property '%s' to value '%s'.",
                    m_modelFile.c_str(), m_tempPropertyDescriptor->GetName().c_str(),
                    value.c_str()));
                ++m_errors;
            }
        }
        else
        {
            result = m_tempDefaultProperty->SetValueByString(
                m_tempPropertyDescriptor->GetPropertyID(), index, value);
            if (PropertyResult_OK != result)
            {
                EE_LOG(efd::kEntity, efd::ILogger::kERR0,
                    ("Error parsing flat model file: '%s'.  "
                    "Failed to set property '%s' to value '%s'->'%s'.",
                    m_modelFile.c_str(), m_tempPropertyDescriptor->GetName().c_str(),
                    index.c_str(), value.c_str()));
                ++m_errors;
            }
        }
    }
    else
    {
        EE_LOG(efd::kEntity, efd::ILogger::kERR0,
            ("Error parsing flat model file: '%s'.  Missing attribute 'value' in 'set' tag.",
            m_modelFile.c_str()));
        ++m_errors;
    }
}


//------------------------------------------------------------------------------------------------
void SAXModelParser::ParseMixin(const TiXmlAttributeSet& attrs)
{
    EE_ASSERT(m_spFlatModel);

    // get the model id
    FlatModelID flatModelID = 0;
    if (!XMLUtils::GetAttribute(attrs, ms_constants->kAttrMixinID, flatModelID))
    {
        EE_LOG(efd::kEntity, efd::ILogger::kERR0,
            ("Error parsing flat model file: '%s'.  Missing attribute 'id' in 'mixin' tag.",
            m_modelFile.c_str()));
        ++m_errors;
    }

    utf8string modelName;
    if (!XMLUtils::GetAttribute(attrs, ms_constants->kAttrMixinName, modelName))
    {
        EE_LOG(efd::kEntity, efd::ILogger::kERR0,
            ("Error parsing flat model file: '%s'.  Missing attribute 'name' in 'mixin' tag.",
            m_modelFile.c_str()));
        ++m_errors;
    }

    // We might need to perform name->id mappings for models that aren't loaded.  The main
    // reason is for invoking behaviors from script.  We are given the model name and need
    // to map it to a model ID.  We don't want to load intermediate flat models unless they
    // are actually used to construct entities so instead we just remember the name->id mappings.
    if (!m_pFlatModelManager->RegisterMixin(modelName, flatModelID))
    {
        EE_LOG(efd::kEntity, efd::ILogger::kERR0,
            ("Error parsing flat model file: '%s'.  "
            "Invalid mixin information, model '%s' (%d) is conflicting.",
            m_modelFile.c_str(), modelName.c_str(), flatModelID));
        ++m_errors;
    }

    m_spFlatModel->AddMixinModel(flatModelID);

    utf8string type;
    if (XMLUtils::GetAttribute(attrs, ms_constants->kAttrMixinType, type))
    {
        BuiltinModelDescriptorPtr spComp = EE_NEW BuiltinModelDescriptor();
        spComp->SetID(flatModelID);
        spComp->SetName(modelName);

        efd::vector< efd::utf8string > tokens;
        type.split(";, \t", tokens);

        for (UInt32 i=0; i < tokens.size(); ++i)
        {
            efd::utf8string& traitName = tokens[i];
            egf::ModelTraits trait;
            if (FlatModelTraitFromName(traitName, trait))
            {
                spComp->SetTrait(trait, true);
            }
            else
            {
                // Should we really treat this as an error?  Or just log about it?
                EE_LOG(efd::kEntity, efd::ILogger::kERR0,
                    ("Error parsing flat model file: '%s'.  Invalid mixin model trait '%s'.",
                    m_modelFile.c_str(), traitName.c_str()));
            }
        }

        if (_AdaptBuiltinModelToProgramType(spComp))
        {
            m_spFlatModel->AddBuiltinModelDescriptor(spComp);
        }
    }
}

//------------------------------------------------------------------------------------------------
void SAXModelParser::ParseExtraData(const TiXmlAttributeSet& attrs)
{
    utf8string type;
    utf8string name;
    if (!XMLUtils::GetAttribute(attrs, ms_constants->kAttrExtraDataType, type))
    {
        EE_LOG(efd::kEntity, efd::ILogger::kERR0,
            ("Error parsing flat model file: '%s'.  Could not find ExtraData attribute 'type'",
            m_modelFile.c_str()));
        ++m_errors;
    }

    if (!XMLUtils::GetAttribute(attrs, ms_constants->kAttrExtraDataName, name))
    {
        EE_LOG(efd::kEntity, efd::ILogger::kERR0,
            ("Error parsing flat model file: '%s'.  Could not find ExtraData attribute 'name'",
            m_modelFile.c_str()));
        ++m_errors;
    }

    m_spExtraData = EE_NEW ExtraData(name, type);
}

//------------------------------------------------------------------------------------------------
void SAXModelParser::ParseExtraDataElement(const TiXmlAttributeSet& attrs)
{
    utf8string type;
    if (!XMLUtils::GetAttribute(attrs, ms_constants->kAttrExtraDataEntryType, type))
    {
        EE_LOG(efd::kEntity, efd::ILogger::kERR0,
            ("Error parsing flat model file: '%s'.  Could not find Entry attribute 'type'",
            m_modelFile.c_str()));
        ++m_errors;
    }

    utf8string key;
    if (!XMLUtils::GetAttribute(attrs, ms_constants->kAttrExtraDataEntryKey, key))
    {
        EE_LOG(efd::kEntity, efd::ILogger::kERR0,
            ("Error parsing flat model file: '%s'.  Could not find Entry attribute 'key'",
            m_modelFile.c_str()));
        ++m_errors;
    }

    utf8string value;
    if (!XMLUtils::GetAttribute(attrs, ms_constants->kAttrExtraDataEntryValue, value))
    {
        EE_LOG(efd::kEntity, efd::ILogger::kERR0,
            ("Error parsing flat model file: '%s'.  Could not find Entry attribute 'value'",
            m_modelFile.c_str()));
        ++m_errors;
    }

    m_spExtraData->AddEntry(type, key, value);
}

//------------------------------------------------------------------------------------------------
void SAXModelParser::FinishedModel()
{
    // In case we haven't already finalized our traits for some other reason we need to do so now.
    // Its safe to call this method multiple times so its ok if we already called this.
    FinalizeTraits();
}

//------------------------------------------------------------------------------------------------
void SAXModelParser::FinalizeTraits()
{
    EE_ASSERT(m_spFlatModel);

    if (!m_fTraitsFinalized)
    {
        m_fTraitsFinalized = true;

        if (_AdaptModelToProgramType())
        {
            bool isBuiltin = m_spFlatModel->GetTrait(ModelTrait_BuiltinModel);
            bool isViewBuiltin = m_spFlatModel->GetTrait(ModelTrait_ReplicaBuiltinModel);
            if (isBuiltin || isViewBuiltin)
            {
                // The most-derived model is itself a built-in model, so we must add a
                // BuiltinModelDescriptor for ourself to ourself.  Luckily we know that the name
                // and ID of the current flat model has already been parsed and set.
                BuiltinModelDescriptorPtr pBMD = EE_NEW BuiltinModelDescriptor();
                pBMD->SetID(m_spFlatModel->GetID());
                pBMD->SetName(m_spFlatModel->GetName());
                pBMD->SetTrait(ModelTrait_BuiltinModel, isBuiltin);
                pBMD->SetTrait(ModelTrait_ReplicaBuiltinModel, isViewBuiltin);

                m_spFlatModel->AddBuiltinModelDescriptor(pBMD);
            }
        }
        else
        {
            m_spFlatModel = NULL;
        }
    }
}


//------------------------------------------------------------------------------------------------
void SAXModelParser::FinishedProperty()
{
    EE_ASSERT(m_spFlatModel);
    EE_ASSERT(m_tempPropertyDescriptor);

    if (m_tempDefaultProperty)
    {
        PropertyResult res =
            m_tempPropertyDescriptor->SetDefaultProperty(*m_tempDefaultProperty);
        if (PropertyResult_OK == res)
        {
            if (m_tempPropertyDescriptor->IsValid())
            {
                if (_AdaptPropertyToProgramType())
                {
                    if (!m_spFlatModel->AddPropertyDescriptor(m_tempPropertyDescriptor))
                    {
                        EE_LOG(efd::kEntity, efd::ILogger::kERR0,
                            ("Error parsing flat model file: '%s'.  "
                            "Failed to add property '%s' to model'%s'.",
                            m_modelFile.c_str(),
                            m_tempPropertyDescriptor->GetName().c_str(),
                            m_spFlatModel->GetName().c_str()));
                        ++m_errors;
                    }
                }
            }
            else
            {
                EE_LOG(efd::kEntity, efd::ILogger::kERR0,
                    ("Error parsing flat model file: '%s'.  "
                    "Property '%s' in model'%s' is invalid.",
                    m_modelFile.c_str(),
                    m_tempPropertyDescriptor->GetName().c_str(),
                    m_spFlatModel->GetName().c_str()));
                ++m_errors;
            }
        }
        else
        {
            EE_LOG(efd::kEntity, efd::ILogger::kERR0,
                ("Error parsing flat model file: '%s'.  "
                "Failed to set default value for property '%s' (error '%d').  "
                "Skipping property in model '%s'",
                m_modelFile.c_str(), m_tempPropertyDescriptor->GetName().c_str(), res,
                m_spFlatModel->GetName().c_str()));
            ++m_errors;
        }
    }
    else
    {
        // This will have already been counted as an error already when we failed to create
        // m_tempDefaultProperty, so don't increment m_errors a second time
        EE_LOG(efd::kEntity, efd::ILogger::kERR0,
            ("Error parsing flat model file: '%s'.  "
            "No default value, skipping property '%s' in model '%s'.",
            m_modelFile.c_str(),
            m_tempPropertyDescriptor->GetName().c_str(),
            m_spFlatModel->GetName().c_str()));
    }

    // the default property and property descriptor get copied.  destroy these instances
    EE_DELETE m_tempDefaultProperty;
    m_tempDefaultProperty = NULL;

    m_tempPropertyDescriptor = NULL;
}

//------------------------------------------------------------------------------------------------
bool SAXModelParser::ValidateBehavior()
{
    // DT32332 Toolbench EMT in version 3.0 would generate flat models marked as virtual but with
    // an empty invocation list. The BehaviorDescriptor::IsValid method catches this condition but
    // we want to avoid treating this as an error until the EMT bug has been fixed.
    if (BehaviorType_Virtual == m_spBehaviorDesc->GetType() &&
        m_spBehaviorDesc->GetInvocationOrderedModelNames().empty())
    {
        return false;
    }

    // DT40944 Toolbench EMT in version 3.1 would generate bogus flat models that contain Abstract
    // behaviors. Abstract is meant only for tool-time use in .model files to make it easier to
    // override standard behaviors such as lifecycle behaviors. For now we don't treat these flat's
    // as errors until enough time has passed that old flat model versions should be cleaned up.
    if (BehaviorType_Abstract == m_spBehaviorDesc->GetType())
    {
        return false;
    }

    if (!m_spBehaviorDesc->IsValid())
    {
        EE_LOG(efd::kEntity, efd::ILogger::kERR0,
            ("Error parsing flat model file: '%s'.  Invalid behavior '%s::%s' (%d).",
            m_modelFile.c_str(),
            m_spBehaviorDesc->GetModelName().c_str(),
            m_spBehaviorDesc->GetName().c_str(),
            m_spBehaviorDesc->GetID()));
        ++m_errors;
        return false;
    }

    return true;
}

//------------------------------------------------------------------------------------------------
void SAXModelParser::FinishedBehavior()
{
    EE_ASSERT(m_spFlatModel);
    EE_ASSERT(m_spBehaviorDesc);

    if (_AdaptBehaviorToProgramType())
    {
        if (ValidateBehavior())
        {
            if (!m_spFlatModel->AddBehaviorDescriptor(m_spBehaviorDesc))
            {
                EE_LOG(efd::kEntity, efd::ILogger::kERR0,
                    ("Error parsing flat model file: '%s'.  "
                    "Failed to add behavior '%s' to model.",
                    m_modelFile.c_str(),
                    m_spBehaviorDesc->GetName().c_str()));
                ++m_errors;
            }
        }
        else
        {
            // skipping invalid behavior. No need to log another error since we will have already
            // logged the bogus condition when it was first detected.
            EE_LOG(efd::kEntity, efd::ILogger::kLVL2,
                ("Skipping behavior '%s' from flat model '%s' as it is invalid.",
                m_spBehaviorDesc->GetName().c_str(),
                m_modelFile.c_str()));
        }
    }

    m_spBehaviorDesc = NULL;
}


//------------------------------------------------------------------------------------------------
bool SAXModelParser::FlatModelTraitFromName(const efd::utf8string& i_strTraitName,
                                             egf::ModelTraits& o_result)
{
    return m_modelTraitMap.find(i_strTraitName, o_result);
}


//------------------------------------------------------------------------------------------------
PropertyTraits SAXModelParser::PropertyTraitFromName(const efd::utf8string& i_strTraitName)
{
    PropertyTraits result = PropertyTrait_None;
    m_propertyTraitMap.find(i_strTraitName, result);
    return result;
}


//------------------------------------------------------------------------------------------------
efd::UInt32 SAXModelParser::ReplicationGroupFromName(const utf8string& i_strTraitName)
{
    UInt32 result = 0xFFFFFFFF;
    m_propertyReplicationGroupMap.find(i_strTraitName, result);
    return result;
}


//------------------------------------------------------------------------------------------------
BehaviorTypes SAXModelParser::BehaviorTypeFromName(const utf8string& i_strTypeName)
{
    BehaviorTypes result = BehaviorType_Invalid;
    m_behaviorTypeMap.find(i_strTypeName, result);
    return result;
}


//------------------------------------------------------------------------------------------------
BehaviorTraits SAXModelParser::BehaviorTraitFromName(const utf8string& i_strTraitName)
{
    BehaviorTraits result = BehaviorTrait_None;
    m_behaviorTraitMap.find(i_strTraitName, result);

    // The immediate trait also implies the NoBlock trait, so set both values:
    if (result == BehaviorTrait_Immediate)
    {
        result = (BehaviorTraits)((UInt32)result | (UInt32)BehaviorTrait_NoBlock);
    }

    return result;
}


//------------------------------------------------------------------------------------------------
efd::ClassID SAXModelParser::PropertyTypeFromName(const utf8string& i_strPropTypeName)
{
    efd::ClassID result = m_pFlatModelManager->GetTypeIDByName(i_strPropTypeName);
    return result;
}


//------------------------------------------------------------------------------------------------
bool SAXModelParser::_AdaptModelToProgramType()
{
    ServiceManager* pServiceManager = m_pFlatModelManager->GetServiceManager();
    if (!pServiceManager)
    {
        return false;
    }

    // NOTE: I might be both a client and a server so be careful how these checks are worded.
    bool isClient = pServiceManager->IsProgramType(ServiceManager::kProgType_Client);
    bool isServer = pServiceManager->IsProgramType(ServiceManager::kProgType_Server);
    EE_ASSERT(isClient || isServer); // <- At least one of these should be set

    if (m_spFlatModel->GetTrait(ModelTrait_BuiltinModel))
    {
        if (m_spFlatModel->GetTrait(ModelTrait_ClientOnlyBuiltinModel) && !isClient)
        {
            m_spFlatModel->SetTrait(ModelTrait_BuiltinModel, false);
        }
        if (m_spFlatModel->GetTrait(ModelTrait_ServerOnlyBuiltinModel) && !isServer)
        {
            m_spFlatModel->SetTrait(ModelTrait_BuiltinModel, false);
        }
    }

    if (m_spFlatModel->GetTrait(ModelTrait_ReplicaBuiltinModel))
    {
        if (m_spFlatModel->GetTrait(ModelTrait_ClientOnlyReplicaBuiltinModel) && !isClient)
        {
            m_spFlatModel->SetTrait(ModelTrait_ReplicaBuiltinModel, false);
        }
        if (m_spFlatModel->GetTrait(ModelTrait_ServerOnlyReplicaBuiltinModel) && !isServer)
        {
            m_spFlatModel->SetTrait(ModelTrait_ReplicaBuiltinModel, false);
        }
    }

    // Tools never create built-ins without expressly having the ToolBuiltinModel trait set:
    bool isTool = pServiceManager->IsProgramType(ServiceManager::kProgType_Tool);
    if (isTool && !m_spFlatModel->GetTrait(ModelTrait_ToolBuiltinModel))
    {
        m_spFlatModel->SetTrait(ModelTrait_BuiltinModel, false);
        m_spFlatModel->SetTrait(ModelTrait_ReplicaBuiltinModel, false);
    }

    //DT27991 Make EMT able to output separate client/server data so
    // the sax parser doesn't have to strip out traits.
    // Remove all the adapter traits just to ensure people don't try to use these at run-time
    // in ways they are not meant to be used.  These are meant to be temporary until
    // such time as the tool can generate separate client/server data.
    m_spFlatModel->SetTrait(ModelTrait_ClientOnlyBuiltinModel, false);
    m_spFlatModel->SetTrait(ModelTrait_ServerOnlyBuiltinModel, false);
    m_spFlatModel->SetTrait(ModelTrait_ClientOnlyReplicaBuiltinModel, false);
    m_spFlatModel->SetTrait(ModelTrait_ServerOnlyReplicaBuiltinModel, false);
    m_spFlatModel->SetTrait(ModelTrait_ToolBuiltinModel, false);

    return true;
}


//------------------------------------------------------------------------------------------------
bool SAXModelParser::_AdaptBuiltinModelToProgramType(BuiltinModelDescriptor* pBMD)
{
    ServiceManager* pServiceManager = m_pFlatModelManager->GetServiceManager();
    if (!pServiceManager)
    {
        return false;
    }

    // NOTE: I might be both a client and a server and a tool or any combination thereof so be
    // careful how these checks are worded.
    bool isClient = pServiceManager->IsProgramType(ServiceManager::kProgType_Client);
    bool isServer = pServiceManager->IsProgramType(ServiceManager::kProgType_Server);
    EE_ASSERT(isClient || isServer); // <- At least one of these should be set

    if (pBMD->GetTrait(ModelTrait_BuiltinModel))
    {
        if (pBMD->GetTrait(ModelTrait_ClientOnlyBuiltinModel) && !isClient)
        {
            pBMD->SetTrait(ModelTrait_BuiltinModel, false);
        }
        if (pBMD->GetTrait(ModelTrait_ServerOnlyBuiltinModel) && !isServer)
        {
            pBMD->SetTrait(ModelTrait_BuiltinModel, false);
        }
    }

    if (pBMD->GetTrait(ModelTrait_ReplicaBuiltinModel))
    {
        if (pBMD->GetTrait(ModelTrait_ClientOnlyReplicaBuiltinModel) && !isClient)
        {
            pBMD->SetTrait(ModelTrait_ReplicaBuiltinModel, false);
        }
        if (pBMD->GetTrait(ModelTrait_ServerOnlyReplicaBuiltinModel) && !isServer)
        {
            pBMD->SetTrait(ModelTrait_ReplicaBuiltinModel, false);
        }
    }

    // Tools never create built-ins without expressly having the ToolBuiltinModel trait set:
    bool isTool = pServiceManager->IsProgramType(ServiceManager::kProgType_Tool);
    if (isTool && !pBMD->GetTrait(ModelTrait_ToolBuiltinModel))
    {
        pBMD->SetTrait(ModelTrait_BuiltinModel, false);
        pBMD->SetTrait(ModelTrait_ReplicaBuiltinModel, false);
    }

    //DT27991 Make EMT able to output separate client/server data so
    // the sax parser doesn't have to strip out traits
    // Remove all the adapter traits just to ensure people don't try to use these at run-time
    // in ways they are not meant to be used.  These are meant to be temporary until
    // such time as the tool can generate separate client/server data.
    pBMD->SetTrait(ModelTrait_ClientOnlyBuiltinModel, false);
    pBMD->SetTrait(ModelTrait_ServerOnlyBuiltinModel, false);
    pBMD->SetTrait(ModelTrait_ClientOnlyReplicaBuiltinModel, false);
    pBMD->SetTrait(ModelTrait_ServerOnlyReplicaBuiltinModel, false);
    pBMD->SetTrait(ModelTrait_ToolBuiltinModel, false);

    // If after being adapted we are still a built-in then return true:
    return pBMD->GetTrait(ModelTrait_BuiltinModel) ||
        pBMD->GetTrait(ModelTrait_ReplicaBuiltinModel);
}


//------------------------------------------------------------------------------------------------
bool SAXModelParser::_AdaptPropertyToProgramType()
{
    ServiceManager* pServiceManager = m_pFlatModelManager->GetServiceManager();
    if (!pServiceManager)
    {
        return false;
    }

    // NOTE: I might be both a client and a server so be careful how these checks are worded.
    bool isClient = pServiceManager->IsProgramType(ServiceManager::kProgType_Client);
    bool isServer = pServiceManager->IsProgramType(ServiceManager::kProgType_Server);
    EE_ASSERT(isClient || isServer); // <- At least one of these should be set

    if (m_tempPropertyDescriptor->GetTrait(PropertyTrait_ServerOnly) && !isServer)
    {
        return false;
    }

    if (m_tempPropertyDescriptor->GetTrait(PropertyTrait_ClientOnly) && !isClient)
    {
        return false;
    }

    // We need to know that the traits for the current model are finalized before we can adapt
    // built-in properties to run-time conditions:
    FinalizeTraits();

    // This property might have originally been from a built-in, but that built-in might have
    // been demoted to non-built-in status when it was adapted in which case the property also
    // needs to be demoted.
    const utf8string& source = m_tempPropertyDescriptor->GetSource();
    if (!source.empty())
    {
        // NOTE: Toolbench automatically sets FromBuiltinModel on properties, but it does not yet
        // set FromReplicaBuiltinModel.  Also it doesn't transfer the various client-only and
        // server-only adaptor flags.  This means we might have to remove traits that were
        // set by Toolbench and/or add traits that Toolbench simply didn't add.  As such these
        // checks are coded to always set both built-in related flags.

        // NOTE: pBMD can come back NULL, that simply means the given source is no longer a
        // built-in under any circumstances after run-time adaptation.
        const BuiltinModelDescriptor* pBMD = m_spFlatModel->GetBuiltinModelDescriptor(source);

        bool isBuiltinProp = pBMD && pBMD->GetTrait(ModelTrait_BuiltinModel);
        m_tempPropertyDescriptor->SetTrait(PropertyTrait_FromBuiltinModel, isBuiltinProp);

        bool isViewBuiltinProp = pBMD && pBMD->GetTrait(ModelTrait_ReplicaBuiltinModel);
        m_tempPropertyDescriptor->SetTrait(
            PropertyTrait_FromReplicaBuiltinModel,
            isViewBuiltinProp);
    }


    //DT27991 Make EMT able to output separate client/server data so
    // the sax parser doesn't have to strip out traits
    // Remove all the adapter traits just to ensure people don't try to use these at run-time
    // in ways they are not meant to be used.  These are meant to be temporary until
    // such time as the tool can generate separate client/server data.
    m_tempPropertyDescriptor->SetTrait(PropertyTrait_ServerOnly, false);
    m_tempPropertyDescriptor->SetTrait(PropertyTrait_ClientOnly, false);

    return true;
}


//------------------------------------------------------------------------------------------------
bool SAXModelParser::_AdaptBehaviorToProgramType()
{
    ServiceManager* pServiceManager = m_pFlatModelManager->GetServiceManager();
    if (!pServiceManager)
    {
        return false;
    }

    // NOTE: I might be both a client and a server so be careful how these checks are worded.
    bool isClient = pServiceManager->IsProgramType(ServiceManager::kProgType_Client);
    bool isServer = pServiceManager->IsProgramType(ServiceManager::kProgType_Server);
    EE_ASSERT(isClient || isServer); // <- At least one of these should be set

    if (m_spBehaviorDesc->GetTrait(BehaviorTrait_ClientExecOnly) && !isClient)
    {
        if (m_spBehaviorDesc->GetTrait(BehaviorTrait_RemotelyVisible))
        {
            m_spBehaviorDesc->SetType(BehaviorType_Remote);
            m_spBehaviorDesc->SetModelName(efd::utf8string::NullString());
            m_spBehaviorDesc->ClearInvocationOrderList();
        }
        else
        {
            return false;
        }
    }

    if (m_spBehaviorDesc->GetTrait(BehaviorTrait_ServerExecOnly) && !isServer)
    {
        if (m_spBehaviorDesc->GetTrait(BehaviorTrait_RemotelyVisible))
        {
            m_spBehaviorDesc->SetType(BehaviorType_Remote);
            m_spBehaviorDesc->SetModelName(efd::utf8string::NullString());
            m_spBehaviorDesc->ClearInvocationOrderList();
        }
        else
        {
            return false;
        }
    }

    // DT27991 Make EMT able to output separate client/server data so the sax parser doesn't have
    // to strip out traits.
    // Remove all the adapter traits just to ensure people don't try to use these at run-time
    // in ways they are not meant to be used. These are meant to be temporary until
    // such time as the tool can generate separate client/server data.
    m_spBehaviorDesc->SetTrait(BehaviorTrait_ClientExecOnly, false);
    m_spBehaviorDesc->SetTrait(BehaviorTrait_ServerExecOnly, false);
    m_spBehaviorDesc->SetTrait(BehaviorTrait_RemotelyVisible, false);

    return true;
}

