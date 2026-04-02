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

//--------------------------------------------------------------------------------------------------
namespace efd
{
//--------------------------------------------------------------------------------------------------
    template <>
    inline void BinaryStreamLoad<>(BinaryStream& is, efd::utf8string* pValue,
        unsigned int uiNumEls)
    {
        for (efd::UInt32 uiIndex = 0; uiIndex < uiNumEls; ++uiIndex)
        {
            // Read the size of the string
            efd::UInt32 uiSize = 0;
            efd::BinaryStreamLoad(is, &uiSize);

            if (uiSize)
            {
                // Read in that many bytes
                efd::UInt8* pucData = EE_ALLOC(efd::UInt8, uiSize);
                efd::BinaryStreamLoad(is, pucData, uiSize);

                // Assign the string at this point that value
                pValue->assign((const char*)pucData, CT_SIZE, uiSize);
                EE_FREE(pucData);
            }
            else
            {
                pValue->clear();
            }

            pValue++;
        }
    }

//--------------------------------------------------------------------------------------------------
    template <>
    inline void BinaryStreamSave<>(BinaryStream& os, const efd::utf8string* pValue, 
        unsigned int uiNumEls)
    {
        for (efd::UInt32 uiIndex = 0; uiIndex < uiNumEls; ++uiIndex)
        {
            // Write the size of the string
            efd::UInt32 uiSize = pValue->size();
            efd::BinaryStreamSave(os, &uiSize);

            if (uiSize)
            {
                // Write that many bytes
                efd::BinaryStreamSave(os, pValue->c_str(), uiSize);
            }

            pValue++;
        }
    }
//--------------------------------------------------------------------------------------------------
}
//--------------------------------------------------------------------------------------------------