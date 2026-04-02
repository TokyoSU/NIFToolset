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

#include <efd/ConfigManager.h>
#include <efd/ConfigSource.h>
#include <efd/Metrics.h>
#include <efd/ILogger.h>
#include <efd/PathUtils.h>
#include <efd/efdLogIDs.h>
#include <efd/IServiceDetailRegister.h>

//------------------------------------------------------------------------------------------------
using namespace efd;

EE_IMPLEMENT_CONCRETE_CLASS_INFO(ConfigManager);

//! The character '.' used to separate section names.
const char IConfigManager::kSectionSeperator = '.';
//! The character '=' used to separate the name from the value in source files and on the
//! command-line.
const char IConfigManager::kNameValueSeperator = '=';
//! The character '>' used to link a name to an existing name/value pair.
const char IConfigManager::kNameLinkSeperator = '>';
//! The character '+' used to add an additional file source to the configuration with an optional
//! section to add it under specified to the left of the seperator.
const char IConfigManager::kNameFileSeperator = '+';
//! The character '#' used to specify that the name should be converted to a unique name in a list
//! but replacing the '#' character with unique number in sequence.
const char IConfigManager::kNameListChar = '#';
const /*static*/ char IConfigManager::kVarStart[] = "$(";
const /*static*/ char IConfigManager::kVarEnd[] = ")";
const /*static*/ char IConfigManager::kIllegalSectionChars[10] =
    {kNameValueSeperator, kNameLinkSeperator, 0};
const /*static*/ char IConfigManager::kIllegalNameChars[10] =
    {kNameValueSeperator, kNameLinkSeperator, 0};

//------------------------------------------------------------------------------------------------
ConfigManager::ConfigManager(
    const efd::utf8string& configFile,
    int argcInit,
    char **argvInit,
    bool processEnvVars)
    : m_root("", NULL, NULL)
{
    // If this default priority is changed, also update the service quick reference documentation
    m_defaultPriority = 8000;

    EE_LOG(efd::kConfiguration, efd::ILogger::kLVL1,
        ("Creating a ConfigManager instance %p",
        this));

    // Set the static root pointer in the section and section entry classes
    SectionEntry::ms_pRoot = &m_root;

#if defined(EE_EFD_CONFIG_DEBUG) && defined(EE_PLATFORM_XBOX360)
    // If we're an Xbox360 in Debug config and no command line args are hardcoded(some tests do)
    // then fetch any XDK command line args.
    if (argcInit == 0)
        ImportXDKCommandLine(argcInit, argvInit);
#endif

    METRICS_ONLY(double fCycleTimeStart = efd::GetCurrentTimeInSec());

    IConfigSourcePtr spCommandLine = EE_NEW CommandLineSource(argcInit, argvInit);
    AddConfigSource(spCommandLine, IConfigSource::kSourcePriorityCmdLine);

    if (!configFile.empty())
    {
        IConfigSourcePtr spIni = EE_NEW IniSource(configFile);
        AddConfigSource(spIni, IConfigSource::kSourcePriorityNormal);
    }

    if (processEnvVars)
    {
        IConfigSourcePtr spEnvVar = EE_NEW EnvVarSource();
        AddConfigSource(spEnvVar, IConfigSource::kSourcePriorityCmdLine);
    }

    EE_LOG(efd::kConfiguration, efd::ILogger::kLVL1,
        ("Reading all the initial configuration information"));

    METRICS_ONLY(double fCycleTimeEnd = efd::GetCurrentTimeInSec());
    EE_LOG_METRIC(kConfiguration, "READ.TOTAL.SEC", float(fCycleTimeEnd - fCycleTimeStart));

#ifdef EE_DEBUG_CONFIGMANAGER
    efd::utf8string fileName = "ConfigOutput.ini";
    WriteConfiguration(fileName);
#endif
}

//------------------------------------------------------------------------------------------------
ConfigManager::~ConfigManager()
{
    EE_LOG(efd::kConfiguration, efd::ILogger::kLVL1,
        ("Destroying a ConfigManager instance 0x%08X", this));

    m_sources.clear();
}

//------------------------------------------------------------------------------------------------
void ConfigManager::OnServiceRegistered(IAliasRegistrar* pAliasRegistrar)
{
    pAliasRegistrar->AddIdentity<IConfigManager>();
    IConfigManager::OnServiceRegistered(pAliasRegistrar);
}

//------------------------------------------------------------------------------------------------
void ConfigManager::WriteConfiguration(
    const efd::utf8string& strFileName,
    bool bPrintRelative /*= true*/)
{
    EE_LOG(efd::kConfiguration, efd::ILogger::kLVL1,
        ("Writing out the current configuration information"));

    METRICS_ONLY(double fCycleTimeStart = efd::GetCurrentTimeInSec());

    File *pkFile = File::GetFile(strFileName.c_str(), File::WRITE_ONLY);

    if (pkFile)
    {
        m_root.WriteConfiguration(pkFile, "", "", bPrintRelative);

        EE_DELETE pkFile;
    }

    METRICS_ONLY(double fCycleTimeEnd = efd::GetCurrentTimeInSec());
    EE_LOG_METRIC(kConfiguration, "WRITE.TOTAL.SEC", float(fCycleTimeEnd - fCycleTimeStart));
}

//------------------------------------------------------------------------------------------------
void ConfigManager::AddConfigSource(IConfigSource *pSource, int iPriority)
{
    if (pSource)
    {
        pSource->SetConfiguration(&m_root, iPriority);
        AddConfigSourceInternal(pSource);
    }
}

//------------------------------------------------------------------------------------------------
void ConfigManager::AddConfigSourceInternal(IConfigSource *pSource)
{
    EE_ASSERT(pSource);

    pSource->ReadConfiguration(this);
    m_sources.push_back(pSource);
}

//------------------------------------------------------------------------------------------------
void ConfigManager::RemoveConfigSource(IConfigSource *pSource)
{
    for (efd::list<IConfigSourcePtr>::iterator it = m_sources.begin();
        it != m_sources.end();
        ++it)
    {
        if (*it == pSource)
        {
            RemoveConfigSourceInternal(pSource);
            m_sources.erase(it);
            return;
        }
    }
}

//------------------------------------------------------------------------------------------------
const ISection* ConfigManager::GetConfiguration()
{
    return &m_root;
}

//------------------------------------------------------------------------------------------------
void ConfigManager::RemoveConfigSourceInternal(IConfigSource* pSource)
{
    m_root.RemoveConfigSource(pSource);
}

//------------------------------------------------------------------------------------------------
bool ConfigManager::FindValue(const efd::utf8string& strName, efd::utf8string& o_value) const
{
    return m_root.FindValue(strName, o_value);
}

//------------------------------------------------------------------------------------------------
efd::utf8string ConfigManager::FindValue(const efd::utf8string& strName) const
{
    return m_root.FindValue(strName);
}

//------------------------------------------------------------------------------------------------
bool ConfigManager::IsTrue(const efd::utf8string& strName, bool defaultValue) const
{
    return m_root.IsTrue(strName, defaultValue);
}

//------------------------------------------------------------------------------------------------
bool ConfigManager::IsFalse(const efd::utf8string& strName, bool defaultValue) const
{
    return m_root.IsFalse(strName, defaultValue);
}

//------------------------------------------------------------------------------------------------
#if defined(EE_EFD_CONFIG_DEBUG) && defined(EE_PLATFORM_XBOX360)
// spoof argv/argc with XDK GetCommandLine() results
/*static*/ void ConfigManager::ImportXDKCommandLine(int& argcInit, char **& argvInit)
{
    const int maxArgs = 6;

    EE_LOG(efd::kConfiguration, efd::ILogger::kLVL2,
        ("Importing command line arguments from XDK."));

    char* cmd = ::GetCommandLine();
    size_t maxArgLength = strlen(cmd);
    if (maxArgLength > 0)
    {
        argvInit = EE_ALLOC(char*, maxArgs);
        for (int i = 0; i < maxArgs; ++i)
        {
            // The largest single arg is no larger than the total arg length.
            argvInit[i] = EE_ALLOC(char, maxArgLength);
            *argvInit[i] = '\0';
        }
        argcInit = ::sscanf(cmd, "%s %s %s %s %s %s",
            argvInit[0], argvInit[1], argvInit[2], argvInit[3], argvInit[4], argvInit[5]);
        // clear unused entries in case later code iterates argvInit instead of trusting argcInit
        for (int i = argcInit; i < maxArgs; ++i)
        {
            EE_FREE(argvInit[i]);
            argvInit[i] = 0;
        }
    }
}
#endif  // defined(EE_EFD_CONFIG_DEBUG) && defined(EE_PLATFORM_XBOX360)

//------------------------------------------------------------------------------------------------
const char* ConfigManager::GetDisplayName() const
{
    return "ConfigManager";
}

//------------------------------------------------------------------------------------------------
