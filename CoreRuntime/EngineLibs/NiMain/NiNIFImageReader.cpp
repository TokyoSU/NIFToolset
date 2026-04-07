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

// Precompiled Header
#include "NiMainPCH.h"

#include <efd/File.h>
#include <NiSystem.h>
#include "NiNIFImageReader.h"
#include "NiPixelData.h"
#include "NiPixelFormat.h"

//--------------------------------------------------------------------------------------------------
NiNIFImageReader::NiNIFImageReader()
{
    /* */
}

//--------------------------------------------------------------------------------------------------
NiNIFImageReader::~NiNIFImageReader()
{
    /* */
}

//--------------------------------------------------------------------------------------------------
bool NiNIFImageReader::CanReadFile(const char* pcFileExtension) const
{
    if (!NiStricmp(pcFileExtension, ".nif"))
        return true;
    else
        return false;
}

//--------------------------------------------------------------------------------------------------
NiPixelData* NiNIFImageReader::ReadFile(efd::File& kIst, NiPixelData*)
{
    // cannot use the read header function - we can only read the entire
    // data file in this case

    // Since the reader is usually owned by a static image converter class,
    // we lock for safety.
    m_kReadCriticalSection.Lock();

    // special load - NIF file must contain a single NiPixelData object
    m_kStream.RemoveAllObjects();
    m_kStream.Load(&kIst);

    if (m_kStream.GetObjectCount() != 1)
    {
        m_kReadCriticalSection.Unlock();
        return NULL;
    }

    NiObject* pkPixelData = m_kStream.GetObjectAt(0);
    if (!NiIsKindOf(NiPixelData, pkPixelData))
    {
        m_kReadCriticalSection.Unlock();
        return NULL;
    }

    // All NiImageReaders are expected to return a raw NiPixelData pointer with no reference
    // count. However since NiStream is used to load NIF images the NiPixelData provided 
    // will have a reference count of one (Owned by the NiObjectPtr array in NiStream). 
    // Additionally we don't want to leave this method without clearing all objects in m_kStream
    // or the NiPixelData object won't be deleted until application shutdown. To address this 
    // situation we first manually increment the ref-count of the NiPixelData object and then
    // clear out all objects referenceed in m_kStream. The IncRefCount() ensures that pkPixelData
    // won't be deleted in RemoveAllObjects(). We then manually set the pixel data's ref count
    // to zero since the NiImageReader API states that ReadFile should return a 'dumb' pointer
    // with no reference count.
    // Note: Direct acccess to NiRefObject::m_uiRefCount is generally not allowed. 
    // NiNIFImageReader is declared a friend of NiRefObject to allow it it directly access
    // m_uiRefCount.
    pkPixelData->IncRefCount();
    m_kStream.RemoveAllObjects();
    pkPixelData->m_uiRefCount = 0;

    m_kReadCriticalSection.Unlock();
    return (NiPixelData*)pkPixelData;
}

//--------------------------------------------------------------------------------------------------
bool NiNIFImageReader::ReadHeader(efd::File& kIst,
    unsigned int& uiWidth, unsigned int& uiHeight,
    NiPixelFormat& kFormat, bool& bMipmap,
    unsigned int& uiFaces)
{
    // Read header is not cheaper than ReadFile in this case
    NiPixelDataPtr spData = ReadFile(kIst, NULL);

    if (spData)
    {
        uiWidth = spData->GetWidth();
        uiHeight = spData->GetHeight();
        kFormat = spData->GetPixelFormat();
        bMipmap = (spData->GetNumMipmapLevels() > 1) ? true : false;
        uiFaces = spData->GetNumFaces();

        return true;
    }
    else
    {
        return false;
    }
}

//--------------------------------------------------------------------------------------------------
