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
// Emergent Game Technologies, Calabasas, California 91302
// http://www.emergent.net

#include "efdPCH.h"

#include <efd/QOSCompare.h>
#include <efd/ConfigManager.h>
#include <efd/EnumManager.h>
#include <efd/efdLogIDs.h>
#include <efd/Logger.h>

const static char* kConfigSection = "QualityOfService";
const static char* kVirtualQOSEnum = "efdVirtualQualityOfService";
const static char* kPhysicalQOSEnum = "efdQualityOfService";

//------------------------------------------------------------------------------------------------
using namespace efd;

efd::map< efd::QualityOfService, efd::QualityOfService > QOSCompare::ms_virtualToPhysical;
bool QOSCompare::ms_configRead = false;
QualityOfService QOSCompare::ms_defaultQualityOfService = kQOS_TCP;

//------------------------------------------------------------------------------------------------
efd::QualityOfService QOSCompare::GetVirtualQOS(
    EnumManager* pEnumManager, 
    const utf8string& stringName)
{
    QualityOfService foundQOS = QOS_INVALID;
    if (pEnumManager)
    {
        efd::DataDrivenEnumBase* pEnum = pEnumManager->FindOrLoadEnum(kVirtualQOSEnum);
        if (pEnum)
        {
            efd::SmartPointer< DataDrivenEnum<efd::UInt32> > spVirtualEnum =
                pEnum->CastTo<efd::UInt32>();;
            if (spVirtualEnum && spVirtualEnum->FindEnumValue(stringName, foundQOS))
            {
                return foundQOS;
            }
        }
    }
    // next try to convert string to integer.  atoi returns 0 if it fails and 0 is kVQOS_Invalid
    foundQOS = atoi(stringName.c_str());
    return foundQOS;
}

//------------------------------------------------------------------------------------------------
efd::QualityOfService QOSCompare::GetPhysicalQOS(
    EnumManager* pEnumManager, 
    const utf8string& stringName)
{
    QualityOfService foundQOS = QOS_INVALID;
    if (pEnumManager)
    {
        efd::DataDrivenEnumBase* pEnum = pEnumManager->FindOrLoadEnum(kPhysicalQOSEnum);
        if (pEnum)
        {
            efd::SmartPointer< DataDrivenEnum<efd::UInt32> > spQOSEnum =
                pEnum->CastTo<efd::UInt32>();
            if (spQOSEnum && spQOSEnum->FindEnumValue(stringName, foundQOS))
                return foundQOS;
        }
    }

    // Next, try to convert string to integer.  atoi returns 0 if it fails and 0 is kVQOS_Invalid.
    foundQOS = atoi(stringName.c_str());
    return foundQOS;
}

//------------------------------------------------------------------------------------------------
void QOSCompare::ReadConfig(IConfigManager* pConfigManager, EnumManager* pEnumManager)
{
    // only read config once
    if (ms_configRead)
        return;
    ms_configRead = true;

    // the default QOS must be reliable, the system counts on it.
    EE_ASSERT(ms_defaultQualityOfService & NET_RELIABLE);

    // Set defaults. Any value not found will default to ms_defaultQualityOfService so any virtual 
    // QOS that needs to be unreliable must have a default set here
    ms_virtualToPhysical[QOS_UNRELIABLE] = kQOS_Unreliable;
    //ms_virtualToPhysical[QOS_UNRELIABLE_ORDERED] = kQOS_UnreliableOrdered;
    ms_virtualToPhysical[QOS_CONNECTIONLESS] = kQOS_UnreliableConnectionless;

    if (!pConfigManager)
        return;
    // Get the root config section
    const ISection* pRoot = pConfigManager->GetConfiguration();
    EE_ASSERT(pRoot);

    // Find our section
    const ISection* config = pRoot->FindSection(kConfigSection);
    if (config)
    {
        // Default must be a physical QOS
        utf8string defaultQOS = config->FindValue("Default");
        if (!defaultQOS.empty())
        {
            QualityOfService physicalQOS = GetPhysicalQOS(pEnumManager, defaultQOS);
            if (physicalQOS != QOS_INVALID)
            {
                ms_defaultQualityOfService = physicalQOS;
                EE_ASSERT(ms_defaultQualityOfService & NET_RELIABLE);
            }
            else
            {
                EE_LOG(
                efd::kNetwork,
                    ILogger::kERR1,
                    ("%s> Invalid Default physical QualityOfService:%s",
                    __FUNCTION__,
                    defaultQOS.c_str()));
            }
        }
        ValueIter end = config->GetEndValueIterator();
        ValueIter i = config->GetBeginValueIterator();
        // iterate and add all qos mappings
        for (; i!=end; ++i)
        {
            QualityOfService virtualQOS = GetVirtualQOS(pEnumManager, i->first);
            QualityOfService physicalQOS = GetPhysicalQOS(pEnumManager, i->second->GetValue());
            bool invalid = false;
            if (virtualQOS == QOS_INVALID)
            {
                EE_LOG(
                    efd::kNetwork,
                    ILogger::kERR1,
                    ("%s> Invalid virtual QualityOfService:%s",
                    __FUNCTION__,
                    i->first.c_str()));
                invalid = true;
            }
            if (physicalQOS == QOS_INVALID)
            {
                EE_LOG(
                    efd::kNetwork,
                    ILogger::kERR1,
                    ("%s> Invalid physical QualityOfService:%s",
                    __FUNCTION__,
                    i->second->GetValue().c_str()));
                invalid = true;
            }

            // add mapping
            if (!invalid)
            {
                ms_virtualToPhysical[virtualQOS] = physicalQOS;
            }
        }
    }
}
