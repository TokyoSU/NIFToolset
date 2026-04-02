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
#include <efd/EnumManager.h>
#include <efd/DDEParser.h>
#include <efd/IConfigManager.h>
#include <efd/ServiceManager.h>
#include <efd/PathUtils.h>
#include <efd/IDs.h>

//------------------------------------------------------------------------------------------------
EE_IMPLEMENT_CONCRETE_CLASS_INFO(efd::EnumManager);

efd::IDs* efd::IDs::s_firstID=NULL;

//------------------------------------------------------------------------------------------------
efd::EnumManager::EnumManager()
    : m_configLoaded(false)
#ifdef EE_PLATFORM_XBOX360
    , m_loadPath("D:\\")
#else
    , m_loadPath(".\\")
#endif
    , m_lastIDProcessed(NULL)
{
    // If this default priority is changed, also update the service quick reference documentation
    m_defaultPriority = 2600;
}

//------------------------------------------------------------------------------------------------
const char* efd::EnumManager::GetDisplayName() const
{
    return "EnumManager";
}

//------------------------------------------------------------------------------------------------
efd::utf8string efd::EnumManager::GetFileForEnum(const efd::utf8string& enumName)
{
    efd::utf8string result = efd::PathUtils::PathCombine(m_loadPath, enumName);
    result = efd::PathUtils::PathAddExtension(result, "enum");
    return result;
}

//------------------------------------------------------------------------------------------------
efd::DataDrivenEnumBase* efd::EnumManager::LoadEnum(const efd::utf8string& enumName)
{
    EE_ASSERT(NULL == FindEnum(enumName));

    efd::DDEParser parser(this, m_pHeaderGen);
    parser.Parse(enumName, GetFileForEnum(enumName));
    return FindEnum(enumName);
}

//------------------------------------------------------------------------------------------------
efd::DataDrivenEnumBase* efd::EnumManager::LoadFile(const efd::utf8string& fileName)
{
    utf8string enumName = efd::PathUtils::PathGetFileName(fileName);
    enumName = efd::PathUtils::PathRemoveFileExtension(enumName);

    EE_ASSERT(NULL == FindEnum(enumName));

    efd::DDEParser parser(this, m_pHeaderGen);
    parser.Parse(enumName, fileName);
    return FindEnum(enumName);
}

//------------------------------------------------------------------------------------------------
efd::DataDrivenEnumBase* efd::EnumManager::FindOrLoadEnum(const efd::utf8string& enumName)
{
    efd::DataDrivenEnumBase* pResult = FindEnum(enumName);
    if (!pResult)
    {
        pResult = LoadEnum(enumName);
    }
    return pResult;
}

//------------------------------------------------------------------------------------------------
efd::DataDrivenEnumBase* efd::EnumManager::FindEnum(const efd::utf8string& enumName)
{
    DDEnumMap::iterator iter = m_enums.find(enumName);
    if (iter != m_enums.end())
    {
        return iter->second;
    }

    return NULL;
}

//------------------------------------------------------------------------------------------------
bool efd::EnumManager::AddEnum(efd::DataDrivenEnumBase* pEnum)
{
    DDEnumMap::iterator iter = m_enums.find(pEnum->GetName());
    if (iter == m_enums.end())
    {
        m_enums[pEnum->GetName()] = pEnum;
        return true;
    }

    // Duplicate item
    return false;
}

//------------------------------------------------------------------------------------------------
void efd::EnumManager::SetHeaderGenerator(efd::IDDEHeaderGenerator* pHeaderGen)
{
    m_pHeaderGen = pHeaderGen;
}

//------------------------------------------------------------------------------------------------
void efd::EnumManager::LoadConfigOptions(efd::IConfigManager* pConfig)
{
    if (pConfig->FindValue("Enum.Path", m_loadPath))
    {
        m_loadPath = efd::PathUtils::PathMakeNative(m_loadPath);
        if (!efd::PathUtils::GetPlatformSupportsRelativePaths())
        {
            if (PathUtils::IsRelativePath(m_loadPath))
                m_loadPath = efd::PathUtils::ConvertToAbsolute(m_loadPath);
        }
    }
    m_configLoaded = true;
}

//------------------------------------------------------------------------------------------------
efd::SyncResult efd::EnumManager::OnPreInit(efd::IDependencyRegistrar* pDependencyRegistrar)
{
    EE_UNUSED_ARG(pDependencyRegistrar);

    if (!m_configLoaded)
    {
        efd::IConfigManager* pConfig =
            m_pServiceManager->GetSystemServiceAs<efd::IConfigManager>();
        LoadConfigOptions(pConfig);
    }
    RegisterIDs();
    return efd::SyncResult_Success;
}

//------------------------------------------------------------------------------------------------
class ExternalIDsEnum : public efd::DataDrivenEnum<efd::UInt32>
{
public:
    ExternalIDsEnum():
      efd::DataDrivenEnum< efd::UInt32 >("ExternalIDs", efd::et_Normal){}

    // Add an item with a specific value
    bool AddItem(const efd::utf8string& name, efd::UInt32 IDvalue)
    {
        efd::UInt32 foundValue;
        if (!m_nameToValue.find(name, foundValue))
        {
            m_nameToValue[name] = IDvalue;
            m_valueToName[IDvalue] = name;
            return true;
        }
        else
        {
            bool valuesMatch = (foundValue == IDvalue);
            if (valuesMatch)
            {
                EE_LOG(
                    efd::kEnumeration,
                    efd::ILogger::kLVL2,
                    ("ExternalIDsEnum::AddItem(): Adding %s with value %d a second time.",
                    name.c_str(),
                    IDvalue));
                return true;
            }
            else
            {
                EE_LOG(
                    efd::kEnumeration,
                    efd::ILogger::kERR1,
                    ("ExternalIDsEnum::AddItem(): Adding %s with value %d, already exists with"
                    "value %d",
                    name.c_str(),
                    IDvalue,
                    foundValue));
                return false;
            }
        }
    }

};

//------------------------------------------------------------------------------------------------
void efd::EnumManager::RegisterIDs()
{
    IDs* h = NULL;

    efd::DataDrivenEnumPtr spEnum = FindEnum("ExternalIDs");
    ExternalIDsEnum* pExternalIDsEnum;
    if (!spEnum)
    {
        pExternalIDsEnum = EE_NEW ExternalIDsEnum;
        spEnum = pExternalIDsEnum;
        AddEnum(spEnum);
    }
    else
    {
        DataDrivenEnum< efd::UInt32 >* pDataDrivenEnum = spEnum->CastTo< efd::UInt32 >();
        pExternalIDsEnum = static_cast<ExternalIDsEnum*>(pDataDrivenEnum);
    }

    // Iterate all instances that are in the static list
    h = IDs::s_firstID;
    while (h != NULL && h != m_lastIDProcessed)
    {
        // Get them registered in the optimized map
        pExternalIDsEnum->AddItem(h->m_name, h->m_IDvalue);

        // drain the list.
        h = h->m_nextID;
    }
    m_lastIDProcessed = IDs::s_firstID;
}

