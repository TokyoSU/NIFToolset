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

#include <efd/ConfigSection.h>
#include <efd/IConfigManager.h>
#include <efd/ConfigSource.h>
#include <efd/Metrics.h>
#include <efd/ILogger.h>
#include <efd/File.h>
#include <efd/BinaryStream.h>
#include <efd/efdLogIDs.h>


using namespace efd;

EE_IMPLEMENT_CONCRETE_CLASS_INFO(SectionEntry);

/*static*/ Section *SectionEntry::ms_pRoot = NULL;

//! Comment character - must be the first none whitespace character
const char efd::IniSource::kCommentChar = ';';
//! Character to open a relative section
const char efd::IniSource::kSectionRelativeOpenChar = '{';
//! Character to close a relative section
const char efd::IniSource::kSectionRelativeCloseChar = '}';
//! Character to open an absolute section header
const char efd::IniSource::kSectionAbsoluteOpenChar = '[';
//! Character to close an absolute section header
const char efd::IniSource:: kSectionAbsoluteCloseChar = ']';


//------------------------------------------------------------------------------------------------
enum __SplitRule
{
    First,
    Last
};
template< __SplitRule sr >
inline bool __SectionSplit(const efd::utf8string& strName, efd::utf8string& o_strSection,
    efd::utf8string& o_strSubSection)
{
    size_t idx;

// Note: We're using the template specifier to identify the operation we want to take.
//       This generates warning C4127: condition expression is constant.
#if defined(EE_PLATFORM_WIN32) || defined(EE_PLATFORM_XBOX360)
#pragma warning(push)
#pragma warning(disable : 4127)
#endif
    if (sr == First)
        idx = strName.find(efd::utf8char_t(IConfigManager::kSectionSeperator));
    else
        idx = strName.rfind(efd::utf8char_t(IConfigManager::kSectionSeperator));
#if defined(EE_PLATFORM_WIN32) || defined(EE_PLATFORM_XBOX360)
#pragma warning(pop)
#endif

    if (idx == efd::utf8string::npos)
    {
        // No seperator, everything goes into o_section
        o_strSection = strName;
        o_strSubSection = "";

        // No split was performed, so return false
        return false;
    }

    o_strSection = strName.substr(0, idx);
    // Start at idx+1 to skip IConfigManager::kSectionSeperator
    o_strSubSection = strName.substr(idx + 1);

    // we split the string, so return true:
    return true;
}

inline bool SectionSplit(const efd::utf8string& strName, efd::utf8string& o_strSection,
    efd::utf8string& o_strSubSection)
{
    return __SectionSplit<First>(strName, o_strSection, o_strSubSection);
}

inline bool SectionSplitLast(const efd::utf8string& strName, efd::utf8string& o_strSection,
    efd::utf8string& o_strSubSection)
{
    return __SectionSplit<Last>(strName, o_strSection, o_strSubSection);
}

//------------------------------------------------------------------------------------------------
inline bool CheckForIllegalChars(const efd::utf8string& strName)
{
    size_t iIllegal = strName.find_first_of(IConfigManager::kIllegalSectionChars);
    if (iIllegal != efd::utf8string::npos)
    {
        EE_LOG(efd::kConfiguration, efd::ILogger::kERR2,
            ("Error: Illegal character in section name %s",
            strName[iIllegal].c_str()));
        return true;
    }
    return false;
}

//------------------------------------------------------------------------------------------------
SectionEntry::SectionEntry(const efd::utf8string& strValue /*= ""*/,
                            int iPriority /*= 0*/,
                            IConfigSource *pSource /*= NULL*/,
                            bool isLink /*= false*/,
                            const SectionEntry *pLink /*= NULL*/)
{
    m_isLink = isLink;
    m_pLink = pLink;
    m_value = strValue;
    m_valueVars = m_value;
    m_priority = iPriority;
    m_spSource = pSource;
}

//------------------------------------------------------------------------------------------------
/*virtual*/ const efd::utf8string& SectionEntry::GetValue() const
{
    // If it's not a link return the value
    if (!m_isLink)
    {
        return m_value;
    }
    else
    {
        // If we have the link object return it's value
        if (m_pLink)
        {
            return m_pLink->GetValue();
        }
        else
        {
            // if we don't have the link object look for it
            if (ms_pRoot)
            {
                const ISectionEntry *pEntry = ms_pRoot->FindEntry(m_value);
                if (pEntry)
                {
                    return pEntry->GetValue();
                }
            }
        }
    }
    // If we don't have a value to return, return an empty string
    return efd::utf8string::NullString();
}

//------------------------------------------------------------------------------------------------
/*virtual*/ int SectionEntry::GetPriority() const
{
    return m_priority;
}

//------------------------------------------------------------------------------------------------
/*virtual*/ int SectionEntry::GetSourcePriority() const
{
    return(m_spSource != NULL) ? m_spSource->GetPriority() : m_priority;
}

//------------------------------------------------------------------------------------------------
/*virtual*/ unsigned int SectionEntry::GetSourceType() const
{
    return(m_spSource != NULL) ? m_spSource->GetSourceType() : IConfigSource::kSourceUnknown;
}

//------------------------------------------------------------------------------------------------
/*virtual*/ const efd::utf8string& SectionEntry::GetSourceName() const
{
    return(m_spSource != NULL) ? m_spSource->GetSourceName() : efd::utf8string::NullString();
}

//------------------------------------------------------------------------------------------------
/*virtual*/ efd::utf8string SectionEntry::GetSourceByString() const
{
    return(m_spSource != NULL) ? m_spSource->SourceToString() : efd::utf8string::NullString();
}

//------------------------------------------------------------------------------------------------
// Section
//------------------------------------------------------------------------------------------------
Section::Section(
    const efd::utf8string& strName,
    Section *pParent,
    IConfigSource *pSource,
    Section *pLink /*= NULL*/)
{
    m_pParent = pParent;
    m_name = strName;
    m_sections.clear();
    m_values.clear();
    m_spSource = pSource;
    m_pLink = pLink;
}

//------------------------------------------------------------------------------------------------
/*virtual*/ Section::~Section()
{
    m_pParent = NULL;

    m_sections.clear();
    m_values.clear();

    m_spSource = NULL;
    m_pLink = NULL;
}

//------------------------------------------------------------------------------------------------
/*virtual*/ const efd::utf8string& Section::GetName() const
{
    return m_name;
}

//------------------------------------------------------------------------------------------------
/*virtual*/ const ISection *Section::GetParent() const
{
    return m_pParent;
}

//------------------------------------------------------------------------------------------------
/*virtual*/ ISection *Section::GetParent()
{
    return m_pParent;
}

//------------------------------------------------------------------------------------------------
/*virtual*/ SectionIter Section::GetBeginChildSectionIterator() const
{
    return m_pLink ? m_pLink->GetBeginChildSectionIterator() : (SectionIter)m_sections.begin();
}

//------------------------------------------------------------------------------------------------
/*virtual*/ SectionIter Section::GetEndChildSectionIterator() const
{
    return m_pLink ? m_pLink->GetEndChildSectionIterator() : (SectionIter)m_sections.end();
}

//------------------------------------------------------------------------------------------------
/*virtual*/ const ISection *Section::FindSection(const efd::utf8string& strName) const
{
    if (m_pLink)
    {
        return m_pLink->FindSection(strName);
    }

    efd::utf8string strSectionName;
    efd::utf8string strSubSection;
    bool bSplit = SectionSplit(strName, strSectionName, strSubSection);

    ISectionPtr pISection;
    m_sections.find(strSectionName, pISection);


    if (pISection && bSplit)
    {
        pISection = pISection->FindSection(strSubSection);
    }

    EE_LOG_METRIC_COUNT(kConfiguration, GetMetricSectionName("LOOKUP.SECTION").c_str());

    return pISection;
}

//------------------------------------------------------------------------------------------------
/*virtual*/ ISection *Section::FindSection(const efd::utf8string& strName)
{
    if (m_pLink)
    {
        return m_pLink->FindSection(strName);
    }

    efd::utf8string strSectionName;
    efd::utf8string strSubSection;
    bool bSplit = SectionSplit(strName, strSectionName, strSubSection);

    ISectionPtr pISection;
    m_sections.find(strSectionName, pISection);

    if (pISection && bSplit)
    {
        pISection = pISection->FindSection(strSubSection);
    }

    return pISection;
}

//------------------------------------------------------------------------------------------------
/*virtual*/ ValueIter Section::GetBeginValueIterator() const
{
    if (m_pLink)
    {
        return m_pLink->GetBeginValueIterator();
    }
    return m_values.begin();
}

//------------------------------------------------------------------------------------------------
/*virtual*/ ValueIter Section::GetEndValueIterator() const
{
    return m_pLink ? m_pLink->GetEndValueIterator() : m_values.end();
}

//------------------------------------------------------------------------------------------------
/*virtual*/ const efd::utf8string& Section::FindValue(const efd::utf8string& strName) const
{
    const ISectionEntry* pEntry = FindEntry(strName);
    if (pEntry)
    {
        return pEntry->GetValue();
    }
    // Since we could not find the value return an empty string
    return efd::utf8string::NullString();
}

//------------------------------------------------------------------------------------------------
bool Section::FindValue(const efd::utf8string& strName, efd::utf8string& o_value) const
{
    const ISectionEntry* pEntry = FindEntry(strName);
    if (pEntry)
    {
        o_value = pEntry->GetValue();
        return true;
    }
    // Do not modify o_value when the entry is not found
    return false;
}

//------------------------------------------------------------------------------------------------
bool Section::IsTrue(const efd::utf8string& strName, bool defaultValue) const
{
    const ISectionEntry* pEntry = FindEntry(strName);
    if (pEntry)
    {
        const efd::utf8string& value = pEntry->GetValue();
        if (0 == value.icompare("true"))
            return true;
        if (value == "1")
            return true;
        return false;
    }
    return defaultValue;
}

//------------------------------------------------------------------------------------------------
bool Section::IsFalse(const efd::utf8string& strName, bool defaultValue) const
{
    const ISectionEntry* pEntry = FindEntry(strName);
    if (pEntry)
    {
        const efd::utf8string& value = pEntry->GetValue();
        if (0 == value.icompare("false"))
            return false;
        if (value == "0")
            return false;
        return true;
    }
    return defaultValue;
}

//------------------------------------------------------------------------------------------------
/*virtual*/ const ISectionEntry *Section::FindEntry(const efd::utf8string& strName) const
{
    if (m_pLink)
    {
        return m_pLink->FindEntry(strName);
    }

    // Check for a prepended section name
    efd::utf8string strSectionName;
    efd::utf8string strSubSection;
    bool bSplit = SectionSplitLast(strName, strSectionName, strSubSection);
    if (bSplit)
    {
        // We have a section prepended to the entry name so find the correct
        // section first and then get the entry from that section
        const ISection *pSection = FindSection(strSectionName);
        if (pSection)
        {
            return pSection->FindEntry(strSubSection);
        }
    }
    else
    {
        // The entry is supposed to be part of this section so look for it here
        ISectionEntryPtr pEntry;
        m_values.find(strName, pEntry);

        EE_LOG_METRIC_COUNT_FMT(kConfiguration,
            ("LOOKUP.ENTRY.%s.%s", GetMetricSectionName().c_str(),
             strName.c_str()));

        if (pEntry)
        {
            return pEntry;
        }
    }
    // Since we could not find the value return NULL
    return NULL;
}

//------------------------------------------------------------------------------------------------
/*virtual*/ ISectionEntry* Section::FindEntry(const efd::utf8string& strName)
{
    if (m_pLink)
    {
        return m_pLink->FindEntry(strName);
    }

    // Check for a prepended section name
    efd::utf8string strSectionName;
    efd::utf8string strSubSection;
    bool bSplit = SectionSplitLast(strName, strSectionName, strSubSection);
    if (bSplit)
    {
        // We have a section prepended to the entry name so find the correct
        // section first and then get the entry from that section
        ISection *pSection = FindSection(strSectionName);
        if (pSection)
        {
            return pSection->FindEntry(strSubSection);
        }
    }
    else
    {
        // The entry is supposed to be part of this section so look for it here
        ISectionEntryPtr pEntry;
        m_values.find(strName, pEntry);

        EE_LOG_METRIC_COUNT_FMT(kConfiguration,
            ("LOOKUP.ENTRY.%s.%s", GetMetricSectionName().c_str(),
             strName.c_str()));

        if (pEntry)
            return pEntry;
    }
    // Since we could not find the value return NULL
    return NULL;
}

//------------------------------------------------------------------------------------------------
/*virtual*/ unsigned int Section::GetSourceType() const
{
    return(m_spSource != NULL) ? m_spSource->GetSourceType() : IConfigSource::kSourceUnknown;
}

//------------------------------------------------------------------------------------------------
/*virtual*/ const efd::utf8string& Section::GetSourceName() const
{
    return(m_spSource != NULL) ? m_spSource->GetSourceName() : efd::utf8string::NullString();
}

//------------------------------------------------------------------------------------------------
/*virtual*/ efd::utf8string Section::GetSourceByString() const
{
    return(m_spSource != NULL) ? m_spSource->SourceToString() : efd::utf8string::NullString();
}

//------------------------------------------------------------------------------------------------
/*virtual*/ ISection *Section::AddSection(const efd::utf8string& strName, IConfigSource *pSource)
{
    if (m_pLink)
    {
        return m_pLink->AddSection(strName, pSource);
    }

    // Check for an illegal separator character
    if (CheckForIllegalChars(strName))
    {
        return NULL;
    }

    efd::utf8string strSectionName;
    efd::utf8string strSubSection;
    bool bSplit = SectionSplit(strName, strSectionName, strSubSection);

    // Check to see if this name is supposed to be a list name and make it
    // unique if required
    ConvertSectionListName(strSectionName);

    ISectionPtr pISection;
    m_sections.find(strSectionName, pISection);

    if (!pISection)
    {
        pISection = EE_NEW Section(strSectionName, this, pSource);

        m_sections[strSectionName] = pISection;

        EE_LOG_METRIC_COUNT_FMT(kConfiguration, ("ADD.SECTION.%s", strName.c_str()));
    }

    if (pISection && bSplit)
    {
        pISection = pISection->AddSection(strSubSection, pSource);
    }

    return pISection;
}

//------------------------------------------------------------------------------------------------
/*virtual*/ bool Section::AddValue(
    ISection *pRoot,
    const efd::utf8string& strName,
    const efd::utf8string& strValue,
    int iPriority,
    IConfigSource *pSource)
{
    if (m_pLink)
    {
        return m_pLink->AddValue(pRoot, strName, strValue, iPriority, pSource);
    }

    // Check for an illegal separator character
    size_t iIllegal = strName.find_first_of(IConfigManager::kIllegalNameChars);
    if (iIllegal != efd::utf8string::npos)
    {
        EE_LOG(efd::kConfiguration, efd::ILogger::kERR2,
            ("Error: Illegal character in name %s",
            strName[ iIllegal].c_str()));
        return false;
    }

    // Check for a prepended section name
    efd::utf8string strSectionName;
    efd::utf8string strSubSection;
    bool bSplit = SectionSplitLast(strName, strSectionName, strSubSection);
    if (bSplit)
    {
        // We have a section prepended to the value name so find the correct
        // section first and then add the value to that section
        ISection *pSection = AddSection(strSectionName, pSource);
        if (pSection)
        {
            return pSection->AddValue(pRoot, strSubSection, strValue, iPriority, pSource);
        }
        else
        {
            EE_LOG(efd::kConfiguration, efd::ILogger::kERR2,
                ("Error: Could not find/create section %s to add entry",
                strSectionName.c_str()));
        }
    }
    else
    {
        // Check to see if this name is supposed to be a list name and make it
        // unique if required
        efd::utf8string strListName = strName;
        ConvertListName(strListName);

        // Look for an existing entry
        ISectionEntryPtr pIEntry;
        m_values.find(strListName, pIEntry);

        SectionEntry* pEntry = (SectionEntry*)pIEntry.data();

        // If there is no existing entry or it is at an equal or lower priority
        // set it to the new values.
        if (!pEntry)
        {
            // Add the new entry
            pEntry = EE_NEW SectionEntry(strValue, iPriority, pSource);
            m_values[strListName] = pEntry;

            EE_LOG_METRIC_COUNT_FMT(kConfiguration, ("ADD.ENTRY.%s", strName.c_str()));
        }
        else if (pEntry->m_priority <= iPriority)
        {
            // Change the old entry
            pEntry->m_value = strValue;
            pEntry->m_valueVars = pEntry->m_value;
            pEntry->m_priority = iPriority;
            pEntry->m_isLink = false;
            pEntry->m_pLink = NULL;
            pEntry->m_spSource = pSource;

            EE_LOG_METRIC_COUNT_FMT(kConfiguration, ("UPDATE.ENTRY.%s", strName.c_str()));
        }
        else
        {
            // Skipping because a higher priority entry exists.  This is still a successful
            // result though, we just don't want to call UpdateVariables.
            EE_LOG(efd::kConfiguration, efd::ILogger::kLVL3,
                ("Skipping name/value '%s'/'%s' in section %s due to higher priority entry",
                strListName.c_str(), strValue.c_str(), GetName().c_str()));
            return true;
        }

        // Update any entries that may use this one as a variable
        UpdateVariables(pRoot, pEntry, pSource);

        return true;
    }

    return false;
}

//------------------------------------------------------------------------------------------------
/*virtual*/ bool Section::AddLink(
    ISection *pRoot,
    const efd::utf8string& strName,
    const efd::utf8string& strLink,
    const ISectionEntry *pILink,
    int iPriority,
    IConfigSource *pSource)
{
    if (m_pLink)
    {
        return m_pLink->AddLink(pRoot, strName, strLink, pILink, iPriority, pSource);
    }
    const SectionEntry *pLink = EE_DYNAMIC_CAST(SectionEntry, pILink);

    // Check for an illegal separator character
    size_t iIllegal = strName.find_first_of(IConfigManager::kIllegalNameChars);
    if (iIllegal != efd::utf8string::npos)
    {
        EE_LOG(efd::kConfiguration, efd::ILogger::kERR2,
            ("Error: Illegal character in name %s",
            strName[ iIllegal].c_str()));
        return false;
    }

    // Check for a prepended section name
    efd::utf8string strSectionName;
    efd::utf8string strSubSection;
    bool bSplit = SectionSplitLast(strName, strSectionName, strSubSection);
    if (bSplit)
    {
        // We have a section prepended to the link name so find the correct
        // section first and then add the link to that section
        ISection *pSection = AddSection(strSectionName, pSource);
        if (pSection)
        {
            return pSection->AddLink(pRoot, strSubSection, strLink, pILink, iPriority, pSource);
        }
        else
        {
            EE_LOG(efd::kConfiguration, efd::ILogger::kERR2,
                ("Error: Could not find/create section %s to add link",
                strSectionName.c_str()));
        }
    }
    else
    {
        // Check to see if this name is supposed to be a list name and make it
        // unique if required
        efd::utf8string strListName = strName;
        ConvertListName(strListName);

        // Look for an existing entry
        ISectionEntryPtr pIEntry;
        m_values.find(strListName, pIEntry);

        SectionEntry* pEntry = (SectionEntry*)pIEntry.data();

        // If there is no existing entry or it is at an equal or lower priority
        // set it to the new values.
        if (!pEntry || (pEntry->m_priority <= iPriority))
        {
            if (!pEntry)
            {
                // Add the new entry
                pEntry = EE_NEW SectionEntry(strLink, iPriority, pSource, true, pLink);
                m_values[strListName] = pEntry;

                EE_LOG_METRIC_COUNT_FMT(kConfiguration, ("ADD.LINK.%s", strLink.c_str()));
            }
            else
            {
                // Change the old entry
                pEntry->m_value = strLink;
                pEntry->m_valueVars = pEntry->m_value;
                pEntry->m_priority = iPriority;
                pEntry->m_isLink = true;
                pEntry->m_pLink = pLink;
                pEntry->m_spSource = pSource;

                EE_LOG_METRIC_COUNT_FMT(kConfiguration, ("UPDATE.LINK.%s", strLink.c_str()));
            }

            // Update any entries that may use this one as a variable
            UpdateVariables(pRoot, pEntry, pSource);

            return true;
        }
    }

    return false;
}

//------------------------------------------------------------------------------------------------
/*virtual*/ bool Section::AddSectionLink(
    const efd::utf8string& strName,
    ISection *pILink,
    IConfigSource *pSource)
{
    bool bRetVal = false;

    // Make sure we have a valid Link object
    if (!pILink)
    {
        EE_LOG(efd::kConfiguration, efd::ILogger::kERR2,
            ("Error: Null pointer passed to AddSectionLink"));
        return false;
    }

    if (m_pLink)
    {
        return m_pLink->AddSectionLink(strName, pILink, pSource);
    }

    // Check for an illegal separator character
    if (CheckForIllegalChars(strName))
    {
        return false;
    }

    efd::utf8string strSectionName;
    efd::utf8string strSubSection;
    bool bSplit = SectionSplit(strName, strSectionName, strSubSection);

    // Check to see if this name is supposed to be a list name and make it
    // unique if required
    ConvertSectionListName(strSectionName);

    ISectionPtr pISection;
    m_sections.find(strSectionName, pISection);

    if (!pISection)
    {
        EE_LOG_METRIC_COUNT_FMT(kConfiguration, ("ADD.SECTION_LINK.%s", strSectionName.c_str()));

        Section *pLink = (Section*) pILink;
        pISection = EE_NEW Section(strSectionName, this, pSource, pLink);

        m_sections[strSectionName] = pISection;
    }

    // If this is a dot separated list of sections we need to recursively add the rest
    if (bSplit)
    {
        bRetVal = pISection->AddSectionLink(strSubSection, pILink, pSource);
    }

    return bRetVal;
}

//------------------------------------------------------------------------------------------------
/*virtual*/ void Section::WriteConfiguration(
    File *pFile,
    const efd::utf8string& strParents,
    const efd::utf8string& strBaseIndent,
    bool bPrintRelative /*= true*/)
{
    efd::utf8string strIndent = strBaseIndent;

    if (!pFile)
    {
        EE_LOG(efd::kConfiguration, efd::ILogger::kERR2,
            ("Error: Could not write to file - NULL pointer"));
        return;
    }

    efd::utf8string strSectionName;
    efd::utf8string strBuffer;
    if (!strParents.empty())
    {
        strSectionName.sprintf(
            "%s%c%s",
            strParents.c_str(),
            IConfigManager::kSectionSeperator,
            GetName().c_str());
    }
    else
    {
        strSectionName = GetName();
    }

    if (m_pLink)
    {
        // Figure out the link path and write it
        efd::utf8string strLinkName = m_pLink->GetName();
        ISection *pParent = m_pLink->GetParent();
        while (pParent)
        {
            efd::utf8string strTemp = strLinkName;
            strLinkName = pParent->GetName();
            strLinkName.append(efd::utf8char_t(IConfigManager::kSectionSeperator));
            strLinkName.append(strTemp);
            pParent = pParent->GetParent();
        }

        // Write the link
        strBuffer.sprintf(
            "%s; %s\n",
            strIndent.c_str(),
            m_spSource ? m_spSource->SourceToString().c_str() : "UNKNOWN");
        pFile->WriteCString(strBuffer.c_str());

        strBuffer.sprintf(
            "%s%c%s%c%s%c\n",
            strIndent.c_str(),
            bPrintRelative ? IniSource::kSectionRelativeOpenChar
                : IniSource::kSectionAbsoluteOpenChar,
            strSectionName.c_str(),
            IConfigManager::kNameLinkSeperator,
            strLinkName.c_str(),
            bPrintRelative ? IniSource::kSectionRelativeCloseChar
                : IniSource::kSectionAbsoluteCloseChar);
        pFile->WriteCString(strBuffer.c_str());

        return;
    }

    if (!strSectionName.empty())
    {
        strBuffer.sprintf(
            "%s; %s\n",
            strIndent.c_str(),
            m_spSource ? m_spSource->SourceToString().c_str() : "UNKNOWN");
        pFile->WriteCString(strBuffer.c_str());

        if (bPrintRelative)
        {
            strBuffer.sprintf(
                "%s%c %s\n",
                strIndent.c_str(),
                IniSource::kSectionRelativeOpenChar,
                GetName().c_str());
            pFile->WriteCString(strBuffer.c_str());
            strIndent.append("  ");
        }
        else
        {
            strBuffer.sprintf(
                "%s%c%s%c\n",
                strIndent.c_str(),
                IniSource::kSectionAbsoluteOpenChar,
                strSectionName.c_str(),
                IniSource::kSectionAbsoluteCloseChar);
            pFile->WriteCString(strBuffer.c_str());
        }
    }

    for (SectionEntryMap::iterator iterValues = m_values.begin();
        m_values.end() != iterValues;
        ++iterValues)
    {
        SectionEntry* pEntry = (SectionEntry*)iterValues->second.data();
        if (pEntry)
        {
            const efd::utf8string& strName = iterValues->first;
            strBuffer.sprintf(
                "%s; Priority: %i %s\n",
                strIndent.c_str(),
                pEntry->m_priority,
                pEntry->m_spSource ? pEntry->m_spSource->SourceToString().c_str()
                    : "UNKNOWN");
            pFile->WriteCString(strBuffer.c_str());

            strBuffer.sprintf(
                "%s%s %c %s\n",
                strIndent.c_str(),
                strName.c_str(),
                pEntry->m_pLink ? IConfigManager::kNameLinkSeperator
                                : IConfigManager::kNameValueSeperator,
                pEntry->m_value.c_str());
            pFile->WriteCString(strBuffer.c_str());
        }
    }

    for (ChildSectionMap::iterator iterSections = m_sections.begin();
        m_sections.end() != iterSections;
        ++iterSections)
    {
        Section* pSection = (Section*)iterSections->second.data();
        if (pSection)
        {
            pSection->WriteConfiguration(pFile, strSectionName, strIndent, bPrintRelative);
        }
    }
    if (bPrintRelative && !strSectionName.empty())
    {
        strBuffer.sprintf("%s%c\n", strBaseIndent.c_str(), IniSource::kSectionRelativeCloseChar);
        pFile->WriteCString(strBuffer.c_str());
    }
}

//------------------------------------------------------------------------------------------------
/*virtual*/ efd::utf8string Section::GetMetricSectionName(const efd::utf8string& strPrefix) const
{
    efd::utf8string strRetVal;

    if (m_pParent)
    {
        strRetVal = m_pParent->GetMetricSectionName(strPrefix);
        strRetVal.append(efd::utf8char_t(IConfigManager::kSectionSeperator));
        strRetVal.append(GetName());
    }
    else
    {
        if (strPrefix.empty())
        {
            strRetVal = "Root";
        }
        else
        {
            strRetVal = strPrefix + ".Root";
        }
    }

    return strRetVal;
}

//------------------------------------------------------------------------------------------------
/*virtual*/ void Section::ConvertListName(efd::utf8string& strName)
{
    efd::utf8string strPrefix;
    efd::utf8string strSuffix;

    if (!strName.empty())
    {
        size_t indexList = strName.find(IConfigManager::kNameListChar);
        if (indexList == efd::utf8string::npos)
            return;

        if (indexList > 0)
            strPrefix = strName.substr(0, indexList);
        if ((indexList + 1) < strName.length())
            strSuffix = strName.substr(indexList + 1);
    }

    int iNum = 1;
    do
    {
        strName.sprintf("%s%i%s", strPrefix.c_str(), iNum++, strSuffix.c_str());
    } while (m_values.end() != m_values.find(strName));
}

//------------------------------------------------------------------------------------------------
/*virtual*/ void Section::ConvertSectionListName(efd::utf8string &strName)
{
    efd::utf8string strPrefix;
    efd::utf8string strSuffix;

    if (!strName.empty())
    {
        size_t indexList = strName.find(efd::utf8char_t(IConfigManager::kNameListChar));
        if (indexList == efd::utf8string::npos)
            return;

        strPrefix = strName.substr(0, indexList);
        strSuffix = strName.substr(indexList + 1);
    }

    int iNum = 1;
    do
    {
        strName.sprintf("%s%i%s", strPrefix.c_str(), iNum++, strSuffix.c_str());
    } while (m_sections.end() != m_sections.find(strName));
}

//------------------------------------------------------------------------------------------------
/*virtual*/ bool Section::ConvertVariables(
    ISection *pRoot,
    SectionEntry *pEntry,
    IConfigSource *pSource,
    unsigned int curDepth)
{
    bool retVal = false;

    if (!pRoot || !pEntry)
        return false;

    // Search for a variable
    pEntry->m_value = pEntry->m_valueVars;
    size_t posStart = pEntry->m_value.find(IConfigManager::kVarStart, 0);

    size_t cchStart = strlen(IConfigManager::kVarStart);
    size_t cchEnd = strlen(IConfigManager::kVarEnd);

    // If a variable exists(loop to process all of them)
    while (posStart != efd::utf8string::npos)
    {
        // Found a variable so set return value true
        retVal = true;

        // Get the value of the variable
        size_t idxStart = posStart + cchStart;
        size_t idxEnd = pEntry->m_value.find(IConfigManager::kVarEnd, idxStart);
        EE_ASSERT(idxEnd >= idxStart);
        efd::utf8string variable = pEntry->m_value.substr(idxStart, idxEnd - idxStart);

        // Find the value of the variable
        ISectionEntry *pIVarEntry = pRoot->FindEntry(variable);
        if (!pIVarEntry)
        {
            // If there is no entry create a dummy entry
            pRoot->AddValue(pRoot, variable, "", -INT_MAX, pSource);
            pIVarEntry = pRoot->FindEntry(variable);
            if (!pIVarEntry)
                continue;
        } // if (!pIVarEntry)

        SectionEntry *pVarEntry = EE_DYNAMIC_CAST(SectionEntry, pIVarEntry);
        const efd::utf8string& varValue = pVarEntry->GetValue();

        // If we are in this method because this entry changed the
        // update the items it depends on
        if (curDepth == 0)
            pVarEntry->m_usedInVar.push_back(pEntry);

        // Construct the new string to insert the value
        efd::utf8string newValue = pEntry->m_value.substr(0, posStart);
        newValue += varValue;
        size_t idxContinuePoint = newValue.length();
        newValue += pEntry->m_value.substr(idxEnd + cchEnd);
        // Check to see if this is a link
        if (!pEntry->m_isLink)
        {
            // This is a value
            pEntry->m_value = newValue;

            // Search for another variable starting from the end of the current replacement
            posStart = pEntry->m_value.find(IConfigManager::kVarStart, idxContinuePoint);
        }
        else
        {
            // This is a link
            pEntry->m_value = newValue;
            pEntry->m_pLink = EE_DYNAMIC_CAST(SectionEntry, pRoot->FindEntry(newValue));

            // Search for another variable starting from the end of the current replacement
            posStart = newValue.find(IConfigManager::kVarStart, idxContinuePoint);
        }
    }

    // Return the value
    return retVal;
}

//------------------------------------------------------------------------------------------------
/*virtual*/ bool Section::UpdateVariables(
    ISection *pRoot,
    SectionEntry *pEntry,
    IConfigSource *pSource,
    unsigned int curDepth /*= 0*/)
{
    bool retVal = false;

    // Prevent infinite recursion by not processing the entry if it is at a
    // depth of more that 20
    if (!pEntry || (curDepth > 20))
        return retVal;

    // Check for and convert variables to values
    retVal = ConvertVariables(pRoot, pEntry, pSource, curDepth);

    // Check to see if there are any entries that use this one as a variable
    if (pEntry->m_usedInVar.empty())
        return retVal;

    for (SectionEntry::SectionUsersList::iterator iter = pEntry->m_usedInVar.begin();
        pEntry->m_usedInVar.end() != iter;
        ++iter)
    {
        SectionEntry* pVarEntry = *iter;

        UpdateVariables(pRoot, pVarEntry, pSource, curDepth + 1);
    } // while (iter)

    return retVal;
}

//------------------------------------------------------------------------------------------------
void Section::RemoveConfigSource(IConfigSource *pSource)
{
    for (ChildSectionMap::iterator iter = m_sections.begin();
        m_sections.end() != iter;
        /*do nothing*/)
    {
        Section* pSection = (Section*)iter->second.data();
        EE_ASSERT(pSection);
        if (pSection->m_spSource == pSource)
        {
            ChildSectionMap::iterator iterToRemove = iter;
            ++iter;
            m_sections.erase(iterToRemove);
        }
        else
        {
            pSection->RemoveConfigSource(pSource);
            ++iter;
        }
    }

    for (SectionEntryMap::iterator iter = m_values.begin();
        m_values.end() != iter;
        /*do nothing*/)
    {
        SectionEntry* pEntry = (SectionEntry*)iter->second.data();
        EE_ASSERT(pEntry);
        if (pEntry->m_spSource == pSource)
        {
            SectionEntryMap::iterator iterToRemove = iter;
            ++iter;
            m_values.erase(iterToRemove);
        }
        else
        {
            ++iter;
        }
    }
}

//------------------------------------------------------------------------------------------------
