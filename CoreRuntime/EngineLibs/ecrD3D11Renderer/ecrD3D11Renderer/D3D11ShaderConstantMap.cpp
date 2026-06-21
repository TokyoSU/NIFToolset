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
#include "ecrD3D11RendererPCH.h"

#include "D3D11ShaderConstantMap.h"

#include "D3D11DataStream.h"
#include "D3D11Renderer.h"
#include "D3D11ShaderCore.h"
#include "D3D11ShaderFactory.h"
#include "D3D11Utility.h"

#include <NiAmbientLight.h>
#include <NiBooleanExtraData.h>
#include <NiColorExtraData.h>
#include <NiDirectionalLight.h>
#include <NiFloatExtraData.h>
#include <NiFloatsExtraData.h>
#include <NiFogProperty.h>
#include <NiRenderObject.h>
#include <NiIntegerExtraData.h>
#include <NiIntegersExtraData.h>
#include <NiMaterialProperty.h>
#include <NiMesh.h>
#include <NiPointLight.h>
#include <NiSCMExtraData.h>
#include <NiShaderAttributeDesc.h>
#include <NiShadowGenerator.h>
#include <NiShadowManager.h>
#include <NiShadowMap.h>
#include <NiSkinningMeshModifier.h>
#include <NiSpotLight.h>
#include <NiTextureEffect.h>
#include <NiTexturingProperty.h>

using namespace ecr;

efd::FixedString D3D11ShaderConstantMap::ms_globalConstantBufferString;

// -2 ^ 20. This number is used to create the light position for directional
// lights.
const efd::Float32 D3D11ShaderConstantMap::ms_dirLightDistance = -1048576.0f;

NiRenderer::RenderingPhase D3D11ShaderConstantMap::ms_phaseMappingArray[NiRenderer::PHASE_COUNT];

//------------------------------------------------------------------------------------------------
void D3D11ShaderConstantMap::_SDMInit()
{
    ms_globalConstantBufferString = "$Globals";
}

//------------------------------------------------------------------------------------------------
void D3D11ShaderConstantMap::_SDMShutdown()
{
    ms_globalConstantBufferString = NULL;
}

//------------------------------------------------------------------------------------------------
D3D11ShaderConstantMap::D3D11ShaderConstantMap(
    NiGPUProgram::ProgramType shaderType) :
    NiShaderConstantMap(shaderType),
    m_constantBufferCurrent(false),
    m_externalStream(false)
{
    ms_phaseMappingArray[0] = NiRenderer::PHASE_PER_SHADER;
    ms_phaseMappingArray[1] = NiRenderer::PHASE_PER_LIGHTSTATE;
    ms_phaseMappingArray[2] = NiRenderer::PHASE_PER_MESH;

    for (efd::UInt32 i = 0; i < NiRenderer::PHASE_COUNT; i++)
        m_phaseEntryArray[i].RemoveAll();

}

//------------------------------------------------------------------------------------------------
D3D11ShaderConstantMap::~D3D11ShaderConstantMap()
{
    // We need to release any entries which are global
    for (efd::UInt32 phase = 0; phase < NiRenderer::PHASE_COUNT; phase++)
    {

        for (efd::UInt32 i = 0; i < m_phaseEntryArray[phase].GetAllocatedSize(); i++)
        {
            NiShaderConstantMapEntryPtr spEntry = m_phaseEntryArray[phase].GetAt(i);
            if (spEntry && spEntry->IsGlobal())
            {
                NiFixedString key = spEntry->GetKey();
                D3D11ShaderFactory::ReleaseGlobalShaderConstant(key);
            }
        }

        m_phaseEntryArray[phase].RemoveAll();
    }
}

//------------------------------------------------------------------------------------------------
NiShaderError D3D11ShaderConstantMap::AddEntry(
    const efd::Char* pKey,
    efd::UInt32 flags, 
    efd::UInt32 extra, 
    efd::UInt32 shaderRegister,
    efd::UInt32 registerCount, 
    const efd::Char* pVariableName,
    efd::UInt32 dataSize, 
    efd::UInt32 dataStride,
    const void* pDataSource, 
    efd::Bool copyData)
{
    NiShaderError shaderError = NISHADERERR_OK;

    // See if the entry is in the list...
    NiShaderConstantMapEntry* pEntry = GetEntry(pKey);
    if (pEntry && pEntry->GetExtra() == extra &&
        pEntry->GetFlags() == flags)
    {
        // Was already in the list. Return an error
        shaderError = NISHADERERR_DUPLICATEENTRYKEY;
        return shaderError;
    }

    // Check what the entry is
    if (NiShaderConstantMapEntry::IsAttribute(flags))
    {
        shaderError = AddAttributeEntry(
            pKey, 
            flags, 
            extra,
            shaderRegister, 
            registerCount, 
            pVariableName, 
            dataSize,
            dataStride, 
            pDataSource, 
            copyData);
    }
    else if (NiShaderConstantMapEntry::IsConstant(flags))
    {
        shaderError = AddConstantEntry(
            pKey, 
            flags, 
            extra,
            shaderRegister, 
            registerCount, 
            pVariableName, 
            dataSize,
            dataStride, 
            pDataSource, 
            copyData);
    }
    else if (NiShaderConstantMapEntry::IsDefined(flags))
    {
        shaderError = AddPredefinedEntry(
            pKey, 
            extra,
            shaderRegister, 
            pVariableName);

        // Check for an already-encoded shader register value, because we must set the
        // encoded register count as well
        efd::UInt32 registerID;
        efd::UInt32 elements;
        efd::Bool isPacked;
        if (DecodePackedRegisterAndElement(
            shaderRegister,
            registerID,
            elements,
            isPacked))
        {
            EE_ASSERT(DecodePackedRegisterAndElement(
                registerCount,
                registerID,
                elements,
                isPacked));
            NiShaderConstantMapEntry* pEntry = GetEntry(pKey);
            pEntry->SetRegisterCount(registerCount);
        }
    }
    else if (NiShaderConstantMapEntry::IsGlobal(flags))
    {
        shaderError = AddGlobalEntry(
            pKey, 
            flags, 
            extra,
            shaderRegister, 
            registerCount, 
            pVariableName, 
            dataSize,
            dataStride, 
            pDataSource, 
            copyData);
    }
    else if (NiShaderConstantMapEntry::IsOperator(flags))
    {
        shaderError = AddOperatorEntry(
            pKey, 
            flags, 
            extra,
            shaderRegister, 
            registerCount, 
            pVariableName);
    }
    else if (NiShaderConstantMapEntry::IsObject(flags))
    {
        efd::UInt32 arrayElements;
        if ((dataSize == 0) || (dataStride == 0))
        {
            arrayElements = 1;
        }
        else
        {
            arrayElements = dataSize / dataStride;
            EE_ASSERT(dataSize % dataStride == 0);
        }

        shaderError = AddObjectEntry(
            pKey, 
            shaderRegister,
            pVariableName, 
            extra,
            NiShaderConstantMapEntry::GetObjectType(flags),
            arrayElements);

        // Check for an already-encoded shader register value, because we must set the
        // encoded register count as well
        efd::UInt32 registerID;
        efd::UInt32 elements;
        efd::Bool isPacked;
        if (DecodePackedRegisterAndElement(
            shaderRegister,
            registerID,
            elements,
            isPacked))
        {
            EE_ASSERT(DecodePackedRegisterAndElement(
                registerCount,
                registerID,
                elements,
                isPacked));
            NiShaderConstantMapEntry* pEntry = GetEntry(pKey);
            pEntry->SetRegisterCount(registerCount);
        }
    }
    else
    {
        D3D11Error::ReportWarning(
            "Attempt to add entry to  shader constant map %s in "
            __FUNCTION__
            " using invalid mapping flags %d.",
            GetName(),
            flags);
        shaderError = NISHADERERR_INVALIDMAPPING;
    }

    // Ensure the register count was properly recorded for those cases where
    // the register count is not passed through to the Add*Entry function.
    if (IsRegisterEncoded(shaderRegister) && shaderError == NISHADERERR_OK)
    {
        EE_ASSERT(IsRegisterEncoded(registerCount));
        NiShaderConstantMapEntry* pFoundEntry = NULL;

        efd::Bool found = false;
        for (efd::UInt32 phase = 0; phase < NiRenderer::PHASE_COUNT && !found; phase++)
        {
            const efd::UInt32 arraySize = m_phaseEntryArray[phase].GetSize();
            for (efd::UInt32 i = 0; i < arraySize && !found ; i++)
            {
                pEntry = m_phaseEntryArray[phase].GetAt(i);

                if (pEntry)
                {
                    // Entries do not necessarily have unique keys
                    // Verify encoded register, which will be unique
                    if (shaderRegister == pEntry->GetShaderRegister())
                    {
                        pFoundEntry = pEntry;
                        found = true;
                    }
                }
            }
        }
        EE_ASSERT(pFoundEntry && efd::Stricmp(pKey, pFoundEntry->GetKey()) == 0);

        pFoundEntry->SetRegisterCount(registerCount);
    }

    return shaderError;
}

//------------------------------------------------------------------------------------------------
NiShaderError D3D11ShaderConstantMap::AddPredefinedEntry(
    const efd::Char* pKey,
    efd::UInt32 extra, 
    efd::UInt32 shaderRegister,
    const efd::Char* pVariableName)
{
    NiShaderError shaderError = NISHADERERR_OK;

    NiShaderConstantMapEntry* pEntry = EE_NEW NiShaderConstantMapEntry();
    EE_ASSERT(pEntry);

    pEntry->SetKey(pKey);
    pEntry->SetExtra(extra);
    pEntry->SetShaderRegister(shaderRegister);
    pEntry->SetVariableName(pVariableName);

    // Set the flags to just the DEFINED type. The setup predefined call
    // will fill in the position masks
    pEntry->SetFlags(NiShaderConstantMapEntry::SCME_MAP_DEFINED);

    // Look-up and set the data.
    shaderError = SetupPredefinedEntry(pEntry);

    // Insert it!
    if (shaderError == NISHADERERR_OK)
    {
        shaderError = InsertEntry(pEntry);
    }
    else
    {
        D3D11Error::ReportWarning(
            "D3D11ShaderConstantMap::SetupPredefinedEntry failed for entry %s "
            "in shader constant map %s in "
            __FUNCTION__
            ".",
            pKey,
            GetName());
        EE_DELETE pEntry;
    }

    // Check for an already-encoded shader register value.
    efd::UInt32 registerID;
    efd::UInt32 elements;
    efd::Bool isPacked;
    if (DecodePackedRegisterAndElement(
        shaderRegister,
        registerID,
        elements,
        isPacked))
    {
        // Register count is not encoded because this function doesn't take it as a parameter.
        // If the shader register is encoded, then the function calling this must set the
        // encoded register count manually.
    }
    else if (shaderRegister != SCM_REGISTER_NONE)
    {
        // If it's not encoded, then we need encode it with the
        // buffer offset/size for the data to occupy.
        SetConstantBufferObsolete();
    }

    return shaderError;
}

//------------------------------------------------------------------------------------------------
NiShaderError D3D11ShaderConstantMap::AddAttributeEntry(
    const efd::Char* pKey,
    efd::UInt32 flags, 
    efd::UInt32 extra, 
    efd::UInt32 shaderRegister,
    efd::UInt32 registerCount, 
    const efd::Char* pVariableName,
    efd::UInt32 dataSize, 
    efd::UInt32 dataStride,
    const void* pDataSource, 
    efd::Bool copyData)
{
    NiShaderError shaderError = NISHADERERR_OK;

    NiShaderConstantMapEntry* pEntry = EE_NEW NiShaderConstantMapEntry();
    EE_ASSERT(pEntry);

    // Attributes will have to be retrieved from the geometry each time they
    // are set...
    pEntry->SetKey(pKey);

    // Make sure the flags are set correctly
    // Clear the old map
    flags &= ~NiShaderConstantMapEntry::SCME_MAP_MASK;
    // Set the confirmed one
    flags |= NiShaderConstantMapEntry::SCME_MAP_ATTRIBUTE;

    pEntry->SetFlags(flags);

    pEntry->SetExtra(extra);
    pEntry->SetShaderRegister(shaderRegister);
    pEntry->SetRegisterCount(registerCount);
    pEntry->SetVariableName(pVariableName);

    if (dataStride == 0)
        dataStride = dataSize;
    pEntry->SetData(dataSize, dataStride, (void*)pDataSource, copyData);

    // Insert it!
    shaderError = InsertEntry(pEntry);
    if (shaderError != NISHADERERR_OK)
    {
        D3D11Error::ReportWarning(
            "D3D11ShaderConstantMap::InsertEntry failed for entry %s "
            "in shader constant map %s in "
            __FUNCTION__
            ".",
            pKey,
            GetName());
        EE_DELETE pEntry;
        return shaderError;
    }

    // Check for an already-encoded shader register value.
    efd::UInt32 registerID;
    efd::UInt32 elements;
    efd::Bool isPacked;
    if (DecodePackedRegisterAndElement(
        shaderRegister,
        registerID,
        elements,
        isPacked))
    {
        // Ensure that the register count is also encoded.
        EE_ASSERT(DecodePackedRegisterAndElement(
            registerCount,
            registerID,
            elements,
            isPacked));
    }
    else if (shaderRegister != SCM_REGISTER_NONE)
    {
        // If it's not encoded, then we need encode it with the
        // buffer offset/size for the data to occupy.
        SetConstantBufferObsolete();
    }

    return shaderError;
}

//------------------------------------------------------------------------------------------------
NiShaderError D3D11ShaderConstantMap::AddConstantEntry(
    const efd::Char* pKey,
    efd::UInt32 flags, 
    efd::UInt32 extra, 
    efd::UInt32 shaderRegister,
    efd::UInt32 registerCount, 
    const efd::Char* pVariableName,
    efd::UInt32 dataSize, 
    efd::UInt32 dataStride,
    const void* pDataSource, 
    efd::Bool copyData)
{
    NiShaderError shaderError = NISHADERERR_OK;

    NiShaderConstantMapEntry* pEntry = EE_NEW NiShaderConstantMapEntry();
    EE_ASSERT(pEntry);

    pEntry->SetKey(pKey);
    pEntry->SetExtra(extra);
    pEntry->SetShaderRegister(shaderRegister);
    pEntry->SetRegisterCount(registerCount);
    pEntry->SetVariableName(pVariableName);

    // Clear the old map
    flags &= ~NiShaderConstantMapEntry::SCME_MAP_MASK;
    // Set the confirmed one
    flags |= NiShaderConstantMapEntry::SCME_MAP_CONSTANT;

    if (!IsRegisterEncoded(shaderRegister) &&
        shaderRegister != SCM_REGISTER_NONE)
    {
        // Store the constant's attribute or register size
        flags &= ~NiShaderConstantMapEntry::GetAttributeMask();
        if (dataSize == sizeof(efd::Float32))
        {
            flags |= NiShaderAttributeDesc::ATTRIB_TYPE_FLOAT;
        }
        if (dataSize == 2 * sizeof(efd::Float32))
        {
            flags |= NiShaderAttributeDesc::ATTRIB_TYPE_POINT2;
        }
        else if (dataSize == 3 * sizeof(efd::Float32))
        {
            flags |= NiShaderAttributeDesc::ATTRIB_TYPE_POINT3;
        }
        else if (dataSize == 4 * sizeof(efd::Float32))
        {
            flags |= NiShaderAttributeDesc::ATTRIB_TYPE_POINT4;
        }
        else if (dataSize == 8 * sizeof(efd::Float32))
        {
            flags |= NiShaderAttributeDesc::ATTRIB_TYPE_FLOAT8;
        }
        else if (dataSize == 9 * sizeof(efd::Float32))
        {
            flags |= NiShaderAttributeDesc::ATTRIB_TYPE_MATRIX3;
        }
        else if (dataSize == 12 * sizeof(efd::Float32))
        {
            flags |= NiShaderAttributeDesc::ATTRIB_TYPE_FLOAT12;
        }
        else if (dataSize == 16 * sizeof(efd::Float32))
        {
            flags |= NiShaderAttributeDesc::ATTRIB_TYPE_MATRIX4;
        }
        else
        {
            // Encode this odd size - assume fully packed data
            EE_ASSERT(dataSize % sizeof(efd::Float32) == 0);
            efd::UInt32 numFloats = dataSize / sizeof(efd::Float32);

            efd::UInt32 startRegister = 0;
            efd::UInt32 startElement = 0;
            efd::UInt32 encodedStartRegister;
            efd::UInt32 encodedRegisterCount;
            CalculatePackingEntry(
                startRegister, 
                startElement,
                numFloats / 4, 
                numFloats % 4, 
                encodedStartRegister,
                encodedRegisterCount, 
                true);
            pEntry->SetShaderRegister(encodedStartRegister);
            pEntry->SetRegisterCount(encodedRegisterCount);
            pEntry->SetVariableHookupValid(true);
        }
    }

    pEntry->SetFlags(flags);

    if (dataStride == 0)
        dataStride = dataSize;
    pEntry->SetData(dataSize, dataStride, (void*)pDataSource, copyData);

    // Insert it!
    shaderError = InsertEntry(pEntry);
    if (shaderError != NISHADERERR_OK)
    {
        D3D11Error::ReportWarning(
            "D3D11ShaderConstantMap::InsertEntry failed for entry %s "
            "in shader constant map %s in "
            __FUNCTION__
            ".",
            pKey,
            GetName());
        EE_DELETE pEntry;
        return shaderError;
    }

    // Check for an already-encoded shader register value.
    efd::UInt32 registerID;
    efd::UInt32 elements;
    efd::Bool isPacked;
    if (DecodePackedRegisterAndElement(
        shaderRegister,
        registerID,
        elements,
        isPacked))
    {
        // Ensure that the register count is also encoded.
        EE_ASSERT(DecodePackedRegisterAndElement(
            registerCount,
            registerID,
            elements,
            isPacked));
    }
    else if (shaderRegister != SCM_REGISTER_NONE)
    {
        // If it's not encoded, then we need encode it with the
        // buffer offset/size for the data to occupy.
        SetConstantBufferObsolete();
    }

    return shaderError;
}

//------------------------------------------------------------------------------------------------
NiShaderError D3D11ShaderConstantMap::AddGlobalEntry(
    const efd::Char* pKey,
    efd::UInt32 flags, 
    efd::UInt32 extra, 
    efd::UInt32 shaderRegister,
    efd::UInt32 registerCount, 
    const efd::Char* pVariableName,
    efd::UInt32 dataSize, 
    efd::UInt32 dataStride,
    const void* pDataSource, 
    efd::Bool copyData)
{
    NiShaderError shaderError = NISHADERERR_OK;

    NiShaderConstantMapEntry* pEntry = EE_NEW NiShaderConstantMapEntry();
    EE_ASSERT(pEntry);

    pEntry->SetKey(pKey);

    // Make sure the flags are set correctly
    // Clear the old map
    flags &= ~NiShaderConstantMapEntry::SCME_MAP_MASK;
    // Set the confirmed one
    flags |= NiShaderConstantMapEntry::SCME_MAP_GLOBAL;
    pEntry->SetFlags(flags);

    pEntry->SetExtra(extra);
    pEntry->SetShaderRegister(shaderRegister);
    pEntry->SetRegisterCount(registerCount);
    pEntry->SetVariableName(pVariableName);

    if (dataStride == 0)
        dataStride = dataSize;
    pEntry->SetData(dataSize, dataStride, (void*)pDataSource, copyData);

    // Insert it!
    shaderError = InsertEntry(pEntry);
    if (shaderError != NISHADERERR_OK)
    {
        D3D11Error::ReportWarning(
            "D3D11ShaderConstantMap::InsertEntry failed for entry %s "
            "in shader constant map %s in "
            __FUNCTION__
            ".",
            pKey,
            GetName());
        EE_DELETE pEntry;
        return shaderError;
    }
    else
    {
        NiShaderAttributeDesc::AttributeType attribType =
            NiShaderConstantMapEntry::GetAttributeType(flags);
        // Register the shader constant map entry
        if (!D3D11ShaderFactory::RegisterGlobalShaderConstant(
            pKey, 
            attribType, 
            dataSize, 
            pDataSource))
        {
            D3D11Error::ReportWarning(
                "D3D11ShaderFactory::RegisterGlobalShaderConstant failed for entry %s "
                "in shader constant map %s in "
                __FUNCTION__
                ".",
                pKey,
                GetName());
            EE_DELETE pEntry;
            return NISHADERERR_UNKNOWN;
        }
    }

    // Check for an already-encoded shader register value.
    efd::UInt32 registerID;
    efd::UInt32 elements;
    efd::Bool isPacked;
    if (DecodePackedRegisterAndElement(
        shaderRegister,
        registerID,
        elements,
        isPacked))
    {
        // Ensure that the register count is also encoded.
        EE_ASSERT(DecodePackedRegisterAndElement(
            registerCount,
            registerID,
            elements,
            isPacked));
    }
    else if (shaderRegister != SCM_REGISTER_NONE)
    {
        // If it's not encoded, then we need encode it with the
        // buffer offset/size for the data to occupy.
        SetConstantBufferObsolete();
    }
    return shaderError;
}

//------------------------------------------------------------------------------------------------
NiShaderError D3D11ShaderConstantMap::AddOperatorEntry(
    const efd::Char* pKey,
    efd::UInt32 flags, 
    efd::UInt32 extra, 
    efd::UInt32 shaderRegister,
    efd::UInt32 registerCount, 
    const efd::Char* pVariableName)
{
    NiShaderError shaderError = NISHADERERR_OK;

    NiShaderConstantMapEntry* pEntry = EE_NEW NiShaderConstantMapEntry();
    EE_ASSERT(pEntry);

    //
    pEntry->SetKey(pKey);

    // Make sure the flags are set correctly
    // Clear the old map
    flags &= ~NiShaderConstantMapEntry::SCME_MAP_MASK;
    // Set the confirmed one
    flags |= NiShaderConstantMapEntry::SCME_MAP_OPERATOR;
    pEntry->SetFlags(flags);

    pEntry->SetExtra(extra);
    pEntry->SetShaderRegister(shaderRegister);
    pEntry->SetRegisterCount(registerCount);
    pEntry->SetVariableName(pVariableName);

    // Insert it!
    shaderError = InsertEntry(pEntry);
    if (shaderError != NISHADERERR_OK)
    {
        D3D11Error::ReportWarning(
            "D3D11ShaderConstantMap::InsertEntry failed for entry %s "
            "in shader constant map %s in "
            __FUNCTION__
            ".",
            pKey,
            GetName());
        EE_DELETE pEntry;
    }

    // Check for an already-encoded shader register value.
    efd::UInt32 registerID;
    efd::UInt32 elements;
    efd::Bool isPacked;
    if (DecodePackedRegisterAndElement(
        shaderRegister,
        registerID,
        elements,
        isPacked))
    {
        // Ensure that the register count is also encoded.
        EE_ASSERT(DecodePackedRegisterAndElement(
            registerCount,
            registerID,
            elements,
            isPacked));
    }
    else if (shaderRegister != SCM_REGISTER_NONE)
    {
        // If it's not encoded, then we need encode it with the
        // buffer offset/size for the data to occupy.
        SetConstantBufferObsolete();
    }
    return shaderError;
}

//------------------------------------------------------------------------------------------------
NiShaderError D3D11ShaderConstantMap::AddObjectEntry(
    const efd::Char* pKey,
    efd::UInt32 shaderRegister, 
    const efd::Char* pVariableName,
    efd::UInt32 objectIndex, 
    NiShaderAttributeDesc::ObjectType objectType,
    efd::UInt32 count)
{
    NiShaderError shaderError = NISHADERERR_OK;

    NiShaderConstantMapEntry* pEntry = EE_NEW NiShaderConstantMapEntry();
    EE_ASSERT(pEntry);

    pEntry->SetKey(pKey);
    pEntry->SetShaderRegister(shaderRegister);
    pEntry->SetVariableName(pVariableName);
    pEntry->SetExtra(objectIndex);

    efd::UInt32 internalFlags =
        ((count << NiShaderConstantMapEntry::SCME_OBJECT_COUNT_SHIFT
        & NiShaderConstantMapEntry::SCME_OBJECT_COUNT_MASK));
    pEntry->SetInternal(internalFlags);

    // Set the flags to the object type.
    pEntry->SetFlags(NiShaderConstantMapEntry::SCME_MAP_OBJECT |
        NiShaderConstantMapEntry::GetObjectFlags(objectType));

    // Look-up and set the data.
    shaderError = SetupObjectEntry(pEntry);
    if (shaderError != NISHADERERR_OK)
    {
        D3D11Error::ReportWarning(
            "D3D11ShaderConstantMap::SetupObjectEntry failed for entry %s "
            "in shader constant map %s in "
            __FUNCTION__
            ".",
            pKey,
            GetName());
        EE_DELETE pEntry;
        return shaderError;
    }

    // Insert it!
    shaderError = InsertEntry(pEntry);
    if (shaderError != NISHADERERR_OK)
    {
        D3D11Error::ReportWarning(
            "D3D11ShaderConstantMap::InsertEntry failed for entry %s "
            "in shader constant map %s in "
            __FUNCTION__
            ".",
            pKey,
            GetName());
        EE_DELETE pEntry;
        return shaderError;
    }

    // Check for an already-encoded shader register value.
    efd::UInt32 registerID;
    efd::UInt32 elements;
    efd::Bool isPacked;
    if (DecodePackedRegisterAndElement(
        shaderRegister,
        registerID,
        elements,
        isPacked))
    {
        // Register count is not encoded because this function doesn't take it as a parameter.
        // If the shader register is encoded, then the function calling this must set the
        // encoded register count manually.
    }
    else if (shaderRegister != SCM_REGISTER_NONE)
    {
        // If it's not encoded, then we need encode it with the
        // buffer offset/size for the data to occupy.
        SetConstantBufferObsolete();
    }
    return shaderError;
}

//------------------------------------------------------------------------------------------------
NiShaderError D3D11ShaderConstantMap::RemoveEntry(const efd::Char* pKey)
{
    NiShaderError shaderError = NISHADERERR_OK;

    efd::UInt32 index = GetEntryIndex(pKey);

    if (index != INVALID_ENTRY_INDEX)
    {
        efd::UInt32 offset = 0;
        for (efd::UInt32 phase = 0; phase < NiRenderer::PHASE_COUNT; phase++)
        {
            const efd::UInt32 arraySize = m_phaseEntryArray[phase].GetSize();
            if (index < arraySize + offset)
            {
                NiShaderConstantMapEntry* pEntry =
                    m_phaseEntryArray[phase].GetAt(index - offset);

                if (pEntry && pEntry->IsGlobal())
                {
                    NiFixedString key = pEntry->GetKey();
                    D3D11ShaderFactory::ReleaseGlobalShaderConstant(key);
                }
                if (pEntry != NULL && pEntry->GetShaderRegister() != SCM_REGISTER_NONE)
                    SetConstantBufferObsolete();

                m_phaseEntryArray[phase].SetAt(index, 0);
            }

            offset += arraySize;
        }

    }
    else
    {
        D3D11Error::ReportWarning(
            "Entry %s not found in "
            __FUNCTION__
            ".",
            pKey);
        shaderError = NISHADERERR_ENTRYNOTFOUND;
    }

    return shaderError;
}

//------------------------------------------------------------------------------------------------
NiShaderConstantMapEntry* D3D11ShaderConstantMap::GetEntry(const efd::Char* pKey)
{
    NiShaderConstantMapEntry* pEntry = 0;
    efd::Bool found = false;

    for (efd::UInt32 phase = 0; phase < NiRenderer::PHASE_COUNT && !found; phase++)
    {
        const efd::UInt32 arraySize = m_phaseEntryArray[phase].GetSize();
        for (efd::UInt32 i = 0; i < arraySize && !found; i++)
        {
            pEntry = m_phaseEntryArray[phase].GetAt(i);
            if (pEntry)
            {
                if (efd::Stricmp(pKey, pEntry->GetKey()) == 0)
                {
                    found = true;
                }
            }
        }

        if (found)
            break;
    }

    if (!found)
        return 0;

    return pEntry;
}

//------------------------------------------------------------------------------------------------
NiShaderConstantMapEntry* D3D11ShaderConstantMap::GetEntryAtIndex(efd::UInt32 index)
{
    efd::UInt32 offset = 0;
    for (efd::UInt32 phase = 0; phase < NiRenderer::PHASE_COUNT; phase++)
    {
        const efd::UInt32 arraySize = m_phaseEntryArray[phase].GetEffectiveSize();
        if (index - offset < arraySize)
        {
            return m_phaseEntryArray[phase].GetAt(index - offset);
        }
        offset += arraySize;
    }

    return NULL;
}

//------------------------------------------------------------------------------------------------
NiShaderError D3D11ShaderConstantMap::UpdateShaderConstants(
    const NiRenderCallContext& callContext, 
    efd::Bool isGlobal)
{
    if (GetEntryCount() == 0)
        return NISHADERERR_OK;

    // New fast reject.
    // Do not lock/update this constant buffer if none of its entries
    // belong to the currently active render phase.
    if (!HasEntriesForActivePhases(callContext.m_uiActivePhases))
        return NISHADERERR_OK;

    if (!m_externalStream && IsConstantBufferCurrent() == false)
    {
        D3D11Error::ReportWarning(
            __FUNCTION__
            ": Cannot allocate constant buffer for shader constant map %s.",
            GetName());
        return NISHADERERR_UNKNOWN;
    }

    EE_ASSERT(m_spShaderConstantDataStream || GetEntryCount() == 0);

    return UpdateShaderConstantValues(callContext, isGlobal);
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11ShaderConstantMap::IsLinkable(const ConstantBufferDesc* pCBDesc) const
{
    if (pCBDesc == NULL)
        return false;

    // Verify the names match
    efd::FixedString mapName = GetName();
    if (!mapName.Exists())
        mapName = D3D11ShaderConstantMap::GetGlobalConstantBufferString();

    if (mapName != pCBDesc->m_bufferName)
        return false;

    // The ConstantBufferDesc contains a flag for each program type it will be providing the
    // buffer to. The ShaderConstantMap only needs to be one of those types, and the buffer can
    // be provided to all of the shader programs.
    if ((pCBDesc->m_shaderTypes & (1 << (efd::UInt32)GetProgramType())) == 0)
        return false;

    return true;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11ShaderConstantMap::LinkShaderConstantBuffer(
    const ConstantBufferDesc* pCBDesc)
{
    efd::FixedString tempName = m_name;
    if (tempName.GetLength() == 0)
        tempName = D3D11ShaderConstantMap::GetGlobalConstantBufferString();

    return CreateShaderConstantDataStream(pCBDesc);
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11ShaderConstantMap::CreateShaderConstantDataStream(
    const ConstantBufferDesc* pCBDesc)
{
    if (!m_externalStream && IsConstantBufferCurrent() == false)
    {
        NiDataStreamElementSet elementSet;
        UpdateConstantBufferPacking(pCBDesc, elementSet);
        efd::UInt32 stride = elementSet.GetStride();
        if (m_spShaderConstantDataStream)
        {
            if (m_spShaderConstantDataStream->GetSize() < stride)
            {
                m_spShaderConstantDataStream->Resize(stride);
            }
            m_constantBufferCurrent = true;
        }
        else if (stride > 0)
        {
            // Ensure size is a multiple of 16, which is a requirement of
            // constant buffer data
            stride = (stride + 15) & 0xFFFFFFF0;
            efd::UInt32 strideDiff = stride - elementSet.GetStride();
            EE_ASSERT(
                strideDiff % (D3D11_COMMONSHADER_CONSTANT_BUFFER_COMPONENT_BIT_COUNT / 8) == 0);
            if (strideDiff != 0)
            {
                // Pad if necessary
                InsertPadding(strideDiff, elementSet);
            }

            m_spShaderConstantDataStream = (D3D11DataStream*)NiDataStream::CreateDataStream(
                elementSet, 
                1,
                NiDataStream::ACCESS_GPU_READ |
                NiDataStream::ACCESS_CPU_WRITE_VOLATILE,
                NiDataStream::USAGE_SHADERCONSTANT);

            if (m_spShaderConstantDataStream == NULL)
            {
                D3D11Error::ReportWarning(
                    "NiDataStream::CreateDataStream failed for buffer %s "
                    "in shader constant map %s in "
                    __FUNCTION__
                    ".",
                    pCBDesc->m_bufferName,
                    GetName());
                return false;
            }

            m_constantBufferCurrent = (m_spShaderConstantDataStream != NULL);
        }
        else
        {
            // No need to create a buffer.
            m_constantBufferCurrent = true;
        }
    }
    EE_ASSERT(m_constantBufferCurrent == false ||
        m_spShaderConstantDataStream != NULL ||
        GetEntryCount() == 0);

    return m_constantBufferCurrent;
}

//------------------------------------------------------------------------------------------------
void D3D11ShaderConstantMap::ReleaseShaderConstantDataStream()
{
    m_spShaderConstantDataStream = NULL;
    m_externalStream = false;

    SetConstantBufferObsolete();
}

//------------------------------------------------------------------------------------------------
const efd::FixedString& D3D11ShaderConstantMap::GetGlobalConstantBufferString()
{
    return ms_globalConstantBufferString;
}

//------------------------------------------------------------------------------------------------
void D3D11ShaderConstantMap::SetupTextureTransformMatrix(
    XMMATRIX& result, 
    const efd::Matrix3* pTexMatrix, 
    efd::Bool transpose)
{
    if (pTexMatrix)
    {
        result.r[0] = XMVectorSet(pTexMatrix->GetEntry(0, 0), pTexMatrix->GetEntry(1, 0), 0.0f, 0.0f);
        result.r[1] = XMVectorSet(pTexMatrix->GetEntry(0, 1), pTexMatrix->GetEntry(1, 1), 0.0f, 0.0f);
        result.r[2] = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
        result.r[3] = XMVectorSet(pTexMatrix->GetEntry(0, 2), pTexMatrix->GetEntry(1, 2), 0.0f, 0.0f);

        if (transpose)
            result = XMMatrixTranspose(result);
    }
    else
    {
        result = XMMatrixIdentity();
    }
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11ShaderConstantMap::UpdateConstantBufferPacking(
    const ConstantBufferDesc* pCBDesc,
    NiDataStreamElementSet& dataStreamElements)
{
    const efd::UInt32 componentSize = D3D11_COMMONSHADER_CONSTANT_BUFFER_COMPONENT_BIT_COUNT / 8;
    EE_ASSERT (pCBDesc);

    // Iterate over variables in the order they appear in the
    // constant buffer
    const efd::UInt32 variableCount = pCBDesc->m_variableCount;
    for (efd::UInt32 i = 0; i < variableCount; i++)
    {
        const efd::FixedString& variableName = pCBDesc->m_nameArray[i];
        const efd::UInt32 variableSize = pCBDesc->m_sizeArray[i];
        const efd::UInt32 variableOffset = pCBDesc->m_startOffsetArray[i];
        const D3D11_SHADER_TYPE_DESC& variableType = pCBDesc->m_typeArray[i];

        // Find entry for this variable, if it exists
        NiShaderConstantMapEntry* pEntry = NULL;

        efd::Bool found = false;
        for (efd::UInt32 phase = 0; phase < NiRenderer::PHASE_COUNT && !found; phase++)
        {
            const efd::UInt32 arraySize = m_phaseEntryArray[phase].GetSize();
            for (efd::UInt32 i = 0; i < arraySize && !found ; i++)
            {

                NiShaderConstantMapEntry* pLocalEntry = m_phaseEntryArray[phase].GetAt(i);

                if (!pLocalEntry)
                    continue;
                efd::FixedString localVariableName = pLocalEntry->GetVariableName();
                if (!localVariableName.Exists())
                    localVariableName = pLocalEntry->GetKey();
                if (localVariableName == variableName)
                {
                    pEntry = pLocalEntry;
                    found = true;
                }
            }
        }

        efd::Bool packRegisters = true;
        efd::Bool isColumnMajor = true;
        NiDataStreamElement::Type elementType;
        efd::UInt32 numRows = 0;
        efd::UInt32 numColumns = 0;
        efd::UInt32 numElements = 1;
        // Examine types for support
        if (variableType.Type == D3D10_SVT_FLOAT)
        {
            elementType = NiDataStreamElement::T_FLOAT32;

            if (variableType.Class == D3D10_SVC_VECTOR)
            {
                numRows = variableType.Rows;
                numColumns = variableType.Columns;
            }
            // Check for matrices
            else if (variableType.Class == D3D10_SVC_MATRIX_ROWS)
            {
                isColumnMajor = false;
                numColumns = variableType.Columns;
                numRows = variableType.Rows;
                if (numColumns != 4)
                    packRegisters = false;
            }
            else if (variableType.Class == D3D10_SVC_MATRIX_COLUMNS)
            {
                numColumns = variableType.Rows;
                numRows = variableType.Columns;
                if (numRows != 4)
                    packRegisters = false;
            }
            else
            {
                numRows = 1;
                numColumns = 1;
            }
            // The Elements in the description are only non-zero for arrays
            if (variableType.Elements > 0)
                numElements = variableType.Elements;
            EE_ASSERT(numElements != 0);
        }
        else if (variableType.Type == D3D10_SVT_UINT || 
            variableType.Type == D3D10_SVT_BOOL)
        {
            elementType = NiDataStreamElement::T_UINT32;

            // Only scalars supported
            if (variableType.Class != D3D10_SVC_SCALAR)
            {
                D3D11Error::ReportWarning(
                    "Variable %s in shader constant map %s of type '%s'."
                    "Only 'float' values are supported in array, vector, and matrix "
                    "variables managed by NiShaderConstantMap.",
                    variableName,
                    GetName(),
                    (variableType.Type == D3D10_SVT_UINT ? "uint" : "bool"));
                return false;
            }
            numColumns = 1;
            numRows = 1;
            // The Elements in the description are only non-zero for arrays
            if (variableType.Elements > 0)
                numElements = variableType.Elements;
            EE_ASSERT(numElements != 0);
        }
        else if (variableType.Type == D3D10_SVT_INT)
        {
            elementType = NiDataStreamElement::T_INT32;

            // Only scalars supported
            if (variableType.Class != D3D10_SVC_SCALAR)
            {
                D3D11Error::ReportWarning(
                    "Variable %s in shader constant map %s of type 'int'."
                    "Only 'float' values are supported in array, vector, and matrix "
                    "variables managed by NiShaderConstantMap.",
                    variableName,
                    GetName());
                return false;
            }
            numColumns = 1;
            numRows = 1;
            // The Elements in the description are only non-zero for arrays
            if (variableType.Elements > 0)
                numElements = variableType.Elements;
            EE_ASSERT(numElements != 0);
        }
        else
        {
            // We don't support setting the other types, but they're
            // currently just ignored anyway. Don't even bother
            // with a warning.
            continue;
        }
        EE_ASSERT(numElements != 0);

        if (pEntry == NULL)
        {
            // Entry for this variable not found - insert appropriate
            // padding and continue on to next one

            // For arrays, variableSize will include the full size of the array
            InsertPadding(variableSize, dataStreamElements);
            continue;
        }

        efd::UInt32 startOffset = variableOffset / componentSize;
        efd::UInt32 startRegister = startOffset /
            D3D11_COMMONSHADER_CONSTANT_BUFFER_REGISTER_COMPONENTS;
        efd::UInt32 startElement = startOffset %
            D3D11_COMMONSHADER_CONSTANT_BUFFER_REGISTER_COMPONENTS;

        efd::UInt32 encodedStartRegister;
        EncodePackedRegisterAndElement(
            encodedStartRegister,
            startRegister, 
            startElement, 
            false);

        efd::UInt32 finalOffset = 
            (variableOffset + variableSize) / componentSize;
        efd::UInt32 finalRegister = finalOffset /
            D3D11_COMMONSHADER_CONSTANT_BUFFER_REGISTER_COMPONENTS;
        efd::UInt32 finalElement = finalOffset %
            D3D11_COMMONSHADER_CONSTANT_BUFFER_REGISTER_COMPONENTS;
        if (startElement > finalElement)
        {
            finalElement += D3D11_COMMONSHADER_CONSTANT_BUFFER_REGISTER_COMPONENTS;
            finalRegister--;
        }
        EE_ASSERT(finalElement >= startElement);
        EE_ASSERT(finalRegister >= startRegister);

        efd::UInt32 encodedRegisterCount;
        EncodePackedRegisterAndElement(encodedRegisterCount,
            finalRegister - startRegister,
            finalElement - startElement, 
            packRegisters);

        pEntry->SetShaderRegister(encodedStartRegister);
        pEntry->SetRegisterCount(encodedRegisterCount);
        pEntry->SetColumnMajor(isColumnMajor);
        pEntry->SetVariableHookupValid(true);

        // Add padding if necessary
        EE_ASSERT(variableOffset >= dataStreamElements.GetStride());
        efd::UInt32 padding = variableOffset - dataStreamElements.GetStride();
        if (padding != 0)
        {
            InsertPadding(padding, dataStreamElements);
        }

        // Add new data elements
        efd::UInt32 numComponents = variableSize / componentSize;
        if (numColumns == 0)
            numColumns = numComponents;
        NiDataStreamElement::Format format = NiDataStreamElement::GetPredefinedFormat(
            elementType, 
            (efd::UInt8)numColumns, 
            false);
        padding = (D3D11_COMMONSHADER_CONSTANT_BUFFER_REGISTER_COMPONENTS - numColumns) * 
            componentSize;

        EE_ASSERT(numRows > 0);
        efd::UInt32 totalRows = numRows * numElements;
        // Fill in all rows before last row, including padding if necessary
        for (efd::UInt32 j = 0; j < totalRows - 1; j++)
        {
            dataStreamElements.AddElement(format);
            if (padding != 0)
                InsertPadding(padding, dataStreamElements);
        }
        // Fill in last row
        dataStreamElements.AddElement(format);
    }

    return true;
}

//------------------------------------------------------------------------------------------------
void D3D11ShaderConstantMap::SetShaderConstantDataStream(
    D3D11DataStream* pStream)
{
    m_spShaderConstantDataStream = pStream;
    m_externalStream = true;
}

//------------------------------------------------------------------------------------------------
efd::Bool ecr::D3D11ShaderConstantMap::HasEntriesForActivePhases(efd::UInt32 uiActivePhases) const
{
    for (efd::UInt32 phase = 0; phase < NiRenderer::PHASE_COUNT; ++phase)
    {
        if ((ms_phaseMappingArray[phase] & uiActivePhases) == 0)
            continue;
        if (m_phaseEntryArray[phase].GetEffectiveSize() > 0)
            return true;
    }
    return false;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11ShaderConstantMap::EncodePackedRegisterAndElement(
    efd::UInt32& encodedValue, 
    efd::UInt32 registerID,
    efd::UInt32 element, 
    efd::Bool packedRegisters)
{
    encodedValue = SCM_REGISTER_ENCODING |
        (element << SCM_REGISTER_ELEMENT_SHIFT) |
        (registerID << SCM_REGISTER_SHIFT) |
        (packedRegisters ? SCM_REGISTER_PACKED_BIT : 0);
    return true;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11ShaderConstantMap::DecodePackedRegisterAndElement(
    efd::UInt32 encodedValue, 
    efd::UInt32& registerID,
    efd::UInt32& element, 
    efd::Bool& packedRegisters)
{
    if (!IsRegisterEncoded(encodedValue))
        return false;

    registerID = (encodedValue & SCM_REGISTER_MASK) >> SCM_REGISTER_SHIFT;
    element = (encodedValue & SCM_REGISTER_ELEMENT_MASK) >> SCM_REGISTER_ELEMENT_SHIFT;
    packedRegisters = ((encodedValue & SCM_REGISTER_PACKED_BIT) != 0);

    return true;
}

//------------------------------------------------------------------------------------------------
efd::UInt32 D3D11ShaderConstantMap::GetEntryIndex(const efd::Char* pKey)
{
    NiShaderConstantMapEntry* pEntry;
    efd::UInt32 offset = 0;
    for (efd::UInt32 phase = 0; phase < NiRenderer::PHASE_COUNT; phase++)
    {
        const efd::UInt32 arraySize = m_phaseEntryArray[phase].GetSize();
        for (efd::UInt32 i = 0; i < arraySize; i++)
        {
            pEntry = m_phaseEntryArray[phase].GetAt(i);
            if (pEntry)
            {
                if (efd::Stricmp(pKey, pEntry->GetKey()) == 0)
                {
                    return i + offset;
                }
            }
        }
        offset += arraySize;
    }

    return (efd::UInt32) INVALID_ENTRY_INDEX;
}

//------------------------------------------------------------------------------------------------
NiShaderError D3D11ShaderConstantMap::InsertEntry(
    NiShaderConstantMapEntry* pEntry)
{
    NiShaderError shaderError = NISHADERERR_OK;
    NiRenderer::RenderingPhase phase = NiShaderConstantMap::GetPhase(pEntry);

    efd::UInt32 phaseIndex = GetPhaseIndex(phase);
    if (m_phaseEntryArray[phaseIndex].AddFirstEmpty(pEntry) == 0xffffffff)
    {
        D3D11Error::ReportWarning(
            "Error in "
            __FUNCTION__
            " adding entry %s to shader constant map %s.",
            pEntry->GetKey(),
            GetName());

        shaderError = NISHADERERR_ENTRYNOTADDED;
    }

    return shaderError;
}

//------------------------------------------------------------------------------------------------
NiShaderError D3D11ShaderConstantMap::SetupPredefinedEntry(NiShaderConstantMapEntry* pEntry)
{
    // Look up the key.
    efd::Bool found = false;

    efd::UInt32 registerCount = 4;
    efd::UInt32 mappingID;

    if (!LookUpPredefinedMapping(pEntry->GetKey(), mappingID, registerCount))
        return NISHADERERR_INVALIDMAPPING;

    NiShaderAttributeDesc::AttributeType attribType =
        LookUpPredefinedMappingType(mappingID, registerCount);
    efd::UInt32 flags = pEntry->GetFlags();
    flags &= ~NiShaderConstantMapEntry::GetAttributeMask();
    flags |= NiShaderConstantMapEntry::GetAttributeFlags(attribType);
    pEntry->SetFlags(flags);

    if (mappingID != 0)
    {
        // FOUND IT!
        pEntry->SetInternal(mappingID);

        if (mappingID == SCM_DEF_CONSTS_TAYLOR_SIN)
        {
            efd::Float32 taylorSin[] = { 1.0f, -0.16161616f, 0.0083333f, -0.00019841f };
            pEntry->SetData(sizeof(taylorSin), sizeof(taylorSin), (void*)taylorSin, true);
        }
        else if (mappingID == SCM_DEF_CONSTS_TAYLOR_COS)
        {
            efd::Float32 taylorCos[] = { -0.5f, -0.041666666f, -0.0013888889f, 0.000024801587f };
            pEntry->SetData(sizeof(taylorCos), sizeof(taylorCos), (void*)taylorCos, true);
        }

        found = true;
    }

    if (!found)
        return NISHADERERR_INVALIDMAPPING;

    return NISHADERERR_OK;
}

//------------------------------------------------------------------------------------------------
NiShaderError D3D11ShaderConstantMap::SetupObjectEntry(NiShaderConstantMapEntry* pEntry)
{
    efd::UInt32 registerCount = 0;
    efd::UInt32 mappingID;
    if (!LookUpObjectMapping(pEntry->GetKey(), mappingID))
    {
        return NISHADERERR_INVALIDMAPPING;
    }

    efd::UInt32 floatCount;
    NiShaderAttributeDesc::AttributeType attribType =
        LookUpObjectMappingType(mappingID, registerCount, floatCount);
    efd::UInt32 flags = pEntry->GetFlags();
    flags &= ~NiShaderConstantMapEntry::GetAttributeMask();
    flags |= NiShaderConstantMapEntry::GetAttributeFlags(attribType);
    pEntry->SetFlags(flags);

    if (mappingID != NiShaderConstantMap::SCM_OBJ_INVALID)
    {
        efd::UInt32 internalFlags = pEntry->GetInternal() &
            ~NiShaderConstantMapEntry::SCME_OBJECT_MAP_MASK;
        internalFlags |= (mappingID <<
            NiShaderConstantMapEntry::SCME_OBJECT_MAP_SHIFT) &
            NiShaderConstantMapEntry::SCME_OBJECT_MAP_MASK;
        pEntry->SetInternal(internalFlags);
        // No need to set register count, since the attribute type has been
        // stored in the flags.
        //pkEntry->SetRegisterCount(registerCount);
    }
    else
    {
        return NISHADERERR_INVALIDMAPPING;
    }

    return NISHADERERR_OK;
}

//------------------------------------------------------------------------------------------------
NiShaderError D3D11ShaderConstantMap::UpdateShaderConstantValues(
    const NiRenderCallContext& callContext, 
    efd::Bool isGlobal)
{
    if (GetEntryCount() == 0)
        return NISHADERERR_OK;

    if (m_spShaderConstantDataStream == NULL)
    {
        D3D11Error::ReportWarning(
            __FUNCTION__
            "failed in shader constant map %s because of a NULL data stream",
            GetName());
        return NISHADERERR_UNKNOWN;
    }

    void* pWriteData = m_spShaderConstantDataStream->Lock(NiDataStream::LOCK_WRITE);
    if (pWriteData == NULL)
    {
        D3D11Error::ReportWarning(
            __FUNCTION__
            "failed in shader constant map %s locking the data stream",
            GetName());
        return NISHADERERR_UNKNOWN;
    }

    efd::Bool success = true;

    // Grab the NiSCMExtraData object which basically holds cached
    // values for extra data pointers so we don't have to call strcmp
    // too much.
    NiSCMExtraData* pShaderData = NULL;
// DT33847: Re-enable NiSCMExtraData usage
#if 0
    NiRenderObject* pMesh = callContext.m_pkMesh;
    if (pMesh)
    {
        pShaderData = (NiSCMExtraData*)pMesh->GetExtraData(
            D3D11ShaderCore::GetEmergentShaderMapName());
    }
#endif
    NiShaderConstantMapEntry* pEntry = NULL;

    for (efd::UInt32 phase = 0; phase < NiRenderer::PHASE_COUNT; phase++)
    {
        if ((ms_phaseMappingArray[phase] & callContext.m_uiActivePhases) == 0)
            continue;

        const efd::UInt32 numEntries = m_phaseEntryArray[phase].GetEffectiveSize();

        efd::UInt32 i = 0;
        while (i < numEntries)
        {
            pEntry = m_phaseEntryArray[phase].GetAt(i++);
            if (!pEntry)
                continue;

            // Allow for skipping of entries if the have a -1 in the
            // shader register.
            if (pEntry->GetShaderRegister() == SCM_REGISTER_NONE)
                continue;

            // If the shader register is > max constants, skip that too
            //        if (pEntry->GetShaderRegister() >= ...)
            //            continue;

            NiShaderError shaderError = NISHADERERR_OK;
            if (pEntry->IsDefined())
            {
                shaderError = UpdateDefinedConstantValue(pWriteData, pEntry, callContext);
            }
            else if (pEntry->IsConstant())
            {
                shaderError = UpdateConstantConstantValue(pWriteData, pEntry, callContext);
            }
            else if (pEntry->IsAttribute())
            {
                shaderError = UpdateAttributeConstantValue(
                    pWriteData, 
                    pEntry, 
                    callContext,
                    isGlobal, 
                    pShaderData);
            }
            else if (pEntry->IsGlobal())
            {
                shaderError = UpdateGlobalConstantValue(pWriteData, pEntry, callContext);
            }
            else if (pEntry->IsOperator())
            {
                shaderError = UpdateOperatorConstantValue(
                    pWriteData, 
                    pEntry, 
                    callContext,
                    isGlobal, 
                    pShaderData);
            }
            else if (pEntry->IsObject())
            {
                shaderError = UpdateObjectConstantValue(pWriteData, pEntry, callContext);
            }
            else
            {
                shaderError = NISHADERERR_INVALIDMAPPING;
            }
            if (shaderError != NISHADERERR_OK)
            {
                switch (shaderError)
                {
                case NISHADERERR_INVALIDMAPPING:
                    D3D11Error::ReportError(
                        D3D11Error::D3D11ERROR_SHADER_CONSTANT_MAPPING_FAILED,
                        __FUNCTION__ 
                        " - Constant %s has invalid mapping 0x%08X.\n",
                        (const efd::Char*)pEntry->GetKey(), 
                        (pEntry->GetFlags() & NiShaderConstantMapEntry::SCME_MAP_MASK));
                    break;
                case NISHADERERR_SETCONSTANTFAILED:
                    D3D11Error::ReportError(
                        D3D11Error::D3D11ERROR_SHADER_CONSTANT_MAPPING_FAILED,
                        __FUNCTION__ 
                        " - Constant %s failed to be set.\n",
                        (const efd::Char*)pEntry->GetKey());
                    break;
                case NISHADERERR_ENTRYNOTFOUND:
                    D3D11Error::ReportError(
                        D3D11Error::D3D11ERROR_SHADER_CONSTANT_MAPPING_FAILED,
                        __FUNCTION__ 
                        " - Constant %s has unknown entry 0x%08X.\n",
                        (const efd::Char*)pEntry->GetKey(), 
                        pEntry->GetInternal());
                    break;
                case NISHADERERR_DYNEFFECTNOTFOUND:
                    if (NiShaderErrorSettings::GetAllowDynEffectNotFound())
                    {
                        D3D11Error::ReportError(
                            D3D11Error::D3D11ERROR_SHADER_CONSTANT_MAPPING_FAILED,
                            __FUNCTION__ 
                            " - Constant %s references a nonexistent "
                            "NiDynamicEffect object. Default values used.\n",
                            (const efd::Char*)pEntry->GetKey());
                    }
                    break;
                default:
                    D3D11Error::ReportError(
                        D3D11Error::D3D11ERROR_SHADER_CONSTANT_MAPPING_FAILED,
                        __FUNCTION__ 
                        " - Constant %s failed to be set with unknown error\n",
                        (const efd::Char*)pEntry->GetKey());
                    break;
                }
                success = false;
            }
        }
    }

    m_spShaderConstantDataStream->Unlock(NiDataStream::LOCK_WRITE);

    return success ? NISHADERERR_OK : NISHADERERR_UNKNOWN;
}

//------------------------------------------------------------------------------------------------
NiShaderError D3D11ShaderConstantMap::UpdateDefinedConstantValue(
    void* pShaderConstantBuffer, 
    NiShaderConstantMapEntry* pEntry,
    const NiRenderCallContext& callContext)
{
    efd::UInt32 encodedShaderRegister = pEntry->GetShaderRegister();
    efd::UInt32 encodedRegisterCount = pEntry->GetRegisterCount();

    efd::UInt32 startRegister = 0;
    efd::UInt32 startElement = 0;
    efd::UInt32 registerCount = 0;
    efd::UInt32 elementCount = 0;
    efd::Bool packRegisters = false;

    efd::Bool valid1 = DecodePackedRegisterAndElement(
        encodedShaderRegister,
        startRegister, 
        startElement, 
        packRegisters);
    EE_ASSERT(packRegisters == false);
    efd::Bool valid2 = DecodePackedRegisterAndElement(
        encodedRegisterCount,
        registerCount, 
        elementCount, 
        packRegisters);
    if (!valid1 || !valid2)
    {
        D3D11Error::ReportWarning(
            "Entry %s in shader constant map %s failed in "
            __FUNCTION__
            " because the shader register was incorrectly encoded in the entry.",
            pEntry->GetKey(),
            GetName());

        // UpdateConstantBufferPacking needs to be called.
        return NISHADERERR_UNKNOWN;
    }

    XMMATRIX tempMatrix;
    efd::UInt32 dataSize = 0;
    efd::UInt32 dataStride = 0;
    const void* pDataSource = ObtainDefinedConstantValue(
        pEntry, 
        callContext, 
        dataSize,
        dataStride,
        tempMatrix);
    if (pDataSource == NULL)
    {
        D3D11Error::ReportWarning(
            "Entry %s in shader constant map %s failed in "
            __FUNCTION__
            " because the defined value for the entry could not be found.",
            pEntry->GetKey(),
            GetName());

        return NISHADERERR_UNKNOWN;
    }

    efd::UInt32 arrayLength = 1;
    if (pEntry->IsArray())
    {
        EE_ASSERT(dataStride != 0);
        arrayLength = dataSize / dataStride;
        // The register is encoded with the total size of all individual entries in an array
    }

    // Bone reordering may take place - pass in reorder array!
    const efd::UInt16* pBonePalette = NULL;
    NiDataStream* pBonePaletteStream = NULL;
    if (pEntry->GetInternal() == SCM_DEF_SKINBONE_MATRIX_3)
    {
        NiMesh* pMesh = NiVerifyStaticCast(NiMesh, callContext.m_pkMesh);
        if (pMesh == NULL)
        {
            D3D11Error::ReportWarning(
                "Entry %s in shader constant map %s failed in "
                __FUNCTION__
                " because the mesh is missing, likely because this is a compute-only pass.",
                pEntry->GetKey(),
                GetName());

            return NISHADERERR_UNKNOWN;
        }

        // Special-case bones, since they aren't counted as an array
        NiSkinningMeshModifier* pSkin = NiGetModifier(NiSkinningMeshModifier, pMesh);

        if (pSkin)
        {
            NiDataStreamRef* pBonePaletteRef =
                pMesh->FindStreamRef(NiCommonSemantics::BONE_PALETTE());
            EE_ASSERT(pBonePaletteRef);

            pBonePaletteStream = pBonePaletteRef->GetDataStream();
            EE_ASSERT(pBonePaletteStream != NULL);

            const NiDataStream::Region& region = pBonePaletteStream->GetRegion(
                callContext.m_uiSubmesh);
#if defined(EE_ASSERTS_ARE_ENABLED)
            // The palette indices are assumed to be 16 bit ints,
            // packed in their own stream
            const NiDataStreamElement& elem = pBonePaletteStream->GetElementDescAt(0);
            EE_ASSERT(elem.GetFormat() == NiDataStreamElement::F_UINT16_1);
            EE_ASSERT(pBonePaletteStream->GetStride() == sizeof(efd::UInt16));
            EE_ASSERT(elem.GetOffset() == 0);
#endif
            const efd::UInt32 submeshBoneCount = region.GetRange();

            // Get a pointer to the current region's bone palette
            pBonePalette = (efd::UInt16*)pBonePaletteStream->LockRegion(
                callContext.m_uiSubmesh,
                NiDataStream::LOCK_READ);
            EE_ASSERT(pBonePalette != NULL);

            // Use lesser of encoded bone count, submesh bone count, and available data
            efd::UInt32 encodedBoneCount = (pEntry->GetExtra() & 0xffff0000) >> 16;
            EE_ASSERT(encodedBoneCount < EE_SINT32_MAX && submeshBoneCount < EE_SINT32_MAX);
            efd::UInt32 availableBoneCount = dataSize / dataStride;
            arrayLength = efd::Min(
                efd::Min((efd::SInt32)encodedBoneCount, (efd::SInt32)submeshBoneCount),
                (efd::SInt32)availableBoneCount);

            // Force register count to 3
            registerCount = 3 * arrayLength;
        }
    }

    NiShaderError shaderError = FillShaderConstantBuffer(
        pShaderConstantBuffer,
        pDataSource, 
        dataSize,
        dataStride,
        startRegister, 
        startElement,
        registerCount, 
        elementCount, 
        arrayLength, 
        packRegisters,
        pBonePalette);

    if (pBonePalette)
    {
        EE_ASSERT(pBonePaletteStream);
        pBonePaletteStream->Unlock(NiDataStream::LOCK_READ);
    }

    return shaderError;
}

//------------------------------------------------------------------------------------------------
NiShaderError D3D11ShaderConstantMap::UpdateConstantConstantValue(
    void* pShaderConstantBuffer, 
    NiShaderConstantMapEntry* pEntry,
    const NiRenderCallContext& callContext)
{
    XMMATRIX tempMatrix;
    efd::UInt32 dataSize = 0;
    efd::UInt32 dataStride = 0;
    const void* pDataSource = ObtainConstantConstantValue(
        pEntry, 
        callContext, 
        dataSize,
        dataStride,
        tempMatrix);
    if (pDataSource == NULL)
    {
        D3D11Error::ReportWarning(
            "Entry %s in shader constant map %s failed in "
            __FUNCTION__
            " because the constant value for the entry could not be found.",
            pEntry->GetKey(),
            GetName());
        return NISHADERERR_UNKNOWN;
    }

    efd::UInt32 encodedShaderRegister = pEntry->GetShaderRegister();
    efd::UInt32 encodedRegisterCount = pEntry->GetRegisterCount();

    efd::UInt32 startRegister = 0;
    efd::UInt32 startElement = 0;
    efd::UInt32 registerCount = 0;
    efd::UInt32 elementCount = 0;
    efd::Bool packRegisters = false;

    efd::Bool valid1 = DecodePackedRegisterAndElement(
        encodedShaderRegister,
        startRegister, 
        startElement, 
        packRegisters);
    EE_ASSERT(packRegisters == false);
    efd::Bool valid2 = DecodePackedRegisterAndElement(
        encodedRegisterCount,
        registerCount, 
        elementCount, 
        packRegisters);
    if (!valid1 || !valid2)
    {
        D3D11Error::ReportWarning(
            "Entry %s in shader constant map %s failed in "
            __FUNCTION__
            " because the shader register was incorrectly encoded in the entry.",
            pEntry->GetKey(),
            GetName());

        // UpdateConstantBufferPacking needs to be called.
        return NISHADERERR_UNKNOWN;
    }

    efd::UInt32 arrayLength = 1;
    if (pEntry->IsArray())
    {
        EE_ASSERT(dataStride != 0);
        arrayLength = dataSize / dataStride;
        // The register is encoded with the total size of all individual entries in an array
    }

    return FillShaderConstantBuffer(
        pShaderConstantBuffer,
        pDataSource, 
        dataSize,
        dataStride,
        startRegister, 
        startElement,
        registerCount, 
        elementCount, 
        arrayLength, 
        packRegisters,
        NULL);
}

//------------------------------------------------------------------------------------------------
NiShaderError D3D11ShaderConstantMap::UpdateAttributeConstantValue(
    void* pShaderConstantBuffer, 
    NiShaderConstantMapEntry* pEntry,
    const NiRenderCallContext& callContext, 
    efd::Bool isGlobal, 
    NiExtraData* pExtraData)
{
    if (callContext.m_pkMesh == NULL)
        return NISHADERERR_UNKNOWN;

    // Grab the attribute from the geometry and set it

    XMMATRIX tempMatrix;
    efd::UInt32 dataSize = 0;
    efd::UInt32 dataStride = 0;
    const void* pDataSource = ObtainAttributeConstantValue(
        pEntry, 
        callContext,
        isGlobal, 
        pExtraData, 
        dataSize,
        dataStride,
        tempMatrix);
    if (pDataSource == NULL)
    {
        if (pEntry->IsTexture())
        {
            // Textures are not set here
            return NISHADERERR_OK;
        }

        D3D11Error::ReportWarning(
            "Entry %s in shader constant map %s failed in "
            __FUNCTION__
            " because the attribute value for the entry could not be found.",
            pEntry->GetKey(),
            GetName());
        return NISHADERERR_UNKNOWN;
    }

    efd::UInt32 encodedShaderRegister = pEntry->GetShaderRegister();
    efd::UInt32 encodedRegisterCount = pEntry->GetRegisterCount();

    efd::UInt32 startRegister = 0;
    efd::UInt32 startElement = 0;
    efd::UInt32 registerCount = 0;
    efd::UInt32 elementCount = 0;
    efd::Bool packRegisters = true;

    efd::Bool valid1 = DecodePackedRegisterAndElement(
        encodedShaderRegister,
        startRegister, 
        startElement, 
        packRegisters);
    EE_ASSERT(packRegisters == false);
    efd::Bool valid2 = DecodePackedRegisterAndElement(
        encodedRegisterCount,
        registerCount, 
        elementCount, 
        packRegisters);
    if (!valid1 || !valid2)
    {
        D3D11Error::ReportWarning(
            "Entry %s in shader constant map %s failed in "
            __FUNCTION__
            " because the shader register was incorrectly encoded in the entry.",
            pEntry->GetKey(),
            GetName());

        // UpdateConstantBufferPacking needs to be called.
        return NISHADERERR_UNKNOWN;
    }

    efd::UInt32 arrayLength = 1;
    if (pEntry->IsArray())
    {
        EE_ASSERT(dataStride != 0);
        arrayLength = dataSize / dataStride;
        // The register is encoded with the total size of all individual entries in an array
    }

    return FillShaderConstantBuffer(
        pShaderConstantBuffer,
        pDataSource, 
        dataSize,
        dataStride,
        startRegister, 
        startElement, 
        registerCount,
        elementCount, 
        arrayLength, 
        packRegisters, 
        NULL);
}

//------------------------------------------------------------------------------------------------
NiShaderError D3D11ShaderConstantMap::UpdateGlobalConstantValue(
    void* pShaderConstantBuffer,
    NiShaderConstantMapEntry* pEntry,
    const NiRenderCallContext& callContext)
{
    XMMATRIX tempMatrix;
    efd::UInt32 dataSize = 0;
    efd::UInt32 dataStride = 0;
    const void* pDataSource = ObtainGlobalConstantValue(
        pEntry, 
        callContext,
        dataSize,
        dataStride,
        tempMatrix);
    if (pDataSource == NULL)
    {
        D3D11Error::ReportWarning(
            "Entry %s in shader constant map %s failed in "
            __FUNCTION__
            " because the global attribute value for the entry could not be found.",
            pEntry->GetKey(),
            GetName());

        return NISHADERERR_UNKNOWN;
    }

    efd::UInt32 encodedShaderRegister = pEntry->GetShaderRegister();
    efd::UInt32 encodedRegisterCount = pEntry->GetRegisterCount();

    efd::UInt32 startRegister = 0;
    efd::UInt32 startElement = 0;
    efd::UInt32 registerCount = 0;
    efd::UInt32 elementCount = 0;
    efd::Bool packRegisters = false;

    efd::Bool valid1 = DecodePackedRegisterAndElement(
        encodedShaderRegister,
        startRegister, 
        startElement, 
        packRegisters);
    EE_ASSERT(packRegisters == false);
    efd::Bool valid2 = DecodePackedRegisterAndElement(
        encodedRegisterCount,
        registerCount, 
        elementCount, 
        packRegisters);
    if (!valid1 || !valid2)
    {
        D3D11Error::ReportWarning(
            "Entry %s in shader constant map %s failed in "
            __FUNCTION__
            " because the shader register was incorrectly encoded in the entry.",
            pEntry->GetKey(),
            GetName());

        // UpdateConstantBufferPacking needs to be called.
        return NISHADERERR_UNKNOWN;
    }

    efd::UInt32 arrayLength = 1;
    if (pEntry->IsArray())
    {
        EE_ASSERT(dataStride != 0);
        arrayLength = dataSize / dataStride;
        // The register is encoded with the total size of all individual entries in an array
    }

    return FillShaderConstantBuffer(
        pShaderConstantBuffer,
        pDataSource, 
        dataSize,
        dataStride,
        startRegister, 
        startElement,
        registerCount, 
        elementCount, 
        arrayLength, 
        packRegisters, 
        NULL);
}

//------------------------------------------------------------------------------------------------
NiShaderError D3D11ShaderConstantMap::UpdateOperatorConstantValue(
    void* pShaderConstantBuffer, 
    NiShaderConstantMapEntry* pEntry,
    const NiRenderCallContext& callContext, 
    efd::Bool isGlobal, 
    NiExtraData* pExtraData)
{
    XMMATRIX tempMatrix;
    efd::UInt32 dataSize = 0;
    efd::UInt32 dataStride = 0;
    const void* pDataSource = ObtainOperatorConstantValue(
        pEntry, 
        callContext,
        isGlobal, 
        pExtraData, 
        dataSize,
        dataStride,
        tempMatrix);
    if (pDataSource == NULL)
    {
        D3D11Error::ReportWarning(
            "Entry %s in shader constant map %s failed in "
            __FUNCTION__
            " because the operator result for the entry could not be found.",
            pEntry->GetKey(),
            GetName());

        return NISHADERERR_UNKNOWN;
    }

    efd::UInt32 encodedShaderRegister = pEntry->GetShaderRegister();
    efd::UInt32 encodedRegisterCount = pEntry->GetRegisterCount();

    efd::UInt32 startRegister = 0;
    efd::UInt32 startElement = 0;
    efd::UInt32 registerCount = 0;
    efd::UInt32 elementCount = 0;
    efd::Bool packRegisters = false;

    efd::Bool valid1 = DecodePackedRegisterAndElement(
        encodedShaderRegister,
        startRegister, 
        startElement, 
        packRegisters);
    EE_ASSERT(packRegisters == false);
    efd::Bool valid2 = DecodePackedRegisterAndElement(
        encodedRegisterCount,
        registerCount, 
        elementCount, 
        packRegisters);
    if (!valid1 || !valid2)
    {
        D3D11Error::ReportWarning(
            "Entry %s in shader constant map %s failed in "
            __FUNCTION__
            " because the shader register was incorrectly encoded in the entry.",
            pEntry->GetKey(),
            GetName());

        // UpdateConstantBufferPacking needs to be called.
        return NISHADERERR_UNKNOWN;
    }

    efd::UInt32 arrayLength = 1;
    if (pEntry->IsArray())
    {
        EE_ASSERT(dataStride != 0);
        arrayLength = dataSize / dataStride;
        // The register is encoded with the total size of all individual entries in an array
    }

    return FillShaderConstantBuffer(
        pShaderConstantBuffer,
        pDataSource, 
        dataSize,
        dataStride,
        startRegister, 
        startElement,
        registerCount, 
        elementCount, 
        arrayLength, 
        packRegisters, 
        NULL);
}

//------------------------------------------------------------------------------------------------
NiShaderError D3D11ShaderConstantMap::UpdateObjectConstantValue(
    void* pShaderConstantBuffer, 
    NiShaderConstantMapEntry* pEntry,
    const NiRenderCallContext& callContext)
{
    efd::UInt32 encodedShaderRegister = pEntry->GetShaderRegister();
    efd::UInt32 encodedRegisterCount = pEntry->GetRegisterCount();

    efd::UInt32 startRegister = 0;
    efd::UInt32 startElement = 0;
    efd::UInt32 registerCount = 0;
    efd::UInt32 elementCount = 0;
    efd::Bool packRegisters = false;

    efd::Bool valid1 = DecodePackedRegisterAndElement(
        encodedShaderRegister,
        startRegister, 
        startElement, 
        packRegisters);
    EE_ASSERT(packRegisters == false);
    efd::Bool valid2 = DecodePackedRegisterAndElement(
        encodedRegisterCount,
        registerCount, 
        elementCount, 
        packRegisters);
    if (!valid1 || !valid2)
    {
        D3D11Error::ReportWarning(
            "Entry %s in shader constant map %s failed in "
            __FUNCTION__
            " because the shader register was incorrectly encoded in the entry.",
            pEntry->GetKey(),
            GetName());

        // UpdateConstantBufferPacking needs to be called.
        return NISHADERERR_UNKNOWN;
    }

    // Stack allocate enough aligned memory to write constants into.
    // Note: EE_STACK_ALLOC does not (apparently) always return 16-byte
    // aligned addresses in the same way that malloc does.
    // The +16 in bufferSize provides headroom so we can round UP to the
    // next 16-byte boundary without overflowing the allocation.
    efd::UInt32 matrixRegisters = (registerCount + 3) & ~0x3;
    efd::UInt32 bufferSize = matrixRegisters * 4 * sizeof(efd::Float32) + 16;
    efd::UInt8* pInitialData = EE_STACK_ALLOC(efd::UInt8, bufferSize);
    size_t pointerAsInt = (size_t)pInitialData;
    XMMATRIX* pResult = (XMMATRIX*)((pointerAsInt + 15) & ~0xF);
    
    efd::UInt32 dataSize = 0;
    efd::UInt32 dataStride = 0;
    const void* pDataSource = ObtainObjectConstantValue(
        pEntry, 
        callContext, 
        dataSize,
        dataStride,
        pResult);
    if (pDataSource == NULL)
    {
        D3D11Error::ReportWarning(
            "Entry %s in shader constant map %s failed in "
            __FUNCTION__
            " because the object attribute value for the entry could not be found.",
            pEntry->GetKey(),
            GetName());

        EE_STACK_FREE(pInitialData);
        return NISHADERERR_UNKNOWN;
    }

    efd::UInt32 arrayLength = 1;
    if (pEntry->IsArray())
    {
        EE_ASSERT(dataStride != 0);
        arrayLength = dataSize / dataStride;
        // The register is encoded with the total size of all individual entries in an array
    }

    NiShaderError eErr = FillShaderConstantBuffer(
        pShaderConstantBuffer,
        pDataSource, 
        dataSize,
        dataStride,
        startRegister, 
        startElement,
        registerCount, 
        elementCount, 
        arrayLength, 
        packRegisters, 
        NULL);
    EE_STACK_FREE(pInitialData);
    return eErr;
}

//------------------------------------------------------------------------------------------------
NiShaderError D3D11ShaderConstantMap::FillShaderConstantBuffer(
    void* pShaderConstantBuffer, 
    const void* pSourceData,
    efd::UInt32 sourceDataSize,
    efd::UInt32 sourceDataStride,
    efd::UInt32 startRegister, 
    efd::UInt32 startElement,
    efd::UInt32 registerCount, 
    efd::UInt32 elementCount,
    efd::UInt32 arrayCount, 
    efd::Bool packRegisters,
    const efd::UInt16* pReorderArray)
{
    const size_t elementSize = D3D11_COMMONSHADER_CONSTANT_BUFFER_COMPONENT_BIT_COUNT / 8;

    efd::UInt8* pTemp = (efd::UInt8*)pShaderConstantBuffer;
    efd::UInt8* pWriteLocation = pTemp + elementSize *
        (startRegister * D3D11_COMMONSHADER_CONSTANT_BUFFER_REGISTER_COMPONENTS + startElement);

    const efd::UInt32 numElementsWritten = elementCount + 
        registerCount * D3D11_COMMONSHADER_CONSTANT_BUFFER_REGISTER_COMPONENTS;

    if (elementCount != 0)
    {
        // The encoded register count is only _full_ registers, so
        // additional elements go to the next register.
        registerCount++;
    }
    else  //  elementCount == 0
    {
        // If no additional elements are required, then we're going
        // to need to fill in all elements of each register.
        elementCount = D3D11_COMMONSHADER_CONSTANT_BUFFER_REGISTER_COMPONENTS;
        packRegisters = true;
    }

    // If packRegisters is true, then we write all 4 elements on all
    // but the last register - on that one, we only write
    // elementCount. If packRegisters if false, we write
    // elementCount elements in all registers.
    const efd::UInt32 writeElements =
        ((packRegisters == false) ?
        elementCount :
        D3D11_COMMONSHADER_CONSTANT_BUFFER_REGISTER_COMPONENTS);

    // DT33848: Store whether source data is packed or not somewhere more convenient.
    const efd::UInt32 readElements = 
        (((packRegisters == false) && (sourceDataSize < numElementsWritten * elementSize)) ?
        elementCount :
        D3D11_COMMONSHADER_CONSTANT_BUFFER_REGISTER_COMPONENTS);

    // The register is encoded with the total size of all individual entries in an array
    const efd::UInt32 writeStride = elementSize * D3D11_COMMONSHADER_CONSTANT_BUFFER_COMPONENTS;
    EE_ASSERT(registerCount % arrayCount == 0);
    registerCount /= arrayCount;
    EE_ASSERT(sourceDataSize / sourceDataStride >= arrayCount);

    if (pReorderArray)
    {
        for (efd::UInt32 i = 0; i < arrayCount; i++)
        {
            efd::UInt8* pSource = (efd::UInt8*)pSourceData +
                pReorderArray[i] * sourceDataStride;

            NiMemcpyFloatArray(
                (efd::Float32*)pWriteLocation,
                (const efd::Float32*)pSource,
                writeElements * (registerCount-1));

            pSource += (registerCount - 1) * readElements * elementSize;

            pWriteLocation += (registerCount - 1) * writeStride;

            NiMemcpyFloatArray(
                (efd::Float32*)pWriteLocation,
                (const efd::Float32*)pSource,
                elementCount);

            pSource += elementCount * elementSize;

            pWriteLocation += writeStride;
        }
    }
    else
    {
        for (efd::UInt32 i = 0; i < arrayCount; i++)
        {
            efd::UInt8* pSource = (efd::UInt8*)pSourceData + i * sourceDataStride;

            for (efd::UInt32 j = 0; j < registerCount - 1; j++)
            {

                NiMemcpyFloatArray(
                    (efd::Float32*)pWriteLocation,
                    (const efd::Float32*)pSource,
                    writeElements);
                pSource += readElements * elementSize;

                pWriteLocation += writeStride;

            }

            NiMemcpyFloatArray(
                (efd::Float32*)pWriteLocation,
                (const efd::Float32*)pSource,
                elementCount);
            pSource += elementCount * elementSize;

            pWriteLocation += writeStride;
        }
    }

    // This is the non-optimized code for comparison.
    /*
    else
    {

        if (elementCount != 0)
        {
            // The encoded register count is only _full_ registers, so
            // additional elements go to the next register.
            registerCount++;
        }
        else
        {
            // If no additional elements are required, then we're going
            // to need to fill in all elements of each register.
            elementCount =
                D3D11_COMMONSHADER_CONSTANT_BUFFER_REGISTER_COMPONENTS;
            packRegisters = true;
        }

        const size_t arrayElementSize =
            registerCount * elementCount * elementSize;

        for (efd::UInt32 i = 0; i < arrayCount; i++)
        {
            efd::UInt8* pSource = (efd::UInt8*)pvSourceData;
            if (pReorderArray)
                pSource += pReorderArray[i] * arrayElementSize;
            else
                pSource += i * arrayElementSize;

            for (efd::UInt32 j = 0; j < registerCount; j++)
            {
                efd::UInt8* puiDest = pWriteLocation;
                // If packRegisters is true, then we write all 4 elements on
                // all but the last register - on that one, we only write
                // elementCount. If packRegisters if false, we write
                // elementCount elements in all registers.
                const efd::UInt32 writeElements =
                    ((packRegisters == false || j == registerCount - 1) ?
                    elementCount :
                    D3D11_COMMONSHADER_CONSTANT_BUFFER_REGISTER_COMPONENTS);
                for (efd::UInt32 k = 0; k < writeElements; k++)
                {
                    // 4-byte register elements
                    memcpy(puiDest, pSource, elementSize);
                    pSource += elementSize;
                    puiDest += elementSize;
                }
                pWriteLocation += elementSize *
                    D3D11_COMMONSHADER_CONSTANT_BUFFER_COMPONENTS;
            }
        }
    }
    */

    return NISHADERERR_OK;
}

//------------------------------------------------------------------------------------------------
const void* D3D11ShaderConstantMap::ObtainDefinedConstantValue(
    NiShaderConstantMapEntry* pEntry, 
    const NiRenderCallContext& callContext,
    efd::UInt32& dataSize,
    efd::UInt32& dataStride,
    XMMATRIX& tempMatrix)
{
    EE_ASSERT(D3D11Renderer::GetRenderer());
    D3D11Renderer* pRenderer = D3D11Renderer::GetRenderer();

    EE_ASSERT(callContext.m_pkMesh != NULL && callContext.m_pkState != NULL);

    efd::UInt32 internalFlags = pEntry->GetInternal();

    switch (internalFlags)
    {
    // Transformations
    case SCM_DEF_PROJ:
    case SCM_DEF_INVPROJ:
    case SCM_DEF_PROJ_T:
    case SCM_DEF_INVPROJ_T:
    {
        efd::Bool invert = false;
        efd::Bool transpose = false;

        if ((internalFlags == SCM_DEF_INVPROJ) ||
            (internalFlags == SCM_DEF_INVPROJ_T))
        {
            invert = true;
        }
        if ((internalFlags == SCM_DEF_PROJ_T) ||
            (internalFlags == SCM_DEF_INVPROJ_T))
        {
            transpose = true;
        }

        // If it's column major, then we must manually transpose.
        if (pEntry->GetColumnMajor())
            transpose = !transpose;

        tempMatrix = pRenderer->GetProjectionMatrix();

        XMVECTOR determinant;
        if (invert)
            tempMatrix = XMMatrixInverse(&determinant, tempMatrix);
        if (transpose)
            tempMatrix = XMMatrixTranspose(tempMatrix);

        dataSize = 4 * 4 * sizeof(efd::Float32);
        dataStride = 4 * 4 * sizeof(efd::Float32);

        return &tempMatrix;
    }
    case SCM_DEF_VIEW:
    case SCM_DEF_INVVIEW:
    case SCM_DEF_VIEW_T:
    case SCM_DEF_INVVIEW_T:
    {
        efd::Bool invert = false;
        efd::Bool transpose = false;

        if ((internalFlags == SCM_DEF_INVVIEW) ||
            (internalFlags == SCM_DEF_INVVIEW_T))
        {
            invert = true;
        }
        if ((internalFlags == SCM_DEF_VIEW_T) ||
            (internalFlags == SCM_DEF_INVVIEW_T))
        {
            transpose = true;
        }

        // If it's column major, then we must manually transpose.
        if (pEntry->GetColumnMajor())
            transpose = !transpose;

        tempMatrix = pRenderer->GetViewMatrix();

        XMVECTOR determinant;
        if (invert)
            tempMatrix = XMMatrixInverse(&determinant, tempMatrix);
        if (transpose)
            tempMatrix = XMMatrixTranspose(tempMatrix);

        dataSize = 4 * 4 * sizeof(efd::Float32);
        dataStride = 4 * 4 * sizeof(efd::Float32);

        return &tempMatrix;
    }
    case SCM_DEF_WORLD:
    case SCM_DEF_INVWORLD:
    case SCM_DEF_WORLD_T:
    case SCM_DEF_INVWORLD_T:
    {
        efd::Bool invert = false;
        efd::Bool transpose = false;

        if ((internalFlags == SCM_DEF_INVWORLD) ||
            (internalFlags == SCM_DEF_INVWORLD_T))
        {
            invert = true;
        }
        if ((internalFlags == SCM_DEF_WORLD_T) ||
            (internalFlags == SCM_DEF_INVWORLD_T))
        {
            transpose = true;
        }

        // If it's column major, then we must manually transpose.
        if (pEntry->GetColumnMajor())
            transpose = !transpose;

        tempMatrix = pRenderer->GetWorldMatrix();

        XMVECTOR determinant;
        if (invert)
            tempMatrix = XMMatrixInverse(&determinant, tempMatrix);
        if (transpose)
            tempMatrix = XMMatrixTranspose(tempMatrix);

        dataSize = 4 * 4 * sizeof(efd::Float32);
        dataStride = 4 * 4 * sizeof(efd::Float32);

        return &tempMatrix;
    }
    case SCM_DEF_WORLDVIEW:
    case SCM_DEF_INVWORLDVIEW:
    case SCM_DEF_WORLDVIEW_T:
    case SCM_DEF_INVWORLDVIEW_T:
    {
        efd::Bool invert = false;
        efd::Bool transpose = false;

        if ((internalFlags == SCM_DEF_INVWORLDVIEW) ||
            (internalFlags == SCM_DEF_INVWORLDVIEW_T))
        {
            invert = true;
        }
        if ((internalFlags == SCM_DEF_WORLDVIEW_T) ||
            (internalFlags == SCM_DEF_INVWORLDVIEW_T))
        {
            transpose = true;
        }

        // If it's column major, then we must manually transpose.
        if (pEntry->GetColumnMajor())
            transpose = !transpose;

        const XMMATRIX& worldMatrix = pRenderer->GetWorldMatrix();
        const XMMATRIX& viewMatrix = pRenderer->GetViewMatrix();
        tempMatrix = worldMatrix * viewMatrix;

        XMVECTOR determinant;
        if (invert)
            tempMatrix = XMMatrixInverse(&determinant, tempMatrix);
        if (transpose)
            tempMatrix = XMMatrixTranspose(tempMatrix);

        dataSize = 4 * 4 * sizeof(efd::Float32);
        dataStride = 4 * 4 * sizeof(efd::Float32);

        return &tempMatrix;
    }
    case SCM_DEF_VIEWPROJ:
    case SCM_DEF_INVVIEWPROJ:
    case SCM_DEF_VIEWPROJ_T:
    case SCM_DEF_INVVIEWPROJ_T:
    {
        efd::Bool invert = false;
        efd::Bool transpose = false;

        if ((internalFlags == SCM_DEF_INVVIEWPROJ) ||
            (internalFlags == SCM_DEF_INVVIEWPROJ_T))
        {
            invert = true;
        }
        if ((internalFlags == SCM_DEF_VIEWPROJ_T) ||
            (internalFlags == SCM_DEF_INVVIEWPROJ_T))
        {
            transpose = true;
        }

        // If it's column major, then we must manually transpose.
        if (pEntry->GetColumnMajor())
            transpose = !transpose;

        const XMMATRIX& viewMatrix = pRenderer->GetViewMatrix();
        const XMMATRIX& projMatrix = pRenderer->GetProjectionMatrix();
        tempMatrix = viewMatrix * projMatrix;

        XMVECTOR determinant;
        if (invert)
            tempMatrix = XMMatrixInverse(&determinant, tempMatrix);
        if (transpose)
            tempMatrix = XMMatrixTranspose(tempMatrix);

        dataSize = 4 * 4 * sizeof(efd::Float32);
        dataStride = 4 * 4 * sizeof(efd::Float32);

        return &tempMatrix;
    }
    case SCM_DEF_WORLDVIEWPROJ:
    case SCM_DEF_INVWORLDVIEWPROJ:
    case SCM_DEF_WORLDVIEWPROJ_T:
    case SCM_DEF_INVWORLDVIEWPROJ_T:
    {
        efd::Bool invert = false;
        efd::Bool transpose = false;

        if ((internalFlags == SCM_DEF_INVWORLDVIEWPROJ) ||
            (internalFlags == SCM_DEF_INVWORLDVIEWPROJ_T))
        {
            invert = true;
        }
        if ((internalFlags == SCM_DEF_WORLDVIEWPROJ_T) ||
            (internalFlags == SCM_DEF_INVWORLDVIEWPROJ_T))
        {
            transpose = true;
        }

        // If it's column major, then we must manually transpose.
        if (pEntry->GetColumnMajor())
            transpose = !transpose;

        const XMMATRIX& worldMatrix = pRenderer->GetWorldMatrix();
        const XMMATRIX& viewMatrix = pRenderer->GetViewMatrix();
        const XMMATRIX& projMatrix = pRenderer->GetProjectionMatrix();
        XMMATRIX worldViewMatrix = worldMatrix * viewMatrix;
        tempMatrix = worldViewMatrix * projMatrix;

        XMVECTOR determinant;
        if (invert)
            tempMatrix = XMMatrixInverse(&determinant, tempMatrix);
        if (transpose)
            tempMatrix = XMMatrixTranspose(tempMatrix);

        dataSize = 4 * 4 * sizeof(efd::Float32);
        dataStride = 4 * 4 * sizeof(efd::Float32);

        return &tempMatrix;
    }
    case SCM_DEF_SKINBONE_MATRIX_3:
    {
        NiMesh* pMesh = NiVerifyStaticCast(NiMesh, callContext.m_pkMesh);

        NiSkinningMeshModifier* pSkin = NiGetModifier(NiSkinningMeshModifier, pMesh);

        if (pSkin)
        {
            dataSize = 3 * 4 * sizeof(efd::Float32) * pSkin->GetBoneCount();
            dataStride = 3 * 4 * sizeof(efd::Float32);

            return pSkin->GetBoneMatrices();
        }
        else
        {
            D3D11Error::ReportWarning(
                "Entry %s in shader constant map %s failed in "
                __FUNCTION__
                " because it requested bone data from a non-skinned mesh.",
                pEntry->GetKey(),
                GetName());
            return NULL;
        }
    }
    // Texture transforms
    case SCM_DEF_TEXTRANSFORMBASE:
    case SCM_DEF_INVTEXTRANSFORMBASE:
    case SCM_DEF_TEXTRANSFORMBASE_T:
    case SCM_DEF_INVTEXTRANSFORMBASE_T:
    {
        const efd::Matrix3* pMatrix = NULL;

        efd::Bool transpose = false;

        const NiTexturingProperty* pTexProp = callContext.m_pkState->GetTexturing();
        if (pTexProp)
        {
            const NiTexturingProperty::Map* pMap = pTexProp->GetBaseMap();
            if (pMap)
            {
                const NiTextureTransform* pTextureTransform = pMap->GetTextureTransform();

                if (pTextureTransform)
                {
                    pMatrix = pTextureTransform->GetMatrix();

                    if ((internalFlags == SCM_DEF_INVTEXTRANSFORMBASE) ||
                        (internalFlags == SCM_DEF_INVTEXTRANSFORMBASE_T))
                    {
                        D3D11Error::ReportWarning(
                            "Entry %s in shader constant map %s failed in "
                            __FUNCTION__
                            " because it attempts to invert a texture transform matrix,"
                            " which is not mathematically invertible."
                            " Non-inverted matrix will be used instead.",
                            pEntry->GetKey(),
                            GetName());
                    }
                    if ((internalFlags == SCM_DEF_TEXTRANSFORMBASE_T) ||
                        (internalFlags == SCM_DEF_INVTEXTRANSFORMBASE_T))
                    {
                        transpose = true;
                    }
                }
            }
        }

        // If it's column major, then we must manually transpose.
        if (pEntry->GetColumnMajor())
            transpose = !transpose;

        SetupTextureTransformMatrix(tempMatrix, pMatrix, transpose);

        dataSize = 4 * 4 * sizeof(efd::Float32);
        dataStride = 4 * 4 * sizeof(efd::Float32);

        return &tempMatrix;
    }
    case SCM_DEF_TEXTRANSFORMDARK:
    case SCM_DEF_INVTEXTRANSFORMDARK:
    case SCM_DEF_TEXTRANSFORMDARK_T:
    case SCM_DEF_INVTEXTRANSFORMDARK_T:
    {
        const efd::Matrix3* pMatrix = NULL;

        efd::Bool transpose = false;

        const NiTexturingProperty* pTexProp = callContext.m_pkState->GetTexturing();
        if (pTexProp)
        {
            const NiTexturingProperty::Map* pMap = pTexProp->GetDarkMap();
            if (pMap)
            {
                const NiTextureTransform* pTextureTransform = pMap->GetTextureTransform();

                if (pTextureTransform)
                {
                    pMatrix = pTextureTransform->GetMatrix();

                    if ((internalFlags == SCM_DEF_INVTEXTRANSFORMDARK) ||
                        (internalFlags == SCM_DEF_INVTEXTRANSFORMDARK_T))
                    {
                        D3D11Error::ReportWarning(
                            "Entry %s in shader constant map %s failed in "
                            __FUNCTION__
                            " because it attempts to invert a texture transform matrix,"
                            " which is not mathematically invertible."
                            " Non-invertex matrix will be used instead.",
                            pEntry->GetKey(),
                            GetName());
                    }
                    if ((internalFlags == SCM_DEF_TEXTRANSFORMDARK_T) ||
                        (internalFlags == SCM_DEF_INVTEXTRANSFORMDARK_T))
                    {
                        transpose = true;
                    }
                }
            }
        }

        // If it's column major, then we must manually transpose.
        if (pEntry->GetColumnMajor())
            transpose = !transpose;

        SetupTextureTransformMatrix(tempMatrix, pMatrix, transpose);

        dataSize = 4 * 4 * sizeof(efd::Float32);
        dataStride = 4 * 4 * sizeof(efd::Float32);

        return &tempMatrix;
    }
    case SCM_DEF_TEXTRANSFORMDETAIL:
    case SCM_DEF_INVTEXTRANSFORMDETAIL:
    case SCM_DEF_TEXTRANSFORMDETAIL_T:
    case SCM_DEF_INVTEXTRANSFORMDETAIL_T:
    {
        const efd::Matrix3* pMatrix = NULL;

        efd::Bool transpose = false;

        const NiTexturingProperty* pTexProp = callContext.m_pkState->GetTexturing();
        if (pTexProp)
        {
            const NiTexturingProperty::Map* pMap = pTexProp->GetDetailMap();
            if (pMap)
            {
                const NiTextureTransform* pTextureTransform = pMap->GetTextureTransform();

                if (pTextureTransform)
                {
                    pMatrix = pTextureTransform->GetMatrix();

                    if ((internalFlags == SCM_DEF_INVTEXTRANSFORMDETAIL) ||
                        (internalFlags == SCM_DEF_INVTEXTRANSFORMDETAIL_T))
                    {
                        D3D11Error::ReportWarning(
                            "Entry %s in shader constant map %s failed in "
                            __FUNCTION__
                            " because it attempts to invert a texture transform matrix,"
                            " which is not mathematically invertible."
                            " Non-invertex matrix will be used instead.",
                            pEntry->GetKey(),
                            GetName());
                    }
                    if ((internalFlags == SCM_DEF_TEXTRANSFORMDETAIL_T) ||
                        (internalFlags == SCM_DEF_INVTEXTRANSFORMDETAIL_T))
                    {
                        transpose = true;
                    }
                }
            }
        }

        // If it's column major, then we must manually transpose.
        if (pEntry->GetColumnMajor())
            transpose = !transpose;

        SetupTextureTransformMatrix(tempMatrix, pMatrix, transpose);

        dataSize = 4 * 4 * sizeof(efd::Float32);
        dataStride = 4 * 4 * sizeof(efd::Float32);

        return &tempMatrix;
    }
    case SCM_DEF_TEXTRANSFORMGLOSS:
    case SCM_DEF_INVTEXTRANSFORMGLOSS:
    case SCM_DEF_TEXTRANSFORMGLOSS_T:
    case SCM_DEF_INVTEXTRANSFORMGLOSS_T:
    {
        const efd::Matrix3* pMatrix = NULL;

        efd::Bool transpose = false;

        const NiTexturingProperty* pTexProp = callContext.m_pkState->GetTexturing();
        if (pTexProp)
        {
            const NiTexturingProperty::Map* pMap = pTexProp->GetGlossMap();
            if (pMap)
            {
                const NiTextureTransform* pTextureTransform = pMap->GetTextureTransform();

                if (pTextureTransform)
                {
                    pMatrix = pTextureTransform->GetMatrix();

                    if ((internalFlags == SCM_DEF_INVTEXTRANSFORMGLOSS) ||
                        (internalFlags == SCM_DEF_INVTEXTRANSFORMGLOSS_T))
                    {
                        D3D11Error::ReportWarning(
                            "Entry %s in shader constant map %s failed in "
                            __FUNCTION__
                            " because it attempts to invert a texture transform matrix,"
                            " which is not mathematically invertible."
                            " Non-invertex matrix will be used instead.",
                            pEntry->GetKey(),
                            GetName());
                    }
                    if ((internalFlags == SCM_DEF_TEXTRANSFORMGLOSS_T) ||
                        (internalFlags == SCM_DEF_INVTEXTRANSFORMGLOSS_T))
                    {
                        transpose = true;
                    }
                }
            }
        }

        // If it's column major, then we must manually transpose.
        if (pEntry->GetColumnMajor())
            transpose = !transpose;

        SetupTextureTransformMatrix(tempMatrix, pMatrix, transpose);

        dataSize = 4 * 4 * sizeof(efd::Float32);
        dataStride = 4 * 4 * sizeof(efd::Float32);

        return &tempMatrix;
    }
    case SCM_DEF_TEXTRANSFORMGLOW:
    case SCM_DEF_INVTEXTRANSFORMGLOW:
    case SCM_DEF_TEXTRANSFORMGLOW_T:
    case SCM_DEF_INVTEXTRANSFORMGLOW_T:
    {
        const efd::Matrix3* pMatrix = NULL;

        efd::Bool transpose = false;

        const NiTexturingProperty* pTexProp = callContext.m_pkState->GetTexturing();
        if (pTexProp)
        {
            const NiTexturingProperty::Map* pMap = pTexProp->GetGlowMap();
            if (pMap)
            {
                const NiTextureTransform* pTextureTransform = pMap->GetTextureTransform();

                if (pTextureTransform)
                {
                    pMatrix = pTextureTransform->GetMatrix();

                    if ((internalFlags == SCM_DEF_INVTEXTRANSFORMGLOW) ||
                        (internalFlags == SCM_DEF_INVTEXTRANSFORMGLOW_T))
                    {
                        D3D11Error::ReportWarning(
                            "Entry %s in shader constant map %s failed in "
                            __FUNCTION__
                            " because it attempts to invert a texture transform matrix,"
                            " which is not mathematically invertible."
                            " Non-invertex matrix will be used instead.",
                            pEntry->GetKey(),
                            GetName());
                    }
                    if ((internalFlags == SCM_DEF_TEXTRANSFORMGLOW_T) ||
                        (internalFlags == SCM_DEF_INVTEXTRANSFORMGLOW_T))
                    {
                        transpose = true;
                    }
                }
            }
        }

        // If it's column major, then we must manually transpose.
        if (pEntry->GetColumnMajor())
            transpose = !transpose;

        SetupTextureTransformMatrix(tempMatrix, pMatrix, transpose);

        dataSize = 4 * 4 * sizeof(efd::Float32);
        dataStride = 4 * 4 * sizeof(efd::Float32);

        return &tempMatrix;
    }
    case SCM_DEF_TEXTRANSFORMBUMP:
    case SCM_DEF_INVTEXTRANSFORMBUMP:
    case SCM_DEF_TEXTRANSFORMBUMP_T:
    case SCM_DEF_INVTEXTRANSFORMBUMP_T:
    {
        const efd::Matrix3* pMatrix = NULL;

        efd::Bool transpose = false;

        const NiTexturingProperty* pTexProp = callContext.m_pkState->GetTexturing();
        if (pTexProp)
        {
            const NiTexturingProperty::Map* pMap = pTexProp->GetBumpMap();
            if (pMap)
            {
                const NiTextureTransform* pTextureTransform = pMap->GetTextureTransform();

                if (pTextureTransform)
                {
                    pMatrix = pTextureTransform->GetMatrix();

                    if ((internalFlags == SCM_DEF_INVTEXTRANSFORMBUMP) ||
                        (internalFlags == SCM_DEF_INVTEXTRANSFORMBUMP_T))
                    {
                        D3D11Error::ReportWarning(
                            "Entry %s in shader constant map %s failed in "
                            __FUNCTION__
                            " because it attempts to invert a texture transform matrix,"
                            " which is not mathematically invertible."
                            " Non-invertex matrix will be used instead.",
                            pEntry->GetKey(),
                            GetName());
                    }
                    if ((internalFlags == SCM_DEF_TEXTRANSFORMBUMP_T) ||
                        (internalFlags == SCM_DEF_INVTEXTRANSFORMBUMP_T))
                    {
                        transpose = true;
                    }
                }
            }
        }

        // If it's column major, then we must manually transpose.
        if (pEntry->GetColumnMajor())
            transpose = !transpose;

        SetupTextureTransformMatrix(tempMatrix, pMatrix, transpose);

        dataSize = 4 * 4 * sizeof(efd::Float32);
        dataStride = 4 * 4 * sizeof(efd::Float32);

        return &tempMatrix;
    }
    case SCM_DEF_TEXTRANSFORMNORMAL:
    case SCM_DEF_INVTEXTRANSFORMNORMAL:
    case SCM_DEF_TEXTRANSFORMNORMAL_T:
    case SCM_DEF_INVTEXTRANSFORMNORMAL_T:
    {
        const efd::Matrix3* pMatrix = NULL;

        efd::Bool transpose = false;

        const NiTexturingProperty* pTexProp = callContext.m_pkState->GetTexturing();
        if (pTexProp)
        {
            const NiTexturingProperty::Map* pMap = pTexProp->GetNormalMap();
            if (pMap)
            {
                const NiTextureTransform* pTextureTransform = pMap->GetTextureTransform();

                if (pTextureTransform)
                {
                    pMatrix = pTextureTransform->GetMatrix();

                    if ((internalFlags == SCM_DEF_INVTEXTRANSFORMNORMAL) ||
                        (internalFlags == SCM_DEF_INVTEXTRANSFORMNORMAL_T))
                    {
                        D3D11Error::ReportWarning(
                            "Entry %s in shader constant map %s failed in "
                            __FUNCTION__
                            " because it attempts to invert a texture transform matrix,"
                            " which is not mathematically invertible."
                            " Non-invertex matrix will be used instead.",
                            pEntry->GetKey(),
                            GetName());
                    }
                    if ((internalFlags == SCM_DEF_TEXTRANSFORMNORMAL_T) ||
                        (internalFlags == SCM_DEF_INVTEXTRANSFORMNORMAL_T))
                    {
                        transpose = true;
                    }
                }
            }
        }

        // If it's column major, then we must manually transpose.
        if (pEntry->GetColumnMajor())
            transpose = !transpose;

        SetupTextureTransformMatrix(tempMatrix, pMatrix, transpose);

        dataSize = 4 * 4 * sizeof(efd::Float32);
        dataStride = 4 * 4 * sizeof(efd::Float32);

        return &tempMatrix;
    }
    case SCM_DEF_TEXTRANSFORMPARALLAX:
    case SCM_DEF_INVTEXTRANSFORMPARALLAX:
    case SCM_DEF_TEXTRANSFORMPARALLAX_T:
    case SCM_DEF_INVTEXTRANSFORMPARALLAX_T:
    {
        const efd::Matrix3* pMatrix = NULL;

        efd::Bool transpose = false;

        const NiTexturingProperty* pTexProp = callContext.m_pkState->GetTexturing();
        if (pTexProp)
        {
            const NiTexturingProperty::Map* pMap = pTexProp->GetParallaxMap();
            if (pMap)
            {
                const NiTextureTransform* pTextureTransform = pMap->GetTextureTransform();

                if (pTextureTransform)
                {
                    pMatrix = pTextureTransform->GetMatrix();

                    if ((internalFlags == SCM_DEF_INVTEXTRANSFORMPARALLAX) ||
                        (internalFlags == SCM_DEF_INVTEXTRANSFORMPARALLAX_T))
                    {
                        D3D11Error::ReportWarning(
                            "Entry %s in shader constant map %s failed in "
                            __FUNCTION__
                            " because it attempts to invert a texture transform matrix,"
                            " which is not mathematically invertible."
                            " Non-invertex matrix will be used instead.",
                            pEntry->GetKey(),
                            GetName());
                    }
                    if ((internalFlags == SCM_DEF_TEXTRANSFORMPARALLAX_T) ||
                        (internalFlags == SCM_DEF_INVTEXTRANSFORMPARALLAX_T))
                    {
                        transpose = true;
                    }
                }
            }
        }

        // If it's column major, then we must manually transpose.
        if (pEntry->GetColumnMajor())
            transpose = !transpose;

        SetupTextureTransformMatrix(tempMatrix, pMatrix, transpose);

        dataSize = 4 * 4 * sizeof(efd::Float32);
        dataStride = 4 * 4 * sizeof(efd::Float32);

        return &tempMatrix;
    }
    case SCM_DEF_TEXTRANSFORMDECAL:
    case SCM_DEF_INVTEXTRANSFORMDECAL:
    case SCM_DEF_TEXTRANSFORMDECAL_T:
    case SCM_DEF_INVTEXTRANSFORMDECAL_T:
    {
        const efd::Matrix3* pMatrix = NULL;

        efd::Bool transpose = false;

        const NiTexturingProperty* pTexProp = callContext.m_pkState->GetTexturing();
        if (pTexProp)
        {
            const NiTexturingProperty::Map* pMap = pTexProp->GetDecalMap(pEntry->GetExtra());
            if (pMap)
            {
                const NiTextureTransform* pTextureTransform = pMap->GetTextureTransform();

                if (pTextureTransform)
                {
                    pMatrix = pTextureTransform->GetMatrix();

                    if ((internalFlags == SCM_DEF_INVTEXTRANSFORMDECAL) ||
                        (internalFlags == SCM_DEF_INVTEXTRANSFORMDECAL_T))
                    {
                        D3D11Error::ReportWarning(
                            "Entry %s in shader constant map %s failed in "
                            __FUNCTION__
                            " because it attempts to invert a texture transform matrix,"
                            " which is not mathematically invertible."
                            " Non-invertex matrix will be used instead.",
                            pEntry->GetKey(),
                            GetName());
                    }
                    if ((internalFlags == SCM_DEF_TEXTRANSFORMDECAL_T) ||
                        (internalFlags == SCM_DEF_INVTEXTRANSFORMDECAL_T))
                    {
                        transpose = true;
                    }
                }
            }
        }

        // If it's column major, then we must manually transpose.
        if (pEntry->GetColumnMajor())
            transpose = !transpose;

        SetupTextureTransformMatrix(tempMatrix, pMatrix,  transpose);

        dataSize = 4 * 4 * sizeof(efd::Float32);
        dataStride = 4 * 4 * sizeof(efd::Float32);

        return &tempMatrix;
    }
    case SCM_DEF_TEXTRANSFORMSHADER:
    case SCM_DEF_INVTEXTRANSFORMSHADER:
    case SCM_DEF_TEXTRANSFORMSHADER_T:
    case SCM_DEF_INVTEXTRANSFORMSHADER_T:
    {
        const efd::Matrix3* pMatrix = NULL;

        efd::Bool transpose = false;

        NiTexturingProperty* pTexProp = callContext.m_pkState->GetTexturing();
        if (pTexProp)
        {
            NiTexturingProperty::Map* pShaderMap = pTexProp->GetShaderMap(pEntry->GetExtra());
            if (pShaderMap)
            {
                NiTextureTransform* pTextureTransform = pShaderMap->GetTextureTransform();

                if (pTextureTransform)
                {
                    pMatrix = pTextureTransform->GetMatrix();

                    if ((internalFlags == SCM_DEF_INVTEXTRANSFORMSHADER) ||
                        (internalFlags == SCM_DEF_INVTEXTRANSFORMSHADER_T))
                    {
                        D3D11Error::ReportWarning(
                            "Entry %s in shader constant map %s failed in "
                            __FUNCTION__
                            " because it attempts to invert a texture transform matrix,"
                            " which is not mathematically invertible."
                            " Non-invertex matrix will be used instead.",
                            pEntry->GetKey(),
                            GetName());
                    }
                    if ((internalFlags == SCM_DEF_TEXTRANSFORMSHADER_T) ||
                        (internalFlags == SCM_DEF_INVTEXTRANSFORMSHADER_T))
                    {
                        transpose = true;
                    }
                }
            }
        }

        // If it's column major, then we must manually transpose.
        if (pEntry->GetColumnMajor())
            transpose = !transpose;

        SetupTextureTransformMatrix(tempMatrix, pMatrix, transpose);

        dataSize = 4 * 4 * sizeof(efd::Float32);
        dataStride = 4 * 4 * sizeof(efd::Float32);

        return &tempMatrix;
    }

    // Lighting
    case SCM_DEF_LIGHT_POS_WS:
    case SCM_DEF_LIGHT_DIR_WS:
    case SCM_DEF_LIGHT_POS_OS:
    case SCM_DEF_LIGHT_DIR_OS:
    {
        D3D11Error::ReportWarning("Constant \"%s\" is not supported.\n",
            (const efd::Char*)pEntry->GetKey());
        break;
    }

    // Materials
    case SCM_DEF_MATERIAL_DIFFUSE:
    {
        NiMaterialProperty* pMaterial = callContext.m_pkState->GetMaterial();
        if (pMaterial)
        {
            tempMatrix.r[0] = XMVectorSet(pMaterial->GetDiffuseColor().r, pMaterial->GetDiffuseColor().g, pMaterial->GetDiffuseColor().b, pMaterial->GetAlpha());
        }

        dataSize = 1 * 4 * sizeof(efd::Float32);
        dataStride = 1 * 4 * sizeof(efd::Float32);

        return &tempMatrix;
    }
    case SCM_DEF_MATERIAL_AMBIENT:
    {
        NiMaterialProperty* pMaterial = callContext.m_pkState->GetMaterial();
        if (pMaterial)
        {
            tempMatrix.r[0] = XMVectorSet(pMaterial->GetAmbientColor().r, pMaterial->GetAmbientColor().g, pMaterial->GetAmbientColor().b, pMaterial->GetAlpha());
        }

        dataSize = 1 * 4 * sizeof(efd::Float32);
        dataStride = 1 * 4 * sizeof(efd::Float32);

        return &tempMatrix;
    }
    case SCM_DEF_MATERIAL_SPECULAR:
    {
        NiMaterialProperty* pMaterial = callContext.m_pkState->GetMaterial();
        if (pMaterial)
        {
            tempMatrix.r[0] = XMVectorSet(pMaterial->GetSpecularColor().r, pMaterial->GetSpecularColor().g, pMaterial->GetSpecularColor().b, pMaterial->GetAlpha());
        }

        dataSize = 1 * 4 * sizeof(efd::Float32);
        dataStride = 1 * 4 * sizeof(efd::Float32);

        return &tempMatrix;
    }
    case SCM_DEF_MATERIAL_EMISSIVE:
    {
        NiMaterialProperty* pMaterial = callContext.m_pkState->GetMaterial();
        if (pMaterial)
        {
            tempMatrix.r[0] = XMVectorSet(pMaterial->GetEmittance().r, pMaterial->GetEmittance().g, pMaterial->GetEmittance().b, pMaterial->GetAlpha());
        }

        dataSize = 1 * 4 * sizeof(efd::Float32);
        dataStride = 1 * 4 * sizeof(efd::Float32);

        return &tempMatrix;
    }
    case SCM_DEF_MATERIAL_POWER:
    {
        NiMaterialProperty* pMaterial = callContext.m_pkState->GetMaterial();
        if (pMaterial)
        {
            tempMatrix.r[0] = XMVectorReplicate(pMaterial->GetShineness());
        }

        dataSize = 1 * 4 * sizeof(efd::Float32);
        dataStride = 1 * 4 * sizeof(efd::Float32);

        return &tempMatrix;
    }
    // Eye
    case SCM_DEF_EYE_POS:
    {
        tempMatrix.r[0] = pRenderer->GetInverseViewMatrix().r[3];

        dataSize = 1 * 4 * sizeof(efd::Float32);
        dataStride = 1 * 4 * sizeof(efd::Float32);

        return &tempMatrix;
    }
    case SCM_DEF_EYE_DIR:
    {
        tempMatrix.r[0] = pRenderer->GetInverseViewMatrix().r[2];

        dataSize = 1 * 4 * sizeof(efd::Float32);
        dataStride = 1 * 4 * sizeof(efd::Float32);

        return &tempMatrix;
    }
    // Constants
    case SCM_DEF_CONSTS_TAYLOR_SIN:
    case SCM_DEF_CONSTS_TAYLOR_COS:
    // Just set the data
    {
        dataSize = 1 * 4 * sizeof(efd::Float32);
        dataStride = 1 * 4 * sizeof(efd::Float32);

        return ObtainConstantConstantValue(pEntry, callContext, dataSize, dataStride, tempMatrix);
    }
    // Time
    case SCM_DEF_CONSTS_TIME:
    case SCM_DEF_CONSTS_SINTIME:
    case SCM_DEF_CONSTS_COSTIME:
    case SCM_DEF_CONSTS_TANTIME:
    case SCM_DEF_CONSTS_TIME_SINTIME_COSTIME_TANTIME:
    {
        // Grab the attribute from the geometry and set it
        NiFloatExtraData* pFloatED = (NiFloatExtraData*)callContext.m_pkMesh->GetExtraData(
            NiShaderConstantMap::GetTimeExtraDataName());
        if (!pFloatED)
        {
            D3D11Error::ReportWarning(
                "Entry %s in shader constant map %s failed in "
                __FUNCTION__
                " because it requires time data, but a time-based extra data object "
                "was not found on the mesh.",
                pEntry->GetKey(),
                GetName());
            return NULL;
        }

        efd::Float32 time = pFloatED->GetValue();

        switch (internalFlags)
        {
        case SCM_DEF_CONSTS_TIME:
            tempMatrix.r[0] = XMVectorReplicate(time);
            break;
        case SCM_DEF_CONSTS_SINTIME:
            tempMatrix.r[0] = XMVectorReplicate(sinf(time));
            break;
        case SCM_DEF_CONSTS_COSTIME:
            tempMatrix.r[0] = XMVectorReplicate(cosf(time));
            break;
        case SCM_DEF_CONSTS_TANTIME:
            tempMatrix.r[0] = XMVectorReplicate(tanf(time));
            break;
        case SCM_DEF_CONSTS_TIME_SINTIME_COSTIME_TANTIME:
            tempMatrix.r[0] = XMVectorSet(time, sinf(time), cosf(time), tanf(time));
            break;
        default:
            D3D11Error::ReportWarning(
                "Entry %s in shader constant map %s failed in "
                __FUNCTION__
                " because the internal flags 0x%08X are invalid.",
                pEntry->GetKey(),
                GetName(),
                internalFlags);
            EE_ASSERT(!"Time set --> Invalid case!");
        }

        dataSize = 1 * 4 * sizeof(efd::Float32);
        dataStride = 1 * 4 * sizeof(efd::Float32);

        return &tempMatrix;
    }
    case SCM_DEF_AMBIENTLIGHT:
    {
        efd::Float32 ambR = 0.0f, ambG = 0.0f, ambB = 0.0f;
        if (callContext.m_pkEffects)
        {
            NiDynEffectStateIter iter = callContext.m_pkEffects->GetLightHeadPos();
            while (iter)
            {
                NiAmbientLight* pLight = NiDynamicCast(
                    NiAmbientLight,
                    callContext.m_pkEffects->GetNextLight(iter));
                if (pLight)
                {
                    NiColor ambient = pLight->GetAmbientColor() * pLight->GetDimmer();
                    ambR += ambient.r;
                    ambG += ambient.g;
                    ambB += ambient.b;
                }
            }
        }
        tempMatrix.r[0] = XMVectorSet(ambR, ambG, ambB, 1.0f);

        dataSize = 1 * 3 * sizeof(efd::Float32);
        dataStride = 1 * 3 * sizeof(efd::Float32);

        return &tempMatrix;
    }
    case SCM_DEF_FOG_DENSITY:
    {
        NiFogProperty* pFog = callContext.m_pkState->GetFog();
        EE_ASSERT(pFog);

        efd::Float32 nearPlane;
        efd::Float32 farPlane;
        pRenderer->GetCameraNearAndFar(nearPlane, farPlane);
        efd::Float32 fogDensity = 1.0f / (pFog->GetDepth() * (farPlane - nearPlane));

        tempMatrix.r[0] = XMVectorReplicate(fogDensity);

        dataSize = 1 * 4 * sizeof(efd::Float32);
        dataStride = 1 * 4 * sizeof(efd::Float32);

        return &tempMatrix;
    }
    case SCM_DEF_FOG_NEARFAR:
    {
        NiFogProperty* pFog = callContext.m_pkState->GetFog();
        EE_ASSERT(pFog);

        efd::Float32 nearPlane, farPlane;
        pRenderer->GetCameraNearAndFar(nearPlane, farPlane);
        efd::Float32 cameraDepthRange = farPlane - nearPlane;

        efd::Float32 worldDepth = cameraDepthRange * pFog->GetDepth();
        efd::Float32 fogNear = farPlane - worldDepth;

        efd::Float32 fogFar = farPlane + pRenderer->GetMaxFogFactor() * worldDepth;

        tempMatrix.r[0] = XMVectorSet(fogNear, fogFar, 0.0f, 0.0f);

        dataSize = 1 * 2 * sizeof(efd::Float32);
        dataStride = 1 * 2 * sizeof(efd::Float32);

        return &tempMatrix;
    }
    case SCM_DEF_FOG_COLOR:
    {
        NiFogProperty* pFog = callContext.m_pkState->GetFog();
        EE_ASSERT(pFog);

        const NiColor& fogColor = pFog->GetFogColor();

        tempMatrix.r[0] = XMVectorSet(fogColor.r, fogColor.g, fogColor.b, 1.0f);

        dataSize = 1 * 4 * sizeof(efd::Float32);
        dataStride = 1 * 4 * sizeof(efd::Float32);

        return &tempMatrix;
    }
    case SCM_DEF_PARALLAX_OFFSET:
    {
        NiTexturingProperty* pTexProp = callContext.m_pkState->GetTexturing();
        efd::Float32 parallaxOffset = 0.0f;
        if (pTexProp)
        {
            NiTexturingProperty::ParallaxMap* pParallaxMap = pTexProp->GetParallaxMap();
            if (pParallaxMap)
            {
                parallaxOffset = pParallaxMap->GetOffset();
            }
        }
        tempMatrix.r[0] = XMVectorSet(parallaxOffset, 0.0f, 0.0f, 0.0f);

        dataSize = 1 * 1 * sizeof(efd::Float32);
        dataStride = 1 * 1 * sizeof(efd::Float32);

        return &tempMatrix;
    }
    case SCM_DEF_BUMP_MATRIX:
    {
        NiTexturingProperty* pTexProp = callContext.m_pkState->GetTexturing();
        NiTexturingProperty::BumpMap* pBumpMap = NULL;
        if (pTexProp)
            pBumpMap = pTexProp->GetBumpMap();
        if (pBumpMap)
        {
            tempMatrix.r[0] = XMVectorSet(pBumpMap->GetBumpMat00(), pBumpMap->GetBumpMat01(), pBumpMap->GetBumpMat10(), pBumpMap->GetBumpMat11());
        }
        else
        {
            tempMatrix.r[0] = XMVectorSet(1.0f, 1.0f, 1.0f, 1.0f);
        }

        dataSize = 1 * 4 * sizeof(efd::Float32);
        dataStride = 1 * 4 * sizeof(efd::Float32);

        return &tempMatrix;
    }
    case SCM_DEF_BUMP_LUMA_OFFSET_AND_SCALE:
    {
        NiTexturingProperty* pTexProp = callContext.m_pkState->GetTexturing();
        NiTexturingProperty::BumpMap* pBumpMap = NULL;
        if (pTexProp)
            pBumpMap = pTexProp->GetBumpMap();
        if (pBumpMap)
        {
            tempMatrix.r[0] = XMVectorSet(pBumpMap->GetLumaOffset(), pBumpMap->GetLumaScale(), 0.0f, 0.0f);
        }
        else
        {
            tempMatrix.r[0] = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
        }

        dataSize = 1 * 2 * sizeof(efd::Float32);
        dataStride = 1 * 2 * sizeof(efd::Float32);

        return &tempMatrix;
    }
    case SCM_DEF_TEXSIZEBASE:
    case SCM_DEF_TEXSIZEDARK:
    case SCM_DEF_TEXSIZEDETAIL:
    case SCM_DEF_TEXSIZEGLOSS:
    case SCM_DEF_TEXSIZEGLOW:
    case SCM_DEF_TEXSIZEBUMP:
    case SCM_DEF_TEXSIZENORMAL:
    case SCM_DEF_TEXSIZEPARALLAX:
    case SCM_DEF_TEXSIZEDECAL:
    case SCM_DEF_TEXSIZESHADER:
    {
        const NiTexturingProperty* pTexProp = callContext.m_pkState->GetTexturing();
        if (pTexProp)
        {
            const NiTexturingProperty::Map* pMap = NULL;

            switch (internalFlags)
            {
            case SCM_DEF_TEXSIZEBASE:
                pMap = pTexProp->GetBaseMap();
                break;
            case SCM_DEF_TEXSIZEDARK:
                pMap = pTexProp->GetDarkMap();
                break;
            case SCM_DEF_TEXSIZEDETAIL:
                pMap = pTexProp->GetDetailMap();
                break;
            case SCM_DEF_TEXSIZEGLOSS:
                pMap = pTexProp->GetGlossMap();
                break;
            case SCM_DEF_TEXSIZEGLOW:
                pMap = pTexProp->GetGlowMap();
                break;
            case SCM_DEF_TEXSIZEBUMP:
                pMap = pTexProp->GetBumpMap();
                break;
            case SCM_DEF_TEXSIZENORMAL:
                pMap = pTexProp->GetNormalMap();
                break;
            case SCM_DEF_TEXSIZEPARALLAX:
                pMap = pTexProp->GetParallaxMap();
                break;
            case SCM_DEF_TEXSIZEDECAL:
                pMap = pTexProp->GetDecalMap(pEntry->GetExtra());
                break;
            case SCM_DEF_TEXSIZESHADER:
                pMap = pTexProp->GetShaderMap(pEntry->GetExtra());
                break;
            }

            if (pMap && pMap->GetTexture())
            {
                NiTexture* pTex = pMap->GetTexture();
                tempMatrix.r[0] = XMVectorSet((efd::Float32)pTex->GetWidth(), (efd::Float32)pTex->GetHeight(), 0.0f, 0.0f);
            }
            else
            {
                tempMatrix.r[0] = XMVectorZero();
            }
        }

        dataSize = 1 * 2 * sizeof(efd::Float32);
        dataStride = 1 * 2 * sizeof(efd::Float32);

        return &tempMatrix;
    }
    case SCM_DEF_ALPHA_TEST_FUNC:
    {
        const NiAlphaProperty* pAlphaProp = callContext.m_pkState->GetAlpha();
        // Fill this vector with 0 or 1 for these situations:
        // X: 1 if we should clip when the value is greater than the ref,
        //    0 otherwise.
        //    This value should be set to 1 for TEST_NEVER, TEST_LESS,
        //    TEST_EQUAL, and TEST_LESS_EQUAL.
        // Y: 1 if we should clip when the value is less than the ref,
        //    0 otherwise.
        //    This value should be set to 1 for TEST_NEVER, TEST_EQUAL,
        //    TEST_GREATER, and TEST_GREATER_EQUAL.
        // Z: 1 if we should clip when the value is equal to the ref,
        //    0 otherwise.
        //    This value should be set to 1 for TEST_NEVER, TEST_LESS,
        //    TEST_NOTEQUAL, and TEST_GREATER.
        efd::Point3 testConditions = efd::Point3::ZERO;
        if (pAlphaProp && pAlphaProp->GetAlphaTesting())
        {
            switch (pAlphaProp->GetTestMode())
            {
            case NiAlphaProperty::TEST_LESS:
                testConditions.x = 1.0f;
                testConditions.z = 1.0f;
                break;
            case NiAlphaProperty::TEST_EQUAL:
                testConditions.x = 1.0f;
                testConditions.y = 1.0f;
                break;
            case NiAlphaProperty::TEST_LESSEQUAL:
                testConditions.x = 1.0f;
                break;
            case NiAlphaProperty::TEST_GREATER:
                testConditions.y = 1.0f;
                testConditions.z = 1.0f;
                break;
            case NiAlphaProperty::TEST_NOTEQUAL:
                testConditions.z = 1.0f;
                break;
            case NiAlphaProperty::TEST_GREATEREQUAL:
                testConditions.y = 1.0f;
                break;
            case NiAlphaProperty::TEST_NEVER:
                testConditions.x = 1.0f;
                testConditions.y = 1.0f;
                testConditions.z = 1.0f;
                break;
            case NiAlphaProperty::TEST_ALWAYS:
            default:
                break;
            }
        }

        tempMatrix.r[0] = XMVectorSet(testConditions.x, testConditions.y, testConditions.z, 0.0f);

        dataSize = 1 * 3 * sizeof(efd::Float32);
        dataStride = 1 * 3 * sizeof(efd::Float32);

        return &tempMatrix;
    }
    case SCM_DEF_ALPHA_TEST_REF:
    {
        const NiAlphaProperty* pAlphaProp = callContext.m_pkState->GetAlpha();
        efd::Float32 alphaRef = 0.0f;
        if (pAlphaProp && pAlphaProp->GetAlphaTesting())
        {
            alphaRef = (efd::Float32)pAlphaProp->GetTestRef() / 255.0f;
        }

        tempMatrix.r[0] = XMVectorSet(alphaRef, 0.0f, 0.0f, 0.0f);

        dataSize = 1 * 1 * sizeof(efd::Float32);
        dataStride = 1 * 1 * sizeof(efd::Float32);

        return &tempMatrix;
    }

    case SCM_DEF_SKINWORLDVIEW:
    case SCM_DEF_INVSKINWORLDVIEW:
    case SCM_DEF_SKINWORLDVIEW_T:
    case SCM_DEF_INVSKINWORLDVIEW_T:
    case SCM_DEF_SKINWORLDVIEWPROJ:
    case SCM_DEF_INVSKINWORLDVIEWPROJ:
    case SCM_DEF_SKINWORLDVIEWPROJ_T:
    case SCM_DEF_INVSKINWORLDVIEWPROJ_T:
    case SCM_DEF_SKINWORLD:
    case SCM_DEF_INVSKINWORLD:
    case SCM_DEF_SKINWORLD_T:
    case SCM_DEF_INVSKINWORLD_T:
    case SCM_DEF_BONE_MATRIX_3:
    case SCM_DEF_BONE_MATRIX_4:
    case SCM_DEF_SKINBONE_MATRIX_4:
    {
        NiFixedString name;
        EE_VERIFY(NiShaderConstantMap::LookUpPredefinedMappingName(internalFlags, name));
        D3D11Error::ReportWarning(
            "The predefined mapping %s is deprecated\n", 
            (const efd::Char*)name);
        break;
    }
    default:
    {
        D3D11Error::ReportWarning(
            "Constant \"%s\" is not supported.\n",
            (const efd::Char*)pEntry->GetKey());
        return NULL;
    }
    }

    return NULL;
}

//------------------------------------------------------------------------------------------------
const void* D3D11ShaderConstantMap::ObtainConstantConstantValue(
    NiShaderConstantMapEntry* pEntry, 
    const NiRenderCallContext&,
    efd::UInt32& dataSize,
    efd::UInt32& dataStride,
    XMMATRIX& tempMatrix)
{
    dataSize = pEntry->GetDataSize();
    dataStride = pEntry->GetDataStride();

    if (pEntry->GetColumnMajor())
    {
        NiShaderAttributeDesc::AttributeType attribType = pEntry->GetAttributeType();

        // Only transpose Matrix3 and Matrix4
        if (attribType == NiShaderAttributeDesc::ATTRIB_TYPE_MATRIX3)
        {
            const efd::Float32* pConstantData = (const efd::Float32*)pEntry->GetDataSource();
            // Even though we are saving this in a matrix, the data is
            // expected to be a packed set of 9 floats. So we need
            // to treat the matrix as just an array of 16 floats, and
            // "transpose" the 3x3 matrix composed of the first 9 entries.

            efd::Float32* pTempMatrixAsFloatArray = (efd::Float32*)&tempMatrix;

            pTempMatrixAsFloatArray[0] = pConstantData[0];
            pTempMatrixAsFloatArray[3] = pConstantData[1];
            pTempMatrixAsFloatArray[6] = pConstantData[2];
            pTempMatrixAsFloatArray[1] = pConstantData[3];
            pTempMatrixAsFloatArray[4] = pConstantData[4];
            pTempMatrixAsFloatArray[7] = pConstantData[5];
            pTempMatrixAsFloatArray[2] = pConstantData[6];
            pTempMatrixAsFloatArray[5] = pConstantData[7];
            pTempMatrixAsFloatArray[8] = pConstantData[8];

            return pTempMatrixAsFloatArray;
        }
        else if (attribType == NiShaderAttributeDesc::ATTRIB_TYPE_MATRIX4)
        {
            const efd::Float32* pConstantData = (const efd::Float32*)pEntry->GetDataSource();
            tempMatrix.r[0] = XMVectorSet(pConstantData[0], pConstantData[4], pConstantData[8],  pConstantData[12]);
            tempMatrix.r[1] = XMVectorSet(pConstantData[1], pConstantData[5], pConstantData[9],  pConstantData[13]);
            tempMatrix.r[2] = XMVectorSet(pConstantData[2], pConstantData[6], pConstantData[10], pConstantData[14]);
            tempMatrix.r[3] = XMVectorSet(pConstantData[3], pConstantData[7], pConstantData[11], pConstantData[15]);

            return &tempMatrix;
        }
    }

    return pEntry->GetDataSource();
}

//------------------------------------------------------------------------------------------------
const void* D3D11ShaderConstantMap::ObtainAttributeConstantValue(
    NiShaderConstantMapEntry* pEntry, 
    const NiRenderCallContext& callContext,
    efd::Bool /*isGlobal*/,
    NiExtraData* /*pExtraData*/,
    efd::UInt32& dataSize,
    efd::UInt32& dataStride,
    XMMATRIX& tempMatrix)
{
    // Mesh will be NULL for Compute-only passes - Attribute constants not available
    if (callContext.m_pkMesh == NULL)
        return NULL;

    // Attempt to get the extra data for this attribute from the cache
    // rather than using strcmp.
    NiExtraData* pExtra = 0;
    // DT33847: Re-enable NiSCMExtraData usage
#if 0
    if (pExtraData)
    {
        NiSCMExtraData* pShaderData = (NiSCMExtraData*)pExtraData;
        pExtra = pShaderData->GetNextEntry(
            pEntry->GetShaderRegister(),
            callContext.m_uiPass, 
            GetProgramType(), 
            isGlobal);

        // Check for a match - it's possible for a mismatch to occur if
        // an entry had its shader register changed (such as when storing
        // HLSL shader constant registers after the NiSCMExtraData was
        // created.)
        if (!pExtra || (pExtra->GetName() != pEntry->GetKey()))
        {
            pExtra = callContext.m_pkMesh->GetExtraData(pEntry->GetKey());

            if (pExtra)
            {
                // If a new match was found, replace the original
                pShaderData->AddEntry(
                    pEntry->GetShaderRegister(),
                    callContext.m_uiPass, 
                    GetProgramType(), 
                    pExtra, 
                    isGlobal);
            }
        }
    }
    else if (!pExtra && callContext.m_pkMesh)
#endif
    {
        pExtra = callContext.m_pkMesh->GetExtraData(pEntry->GetKey());
    }

    // If extra data can't be found, use default value
    if (pExtra == NULL)
        return ObtainConstantConstantValue(pEntry, callContext, dataSize, dataStride, tempMatrix);

    dataSize = pEntry->GetDataSize();
    dataStride = pEntry->GetDataStride();

    NiShaderAttributeDesc::AttributeType attribType = pEntry->GetAttributeType();
    switch (attribType)
    {
    case NiShaderAttributeDesc::ATTRIB_TYPE_ARRAY:
    {
        // Get a pointer to the extra data and verify we have enough data
        // (more data is fine, it's just ignored, but less data could crash)
        if (NiIsExactKindOf(NiFloatsExtraData, pExtra))
        {
            efd::Float32* pValue;
            efd::UInt32 extraDataSize;

            NiFloatsExtraData* pFloatsED = (NiFloatsExtraData*)pExtra;
            pFloatsED->GetArray(extraDataSize, pValue);

            EE_ASSERT(extraDataSize * sizeof(efd::Float32) >= pEntry->GetDataSize());
            return pValue;
        }
        else if (NiIsExactKindOf(NiIntegersExtraData, pExtra))
        {
            efd::SInt32* pValue;
            efd::UInt32 extraDataSize;

            NiIntegersExtraData* pIntsED = (NiIntegersExtraData*)pExtra;
            pIntsED->GetArray(extraDataSize, pValue);

            EE_ASSERT(extraDataSize * sizeof(efd::SInt32) >= pEntry->GetDataSize());
            return pValue;
        }
    }
    case NiShaderAttributeDesc::ATTRIB_TYPE_BOOL:
    {
        NiBooleanExtraData* pBoolED = (NiBooleanExtraData*)pExtra;

        // Ugly nastiness!
        *((efd::SInt32*)tempMatrix.r) = pBoolED->GetValue();

        return (efd::Float32*)tempMatrix.r;
    }
    case NiShaderAttributeDesc::ATTRIB_TYPE_UNSIGNEDINT:
    {
        NiIntegerExtraData* pIntED = (NiIntegerExtraData*)pExtra;

        // Ugly nastiness!
        *((efd::SInt32*)tempMatrix.r) = pIntED->GetValue();

        return (efd::Float32*)tempMatrix.r;
    }
    case NiShaderAttributeDesc::ATTRIB_TYPE_FLOAT:
    {
        NiFloatExtraData* pFloatED = (NiFloatExtraData*)pExtra;

        tempMatrix.r[0] = XMVectorSet(pFloatED->GetValue(), 0.0f, 0.0f, 0.0f);

        return (efd::Float32*)tempMatrix.r;
    }
    case NiShaderAttributeDesc::ATTRIB_TYPE_POINT2:
    {
        NiFloatsExtraData* pFloatsED = (NiFloatsExtraData*)pExtra;

        efd::UInt32 arraySize;
        efd::Float32* pfValue;

        pFloatsED->GetArray(arraySize, pfValue);
        EE_ASSERT(arraySize >= 2);

        return pfValue;
    }
    case NiShaderAttributeDesc::ATTRIB_TYPE_POINT3:
    {
        NiFloatsExtraData* pFloatsED = (NiFloatsExtraData*)pExtra;

        efd::UInt32 arraySize;
        efd::Float32* pValue;

        pFloatsED->GetArray(arraySize, pValue);
        EE_ASSERT(arraySize >= 3);

        return pValue;
    }
    case NiShaderAttributeDesc::ATTRIB_TYPE_POINT4:
    {
        if (NiIsExactKindOf(NiFloatsExtraData, pExtra))
        {
            NiFloatsExtraData* pFloatsED = (NiFloatsExtraData*)pExtra;

            efd::UInt32 arraySize;
            efd::Float32* pValue;

            pFloatsED->GetArray(arraySize, pValue);
            EE_ASSERT(arraySize >= 4);

            return pValue;
        }
        else if (NiIsExactKindOf(NiColorExtraData, pExtra))
        {
            NiColorExtraData* pColorED = (NiColorExtraData*)pExtra;

            tempMatrix.r[0] = XMVectorSet(pColorED->GetRed(), pColorED->GetGreen(), pColorED->GetBlue(), pColorED->GetAlpha());

            return &tempMatrix;
        }
        else
        {
            D3D11Error::ReportWarning(
                "Entry %s in shader constant map %s failed in "
                __FUNCTION__
                " because the entry requires an extra data object of type NiFloatsExtraData "
                "or NiColorExtraData, but the extra data object %s is of type %s.",
                pEntry->GetKey(),
                GetName(),
                pExtra->GetName(),
                pExtra->GetRTTI()->GetName());
            return NULL;
        }
    }
    case NiShaderAttributeDesc::ATTRIB_TYPE_MATRIX3:
    {
        NiFloatsExtraData* pFloatsED = (NiFloatsExtraData*)pExtra;

        efd::UInt32 arraySize;
        efd::Float32* pValue;

        pFloatsED->GetArray(arraySize, pValue);
        EE_ASSERT(arraySize >= 9);

        EE_ASSERT(pEntry->GetDataSize() >= 9 * sizeof(efd::Float32));

        if (pEntry->GetColumnMajor())
        {
            // Transpose if necessary
            // Even though we are saving this in a matrix, the data is
            // expected to be a packed set of 9 floats. So we need
            // to treat the matrix as just an array of 16 floats, and
            // "transpose" the 3x3 matrix composed of the first 9 entries.

            efd::Float32* pTempMatrix = (efd::Float32*)&tempMatrix;

            pTempMatrix[0] = pValue[0];
            pTempMatrix[3] = pValue[1];
            pTempMatrix[6] = pValue[2];
            pTempMatrix[1] = pValue[3];
            pTempMatrix[4] = pValue[4];
            pTempMatrix[7] = pValue[5];
            pTempMatrix[2] = pValue[6];
            pTempMatrix[5] = pValue[7];
            pTempMatrix[8] = pValue[8];

            return pTempMatrix;
        }
        else
        {
            return pValue;
        }
    }
    case NiShaderAttributeDesc::ATTRIB_TYPE_MATRIX4:
    {
        NiFloatsExtraData* pFloatsED = (NiFloatsExtraData*)pExtra;

        efd::UInt32 arraySize;
        efd::Float32* pValue;

        pFloatsED->GetArray(arraySize, pValue);
        EE_ASSERT(arraySize >= 16);

        EE_ASSERT(pEntry->GetDataSize() >= 16 * sizeof(efd::Float32));

        if (pEntry->GetColumnMajor())
        {
            // Transpose if necessary
            tempMatrix.r[0] = XMVectorSet(pValue[0], pValue[4], pValue[8],  pValue[12]);
            tempMatrix.r[1] = XMVectorSet(pValue[1], pValue[5], pValue[9],  pValue[13]);
            tempMatrix.r[2] = XMVectorSet(pValue[2], pValue[6], pValue[10], pValue[14]);
            tempMatrix.r[3] = XMVectorSet(pValue[3], pValue[7], pValue[11], pValue[15]);

            return &tempMatrix;
        }
        else
        {
            return pValue;
        }
    }
    case NiShaderAttributeDesc::ATTRIB_TYPE_COLOR:
    {
        NiColorExtraData* pColorED = (NiColorExtraData*)pExtra;

        tempMatrix.r[0] = XMVectorSet(pColorED->GetRed(), pColorED->GetGreen(), pColorED->GetBlue(), pColorED->GetAlpha());

        return &tempMatrix;
    }
    case NiShaderAttributeDesc::ATTRIB_TYPE_FLOAT8:
    {
        NiFloatsExtraData* pFloatsED = (NiFloatsExtraData*)pExtra;

        efd::UInt32 arraySize;
        efd::Float32* pValue;

        pFloatsED->GetArray(arraySize, pValue);
        EE_ASSERT(arraySize >= 8);

        EE_ASSERT(pEntry->GetDataSize() >= 8 * sizeof(efd::Float32));

        return pValue;
    }
    case NiShaderAttributeDesc::ATTRIB_TYPE_FLOAT12:
    {
        NiFloatsExtraData* pFloatsED = (NiFloatsExtraData*)pExtra;

        efd::UInt32 arraySize;
        efd::Float32* pValue;

        pFloatsED->GetArray(arraySize, pValue);
        EE_ASSERT(arraySize >= 12);

        EE_ASSERT(pEntry->GetDataSize() >= 12 * sizeof(efd::Float32));

        return pValue;
    }
    }

    D3D11Error::ReportWarning(
        "Unsupported attribute type encountered in "
        __FUNCTION__
        " for entry %s in shader constant map %s.",
        pEntry->GetKey(),
        GetName());
    return NULL;
}

//------------------------------------------------------------------------------------------------
const void* D3D11ShaderConstantMap::ObtainGlobalConstantValue(
    NiShaderConstantMapEntry* pEntry, 
    const NiRenderCallContext&,
    efd::UInt32& dataSize,
    efd::UInt32& dataStride,
    XMMATRIX& tempMatrix)
{
    NiShaderFactory* pFactory = NiShaderFactory::GetInstance();
    NiGlobalConstantEntry* pGlobalEntry =
        pFactory->GetGlobalShaderConstantEntry(pEntry->GetKey());
    void* pDataSource = NULL;
    if (pGlobalEntry)
        pDataSource = pGlobalEntry->GetDataSource();
    else
        pDataSource = pEntry->GetDataSource();

    dataSize = pEntry->GetDataSize();
    dataStride = pEntry->GetDataStride();

    if (pEntry->GetColumnMajor())
    {
        NiShaderAttributeDesc::AttributeType attribType = pEntry->GetAttributeType();

        // Only transpose Matrix3 and Matrix4
        if (attribType == NiShaderAttributeDesc::ATTRIB_TYPE_MATRIX3)
        {
            const efd::Float32* pConstantData = (const efd::Float32*)pEntry->GetDataSource();

            // Even though we are saving this in a matrix, the data is
            // expected to be a packed set of 9 floats. So we need
            // to treat the matrix as just an array of 16 floats, and
            // "transpose" the 3x3 matrix composed of the first 9 entries.

            efd::Float32* pTempMatrix = (efd::Float32*)&tempMatrix;

            pTempMatrix[0] = pConstantData[0];
            pTempMatrix[3] = pConstantData[1];
            pTempMatrix[6] = pConstantData[2];
            pTempMatrix[1] = pConstantData[3];
            pTempMatrix[4] = pConstantData[4];
            pTempMatrix[7] = pConstantData[5];
            pTempMatrix[2] = pConstantData[6];
            pTempMatrix[5] = pConstantData[7];
            pTempMatrix[8] = pConstantData[8];

            return pTempMatrix;
        }
        else if (attribType == NiShaderAttributeDesc::ATTRIB_TYPE_MATRIX4)
        {
            const efd::Float32* pConstantData = (const efd::Float32*)pEntry->GetDataSource();
            tempMatrix.r[0] = XMVectorSet(pConstantData[0], pConstantData[4], pConstantData[8],  pConstantData[12]);
            tempMatrix.r[1] = XMVectorSet(pConstantData[1], pConstantData[5], pConstantData[9],  pConstantData[13]);
            tempMatrix.r[2] = XMVectorSet(pConstantData[2], pConstantData[6], pConstantData[10], pConstantData[14]);
            tempMatrix.r[3] = XMVectorSet(pConstantData[3], pConstantData[7], pConstantData[11], pConstantData[15]);

            return &tempMatrix;
        }
    }

    return pDataSource;
}

//------------------------------------------------------------------------------------------------
const void* D3D11ShaderConstantMap::ObtainOperatorConstantValue(
    NiShaderConstantMapEntry* pEntry, 
    const NiRenderCallContext& callContext,
    efd::Bool isGlobal, 
    NiExtraData* pExtraData, 
    efd::UInt32& dataSize,
    efd::UInt32& dataStride,
    XMMATRIX& tempMatrix)
{
    efd::UInt32 extra = pEntry->GetExtra();

    efd::UInt32 entry1 = extra & NiShaderConstantMapEntry::SCME_OPERATOR_ENTRY1_MASK;
    efd::UInt32 entry2 = (extra & NiShaderConstantMapEntry::SCME_OPERATOR_ENTRY2_MASK) >>
        NiShaderConstantMapEntry::SCME_OPERATOR_ENTRY2_SHIFT;
    efd::UInt32 operation = extra & NiShaderConstantMapEntry::SCME_OPERATOR_MASK;
    efd::Bool transpose = 
        (extra & NiShaderConstantMapEntry::SCME_OPERATOR_RESULT_TRANSPOSE) ? true : false;
    efd::Bool invert = 
        (extra & NiShaderConstantMapEntry::SCME_OPERATOR_RESULT_INVERSE) ? true : false;

    // If it's column major, then we must manually transpose.
    if (pEntry->GetColumnMajor())
        transpose = !transpose;

    // Grab the two entries
    NiShaderConstantMapEntry* pEntry1 = GetEntryAtIndex(entry1);
    NiShaderConstantMapEntry* pEntry2 = GetEntryAtIndex(entry2);

    if (!pEntry1 || !pEntry2)
    {
        NiShaderFactory::ReportError(NISHADERERR_UNKNOWN, false,
            "Invalid entries in OperatorConstant\n");
        return NULL;
    }

    // Determine the results data type and set it in the flags
    NiShaderAttributeDesc::AttributeType attribType1 = pEntry1->GetAttributeType();
    NiShaderAttributeDesc::AttributeType attribType2 = pEntry2->GetAttributeType();

    const void* pOperand1 = NULL;
    const void* pOperand2 = NULL;

    XMMATRIX operand1;
    XMMATRIX operand2;
    efd::UInt32 dataSizeOp1 = 0;
    efd::UInt32 dataStrideOp1 = 0;
    efd::UInt32 dataSizeOp2 = 0;
    efd::UInt32 dataStrideOp2 = 0;

    // Setup entry 1s value
    if (pEntry1->IsDefined())
    {
        attribType1 = LookUpPredefinedMappingType(pEntry1->GetKey());
        pOperand1 = ObtainDefinedConstantValue(
            pEntry1, 
            callContext, 
            dataSizeOp1, 
            dataStrideOp1, 
            operand1);
    }
    else if (pEntry1->IsGlobal())
    {
        pOperand1 = ObtainGlobalConstantValue(
            pEntry1, 
            callContext, 
            dataSizeOp1, 
            dataStrideOp1, 
            operand1);
    }
    else if (pEntry1->IsAttribute())
    {
        pOperand1 = ObtainAttributeConstantValue(
            pEntry1, 
            callContext, 
            isGlobal, 
            pExtraData, 
            dataSizeOp1, 
            dataStrideOp1, 
            operand1);
    }
    else if (pEntry1->IsConstant())
    {
        pOperand1 = ObtainConstantConstantValue(
            pEntry1, 
            callContext, 
            dataSizeOp1, 
            dataStrideOp1, 
            operand1);
    }

    if (pOperand1 == NULL)
    {
        D3D11Error::ReportWarning(
            "Failure obtaining data for operand 1 in "
            __FUNCTION__
            " for entry %s in shader constant map %s.",
            pEntry->GetKey(),
            GetName());

        return NULL;
    }

    // Setup entry 2s value
    if (pEntry2->IsDefined())
    {
        attribType2 = LookUpPredefinedMappingType(pEntry2->GetKey());
        pOperand2 = ObtainDefinedConstantValue(
            pEntry2, 
            callContext, 
            dataSizeOp2, 
            dataStrideOp2, 
            operand2);
    }
    else if (pEntry2->IsGlobal())
    {
        pOperand2 = ObtainGlobalConstantValue(
            pEntry2, 
            callContext, 
            dataSizeOp2, 
            dataStrideOp2, 
            operand2);
    }
    else if (pEntry2->IsAttribute())
    {
        pOperand2 = ObtainAttributeConstantValue(
            pEntry2, 
            callContext, 
            isGlobal,
            pExtraData, 
            dataSizeOp2, 
            dataStrideOp2, 
            operand2);
    }
    else if (pEntry2->IsConstant())
    {
        pOperand2 = ObtainConstantConstantValue(
            pEntry2, 
            callContext, 
            dataSizeOp2, 
            dataStrideOp2, 
            operand2);
    }

    if (pOperand2 == NULL)
    {
        D3D11Error::ReportWarning(
            "Failure obtaining data for operand 2 in "
            __FUNCTION__
            " for entry %s in shader constant map %s.",
            pEntry->GetKey(),
            GetName());
        return NULL;
    }

    if (attribType1 == NiShaderAttributeDesc::ATTRIB_TYPE_UNDEFINED)
    {
        D3D11Error::ReportWarning(
            "Undefined attribute type found for operand 1 in "
            __FUNCTION__
            " for entry %s in shader constant map %s.",
            pEntry->GetKey(),
            GetName());
        return NULL;
    }
    
    if (attribType2 == NiShaderAttributeDesc::ATTRIB_TYPE_UNDEFINED)
    {
        D3D11Error::ReportWarning(
            "Undefined attribute type found for operand 2 in "
            __FUNCTION__
            " for entry %s in shader constant map %s.",
            pEntry->GetKey(),
            GetName());
        return NULL;
    }

    // Perform the operation
    switch (operation)
    {
    case NiShaderConstantMapEntry::SCME_OPERATOR_MULTIPLY:
        return PerformOperatorMultiply(
            pOperand1, 
            attribType1, 
            pOperand2, 
            attribType2,
            invert, 
            transpose, 
            dataSize,
            dataStride,
            tempMatrix);
    case NiShaderConstantMapEntry::SCME_OPERATOR_DIVIDE:
        return PerformOperatorDivide(
            pOperand1, 
            attribType1, 
            pOperand2, 
            attribType2,
            invert, 
            transpose,
            dataSize,
            dataStride,
            tempMatrix);
    case NiShaderConstantMapEntry::SCME_OPERATOR_ADD:
        return PerformOperatorAdd(
            pOperand1, 
            attribType1, 
            pOperand2, 
            attribType2,
            invert, 
            transpose, 
            dataSize,
            dataStride,
            tempMatrix);
    case NiShaderConstantMapEntry::SCME_OPERATOR_SUBTRACT:
        return PerformOperatorSubtract(pOperand1, 
            attribType1, 
            pOperand2, 
            attribType2,
            invert, 
            transpose, 
            dataSize,
            dataStride,
            tempMatrix);
    }

    return NULL;
}

//------------------------------------------------------------------------------------------------
const void* D3D11ShaderConstantMap::ObtainObjectConstantValue(
    NiShaderConstantMapEntry* pEntry, 
    const NiRenderCallContext& callContext,
    efd::UInt32& dataSize,
    efd::UInt32& dataStride,
    XMMATRIX* pResult)
{
    // Get NiDynamicEffect corresponding to this object type.
    NiDynamicEffect* pDynEffect = GetDynamicEffectForObject(
        callContext.m_pkEffects,
        pEntry->GetObjectType(), 
        pEntry->GetExtra());

    // Get the register count for the mapping type.
    ObjectMappings eMapping = (ObjectMappings)
        ((pEntry->GetInternal() &NiShaderConstantMapEntry::SCME_OBJECT_MAP_MASK) >>
        NiShaderConstantMapEntry::SCME_OBJECT_MAP_SHIFT);
    efd::UInt32 registerCount;
    efd::UInt32 floatCount;
    NiShaderAttributeDesc::AttributeType attribType = LookUpObjectMappingType(
        eMapping, 
        registerCount, 
        floatCount);
    if (attribType == NiShaderAttributeDesc::ATTRIB_TYPE_UNDEFINED)
    {
        D3D11Error::ReportWarning(
            "Undefined attribute type found in "
            __FUNCTION__
            " for entry %s in shader constant map %s.",
            pEntry->GetKey(),
            GetName());

        return NULL;
    }

    // Get data to set.
    if (!ObtainDataFromDynamicEffect(
        pEntry, 
        eMapping, 
        pDynEffect, 
        callContext,
        dataSize,
        dataStride,
        pResult))
    {
        D3D11Error::ReportMessage(
            "Requested dynamic effect not found in "
            __FUNCTION__
            " for entry %s in shader constant map %s "
            " on mesh '%s, pointer: 0x%08X; "
            " Default value used instead.",
            pEntry->GetKey(),
            GetName(),
            callContext.m_pkMesh->GetName(),
            callContext.m_pkMesh);
    }
    return pResult;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11ShaderConstantMap::ObtainDataFromDynamicEffect(
    NiShaderConstantMapEntry* pEntry, 
    ObjectMappings objectMapping,
    NiDynamicEffect* pDynEffect, 
    const NiRenderCallContext& callContext,
    efd::UInt32& dataSize,
    efd::UInt32& dataStride,
    XMMATRIX* pResult)
{
    EE_ASSERT(callContext.m_pkWorld != NULL && 
        callContext.m_pkWorldBound != NULL &&
        callContext.m_pkMesh != NULL);
#ifdef EE_ASSERTS_ARE_ENABLED
    // Ensure that the data size matches the object type.
    efd::UInt32 registerCount;
    efd::UInt32 floatCount;
    NiShaderAttributeDesc::AttributeType attribType = LookUpObjectMappingType(
        objectMapping, 
        registerCount, 
        floatCount);
    EE_ASSERT(attribType != NiShaderAttributeDesc::ATTRIB_TYPE_UNDEFINED);
#endif

    switch (objectMapping)
    {
    case NiShaderConstantMap::SCM_OBJ_DIMMER:
        dataSize = 1 * 1 * sizeof(efd::Float32);
        dataStride = 1 * 1 * sizeof(efd::Float32);
        if (pDynEffect && pDynEffect->GetEffectType() != NiDynamicEffect::TEXTURE_EFFECT)
        {
            EE_ASSERT(NiIsKindOf(NiLight, pDynEffect));

            ((efd::Float32*)pResult->r)[0] = ((NiLight*)pDynEffect)->GetDimmer();
            return true;
        }
        else
        {
            ((efd::Float32*)pResult->r)[0] = 0.0f;
            return false;
        }
    case NiShaderConstantMap::SCM_OBJ_UNDIMMEDAMBIENT:
        dataSize = 1 * 4 * sizeof(efd::Float32);
        dataStride = 1 * 4 * sizeof(efd::Float32);
        if (pDynEffect && pDynEffect->GetEffectType() != NiDynamicEffect::TEXTURE_EFFECT)
        {
            EE_ASSERT(NiIsKindOf(NiLight, pDynEffect));

            const NiColor& ambientColor = ((NiLight*)pDynEffect)->GetAmbientColor();
            ((efd::Float32*)pResult->r)[0] = ambientColor.r;
            ((efd::Float32*)pResult->r)[1] = ambientColor.g;
            ((efd::Float32*)pResult->r)[2] = ambientColor.b;
            ((efd::Float32*)pResult->r)[3] = 1.0f;
            return true;
        }
        else
        {
            ((efd::Float32*)pResult->r)[0] = 0.0f;
            ((efd::Float32*)pResult->r)[1] = 0.0f;
            ((efd::Float32*)pResult->r)[2] = 0.0f;
            ((efd::Float32*)pResult->r)[3] = 1.0f;
            return false;
        }
    case NiShaderConstantMap::SCM_OBJ_UNDIMMEDDIFFUSE:
        dataSize = 1 * 4 * sizeof(efd::Float32);
        dataStride = 1 * 4 * sizeof(efd::Float32);
        if (pDynEffect &&
            pDynEffect->GetEffectType() != NiDynamicEffect::TEXTURE_EFFECT)
        {
            EE_ASSERT(NiIsKindOf(NiLight, pDynEffect));

            const NiColor& diffuseColor = ((NiLight*)pDynEffect)->GetDiffuseColor();
            ((efd::Float32*)pResult->r)[0] = diffuseColor.r;
            ((efd::Float32*)pResult->r)[1] = diffuseColor.g;
            ((efd::Float32*)pResult->r)[2] = diffuseColor.b;
            ((efd::Float32*)pResult->r)[3] = 1.0f;
            return true;
        }
        else
        {
            ((efd::Float32*)pResult->r)[0] = 0.0f;
            ((efd::Float32*)pResult->r)[1] = 0.0f;
            ((efd::Float32*)pResult->r)[2] = 0.0f;
            ((efd::Float32*)pResult->r)[3] = 1.0f;
            return false;
        }
    case NiShaderConstantMap::SCM_OBJ_UNDIMMEDSPECULAR:
        dataSize = 1 * 4 * sizeof(efd::Float32);
        dataStride = 1 * 4 * sizeof(efd::Float32);
        if (pDynEffect &&
            pDynEffect->GetEffectType() != NiDynamicEffect::TEXTURE_EFFECT)
        {
            EE_ASSERT(NiIsKindOf(NiLight, pDynEffect));

            const NiColor& specularColor = ((NiLight*)pDynEffect)->GetSpecularColor();
            ((efd::Float32*)pResult->r)[0] = specularColor.r;
            ((efd::Float32*)pResult->r)[1] = specularColor.g;
            ((efd::Float32*)pResult->r)[2] = specularColor.b;
            ((efd::Float32*)pResult->r)[3] = 1.0f;
            return true;
        }
        else
        {
            ((efd::Float32*)pResult->r)[0] = 0.0f;
            ((efd::Float32*)pResult->r)[1] = 0.0f;
            ((efd::Float32*)pResult->r)[2] = 0.0f;
            ((efd::Float32*)pResult->r)[3] = 1.0f;
            return false;
        }
    case NiShaderConstantMap::SCM_OBJ_AMBIENT:
        dataSize = 1 * 4 * sizeof(efd::Float32);
        dataStride = 1 * 4 * sizeof(efd::Float32);
        if (pDynEffect && pDynEffect->GetEffectType() != NiDynamicEffect::TEXTURE_EFFECT)
        {
            EE_ASSERT(NiIsKindOf(NiLight, pDynEffect));

            NiColor amientColor = ((NiLight*)pDynEffect)->GetAmbientColor();
            amientColor *= ((NiLight*)pDynEffect)->GetDimmer();
            ((efd::Float32*)pResult->r)[0] = amientColor.r;
            ((efd::Float32*)pResult->r)[1] = amientColor.g;
            ((efd::Float32*)pResult->r)[2] = amientColor.b;
            ((efd::Float32*)pResult->r)[3] = 1.0f;
            return true;
        }
        else
        {
            ((efd::Float32*)pResult->r)[0] = 0.0f;
            ((efd::Float32*)pResult->r)[1] = 0.0f;
            ((efd::Float32*)pResult->r)[2] = 0.0f;
            ((efd::Float32*)pResult->r)[3] = 1.0f;
            return false;
        }
    case NiShaderConstantMap::SCM_OBJ_DIFFUSE:
        dataSize = 1 * 4 * sizeof(efd::Float32);
        dataStride = 1 * 4 * sizeof(efd::Float32);
        if (pDynEffect && pDynEffect->GetEffectType() != NiDynamicEffect::TEXTURE_EFFECT)
        {
            EE_ASSERT(NiIsKindOf(NiLight, pDynEffect));

            NiColor diffuseColor = ((NiLight*)pDynEffect)->GetDiffuseColor();
            diffuseColor *= ((NiLight*)pDynEffect)->GetDimmer();
            ((efd::Float32*)pResult->r)[0] = diffuseColor.r;
            ((efd::Float32*)pResult->r)[1] = diffuseColor.g;
            ((efd::Float32*)pResult->r)[2] = diffuseColor.b;
            ((efd::Float32*)pResult->r)[3] = 1.0f;
            return true;
        }
        else
        {
            ((efd::Float32*)pResult->r)[0] = 0.0f;
            ((efd::Float32*)pResult->r)[1] = 0.0f;
            ((efd::Float32*)pResult->r)[2] = 0.0f;
            ((efd::Float32*)pResult->r)[3] = 1.0f;
            return false;
        }
    case NiShaderConstantMap::SCM_OBJ_SPECULAR:
        dataSize = 1 * 4 * sizeof(efd::Float32);
        dataStride = 1 * 4 * sizeof(efd::Float32);
        if (pDynEffect && pDynEffect->GetEffectType() != NiDynamicEffect::TEXTURE_EFFECT)
        {
            EE_ASSERT(NiIsKindOf(NiLight, pDynEffect));

            NiColor specularColor = ((NiLight*)pDynEffect)->GetSpecularColor();
            specularColor *= ((NiLight*)pDynEffect)->GetDimmer();
            ((efd::Float32*)pResult->r)[0] = specularColor.r;
            ((efd::Float32*)pResult->r)[1] = specularColor.g;
            ((efd::Float32*)pResult->r)[2] = specularColor.b;
            ((efd::Float32*)pResult->r)[3] = 1.0f;
            return true;
        }
        else
        {
            ((efd::Float32*)pResult->r)[0] = 0.0f;
            ((efd::Float32*)pResult->r)[1] = 0.0f;
            ((efd::Float32*)pResult->r)[2] = 0.0f;
            ((efd::Float32*)pResult->r)[3] = 1.0f;
            return false;
        }
    case NiShaderConstantMap::SCM_OBJ_WORLDPOSITION:
        dataSize = 1 * 4 * sizeof(efd::Float32);
        dataStride = 1 * 4 * sizeof(efd::Float32);
        if (pDynEffect)
        {
            if (pDynEffect->GetEffectType() == NiDynamicEffect::DIR_LIGHT ||
                pDynEffect->GetEffectType() == NiDynamicEffect::SHADOWDIR_LIGHT)
            {
                efd::Point3 pos =
                    ((NiDirectionalLight*)pDynEffect)->GetWorldDirection() * ms_dirLightDistance;
                ((efd::Float32*)pResult->r)[0] = pos.x;
                ((efd::Float32*)pResult->r)[1] = pos.y;
                ((efd::Float32*)pResult->r)[2] = pos.z;
                ((efd::Float32*)pResult->r)[3] = 1.0f;
            }
            else
            {
                ((efd::Float32*)pResult->r)[0] = pDynEffect->GetWorldTranslate().x;
                ((efd::Float32*)pResult->r)[1] = pDynEffect->GetWorldTranslate().y;
                ((efd::Float32*)pResult->r)[2] = pDynEffect->GetWorldTranslate().z;
                ((efd::Float32*)pResult->r)[3] = 1.0f;
            }
            return true;
        }
        else
        {
            ((efd::Float32*)pResult->r)[0] = 0.0f;
            ((efd::Float32*)pResult->r)[1] = 0.0f;
            ((efd::Float32*)pResult->r)[2] = 0.0f;
            ((efd::Float32*)pResult->r)[3] = 1.0f;
            return false;
        }
    case NiShaderConstantMap::SCM_OBJ_MODELPOSITION:
        dataSize = 1 * 4 * sizeof(efd::Float32);
        dataStride = 1 * 4 * sizeof(efd::Float32);
        if (pDynEffect)
        {
            NiTransform invWorld;
            callContext.m_pkWorld->Invert(invWorld);

            efd::Point3 pos;
            if (pDynEffect->GetEffectType() == NiDynamicEffect::DIR_LIGHT ||
                pDynEffect->GetEffectType() == NiDynamicEffect::SHADOWDIR_LIGHT)
            {
                pos = invWorld *
                    (((NiDirectionalLight*)pDynEffect)->GetWorldDirection() * ms_dirLightDistance);
            }
            else
            {
                pos = invWorld * pDynEffect->GetWorldTranslate();
            }

            ((efd::Float32*)pResult->r)[0] = pos.x;
            ((efd::Float32*)pResult->r)[1] = pos.y;
            ((efd::Float32*)pResult->r)[2] = pos.z;
            ((efd::Float32*)pResult->r)[3] = 1.0f;
            return true;
        }
        else
        {
            ((efd::Float32*)pResult->r)[0] = 0.0f;
            ((efd::Float32*)pResult->r)[1] = 0.0f;
            ((efd::Float32*)pResult->r)[2] = 0.0f;
            ((efd::Float32*)pResult->r)[3] = 1.0f;
            return false;
        }
    case NiShaderConstantMap::SCM_OBJ_WORLDDIRECTION:
        dataSize = 1 * 4 * sizeof(efd::Float32);
        dataStride = 1 * 4 * sizeof(efd::Float32);
        if (pDynEffect && pDynEffect->GetEffectType() != NiDynamicEffect::TEXTURE_EFFECT)
        {
            EE_ASSERT(NiIsKindOf(NiLight, pDynEffect));

            efd::Point3 dir;
            if (pDynEffect->GetEffectType() == NiDynamicEffect::POINT_LIGHT ||
                pDynEffect->GetEffectType() == NiDynamicEffect::SHADOWPOINT_LIGHT)
            {
                // Get the normalized vector from the light to the
                // center of the bounding volume of the rendered object
                // in world space.
                dir = callContext.m_pkWorldBound->GetCenter() - pDynEffect->GetWorldTranslate();
                dir.Unitize();
            }
            else if (pDynEffect->GetEffectType() == NiDynamicEffect::DIR_LIGHT ||
                pDynEffect->GetEffectType() == NiDynamicEffect::SHADOWDIR_LIGHT)
            {
                dir = ((NiDirectionalLight*)pDynEffect)->GetWorldDirection();
            }
            else
            {
                EE_ASSERT(NiIsExactKindOf(NiSpotLight, pDynEffect));
                dir = ((NiSpotLight*)pDynEffect)->GetWorldDirection();
            }

            ((efd::Float32*)pResult->r)[0] = dir.x;
            ((efd::Float32*)pResult->r)[1] = dir.y;
            ((efd::Float32*)pResult->r)[2] = dir.z;
            ((efd::Float32*)pResult->r)[3] = 1.0f;
            return true;
        }
        else
        {
            ((efd::Float32*)pResult->r)[0] = 1.0f;
            ((efd::Float32*)pResult->r)[1] = 0.0f;
            ((efd::Float32*)pResult->r)[2] = 0.0f;
            ((efd::Float32*)pResult->r)[3] = 1.0f;
            return false;
        }
    case NiShaderConstantMap::SCM_OBJ_MODELDIRECTION:
        dataSize = 1 * 4 * sizeof(efd::Float32);
        dataStride = 1 * 4 * sizeof(efd::Float32);
        if (pDynEffect && pDynEffect->GetEffectType() != NiDynamicEffect::TEXTURE_EFFECT)
        {
            EE_ASSERT(NiIsKindOf(NiLight, pDynEffect));

            efd::Point3 direction;
            if (pDynEffect->GetEffectType() == NiDynamicEffect::POINT_LIGHT ||
                pDynEffect->GetEffectType() ==
                NiDynamicEffect::SHADOWPOINT_LIGHT)
            {
                // Get the normalized vector from the light to the
                // center of the bounding volume of the rendered object
                // in world space.
                direction = callContext.m_pkWorldBound->GetCenter() -
                    pDynEffect->GetWorldTranslate();
                direction.Unitize();
            }
            else if (pDynEffect->GetEffectType() ==
                NiDynamicEffect::DIR_LIGHT ||
                pDynEffect->GetEffectType() ==
                NiDynamicEffect::SHADOWDIR_LIGHT)
            {
                direction = ((NiDirectionalLight*)pDynEffect)->GetWorldDirection();
            }
            else
            {
                EE_ASSERT(NiIsKindOf(NiSpotLight, pDynEffect));
                direction = ((NiSpotLight*)pDynEffect)->GetWorldDirection();
            }

            // Convert direction vector to rendered object's model space.
            direction = callContext.m_pkWorld->m_Rotate.Transpose() * direction;

            ((efd::Float32*)pResult->r)[0] = direction.x;
            ((efd::Float32*)pResult->r)[1] = direction.y;
            ((efd::Float32*)pResult->r)[2] = direction.z;
            ((efd::Float32*)pResult->r)[3] = 1.0f;
            return true;
        }
        else
        {
            ((efd::Float32*)pResult->r)[0] = 1.0f;
            ((efd::Float32*)pResult->r)[1] = 0.0f;
            ((efd::Float32*)pResult->r)[2] = 0.0f;
            ((efd::Float32*)pResult->r)[3] = 1.0f;
            return false;
        }
    case NiShaderConstantMap::SCM_OBJ_WORLDTRANSFORM:
        dataSize = 4 * 4 * sizeof(efd::Float32);
        dataStride = 4 * 4 * sizeof(efd::Float32);
        if (pDynEffect)
        {
            if (pDynEffect->GetEffectType() == NiDynamicEffect::DIR_LIGHT ||
                pDynEffect->GetEffectType() == NiDynamicEffect::SHADOWDIR_LIGHT)
            {
                efd::Point3 translation =
                    ((NiDirectionalLight*)pDynEffect)->GetWorldDirection() * ms_dirLightDistance;
                // If it's column major, then we must manually transpose.
                if (pEntry->GetColumnMajor())
                {
                    D3D11Utility::GetD3DTransposeFromNi(
                        *pResult,
                        pDynEffect->GetWorldRotate(), 
                        translation,
                        pDynEffect->GetWorldScale());
                }
                else
                {
                    D3D11Utility::GetD3DFromNi(
                        *pResult,
                        pDynEffect->GetWorldRotate(), 
                        translation,
                        pDynEffect->GetWorldScale());
                }
            }
            else
            {
                // If it's column major, then we must manually transpose.
                if (pEntry->GetColumnMajor())
                {
                    D3D11Utility::GetD3DTransposeFromNi(
                        *pResult,
                        pDynEffect->GetWorldTransform());
                }
                else
                {
                    D3D11Utility::GetD3DFromNi(
                        *pResult,
                        pDynEffect->GetWorldTransform());
                }
            }
            return true;
        }
        else
        {
            *pResult = XMMatrixIdentity();
            return false;
        }
    case NiShaderConstantMap::SCM_OBJ_MODELTRANSFORM:
        dataSize = 4 * 4 * sizeof(efd::Float32);
        dataStride = 4 * 4 * sizeof(efd::Float32);
        if (pDynEffect)
        {
            NiTransform invWorld;
            callContext.m_pkWorld->Invert(invWorld);

            if (pDynEffect->GetEffectType() == NiDynamicEffect::DIR_LIGHT ||
                pDynEffect->GetEffectType() ==
                NiDynamicEffect::SHADOWDIR_LIGHT)
            {
                NiTransform dynEffectWorld = pDynEffect->GetWorldTransform();
                dynEffectWorld.m_Translate = 
                    ((NiDirectionalLight*)pDynEffect)->GetWorldDirection() * ms_dirLightDistance;
                // If it's column major, then we must manually transpose.
                if (pEntry->GetColumnMajor())
                {
                    D3D11Utility::GetD3DTransposeFromNi(*pResult, invWorld * dynEffectWorld);
                }
                else
                {
                    D3D11Utility::GetD3DFromNi(*pResult, invWorld * dynEffectWorld);
                }
            }
            else
            {
                // If it's column major, then we must manually transpose.
                if (pEntry->GetColumnMajor())
                {
                    D3D11Utility::GetD3DTransposeFromNi(
                        *pResult, 
                        invWorld * pDynEffect->GetWorldTransform());
                }
                else
                {
                    D3D11Utility::GetD3DFromNi(
                        *pResult, 
                        invWorld * pDynEffect->GetWorldTransform());
                }
            }
            return true;
        }
        else
        {
            *pResult = XMMatrixIdentity();
            return false;
        }
    case NiShaderConstantMap::SCM_OBJ_SPOTATTENUATION:
        dataSize = 1 * 4 * sizeof(efd::Float32);
        dataStride = 1 * 4 * sizeof(efd::Float32);
        if (pDynEffect && (pDynEffect->GetEffectType() == NiDynamicEffect::SPOT_LIGHT ||
            pDynEffect->GetEffectType() == NiDynamicEffect::SHADOWSPOT_LIGHT))
        {
            EE_ASSERT(NiIsKindOf(NiSpotLight, pDynEffect));

            ((efd::Float32*)pResult->r)[0] = efd::Cos(
                ((NiSpotLight*)pDynEffect)->GetInnerSpotAngle() * efd::EE_PI / 180.0f);
            ((efd::Float32*)pResult->r)[1] = efd::Cos(
                ((NiSpotLight*)pDynEffect)->GetSpotAngle() * efd::EE_PI / 180.0f);
            ((efd::Float32*)pResult->r)[2] = ((NiSpotLight*)pDynEffect)->GetSpotExponent();
            ((efd::Float32*)pResult->r)[3] = 0.0f;
            return true;
        }
        else
        {
            ((efd::Float32*)pResult->r)[0] = -1.0f;
            ((efd::Float32*)pResult->r)[1] = 0.0f;
            ((efd::Float32*)pResult->r)[2] = 0.0f;
            ((efd::Float32*)pResult->r)[3] = 0.0f;
            return false;
        }
    case NiShaderConstantMap::SCM_OBJ_ATTENUATION:
        dataSize = 1 * 4 * sizeof(efd::Float32);
        dataStride = 1 * 4 * sizeof(efd::Float32);
        if (pDynEffect && (pDynEffect->GetEffectType() == NiDynamicEffect::POINT_LIGHT ||
            pDynEffect->GetEffectType() == NiDynamicEffect::SHADOWPOINT_LIGHT ||
            pDynEffect->GetEffectType() == NiDynamicEffect::DIR_LIGHT ||
            pDynEffect->GetEffectType() == NiDynamicEffect::SHADOWDIR_LIGHT ||
            pDynEffect->GetEffectType() == NiDynamicEffect::SPOT_LIGHT ||
            pDynEffect->GetEffectType() == NiDynamicEffect::SHADOWSPOT_LIGHT))
        {
            EE_ASSERT(NiIsKindOf(NiLight, pDynEffect));

            ((efd::Float32*)pResult->r)[0] = 1.0f;
            ((efd::Float32*)pResult->r)[1] = ((NiLight*)pDynEffect)->GetFalloff();
            ((efd::Float32*)pResult->r)[2] = 1.0f;
            ((efd::Float32*)pResult->r)[3] = ((NiLight*)pDynEffect)->GetRange();

            return true;
        }
        else
        {
            ((efd::Float32*)pResult->r)[0] = 1.0f;
            ((efd::Float32*)pResult->r)[1] = 1.0f;
            ((efd::Float32*)pResult->r)[2] = 1.0f;
            ((efd::Float32*)pResult->r)[3] = 0.0f;
            return false;
        }
    case NiShaderConstantMap::SCM_OBJ_WORLDPROJECTIONMATRIX:
        dataSize = 4 * 4 * sizeof(efd::Float32);
        dataStride = 4 * 4 * sizeof(efd::Float32);
        if (pDynEffect && pDynEffect->GetEffectType() == NiDynamicEffect::TEXTURE_EFFECT)
        {
            EE_ASSERT(NiIsKindOf(NiTextureEffect, pDynEffect));

            // If it's column major, then we must manually transpose.
            if (pEntry->GetColumnMajor())
            {
                D3D11Utility::GetD3DTransposeFromNi(
                    *pResult,
                    ((NiTextureEffect*)pDynEffect)->GetWorldProjectionMatrix(), 
                    efd::Point3::ZERO, 
                    1.0f);
            }
            else
            {
                D3D11Utility::GetD3DFromNi(
                    *pResult,
                    ((NiTextureEffect*)pDynEffect)->GetWorldProjectionMatrix(), 
                    efd::Point3::ZERO, 
                    1.0f);
            }
            return true;
        }
        else
        {
            *pResult = XMMatrixIdentity();
            return false;
        }
    case NiShaderConstantMap::SCM_OBJ_MODELPROJECTIONMATRIX:
        dataSize = 4 * 4 * sizeof(efd::Float32);
        dataStride = 4 * 4 * sizeof(efd::Float32);
        if (pDynEffect && pDynEffect->GetEffectType() == NiDynamicEffect::TEXTURE_EFFECT)
        {
            EE_ASSERT(NiIsKindOf(NiTextureEffect, pDynEffect));

            // If it's column major, then we must manually transpose.
            if (pEntry->GetColumnMajor())
            {
                D3D11Utility::GetD3DTransposeFromNi(
                    *pResult,
                    callContext.m_pkWorld->m_Rotate.Transpose() * 
                    ((NiTextureEffect*)pDynEffect)->GetWorldProjectionMatrix(), 
                    efd::Point3::ZERO, 
                    1.0f);
            }
            else
            {
                D3D11Utility::GetD3DFromNi(
                    *pResult,
                    callContext.m_pkWorld->m_Rotate.Transpose() *
                    ((NiTextureEffect*)pDynEffect)->GetWorldProjectionMatrix(), 
                    efd::Point3::ZERO, 
                    1.0f);
            }
            return true;
        }
        else
        {
            *pResult = XMMatrixIdentity();
            return false;
        }
    case NiShaderConstantMap::SCM_OBJ_WORLDPROJECTIONTRANSLATION:
        dataSize = 1 * 4 * sizeof(efd::Float32);
        dataStride = 1 * 4 * sizeof(efd::Float32);
        if (pDynEffect && pDynEffect->GetEffectType() == NiDynamicEffect::TEXTURE_EFFECT)
        {
            EE_ASSERT(NiIsKindOf(NiTextureEffect, pDynEffect));

            const efd::Point3& translation = 
                ((NiTextureEffect*)pDynEffect)->GetWorldProjectionTranslation();

            ((efd::Float32*)pResult->r)[0] = translation.x;
            ((efd::Float32*)pResult->r)[1] = translation.y;
            ((efd::Float32*)pResult->r)[2] = translation.z;
            ((efd::Float32*)pResult->r)[3] = 1.0f;
            return true;
        }
        else
        {
            ((efd::Float32*)pResult->r)[0] = 0.0f;
            ((efd::Float32*)pResult->r)[1] = 0.0f;
            ((efd::Float32*)pResult->r)[2] = 0.0f;
            ((efd::Float32*)pResult->r)[3] = 1.0f;
            return false;
        }
    case NiShaderConstantMap::SCM_OBJ_MODELPROJECTIONTRANSLATION:
        dataSize = 1 * 4 * sizeof(efd::Float32);
        dataStride = 1 * 4 * sizeof(efd::Float32);
        if (pDynEffect && pDynEffect->GetEffectType() == NiDynamicEffect::TEXTURE_EFFECT)
        {
            EE_ASSERT(NiIsKindOf(NiTextureEffect, pDynEffect));

            NiTransform invWorld;
            callContext.m_pkWorld->Invert(invWorld);

            efd::Point3 translation = 
                ((NiTextureEffect*)pDynEffect)->GetWorldProjectionTranslation();
            translation = invWorld * translation;

            ((efd::Float32*)pResult->r)[0] = translation.x;
            ((efd::Float32*)pResult->r)[1] = translation.y;
            ((efd::Float32*)pResult->r)[2] = translation.z;
            ((efd::Float32*)pResult->r)[3] = 1.0f;
            return true;
        }
        else
        {
            ((efd::Float32*)pResult->r)[0] = 0.0f;
            ((efd::Float32*)pResult->r)[1] = 0.0f;
            ((efd::Float32*)pResult->r)[2] = 0.0f;
            ((efd::Float32*)pResult->r)[3] = 1.0f;
            return false;
        }
    case NiShaderConstantMap::SCM_OBJ_WORLDCLIPPINGPLANE:
        dataSize = 1 * 4 * sizeof(efd::Float32);
        dataStride = 1 * 4 * sizeof(efd::Float32);
        if (pDynEffect && pDynEffect->GetEffectType() == NiDynamicEffect::TEXTURE_EFFECT)
        {
            EE_ASSERT(NiIsKindOf(NiTextureEffect, pDynEffect));

            NiTextureEffect* pTexEffect = (NiTextureEffect*) pDynEffect;
            if (pTexEffect->GetClippingPlaneEnable())
            {
                const NiPlane& plane = pTexEffect->GetWorldClippingPlane();
                ((efd::Float32*)pResult->r)[0] = plane.GetNormal().x;
                ((efd::Float32*)pResult->r)[1] = plane.GetNormal().y;
                ((efd::Float32*)pResult->r)[2] = plane.GetNormal().z;
                ((efd::Float32*)pResult->r)[3] = plane.GetConstant();
            }
            else
            {
                ((efd::Float32*)pResult->r)[0] = 0.0f;
                ((efd::Float32*)pResult->r)[1] = 0.0f;
                ((efd::Float32*)pResult->r)[2] = 0.0f;
                ((efd::Float32*)pResult->r)[3] = 0.0f;
            }
            return true;
        }
        else
        {
            ((efd::Float32*)pResult->r)[0] = 0.0f;
            ((efd::Float32*)pResult->r)[1] = 0.0f;
            ((efd::Float32*)pResult->r)[2] = 0.0f;
            ((efd::Float32*)pResult->r)[3] = 0.0f;
            return false;
        }
    case NiShaderConstantMap::SCM_OBJ_MODELCLIPPINGPLANE:
        dataSize = 1 * 4 * sizeof(efd::Float32);
        dataStride = 1 * 4 * sizeof(efd::Float32);
        if (pDynEffect && pDynEffect->GetEffectType() == NiDynamicEffect::TEXTURE_EFFECT)
        {
            EE_ASSERT(NiIsKindOf(NiTextureEffect, pDynEffect));

            NiTextureEffect* pTexEffect = (NiTextureEffect*) pDynEffect;
            if (pTexEffect->GetClippingPlaneEnable())
            {
                NiTransform invWorld;
                callContext.m_pkWorld->Invert(invWorld);

                NiPlane kPlane = pTexEffect->GetWorldClippingPlane();
                efd::Point3 msNormal = invWorld.m_Rotate * kPlane.GetNormal();
                efd::Point3 msPoint = invWorld * (kPlane.GetNormal() * kPlane.GetConstant());
                efd::Float32 msConstant = msNormal * msPoint;

                ((efd::Float32*)pResult->r)[0] = msNormal.x;
                ((efd::Float32*)pResult->r)[1] = msNormal.y;
                ((efd::Float32*)pResult->r)[2] = msNormal.z;
                ((efd::Float32*)pResult->r)[3] = msConstant;
            }
            else
            {
                ((efd::Float32*)pResult->r)[0] = 0.0f;
                ((efd::Float32*)pResult->r)[1] = 0.0f;
                ((efd::Float32*)pResult->r)[2] = 0.0f;
                ((efd::Float32*)pResult->r)[3] = 0.0f;
            }
            return true;
        }
        else
        {
            ((efd::Float32*)pResult->r)[0] = 0.0f;
            ((efd::Float32*)pResult->r)[1] = 0.0f;
            ((efd::Float32*)pResult->r)[2] = 0.0f;
            ((efd::Float32*)pResult->r)[3] = 0.0f;
            return false;
        }
    case NiShaderConstantMap::SCM_OBJ_TEXCOORDGEN:
        dataSize = 1 * 1 * sizeof(efd::Float32);
        dataStride = 1 * 1 * sizeof(efd::Float32);
        if (pDynEffect && pDynEffect->GetEffectType() == NiDynamicEffect::TEXTURE_EFFECT)
        {
            EE_ASSERT(NiIsKindOf(NiTextureEffect, pDynEffect));

            efd::Float32 texCoordGen = 0.0f;
            NiTextureEffect::CoordGenType texCoordGenType = 
                ((NiTextureEffect*)pDynEffect)->GetTextureCoordGen();
            switch (texCoordGenType)
            {
            case NiTextureEffect::WORLD_PARALLEL:
                // D3DTSS_TCI_CAMERASPACEPOSITION
                texCoordGen = 2.0f;
                break;
            case NiTextureEffect::WORLD_PERSPECTIVE:
                // D3DTSS_TCI_CAMERASPACEPOSITION
                texCoordGen = 2.0f;
                break;
            case NiTextureEffect::SPHERE_MAP:
                // D3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR
                texCoordGen = 3.0f;
                break;
            case NiTextureEffect::SPECULAR_CUBE_MAP:
                // D3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR
                texCoordGen = 3.0f;
                break;
            case NiTextureEffect::DIFFUSE_CUBE_MAP:
                // D3DTSS_TCI_CAMERASPACENORMAL
                texCoordGen = 1.0f;
                break;
            default:
                break;
            }

            ((efd::Float32*)pResult->r)[0] = texCoordGen;
            return true;
        }
        else
        {
            ((efd::Float32*)pResult->r)[0] = 0.0f;
            return false;
        }
    case NiShaderConstantMap::SCM_OBJ_WORLDPROJECTIONTRANSFORM:
        dataSize = 4 * 4 * sizeof(efd::Float32);
        dataStride = 4 * 4 * sizeof(efd::Float32);
        if (pDynEffect && pDynEffect->GetEffectType() == NiDynamicEffect::TEXTURE_EFFECT)
        {
            EE_ASSERT(NiIsKindOf(NiTextureEffect, pDynEffect));

            NiTextureEffect* pTexEffect = (NiTextureEffect*)pDynEffect;
            // If it's column major, then we must manually transpose.
            if (pEntry->GetColumnMajor())
            {
                D3D11Utility::GetD3DTransposeFromNi(
                    *pResult,
                    pTexEffect->GetWorldProjectionMatrix(),
                    pTexEffect->GetWorldProjectionTranslation(), 
                    1.0f);
            }
            else
            {
                D3D11Utility::GetD3DFromNi(
                    *pResult,
                    pTexEffect->GetWorldProjectionMatrix(),
                    pTexEffect->GetWorldProjectionTranslation(), 
                    1.0f);
            }
            return true;
        }
        else
        {
            *pResult = XMMatrixIdentity();
            return false;
        }
    case NiShaderConstantMap::SCM_OBJ_MODELPROJECTIONTRANSFORM:
        dataSize = 4 * 4 * sizeof(efd::Float32);
        dataStride = 4 * 4 * sizeof(efd::Float32);
        if (pDynEffect && pDynEffect->GetEffectType() == NiDynamicEffect::TEXTURE_EFFECT)
        {
            EE_ASSERT(NiIsKindOf(NiTextureEffect, pDynEffect));

            NiTextureEffect* pTexEffect = (NiTextureEffect*)pDynEffect;
            NiTransform invWorld;
            callContext.m_pkWorld->Invert(invWorld);

            efd::Point3 translation = 
                ((NiTextureEffect*)pDynEffect)->GetWorldProjectionTranslation();
            translation = invWorld * translation;
            // If it's column major, then we must manually transpose.
            if (pEntry->GetColumnMajor())
            {
                D3D11Utility::GetD3DTransposeFromNi(
                    *pResult,
                    callContext.m_pkWorld->m_Rotate.Transpose() *
                    pTexEffect->GetWorldProjectionMatrix(),
                    translation, 
                    1.0f);
            }
            else
            {
                D3D11Utility::GetD3DFromNi(
                    *pResult,
                    callContext.m_pkWorld->m_Rotate.Transpose() *
                    pTexEffect->GetWorldProjectionMatrix(),
                    translation, 
                    1.0f);
            }
            return true;
        }
        else
        {
            *pResult = XMMatrixIdentity();
            return false;
        }

    case NiShaderConstantMap::SCM_OBJ_WORLDTOSHADOWMAPMATRIX:
        dataSize = 4 * 4 * sizeof(efd::Float32);
        dataStride = 4 * 4 * sizeof(efd::Float32);
        if (pDynEffect)
        {
            EE_ASSERT(NiIsKindOf(NiLight, pDynEffect));

            NiShadowGenerator* pGenerator =
                pDynEffect->GetShadowGenerator();
            EE_ASSERT(pGenerator);

            NiShadowMap* pShadowMap = pGenerator->RetrieveShadowMap(
                NiShadowGenerator::AUTO_DETERMINE_SM_INDEX, callContext.m_pkMesh);
            EE_ASSERT(pShadowMap);

            XMMATRIX worldToShadowMap(pShadowMap->GetWorldToShadowMap());
            // If it's column major, then we must manually transpose.
            // The data is already stored transposed, so we only transpose
            // if it's _not_ column major.
            if (pEntry->GetColumnMajor())
            {
                *pResult = worldToShadowMap;
            }
            else
            {
                *pResult = XMMatrixTranspose(worldToShadowMap);
            }

            return true;
        }
        else
        {
            *pResult = XMMatrixIdentity();
            return false;
        }
    case NiShaderConstantMap::SCM_OBJ_SHADOWMAPTEXSIZE:
        dataSize = 1 * 4 * sizeof(efd::Float32);
        dataStride = 1 * 4 * sizeof(efd::Float32);
        if (pDynEffect)
        {
            EE_ASSERT(NiIsKindOf(NiLight, pDynEffect));

            NiShadowGenerator* pGenerator = pDynEffect->GetShadowGenerator();
            EE_ASSERT(pGenerator);

            NiShadowMap* pShadowMap = pGenerator->RetrieveShadowMap(
                NiShadowGenerator::AUTO_DETERMINE_SM_INDEX, 
                callContext.m_pkMesh);
            EE_ASSERT(pShadowMap);

            ((efd::Float32*)pResult->r)[0] = (efd::Float32)pShadowMap->GetTexture()->GetWidth();
            ((efd::Float32*)pResult->r)[1] = (efd::Float32)pShadowMap->GetTexture()->GetHeight();
            ((efd::Float32*)pResult->r)[2] = 0.0f;
            ((efd::Float32*)pResult->r)[3] = 0.0f;

            return true;
        }
        else
        {
            ((efd::Float32*)pResult->r)[0] = 0.0f;
            ((efd::Float32*)pResult->r)[1] = 0.0f;
            ((efd::Float32*)pResult->r)[2] = 0.0f;
            ((efd::Float32*)pResult->r)[3] = 0.0f;
            return false;
        }

    case NiShaderConstantMap::SCM_OBJ_SHADOWBIAS:
    case NiShaderConstantMap::SCM_OBJ_SHADOW_VSM_POWER_EPSILON:
        dataSize = 1 * 4 * sizeof(efd::Float32);
        dataStride = 1 * 4 * sizeof(efd::Float32);
        if (pDynEffect)
        {
            NiShadowGenerator* pGenerator = pDynEffect->GetShadowGenerator();
            EE_ASSERT(pGenerator);

            // Only single register shader constants supported by callback.
            return pGenerator->GetShaderConstantData(
                pResult,
                sizeof(efd::Float32) * 4, 
                callContext.m_pkMesh,
                NiShadowGenerator::AUTO_DETERMINE_SM_INDEX, 
                objectMapping,
                callContext.m_pkState, 
                callContext.m_pkEffects, 
                *callContext.m_pkWorld,
                *callContext.m_pkWorldBound, 
                callContext.m_uiPass);
        }
        else
        {
            ((efd::Float32*)pResult->r)[0] = 0.0f;
            ((efd::Float32*)pResult->r)[1] = 0.0f;
            ((efd::Float32*)pResult->r)[2] = 0.0f;
            ((efd::Float32*)pResult->r)[3] = 0.0f;
            return false;
        }
    case NiShaderConstantMap::SCM_OBJ_SHADOW_PSSM_SPLITDISTANCES:
        {
            efd::UInt32 count = 
                (pEntry->GetInternal() & NiShaderConstantMapEntry::SCME_OBJECT_COUNT_MASK) >>
                NiShaderConstantMapEntry::SCME_OBJECT_COUNT_SHIFT;

            dataSize = 1 * 4 * sizeof(efd::Float32) * count;
            dataStride = 1 * 4 * sizeof(efd::Float32);
            if (pDynEffect)
            {
                NiShadowGenerator* pGenerator = pDynEffect->GetShadowGenerator();
                EE_ASSERT(pGenerator);

                // Only single register shader constants supported by callback.
                return pGenerator->GetShaderConstantData(
                    pResult,
                    dataSize, 
                    callContext.m_pkMesh,
                    NiShadowGenerator::AUTO_DETERMINE_SM_INDEX, 
                    objectMapping,
                    callContext.m_pkState, 
                    callContext.m_pkEffects, 
                    *callContext.m_pkWorld,
                    *callContext.m_pkWorldBound, 
                    callContext.m_uiPass);
            }
            else
            {
                for (efd::UInt32 i = 0; i < count; ++i)
                {
                    ((efd::Float32*)pResult->r)[i*4+0] = 0.0f;
                    ((efd::Float32*)pResult->r)[i*4+1] = 0.0f;
                    ((efd::Float32*)pResult->r)[i*4+2] = 0.0f;
                    ((efd::Float32*)pResult->r)[i*4+3] = 0.0f;
                }
                return false;
            }
        }
    case NiShaderConstantMap::SCM_OBJ_SHADOW_PSSM_SPLITMATRICES:
        {
            efd::UInt32 numMatrices = 
                (pEntry->GetInternal() & NiShaderConstantMapEntry::SCME_OBJECT_COUNT_MASK) >>
                NiShaderConstantMapEntry::SCME_OBJECT_COUNT_SHIFT;
            dataSize = 4 * 4 * sizeof(efd::Float32) * numMatrices;
            dataStride = 4 * 4 * sizeof(efd::Float32);
            if (pDynEffect)
            {
                NiShadowGenerator* pGenerator = pDynEffect->GetShadowGenerator();
                EE_ASSERT(pGenerator);

                if (!pGenerator->GetShaderConstantData(
                    pResult,
                    dataSize, 
                    callContext.m_pkMesh,
                    NiShadowGenerator::AUTO_DETERMINE_SM_INDEX, 
                    objectMapping,
                    callContext.m_pkState, 
                    callContext.m_pkEffects, 
                    *callContext.m_pkWorld,
                    *callContext.m_pkWorldBound, 
                    callContext.m_uiPass))
                {
                    break;
                }

                if (!pEntry->GetColumnMajor())
                {
                    // Transpose all the matrices retrieved
                    for (efd::UInt32 i = 0; i < numMatrices; ++i)
                        pResult[i] = XMMatrixTranspose(pResult[i]);
                }
                return true;
            }
            else
            {
                for (efd::UInt32 i = 0; i < numMatrices; ++i)
                {
                    pResult[i] = XMMatrixIdentity();
                }
                return false;
            }
        }
    case NiShaderConstantMap::SCM_OBJ_SHADOW_PSSM_ATLASVIEWPORTS:
        {
            efd::UInt32 count = 
                (pEntry->GetInternal() & NiShaderConstantMapEntry::SCME_OBJECT_COUNT_MASK) >>
                NiShaderConstantMapEntry::SCME_OBJECT_COUNT_SHIFT;
            dataSize = 1 * 4 * sizeof(efd::Float32) * count;
            dataStride = 1 * 4 * sizeof(efd::Float32);

            if (pDynEffect)
            {
                NiShadowGenerator* pGenerator = pDynEffect->GetShadowGenerator();
                EE_ASSERT(pGenerator);

                return pGenerator->GetShaderConstantData(
                    pResult,
                    dataSize, 
                    callContext.m_pkMesh,
                    NiShadowGenerator::AUTO_DETERMINE_SM_INDEX, 
                    objectMapping,
                    callContext.m_pkState, 
                    callContext.m_pkEffects, 
                    *callContext.m_pkWorld,
                    *callContext.m_pkWorldBound, 
                    callContext.m_uiPass);
            }
            else
            {
                for (efd::UInt32 i = 0; i < count; ++i)
                {
                    ((efd::Float32*)pResult->r)[i*4+0] = 1.0f;
                    ((efd::Float32*)pResult->r)[i*4+1] = 1.0f;
                    ((efd::Float32*)pResult->r)[i*4+2] = 0.0f;
                    ((efd::Float32*)pResult->r)[i*4+3] = 0.0f;
                }
                return false;
            }
        }
    case NiShaderConstantMap::SCM_OBJ_SHADOW_PSSM_TRANSITIONMATRIX:
        dataSize = 4 * 4 * sizeof(efd::Float32);
        dataStride = 4 * 4 * sizeof(efd::Float32);
        if (pDynEffect)
        {
            NiShadowGenerator* pGenerator = pDynEffect->GetShadowGenerator();
            EE_ASSERT(pGenerator);

            if (!pGenerator->GetShaderConstantData(
                pResult,
                sizeof(*pResult), 
                callContext.m_pkMesh,
                NiShadowGenerator::AUTO_DETERMINE_SM_INDEX, 
                objectMapping,
                callContext.m_pkState, 
                callContext.m_pkEffects, 
                *callContext.m_pkWorld,
                *callContext.m_pkWorldBound, 
                callContext.m_uiPass))
            {
                break;
            }

            if (!pEntry->GetColumnMajor())
            {
                // Transpose all the matrices retrieved
                *pResult = XMMatrixTranspose(*pResult);
            }

            return true;
        }
        else
        {
            *pResult = XMMatrixIdentity();
            return false;
        }
    case NiShaderConstantMap::SCM_OBJ_SHADOW_PSSM_TRANSITIONSIZE:
        dataSize = 1 * 4 * sizeof(efd::Float32);
        dataStride = 1 * 4 * sizeof(efd::Float32);
        if (pDynEffect)
        {
            NiShadowGenerator* pGenerator = pDynEffect->GetShadowGenerator();
            EE_ASSERT(pGenerator);

            return pGenerator->GetShaderConstantData(
                pResult,
                sizeof(efd::Float32) * 4, 
                callContext.m_pkMesh,
                NiShadowGenerator::AUTO_DETERMINE_SM_INDEX, 
                objectMapping,
                callContext.m_pkState, 
                callContext.m_pkEffects, 
                *callContext.m_pkWorld,
                *callContext.m_pkWorldBound, 
                callContext.m_uiPass);
        }
        else
        {
            ((efd::Float32*)pResult->r)[0] = 0.0f;
            ((efd::Float32*)pResult->r)[1] = 0.0f;
            ((efd::Float32*)pResult->r)[2] = 0.0f;
            ((efd::Float32*)pResult->r)[3] = 0.0f;
            return false;
        }
    default:
        break;
    }

    D3D11Error::ReportWarning(
        "Invalid object mapping 0x%08X found in "
        __FUNCTION__
        " for shader constant map entry %s.",
        objectMapping,
        pEntry->GetKey());
    return false;
}

//------------------------------------------------------------------------------------------------
const void* D3D11ShaderConstantMap::PerformOperatorMultiply(
    const void* pOperand1, 
    NiShaderAttributeDesc::AttributeType attribType1,
    const void* pOperand2, 
    NiShaderAttributeDesc::AttributeType attribType2,
    efd::Bool invert, 
    efd::Bool transpose, 
    efd::UInt32& dataSize,
    efd::UInt32& dataStride,
    XMMATRIX& tempMatrix)
{
    if (attribType1 == NiShaderAttributeDesc::ATTRIB_TYPE_MATRIX4)
    {
        efd::Bool resultIsMatrix = true;
        XMMATRIX* pMatrix1 = (XMMATRIX*)pOperand1;
        if (attribType2 == NiShaderAttributeDesc::ATTRIB_TYPE_MATRIX4)
        {
            XMMATRIX* pMatrix2 = (XMMATRIX*)pOperand2;

            dataSize = 4 * 4 * sizeof(efd::Float32);
            dataStride = 4 * 4 * sizeof(efd::Float32);

            tempMatrix = (*pMatrix1) * (*pMatrix2);
        }
        else if (attribType2 == NiShaderAttributeDesc::ATTRIB_TYPE_UNSIGNEDINT)
        {
            efd::UInt32 operand2AsUInt32 = *((efd::UInt32*)pOperand2);
            efd::Float32 operand2AsFloat = (efd::Float32)operand2AsUInt32;

            dataSize = 4 * 4 * sizeof(efd::Float32);
            dataStride = 4 * 4 * sizeof(efd::Float32);

            for (efd::UInt32 i = 0; i < 4; i++)
                tempMatrix.r[i] = pMatrix1->r[i] * operand2AsFloat;
        }
        else if (attribType2 == NiShaderAttributeDesc::ATTRIB_TYPE_FLOAT)
        {
            efd::Float32 operand2AsFloat = *((efd::Float32*)pOperand2);

            dataSize = 4 * 4 * sizeof(efd::Float32);
            dataStride = 4 * 4 * sizeof(efd::Float32);

            for (efd::UInt32 i = 0; i < 4; i++)
                tempMatrix.r[i] = pMatrix1->r[i] * operand2AsFloat;
        }
        else if (attribType2 == NiShaderAttributeDesc::ATTRIB_TYPE_POINT4 ||
            attribType2 == NiShaderAttributeDesc::ATTRIB_TYPE_COLOR)
        {
            resultIsMatrix = false;

            dataSize = 1 * 4 * sizeof(efd::Float32);
            dataStride = 1 * 4 * sizeof(efd::Float32);

            XMVECTOR vector2 = XMLoadFloat4((XMFLOAT4*)pOperand2);
            tempMatrix.r[0] = XMVector4Transform(vector2, *pMatrix1);
        }
        else
        {
            D3D11Error::ReportWarning(
                "Attribute type 0x%08X is invalid for attribute 2 of the multiply operator when "
                "attribute 1 is of type 0x%08X in "
                __FUNCTION__
                ".",
                attribType2,
                attribType1);
            return NULL;
        }

        if (resultIsMatrix)
        {
            XMVECTOR determinant;
            if (invert)
                tempMatrix = XMMatrixInverse(&determinant, tempMatrix);
            if (transpose)
                tempMatrix = XMMatrixTranspose(tempMatrix);
        }
    }
    else if (attribType1 == NiShaderAttributeDesc::ATTRIB_TYPE_POINT4 ||
        attribType1 == NiShaderAttributeDesc::ATTRIB_TYPE_COLOR)
    {
        XMVECTOR vector1 = XMLoadFloat4((XMFLOAT4*)pOperand1);
        XMVECTOR* pResult = &(tempMatrix.r[0]);

        dataSize = 1 * 4 * sizeof(efd::Float32);
        dataStride = 1 * 4 * sizeof(efd::Float32);

        if (attribType2 == NiShaderAttributeDesc::ATTRIB_TYPE_MATRIX4)
        {
            XMMATRIX* pMatrix2 = (XMMATRIX*)pOperand2;

            *pResult = XMVector4Transform(vector1, *pMatrix2);
        }
        else if (attribType2 == NiShaderAttributeDesc::ATTRIB_TYPE_UNSIGNEDINT)
        {
            efd::UInt32 operand2AsUInt32 = *((efd::UInt32*)pOperand2);
            efd::Float32 operand2AsFloat = (efd::Float32)operand2AsUInt32;

            *pResult = vector1 * operand2AsFloat;
        }
        else if (attribType2 == NiShaderAttributeDesc::ATTRIB_TYPE_FLOAT)
        {
            efd::Float32 operand2AsFloat = *((efd::Float32*)pOperand2);

            *pResult = vector1 * operand2AsFloat;
        }
        else if (attribType2 == NiShaderAttributeDesc::ATTRIB_TYPE_POINT4 ||
            attribType2 == NiShaderAttributeDesc::ATTRIB_TYPE_COLOR)
        {
            XMVECTOR vector2 = XMLoadFloat4((XMFLOAT4*)pOperand2);

            *pResult = vector1 * vector2;
        }
        else
        {
            D3D11Error::ReportWarning(
                "Attribute type 0x%08X is invalid for attribute 2 of the multiply operator when "
                "attribute 1 is of type 0x%08X in "
                __FUNCTION__
                ".",
                attribType2,
                attribType1);
            return NULL;
        }
    }
    else
    {
        D3D11Error::ReportWarning(
            "Attribute type 0x%08X is invalid for attribute 1 of the multiply operator in "
            __FUNCTION__
            ".",
            attribType1);
        return NULL;
    }

    return &tempMatrix;
}

//------------------------------------------------------------------------------------------------
const void* D3D11ShaderConstantMap::PerformOperatorDivide(
    const void* pOperand1, 
    NiShaderAttributeDesc::AttributeType attribType1,
    const void* pOperand2, 
    NiShaderAttributeDesc::AttributeType attribType2,
    efd::Bool invert, 
    efd::Bool transpose, 
    efd::UInt32& dataSize,
    efd::UInt32& dataStride,
    XMMATRIX& tempMatrix)
{
    if (attribType1 == NiShaderAttributeDesc::ATTRIB_TYPE_MATRIX4)
    {
        XMMATRIX* pMatrix1 = (XMMATRIX*)pOperand1;

        dataSize = 4 * 4 * sizeof(efd::Float32);
        dataStride = 4 * 4 * sizeof(efd::Float32);

        if (attribType2 == NiShaderAttributeDesc::ATTRIB_TYPE_UNSIGNEDINT)
        {
            efd::UInt32 operand2AsUInt32 = *((efd::UInt32*)pOperand2);
            efd::Float32 invOperand2AsFloat = 1.0f / (efd::Float32)operand2AsUInt32;

            for (efd::UInt32 i = 0; i < 4; i++)
                tempMatrix.r[i] = pMatrix1->r[i] * invOperand2AsFloat;
        }
        else if (attribType2 == NiShaderAttributeDesc::ATTRIB_TYPE_FLOAT)
        {
            efd::Float32 invOperand2AsFloat = 1.0f / *((efd::Float32*)pOperand2);

            for (efd::UInt32 i = 0; i < 4; i++)
                tempMatrix.r[i] = pMatrix1->r[i] * invOperand2AsFloat;
        }
        else
        {
            D3D11Error::ReportWarning(
                "Attribute type 0x%08X is invalid for attribute 2 of the divide operator when "
                "attribute 1 is of type 0x%08X in "
                __FUNCTION__
                ".",
                attribType2,
                attribType1);
            return NULL;
        }

        XMVECTOR determinant;
        if (invert)
            tempMatrix = XMMatrixInverse(&determinant, tempMatrix);
        if (transpose)
            tempMatrix = XMMatrixTranspose(tempMatrix);
    }
    else if (attribType1 == NiShaderAttributeDesc::ATTRIB_TYPE_POINT4 ||
        attribType1 == NiShaderAttributeDesc::ATTRIB_TYPE_COLOR)
    {
        XMVECTOR vector1 = XMLoadFloat4((XMFLOAT4*)pOperand1);
        XMVECTOR* pResult = &(tempMatrix.r[0]);

        dataSize = 1 * 4 * sizeof(efd::Float32);
        dataStride = 1 * 4 * sizeof(efd::Float32);

        if (attribType2 == NiShaderAttributeDesc::ATTRIB_TYPE_UNSIGNEDINT)
        {
            efd::UInt32 operand2AsUInt32 = *((efd::UInt32*)pOperand2);
            efd::Float32 operand2AsFloat = (efd::Float32)operand2AsUInt32;

            *pResult = vector1 / operand2AsFloat;
        }
        else if (attribType2 == NiShaderAttributeDesc::ATTRIB_TYPE_FLOAT)
        {
            efd::Float32 operand2AsFloat = *((efd::Float32*)pOperand2);

            *pResult = vector1 / operand2AsFloat;
        }
        else if (attribType2 == NiShaderAttributeDesc::ATTRIB_TYPE_POINT4 ||
            attribType2 == NiShaderAttributeDesc::ATTRIB_TYPE_COLOR)
        {
            XMVECTOR vector2 = XMLoadFloat4((XMFLOAT4*)pOperand2);

            *pResult = vector1 / vector2;
        }
        else
        {
            D3D11Error::ReportWarning(
                "Attribute type 0x%08X is invalid for attribute 2 of the divide operator when "
                "attribute 1 is of type 0x%08X in "
                __FUNCTION__
                ".",
                attribType2,
                attribType1);
            return NULL;
        }
    }
    else
    {
        D3D11Error::ReportWarning(
            "Attribute type 0x%08X is invalid for attribute 1 of the divide operator in "
            __FUNCTION__
            ".",
            attribType1);
        return NULL;
    }

    return &tempMatrix;
}

//------------------------------------------------------------------------------------------------
const void* D3D11ShaderConstantMap::PerformOperatorAdd(
    const void* pOperand1, 
    NiShaderAttributeDesc::AttributeType attribType1,
    const void* pOperand2, 
    NiShaderAttributeDesc::AttributeType attribType2,
    efd::Bool, 
    efd::Bool, 
    efd::UInt32& dataSize,
    efd::UInt32& dataStride,
    XMMATRIX& tempMatrix)
{
    if (attribType1 == NiShaderAttributeDesc::ATTRIB_TYPE_UNSIGNEDINT)
    {
        efd::UInt32 operand1AsUInt32 = *(efd::UInt32*)pOperand1;

        if (attribType2 == NiShaderAttributeDesc::ATTRIB_TYPE_UNSIGNEDINT)
        {
            efd::UInt32 operand2AsUInt32 = *(efd::UInt32*)pOperand2;

            dataSize = 1 * 1 * sizeof(efd::Float32);
            dataStride = 1 * 1 * sizeof(efd::Float32);

            *((efd::UInt32*)tempMatrix.r) = operand1AsUInt32 + operand2AsUInt32;
        }
        else if (attribType2 == NiShaderAttributeDesc::ATTRIB_TYPE_FLOAT)
        {
            efd::Float32 operand1AsFloat = (efd::Float32)operand1AsUInt32;
            efd::Float32 operand2AsFloat = *(efd::Float32*)pOperand2;

            dataSize = 1 * 1 * sizeof(efd::Float32);
            dataStride = 1 * 1 * sizeof(efd::Float32);

            ((efd::Float32*)tempMatrix.r)[0] = operand1AsFloat + operand2AsFloat;
        }
        else if (attribType2 == NiShaderAttributeDesc::ATTRIB_TYPE_POINT4 ||
            attribType2 == NiShaderAttributeDesc::ATTRIB_TYPE_COLOR)
        {
            efd::Float32 operand1AsFloat = (efd::Float32)operand1AsUInt32;
            XMFLOAT4* pVector2 = (XMFLOAT4*)pOperand2;

            dataSize = 1 * 4 * sizeof(efd::Float32);
            dataStride = 1 * 4 * sizeof(efd::Float32);

            ((efd::Float32*)tempMatrix.r)[0] = operand1AsFloat + pVector2->x;
            ((efd::Float32*)tempMatrix.r)[1] = operand1AsFloat + pVector2->y;
            ((efd::Float32*)tempMatrix.r)[2] = operand1AsFloat + pVector2->z;
            ((efd::Float32*)tempMatrix.r)[3] = operand1AsFloat + pVector2->w;
        }
        else
        {
            D3D11Error::ReportWarning(
                "Attribute type 0x%08X is invalid for attribute 2 of the add operator when "
                "attribute 1 is of type 0x%08X in "
                __FUNCTION__
                ".",
                attribType2,
                attribType1);
            return NULL;
        }
    }
    else if (attribType1 == NiShaderAttributeDesc::ATTRIB_TYPE_FLOAT)
    {
        efd::Float32 operand1AsFloat = *(efd::Float32*)pOperand1;
        if (attribType2 == NiShaderAttributeDesc::ATTRIB_TYPE_UNSIGNEDINT)
        {
            efd::UInt32 operand2AsUInt32 = *((efd::UInt32*)pOperand2);
            efd::Float32 operand2AsFloat = (efd::Float32)operand2AsUInt32;

            dataSize = 1 * 1 * sizeof(efd::Float32);
            dataStride = 1 * 1 * sizeof(efd::Float32);

            ((efd::Float32*)tempMatrix.r)[0] = operand1AsFloat + operand2AsFloat;
        }
        else if (attribType2 == NiShaderAttributeDesc::ATTRIB_TYPE_FLOAT)
        {
            efd::Float32 operand2AsFloat = *((efd::Float32*)pOperand2);

            dataSize = 1 * 1 * sizeof(efd::Float32);
            dataStride = 1 * 1 * sizeof(efd::Float32);

            ((efd::Float32*)tempMatrix.r)[0] = operand1AsFloat + operand2AsFloat;
        }
        else if (attribType2 == NiShaderAttributeDesc::ATTRIB_TYPE_POINT4 ||
            attribType2 == NiShaderAttributeDesc::ATTRIB_TYPE_COLOR)
        {
            XMFLOAT4* pVector2 = (XMFLOAT4*)pOperand2;

            dataSize = 1 * 4 * sizeof(efd::Float32);
            dataStride = 1 * 4 * sizeof(efd::Float32);

            ((efd::Float32*)tempMatrix.r)[0] = operand1AsFloat + pVector2->x;
            ((efd::Float32*)tempMatrix.r)[1] = operand1AsFloat + pVector2->y;
            ((efd::Float32*)tempMatrix.r)[2] = operand1AsFloat + pVector2->z;
            ((efd::Float32*)tempMatrix.r)[3] = operand1AsFloat + pVector2->w;
        }
        else
        {
            D3D11Error::ReportWarning(
                "Attribute type 0x%08X is invalid for attribute 2 of the add operator when "
                "attribute 1 is of type 0x%08X in "
                __FUNCTION__
                ".",
                attribType2,
                attribType1);
            return NULL;
        }
    }
    else if (attribType1 == NiShaderAttributeDesc::ATTRIB_TYPE_POINT4 ||
        attribType1 == NiShaderAttributeDesc::ATTRIB_TYPE_COLOR)
    {
        XMFLOAT4* pVector1 = (XMFLOAT4*)pOperand1;

        dataSize = 1 * 4 * sizeof(efd::Float32);
        dataStride = 1 * 4 * sizeof(efd::Float32);

        if (attribType2 == NiShaderAttributeDesc::ATTRIB_TYPE_UNSIGNEDINT)
        {
            efd::UInt32 operand2AsUInt32 = *((efd::UInt32*)pOperand2);
            efd::Float32 operand2AsFloat = (efd::Float32)operand2AsUInt32;

            ((efd::Float32*)tempMatrix.r)[0] = pVector1->x + operand2AsFloat;
            ((efd::Float32*)tempMatrix.r)[1] = pVector1->y + operand2AsFloat;
            ((efd::Float32*)tempMatrix.r)[2] = pVector1->z + operand2AsFloat;
            ((efd::Float32*)tempMatrix.r)[3] = pVector1->w + operand2AsFloat;
        }
        else if (attribType2 == NiShaderAttributeDesc::ATTRIB_TYPE_FLOAT)
        {
            efd::Float32 operand2AsFloat = *((efd::Float32*)pOperand2);

            ((efd::Float32*)tempMatrix.r)[0] = pVector1->x + operand2AsFloat;
            ((efd::Float32*)tempMatrix.r)[1] = pVector1->y + operand2AsFloat;
            ((efd::Float32*)tempMatrix.r)[2] = pVector1->z + operand2AsFloat;
            ((efd::Float32*)tempMatrix.r)[3] = pVector1->w + operand2AsFloat;
        }
        else if (attribType2 == NiShaderAttributeDesc::ATTRIB_TYPE_POINT4 ||
            attribType2 == NiShaderAttributeDesc::ATTRIB_TYPE_COLOR)
        {
            XMVECTOR vector1 = XMLoadFloat4(pVector1);
            XMVECTOR vector2 = XMLoadFloat4((XMFLOAT4*)pOperand2);

            tempMatrix.r[0] = vector1 + vector2;
        }
        else
        {
            D3D11Error::ReportWarning(
                "Attribute type 0x%08X is invalid for attribute 2 of the add operator when "
                "attribute 1 is of type 0x%08X in "
                __FUNCTION__
                ".",
                attribType2,
                attribType1);
            return NULL;
        }
    }
    else
    {
        D3D11Error::ReportWarning(
            "Attribute type 0x%08X is invalid for attribute 1 of the add operator in "
            __FUNCTION__
            ".",
            attribType1);
        return NULL;
    }

    return &tempMatrix;
}

//------------------------------------------------------------------------------------------------
const void* D3D11ShaderConstantMap::PerformOperatorSubtract(
    const void* pOperand1, 
    NiShaderAttributeDesc::AttributeType attribType1,
    const void* pOperand2, 
    NiShaderAttributeDesc::AttributeType attribType2,
    efd::Bool, 
    efd::Bool, 
    efd::UInt32& dataSize,
    efd::UInt32& dataStride,
    XMMATRIX& tempMatrix)
{
    if (attribType1 == NiShaderAttributeDesc::ATTRIB_TYPE_UNSIGNEDINT)
    {
        efd::UInt32 operand1AsUInt32 = *(efd::UInt32*)pOperand1;

        if (attribType2 == NiShaderAttributeDesc::ATTRIB_TYPE_UNSIGNEDINT)
        {
            efd::UInt32 operand2AsUInt32 = *(efd::UInt32*)pOperand2;

            dataSize = 1 * 1 * sizeof(efd::Float32);
            dataStride = 1 * 1 * sizeof(efd::Float32);

            *((efd::UInt32*)tempMatrix.r) = operand1AsUInt32 - operand2AsUInt32;
        }
        else if (attribType2 == NiShaderAttributeDesc::ATTRIB_TYPE_FLOAT)
        {
            efd::Float32 operand1AsFloat = (efd::Float32)operand1AsUInt32;
            efd::Float32 operand2AsFloat = *(efd::Float32*)pOperand2;

            dataSize = 1 * 1 * sizeof(efd::Float32);
            dataStride = 1 * 1 * sizeof(efd::Float32);

            ((efd::Float32*)tempMatrix.r)[0] = operand1AsFloat - operand2AsFloat;
        }
        else if (attribType2 == NiShaderAttributeDesc::ATTRIB_TYPE_POINT4 ||
            attribType2 == NiShaderAttributeDesc::ATTRIB_TYPE_COLOR)
        {
            efd::Float32 operand1AsFloat = (efd::Float32)operand1AsUInt32;

            dataSize = 1 * 4 * sizeof(efd::Float32);
            dataStride = 1 * 4 * sizeof(efd::Float32);

            XMFLOAT4* pVector2 = (XMFLOAT4*)pOperand2;
            ((efd::Float32*)tempMatrix.r)[0] = operand1AsFloat - pVector2->x;
            ((efd::Float32*)tempMatrix.r)[1] = operand1AsFloat - pVector2->y;
            ((efd::Float32*)tempMatrix.r)[2] = operand1AsFloat - pVector2->z;
            ((efd::Float32*)tempMatrix.r)[3] = operand1AsFloat - pVector2->w;
        }
        else
        {
            D3D11Error::ReportWarning(
                "Attribute type 0x%08X is invalid for attribute 2 of the subtract operator when "
                "attribute 1 is of type 0x%08X in "
                __FUNCTION__
                ".",
                attribType2,
                attribType1);
            return NULL;
        }
    }
    else if (attribType1 == NiShaderAttributeDesc::ATTRIB_TYPE_FLOAT)
    {
        efd::Float32 operand1AsFloat = *(efd::Float32*)pOperand1;
        if (attribType2 == NiShaderAttributeDesc::ATTRIB_TYPE_UNSIGNEDINT)
        {
            efd::UInt32 operand2AsUInt32 = *(efd::UInt32*)pOperand2;
            efd::Float32 operand2AsFloat = (efd::Float32)operand2AsUInt32;

            dataSize = 1 * 1 * sizeof(efd::Float32);
            dataStride = 1 * 1 * sizeof(efd::Float32);

            ((efd::Float32*)tempMatrix.r)[0] = operand1AsFloat - operand2AsFloat;
        }
        else if (attribType2 == NiShaderAttributeDesc::ATTRIB_TYPE_FLOAT)
        {
            efd::Float32 operand2AsFloat = *(efd::Float32*)pOperand2;

            dataSize = 1 * 1 * sizeof(efd::Float32);
            dataStride = 1 * 1 * sizeof(efd::Float32);

            ((efd::Float32*)tempMatrix.r)[0] = operand1AsFloat - operand2AsFloat;
        }
        else if (attribType2 == NiShaderAttributeDesc::ATTRIB_TYPE_POINT4 ||
            attribType2 == NiShaderAttributeDesc::ATTRIB_TYPE_COLOR)
        {
            XMFLOAT4* pVector2 = (XMFLOAT4*)pOperand2;

            dataSize = 1 * 4 * sizeof(efd::Float32);
            dataStride = 1 * 4 * sizeof(efd::Float32);

            ((efd::Float32*)tempMatrix.r)[0] = operand1AsFloat - pVector2->x;
            ((efd::Float32*)tempMatrix.r)[1] = operand1AsFloat - pVector2->y;
            ((efd::Float32*)tempMatrix.r)[2] = operand1AsFloat - pVector2->z;
            ((efd::Float32*)tempMatrix.r)[3] = operand1AsFloat - pVector2->w;
        }
        else
        {
            D3D11Error::ReportWarning(
                "Attribute type 0x%08X is invalid for attribute 2 of the subtract operator when "
                "attribute 1 is of type 0x%08X in "
                __FUNCTION__
                ".",
                attribType2,
                attribType1);
            return NULL;
        }
    }
    else if (attribType1 == NiShaderAttributeDesc::ATTRIB_TYPE_POINT4 ||
        attribType1 == NiShaderAttributeDesc::ATTRIB_TYPE_COLOR)
    {
        XMFLOAT4* pVector1 = (XMFLOAT4*)pOperand1;

        dataSize = 1 * 4 * sizeof(efd::Float32);
        dataStride = 1 * 4 * sizeof(efd::Float32);

        if (attribType2 == NiShaderAttributeDesc::ATTRIB_TYPE_UNSIGNEDINT)
        {
            efd::UInt32 operand2AsUInt32 = *(efd::UInt32*)pOperand2;
            efd::Float32 operand2AsFloat = (efd::Float32)operand2AsUInt32;

            ((efd::Float32*)tempMatrix.r)[0] = pVector1->x - operand2AsFloat;
            ((efd::Float32*)tempMatrix.r)[1] = pVector1->y - operand2AsFloat;
            ((efd::Float32*)tempMatrix.r)[2] = pVector1->z - operand2AsFloat;
            ((efd::Float32*)tempMatrix.r)[3] = pVector1->w - operand2AsFloat;
        }
        else if (attribType2 == NiShaderAttributeDesc::ATTRIB_TYPE_FLOAT)
        {
            efd::Float32 operand2AsFloat = *(efd::Float32*)pOperand2;

            ((efd::Float32*)tempMatrix.r)[0] = pVector1->x - operand2AsFloat;
            ((efd::Float32*)tempMatrix.r)[1] = pVector1->y - operand2AsFloat;
            ((efd::Float32*)tempMatrix.r)[2] = pVector1->z - operand2AsFloat;
            ((efd::Float32*)tempMatrix.r)[3] = pVector1->w - operand2AsFloat;
        }
        else if (attribType2 == NiShaderAttributeDesc::ATTRIB_TYPE_POINT4 ||
            attribType2 == NiShaderAttributeDesc::ATTRIB_TYPE_COLOR)
        {
            XMVECTOR vector1 = XMLoadFloat4(pVector1);
            XMVECTOR vector2 = XMLoadFloat4((XMFLOAT4*)pOperand2);

            tempMatrix.r[0] = vector1 - vector2;
        }
        else
        {
            D3D11Error::ReportWarning(
                "Attribute type 0x%08X is invalid for attribute 2 of the subtract operator when "
                "attribute 1 is of type 0x%08X in "
                __FUNCTION__
                ".",
                attribType2,
                attribType1);
            return NULL;
        }
    }
    else
    {
        D3D11Error::ReportWarning(
            "Attribute type 0x%08X is invalid for attribute 1 of the subtract operator in "
            __FUNCTION__
            ".",
            attribType1);
        return NULL;
    }

    return &tempMatrix;
}

//------------------------------------------------------------------------------------------------
void D3D11ShaderConstantMap::CalculatePackingEntry(
    efd::UInt32& currentRegister,
    efd::UInt32& currentElement, 
    efd::UInt32 registerCount,
    efd::UInt32 elementCount, 
    efd::UInt32& encodedStartRegister,
    efd::UInt32& encodedRegisterCount, 
    efd::Bool packRegisters)
{
    efd::UInt32 startRegister = currentRegister;
    efd::UInt32 startElement = currentElement;
    if ((registerCount > 0 && currentElement > 0) ||
        (currentElement + elementCount > D3D11_COMMONSHADER_CONSTANT_BUFFER_REGISTER_COMPONENTS))
    {
        startRegister++;
        startElement = 0;
    }

    efd::UInt32 finalRegister = startRegister + registerCount;
    efd::UInt32 finalElement = startElement + elementCount;
    while (finalElement > D3D11_COMMONSHADER_CONSTANT_BUFFER_REGISTER_COMPONENTS)
    {
        finalElement -= D3D11_COMMONSHADER_CONSTANT_BUFFER_REGISTER_COMPONENTS;
        finalRegister++;
    }

    EE_ASSERT(finalElement >= startElement);
    EE_ASSERT(finalRegister >= startRegister);
    EncodePackedRegisterAndElement(encodedStartRegister, startRegister, startElement, false);

    EncodePackedRegisterAndElement(
        encodedRegisterCount, 
        finalRegister - startRegister, 
        finalElement - startElement,
        packRegisters);

    if (finalElement == D3D11_COMMONSHADER_CONSTANT_BUFFER_REGISTER_COMPONENTS)
    {
        finalElement = 0;
        finalRegister++;
    }

    currentRegister = finalRegister;
    currentElement = finalElement;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11ShaderConstantMap::IsRegisterEncoded(efd::UInt32 registerID)
{
    return ((registerID & SCM_REGISTER_ENCODING_MASK) == SCM_REGISTER_ENCODING);
}

//------------------------------------------------------------------------------------------------
void D3D11ShaderConstantMap::InsertPadding(efd::UInt32 paddingInBytes,
    NiDataStreamElementSet& dataStreamElements)
{
    const efd::UInt32 componentSize = D3D11_COMMONSHADER_CONSTANT_BUFFER_COMPONENT_BIT_COUNT / 8;

    // Padding had better be a multiple of 4 bytes!
    EE_ASSERT((paddingInBytes % componentSize) == 0);
    efd::UInt32 paddingComponentCount = paddingInBytes / componentSize;

    efd::UInt32 paddingRows = paddingComponentCount /
        D3D11_COMMONSHADER_CONSTANT_BUFFER_REGISTER_COMPONENTS;
    efd::UInt32 paddingRemainder = paddingComponentCount %
        D3D11_COMMONSHADER_CONSTANT_BUFFER_REGISTER_COMPONENTS;

    // Padding will be of type UINT32.
    while (paddingRows-- > 0)
        dataStreamElements.AddElement(NiDataStreamElement::F_UINT32_4);
    if (paddingRemainder)
    {
        NiDataStreamElement::Format format =
            NiDataStreamElement::GetPredefinedFormat(
            NiDataStreamElement::T_UINT32, 
            (efd::UInt8)paddingRemainder,
            false);
        EE_ASSERT(format != NiDataStreamElement::F_UNKNOWN);
        dataStreamElements.AddElement(format);
    }
}

//------------------------------------------------------------------------------------------------
