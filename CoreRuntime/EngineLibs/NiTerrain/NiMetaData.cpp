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

#include "NiTerrainPCH.h"

#include "NiTerrainXMLHelpers.h"
#include "NiMetaData.h"

#include <efd/ecrLogIDs.h>

using namespace efd;


//--------------------------------------------------------------------------------------------------
NiMetaData::NiMetaData(const NiMetaData& kMetaData) :
    m_pkValueMap(0)
{
    *this = kMetaData;
}

//--------------------------------------------------------------------------------------------------
NiMetaData::NiMetaData() :
    m_pkValueMap(0)
{
}

//--------------------------------------------------------------------------------------------------
bool NiMetaData::Set(const NiFixedString& kKey, NiInt32 iValue,
    KeyType eType, float fWeight)
{
    if (!m_pkValueMap)
        m_pkValueMap = NiNew NiTStringPointerMap<NiMetaData::MetaDataValue*>;

    NiMetaData::MetaDataValue* pkCurKeyData;
    if (!m_pkValueMap->GetAt(kKey, pkCurKeyData))
    {
        pkCurKeyData = NiNew MetaDataValue();
        m_pkValueMap->SetAt(kKey, pkCurKeyData);
    }

    EE_ASSERT(eType == INTEGER || eType == INTEGER_BLENDED);

    pkCurKeyData->eType = eType;
    pkCurKeyData->iValue = iValue;
    pkCurKeyData->kStringValue = NULL;
    pkCurKeyData->fWeight = fWeight;

    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiMetaData::Set(const NiFixedString& kKey, float fValue, KeyType eType,
    float fWeight)
{
    if (!m_pkValueMap)
        m_pkValueMap = NiNew NiTStringPointerMap<NiMetaData::MetaDataValue*>;

    NiMetaData::MetaDataValue* pkCurKeyData;
    if (!m_pkValueMap->GetAt(kKey, pkCurKeyData))
    {
        pkCurKeyData = NiNew MetaDataValue();
        m_pkValueMap->SetAt(kKey, pkCurKeyData);
    }

    EE_ASSERT(eType == FLOAT || eType == FLOAT_BLENDED);

    pkCurKeyData->eType = eType;
    pkCurKeyData->fValue = fValue;
    pkCurKeyData->kStringValue = NULL;
    pkCurKeyData->fWeight = fWeight;

    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiMetaData::Set(const NiFixedString& kKey, const NiFixedString& kValue,
    KeyType eType, float fWeight)
{
    if (!m_pkValueMap)
        m_pkValueMap = NiNew NiTStringPointerMap<NiMetaData::MetaDataValue*>;

    NiMetaData::MetaDataValue* pkCurKeyData;
    if (!m_pkValueMap->GetAt(kKey, pkCurKeyData))
    {
        pkCurKeyData = NiNew MetaDataValue();
        m_pkValueMap->SetAt(kKey, pkCurKeyData);
    }

    EE_ASSERT(eType == STRING);

    pkCurKeyData->eType = eType;
    pkCurKeyData->iValue = 0;
    pkCurKeyData->kStringValue = kValue;
    pkCurKeyData->fWeight = fWeight;

    return true;
}

//--------------------------------------------------------------------------------------------------
void NiMetaData::UpdateWeights(float fWeight) const
{
    if (!m_pkValueMap)
        return;

    NiTMapIterator kIter = m_pkValueMap->GetFirstPos();
    while (kIter)
    {
        const char* pcStr;
        MetaDataValue* pkValue;

        m_pkValueMap->GetNext(kIter, pcStr, pkValue);
        EE_ASSERT(pkValue);
        pkValue->fWeight = fWeight;
    }
}

//--------------------------------------------------------------------------------------------------
float NiMetaData::GetWeight() const
{
    if (!m_pkValueMap)
        return 0.0f;

    NiTMapIterator kIter = m_pkValueMap->GetFirstPos();
    const char* pcStr;
    MetaDataValue* pkValue;

    if (kIter)
    {
        m_pkValueMap->GetNext(kIter, pcStr, pkValue);
        EE_ASSERT(pkValue);
        return pkValue->fWeight;
    }
    else
    {
        return 1.0f;
    }
}

//--------------------------------------------------------------------------------------------------
void NiMetaData::Blend(const NiMetaData* pkMetaData, float fWeight)
{
    if (fWeight <= 0.0f || !pkMetaData->m_pkValueMap)
        return;

    if (!m_pkValueMap)
        m_pkValueMap = NiNew NiTStringPointerMap<NiMetaData::MetaDataValue*>;

    const char* pcCurKey;
    NiMetaData::MetaDataValue* pkCurKeyData;

    // Iterate over the map and add all the key values:
    NiTMapIterator kIter = pkMetaData->m_pkValueMap->GetFirstPos();
    while (kIter != NULL)
    {
        pkMetaData->m_pkValueMap->GetNext(kIter, pcCurKey, pkCurKeyData);

        // For each item in the metadata:
        NiMetaData::MetaDataValue* pkLocalKeyData = 0;
        if (!m_pkValueMap->GetAt(pcCurKey, pkLocalKeyData))
        {
            // No value exists for this yet
            pkLocalKeyData = NiNew MetaDataValue();
            pkLocalKeyData->eType = pkCurKeyData->eType;
            m_pkValueMap->SetAt(pcCurKey, pkLocalKeyData);

            // Initialise the value:
            pkLocalKeyData->fWeight = 0.0f;
            switch (pkLocalKeyData->eType)
            {
                case FLOAT:
                case FLOAT_BLENDED:
                    pkLocalKeyData->fValue = 0.0f;
                    break;
                case INTEGER:
                case INTEGER_BLENDED:
                    pkLocalKeyData->iValue = 0;
                    break;
                case STRING:
                    pkLocalKeyData->kStringValue = "";
                    break;
                default:
                    *pkLocalKeyData = 0;
                    break;
            }
        }
        else
        {
            // Check that the types are correct            
            if (pkLocalKeyData->eType != pkCurKeyData->eType)
            {
                pkLocalKeyData->eType = ERROR_TYPE;
                pkLocalKeyData->kStringValue = "NiMetaData Blending Error: Metadata types are not "
                    "consistent for the same key.";
                pkLocalKeyData->fWeight = fWeight;

                EE_LOG(efd::kGamebryoGeneral0, efd::ILogger::kERR1,
                    ("NiMetaData: Could not blend meta data key. Metadata types are not "
                    "consistent for the same key."));
                continue;
            }

        }

        switch (pkLocalKeyData->eType)
        {
            case FLOAT_BLENDED:
                pkLocalKeyData->fValue +=
                    pkCurKeyData->fValue * fWeight;
                break;
            case INTEGER_BLENDED:
                pkLocalKeyData->iValue +=
                    (NiUInt32)(pkCurKeyData->iValue * fWeight);
                break;
            default:
                if (pkLocalKeyData->fWeight < fWeight)
                {
                    *pkLocalKeyData = *pkCurKeyData;
                    pkLocalKeyData->fWeight = fWeight;
                }
                break;
        }
    }
}

//--------------------------------------------------------------------------------------------------
bool NiMetaData::Get(const NiFixedString& kKey, NiInt32& iValue,
    float& fWeight) const
{
    if (!m_pkValueMap)
        return false;

    NiMetaData::MetaDataValue* pkCurKeyData;
    if (!m_pkValueMap->GetAt(kKey, pkCurKeyData))
        return false;

    if (pkCurKeyData->eType == INTEGER ||
        pkCurKeyData->eType == INTEGER_BLENDED)
    {
        iValue = pkCurKeyData->iValue;
        fWeight = pkCurKeyData->fWeight;
        return true;
    }
    return false;
}

//--------------------------------------------------------------------------------------------------
bool NiMetaData::Get(const NiFixedString& kKey, float& fValue,
    float& fWeight) const
{
    if (!m_pkValueMap)
        return false;

    NiMetaData::MetaDataValue* pkCurKeyData;
    if (!m_pkValueMap->GetAt(kKey, pkCurKeyData))
        return false;

    if (pkCurKeyData->eType == FLOAT ||
        pkCurKeyData->eType == FLOAT_BLENDED)
    {
        fValue = pkCurKeyData->fValue;
        fWeight = pkCurKeyData->fWeight;
        return true;
    }
    return false;
}

//--------------------------------------------------------------------------------------------------
bool NiMetaData::Get(const NiFixedString& kKey, NiFixedString& kValue,
    float& fWeight) const
{
    if (!m_pkValueMap)
        return false;

    MetaDataValue* pkCurKeyData;
    if (!m_pkValueMap->GetAt(kKey, pkCurKeyData))
        return false;

    if (pkCurKeyData->eType == STRING || 
        pkCurKeyData->eType == ERROR_TYPE)
    {
        kValue = pkCurKeyData->kStringValue;
        fWeight = pkCurKeyData->fWeight;
        return true;
    }
    return false;
}

//--------------------------------------------------------------------------------------------------
bool NiMetaData::Get(const NiFixedString& kKey, NiInt32& iValue) const
{
    if (!m_pkValueMap)
        return false;

    NiMetaData::MetaDataValue* pkCurKeyData;
    if (!m_pkValueMap->GetAt(kKey, pkCurKeyData))
        return false;

    if (pkCurKeyData->eType == INTEGER ||
        pkCurKeyData->eType == INTEGER_BLENDED)
    {
        iValue = pkCurKeyData->iValue;
        return true;
    }
    return false;
}

//--------------------------------------------------------------------------------------------------
bool NiMetaData::Get(const NiFixedString& kKey, float& fValue) const
{
    if (!m_pkValueMap)
        return false;

    NiMetaData::MetaDataValue* pkCurKeyData;
    if (!m_pkValueMap->GetAt(kKey, pkCurKeyData))
        return false;

    if (pkCurKeyData->eType == FLOAT ||
        pkCurKeyData->eType == FLOAT_BLENDED)
    {
        fValue = pkCurKeyData->fValue;
        return true;
    }
    return false;
}

//--------------------------------------------------------------------------------------------------
bool NiMetaData::Get(const NiFixedString& kKey, NiFixedString& kValue) const
{
    if (!m_pkValueMap)
        return false;

    MetaDataValue* pkCurKeyData;
    if (!m_pkValueMap->GetAt(kKey, pkCurKeyData))
        return false;

    if (pkCurKeyData->eType == STRING || pkCurKeyData->eType == ERROR_TYPE)
    {
        kValue = pkCurKeyData->kStringValue;
        return true;
    }
    return false;
}

//--------------------------------------------------------------------------------------------------
bool NiMetaData::GetKeyType(const NiFixedString& kKey,
    KeyType& kKeyType) const
{
    MetaDataValue* pkCurKeyData;
    if (!m_pkValueMap->GetAt(kKey, pkCurKeyData))
        return false;

    kKeyType = pkCurKeyData->eType;
    return true;
}

//--------------------------------------------------------------------------------------------------
void NiMetaData::GetKeys(NiTObjectSet<NiFixedString>& kKeyNames) const
{
    if (!m_pkValueMap)
        return;

    const char* pcCurKey;
    NiMetaData::MetaDataValue* pkCurKeyData;

    // Iterate over the map and add all the key values:
    NiTMapIterator kIter = m_pkValueMap->GetFirstPos();
    while (kIter != NULL)
    {
        m_pkValueMap->GetNext(kIter, pcCurKey, pkCurKeyData);

        kKeyNames.Add(pcCurKey);
    }
}

//--------------------------------------------------------------------------------------------------
void NiMetaData::Save(TiXmlElement* pkRootElement) const
{
    if (!m_pkValueMap)
        return;

    const char* pcCurKey;
    NiMetaData::MetaDataValue* pkCurKeyData;

    // Loop through all the values an save them.
    NiTMapIterator kIter = m_pkValueMap->GetFirstPos();
    while (kIter != NULL)
    {
        m_pkValueMap->GetNext(kIter, pcCurKey, pkCurKeyData);

        //Save to the DOM
        TiXmlElement* pkMediaEntryElement =
            NiTerrainXMLHelpers::CreateElement("MetaEntry", pkRootElement);
        {
            KeyType eKeyType = pkCurKeyData->eType;

            NiTerrainXMLHelpers::WriteElement(pkMediaEntryElement,
                "Key", pcCurKey);

            NiTerrainXMLHelpers::WriteElement(pkMediaEntryElement,
                "DataType", (int) eKeyType);

            switch (eKeyType)
            {
                case INTEGER:
                    NiTerrainXMLHelpers::WriteElement(pkMediaEntryElement,
                        "Value", pkCurKeyData->iValue);
                    break;

                case INTEGER_BLENDED:
                    NiTerrainXMLHelpers::WriteElement(pkMediaEntryElement,
                        "Value", pkCurKeyData->iValue);
                    break;

                case FLOAT:
                    NiTerrainXMLHelpers::WriteElement(pkMediaEntryElement,
                        "Value", pkCurKeyData->fValue);
                    break;

                case FLOAT_BLENDED:
                    NiTerrainXMLHelpers::WriteElement(pkMediaEntryElement,
                        "Value", pkCurKeyData->fValue);
                    break;

                case STRING:
                    NiTerrainXMLHelpers::WriteElement(pkMediaEntryElement,
                        "Value", pkCurKeyData->kStringValue);
                    break;

                case ERROR_TYPE:
                    EE_FAIL("NiMetaData: Saving error type");
                    break;

                default:
                    break;
            }
        }
    }
}

//--------------------------------------------------------------------------------------------------
void NiMetaData::Load(const TiXmlElement* pkRootElement)
{
    const char* pcCurKey;

    // Make sure there is a value map to add to
    if (!m_pkValueMap)
        m_pkValueMap = NiNew NiTStringPointerMap<NiMetaData::MetaDataValue*>;

    // Loop over the DOM data and create the keys:
    const TiXmlElement* pkCurrentSection = pkRootElement->FirstChildElement();
    while (pkCurrentSection)
    {
        // This should be a metadata entry.
        NiTerrainXMLHelpers::ReadElement(pkCurrentSection, "Key", pcCurKey);

        int iDataType;
        NiTerrainXMLHelpers::ReadElement(pkCurrentSection, "DataType",
            (int&)iDataType);

        switch ((KeyType)iDataType)
        {
            case INTEGER:
            {
                NiInt32 iValue;
                NiTerrainXMLHelpers::ReadElement(pkCurrentSection, "Value",
                    iValue);
                Set(pcCurKey, iValue);
            }
            break;

            case INTEGER_BLENDED:
            {
                NiInt32 iValue;
                NiTerrainXMLHelpers::ReadElement(pkCurrentSection, "Value",
                    iValue);
                Set(pcCurKey, iValue, INTEGER_BLENDED);
            }
            break;

            case FLOAT:
            {
                float fValue;
                NiTerrainXMLHelpers::ReadElement(pkCurrentSection, "Value",
                    fValue);
                Set(pcCurKey, fValue);
            }
            break;

            case FLOAT_BLENDED:
            {
                float fValue;
                NiTerrainXMLHelpers::ReadElement(pkCurrentSection, "Value",
                    fValue);
                Set(pcCurKey, fValue, FLOAT_BLENDED);
            }
            break;

            case STRING:
            {
                const char* pcValue;
                NiTerrainXMLHelpers::ReadElement(pkCurrentSection, "Value",
                    pcValue);
                Set(pcCurKey, pcValue);
            }
            break;

            default:
                break;
        }
        pkCurrentSection = pkCurrentSection->NextSiblingElement();
    }
}

//--------------------------------------------------------------------------------------------------
NiMetaData& NiMetaData::operator= (const NiMetaData& kMetaData)
{
    if (this == &kMetaData)
        return *this;

    if (kMetaData.m_pkValueMap)
    {
        if (!m_pkValueMap)
        {
            m_pkValueMap =
                NiNew NiTStringPointerMap<NiMetaData::MetaDataValue*>;
        }
        else
        {
            RemoveAllKeys();
        }

        NiTMapIterator kIterator = kMetaData.m_pkValueMap->GetFirstPos();
        MetaDataValue* pkCurKeyData;
        const char* pcCurKey;

        while (kIterator)
        {
            kMetaData.m_pkValueMap->GetNext(kIterator, pcCurKey, pkCurKeyData);
            
            MetaDataValue* pkNewKeyData = NiNew MetaDataValue();
            *pkNewKeyData = *pkCurKeyData;

            // Value
            m_pkValueMap->SetAt(pcCurKey, pkNewKeyData);
        }
    }

    return *this;
}
