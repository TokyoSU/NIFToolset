// EMERGENT GAME TECHNOLOGIES PROPRIETARY INFORMATION
//
// This software is supplied under the terms of a license agreement or
// nondisclosure agreement with Emergent Game Technologies and may not
// be copied or disclosed except in accordance with the terms of that
// agreement.
//
//      Copyright (c) 2006-2009 Todd Berkebile.
//      Copyright (c) 1996-2009 Emergent Game Technologies.
//      All Rights Reserved.
//
// Emergent Game Technologies, Calabasas, CA 91302
// http://www.emergent.net

#include "efdPCH.h"

#include <efd/ParameterList.h>
#include <efd/MemTracker.h>

using namespace efd;

//------------------------------------------------------------------------------------------------
// IParameterConverter
//------------------------------------------------------------------------------------------------
IParameterConverter::~IParameterConverter()
{
}

//------------------------------------------------------------------------------------------------
// ParameterConverterManager
//------------------------------------------------------------------------------------------------
ParameterConverterManager::ConverterMap ParameterConverterManager::ms_converters;

//------------------------------------------------------------------------------------------------
void ParameterConverterManager::RegisterConverter(
    efd::ClassID from,
    efd::ClassID to,
    IParameterConverter* converter)
{
    EE_ASSERT(from && to && from != to);
    ms_converters[MakeConverterID(from, to)] = converter;
}

//------------------------------------------------------------------------------------------------
efd::IParameterConverter* ParameterConverterManager::FindConverter(
    efd::ClassID from,
    efd::ClassID to)
{
    ConverterMap::iterator iter = ms_converters.find(MakeConverterID(from, to));
    if (iter != ms_converters.end())
    {
        return iter->second;
    }
    return NULL;
}

//------------------------------------------------------------------------------------------------
bool ParameterConverterManager::RemoveConverter(efd::ClassID from, efd::ClassID to)
{
    ConverterMap::iterator iter = ms_converters.find(MakeConverterID(from, to));
    if (iter != ms_converters.end())
    {
        EE_DELETE iter->second;
        ms_converters.erase(iter);
        return true;
    }
    return false;
}

//------------------------------------------------------------------------------------------------
void ParameterConverterManager::RemoveAllConverters()
{
    for (ConverterMap::iterator iter = ms_converters.begin();
        iter != ms_converters.end();
        ++ iter)
    {
        EE_DELETE iter->second;
    }
    ms_converters.clear();
}

//------------------------------------------------------------------------------------------------
// ParameterList
//------------------------------------------------------------------------------------------------
ParameterList::ParameterList()
{
    EE_MEM_SETDETAILEDREPORT(this, ParameterList::LeakDump);
}

//------------------------------------------------------------------------------------------------
efd::ClassID ParameterList::GetParameterDataType(efd::UInt32 i_index) const
{
    if (i_index >= m_typeInfo.size())
    {
        return efd::kInvalidClassID;
    }
    return m_typeInfo[i_index].m_dataType;
}

//------------------------------------------------------------------------------------------------
efd::ContainerType ParameterList::GetParameterContainerType(efd::UInt32 i_index) const
{
    if (i_index >= m_typeInfo.size())
    {
        return efd::ct_Invalid;
    }
    return static_cast<efd::ContainerType>(m_typeInfo[i_index].m_container);
}

//------------------------------------------------------------------------------------------------
inline const char* SkipCompressedSize(const char* pszItemName)
{
    // The data points to a compressed UInt32 followed by null-terminated string data.
    // We want to skip over the variable size (see the code for SR_As32Bit_Compressed
    // for details of how the compression works) so we need to compute how many bytes
    // it uses. The top two bits of the first byte tell us what we need to know:
    efd::UInt32 cSizeBytes = ((*pszItemName) >> 6) + 1;
    // 1 means 1, 2 means 2, 3 means 4, and 4 means 5:
    if (cSizeBytes > 2) ++cSizeBytes;

    return pszItemName + cSizeBytes;
}

//------------------------------------------------------------------------------------------------
const char* ParameterList::GetParameterName(efd::UInt32 i_index) const
{
    if (i_index >= m_typeInfo.size())
    {
        // Bad index
        return NULL;
    }
    if (!m_typeInfo[i_index].m_nameSize)
    {
        // Item is not named
        return NULL;
    }
    EE_ASSERT(m_parameterStorage.GetSize() >= m_typeInfo[i_index].m_nameOffset);

    const char* pszItemName =
        (const char*)(m_parameterStorage.GetBuffer() + m_typeInfo[i_index].m_nameOffset);

    // Skip the compressed string size:
    return SkipCompressedSize(pszItemName);
}

//------------------------------------------------------------------------------------------------
efd::SInt32 ParameterList::GetIndexFromName(const char* pszName) const
{
    for (efd::UInt32 i = 0; i < m_typeInfo.size(); ++i)
    {
        // Not all items have a name, unnamed items will have 0 for the name size.
        if (m_typeInfo[i].m_nameSize)
        {
            EE_ASSERT(m_parameterStorage.GetSize() >= m_typeInfo[i].m_nameOffset);
            const efd::UInt8* pData = m_parameterStorage.GetBuffer() + m_typeInfo[i].m_nameOffset;
            const char* pszItemName = SkipCompressedSize((const char*)pData);
            if (0 == efd::Stricmp(pszName, pszItemName))
            {
                return (efd::SInt32)i;
            }
        }
    }
    return -1;
}

//------------------------------------------------------------------------------------------------
void ParameterList::Serialize(efd::Archive& ar)
{
    SR_ResizableArray<>::Serialize(m_typeInfo, ar);
    Serializer::SerializeObject(m_parameterStorage, ar);
}

//------------------------------------------------------------------------------------------------
void ParameterList::ParameterTypeInfo::Serialize(efd::Archive& ar)
{
    Serializer::SerializeObject(m_dataType, ar);
    Serializer::SerializeObject(m_nameOffset, ar);
    Serializer::SerializeObject(m_nameSize, ar);
    Serializer::SerializeObject(m_offset, ar);
    Serializer::SerializeObject(m_size, ar);
    Serializer::SerializeObject(m_container, ar);
}

//------------------------------------------------------------------------------------------------
void ParameterList::LeakDump(void* pMem, char* o_buffer, unsigned int i_cchBuffer)
{
    ParameterList* pThis = reinterpret_cast<ParameterList*>(pMem);

    // save some room at the end
    EE_ASSERT(i_cchBuffer > 5);
    i_cchBuffer -= 5;

    UInt32 numParams = pThis->m_typeInfo.size();
    SInt32 sizeUsed =
        efd::Snprintf(o_buffer, i_cchBuffer, EE_TRUNCATE, "ParameterList<%d", numParams);
    for (UInt32 i = 0; i < numParams; ++i)
    {
        o_buffer += sizeUsed;
        i_cchBuffer -= sizeUsed;

        const char* pszName = pThis->GetParameterName(i);
        if (pszName)
        {
            sizeUsed = efd::Snprintf(o_buffer, i_cchBuffer, EE_TRUNCATE, ", '%s'", pszName);
        }
        else
        {
            sizeUsed = efd::Snprintf(o_buffer, i_cchBuffer, EE_TRUNCATE, ", %u", i);
        }
    }
    o_buffer += sizeUsed;
    i_cchBuffer -= sizeUsed;

    if (0 == i_cchBuffer)
    {
        // filled the buffer, show '...' at end
        efd::Strcat(o_buffer, 5, "...>");
    }
    else
    {
        efd::Strcat(o_buffer, 5, ">");
    }
}

//------------------------------------------------------------------------------------------------
const efd::SmartBuffer ParameterList::GetParameterStorage(efd::UInt32 index)
{
    return m_parameterStorage.MakeWindow(m_typeInfo[index].m_offset, m_typeInfo[index].m_size);
}

//------------------------------------------------------------------------------------------------
efd::Archive efd::ParameterList::PrepareNewParameter()
{
    ParameterTypeInfo pti;
    pti.m_dataType = kInvalidClassID;
    pti.m_nameOffset = 0;
    pti.m_nameSize = 0;
    pti.m_offset = 0;
    pti.m_size = 0;
    pti.m_container = ct_Invalid;
    m_typeInfo.push_back(pti);

    efd::Archive ar(efd::Archive::Packing, m_parameterStorage);
    // skip to the end of the previously used space
    ar.GetBytes(m_parameterStorage.GetSize());
    return ar;
}

//------------------------------------------------------------------------------------------------
void efd::ParameterList::SetParameterName(const utf8string& name, efd::Archive& ar)
{
    ParameterTypeInfo& pti = m_typeInfo.back();

    pti.m_nameOffset = (efd::UInt16)ar.GetCurrentPosition();
    efd::Serializer::SerializeConstObject(name, ar);
    pti.m_nameSize = (efd::UInt16)ar.GetCurrentPosition() - pti.m_nameOffset;
    EE_ASSERT(pti.m_nameSize > name.size() + 1);
}

//------------------------------------------------------------------------------------------------
efd::SInt32 efd::ParameterList::FinishNewParameter(efd::Archive& ar)
{
    EE_ASSERT(m_typeInfo.back().m_dataType != kInvalidClassID);
    EE_ASSERT(m_typeInfo.back().m_container != ct_Invalid);

    if (ar.GetError())
    {
        m_typeInfo.pop_back();
        return -1;
    }

    m_parameterStorage = ar.GetUsedBuffer();
    return m_typeInfo.size() - 1;
}

//------------------------------------------------------------------------------------------------
