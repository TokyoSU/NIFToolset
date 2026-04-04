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

#include <egf/SAXEntityParser.h>
#include <egf/EntityManager.h>
#include <efd/ILogger.h>
#include <efd/EEHelpers.h>
#include <efd/ID128.h>
#include <efd/File.h>
#include <egf/egfLogIDs.h>
#include <egf/PlaceableModel.h>

using namespace efd;
using namespace egf;

//------------------------------------------------------------------------------------------------
struct egf::XmlEntityStringConstants : public efd::MemObject
{
    // Tags and attribute values used in parsing
    EE_TIXML_STRING kEntityTagGame;
    EE_TIXML_STRING kEntityTagReferences;
    EE_TIXML_STRING kEntityTagReference;

    EE_TIXML_STRING kEntityTagEntitySet;

    EE_TIXML_STRING kEntityTagEntity;
    EE_TIXML_STRING kEntityAttrModelName;
    EE_TIXML_STRING kEntityAttrEntityId;
    EE_TIXML_STRING kEntityAttrIteration;

    EE_TIXML_STRING kEntityTagProperty;
    EE_TIXML_STRING kEntityAttrPropertyName;

    EE_TIXML_STRING kEntityTagSet;
    EE_TIXML_STRING kEntityAttrSetIndex;
    EE_TIXML_STRING kEntityAttrSetValue;

    EE_TIXML_STRING kEntityTagToolSettings;

    XmlEntityStringConstants()
        : kEntityTagGame("game")
        , kEntityTagReferences("model_references")
        , kEntityTagReference("reference")
        , kEntityTagEntitySet("entitySet")
        , kEntityTagEntity("entity")
        , kEntityAttrModelName("modelName")
        , kEntityAttrEntityId("id")
        , kEntityAttrIteration("iterations")
        , kEntityTagProperty("property")
        , kEntityAttrPropertyName("name")
        , kEntityTagSet("set")
        , kEntityAttrSetIndex("index")
        , kEntityAttrSetValue("value")
        , kEntityTagToolSettings("toolSettings")
    {
    }
};

//------------------------------------------------------------------------------------------------
/*static*/ XmlEntityStringConstants* SAXEntityParser::ms_constants;

//------------------------------------------------------------------------------------------------
/*static*/ void SAXEntityParser::_SDMInit()
{
    ms_constants = EE_NEW XmlEntityStringConstants();
}

//------------------------------------------------------------------------------------------------
/*static*/ void SAXEntityParser::_SDMShutdown()
{
    EE_DELETE ms_constants;
}

//------------------------------------------------------------------------------------------------
SAXEntityParser::SAXEntityParser(
    const char* pXmlBuffer,
    const efd::utf8string& documentName,
    efd::UInt32 blockInstance,
    egf::FlatModelManager* pfmm,
    egf::EntityManager* pEntityManager,
    efd::set<egf::EntityID>* pPreviouslyLoaded)
    : m_blockFile(documentName)
    , m_blockInstance(blockInstance)
    , m_pXmlBuffer(pXmlBuffer)
    , m_parserState(NotParsing)
    , m_pfmm(pfmm)
    , m_propID(0)
    , m_propIsEntityRef(false)
    , m_entitiesParsed(0)
    , m_maxEntityLoadsPerParse(10)
    , m_bPartialLoading(true)
    , m_pEntityManager(pEntityManager)
    , m_pPreviouslyLoadedEntities(pPreviouslyLoaded)
    , m_errors(0)
    , m_createMasterEntities(true)
    , m_useRotation(false)
    , m_useOffset(false)
{
    m_pDocument = EE_EXTERNAL_NEW TiXmlDocument(m_blockFile.c_str());
}

//------------------------------------------------------------------------------------------------
SAXEntityParser::~SAXEntityParser()
{
    EE_EXTERNAL_DELETE m_pDocument;
}

//------------------------------------------------------------------------------------------------
void SAXEntityParser::ApplyRotation(const efd::Point3& rotation)
{
    m_rotation = rotation;
    m_rotInRads = rotation * -efd::EE_DEGREES_TO_RADIANS;
    m_useRotation = true;
}

//------------------------------------------------------------------------------------------------
void SAXEntityParser::ApplyOffset(const efd::Point3& offset)
{
    m_offset = offset;
    m_useOffset = true;
}

//------------------------------------------------------------------------------------------------
efd::UInt32 SAXEntityParser::GetMaxEntityLoadsPerParsing() const
{
    return m_maxEntityLoadsPerParse;
}

//------------------------------------------------------------------------------------------------
void SAXEntityParser::SetMaxEntityLoadsPerParsing(efd::UInt32 entities)
{
    m_maxEntityLoadsPerParse = entities;
}

//------------------------------------------------------------------------------------------------
efd::UInt32 SAXEntityParser::GetEntitiesParsed() const
{
    return m_entitiesParsed;
}

//------------------------------------------------------------------------------------------------
SAXEntityParser::Result SAXEntityParser::BeginParse(efd::UInt32& o_errors)
{
    m_pDocument->SetSAXHandler(this);

    TiXmlBase::SAXResult result = 
        m_pDocument->SAXBeginParse(m_pXmlBuffer, TIXML_ENCODING_UTF8);

    o_errors = m_pDocument->Error() + m_errors;

    switch(result)
    {
    case TiXmlBase::TIXML_SAX_PARSING:
    case TiXmlBase::TIXML_SAX_INTERRUPTED:
        return SAXEntityParser::sep_Loading;
    case TiXmlBase::TIXML_SAX_DONE:
        return SAXEntityParser::sep_Loaded;
    case TiXmlBase::TIXML_SAX_ERROR:
    case TiXmlBase::TIXML_SAX_UNKNOWN:
        break;
    }

    return SAXEntityParser::sep_Failed;
}

//------------------------------------------------------------------------------------------------
SAXEntityParser::Result SAXEntityParser::Parse(
    efd::list<EntityPtr>*& o_entityList,
    efd::UInt32& o_errors)
{
    // Reset the count of how many entities were parsed on this call to Parse.
    m_entitiesParsed = 0;

    o_entityList = &m_entityList;

    TiXmlBase::SAXResult result = 
        m_pDocument->SAXParse(TIXML_ENCODING_UTF8);

    o_errors = m_pDocument->Error() + m_errors;

    switch(result)
    {
    case TiXmlBase::TIXML_SAX_PARSING:
    case TiXmlBase::TIXML_SAX_INTERRUPTED:
        return SAXEntityParser::sep_Loading;
    case TiXmlBase::TIXML_SAX_DONE:
        ApplyEntityLinks();
        return SAXEntityParser::sep_Loaded;
    case TiXmlBase::TIXML_SAX_ERROR:
    case TiXmlBase::TIXML_SAX_UNKNOWN:
        break;
    }

    return SAXEntityParser::sep_Failed;
}

//------------------------------------------------------------------------------------------------
efd::list<EntityPtr>* SAXEntityParser::GetParserResults()
{
    return &m_entityList;
}

//------------------------------------------------------------------------------------------------
efd::UInt32 SAXEntityParser::DeprecatedParse(
    const efd::utf8string& i_blockFile,
    FlatModelManager* i_pfmm,
    efd::list<EntityPtr>& o_entityList)

{
    return _Parse(i_blockFile, i_pfmm, true, o_entityList);
}

//------------------------------------------------------------------------------------------------
efd::UInt32 SAXEntityParser::ParseForReplicationTesting(
    const efd::utf8string& i_blockFile,
    FlatModelManager* i_pfmm,
    efd::list<EntityPtr>& o_entityList)

{
    return _Parse(i_blockFile, i_pfmm, false, o_entityList);
}

//------------------------------------------------------------------------------------------------
efd::UInt32 SAXEntityParser::_Parse(
    const efd::utf8string& i_blockFile,
    FlatModelManager* i_pfmm,
    bool i_createMasterEntities,
    efd::list<EntityPtr>& o_entityList,
    ServiceManager* pServiceManager)
{
    // create a SAX parser
    TiXmlDocument kDocument(i_blockFile.c_str());

    File* pkFile = efd::File::GetFile(kDocument.Value(), File::READ_ONLY);
    if (pkFile)
    {
        unsigned int uiSize = pkFile->GetFileSize();
        char* pBuffer = EE_ALLOC(char, uiSize + 1);
        pkFile->Read(pBuffer, uiSize);
        pBuffer[uiSize] = '\0';

        egf::EntityManager* pEM = NULL;
        if (pServiceManager)
        {
            pEM = pServiceManager->GetSystemServiceAs<egf::EntityManager>();
        }

        SAXEntityParser parser(
            pBuffer,
            i_blockFile,
            0,
            i_pfmm,
            pEM,
            NULL);

        parser.m_createMasterEntities = i_createMasterEntities;

        // We disable partial entity loading for this test case because we're doing it as a 
        // one shot thing.
        parser.m_bPartialLoading = false;

        kDocument.SAXParseBuffer(pBuffer, &parser);

        // Copy the list of entities we parsed into the list that was passed in.
        o_entityList = parser.m_entityList;

        EE_FREE(pBuffer);
        EE_DELETE(pkFile);

        // Once we have parsed all entities, then we link them:
        parser.ApplyEntityLinks();

        return (efd::UInt32)kDocument.Error() + parser.m_errors;
    }

    return 1;
}

//------------------------------------------------------------------------------------------------
void SAXEntityParser::startElement(
    const EE_TIXML_STRING& localname,
    const TiXmlAttributeSet& attrs)
{
    if (localname == ms_constants->kEntityTagGame ||
        localname == ms_constants->kEntityTagReference ||
        localname == ms_constants->kEntityTagReferences)
    {
        // Ignore tags used by the tool that don't effect runtime
        return;
    }

    // switch on the current state of the parser
    switch (m_parserState)
    {
    case NotParsing:
        // look for the modelGroup or toolSettings tag.  if something else
        // comes first, then that's an error
        if (localname == ms_constants->kEntityTagEntitySet)
        {
            m_parserState = InDocument;
        }
        else if (localname == ms_constants->kEntityTagToolSettings)
        {
            m_parserState = InToolSettings;
        }
        else
        {
            EE_LOG(efd::kEntity, efd::ILogger::kERR0,
                ("Error parsing entity set file: '%s'.  Expected a 'entitySet' tag.",
                m_blockFile.c_str()));

            ++m_errors;
        }
        break;

    case InDocument:
        // look for the entity tag.  if something else comes first, then that's an error
        if (localname == ms_constants->kEntityTagEntity)
        {
            if (ParseEntity(attrs))
            {
                m_parserState = InEntity;
            }
            else
            {
                m_parserState = SkipEntity;
            }
        }
        else
        {
            EE_LOG(efd::kEntity, efd::ILogger::kERR0,
                ("Error parsing entity set file: '%s'.  Expected a 'entity' tag.",
                m_blockFile.c_str()));

            ++m_errors;
        }
        break;

    case SkipEntity:
        {
            // We expect to see property and set value tags here
            if ((localname == ms_constants->kEntityTagProperty) ||
                (localname == ms_constants->kEntityTagSet))
            {
                // If we are skipping this entity, we don't care about the property values.
                return;
            }
            else
            {
                EE_LOG(efd::kEntity, efd::ILogger::kERR0,
                    ("Error parsing entity set file: '%s'.  Expected a 'property' or 'set' tag.",
                    m_blockFile.c_str()));

                ++m_errors;
            }
        }
        break;

    case InEntity:
        // look for a property tag.  if something else comes first, then that's an error
        if (localname == ms_constants->kEntityTagProperty)
        {
            m_parserState = InProperty;
            ParseProperty(attrs);
        }
        else
        {
            EE_LOG(efd::kEntity, efd::ILogger::kERR0,
                ("Error parsing entity set file: '%s'.  Expected a 'property' tag.",
                m_blockFile.c_str()));

            ++m_errors;
        }
        break;

    case InProperty:
        // look for a set tag.  if something else comes first,
        // then that's an error
        if (localname == ms_constants->kEntityTagSet)
        {
            m_parserState = InPropertyValue;
            ParsePropertyValue(attrs);
        }
        else
        {
            EE_LOG(efd::kEntity, efd::ILogger::kERR0,
                ("Error parsing entity set file: '%s'.  Expected a 'set' tag.",
                m_blockFile.c_str()));

            ++m_errors;
        }
        break;

    case InToolSettings:
        // Ignore all tags.
        break;

    default:
        // This should be unreachable
        EE_LOG(efd::kEntity, efd::ILogger::kERR0,
            ("Error parsing entity set file: '%s'.  Invalid state %d.",
            m_blockFile.c_str(), m_parserState));
        EE_ASSERT(false);
        break;
    }
}


//------------------------------------------------------------------------------------------------
void SAXEntityParser::endElement(const EE_TIXML_STRING& localname)
{
    if (localname == ms_constants->kEntityTagGame ||
         localname == ms_constants->kEntityTagReference ||
         localname == ms_constants->kEntityTagReferences)
    {
        // Ignore tags used by the tool that don't effect runtime
        return;
    }

    switch (m_parserState)
    {
    case NotParsing:
        // Do nothing
        break;

    case InDocument:
        // should only get here at the end of the entitySet.
        if (localname == ms_constants->kEntityTagEntitySet)
        {
            m_parserState = NotParsing;
        }
        else
        {
            EE_LOG(efd::kEntity, efd::ILogger::kERR0,
                ("Reached a bad state while parsing entity set file: '%s'. "
                "Expected an ending 'entitySet' tag.",
                m_blockFile.c_str()));

            ++m_errors;
        }
        break;

    case SkipEntity:
        if (localname == ms_constants->kEntityTagEntity)
        {
            FinishedEntity(false);
            m_parserState = InDocument;
        }
        else if ((localname == ms_constants->kEntityTagProperty) ||
            (localname == ms_constants->kEntityTagSet))
        {
            // Ignore property and set value tags.
            return;
        }
        else
        {
            EE_LOG(efd::kEntity, efd::ILogger::kERR0,
               ("Error parsing entity set file: '%s'. Expected an ending 'property' or 'set' tag.",
                m_blockFile.c_str()));

            ++m_errors;
        }
        break;

    case InEntity:
        // should only get here at the end of the model
        if (localname == ms_constants->kEntityTagEntity)
        {
            FinishedEntity(true);
            m_parserState = InDocument;
        }
        else
        {
            EE_LOG(efd::kEntity, efd::ILogger::kERR0,
                ("Reached a bad state while parsing entity set file: '%s'. "
                "Expected an ending 'entity' tag.",
                m_blockFile.c_str()));

            ++m_errors;
        }
        break;

    case InProperty:
        // should only get here at the end of a property
        if (localname == ms_constants->kEntityTagProperty)
        {
            FinishedProperty();
            m_parserState = InEntity;
        }
        else
        {
            EE_LOG(efd::kEntity, efd::ILogger::kERR0,
                ("Reached a bad state while parsing entity set file: '%s'.  "
                "Expected an ending 'property' tag.",
                m_blockFile.c_str()));

            ++m_errors;
        }
        break;

    case InPropertyValue:
        // should only get here at the end of a set value tag
        if (localname == ms_constants->kEntityTagSet)
        {
            m_parserState = InProperty;
        }
        else
        {
            EE_LOG(efd::kEntity, efd::ILogger::kERR0,
                ("Reached a bad state while parsing entity set file: '%s'.  "
                "Expected an ending 'set' tag.",
                m_blockFile.c_str()));

            ++m_errors;
        }
        break;

    case InToolSettings:
        // Watch for end of tools section
        if (localname == ms_constants->kEntityTagToolSettings)
        {
            m_parserState = NotParsing;
        }
        break;

    default:
        // This should be unreachable
        EE_LOG(efd::kEntity, efd::ILogger::kERR0,
            ("Error parsing entity set file: '%s'.  Invalid state %d.",
            m_blockFile.c_str(), m_parserState));
        EE_ASSERT(false);
        break;
    }
}

//------------------------------------------------------------------------------------------------
void SAXEntityParser::characters(const EE_TIXML_STRING& chars)
{
    // called for any characters that aren't tags.  This even, apparently, includes whitespace at
    // the document level.  That means if you need to do any meaningful parsing here you MUST
    // pay close attention to the current parser state.

    EE_UNUSED_ARG(chars);
}

//------------------------------------------------------------------------------------------------
bool SAXEntityParser::ParseEntity(const TiXmlAttributeSet& attrs)
{
    efd::ID128 dataFileID;
    efd::UInt32 iteration = 0;
    utf8string modelName;
    Entity* pExisting = NULL;

    // First, get the datafileId of the entity
    if (XMLUtils::GetAttribute(attrs, ms_constants->kEntityAttrEntityId, dataFileID))
    {
        // If this is reload, we are going to reuse the EntityPtr if possible.
        if (m_pPreviouslyLoadedEntities)
        {
            EE_ASSERT(m_pEntityManager);

            // Find all entities that use the same dataFileID. This could be more than one if the
            // block is being instanced.
            efd::set<Entity*> possibleMatches;
            m_pEntityManager->LookupEntityByDataFileID(dataFileID, possibleMatches);
            for (efd::set<Entity*>::iterator it = possibleMatches.begin();
                it != possibleMatches.end();
                ++it)
            {
                // For each entity with the same dataFileID, see if that entity was previously in
                // the block that we are reloading:
                if (m_pPreviouslyLoadedEntities->count((*it)->GetEntityID()))
                {
                    pExisting = *it;
                    break;
                }
            }
        }
    }
    else
    {
        // Error, missing attribute id
        EE_LOG(efd::kEntity, efd::ILogger::kERR0,
            ("Error parsing entity set file: '%s'.  "
            "Missing attribute 'id' in 'entity' tag.",
            m_blockFile.c_str()));
        ++m_errors;
    }

    // Then, get the iteration count on the entity and in the case of a reload, figure out
    // if we need to update it or not.
    if (XMLUtils::GetAttribute(attrs, ms_constants->kEntityAttrIteration, iteration))
    {
        // If we have the entity is found, reset all properties to their defaults, then apply the
        // overrides from the block file
        if (pExisting)
        {
            m_spEntity = pExisting;

            if (m_spEntity->GetIterationCount() != iteration)
            {
                // Set the iteration count to the new value.
                m_spEntity->SetIterationCount(iteration);

                // Reset all overridden properties on the entity.
                m_spEntity->ResetAllProperties();
                return true;
            }
            else
            {
                // Entity did not change, skip parsing of that entity.
                return false;
            }
        }
    }
    else
    {
        // Error, missing attribute iterations
        EE_LOG(efd::kEntity, efd::ILogger::kERR0,
            ("Error parsing entity set file: '%s'.  "
            "Missing attribute 'iterations' in 'entity' tag.",
            m_blockFile.c_str()));
        ++m_errors;
    }

    // If we get to here, it means that the parse is not a reload or that the entity has been
    // modified.
    if (XMLUtils::GetAttribute(attrs, ms_constants->kEntityAttrModelName, modelName))
    {
        m_spEntity = m_pfmm->FactoryEntity(modelName, 0ull, m_createMasterEntities);
        if (!m_spEntity)
        {
            EE_LOG(efd::kEntity, efd::ILogger::kERR0,
                ("Error parsing entity set file: '%s'.  "
                "Failed to create Entity with model '%s'.",
                m_blockFile.c_str(), modelName.c_str()));

            ++m_errors;
        }
        else
        {
            m_spEntity->SetDataFileID(dataFileID);
            m_spEntity->SetBlockInstance(m_blockInstance);
            m_spEntity->SetIterationCount(iteration);
            return true;
        }
    }
    else
    {
        // Error, missing attribute modelName
        EE_LOG(efd::kEntity, efd::ILogger::kERR0,
            ("Error parsing entity set file: '%s'.  "
            "Missing attribute 'modelName' in 'entity' tag.",
            m_blockFile.c_str()));
        ++m_errors;
    }
    return false;
}

//------------------------------------------------------------------------------------------------
void SAXEntityParser::ParseProperty(const TiXmlAttributeSet& attrs)
{
    // get the property name
    if (XMLUtils::GetAttribute(attrs, ms_constants->kEntityAttrPropertyName, m_propertyName))
    {
        // make sure entity is valid
        if (!m_spEntity)
        {
            // error, non-existent model
            EE_LOG(efd::kEntity, efd::ILogger::kERR0,
                ("Error parsing entity set file: '%s'.  Entity does not exist!",
                m_blockFile.c_str(), m_propertyName.c_str()));
            ++m_errors;
            return;
        }

        // make sure model is valid
        if (!m_spEntity->GetModel())
        {
            // error, non-existent model
            EE_LOG(efd::kEntity, efd::ILogger::kERR0,
                ("Error parsing entity set file: '%s'.  Model does not exist!",
                m_blockFile.c_str(), m_propertyName.c_str()));
            ++m_errors;
            return;
        }

        // make sure its a valid property name:
        const PropertyDescriptor* pd =
            m_spEntity->GetModel()->GetPropertyDescriptor(m_propertyName);
        if (pd)
        {
            m_propID = pd->GetPropertyID();
            EE_ASSERT(m_propID);

            m_propIsEntityRef = (pd->GetDataClassID() == CLASS_ID_ENTITYID);
        }
        else
        {
            // error, invalid property name
            EE_LOG(efd::kEntity, efd::ILogger::kERR0,
                ("Error parsing entity set file: '%s'.  Invalid property name '%s'.",
                m_blockFile.c_str(), m_propertyName.c_str()));

            ++m_errors;
        }
    }
    else
    {
        // Error, missing attribute name
        EE_LOG(efd::kEntity, efd::ILogger::kERR0,
            ("Error parsing entity set file: '%s'.  "
            "Missing attribute 'name' in 'property' tag.",
            m_blockFile.c_str()));
        ++m_errors;
    }
}

//------------------------------------------------------------------------------------------------
void SAXEntityParser::ParsePropertyValue(const TiXmlAttributeSet& attrs)
{
    // index is required for Assoc properties.
    utf8string index;
    XMLUtils::GetAttribute(attrs, ms_constants->kEntityAttrSetIndex, index);

    utf8string value;
    if (XMLUtils::GetAttribute(attrs, ms_constants->kEntityAttrSetValue, value))
    {
        if (!m_spEntity)
        {
            EE_LOG(efd::kEntity, efd::ILogger::kERR0,
                ("Error parsing entity set file: '%s'.  "
                "Failed to set property '%s' to value '%s' because Entity is NULL!",
                m_blockFile.c_str(), m_propertyName.c_str(), value.c_str()));
            ++m_errors;
            return;
        }

        PropertyResult result;
        if (m_propIsEntityRef)
        {
            // Store linking information for later, we can't perform the links until all entities
            // have been parsed.
            ID128 dest;
            if (ParseHelper<ID128>::FromString(value, dest))
            {
                m_ldl.push_back(LinkData(m_spEntity, m_propID, index, dest));
            }
            else
            {
                EE_LOG(efd::kEntity, efd::ILogger::kERR0,
                    ("Error parsing entity set file: '%s'.  "
                    "Failed to parse EntityRef for property '%s' to value '%s'.",
                    m_blockFile.c_str(), m_propertyName.c_str(), value.c_str()));
                ++m_errors;
            }
        }
        else if (index.empty())
        {
            result = m_spEntity->SetPropertyValueByString(m_propID, value);
            if (PropertyResult_OK != result)
            {
                EE_LOG(efd::kEntity, efd::ILogger::kERR0,
                    ("Error parsing entity set file: '%s'.  "
                    "Failed to set property '%s' to value '%s'.",
                    m_blockFile.c_str(), m_propertyName.c_str(), value.c_str()));
                ++m_errors;
            }
        }
        else
        {
            result = m_spEntity->SetPropertyValueByString(m_propID, index, value);
            if (PropertyResult_OK != result)
            {
                EE_LOG(efd::kEntity, efd::ILogger::kERR0,
                    ("Error parsing entity set file: '%s'.  "
                    "Failed to set property '%s' to value '%s'->'%s'.",
                    m_blockFile.c_str(), m_propertyName.c_str(), index.c_str(), value.c_str()));
                ++m_errors;
            }
        }
    }
    else
    {
        EE_LOG(efd::kEntity, efd::ILogger::kERR0,
            ("Error parsing entity set file: '%s'.  "
            "Missing attribute 'value' in 'set' tag.",
            m_blockFile.c_str()));
        ++m_errors;
    }
}

//------------------------------------------------------------------------------------------------
void SAXEntityParser::FinishedEntity(bool isNewEntity)
{
    if (m_spEntity)
    {
        if (isNewEntity && (m_useOffset || m_useRotation))
        {
            PlaceableModel* pPlaceable =
                EE_DYNAMIC_CAST(PlaceableModel,
                m_spEntity->FindBuiltinModel(kFlatModelID_StandardModelLibrary_Placeable));
            if (pPlaceable)
            {
                Point3 position = pPlaceable->GetPosition();

                const Point3& offset = m_useOffset ? m_offset : Point3::ZERO;
                if (m_useRotation)
                {
                    // Convert degrees to radians
                    efd::Matrix3 transform;
                    transform.FromEulerAnglesXYZ(m_rotInRads.x, m_rotInRads.y, m_rotInRads.z);
                    Point3 newPosition;
                    Matrix3::TransformVertices(transform, offset, 1, &position, &newPosition);
                    pPlaceable->SetPosition(newPosition);

                    pPlaceable->SetRotation(pPlaceable->GetRotation() + m_rotation);
                }
                else
                {
                    pPlaceable->SetPosition(position + offset);
                }
            }
        }
        m_entityList.push_back(m_spEntity);

        if (m_spEntity->GetDataFileID().IsValid())
        {
            m_entityMap[m_spEntity->GetDataFileID()] = m_spEntity;
        }

        m_spEntity = NULL;

        if (m_bPartialLoading)
        {
            ++m_entitiesParsed;
            if (m_entitiesParsed >= m_maxEntityLoadsPerParse)
            {
                m_pDocument->SAXInterruptParse();
            }
        }
    }
}

//------------------------------------------------------------------------------------------------
void SAXEntityParser::FinishedProperty()
{
    // Protect against previous values effecting the future state:
    m_propertyName.clear();
    m_propID = 0;
}

//------------------------------------------------------------------------------------------------
void SAXEntityParser::ApplyEntityLinks()
{
    for (LinkDataList::iterator iter = m_ldl.begin(); iter != m_ldl.end(); ++iter)
    {
        LinkData& ld = *iter;
        if (ld.m_pSource == NULL)
        {
            continue;
        }

        if (!ld.m_idDest.IsValid())
        {
            m_errors += ld.SetLink(m_blockFile, kENTITY_INVALID);
            continue;
        }

        DataIdToEntityMap::iterator targetIter = m_entityMap.find(ld.m_idDest);
        if (targetIter != m_entityMap.end())
        {
            Entity* pDest = targetIter->second;
            EE_ASSERT(pDest);
            m_errors += ld.SetLink(m_blockFile, pDest->GetEntityID());
            continue;
        }

        // This could be a cross-block link, so try to find the entity in the entity manager. There
        // are several restrictions, we only want to find entities that are either in the same
        // instance number we are in or else are in the global instance.
        if (m_pEntityManager)
        {
            bool found = false;
            egf::EntityID linkValue;

            efd::set<Entity*> possibleMatches;
            m_pEntityManager->LookupEntityByDataFileID(ld.m_idDest, possibleMatches);
            for (efd::set<Entity*>::iterator it = possibleMatches.begin();
                it != possibleMatches.end();
                ++it)
            {
                efd::UInt32 instance = (*it)->GetBlockInstance();
                if (instance == m_blockInstance)
                {
                    found = true;
                    linkValue = (*it)->GetEntityID();
                    // found an exact match, so we're done
                    break;
                }
                else if (0 == instance)
                {
                    found = true;
                    linkValue = (*it)->GetEntityID();
                    // keep looking for a better match
                }
            }

            if (found)
            {
                m_errors += ld.SetLink(m_blockFile, kENTITY_INVALID);
                continue;
            }
        }

        // Note: A missing link does not fail the block load. With new mechanisms to
        // conditionally load some entities in a block the link could be missing if the
        // destination entity is not created whereas previously this was an unexpected case.
        // We still log it as an error as it's still a bad condition, but we don't increment
        // m_errors as that would cause the entire block load to fail.
        utf8string idstring;
        efd::ParseHelper<efd::ID128>::ToString(ld.m_idDest, idstring);

        EE_LOG(efd::kEntity, efd::ILogger::kERR1,
            ("Warning parsing entity set file: '%s'. Missing target entity "
            "%s while linking entity %s property '%s'.",
            m_blockFile.c_str(),
            idstring.c_str(),
            ld.m_pSource->GetEntityID().ToString().c_str(),
            ld.m_pSource->GetModel()->GetPropertyDescriptor(
                ld.m_PropertyToLink)->GetName().c_str()));
    }
}

//------------------------------------------------------------------------------------------------
efd::UInt32 SAXEntityParser::LinkData::SetLink(
    const efd::utf8string& blockFile,
    egf::EntityID targetID)
{
    EE_UNUSED_ARG(blockFile); // not used when logging is disabled.

    PropertyResult result;
    if (m_key.empty())
    {
        result = m_pSource->SetPropertyValue(m_PropertyToLink, targetID);
        if (PropertyResult_OK != result)
        {
            EE_LOG(efd::kEntity, efd::ILogger::kERR0,
                ("Error parsing entity set file: '%s'.  "
                "Failed to set EntityRef property '%s'.",
                blockFile.c_str(),
                m_pSource->GetModel()->GetPropertyDescriptor(
                m_PropertyToLink)->GetName().c_str()));
            return 1;
        }
    }
    else
    {
        result = m_pSource->SetPropertyValue(m_PropertyToLink, m_key, targetID);
        if (PropertyResult_OK != result)
        {
            EE_LOG(efd::kEntity, efd::ILogger::kERR0,
                ("Error parsing entity set file: '%s'.  "
                "Failed to set EntityRef property '%s' ->'%s'.",
                blockFile.c_str(),
                m_pSource->GetModel()->GetPropertyDescriptor(
                m_PropertyToLink)->GetName().c_str(),
                m_key.c_str()));
            return 1;
        }
    }
    return 0;
}

//------------------------------------------------------------------------------------------------
