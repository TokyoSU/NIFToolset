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

/// The logger system uses macros to gather file/line and app information and push it into
/// various log destinations. The macros rely on LoggerSingleton.
/// The default logger can be replaced by custom loggers (SystemLogger, RemoteLogger).

#include "efdPCH.h"

#include <stdio.h>
#include <efd/Logger.h>
#include <efd/efdLogIDs.h>
#include <stdarg.h>
#include <efd/Asserts.h>
#include <efd/FileDestination.h>
#include <efd/PrintDestination.h>
#include <efd/DebugOutDestination.h>
#include <efd/NetMetricsDestination.h>
#include <efd/MemoryDefines.h>
#include <efd/IConfigManager.h>
#include <efd/AssertDialog.h>
#include <efd/Utilities.h>
#include <efd/MemManager.h>
#include <efd/Category.h>
#include <efd/EnumManager.h>
#include <efd/ServiceManager.h>
#include <efd/efdClassIDs.h>
#include <efd/IBase.h>

#if defined(EE_PLATFORM_LINUX)
#include <sys/time.h>
#elif defined(EE_PLATFORM_PS3)
#include <sys/time.h>
#include <sys/sys_time.h>
#else
#include <time.h>
#include <sys/timeb.h>
#endif

//------------------------------------------------------------------------------------------------
#if !defined(EE_DISABLE_LOGGING)

#define EE_ASSERT_BADDESTINATION_IMPL(message) EE_FAIL(message)

#define EE_ASSERT_BADDESTINATION(destination)                                           \
    {                                                                                   \
        efd::utf8string errMessage = utf8string(Formatted,                              \
            "Logger Failure: Unknown File Destination (%s) specified in '%s'.\n"        \
            "If you are hitting this error then you need to make sure you have opened " \
            "a log destination and added it to the Logger using AddDest before "        \
            "using the destination name in logger functions",                           \
            destination,                                                                \
            __FUNCTION__);                                                              \
        if (efd::AssertHelper::GetAssertHandler())                                      \
        {                                                                               \
            EE_ASSERT_BADDESTINATION_IMPL(errMessage.c_str());                          \
        }                                                                               \
        else                                                                            \
        {                                                                               \
            fprintf(stderr, "%s\n", errMessage.c_str());                                \
        }                                                                               \
    }
#else // !defined(EE_DISABLE_LOGGING)
#define EE_ASSERT_BADDESTINATION(destination) (void)false
#endif

//------------------------------------------------------------------------------------------------
namespace efd
{
    EE_EFD_ENTRY ILogger* GetLogger()
    {
        return LoggerSingleton::Instance()->GetLogger();
    }
} // end namespace efd

using namespace efd;


//------------------------------------------------------------------------------------------------
EE_IMPLEMENT_CONCRETE_CLASS_INFO(Logger);


//------------------------------------------------------------------------------------------------
const /*static*/ char* Logger::kConfigSection = "Log";
const /*static*/ char* Logger::kConfigFilesSection = "FileDests";
const /*static*/ char* Logger::kConfigDestFileNameKey = "FileName";
const /*static*/ char* Logger::kConfigDestFileModeKey = "FileMode";
const /*static*/ char* Logger::kConfigDestFileMsgKey = "FileInfoWithMsg";
const /*static*/ char* Logger::kConfigDestFileAssertKey = "FileInfoWithAssert";
const /*static*/ char* kConfigDestination = "Destinations";
const /*static*/ char* kConfigDestUseDefaultLevels = "UseDefaults";
const /*static*/ char* kConfigDestType = "Type";
const /*static*/ char* kConfigDestDecorate = "Decorate";
const /*static*/ char* Logger::kConfigLevelNamesSection = "Levels";
const /*static*/ char* Logger::kConfigLogLevelsSection = "Filters";


//------------------------------------------------------------------------------------------------
LoggerSingleton* LoggerSingleton::ms_instance = NULL;


//------------------------------------------------------------------------------------------------
ILogDestination* FileDestFactory(
    const efd::utf8string& destSectionName,
    const efd::ISection* pDestSection);
ILogDestination* PrintDestFactory(
    const efd::utf8string& destSectionName,
    const efd::ISection* pDestSection);
ILogDestination* DebugOutDestFactory(
    const efd::utf8string& destSectionName,
    const efd::ISection* pDestSection);
ILogDestination* NetMetricsFactory(
    const efd::utf8string& destSectionName,
    const efd::ISection* pDestSection);


//------------------------------------------------------------------------------------------------
Logger::Logger()
: m_activeDestinations(0)
, m_pServiceManager(NULL)
, m_clockID(0)
, m_flushLevel(kLogMask_UptoErr3)
, m_alreadyConfigured(false)
{
    RegisterLevelMaskName(kLogMask_Err0, "Err0");
    RegisterLevelMaskName(kLogMask_Err1, "Err1");
    RegisterLevelMaskName(kLogMask_Err2, "Err2");
    RegisterLevelMaskName(kLogMask_Err3, "Err3");

    RegisterLevelMaskName(kLogMask_Lvl0, "Lvl0");
    RegisterLevelMaskName(kLogMask_Lvl1, "Lvl1");
    RegisterLevelMaskName(kLogMask_Lvl2, "Lvl2");
    RegisterLevelMaskName(kLogMask_Lvl3, "Lvl3");

    RegisterLevelMaskName(kLogMask_UptoLvl1, "UptoLvl1");
    RegisterLevelMaskName(kLogMask_UptoLvl2, "UptoLvl2");
    RegisterLevelMaskName(kLogMask_UptoLvl3, "UptoLvl3");

    RegisterLevelMaskName(kLogMask_UptoErr1, "UptoErr1");
    RegisterLevelMaskName(kLogMask_UptoErr2, "UptoErr2");
    RegisterLevelMaskName(kLogMask_UptoErr3, "UptoErr3");

    RegisterLevelMaskName(kLogMask_All, "All");
    RegisterLevelMaskName(kLogMask_None, "None");

    // Old style values are still supported, but the names above are preferred.
    RegisterLevelMaskName(kLogMask_UptoLvl1, "UPTO_LVL1");
    RegisterLevelMaskName(kLogMask_UptoLvl2, "UPTO_LVL2");
    RegisterLevelMaskName(kLogMask_UptoLvl3, "UPTO_LVL3");

    RegisterLevelMaskName(kLogMask_UptoErr1, "UPTO_ERR1");
    RegisterLevelMaskName(kLogMask_UptoErr2, "UPTO_ERR2");
    RegisterLevelMaskName(kLogMask_UptoErr3, "UPTO_ERR3");

    // Register default output handler factories
    RegisterDestinationFactory("File", FileDestFactory);
    RegisterDestinationFactory("Printf", PrintDestFactory);
    RegisterDestinationFactory("DebugOut", DebugOutDestFactory);
    RegisterDestinationFactory("NetMetrics", NetMetricsFactory);

#ifdef EE_USE_DEBUGOUT_BY_DEFAULT
    // Add a debug out destination for errors.
    DebugOutDestination* debugOut = EE_NEW DebugOutDestination(
        "DebugOut0",
        true,
        true,
        true);

    AddDest(debugOut, false, false);

    SetLogLevel(
        efd::kALL,
        efd::ILogger::kLogMask_UptoErr2,
        "DebugOut0");
#endif
}

//------------------------------------------------------------------------------------------------
Logger::~Logger()
{
    // Remove any log destinations that may exist
    RemoveAllDests();
}

//------------------------------------------------------------------------------------------------
/*virtual*/
void Logger::Log(
    efd::UInt16 module,
    efd::UInt8 level,
    const char* file,
    efd::SInt32 line,
    const char* msg)
{
    // Grab the machines date and time
    char timestamp[36];
    GetMachineTimeStamp(timestamp, 36);

    // Get the game timestamp. This requires the application to have passed in a ServiceManager
    // pointer via SetServiceManager. This is game time, so (for example), it doesn't advance while
    // the Scheduler is paused.
    TimeType gameTimeStamp = 0.0f;
    if (NULL != m_pServiceManager)
    {
        gameTimeStamp = m_pServiceManager->GetTime(m_clockID);
    }

    UInt32 destinationMask = GetModuleSettings(module).GetDestinationsMask(level);
    destinationMask = destinationMask & m_activeDestinations;

    int i=0;
    while (destinationMask)
    {
        if (m_destinations[i])
        {
            if (destinationMask & 1)
            {
                m_destinations[i]->LogMessage(
                    false,
                    timestamp,
                    gameTimeStamp,
                    GetModuleName(module).c_str(),
                    GetLevelName(level),
                    file,
                    line,
                    msg);
            }
        }
        ++i;
        destinationMask = destinationMask>>1;
    }

    // Go ahead and flush the logs if this is an important level to us.
    if (level & m_flushLevel)
        FlushFileLogDestinations();
}


//------------------------------------------------------------------------------------------------
efd::Bool Logger::AssertMessage(
    const char* file,
    efd::SInt32 line,
    const char* pFunc,
    const char* pred,
    const char* msg,
    const char* pStack)
{
    // The assert macro has already checked that the statement should be
    // logged so there is no need to check again.

    // Grab the machines date and time
    char timestamp[36];
    GetMachineTimeStamp(timestamp, 36);

    // All asserts go to the default (first) destination, so grab it
    ILogDestination* pLog = m_destinations[0];

    // Make sure there is a destination
    if (pLog)
    {
        if (NULL == msg)
        {
            msg = pred;
        }

        pLog->BeginLog(
            true,
            timestamp,
            0.0,
            "Assert",
            GetLevelName(kERR0),
            file,
            line);

        if (pFunc)
        {
            pLog->ContinueLog("\nFunction:");
            pLog->ContinueLog(pFunc);
            pLog->ContinueLog("\n");
        }

        if (pred != msg)
        {
            pLog->ContinueLog("Predicate: ");
            pLog->ContinueLog(pred);
            pLog->ContinueLog("\nMessage: ");
        }

        pLog->ContinueLog(msg);

        if (pStack && m_bUseStackTrace)
        {
            pLog->ContinueLog("\nStack Trace:\n");
            pLog->ContinueLog(pStack);
        }

        pLog->EndLog();
    }
    else
    {
        // Logger Error: No File Destination configured.
        fprintf(stderr, "\nLogger Assert: %s\n", msg);
        if (pStack && m_bUseStackTrace)
        {
            fprintf(stderr, "Stack Trace:\n%s\n", pStack);
        }
    }

    // make sure our buffers are flushed so we don't lose any messages.
    Flush();

    return false; // continue hitting this EE_ASSERT
}

//------------------------------------------------------------------------------------------------
UInt16 Logger::RegisterModuleName(const char* name)
{
    static UInt16 nextModuleId = efd::kNextId;

    // sanity checks ... is there really a name?
    if (!name)
    {
        return kUnknownModule;
    }

    // If the enum already contains this name, use the existing value
    UInt16 value = GetModuleInt(name);
    if (kUnknownModule != value)
    {
        return value;
    }

    // there should be no existing entry at our unique id ...
    // this should never actually loop unless our logic is loopy ...
    while (m_moduleNames.count(nextModuleId) != 0)
    {
        nextModuleId++;
    }

    m_moduleNames[nextModuleId] = name;
    return nextModuleId++;
}

//------------------------------------------------------------------------------------------------
bool Logger::RegisterModuleName(UInt16 module, const char* name)
{
    // if the enumeration already contains this value, we cannot add it
    if (m_spModuleEnum)
    {
        if (m_spModuleEnum->HasValue(module) || m_spModuleEnum->HasName(name))
        {
            return false;
        }
    }

    m_moduleNames[module] = name;
    return true;
}

//------------------------------------------------------------------------------------------------
void Logger::RegisterLevelMaskName(UInt8 level, const char* name)
{
    m_levelNames[name] = level;
}

//------------------------------------------------------------------------------------------------
efd::Bool Logger::SetLogLevel(
    UInt16 module,
    UInt8 levelMask,
    const efd::utf8string& name)
{
    // Start by assuming this is the 'all' destination, which means setting all bits
    efd::UInt32 destinationMask = 0xFFFFFFFF;
    if (!name.empty())
    {
        efd::UInt32 destIndex = GetDestinationIndexByName(name);
        if ((efd::UInt32)-1 == destIndex)
        {
            // If we get here, then this is an unknown destination and we should print an error
            // message.
            EE_ASSERT_BADDESTINATION(name.c_str());
            return false;
        }

        destinationMask = 1 << destIndex;
    }

    if (kALL == module)
    {
        efd::UInt16 debugOutIndex = (efd::UInt16)-1;
        if (0xFFFFFFFF == destinationMask)
        {
            debugOutIndex = (efd::UInt16)GetDestinationIndexByName("DebugOut0");

            // We can apply a setting to all modules for all destinations simply by erasing the
            // m_modules table and then setting the level in the default ModuleSetting.
            m_modules.clear();
        }
        else
        {
            // We need to iterate all customized modules to apply this setting just for the
            // specified destination.
            for (ModuleSettingsMap::iterator iter = m_modules.begin();
                iter != m_modules.end();
                ++iter)
            {
                iter->second.SetLogMask(levelMask, destinationMask);
            }
        }

        // Any change of all modules effects the defaults since any module not found in m_modules
        // will automatically fall back to using the default settings.
        m_defaults.SetLogMask(levelMask, destinationMask);

        // Don't clear default settings for DebugOut0 destination.
        if ((efd::UInt16)-1 != debugOutIndex)
        {
            destinationMask = 1 << debugOutIndex;
            m_defaults.SetLogMask(efd::ILogger::kLogMask_UptoErr3, destinationMask);
        }
    }
    else
    {
        // We need to either find or add a ModuleSettings to the m_modules map since we're about
        // to change this module from the default settings.
        ModuleSettingsMap::iterator iter = m_modules.find(module);
        if (iter == m_modules.end())
        {
            // Not found, insert a copy of the defaults.  We do this in order to avoid changing
            // whatever settings were previously effecting all the other log destinations.
            iter = m_modules.insert(ModuleSettingsMap::value_type(module, m_defaults)).first;
        }
        EE_ASSERT(iter != m_modules.end());

        // Now we set or clear the bits in the destinationMask based on the bits in the levelMask.
        // Bits in the ModuleSettings which are not in the destinationMask are uneffected.
        iter->second.SetLogMask(levelMask, destinationMask);
    }

    return true;
}

//------------------------------------------------------------------------------------------------
efd::UInt16 Logger::GetModuleInt(const efd::utf8string& moduleName)
{
    // First lookup the name in the enumeration
    efd::UInt16 value;
    if (m_spModuleEnum && m_spModuleEnum->FindEnumValue(moduleName, value))
    {
        return value;
    }

    //If not found, find the module name in the map of dynamically added modules
    ModuleNameMap::iterator mni;
    for (mni = m_moduleNames.begin(); mni != m_moduleNames.end(); mni++)
    {
        if (moduleName.icompare((*mni).second) == 0)
            return (*mni).first;
    }

    // Return kUnknown if the ID is not found
    return kUnknownModule;
}

//------------------------------------------------------------------------------------------------
void Logger::GetModules(efd::map<efd::UInt16, efd::utf8string >& names) const
{
    // DT32409 Most modules come from enum files and don't get included here. We should mention
    // what enum file is in effect, or iterate the enum and add those too.
    // Add any dynamically registered modules:
    ModuleNameMap::const_iterator iter = m_moduleNames.begin();
    for (; iter != m_moduleNames.end(); ++iter)
    {
        names[iter->first] = iter->second;
    }
}

//------------------------------------------------------------------------------------------------
efd::utf8string Logger::GetModuleName(UInt16 module)
{
    // First lookup the value in the enumeration
    efd::utf8string name;
    if (m_spModuleEnum && m_spModuleEnum->FindEnumName(module, name))
    {
        return name;
    }

    // Next look in the dynamically added module names
    ModuleNameMap::iterator iter = m_moduleNames.find(module);
    if (iter != m_moduleNames.end())
    {
        return (*iter).second;
    }

    // final fallback is to use the integer value
    name.sprintf("%d", module);
    return name;
}

//------------------------------------------------------------------------------------------------
const char* Logger::GetLevelName(UInt8 level)
{
    static const char* m_rgLevels[] =
    {
        "Err0",
        "Err1",
        "Err2",
        "Err3",
        "Lvl0",
        "Lvl1",
        "Lvl2",
        "Lvl3"
    };

    if (level < EE_ARRAYSIZEOF(m_rgLevels))
    {
        return m_rgLevels[level];
    }

    return "Unknown";
}

//------------------------------------------------------------------------------------------------
UInt16 Logger::StringToModuleID(const efd::utf8string& module)
{
    UInt16 moduleID = 0;
    if (!efd::ParseHelper<UInt16>::FromString(module, moduleID))
    {
        // The string did not parse as a number, so it must be a name:
        moduleID = GetModuleInt(module);
    }

    return moduleID;
}

//------------------------------------------------------------------------------------------------
UInt8 Logger::GetLevelMaskByName(const efd::utf8string& levelName)
{
    // Split the string on the | symbols
    efd::vector<efd::utf8string> tokens;
    efd::UInt32 numTokens = levelName.split("|", tokens);

    UInt8 level = 0;
    // Loop through each of the tokens and find the value
    for (efd::UInt32 token = 0; token < numTokens; token++)
    {
        utf8string& thisToken = tokens[token];
        thisToken.trim();

        UInt8 thisLevel = 0;
        if (!efd::ParseHelper<UInt8>::FromString(thisToken, thisLevel))
        {
            LevelNameMap::iterator iter = m_levelNames.find(thisToken);
            if (iter != m_levelNames.end())
            {
                thisLevel = iter->second;
            }
            else
            {
                // When you encounter this failure, it indicates a typo
                // in the filter description in the config file. We die
                // here instead of failing silently because there is
                // nothing more frustrating than running an elaborate test
                // only to discover you didn't log the data you needed
                // because of said typo. ("UpTo" vs. "Upto", anyone?)
                efd::PrintfHelper msg("Invalid log filter value '%s'",
                    thisToken.c_str());
                EE_FAIL(msg);
            }
        }
        level |= thisLevel;
    }

    return level;
}

//------------------------------------------------------------------------------------------------
efd::Bool Logger::IsLogging(efd::UInt16 module, efd::UInt8 level) const
{
    const ModuleSettings& mod = GetModuleSettings(module);
    return 0 != mod.GetDestinationsMask(level);
}

//------------------------------------------------------------------------------------------------
efd::Bool Logger::AddDest(ILogDestination* pDest, bool useDefaults, bool overrideExisting)
{
    // Initialize the destination and return false if it fails
    if (!pDest || !pDest->OnInit())
    {
        return false;
    }

    SInt32 firstFreeSlot = -1;
    for (SInt32 i=0; i<kMaxLogDestinations; ++i)
    {
        if (m_destinations[i])
        {
            // If this dest name already exists, return false
            if (m_destinations[i]->GetName() == pDest->GetName())
            {
                if (overrideExisting)
                {
                    firstFreeSlot = i;
                    break;
                }
                return false;
            }
        }
        else if (-1 == firstFreeSlot)
        {
            firstFreeSlot = i;
        }
    }

    if (-1 == firstFreeSlot)
    {
        // maximum log destinations exceeded!
        return false;
    }

    // If we made it here, then this is a destination with a unique name
    m_destinations[firstFreeSlot] = pDest;
    m_activeDestinations = BitUtils::SetBitByIndex(m_activeDestinations, firstFreeSlot);

    if (useDefaults)
    {
        SetDefaultLogLevels(pDest->GetName());
    }
    else
    {
        SetLogLevel(kALL, kLogMask_None, pDest->GetName());
    }
    return true;
}

//------------------------------------------------------------------------------------------------
void Logger::RemoveDest(const efd::utf8string& name)
{
    bool resetDefaultDestination = false;

    SInt32 firstNonNullUsedSlot = -1;
    for (SInt32 i=0; i<kMaxLogDestinations; ++i)
    {
        if (m_destinations[i])
        {
            if (m_destinations[i]->GetName() == name)
            {
                resetDefaultDestination = (i == 0);
                m_destinations[i] = NULL;
                m_activeDestinations = BitUtils::ClearBitByIndex(m_activeDestinations, i);
                break;
            }
            else if (-1 == firstNonNullUsedSlot)
            {
                firstNonNullUsedSlot = i;
            }
        }
    }

    if (resetDefaultDestination && -1 != firstNonNullUsedSlot)
    {
        // Swap destination "firstNonNullUsedSlot" into slot "0".  This step requires a bit shift
        // for every mask in every ModuleSetting in the m_modules map.
        m_destinations[0] = m_destinations[firstNonNullUsedSlot];
        m_destinations[firstNonNullUsedSlot] = NULL;

        m_activeDestinations = BitUtils::ClearBitByIndex(m_activeDestinations,
            firstNonNullUsedSlot);
        m_activeDestinations = BitUtils::SetBitByIndex(m_activeDestinations, 0);

        // Move the bits from the old to the new location for all the modules, including the
        // default.
        for (ModuleSettingsMap::iterator iter = m_modules.begin();
            iter != m_modules.end();
            ++iter)
        {
            iter->second.MoveDestination(firstNonNullUsedSlot, 0);
        }
        m_defaults.MoveDestination(firstNonNullUsedSlot, 0);
    }
}

//------------------------------------------------------------------------------------------------
void Logger::RemoveDestWithoutChangingDefault(const efd::utf8string& name)
{
    for (SInt32 i=0; i<kMaxLogDestinations; ++i)
    {
        if (m_destinations[i])
        {
            if (m_destinations[i]->GetName() == name)
            {
                m_destinations[i] = NULL;
                m_activeDestinations = BitUtils::ClearBitByIndex(m_activeDestinations, i);
                return;
            }
        }
    }
}

//------------------------------------------------------------------------------------------------
void Logger::RemoveDest(ILogDestination* pDest)
{
    RemoveDest(pDest->GetName());
}

//------------------------------------------------------------------------------------------------
void Logger::RemoveAllDests()
{
    for (SInt32 i=0; i<kMaxLogDestinations; ++i)
    {
        m_destinations[i] = NULL;
    }
    m_activeDestinations = 0;
}

//------------------------------------------------------------------------------------------------
void Logger::GetDestinationNames(efd::set<efd::utf8string>& names)
{
    for (SInt32 i=0; i<kMaxLogDestinations; ++i)
    {
        if (m_destinations[i])
        {
            names.insert(m_destinations[i]->GetName());
        }
    }
}

//------------------------------------------------------------------------------------------------
bool Logger::IsDestination(const efd::utf8string& name) const
{
    for (SInt32 i=0; i<kMaxLogDestinations; ++i)
    {
        if (m_destinations[i] && m_destinations[i]->GetName() == name)
        {
            return true;
        }
    }
    return false;
}

//------------------------------------------------------------------------------------------------
efd::Bool Logger::ReadConfig(IConfigManager* pConfigManager, EnumManager* pEnumManager, bool force)
{
    if (m_alreadyConfigured && !force)
    {
        return true;
    }

    // Check for a valid pointer
    if (!pConfigManager)
    {
        EE_LOG(
            kLog,
            Logger::kERR1,
            ("NULL ConfigManager pointer passed to read config"));
        return false;
    }

    // Check for a main config section
    if (!pConfigManager->GetConfiguration())
    {
        EE_LOG(
            kLog,
            Logger::kERR1,
            ("ConfigManager has no configuration pointer"));
        return false;
    }

    const ISection *pLogSection
        = pConfigManager->GetConfiguration()->FindSection(Logger::kConfigSection);

    // First, if we were provided an EnumManger we attempt to load an enumeration which defines
    // the log module names.  If provided, this enum allows us to use friendly string names to
    // configure logging instead of just integer values.
    if (pEnumManager)
    {
        // We default to loading the list of foundation modules
        utf8string enumName("efdLogIDs");
        if (pLogSection)
        {
            // But this can be overridden via config by setting "Log.ModuleEnum" to the name of a
            // more derived enum.
            pLogSection->FindValue("ModuleEnum", enumName);
        }
        SetModuleEnum(pEnumManager->FindOrLoadEnum(enumName));
    }

    // Look for the logger configuration section
    if (!pLogSection)
    {
        // It is not necessarily an error to not find a logger config section
        // so simply log that it is missing as a high level and move on
        EE_LOG(
            kLog,
            ILogger::kLVL0,
            ("Did not find a log section in the configuration manager"));
        return false;
    }

    // Handling for generic destinations, which can be file destinations as above or can be
    // other destinations that have factories registered.
    const ISection *pDestinations = pLogSection->FindSection(kConfigDestination);
    if (pDestinations)
    {
        // Loop through the child sections and see if they contain valid dests
        for (efd::SectionIter iter = pDestinations->GetBeginChildSectionIterator();
            iter != pDestinations->GetEndChildSectionIterator();
            iter++)
        {
            const utf8string& sectionName = iter->first;
            const ISection* pSection = iter->second;

            // Determine if we should add the destination with default log values in effect
            bool useDefaults = pSection->IsTrue(kConfigDestUseDefaultLevels);

            // Determine the type of this output handler:
            utf8string strType;
            if (pSection->FindValue(kConfigDestType, strType))
            {
                // We are about to re-create this destination, remove any previous destination with
                // the same name just in case we are replacing an already defined destination.  For
                // example, you might create a printf destination in you main C++ code but then
                // replace it with a file destination in your config file. If the destination we
                // remove happens to be the default then we leave slot zero empty so the newly
                // created destination will become the new default destination.
                RemoveDestWithoutChangingDefault(sectionName);

                ILogDestinationPtr pDest = FactoryLogDestination(strType, sectionName, pSection);
                if (pDest)
                {
                    if (!AddDest(pDest, useDefaults, true))
                    {
                        efd::utf8string msg(efd::Formatted,
                            "Failed to initialize log destination '%s' of type '%s'", 
                            sectionName.c_str(), strType.c_str());

                        EE_LOG(efd::kLog, ILogger::kERR2, ("%s", msg.c_str()));
                        EE_OUTPUT_DEBUG_STRING(msg.c_str());
                    }
                }
                else
                {
                    efd::utf8string msg(efd::Formatted,
                        "Failed to create log destination '%s' of type '%s'", 
                            sectionName.c_str(), strType.c_str());

                    EE_LOG(efd::kLog, ILogger::kERR2, ("%s", msg.c_str()));
                    EE_OUTPUT_DEBUG_STRING(msg.c_str());
                }
            }
        }
    }

    // This function reads new log level mask values which allows you to extend the default
    // values such as 'Err1' and 'UptoLvl2'
    ReadLogLevelNames(pLogSection);

    const ISection *pLogLevelsSection = pLogSection->FindSection(Logger::kConfigLogLevelsSection);

    // Look for the log level configuration section
    if (!pLogLevelsSection)
    {
        // It is not necessarily an error to not find a logger config section
        // so simply log that it is missing as a high level and move on
        EE_LOG(
            kLog,
            ILogger::kLVL0,
            ("Did not find log level section in the configuration manager"));
    }
    else
    {
        // Check for default filters (no destination specified)
        ReadFilterConfig("", pLogLevelsSection);

        // Loop through the child sections and see if they contain valid filters
        for (efd::SectionIter iter = pLogLevelsSection->GetBeginChildSectionIterator();
            iter != pLogLevelsSection->GetEndChildSectionIterator();
            iter++)
        {
            ReadFilterConfig(iter->first, iter->second);
        }
    }

    m_alreadyConfigured = true;
    return true;
}

//------------------------------------------------------------------------------------------------
void Logger::ReadLogLevelNames(const efd::ISection* pLogSection)
{
    const ISection *pLogLevelNames = pLogSection->FindSection(Logger::kConfigLevelNamesSection);

    // Look for the log level configuration section
    if (pLogLevelNames)
    {
        // Find the log levels
        for (efd::ValueIter iter = pLogLevelNames->GetBeginValueIterator();
            iter != pLogLevelNames->GetEndValueIterator();
            iter++)
        {
            // Get the module's value section
            ISectionEntry* pValSection = iter->second;
            if (pValSection)
            {
                // Get the module's value
                UInt8 level = GetLevelMaskByName(pValSection->GetValue());

                // Set the logging level
                RegisterLevelMaskName(level, iter->first.c_str());
            }
            else
            {
                EE_FAIL_MESSAGE(("%s configuration value (section) is null",
                    iter->first.c_str()));
            }
        }
    }
}

//------------------------------------------------------------------------------------------------
void Logger::ResetLogLevels()
{
    // Simply clear the modules map, then all SetLogLevels are wiped out
    m_modules.clear();
    // Also clear out the defaults too.
    m_defaults.Clear();
}

//------------------------------------------------------------------------------------------------
void Logger::ResetLogLevels(const efd::utf8string& dest)
{
    SInt32 indexToClear = GetDestinationIndexByName(dest);
    if (-1 != indexToClear)
    {
        UInt32 destinationMask = 1<<indexToClear;

        for (ModuleSettingsMap::iterator iter = m_modules.begin();
            iter != m_modules.end();
            ++iter)
        {
            iter->second.Clear(destinationMask);
        }
        m_defaults.Clear(destinationMask);
    }
}

//------------------------------------------------------------------------------------------------
void Logger::GetMachineTimeStamp(char *timeStamp, efd::UInt32 bufferSize)
{
#if defined(EE_PLATFORM_PS3)
    // Use a relative timestamp from initialization
    sys_time_sec_t sec = 0;
    sys_time_nsec_t nsec = 0;
    sys_time_get_current_time(&sec, &nsec);
    static efd::UInt32 s_start_sec = 0;
    if (s_start_sec == 0)
    {
        s_start_sec = (efd::UInt32)sec;
    }
    efd::Snprintf(
        timeStamp,
        bufferSize,
        EE_TRUNCATE,
        "%ld.%04ld",
        (efd::UInt32)sec-s_start_sec, (efd::UInt32)(nsec/100000));
    // Dividing nanoseconds by 100000 - tenths of a millisecond is plenty accurate for
    // our purposes.

#elif defined(EE_PLATFORM_LINUX)
    struct timeval tv;
    struct tm lt;

    gettimeofday(&tv,NULL); // It'd be nice if it was in local time
    localtime_r(&tv.tv_sec,&lt);

    // NB: tm_year is number of years since 1900, tm_mon is zero based
    efd::Snprintf(
        timeStamp,
        bufferSize,
        EE_TRUNCATE,
        "%4d/%02d/%02d %02d:%02d:%02d.%03d",
        lt.tm_year+1900,lt.tm_mon+1,lt.tm_mday,
        lt.tm_hour,lt.tm_min,lt.tm_sec,
        tv.tv_usec/1000);
#else
    struct tm *newtime;
    struct __timeb64 timebuffer;

    _ftime64(&timebuffer);

    newtime = _localtime64(&timebuffer.time); /* Convert to local time. */

    // NB: tm_year is year minus 1900, and tm_mon is zero-based
    efd::Snprintf(
        timeStamp,
        bufferSize,
        EE_TRUNCATE,
        "%4d/%02d/%02d %02d:%02d:%02d.%03d",
        newtime->tm_year+1900,newtime->tm_mon+1,newtime->tm_mday,
        newtime->tm_hour,newtime->tm_min,newtime->tm_sec,
        timebuffer.millitm);

#endif
}

//------------------------------------------------------------------------------------------------
efd::Bool Logger::ReadDestConfig(
    const efd::utf8string& destName,
    const efd::ISection* pDestSection)
{
    ILogDestination* pDest = FileDestFactory(destName, pDestSection);
    if (pDest)
    {
        return AddDest(pDest);
    }
    return false;
}

//------------------------------------------------------------------------------------------------
ILogDestination* FileDestFactory(
    const efd::utf8string& destSectionName,
    const efd::ISection* pDestSection)
{
    // Look for the file name
    efd::utf8string fileName;
    if (pDestSection->FindValue(Logger::kConfigDestFileNameKey, fileName))
    {
        // Find the file mode (assume the default of overwrite)
        FileDestination::FileOption fileMode = FileDestination::kFileOverwrite;
        efd::utf8string value;
        if (pDestSection->FindValue(Logger::kConfigDestFileModeKey, value))
        {
            // Check the file mode specified
            if (value == "FileAppend")
                fileMode = FileDestination::kFileAppend;
            else if (value == "FileOverwrite")
                fileMode = FileDestination::kFileOverwrite;
            else if (value == "UniqueFileName")
                fileMode = FileDestination::kUniqueFileName;
            else if (value == "IndexedFileName")
                fileMode = FileDestination::kIndexedFileName;
        }
        efd::Bool fileWithMsg = pDestSection->IsTrue(Logger::kConfigDestFileMsgKey);
        efd::Bool fileWithAssert = pDestSection->IsFalse(Logger::kConfigDestFileAssertKey);
        efd::Bool flushOnWrite = pDestSection->IsTrue("FlushOnWrite");

        // Create the file destination
        return EE_NEW FileDestination(
            destSectionName,
            fileName,
            fileMode,
            fileWithMsg,
            fileWithAssert,
            flushOnWrite);
    }
    return NULL;
}

//------------------------------------------------------------------------------------------------
ILogDestination* PrintDestFactory(
    const efd::utf8string& destSectionName,
    const efd::ISection* pDestSection)
{
    efd::Bool decorateMessage = pDestSection->IsTrue(kConfigDestDecorate);
    efd::Bool fileWithMsg = pDestSection->IsTrue(Logger::kConfigDestFileMsgKey);
    efd::Bool fileWithAssert = pDestSection->IsFalse(Logger::kConfigDestFileAssertKey);

    // Create the destination
    return EE_NEW PrintDestination(
        destSectionName,
        decorateMessage,
        fileWithMsg,
        fileWithAssert);
}

//------------------------------------------------------------------------------------------------
ILogDestination* DebugOutDestFactory(
    const efd::utf8string& destSectionName,
    const efd::ISection* pDestSection)
{
    efd::Bool decorateMessage = pDestSection->IsTrue(kConfigDestDecorate);
    efd::Bool fileWithMsg = pDestSection->IsTrue(Logger::kConfigDestFileMsgKey);
    efd::Bool fileWithAssert = pDestSection->IsFalse(Logger::kConfigDestFileAssertKey);

    // Create the destination
    return EE_NEW DebugOutDestination(
        destSectionName,
        decorateMessage,
        fileWithMsg,
        fileWithAssert);
}

//------------------------------------------------------------------------------------------------
ILogDestination* NetMetricsFactory(
    const efd::utf8string& destSectionName,
    const efd::ISection* pDestSection)
{
    // Look for the file name
    efd::utf8string fileName;
    if (!pDestSection->FindValue(Logger::kConfigDestFileNameKey, fileName))
    {
        fileName = "NetMetrics.log";
    }

    // Create the net metrics destination
    return EE_NEW NetMetricsDestination(
        destSectionName,
        fileName);
}

//------------------------------------------------------------------------------------------------
bool Logger::RegisterDestinationFactory(const utf8string& strName, DestinationFactoryMethod pfn)
{
    m_destinationFactory[strName] = pfn;
    return true;
}


//------------------------------------------------------------------------------------------------
ILogDestination* Logger::FactoryLogDestination(
    const efd::utf8string& type,
    const efd::utf8string& strSectionName,
    const efd::ISection* pDestSection)
{
    DestinationFactory::iterator iter = m_destinationFactory.find(type);
    if (iter != m_destinationFactory.end())
    {
        DestinationFactoryMethod pfn = iter->second;
        return (pfn)(strSectionName, pDestSection);
    }
    return NULL;
}


//------------------------------------------------------------------------------------------------
void Logger::ReadFilterConfig(
    const efd::utf8string& destName,
    const efd::ISection* pFilterSection)
{
    // First we need to read out any 'All' module configuration.  These must be set first since
    // all overrides any other settings
    for (efd::ValueIter iter = pFilterSection->GetBeginValueIterator();
        iter != pFilterSection->GetEndValueIterator();
        iter++)
    {
        // Get the module's ID from the string
        UInt16 moduleID = StringToModuleID(iter->first);
        if (kALL == moduleID)
        {
            ReadFilterValue(moduleID, iter->second, destName);
        }
    }

    // Now read all the other non-all modules
    for (efd::ValueIter iter = pFilterSection->GetBeginValueIterator();
        iter != pFilterSection->GetEndValueIterator();
        iter++)
    {
        // Get the module's ID from the string
        UInt16 moduleID = StringToModuleID(iter->first);
        if (kALL != moduleID)
        {
            ReadFilterValue(moduleID, iter->second, destName);
        }
    }
}

//------------------------------------------------------------------------------------------------
void Logger::ReadFilterValue(
    UInt16 moduleID,
    ISectionEntry* pValSection,
    const efd::utf8string& destName)
{
    // Get the module's value section
    if (pValSection != NULL)
    {
        // Get the module's value
        UInt8 level = GetLevelMaskByName(pValSection->GetValue());

        // Set the logging level
        SetLogLevel(moduleID, level, destName);
    }
    else
    {
        EE_FAIL_MESSAGE(("%s configuration value (section) is null",
            GetModuleName(moduleID).c_str()));
    }
}

//------------------------------------------------------------------------------------------------
efd::UInt8 Logger::GetLogLevel(efd::UInt16 moduleId, const utf8string& name) const
{
    SInt32 destIndex = GetDestinationIndexByName(name);
    if (-1 != destIndex)
    {
        const ModuleSettings& mod = GetModuleSettings(moduleId);
        return mod.GetLogMaskForDestination(destIndex);
    }
    return efd::ILogger::kLogMask_None;
}


//------------------------------------------------------------------------------------------------
void Logger::GetLevelNames(efd::set< efd::utf8string >& names) const
{
    LevelNameMap::const_iterator it = m_levelNames.begin();
    for (; it != m_levelNames.end(); ++it)
    {
        names.insert(it->first);
    }
}


//------------------------------------------------------------------------------------------------
void Logger::SetDefaultLogLevels(const efd::utf8string& destName)
{
    SetLogLevel(efd::kALL,
        efd::ILogger::kLogMask_UptoErr3 | efd::ILogger::kLogMask_UptoLvl2,
        destName);
}

//------------------------------------------------------------------------------------------------
void Logger::Flush()
{
    for (efd::UInt32 i = 0; i < EE_ARRAYSIZEOF(m_destinations); ++i)
    {
        if (m_destinations[i])
        {
            m_destinations[i]->Flush();
        }
    }
}

//------------------------------------------------------------------------------------------------
void Logger::FlushFileLogDestinations()
{
    for (efd::UInt32 i = 0; i < EE_ARRAYSIZEOF(m_destinations); ++i)
    {
        FileDestination* fileDestination = 
            EE_DYNAMIC_CAST(FileDestination, (ILogDestination*)m_destinations[i]);

        if (fileDestination != NULL)
        {
            fileDestination->Flush();
        }
    }
}

//------------------------------------------------------------------------------------------------
void Logger::Clone(const efd::Logger* logger)
{
    if (logger)
    {
        m_spModuleEnum  = logger->m_spModuleEnum;
        m_moduleNames = logger->m_moduleNames;
        m_levelNames = logger->m_levelNames;
    }
}

//------------------------------------------------------------------------------------------------
efd::SInt32 Logger::GetDestinationIndexByName(const utf8string& name) const
{
    for (efd::UInt32 i = 0; i < EE_ARRAYSIZEOF(m_destinations); ++i)
    {
        if (m_destinations[i] && m_destinations[i]->GetName() == name)
        {
            return i;
        }
    }
    return -1;
}

//------------------------------------------------------------------------------------------------
void Logger::SetModuleEnum(efd::DataDrivenEnumBase* pEnum)
{
    if (pEnum)
    {
        m_spModuleEnum = pEnum->CastTo<efd::UInt16>();
    }
}


//------------------------------------------------------------------------------------------------
LoggerSingleton* LoggerSingleton::Instance()
{
    if (!ms_instance)
    {
        EE_ASSERT(efd::MemManager::IsInitialized());

        ms_instance = EE_NEW LoggerSingleton();

        // Use a Logger with a PrintfDestination as the default logger.
        ms_instance->m_spLogger = EE_NEW Logger();
        ILogDestinationPtr spDefault = EE_NEW PrintDestination("default");
        ms_instance->m_spLogger->AddDest(spDefault , true);
    }

    return ms_instance;
}

//------------------------------------------------------------------------------------------------
void LoggerSingleton::DestroyInstance()
{
    if (ms_instance)
    {
        ms_instance->m_spLogger = NULL;
        EE_DELETE ms_instance;
        ms_instance = NULL;
    }
}

//------------------------------------------------------------------------------------------------
void LoggerSingleton::Initialize(LoggerPtr logger)
{
    EE_ASSERT(logger != 0);
    if (ms_instance)
    {
        ILoggerPtr ilogger = ms_instance->GetLogger();
        Logger* currentLogger = EE_DYNAMIC_CAST(Logger, ilogger);
        logger->Clone(currentLogger);
    }
    else
    {
        ms_instance = EE_NEW LoggerSingleton;
    }
    ms_instance->m_spLogger = logger;
}

//------------------------------------------------------------------------------------------------
ILogger* LoggerSingleton::GetLogger()
{
    return m_spLogger;
}

//------------------------------------------------------------------------------------------------
LoggerSingleton::LoggerSingleton()
: m_spLogger()
{
}

//------------------------------------------------------------------------------------------------
Logger::ModuleSettings::ModuleSettings()
{
    Clear();
}

//------------------------------------------------------------------------------------------------
void Logger::ModuleSettings::Clear()
{
    for (efd::UInt32 i = 0; i < EE_ARRAYSIZEOF(m_activeDestinations); ++i)
    {
        m_activeDestinations[i] = 0;
    }
}

//------------------------------------------------------------------------------------------------
void Logger::ModuleSettings::Clear(efd::UInt32 mask)
{
    for (efd::UInt32 i = 0; i < EE_ARRAYSIZEOF(m_activeDestinations); ++i)
    {
        m_activeDestinations[i] = BitUtils::ClearBits(m_activeDestinations[i], mask);
    }
}

//------------------------------------------------------------------------------------------------
void Logger::ModuleSettings::MoveDestination(efd::UInt32 oldIndex, efd::UInt32 newIndex)
{
    for (efd::UInt32 i = 0; i < EE_ARRAYSIZEOF(m_activeDestinations); ++i)
    {
        // Set the new index to the value of the old index, whatever that might be:
        m_activeDestinations[i] = BitUtils::SetBitOnOrOffByIndex(m_activeDestinations[i], newIndex,
            BitUtils::TestBitByIndex(m_activeDestinations[i], oldIndex));
        // Then clear the old index:
        m_activeDestinations[i] = BitUtils::ClearBitByIndex(m_activeDestinations[i], oldIndex);
    }
}

//------------------------------------------------------------------------------------------------
efd::UInt32 Logger::ModuleSettings::GetDestinationsMask(efd::UInt8 level) const
{
    EE_ASSERT(level < 8);
    if (level < 8)
    {
        return m_activeDestinations[level];
    }
    return 0;
}

//------------------------------------------------------------------------------------------------
efd::UInt8 Logger::ModuleSettings::GetLogMaskForDestination(efd::SInt32 destIndex) const
{
    efd::UInt8 result = 0;
    efd::UInt32 destMask = 1<<destIndex;
    for (efd::UInt32 i = 0; i < EE_ARRAYSIZEOF(m_activeDestinations); ++i)
    {
        if (0 != (m_activeDestinations[i] & destMask))
        {
            result = BitUtils::SetBitByIndex(result, i);
        }
    }
    return result;
}

//------------------------------------------------------------------------------------------------
void Logger::ModuleSettings::SetLogMask(efd::UInt8 levelMask, efd::UInt32 destMask)
{
    for (efd::UInt32 i = 0; i < EE_ARRAYSIZEOF(m_activeDestinations); ++i)
    {
        // If the levelMask bit for a given index is set then we turn on all the bits
        // the in the destination mask, otherwise we turn off the bits in the destination
        // mask.
        m_activeDestinations[i] = BitUtils::SetBitsOnOrOff(
            m_activeDestinations[i],
            destMask,
            BitUtils::TestBitByIndex(levelMask, i));
    }
}

//------------------------------------------------------------------------------------------------
void Logger::SetServiceManager(ServiceManager* pServiceManager)
{
    m_pServiceManager = pServiceManager;
}

//------------------------------------------------------------------------------------------------
void Logger::SetGameClockClassID(ClassID id)
{
    m_clockID = id;
}

//------------------------------------------------------------------------------------------------
void Logger::SetFlushLogLevel(efd::UInt8 level)
{
    m_flushLevel = level;
}

//------------------------------------------------------------------------------------------------
