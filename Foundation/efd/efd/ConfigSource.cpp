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

#include <efd/ConfigSource.h>
#include <efd/ConfigSection.h>
#include <efd/IConfigManager.h>
#include <efd/Metrics.h>
#include <efd/Utilities.h>
#include <efd/ILogger.h>
#include <efd/Utilities.h>
#include <efd/ILogger.h>
#include <efd/PathUtils.h>
#include <efd/efdLogIDs.h>

#if defined(EE_PLATFORM_XBOX360)
#include "Xtl.h"
#include "Winbase.h"
#endif

using namespace efd;

//------------------------------------------------------------------------------------------------
EE_IMPLEMENT_CONCRETE_CLASS_INFO(ConfigSource);

//------------------------------------------------------------------------------------------------
ConfigSource::ConfigSource(IConfigSource::tSourceType sourceType,
    const efd::utf8string& sourceName)
    : m_sourceType(sourceType)
    , m_sourceName(sourceName)
    , m_basePath()
    , m_pRoot(NULL)
    , m_priority(0)
{
    EE_LOG(efd::kConfiguration, efd::ILogger::kLVL1,
        ("Creating a configuration source '%s' (0x%p) of type %i",
        sourceName.c_str(),
        this,
        sourceType));
}

//------------------------------------------------------------------------------------------------
/*virtual*/ ConfigSource::~ConfigSource()
{
    EE_LOG(efd::kConfiguration, efd::ILogger::kLVL1,
        ("Destroying a configuration source '%s' (0x%p) of type %i",
        m_sourceName.c_str(),
        this,
        m_sourceType));

#ifdef EE_EFD_CONFIG_DEBUG
    m_sourceType = IConfigSource::kSourceUnknown;
    m_sourceName = "INVALID";
    m_basePath = "INVALID";
    m_pRoot = NULL;
    m_priority = -INT_MAX;
#endif
}

//------------------------------------------------------------------------------------------------
/*virtual*/ int ConfigSource::GetPriority() const
{
    return m_priority;
}

//------------------------------------------------------------------------------------------------
/*virtual*/ unsigned int ConfigSource::GetSourceType() const
{
    return m_sourceType;
}

//------------------------------------------------------------------------------------------------
/*virtual*/ const efd::utf8string& ConfigSource::GetSourceName() const
{
    return m_sourceName;
}

//------------------------------------------------------------------------------------------------
/*virtual*/ efd::utf8string ConfigSource::SourceToString() const
{
    efd::utf8string temp(efd::Formatted,
        "Source Type: %i Source Name: \"%s\" Source Priority: %i",
        m_sourceType,
        m_sourceName.c_str(),
        m_priority);
    return temp;
}

//------------------------------------------------------------------------------------------------
/*virtual*/ void ConfigSource::SetConfiguration(ISection *pRoot, int iPriority)
{
    EE_LOG(efd::kConfiguration, efd::ILogger::kLVL2,
        ("Setting root configuration(0x%08X) and priority(%i) for %s",
        pRoot,
        iPriority,
        GetSourceName().c_str()));

    m_pRoot = pRoot;
    m_priority = iPriority;
}

//------------------------------------------------------------------------------------------------
/*virtual*/ bool ConfigSource::ProcessNameValuePair(
    IConfigManager *pManager,
    ISection *pSection,
    const efd::utf8string& pairString)
{
    bool bRetVal = false;

    // If there is a manager pointer allow new sources to be added
    efd::utf8string sepSet;
    if (pManager)
    {
        sepSet += efd::utf8char_t(IConfigManager::kNameFileSeperator);
    }
    // If there is a section pointer allow name/value pairs to be added
    if (pSection)
    {
        sepSet += efd::utf8char_t(IConfigManager::kNameValueSeperator);
        // If there is also a root pointer allow links to be added
        if (m_pRoot)
        {
            sepSet += efd::utf8char_t(IConfigManager::kNameLinkSeperator);
        }
    }

    // Search the string for the first seperator character
    size_t seperator = pairString.find_first_of(sepSet, 0);

    if (seperator != efd::utf8string::npos)
    {
        // Grab the seperator character
        char sepChar = pairString[seperator].ToAscii();

        // If we are here, then we have a name value pair possibly also
        // containing one or more section ids.
        // Break the string into separate name and value strings.
        efd::utf8string strName = pairString.substr(0, seperator);
        strName.trim();
        efd::utf8string strValue = pairString.substr(seperator + 1);
        strValue.trim();

        // Check if this is a name/value seperator
        if (sepChar == IConfigManager::kNameValueSeperator)
        {
            bRetVal = pSection->AddValue(m_pRoot, strName, strValue, m_priority, this);
        }
        // Check if this is a name/link seperator
        else if (sepChar == IConfigManager::kNameLinkSeperator)
        {
            const ISectionEntry *pILink = m_pRoot->FindEntry(strValue);
            bRetVal = pSection->AddLink(m_pRoot, strName, strValue, pILink, m_priority, this);
        }
        else
        {
            // Must be a name/file seperator
            EE_ASSERT(sepChar == IConfigManager::kNameFileSeperator);
            bRetVal = AddFileSource(pManager, strName, strValue, m_priority - 1);
        }

        if (!bRetVal)
        {
            EE_LOG(efd::kConfiguration, efd::ILogger::kERR2,
                ("Error: Could not add configuration name/value '%s'/'%s' to section %s",
                strName.c_str(),
                strValue.c_str(),
                (pSection != NULL) ? pSection->GetName().c_str() : "UNKNOWN"));
        }
    }

    return bRetVal;
}

//------------------------------------------------------------------------------------------------
/*virtual*/ bool ConfigSource::AddFileSource(
    IConfigManager *pManager,
    const efd::utf8string& sectionName,
    const efd::utf8string& fileName,
    int priority /*= IConfigSource::kSourcePriorityNormal*/)
{
    bool bRetVal = false;

    // Make sure we have valid pointers
    if (pManager && m_pRoot)
    {
        efd::utf8string fullPath = fileName;

        // If its a relative path and it doesn't start with a variable then we combine it
        // with the base path.
        if (PathUtils::IsRelativePath(fileName) && (fileName[0] != efd::utf8char_t('$')))
        {
            fullPath = PathUtils::PathCombine(m_basePath, fileName);
            fullPath = PathUtils::PathRemoveDotDots(fullPath);
        }

        // Create the new ini file source
        IConfigSourcePtr spSource = EE_NEW IniSource(fullPath);
        if (spSource)
        {
            // If there is a section name create the section so it can
            // be the root of the new source file
            ISection *pSection = m_pRoot;
            if (!sectionName.empty())
            {
                pSection = m_pRoot->AddSection(sectionName, this);
            }
            // If we have a valid root section then set the configuration
            // and add the new source to the managers list
            if (pSection)
            {
                spSource->SetConfiguration(pSection, priority);
                pManager->AddConfigSourceInternal(spSource);
                bRetVal = true;
            }
            else
            {
                EE_LOG(efd::kConfiguration, efd::ILogger::kERR2,
                    ("Could not create a valid root section for additional "
                    "configuration source to be added to(%s)",
                    fullPath.c_str()));
            }
        }
        else
        {
            EE_LOG(efd::kConfiguration, efd::ILogger::kERR2,
                ("Could not create a new Ini File Source for (%s)",
                fullPath.c_str()));
        }
    } // if (pManager)

    return bRetVal;
}


//------------------------------------------------------------------------------------------------
EE_IMPLEMENT_CONCRETE_CLASS_INFO(CommandLineSource);

const /*static*/ char *CommandLineSource::kDefaultSection = "CommandLineSource";
const /*static*/ char *CommandLineSource::kParam_name = "Param";
const /*static*/ char *CommandLineSource::kExeName = "ExeName";

//------------------------------------------------------------------------------------------------
CommandLineSource::CommandLineSource(int argcInit, char **argvInit)
    : ConfigSource(IConfigSource::kSourceCommandLine, "Command Line")
{
    m_pRoot = NULL;
    if (argcInit == 0)
    {
        // do not do magical windows commandline grabbing
        // instead 0/NULL means no parameters
        // this prevents accidental platform specific coding
        m_argc = 0;
        m_ppArgv = NULL;
    }
    else
    {
        // if here, we passed in the command line arguments when config
        //  manager was called(this is the recommended method of getting
        //  command line args!)
        m_argc = argcInit;
        m_ppArgv = argvInit;
    }
}

//------------------------------------------------------------------------------------------------
/*virtual*/ CommandLineSource::~CommandLineSource()
{
    m_pRoot = NULL;
#ifdef EE_EFD_CONFIG_DEBUG
    m_argc = -1;
    m_ppArgv = NULL;
#endif
}

//------------------------------------------------------------------------------------------------
/*virtual*/ bool CommandLineSource::ReadConfiguration(IConfigManager *pManager)
{
    ISection *pSection = m_pRoot;
    ISection *pDefaultSection = NULL;

    EE_LOG(efd::kConfiguration, efd::ILogger::kLVL1,
        ("Reading configuration from %s at priority %i",
        GetSourceName().c_str(),
        m_priority));

    METRICS_ONLY(double fCycleTimeStart = efd::GetCurrentTimeInSec());

    // Set the base path
    m_basePath = PathUtils::GetWorkingDirectory();

    // Process the exe name
    if (m_argc > 0)
    {
        // Put the application executable name into the default section under
        // the key ExeName.
        pDefaultSection = pSection = pSection->AddSection(kDefaultSection, this);
        if (!pSection)
        {
            EE_LOG(efd::kConfiguration, efd::ILogger::kERR2,
                ("Could not create the configuration section specified '%s'",
                 kDefaultSection));
        }

        efd::utf8string strName = kExeName;
        efd::utf8string strValue = m_ppArgv[ 0];

        if (pSection)
        {
            pSection->AddValue(m_pRoot, strName, strValue, m_priority, this);
        }
        else
        {
            EE_LOG(efd::kConfiguration, efd::ILogger::kERR2,
                ("Error: No Section = Could not add configuration name/value '%s'/'%s'",
                strName.c_str(),
                strValue.c_str()));
        }
    }

    for (int iArg = 1; iArg < m_argc; iArg++)
    {
        bool bRetVal = false;
        // Reset the section to add values in to
        pSection = m_pRoot;

        efd::utf8string strValue(m_ppArgv[ iArg]);
        if (ProcessNameValuePair(pManager, pSection, strValue) == false)
        {
            // If here, then the string is not a name value pair so we will
            // add it to the special section created fro the first argument.
            pSection = pDefaultSection;
            if (pSection)
            {
                efd::utf8string strName(efd::Formatted, "%s%i", kParam_name, iArg);

                bRetVal = pSection->AddValue(m_pRoot, strName, strValue, m_priority, this);
            }
        }
    }

    METRICS_ONLY(double fCycleTimeEnd = efd::GetCurrentTimeInSec());
    EE_LOG_METRIC(kConfiguration, "READ.CMD_LINE.SEC", float(fCycleTimeEnd - fCycleTimeStart));

    return true;
}


//------------------------------------------------------------------------------------------------
EE_IMPLEMENT_CONCRETE_CLASS_INFO(IniSource);

//------------------------------------------------------------------------------------------------
IniSource::IniSource(const efd::utf8string& strFileName)
: ConfigSource(IConfigSource::kSourceIniFile, strFileName)
{
    m_pRoot = NULL;
}

//------------------------------------------------------------------------------------------------
/*virtual*/ IniSource::~IniSource()
{
    m_pRoot = NULL;
}

//------------------------------------------------------------------------------------------------
/*virtual*/ bool IniSource::ReadConfiguration(IConfigManager *pManager)
{
    char pcLine[1024];
    ISection *pSection = m_pRoot;
    efd::utf8string sourceName = m_sourceName;

    METRICS_ONLY(double fCycleTimeStart = efd::GetCurrentTimeInSec());

    EE_LOG(efd::kConfiguration, efd::ILogger::kLVL1,
        ("Reading configuration from %s at priority %i",
        GetSourceName().c_str(),
        m_priority));

    // Search for a variable
    size_t posStart = sourceName.find(IConfigManager::kVarStart, 0);
    size_t VarStartSize = strlen(IConfigManager::kVarStart);

    // If a variable exists(loop to process all of them)
    while (posStart != efd::utf8string::npos)
    {
        // Get the value of the variable
        size_t idxStart = posStart + VarStartSize;
        size_t idxEnd = sourceName.find(IConfigManager::kVarEnd, idxStart);
        EE_ASSERT(idxEnd >= idxStart);
        efd::utf8string variable = sourceName.substr(idxStart, idxEnd - idxStart);

        efd::utf8string varValue = m_pRoot->FindValue(variable);

        // Construct the new string to insert the value
        efd::utf8string newValue = sourceName.substr(0, posStart);
        newValue += varValue;
        newValue += sourceName.substr(idxEnd + strlen(IConfigManager::kVarEnd));
        sourceName = newValue;

        // Search for another variable
        posStart = sourceName.find(IConfigManager::kVarStart, 0);
    }

    // We need to convert to an absolute file name on platforms like Xbox360 and PS3 because
    // they don't support relative names.  But in a bit we're gonna need the full path in order
    // to update m_basePath so we might as well just always use an absolute path:
    if (PathUtils::IsRelativePath(sourceName))
    {
        sourceName = PathUtils::ConvertToAbsolute(sourceName);
    }

    File* pIniFile = File::GetFile(sourceName.c_str(), File::READ_ONLY);

    // Open the file
    if (!pIniFile)
    {
        EE_LOG(efd::kConfiguration, efd::ILogger::kERR2,
            ("Could not open INI File '%s'",
             sourceName.c_str()));
        return false;
    }

    // Calculate the base path for the config file
    m_basePath = PathUtils::PathRemoveFileName(sourceName);

    // Process the lines of the file
    while (pIniFile->GetLine(pcLine, 1024))
    {
        efd::utf8string strArg = pcLine;

        // Search for a comment.  We ignore everything after the comment character.
        size_t iComment = strArg.find(efd::utf8char_t(IniSource::kCommentChar));
        if (iComment != efd::utf8string::npos)
        {
            strArg = strArg.substr(0, iComment);
        }

        // Now trim whitespace, this could result in a zero length string.
        strArg.trim();

        // Check for an empty line.
        if (strArg.length() == 0)
            continue;

        size_t iCloseSection = strArg.find(efd::utf8char_t(IniSource::kSectionRelativeCloseChar));
        if (iCloseSection != efd::utf8string::npos)
        {
            // Point to the parent section for storage because this section is closed now.
            pSection = pSection->GetParent();
            continue;
        }
        size_t iSectionRelative = strArg.find(efd::utf8char_t(
            IniSource::kSectionRelativeOpenChar));
        if (iSectionRelative != efd::utf8string::npos)
        {
            // Get the section name.
            efd::utf8string strSection = strArg.substr(iSectionRelative + 1);
            strSection.trim();
            // Add the section and point to it for storing name/value pairs.
            pSection = pSection->AddSection(strSection, this);
            continue;
        }
        size_t iSectionAbsolute = strArg.find(efd::utf8char_t(
            IniSource::kSectionAbsoluteOpenChar));
        if (iSectionAbsolute != efd::utf8string::npos)
        {
            // Get the section name.
            efd::utf8string strSection = strArg.substr(iSectionAbsolute + 1);
            strSection.trim(efd::TrimFront);
            iSectionAbsolute = strSection.rfind(efd::utf8char_t(
                IniSource::kSectionAbsoluteCloseChar));
            strSection = strSection.substr(0, iSectionAbsolute);
            strSection.trim(efd::TrimBack);
            size_t iLink = strSection.find(efd::utf8char_t(IConfigManager::kNameLinkSeperator));
            if (iLink == efd::utf8string::npos)
            {
                // Add the section and point to it for storing name/value pairs.
                pSection = m_pRoot->AddSection(strSection, this);
            }
            else
            {
                efd::utf8string strSectionLink = strSection.substr(iLink + 1);
                strSectionLink.trim();
                strSection = strSection.substr(0, iLink);
                strSection.trim();
                ISection *pILink = m_pRoot->FindSection(strSectionLink);
                // If there is no section to link to, create one!
                if (!pILink)
                {
                    pILink = m_pRoot->AddSection(strSectionLink, this);
                }
                // Add the section link, but do NOT point to it for storing name/value pairs
                m_pRoot->AddSectionLink(strSection, pILink, this);
            }
            continue;
        }

        ProcessNameValuePair(pManager, pSection, strArg);
    }

    EE_DELETE pIniFile;

    METRICS_ONLY(double fCycleTimeEnd = efd::GetCurrentTimeInSec());
    EE_LOG_METRIC(kConfiguration, "READ.INI.SEC", float(fCycleTimeEnd - fCycleTimeStart));

    return true;
}


//------------------------------------------------------------------------------------------------
EE_IMPLEMENT_CONCRETE_CLASS_INFO(EnvVarSource);

const /*static*/ char *EnvVarSource::kDefaultSection = "Environment Variables";

//------------------------------------------------------------------------------------------------
EnvVarSource::EnvVarSource()
: ConfigSource(IConfigSource::kSourceEnvVar, "EnvVar")
{
    m_pRoot = NULL;
}

//------------------------------------------------------------------------------------------------
/*virtual*/ EnvVarSource::~EnvVarSource()
{
    m_pRoot = NULL;
}

//------------------------------------------------------------------------------------------------
/*virtual*/ bool EnvVarSource::ReadConfiguration(IConfigManager *pManager)
{
    METRICS_ONLY(double fCycleTimeStart = efd::GetCurrentTimeInSec());

    EE_LOG(efd::kConfiguration, efd::ILogger::kLVL1,
        ("Reading configuration from %s at priority %i",
        GetSourceName().c_str(),
        m_priority));

    // Set the base path
    m_basePath = PathUtils::GetWorkingDirectory();

    // Add the default section and point to it for storing name/value pairs
    ISection *pSection = m_pRoot->AddSection(kDefaultSection, this);
    if (!pSection)
    {
        EE_LOG(efd::kConfiguration, efd::ILogger::kERR2,
            ("Error: Could not add default section for EnvVar '%s'",
            kDefaultSection));

        return false;
    }

#if defined(EE_PLATFORM_WIN32) || defined(EE_PLATFORM_LINUX)
    efd::utf8string strName;
    efd::utf8string strValue;
    char **pEnvVars = environ;
    int iEnvVar = 0;
    while (pEnvVars[iEnvVar] != NULL)
    {
        bool bRetVal = false;

        strName = pEnvVars[iEnvVar];

        size_t iEqual = strName.find(efd::utf8char_t('='));
        strValue = strName.substr(iEqual + 1);
        strName = strName.substr(0, iEqual);

        strName.trim();
        strValue.trim();

        bRetVal = pSection->AddValue(m_pRoot, strName, strValue, m_priority, this);
        if (!bRetVal)
        {
            EE_LOG(efd::kConfiguration, efd::ILogger::kERR2,
                ("Error: Could not add configuration name/value '%s'/'%s' to section %s",
                 strName.c_str(),
                 strValue.c_str(),
                 (pSection != NULL) ? pSection->GetName().c_str() : "UNKNOWN"));
        }

        iEnvVar++;
    }
#endif // defined(EE_PLATFORM_WIN32) || defined(EE_PLATFORM_LINUX)

    METRICS_ONLY(double fCycleTimeEnd = efd::GetCurrentTimeInSec());
    EE_LOG_METRIC(kConfiguration, "READ.ENV_VAR.SEC", float(fCycleTimeEnd - fCycleTimeStart));

    EE_UNUSED_ARG(pManager);

    return true;
}

//------------------------------------------------------------------------------------------------
