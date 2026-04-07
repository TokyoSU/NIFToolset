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

#include "efdPCH.h"
#include <efd/DDEParser.h>
#include <efd/ParseHelper.h>
#include <efd/EnumManager.h>

#include <efd/ILogger.h>
#include <efd/efdLogIDs.h>

#if defined(EE_USE_NATIVE_STL)
#include <limits>
#else
#include <limits>
#endif
#include <efd/ClassIDHelpers.h>
#include <efd/File.h>

#define MyLogMsg(msg) EE_LOG(efd::kEnumeration, efd::ILogger::kLVL1, msg)
#define MyLogErr(msg) EE_LOG(efd::kEnumeration, efd::ILogger::kERR1, msg)


namespace efd
{

//------------------------------------------------------------------------------------------------

namespace details
{
    enum EnumAddResult
    {
        ear_Success,

        // Attempt to add a name that already exists
        ear_DuplicateName,

        // Attempt to add a value that already exists
        ear_DuplicateValue,

        // Attempt to add a value that is beyond the maximum allowed value for this enum
        ear_RangeError,

        // Attempt to add an alias for a value when that value is not in use
        ear_AliasValueError,
    };


    template< typename T >
    class DataDrivenEnumCreator : public efd::DataDrivenEnum<T>
    {
        typedef efd::DataDrivenEnum<T> Base;
        T m_nextValue;
        T m_maxValue;

    public:
        DataDrivenEnumCreator(const efd::utf8string& i_name, efd::EnumType i_et)
            : DataDrivenEnum<T>(i_name, i_et)
        {
            Base::m_invalidValue = 0;
            m_nextValue = 1;
            m_maxValue = (EE_STL_NAMESPACE::numeric_limits<T>::max)();
        }

        DataDrivenEnumCreator(const efd::utf8string& i_name, DataDrivenEnum<T>* i_pParent)
            : DataDrivenEnum<T>(i_name, i_pParent)
        {
            DataDrivenEnumCreator<T>* pParent = (DataDrivenEnumCreator<T>*)i_pParent;

            Base::m_invalidValue = pParent->m_invalidValue;
            m_nextValue = pParent->m_nextValue;
            m_maxValue = pParent->m_maxValue;
        }

        bool SetMax(T maxValue)
        {
            if (m_maxValue > m_nextValue)
            {
                m_maxValue = maxValue;
                return true;
            }
            return false;
        }

        /// Set the initial value to be used in this enum
        /// @note If using SetInvalid or NoInvalidEntry, you must call those methods before
        /// calling SetStart.
        void SetStart(const T& value)
        {
            m_nextValue = value;
        }

        // Add an item with the next available value
        EnumAddResult AddItem(const efd::utf8string& name)
        {
            return AddItem(name, m_nextValue);
        }

        // Add an item with a specific value
        EnumAddResult AddItem(const efd::utf8string& name, const T& value)
        {
            // verify value is in range
            if (value > m_maxValue)
            {
                return ear_RangeError;
            }
            if (Base::HasName(name))
            {
                return ear_DuplicateName;
            }
            if (Base::HasValue(value))
            {
                return ear_DuplicateValue;
            }

            Base::m_nameToValue[ name ] = value;
            Base::m_valueToName[ value ] = name;

            m_nextValue = value;

            if (Base::GetEnumType() == et_Bitwise)
            {
                m_nextValue <<= 1;
            }
            else
            {
                ++m_nextValue;
            }

            return ear_Success;
        }

        // Add an alias with a specific value.
        EnumAddResult AddAlias(const efd::utf8string& name, const T& value)
        {
            if (Base::HasName(name))
            {
                return ear_DuplicateName;
            }
            if (!Base::HasValue(value))
            {
                return ear_AliasValueError;
            }

            // Aliases ONLY map from Name to Value, there is already another value to name
            // mapping for the non-aliased version
            Base::m_nameToValue[ name ] = value;

            return ear_Success;
        }

        // skip an enum value
        EnumAddResult AddPlaceholder()
        {
            if (m_nextValue > m_maxValue)
            {
                return ear_RangeError;
            }

            ++m_nextValue;
            return ear_Success;
        }

        // This enum has no reserved "invalid" value
        void NoInvalidEntry()
        {
            Base::m_fUseInvalid = false;
            m_nextValue = 0;
        }

        bool SetInvalid(const efd::utf8string& name, const T& value)
        {
            if (!Base::m_spParent)
            {
                Base::m_fUseInvalid = true;
                if (ear_Success == AddItem(name, value))
                {
                    // Here I'm checking for "value < 0", but I cannot simply say "value < 0"
                    // because then GCC generates a warning about "condition always true" which
                    // we then treat as an error.
                    if (value != 0 && value < 1)
                    {
                        m_nextValue = 0;
                    }
                    else
                    {
                        m_nextValue = value;
                        ++m_nextValue;
                    }

                    Base::m_invalidName = name;
                    Base::m_invalidValue = value;
                    return true;
                }
            }
            return false;
        }

        //@}

    protected:
    };

} // end namespace details
} // end namespace efd


using namespace efd;


//------------------------------------------------------------------------------------------------
DDEParser::DDEParser(efd::EnumManager* pEnumMgr, IDDEHeaderGenerator* pHeaderGen)
    : m_errors(0)
    , m_enumsAdded(0)
    , m_sourceFile("")
    , m_parserState(NotParsing)
    , m_pEnumMgr(pEnumMgr)
    , m_spHeaderGen(pHeaderGen)
    , kTagHeader("header")
    , kTagEnum("enum")
    , kAttrType("type")
    , kAttrStorage("storage")
    , kAttrMax("max")
    , kAttrBase("base")
    , kAttrStart("start")
    , kAttrInvalid("invalid")
    , kTagItem("item")
    , kAttrName("name")
    , kAttrValue("value")
    , kTagAlias("alias")
{
    EE_ASSERT(m_pEnumMgr);
}


//------------------------------------------------------------------------------------------------
DDEParser::~DDEParser()
{
    // nothing to release -- everything is owned by other classes.
}

//------------------------------------------------------------------------------------------------
void DDEParser::Parse(const efd::utf8string& enumName, const efd::utf8string& sourceFile)
{
    // We need this name later when we add ourself to the Enum Manager:
    m_enumName = enumName;

    // Remember the file name for use in log statements:
    m_sourceFile = sourceFile;

    MyLogMsg(("Parsing Data Driven Enum '%s' from file '%s'.",
        m_enumName.c_str(), m_sourceFile.c_str()));

    // create a SAX parser
    TiXmlDocument parser (sourceFile.c_str());

    File* pkFile = efd::File::GetFile(parser.Value(), File::READ_ONLY);
    if (pkFile)
    {
        unsigned int uiSize = pkFile->GetFileSize();
        char* pBuffer = EE_ALLOC(char, uiSize + 1);
        pkFile->Read(pBuffer, uiSize);
        pBuffer[uiSize] = '\0';
        parser.SAXParseBuffer(pBuffer, this);
        EE_FREE(pBuffer);
        EE_DELETE(pkFile);
    }
}


//------------------------------------------------------------------------------------------------
void DDEParser::startElement(const EE_TIXML_STRING& localname, const TiXmlAttributeSet& attrs)
{
    // switch on the current state of the parser
    switch (m_parserState)
    {
    case NotParsing:
        // First tag must be Enum.  if something else comes first, then that's an error
        if (localname == kTagEnum)
        {
            m_parserState = InEnum;
            ParseEnum(attrs);
        }
        else
        {
            MyLogErr(("Error parsing enum file: '%s'.\nExpected a 'header' or 'enum' tag",
                m_sourceFile.c_str()));

            ++m_errors;
        }
        break;

    case InEnum:
        // look for either a property, behavior, trait, or mixin tag.  if something else
        // comes first, then that's an error
        if (localname == kTagHeader)
        {
            m_parserState = InHeader;
            ParseHeader(attrs);
        }
        else if (localname == kTagItem)
        {
            m_parserState = InItem;
            ParseItem(attrs);
        }
        else if (localname == kTagAlias)
        {
            m_parserState = InAlias;
            ParseAlias(attrs);
        }
        else
        {
            MyLogErr(("Error parsing enum file: '%s'.\nExpected an 'item' or 'alias' tag",
                m_sourceFile.c_str()));

            ++m_errors;
        }
        break;

    default:
        // This should be unreachable
        EE_FAIL_MESSAGE(("Error parsing enum file: '%s'.  Invalid state %d.",
            m_sourceFile.c_str(), m_parserState));
        MyLogErr(("Error parsing enum file: '%s'.\nInvalid state %d.",
            m_sourceFile.c_str(), m_parserState));
        break;
    }
}


//------------------------------------------------------------------------------------------------
void DDEParser::endElement(const EE_TIXML_STRING& localname)
{
    switch (m_parserState)
    {
    case InEnum:
        // should only get here at the end of the model
        if (localname == kTagEnum)
        {
            FinishedEnum();
            m_parserState = NotParsing;
        }
        else
        {
            MyLogErr(("Error parsing enum file: '%s'.\nExpected an ending 'enum' tag",
                m_sourceFile.c_str()));

            ++m_errors;
        }
        break;

    case InHeader:
        // should only get here at the end of the model
        if (localname == kTagHeader)
        {
            m_parserState = InEnum;
        }
        else
        {
            MyLogErr(("Error parsing enum file: '%s'.\nExpected an ending 'header' tag",
                m_sourceFile.c_str()));

            ++m_errors;
        }
        break;

    case InItem:
        // should only get here at the end of a interval
        if (localname == kTagItem)
        {
            m_parserState = InEnum;
        }
        else
        {
            MyLogErr(("Error parsing enum file: '%s'.\nExpected an ending 'item' tag",
                m_sourceFile.c_str()));

            ++m_errors;
        }
        break;

    case InAlias:
        // should only get here at the end of a qos
        if (localname == kTagAlias)
        {
            m_parserState = InEnum;
        }
        else
        {
            MyLogErr(("Error parsing enum file: '%s'.\nExpected an ending 'alias' tag",
                m_sourceFile.c_str()));

            ++m_errors;
        }
        break;

    default:
        // This should be unreachable
        MyLogErr(("Error parsing enum file: '%s'.\nInvalid state %d.",
            m_sourceFile.c_str(), m_parserState));
        EE_ASSERT(false);
        break;
    }
}


//------------------------------------------------------------------------------------------------
void DDEParser::characters(const EE_TIXML_STRING& chars)
{
    // called for any characters that aren't tags.  This even, apparently, includes whitespace at
    // the document level.  That means if you need to do any meaningful parsing here you MUST
    // pay close attention to the current parser state.
    EE_UNUSED_ARG(chars);
}


//------------------------------------------------------------------------------------------------
void DDEParser::ParseHeader(const TiXmlAttributeSet& attrs)
{
    if (m_spHeaderGen)
    {
        if (!m_spHeaderGen->ParseHeaderTag(m_sourceFile, attrs))
        {
            // details of the error are logged inside of Finish method.
            ++m_errors;
        }
    }
}


//------------------------------------------------------------------------------------------------
void DDEParser::ParseEnum(const TiXmlAttributeSet& attrs)
{
    EE_ASSERT(!m_spDataDrivenEnum);

    DataDrivenEnumBase* pBaseEnum = NULL;
    utf8string baseEnum;
    if (XMLUtils::GetAttribute(attrs, kAttrBase, baseEnum))
    {
        pBaseEnum = m_pEnumMgr->FindOrLoadEnum(baseEnum);
        if (!pBaseEnum)
        {
            MyLogErr(("Error parsing enum file: '%s'.\nBase enum '%s' not found.",
                m_sourceFile.c_str(), baseEnum.c_str()));
            ++m_errors;
            return;
        }
    }

    // The storage type only effects header file generation, ignore this field otherwise.
    efd::ClassID storageClass = efd::kTypeID_UInt32;
    if (pBaseEnum)
    {
        storageClass = pBaseEnum->GetStorageType();
    }

    utf8string storage;
    if (XMLUtils::GetAttribute(attrs, kAttrStorage, storage))
    {
        if (pBaseEnum)
        {
            MyLogErr(("Error parsing enum file: '%s'.\nStorage type '%s' ignored, "
                "base value will be used.",
                m_sourceFile.c_str(), storage.c_str()));
            ++m_errors;
        }
        else if (storage == "efd::UInt8") storageClass = efd::kTypeID_UInt8;
        else if (storage == "efd::SInt8") storageClass = efd::kTypeID_SInt8;
        else if (storage == "efd::UInt16") storageClass = efd::kTypeID_UInt16;
        else if (storage == "efd::SInt16") storageClass = efd::kTypeID_SInt16;
        else if (storage == "efd::UInt32") storageClass = efd::kTypeID_UInt32;
        else if (storage == "efd::SInt32") storageClass = efd::kTypeID_SInt32;
        else if (storage == "efd::UInt64") storageClass = efd::kTypeID_UInt64;
        else if (storage == "efd::SInt64") storageClass = efd::kTypeID_SInt64;
        else
        {
            MyLogErr(("Error parsing enum file: '%s'.\nStorage type '%s' not supported.",
                m_sourceFile.c_str(), storage.c_str()));
            ++m_errors;
        }
    }

    switch (storageClass)
    {
    case efd::kTypeID_UInt8:
        ParseEnumHelper<efd::UInt8>(attrs, pBaseEnum);
        break;
    case efd::kTypeID_SInt8:
        ParseEnumHelper<efd::SInt8>(attrs, pBaseEnum);
        break;
    case efd::kTypeID_UInt16:
        ParseEnumHelper<efd::UInt16>(attrs, pBaseEnum);
        break;
    case efd::kTypeID_SInt16:
        ParseEnumHelper<efd::SInt16>(attrs, pBaseEnum);
        break;
    case efd::kTypeID_UInt32:
        ParseEnumHelper<efd::UInt32>(attrs, pBaseEnum);
        break;
    case efd::kTypeID_SInt32:
        ParseEnumHelper<efd::SInt32>(attrs, pBaseEnum);
        break;
    case efd::kTypeID_UInt64:
        ParseEnumHelper<efd::UInt64>(attrs, pBaseEnum);
        break;
    case efd::kTypeID_SInt64:
        ParseEnumHelper<efd::SInt64>(attrs, pBaseEnum);
        break;
    }
}

//------------------------------------------------------------------------------------------------
template< typename T >
void DDEParser::ParseEnumHelper(const TiXmlAttributeSet& attrs,
                                 efd::DataDrivenEnumBase* pBaseEnum)
{
    efd::DataDrivenEnum<T>* pTypedParent = NULL;
    if (pBaseEnum)
    {
        pTypedParent = pBaseEnum->CastTo<T>();
        if (!pTypedParent)
        {
            EE_FAIL("This should be impossible since we used the parent enum to find our type!");
            MyLogErr(("Error parsing enum file: '%s'.\n"
                "Base enum '%s' has incompatible storage type.",
                m_sourceFile.c_str(), pBaseEnum->GetName().c_str()));
            ++m_errors;
            return;
        }
    }

    efd::EnumType et = efd::et_Normal;

    utf8string type;
    if (XMLUtils::GetAttribute(attrs, kAttrType, type))
    {
        if (pBaseEnum)
        {
            MyLogErr(("Error parsing enum file: '%s'.\nType '%s' ignored, base value will be used.",
                m_sourceFile.c_str(), type.c_str()));
            ++m_errors;
        }
        else if (type == "Bitfield")
        {
            et = efd::et_Bitwise;
        }
        else if (type == "Normal")
        {
            // do nothing
        }
        else
        {
            MyLogErr(("Error parsing enum file: '%s'.\nInvalid enum type '%s'.",
                m_sourceFile.c_str(), type.c_str()));
            ++m_errors;
        }
    }

    efd::details::DataDrivenEnumCreator<T>* pEnum = NULL;
    if (pTypedParent)
    {
        pEnum = EE_NEW efd::details::DataDrivenEnumCreator<T>(m_enumName, pTypedParent);
    }
    else
    {
        pEnum = EE_NEW efd::details::DataDrivenEnumCreator<T>(m_enumName, et);
    }

    m_spDataDrivenEnum = pEnum;


    // This determines whether we start at zero or one by default
    utf8string invalid;
    if (XMLUtils::GetAttribute(attrs, kAttrInvalid, invalid))
    {
        if (pTypedParent)
        {
            MyLogErr(("Error parsing enum file: '%s'.\n"
                "'invalid' attribute '%s' ignored, base value will be used.",
                m_sourceFile.c_str(), invalid.c_str()));
            ++m_errors;
        }
        else if (invalid.empty())
        {
            pEnum->NoInvalidEntry();
        }
        else
        {
            T value;
            if (SplitNameAndValue(invalid, value))
            {
                pEnum->SetInvalid(invalid, value);
            }
            else
            {
                MyLogErr(("Error parsing enum file: '%s'.\n"
                    "Failed to parse 'invalid' attribute '%s'.",
                    m_sourceFile.c_str(), invalid.c_str()));
                ++m_errors;
            }
        }
    }

    T start;
    if (XMLUtils::GetAttribute(attrs, kAttrStart, start))
    {
        // DT32375 verify that the start value is in a valid range (and is a power of 2 for bitwise
        // enums)
        pEnum->SetStart(start);
    }

    // determines the maximum value
    T maxValue;
    if (XMLUtils::GetAttribute(attrs, kAttrMax, maxValue))
    {
        if (!pEnum->SetMax(maxValue))
        {
            MyLogErr(("Error parsing enum file: '%s'.\nMax value must be larger than start value.",
                m_sourceFile.c_str()));
            ++m_errors;
        }
    }
}


//------------------------------------------------------------------------------------------------
void DDEParser::ParseItem(const TiXmlAttributeSet& attrs)
{
    if (m_spDataDrivenEnum)
    {
        switch (m_spDataDrivenEnum->GetStorageType())
        {
        case efd::kTypeID_UInt8:
            ParseItemHelper<efd::UInt8>(attrs);
            break;
        case efd::kTypeID_SInt8:
            ParseItemHelper<efd::SInt8>(attrs);
            break;
        case efd::kTypeID_UInt16:
            ParseItemHelper<efd::UInt16>(attrs);
            break;
        case efd::kTypeID_SInt16:
            ParseItemHelper<efd::SInt16>(attrs);
            break;
        case efd::kTypeID_UInt32:
            ParseItemHelper<efd::UInt32>(attrs);
            break;
        case efd::kTypeID_SInt32:
            ParseItemHelper<efd::SInt32>(attrs);
            break;
        case efd::kTypeID_UInt64:
            ParseItemHelper<efd::UInt64>(attrs);
            break;
        case efd::kTypeID_SInt64:
            ParseItemHelper<efd::SInt64>(attrs);
            break;
        }
    }
}

//------------------------------------------------------------------------------------------------
template< typename T >
void DDEParser::ParseItemHelper(const TiXmlAttributeSet& attrs)
{
    efd::details::DataDrivenEnumCreator<T>* pEnum =
        (efd::details::DataDrivenEnumCreator<T>*)m_spDataDrivenEnum->CastTo<T>();

    utf8string name;
    if (!XMLUtils::GetAttribute(attrs, kAttrName, name) || name.empty())
    {
        pEnum->AddPlaceholder();
        return;
    }

    efd::utf8string strValue;
    if (XMLUtils::GetAttribute(attrs, kAttrValue, strValue))
    {
        T value;
        if (ParseValue(strValue, value))
        {
            switch (pEnum->AddItem(name, value))
            {
            case efd::details::ear_Success:
                // do nothing
                break;

            case efd::details::ear_DuplicateValue:
                MyLogErr(("Error parsing enum file: '%s'.\n"
                    "Failed to add item '%s', value %s is a duplicate.",
                    m_sourceFile.c_str(), name.c_str(), strValue.c_str()));
                ++m_errors;
                break;

            case efd::details::ear_RangeError:
                MyLogErr(("Error parsing enum file: '%s'.\n"
                    "Failed to add item '%s', value %s is too large.",
                    m_sourceFile.c_str(), name.c_str(), strValue.c_str()));
                ++m_errors;
                break;

            default:
                MyLogErr(("Error parsing enum file: '%s'.\nFailed to add item '%s' with value %s.",
                    m_sourceFile.c_str(), name.c_str(), strValue.c_str()));
                ++m_errors;
                break;
            }
        }
        else
        {
            MyLogErr(("Error parsing enum file: '%s'.\nEnum value '%s' did not parse.",
                m_sourceFile.c_str(), name.c_str(), strValue.c_str()));
            ++m_errors;
        }
    }
    else
    {
        switch (pEnum->AddItem(name))
        {
        case efd::details::ear_Success:
            // do nothing
            break;

        case efd::details::ear_RangeError:
            MyLogErr(("Error parsing enum file: '%s'.\n"
                "Failed to add item '%s', enumeration is full.",
                m_sourceFile.c_str(), name.c_str()));
            ++m_errors;
            break;

        default:
            MyLogErr(("Error parsing enum file: '%s'.\nFailed to add item '%s'.",
                m_sourceFile.c_str(), name.c_str()));
            ++m_errors;
            break;
        }
    }
}


//------------------------------------------------------------------------------------------------
void DDEParser::ParseAlias(const TiXmlAttributeSet& attrs)
{
    if (m_spDataDrivenEnum)
    {
        switch (m_spDataDrivenEnum->GetStorageType())
        {
        case efd::kTypeID_UInt8:
            ParseAliasHelper<efd::UInt8>(attrs);
            break;
        case efd::kTypeID_SInt8:
            ParseAliasHelper<efd::SInt8>(attrs);
            break;
        case efd::kTypeID_UInt16:
            ParseAliasHelper<efd::UInt16>(attrs);
            break;
        case efd::kTypeID_SInt16:
            ParseAliasHelper<efd::SInt16>(attrs);
            break;
        case efd::kTypeID_UInt32:
            ParseAliasHelper<efd::UInt32>(attrs);
            break;
        case efd::kTypeID_SInt32:
            ParseAliasHelper<efd::SInt32>(attrs);
            break;
        case efd::kTypeID_UInt64:
            ParseAliasHelper<efd::UInt64>(attrs);
            break;
        case efd::kTypeID_SInt64:
            ParseAliasHelper<efd::SInt64>(attrs);
            break;
        }
    }
}

//------------------------------------------------------------------------------------------------
template< typename T >
void DDEParser::ParseAliasHelper(const TiXmlAttributeSet& attrs)
{
    efd::details::DataDrivenEnumCreator<T>* pEnum =
        (efd::details::DataDrivenEnumCreator<T>*)m_spDataDrivenEnum->CastTo<T>();

    efd::utf8string name;
    if (!XMLUtils::GetAttribute(attrs, kAttrName, name))
    {
        MyLogErr(("Error parsing enum file: '%s'.\nMissing attribute 'name' in 'alias' tag.",
            m_sourceFile.c_str()));
        ++m_errors;
    }

    efd::utf8string valueName;
    if (!XMLUtils::GetAttribute(attrs, kAttrValue, valueName))
    {
        MyLogErr(("Error parsing enum file: '%s'.\nMissing attribute 'value' in 'alias' tag.",
            m_sourceFile.c_str()));
        ++m_errors;
    }

    T rawValue;
    if (pEnum->FindEnumValue(valueName, rawValue))
    {
        switch (pEnum->AddAlias(name, rawValue))
        {
        case efd::details::ear_Success:
            // do nothing
            break;

        case efd::details::ear_DuplicateName:
            MyLogErr(("Error parsing enum file: '%s'.\nCannot add alias '%s', name already in use.",
                m_sourceFile.c_str(), name.c_str()));
            ++m_errors;
            break;

        default:
            MyLogErr(("Error parsing enum file: '%s'.\nCannot add alias '%s'.",
                m_sourceFile.c_str(), name.c_str()));
            ++m_errors;
            break;
        }
    }
    else
    {
        // attempt to parse bitwise enum value delimited by '|'
        efd::vector< utf8string > results;
        const utf8string delimiter = "|";
        valueName.split(delimiter, results);
        if (results.size())
        {
            rawValue = 0;
            T tempValue;
            for (efd::vector< utf8string >::iterator it = results.begin();
                it != results.end();
                ++it)
            {
                if (pEnum->FindEnumValue(*it, tempValue))
                {
                    rawValue |= tempValue;
                }
                else
                {
                    MyLogErr(("Error parsing enum file: '%s'.\nAlias value '%s' does not exist.",
                        m_sourceFile.c_str(), (*it).c_str()));
                    ++m_errors;
                    return;
                }
            }
            //pEnum->AddItem(valueName, rawValue);
            switch (pEnum->AddItem(name, rawValue))
            {
            case efd::details::ear_Success:
                // do nothing
                break;

            case efd::details::ear_DuplicateName:
                MyLogErr(("Error parsing enum file: '%s'.\n"
                    "Cannot add alias '%s', name already in use.",
                    m_sourceFile.c_str(), name.c_str()));
                ++m_errors;
                break;

            default:
                MyLogErr(("Error parsing enum file: '%s'.\nCannot add alias '%s'.",
                    m_sourceFile.c_str(), name.c_str()));
                ++m_errors;
                break;
            }
        }
        else
        {
            MyLogErr(("Error parsing enum file: '%s'.\nAlias value '%s' does not exist.",
                m_sourceFile.c_str(), valueName.c_str()));
            ++m_errors;
        }
    }
}
//------------------------------------------------------------------------------------------------
void DDEParser::FinishedEnum()
{
    if (m_spDataDrivenEnum)
    {
        if (m_pEnumMgr->AddEnum(m_spDataDrivenEnum))
        {
            MyLogMsg(("Added Data Driven Enum '%s' from file '%s'.",
                m_spDataDrivenEnum->GetName().c_str(), m_sourceFile.c_str()));
            ++m_enumsAdded;
        }
        else
        {
            MyLogErr(("Error parsing enum file: '%s'.\nCannot add enum '%s' to Enum Manager.",
                m_sourceFile.c_str(), m_spDataDrivenEnum->GetName().c_str()));
            ++m_errors;
        }

        if (m_spHeaderGen)
        {
            if (!m_spHeaderGen->GenerateEnum(m_spDataDrivenEnum))
            {
                // details of the error are logged inside of Finish method.
                ++m_errors;
            }
        }

        m_spDataDrivenEnum = NULL;
    }
}

//------------------------------------------------------------------------------------------------
template< typename T >
bool DDEParser::SplitNameAndValue(efd::utf8string& io_invalid, T& o_value)
{
    // Default to zero if no value is given
    o_value = (T)0;

    utf8string::size_type pos = io_invalid.find_first_of("=");
    if (utf8string::npos == pos)
    {
        // No "=" found, that means the entire string is the invalid tag and the default zero is
        // the correct o_value result.
        return true;
    }

    utf8string strValue = io_invalid.substr(pos+1);
    io_invalid = io_invalid.substr(0, pos);

    // In the case of "=12345" use the default name:
    if (io_invalid.empty())
    {
        io_invalid = "INVALID";
    }

    if (!strValue.empty())
    {
        return ParseValue(strValue, o_value);
    }

    return true;
}

//------------------------------------------------------------------------------------------------
template< typename T >
bool DDEParser::ParseValue(const efd::utf8string& i_strValue, T& o_value)
{
    bool parsed = efd::ParseHelper< T >::FromString(i_strValue, o_value);
    if (!parsed)
    {
        if (i_strValue == "EE_MAX")
        {
            o_value = (EE_STL_NAMESPACE::numeric_limits<T>::max)();
            parsed = true;
        }
        else if (i_strValue == "EE_MIN")
        {
            o_value = (EE_STL_NAMESPACE::numeric_limits<T>::min)();
            parsed = true;
        }
    }
    return parsed;
}

//------------------------------------------------------------------------------------------------
