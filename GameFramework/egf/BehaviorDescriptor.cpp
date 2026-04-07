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

#include <egf/BehaviorDescriptor.h>
#include <egf/FlatModel.h>
#include <efd/ServiceManager.h>
#include <efd/ILogger.h>
#include <efd/Metrics.h>
#include <efd/PathUtils.h>
#include <efd/BitUtils.h>
#include <egf/egfLogIDs.h>
#include <efd/MemTracker.h>


using namespace efd;
using namespace egf;


BehaviorDescriptor::BehaviorMap BehaviorDescriptor::ms_behaviorMap;

//------------------------------------------------------------------------------------------------
// Constructors for the BehaviorDescriptor class
//
// Since we use framework to create behavior instances, we don't get to pass params, so we call
// the default constructor and then private Setter methods from static create methods to fill in
// data...
BehaviorDescriptor::BehaviorDescriptor(BehaviorID idBehavior, const efd::utf8string& strName,
    const efd::utf8string& strModelName, BehaviorTypes type, efd::UInt32 flags)
: m_behaviorID(idBehavior)
, m_traits(flags)
, m_name(strName)
, m_modelName(strModelName)
, m_type(type)
, m_source()
, m_pBehaviorFunction(NULL)
{
    EE_MEM_SETDETAILEDREPORT(this, BehaviorDescriptor::LeakDump);
}

//------------------------------------------------------------------------------------------------
BehaviorDescriptor::BehaviorDescriptor(const BehaviorDescriptor& i_other)
: m_behaviorID(i_other.m_behaviorID)
, m_traits(i_other.m_traits)
, m_name(i_other.m_name)
, m_modelName(i_other.m_modelName)
, m_type(i_other.m_type)
, m_source(i_other.m_source)
, m_pBehaviorFunction(i_other.m_pBehaviorFunction)
{
}

//------------------------------------------------------------------------------------------------
BehaviorDescriptor::~BehaviorDescriptor()
{
}

//------------------------------------------------------------------------------------------------
bool BehaviorDescriptor::Initialize()
{
    if (m_traits & BehaviorTrait_Init)
    {
        // Already initialized; nothing else to do.
        return true;
    }

    // Increment another created behavior.
    EE_LOG_METRIC_COUNT_FMT(kBehavior,
        ("INITIALIZE.%s.%u",
         m_modelName.c_str(),
         m_behaviorID));

    // Now see if have a "C" or "Cpp" behavior script.
    switch (m_type)
    {
    case BehaviorType_C:
    case BehaviorType_Cpp:
        {
            // Look for statically registered behaviors first.
            m_pBehaviorFunction = GetStaticBehavior(m_modelName, GetName());

            if (m_pBehaviorFunction)
                break;  // We found it in the static map; use that one.

            // Get a handle to the DLL module.
#if defined(EE_DYNAMIC_BEHAVIOR_LOAD)
            if (m_source.LoadModule(m_modelName))
            {
                // If the handle is valid, try to get the function address.
                m_pBehaviorFunction = (CBehaviorFunc)m_source.GetMethod(GetName());
                if (!m_pBehaviorFunction)
                {
                    // If here, was not able to find the behavior function in that DLL.
                    EE_LOG(efd::kBehavior, efd::ILogger::kERR2,
                        ("Error: Unable to find behavior function %s in model DLL file %s.  "
                        "Attempt to add behavior aborted.", GetName().c_str(),
                        m_modelName.c_str(), GetName().c_str()));
                    return false;
                }
            }
            else
            {
                // If here, unable to open the DLL for this script.
                EE_LOG(efd::kBehavior, efd::ILogger::kERR2,
                    ("Error: Unable to open DLL %s for behavior %s (%i).  Attempt to add "
                    "behavior aborted.", m_modelName.c_str(), GetName().c_str(), GetID()));
                return false;
            }
#else
            EE_LOG(efd::kBehavior, efd::ILogger::kERR2,
                ("Error: Attempt to load a DLL behavior in a configuration that does not support "
                "it."));
            EE_LOG(efd::kBehavior, efd::ILogger::kERR2,
                ("DLL: %s for behavior %s (%i). Attempt to add behavior aborted.",
                 m_modelName.c_str(), GetName().c_str(), GetID()));
            return false;
#endif
        }
        break;

    case BehaviorType_Builtin:
    case BehaviorType_Python:
    case BehaviorType_Remote:
    case BehaviorType_Lua:
    case BehaviorType_Abstract:

        // Nothing to do here.
        break;

    default:
        EE_LOG(efd::kBehavior, efd::ILogger::kERR2,
            ("Error: Cannot initialize behavior '%s::%s' with invalid type %d.",
            GetModelName().c_str(), GetName().c_str(), GetType()));
        return false;
    }

    m_traits = m_traits | BehaviorTrait_Init;   // Set flags (including we are initialized).

    return true;
}

//------------------------------------------------------------------------------------------------
bool BehaviorDescriptor::IsValid() const
{
    if (0 == m_behaviorID)
    {
        EE_LOG(efd::kBehavior, efd::ILogger::kERR2,
            ("BehaviorDescriptor::IsValid: invalid behavior id in '%s::%s'.",
            m_modelName.c_str(), m_name.c_str()));
        return false;
    }
    if (m_name.empty())
    {
        EE_LOG(efd::kBehavior, efd::ILogger::kERR2,
            ("BehaviorDescriptor::IsValid: no behavior name provided in behavior (%i) from "
            "model '%s'.", m_behaviorID, m_modelName.c_str()));
        return false;
    }
    switch (m_type)
    {
    case BehaviorType_Invalid:
    case BehaviorType_Abstract:
        EE_LOG(efd::kBehavior, efd::ILogger::kERR2,
            ("BehaviorDescriptor::IsValid: invalid type %s in '%s::%s'.",
            GetTypeName(),
            m_modelName.c_str(),
            m_name.c_str()));
        return false;

    case BehaviorType_Virtual:
        if (m_invocationOrderedModelNames.empty())
        {
            EE_LOG(efd::kBehavior, efd::ILogger::kERR2,
                ("BehaviorDescriptor::IsValid: empty invocation list in virtual behavior %s (%i).",
                m_name.c_str(),
                m_behaviorID));
            return false;
        }
        // ... fall through ...
    case BehaviorType_Remote:
        // These types can have an empty m_modelName so skip that check
        break;

    default:
        if (m_modelName.empty())
        {
            EE_LOG(efd::kBehavior, efd::ILogger::kERR2,
                ("BehaviorDescriptor::IsValid: no model name provided in behavior %s (%i).",
                m_name.c_str(),
                m_behaviorID));
            return false;
        }
        break;
    }

    return true;
}

//------------------------------------------------------------------------------------------------
const efd::utf8string& BehaviorDescriptor::GetName() const
{
    return m_name;
}

//------------------------------------------------------------------------------------------------
const efd::utf8string& BehaviorDescriptor::GetModelName() const
{
    return m_modelName;
}

//------------------------------------------------------------------------------------------------
void BehaviorDescriptor::SetModelName(const efd::utf8string& modelName)
{
    m_modelName = modelName;
}

//------------------------------------------------------------------------------------------------
BehaviorID BehaviorDescriptor::GetID() const
{
    return m_behaviorID;
}

//------------------------------------------------------------------------------------------------
BehaviorTypes BehaviorDescriptor::GetType() const
{
    return m_type;
}

//------------------------------------------------------------------------------------------------
void BehaviorDescriptor::SetType(BehaviorTypes i_type)
{
    m_type = i_type;
}

//------------------------------------------------------------------------------------------------
efd::Bool BehaviorDescriptor::GetTrait(BehaviorTraits trait) const
{
    return BitUtils::AllBitsAreSet(m_traits, (UInt32)trait);
}

//------------------------------------------------------------------------------------------------
efd::UInt32 BehaviorDescriptor::GetTraits() const
{
    return m_traits;
}

//------------------------------------------------------------------------------------------------
void BehaviorDescriptor::SetTrait(BehaviorTraits trait, efd::Bool val)
{
    m_traits = BitUtils::SetBitsOnOrOff(m_traits, (UInt32)trait, val);
}

//------------------------------------------------------------------------------------------------
CBehaviorFunc BehaviorDescriptor::GetCFunctionPtr() const
{
    return m_pBehaviorFunction;
}

//------------------------------------------------------------------------------------------------
const BehaviorDescriptor::InvocationOrderedModelNamesList&
    BehaviorDescriptor::GetInvocationOrderedModelNames() const
{
    return m_invocationOrderedModelNames;
}

//------------------------------------------------------------------------------------------------
void BehaviorDescriptor::AddInvocationOrderedModelName(const efd::utf8string& modelName)
{
    m_invocationOrderedModelNames.push_back(modelName);
}

//------------------------------------------------------------------------------------------------
void BehaviorDescriptor::ClearInvocationOrderList()
{
    m_invocationOrderedModelNames.clear();
}

//------------------------------------------------------------------------------------------------
bool BehaviorDescriptor::AddStaticBahavior(const efd::utf8string& modelName,
    const efd::utf8string& behaviorName, CBehaviorFunc behaviorFunction)
{
    efd::utf8string lookupName(Formatted, "%s!%s", modelName.c_str(), behaviorName.c_str());

    if (ms_behaviorMap.find(lookupName) != ms_behaviorMap.end())
        return false;

    ms_behaviorMap[lookupName] = behaviorFunction;
    return true;
}

//------------------------------------------------------------------------------------------------
CBehaviorFunc BehaviorDescriptor::GetStaticBehavior(const efd::utf8string& modelName,
    const efd::utf8string& behaviorName)
{
    efd::utf8string lookupName(Formatted, "%s!%s", modelName.c_str(), behaviorName.c_str());
    BehaviorMap::iterator it = ms_behaviorMap.find(lookupName);

    if (it== ms_behaviorMap.end())
        return NULL;

    return (*it).second;
}

//------------------------------------------------------------------------------------------------
struct StrValuePair
{
    const char* str;
    int value;
};

//------------------------------------------------------------------------------------------------
//  String table for behavior types -- must match the order declared in BehaviorTypes
StrValuePair BehaviorTypeTable[] =
{
    {"Invalid",   BehaviorType_Invalid},
    {"C",         BehaviorType_C},
    {"Cpp",       BehaviorType_Cpp},
    {"Builtin",   BehaviorType_Builtin},
    {"Remote",    BehaviorType_Remote},
    {"Virtual",   BehaviorType_Virtual},
    {"Lua",       BehaviorType_Lua},
    {"Python",    BehaviorType_Python},
    {"Abstract",  BehaviorType_Abstract},
};

//------------------------------------------------------------------------------------------------
const char* BehaviorDescriptor::GetTypeName() const
{
    if ((UInt32)m_type < EE_ARRAYSIZEOF(BehaviorTypeTable))
    {
        return BehaviorTypeTable[m_type].str;
    }

    return "[Unknown Behavior Type]";
}

//------------------------------------------------------------------------------------------------
void BehaviorDescriptor::LeakDump(void* pMem, char* o_buffer, unsigned int i_cchBuffer)
{
    BehaviorDescriptor* pbd = reinterpret_cast<BehaviorDescriptor*>(pMem);

    efd::Snprintf(o_buffer, i_cchBuffer, EE_TRUNCATE, "BehaviorDescriptor<%s:%s>",
        pbd ? pbd->GetModelName().c_str() : "unknown",
        pbd ? pbd->GetName().c_str() : "unknown");
}

//------------------------------------------------------------------------------------------------
void BehaviorDescriptor::_SDMInit()
{
}

//------------------------------------------------------------------------------------------------
void BehaviorDescriptor::_SDMShutdown()
{
    ms_behaviorMap.clear();
}

//------------------------------------------------------------------------------------------------
