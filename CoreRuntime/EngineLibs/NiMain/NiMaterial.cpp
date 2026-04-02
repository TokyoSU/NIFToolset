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

#include "NiMaterial.h"
#include "NiMaterialInstance.h"
#include <NiRWLock.h>


NiTFixedStringMap<NiMaterial*>* NiMaterial::ms_pkMaterials = NULL;
char NiMaterial::ms_acDefaultWorkingDirectory[NI_MAX_PATH];
NiRWLock NiMaterial::ms_kRWLock;


NiImplementRootRTTI(NiMaterial);

//--------------------------------------------------------------------------------------------------
NiMaterial::NiMaterial(const NiFixedString& kName) :
    m_kMaterialName(kName)
{
    EE_ASSERT(ms_pkMaterials);

    // Ensure name is nontrivial
    EE_ASSERT(kName.GetLength() != 0 &&
        "Material instances must have valid names!");

    // Ensure material is unique by name
    EE_ASSERT((GetMaterial(kName) == 0) &&
        "Derived NiMaterials must ensure in their Create function that their "
        "Material name is unique by calling GetMaterial!");

    ms_kRWLock.LockWrite();
    ms_pkMaterials->SetAt(m_kMaterialName, this);
    ms_kRWLock.UnlockWrite();
}

//--------------------------------------------------------------------------------------------------
NiMaterial::~NiMaterial()
{
    ms_kRWLock.LockWrite();
    EE_ASSERT(GetMaterial(m_kMaterialName) == this);

    ms_pkMaterials->RemoveAt(m_kMaterialName);
    ms_kRWLock.UnlockWrite();
}

//--------------------------------------------------------------------------------------------------
void NiMaterial::SetWorkingDirectory(const char*)
{
}

//--------------------------------------------------------------------------------------------------
NiMaterial* NiMaterial::GetMaterial(const NiFixedString& kName)
{
    EE_ASSERT(ms_pkMaterials);
    if (kName.GetLength() == 0)
        return NULL;

    NiMaterial* pkMaterial = 0;
    ms_kRWLock.LockRead();
    ms_pkMaterials->GetAt(kName, pkMaterial);
    ms_kRWLock.UnlockRead();

    return pkMaterial;
}

//--------------------------------------------------------------------------------------------------
unsigned int NiMaterial::GetMaterialCount()
{
    unsigned int uiCount=0;
    EE_ASSERT(ms_pkMaterials);
    ms_kRWLock.LockRead();
    uiCount = ms_pkMaterials->GetCount();
    ms_kRWLock.UnlockRead();
    return uiCount;
}

//--------------------------------------------------------------------------------------------------
void NiMaterial::BeginReadMaterialList()
{
    ms_kRWLock.LockRead();
}

//--------------------------------------------------------------------------------------------------
void NiMaterial::EndReadMaterialList()
{
    ms_kRWLock.UnlockRead();
}

//--------------------------------------------------------------------------------------------------
void NiMaterial::BeginWriteMaterialList()
{
    ms_kRWLock.LockWrite();
}

//--------------------------------------------------------------------------------------------------
void NiMaterial::EndWriteMaterialList()
{
    ms_kRWLock.UnlockWrite();
}

//--------------------------------------------------------------------------------------------------
NiMaterialIterator NiMaterial::GetFirstMaterialIter()
{
    return ms_pkMaterials->GetFirstPos();
}

//--------------------------------------------------------------------------------------------------
NiMaterial* NiMaterial::GetNextMaterial(NiMaterialIterator& kIter)
{
    if (kIter != NULL)
    {
         NiMaterial* pkMaterial = NULL;
         NiFixedString kName;
         ms_pkMaterials->GetNext(kIter, kName, pkMaterial);
         return pkMaterial;
    }
    return NULL;
}

//--------------------------------------------------------------------------------------------------
void NiMaterial::UnloadShadersForAllMaterials()
{
    ms_kRWLock.LockRead();
    NiTMapIterator kIter = ms_pkMaterials->GetFirstPos();

    NiMaterial* pkMaterial = NULL;
    NiFixedString kName;
    while (kIter)
    {
        ms_pkMaterials->GetNext(kIter, kName, pkMaterial);

        if (pkMaterial)
        {
            pkMaterial->UnloadShaders();
        }
    }
    ms_kRWLock.UnlockRead();
}

//--------------------------------------------------------------------------------------------------
void NiMaterial::SetWorkingDirectoryForAllMaterials(const char* pcWorkingDir)
{
    ms_kRWLock.LockRead();
    NiTMapIterator kIter = ms_pkMaterials->GetFirstPos();

    NiMaterial* pkMaterial = NULL;
    NiFixedString kName;
    while (kIter)
    {
        ms_pkMaterials->GetNext(kIter, kName, pkMaterial);

        if (pkMaterial)
        {
            pkMaterial->SetWorkingDirectory(pcWorkingDir);
        }
    }
    ms_kRWLock.UnlockRead();
}

//--------------------------------------------------------------------------------------------------
void NiMaterial::_SDMInit()
{
    ms_pkMaterials = NiNew NiTFixedStringMap<NiMaterial*>;
    SetDefaultWorkingDirectory(NULL);
}

//--------------------------------------------------------------------------------------------------
void NiMaterial::_SDMShutdown()
{
    NiDelete ms_pkMaterials;
    ms_pkMaterials = NULL;
}

//--------------------------------------------------------------------------------------------------
void NiMaterial::SetDefaultWorkingDirectory(const char* pcWorkingDir)
{
    if (pcWorkingDir == NULL)
    {
        ms_acDefaultWorkingDirectory[0] = '\0';
    }
    else
    {
        NiStrcpy(ms_acDefaultWorkingDirectory, NI_MAX_PATH, pcWorkingDir);
        NiPath::Standardize(ms_acDefaultWorkingDirectory);
        if (NiPath::IsRelative(ms_acDefaultWorkingDirectory))
        {
            EE_VERIFY(NiPath::ConvertToAbsolute(
                ms_acDefaultWorkingDirectory, NI_MAX_PATH));
        }
    }
}

//--------------------------------------------------------------------------------------------------
const char* NiMaterial::GetDefaultWorkingDirectory()
{
    if (ms_acDefaultWorkingDirectory[0] == '\0')
        return NULL;
    else
        return ms_acDefaultWorkingDirectory;
}

//--------------------------------------------------------------------------------------------------
