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

#include <egf/SAXBlockParser.h>
#include <egf/EntityManager.h>
#include <efd/ILogger.h>
#include <efd/EEHelpers.h>
#include <efd/ID128.h>
#include <efd/File.h>
#include <egf/egfLogIDs.h>

using namespace efd;
using namespace egf;

//------------------------------------------------------------------------------------------------
// Tags used in our parsing.  We need to set these at run-time because they are 16bit values
// even on Linux where "wide" strings are 32bit.
static EE_TIXML_STRING kEntityTagGame("game");
static EE_TIXML_STRING kEntityTagReferences("model_references");
static EE_TIXML_STRING kEntityTagReference("reference");

static EE_TIXML_STRING kEntityTagEntitySet("entitySet");

static EE_TIXML_STRING kEntityTagEntity("entity");
static EE_TIXML_STRING kEntityAttrModelName("modelName");
static EE_TIXML_STRING kEntityAttrEntityId("id");
static EE_TIXML_STRING kEntityAttrIteration("iterations");

static EE_TIXML_STRING kEntityTagProperty("property");
static EE_TIXML_STRING kEntityAttrPropertyName("name");

static EE_TIXML_STRING kEntityTagSet("set");
static EE_TIXML_STRING kEntityAttrSetIndex("index");
static EE_TIXML_STRING kEntityAttrSetValue("value");

static EE_TIXML_STRING kEntityTagToolSettings("toolSettings");


//------------------------------------------------------------------------------------------------
SAXBlockParser::SAXBlockParser(
    const efd::utf8string& blockFile,
    efd::set<efd::utf8string>& o_flatModelNameSet)
: m_blockFile(blockFile)
, m_parserState(NotParsing)
, m_flatModelNameSet(o_flatModelNameSet)
, m_errors(0)
{
}

//------------------------------------------------------------------------------------------------
SAXBlockParser::~SAXBlockParser()
{
    // nothing to release -- everything is owned by other classes.
}

//------------------------------------------------------------------------------------------------
efd::UInt32 SAXBlockParser::Parse(
    const efd::utf8string& xblockFile,
    efd::set<efd::utf8string>& o_flatModelNameSet)
{
    SAXBlockParser parser(xblockFile, o_flatModelNameSet);

    // create a SAX parser
    TiXmlDocument kDocument(xblockFile.c_str());

    File* pkFile = efd::File::GetFile(kDocument.Value(), File::READ_ONLY);
    if (pkFile)
    {
        unsigned int uiSize = pkFile->GetFileSize();
        char* pBuffer = EE_ALLOC(char, uiSize + 1);
        pkFile->Read(pBuffer, uiSize);
        pBuffer[uiSize] = '\0';
        kDocument.SAXParseBuffer(pBuffer, &parser);
        EE_FREE(pBuffer);
        EE_DELETE(pkFile);
    }
    else
    {
        parser.m_errors++;
    }

    return (efd::UInt32)kDocument.Error() + parser.m_errors;
}

//------------------------------------------------------------------------------------------------
efd::UInt32 SAXBlockParser::ParseBuffer(
    const efd::utf8string& name,
    const char* buffer,
    efd::set<efd::utf8string>& o_flatModelNameSet)
{
    SAXBlockParser parser(name, o_flatModelNameSet);
    TiXmlDocument kDocument(name.c_str());
    kDocument.SAXParseBuffer(buffer, &parser);
    return (efd::UInt32)kDocument.Error() + parser.m_errors;
}

//------------------------------------------------------------------------------------------------
void SAXBlockParser::startElement(const EE_TIXML_STRING& localname, const TiXmlAttributeSet& attrs)
{
    if (localname == kEntityTagGame ||
        localname == kEntityTagReference ||
        localname == kEntityTagReferences)
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
        if (localname == kEntityTagEntitySet)
        {
            m_parserState = InDocument;
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
        // look for the entity tag.
        if (localname == kEntityTagEntity)
        {
            ParseEntity(attrs);
        }
        else
        {
            // do nothing - skip this tag.
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
void SAXBlockParser::endElement(const EE_TIXML_STRING& localname)
{
    if (localname == kEntityTagGame ||
        localname == kEntityTagReference ||
        localname == kEntityTagReferences)
    {
        // Ignore tags used by the tool that don't effect runtime
        return;
    }

    switch (m_parserState)
    {
    case InDocument:
        // should only get here at the end of the entitySet.
        if (localname == kEntityTagEntitySet)
        {
            m_parserState = NotParsing;
        }
        break;

    case NotParsing:
        // Do nothing
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
void SAXBlockParser::characters(const EE_TIXML_STRING& chars)
{
    // called for any characters that aren't tags.  This even, apparently, includes whitespace at
    // the document level.  That means if you need to do any meaningful parsing here you MUST
    // pay close attention to the current parser state.

    EE_UNUSED_ARG(chars);
}

//------------------------------------------------------------------------------------------------
bool SAXBlockParser::ParseEntity(const TiXmlAttributeSet& attrs)
{
    utf8string modelName;

    // If we get to here, it means that the parse is not a reload or that the entity has been
    // modified.
    if (XMLUtils::GetAttribute(attrs, kEntityAttrModelName, modelName))
    {
        m_flatModelNameSet.insert(modelName);
    }
    else
    {
        // Error, missing attribute modelName
        EE_LOG(efd::kEntity, efd::ILogger::kERR0,
            ("Error parsing entity set file: '%s'. "
            "Missing attribute 'modelName' in 'entity' tag.",
            m_blockFile.c_str()));
        ++m_errors;
    }
    return false;
}

