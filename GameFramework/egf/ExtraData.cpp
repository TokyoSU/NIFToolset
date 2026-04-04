#include "egfPCH.h"

#include "ExtraData.h"

using namespace egf;
using namespace efd;

ExtraData::ExtraData()
{
}

//--------------------------------------------------------------------------------------------------
ExtraData::ExtraData(const utf8string& name, const utf8string& type)
{
    m_name = name;
    m_type = type;
}

//--------------------------------------------------------------------------------------------------
const utf8string& ExtraData::GetName() const
{
    return m_name;
}

//--------------------------------------------------------------------------------------------------
const utf8string& ExtraData::GetType() const
{
    return m_type;
}

//--------------------------------------------------------------------------------------------------
void ExtraData::AddEntry(const utf8string& type,
        const utf8string& key,
        const utf8string& value)
{
    EE_ASSERT(key.length() > 0);

    ExtraDataEntryPtr entry = EE_NEW ExtraDataEntry();
    entry->m_type = type;
    entry->m_key = key;
    entry->m_value = value;
    m_entries[key] = entry;
}

//--------------------------------------------------------------------------------------------------
ExtraDataEntry* ExtraData::GetEntry(const utf8string& key)
{
    return m_entries[key];
}

//--------------------------------------------------------------------------------------------------
void ExtraData::GetEntriesOfType(const utf8string& type,
                                list<ExtraDataEntryPtr>& o_entries)
{
    for (ExtraDataEntryByName::const_iterator
        iter = m_entries.begin();
        iter != m_entries.end();
        iter++)
    {
        if (iter->second->m_type == type)
        {
            o_entries.push_back(iter->second);
        }
    }
}

//--------------------------------------------------------------------------------------------------
void ExtraData::GetEntriesOfType(const utf8string& type,
                                map<utf8string, ExtraDataEntryPtr> & o_entries)
{
    for (ExtraDataEntryByName::const_iterator
        iter = m_entries.begin();
        iter != m_entries.end();
        iter++)
    {
        if (iter->second->m_type == type)
        {
            o_entries[iter->first] = iter->second;
        }
    }
}
