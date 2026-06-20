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

#include "D3D11FragmentShader.h"
#include "D3D11Pass.h"
#include "D3D11Renderer.h"
#include "D3D11RenderStateManager.h"
#include "D3D11ShaderProgramFactory.h"

#include <NiLight.h>
#include <NiMaterialDescriptor.h>
#include <NiShaderDesc.h>
#include <NiShadowGenerator.h>
#include <NiShadowMap.h>
#include <NiNoiseTexture.h>
#include <NiStandardMaterial.h>
#include <NiTextureEffect.h>
#include <NiTextureStage.h>
#include <NiMesh.h>

using namespace ecr;

NiImplementRTTI(D3D11FragmentShader, D3D11ShaderCore, NiTypeMask::D3D11FragmentShader);

//------------------------------------------------------------------------------------------------
D3D11FragmentShader::D3D11FragmentShader(NiMaterialDescriptor* pDesc)
{
    EE_ASSERT(pDesc != NULL);
    m_descriptor.m_spMatDesc = pDesc;
}

//------------------------------------------------------------------------------------------------
D3D11FragmentShader::~D3D11FragmentShader()
{
    /* */
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11FragmentShader::IsGenericallyConfigurable()
{
    return true;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11FragmentShader::AppendRenderPass(efd::UInt32& passId)
{
    D3D11PassPtr spPass;
    D3D11Pass::CreateNewPass(spPass);
    EE_ASSERT(spPass);
    passId = m_passArray.Add(spPass);

    return true;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11FragmentShader::SetAlphaOverride(
    efd::UInt32 passId,
    efd::Bool alphaBlend, efd::Bool usePreviousSrcBlendMode,
    NiAlphaProperty::AlphaFunction srcBlendMode,
    efd::Bool usePreviousDestBlendMode,
    NiAlphaProperty::AlphaFunction destBlendMode)
{
    if (passId >= m_passArray.GetSize())
        return false;

    D3D11Pass* pPass = m_passArray.GetAt(passId);
    if (!pPass)
        return false;

    D3D11Pass* pPreviousPass = NULL;
    if (passId > 0)
        pPreviousPass = m_passArray.GetAt(passId - 1);

    D3D11RenderStateGroup* pRSGroup = pPass->GetRenderStateGroup();
    if (pRSGroup == NULL)
    {
        pRSGroup = EE_NEW D3D11RenderStateGroup;
        pPass->SetRenderStateGroup(pRSGroup);
    }

    D3D11RenderStateGroup* pPreviousPassRSGroup = NULL;
    if (pPreviousPass)
        pPreviousPassRSGroup = pPreviousPass->GetRenderStateGroup();

    if (alphaBlend)
    {
        // FragmentShader supports only first render target
        pRSGroup->SetBSBlendEnable(0, true);

        if (usePreviousSrcBlendMode)
        {
            efd::Bool valid = false;
            D3D11_BLEND src = D3D11_BLEND_ZERO;
            if (pPreviousPassRSGroup)
                valid = pPreviousPassRSGroup->GetBSSrcBlend(0, src);
            if (valid)
                pRSGroup->SetBSSrcBlend(0, src);
            else
                pRSGroup->RemoveBSSrcBlend(0);
        }
        else
        {
            pRSGroup->SetBSSrcBlend(
                0, 
                D3D11RenderStateManager::ConvertGbBlendToD3D11Blend(srcBlendMode));
        }

        if (usePreviousDestBlendMode)
        {
            efd::Bool valid = false;
            D3D11_BLEND dest = D3D11_BLEND_ZERO;
            if (pPreviousPassRSGroup)
                valid = pPreviousPassRSGroup->GetBSDestBlend(0, dest);
            if (valid)
                pRSGroup->SetBSDestBlend(0, dest);
            else
                pRSGroup->RemoveBSDestBlend(0);
        }
        else
        {
            pRSGroup->SetBSSrcBlend(
                0, 
                D3D11RenderStateManager::ConvertGbBlendToD3D11Blend(destBlendMode));
        }

        // This function does assume that the operation is add.
        pRSGroup->SetBSBlendOp(0, D3D11_BLEND_OP_ADD);

        // Alpha modes will be combined in the same manner as the color modes
    }
    else
    {
        for (efd::UInt32 i = 0; i < D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT; i++)
        {
            pRSGroup->SetBSBlendEnable(i, false);
        }
    }

    return true;
}

//--------------------------------------------------------------------------------------------------
bool D3D11FragmentShader::SetAlphaTestOverride(unsigned int passId,
    bool bAlphaTest)
{
    if (passId >= m_passArray.GetSize())
        return false;

    D3D11Pass* pPass = m_passArray.GetAt(passId);
    if (!pPass)
        return false;

    // D3D11 only supports shader alpha testing. So no render states exist for turning it on. 
    return bAlphaTest == false;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11FragmentShader::SetGPUProgram(
    efd::UInt32 passId,
    NiGPUProgram* pProgram, 
    NiGPUProgram::ProgramType& shaderType)
{
    if (passId >= m_passArray.GetSize())
        return false;

    D3D11Pass* pPass = m_passArray.GetAt(passId);
    if (!pPass)
        return false;

    switch (pProgram->GetProgramType())
    {
    case NiGPUProgram::PROGRAM_VERTEX:
        pPass->SetVertexShader((D3D11VertexShader*)pProgram);
        shaderType = NiGPUProgram::PROGRAM_VERTEX;
        break;
    case NiGPUProgram::PROGRAM_GEOMETRY:
        pPass->SetGeometryShader((D3D11GeometryShader*)pProgram);
        shaderType = NiGPUProgram::PROGRAM_GEOMETRY;
        break;
    case NiGPUProgram::PROGRAM_PIXEL:
        pPass->SetPixelShader((D3D11PixelShader*)pProgram);
        shaderType = NiGPUProgram::PROGRAM_PIXEL;
        break;
    }
    return true;
}

//------------------------------------------------------------------------------------------------
NiShaderConstantMap* D3D11FragmentShader::CreateShaderConstantMap(
    efd::UInt32 passId, 
    NiGPUProgram::ProgramType shaderType,
    efd::UInt32)
{
    if (passId >= m_passArray.GetSize())
        return nullptr;

    D3D11Pass* pPass = m_passArray.GetAt(passId);
    if (!pPass)
        return nullptr;

    D3D11ShaderConstantMap* pSCM = EE_NEW D3D11ShaderConstantMap(shaderType);
    pPass->AddConstantMap(pSCM);

    return pSCM;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11FragmentShader::AppendTextureSampler(
    efd::UInt32 passId,
    efd::UInt32& /*samplerId*/, 
    const NiFixedString& semantic,
    const NiFixedString& variableName, 
    efd::UInt32 instance)
{
    if (passId >= m_passArray.GetSize())
        return false;

    // Currently, the sampler and the texture share the same name. 
    // Also, only pixel samplers are supported in the fragment shader system.
    efd::FixedString resourceName = variableName;
    efd::FixedString samplerName = variableName;

    D3D11Pass* pPass = m_passArray.GetAt(passId);
    if (!pPass)
        return false;

    efd::UInt32 gbTextureFlag = 0;
    if (NiTexturingProperty::GetMapIDFromName(semantic, gbTextureFlag))
    {
        switch (gbTextureFlag)
        {
        case NiTexturingProperty::BASE_INDEX:
            pPass->SetShaderResourceAsGamebryoMap(
                resourceName, 
                NiTextureStage::TSTF_NDL_BASE,
                0);
            break;
        case NiTexturingProperty::DARK_INDEX:
            pPass->SetShaderResourceAsGamebryoMap(
                resourceName,
                NiTextureStage::TSTF_NDL_DARK, 
                0);
            break;
        case NiTexturingProperty::DETAIL_INDEX:
            pPass->SetShaderResourceAsGamebryoMap(
                resourceName,
                NiTextureStage::TSTF_NDL_DETAIL, 
                0);
            break;
        case NiTexturingProperty::GLOSS_INDEX:
            pPass->SetShaderResourceAsGamebryoMap(
                resourceName,
                NiTextureStage::TSTF_NDL_GLOSS, 
                0);
            break;
        case NiTexturingProperty::GLOW_INDEX:
            pPass->SetShaderResourceAsGamebryoMap(
                resourceName,
                NiTextureStage::TSTF_NDL_GLOW, 
                0);
            break;
        case NiTexturingProperty::BUMP_INDEX:
            pPass->SetShaderResourceAsGamebryoMap(
                resourceName,
                NiTextureStage::TSTF_NDL_BUMP, 
                0);
            break;
        case NiTexturingProperty::NORMAL_INDEX:
            pPass->SetShaderResourceAsGamebryoMap(
                resourceName,
                NiTextureStage::TSTF_NDL_NORMAL, 
                0);
            break;
        case NiTexturingProperty::PARALLAX_INDEX:
            pPass->SetShaderResourceAsGamebryoMap(
                resourceName,
                NiTextureStage::TSTF_NDL_PARALLAX, 
                0);
            break;
        case NiTexturingProperty::DECAL_BASE:
            pPass->SetShaderResourceAsGamebryoMap(
                resourceName,
                NiTextureStage::TSTF_MAP_DECAL, 
                (efd::UInt16)instance);
            break;
        case NiTexturingProperty::SHADER_BASE:
            pPass->SetShaderResourceAsGamebryoMap(
                resourceName,
                NiTextureStage::TSTF_MAP_SHADER, 
                (efd::UInt16)instance);
            break;
        default:
            EE_ASSERT(!"Unknown map entry!");
            return false;
            break;
        }
    }
    else if (NiTextureEffect::GetTypeIDFromName(semantic, gbTextureFlag))
    {
        efd::UInt16 objectTextureFlags = 0;
        switch ((NiTextureEffect::TextureType)gbTextureFlag)
        {
        case NiTextureEffect::PROJECTED_LIGHT:
            objectTextureFlags = static_cast<efd::UInt16>(
                (objectTextureFlags & ~NiTextureStage::TSOTF_TYPE_MASK) |
                (NiShaderAttributeDesc::OT_EFFECT_PROJECTEDLIGHTMAP <<
                NiTextureStage::TSOTF_TYPE_SHIFT));
            break;
        case NiTextureEffect::PROJECTED_SHADOW:
            objectTextureFlags = static_cast<efd::UInt16>(
                (objectTextureFlags & ~NiTextureStage::TSOTF_TYPE_MASK) |
                (NiShaderAttributeDesc::OT_EFFECT_PROJECTEDSHADOWMAP <<
                NiTextureStage::TSOTF_TYPE_SHIFT));
            break;
        case NiTextureEffect::ENVIRONMENT_MAP:
            objectTextureFlags = static_cast<efd::UInt16>(
                (objectTextureFlags & ~NiTextureStage::TSOTF_TYPE_MASK) |
                (NiShaderAttributeDesc::OT_EFFECT_ENVIRONMENTMAP <<
                NiTextureStage::TSOTF_TYPE_SHIFT));
            break;
        case NiTextureEffect::FOG_MAP:
            objectTextureFlags = static_cast<efd::UInt16>(
                (objectTextureFlags & ~NiTextureStage::TSOTF_TYPE_MASK) |
                (NiShaderAttributeDesc::OT_EFFECT_FOGMAP <<
                NiTextureStage::TSOTF_TYPE_SHIFT));
            break;
        case NiTextureEffect::LPP_GBUFFER:
            objectTextureFlags = static_cast<efd::UInt16>(
                (objectTextureFlags & ~NiTextureStage::TSOTF_TYPE_MASK) |
                (NiShaderAttributeDesc::OT_EFFECT_LPP_GBUFFER <<
                NiTextureStage::TSOTF_TYPE_SHIFT));
            break;
        case NiTextureEffect::LPP_LBUFFER:
            objectTextureFlags = static_cast<efd::UInt16>(
                (objectTextureFlags & ~NiTextureStage::TSOTF_TYPE_MASK) |
                (NiShaderAttributeDesc::OT_EFFECT_LPP_LBUFFER <<
                NiTextureStage::TSOTF_TYPE_SHIFT));
            break;
        default:
            EE_ASSERT(!"Unknown texture effect type");
            break;
        }

        objectTextureFlags = static_cast<efd::UInt16>(
            (objectTextureFlags & ~NiTextureStage::TSOTF_INDEX_MASK) | instance);

        pPass->SetShaderResourceAsTextureObject(
            resourceName,
            objectTextureFlags);
    }
    else if (NiShadowMap::GetLightTypeFromName(semantic, gbTextureFlag))
    {
        efd::UInt16 objectTextureFlags = 0;
        switch ((NiStandardMaterial::LightType)gbTextureFlag)
        {
        case NiStandardMaterial::LIGHT_DIR:
            objectTextureFlags = static_cast<efd::UInt16>(
                (objectTextureFlags & ~NiTextureStage::TSOTF_TYPE_MASK) |
                (NiShaderAttributeDesc::OT_EFFECT_DIRSHADOWMAP <<
                NiTextureStage::TSOTF_TYPE_SHIFT));
            break;
        case NiStandardMaterial::LIGHT_SPOT:
            objectTextureFlags = static_cast<efd::UInt16>(
                (objectTextureFlags & ~NiTextureStage::TSOTF_TYPE_MASK) |
                (NiShaderAttributeDesc::OT_EFFECT_SPOTSHADOWMAP <<
                NiTextureStage::TSOTF_TYPE_SHIFT));
            break;
        case NiStandardMaterial::LIGHT_POINT:
            objectTextureFlags = static_cast<efd::UInt16>(
                (objectTextureFlags & ~NiTextureStage::TSOTF_TYPE_MASK) |
                (NiShaderAttributeDesc::OT_EFFECT_POINTSHADOWMAP <<
                NiTextureStage::TSOTF_TYPE_SHIFT));
            break;
        default:
            EE_ASSERT(!"Unknown light type");
            break;
        };

        objectTextureFlags = static_cast<efd::UInt16>(
            (objectTextureFlags & ~NiTextureStage::TSOTF_INDEX_MASK) | instance);

        pPass->SetShaderResourceAsTextureObject(
            resourceName,
            objectTextureFlags);
    }
    else if (NiNoiseTexture::GetTypeIDFromName(semantic, gbTextureFlag))
    {
        efd::UInt16 objectTextureFlags = 0;

        switch ((NiStandardMaterial::NoiseMapType)gbTextureFlag)
        {
        case NiStandardMaterial::NOISE_GREYSCALE:
            objectTextureFlags = static_cast<efd::UInt16>(
                (objectTextureFlags & ~NiTextureStage::TSOTF_TYPE_MASK) |
                (NiShaderAttributeDesc::OT_EFFECT_PSSMSLICENOISEMASK <<
                NiTextureStage::TSOTF_TYPE_SHIFT));
            break;
        default:
            EE_ASSERT(!"Unknown noise type");
            break;
        };

        objectTextureFlags = static_cast<efd::UInt16>(
            (objectTextureFlags & ~NiTextureStage::TSOTF_INDEX_MASK) |
            instance);

        pPass->SetShaderResourceAsTextureObject(
            resourceName,
            objectTextureFlags);
    }

    D3D11RenderStateGroup* pRSGroup = pPass->GetRenderStateGroup();
    if (pRSGroup == NULL)
    {
        pRSGroup = EE_NEW D3D11RenderStateGroup;
        pPass->SetRenderStateGroup(pRSGroup);
    }
    pRSGroup->AddSampler(samplerName);

    return true;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11FragmentShader::Initialize()
{
    if (m_bInitialized)
        return true;

    // All bone matrices will be transposed, 3-bone matrices in world space
    SetBoneParameters(true, 3, true);

    if (!D3D11ShaderCore::Initialize())
        return false;

    return true;
}

//------------------------------------------------------------------------------------------------
efd::UInt32 D3D11FragmentShader::UpdatePipeline(const NiRenderCallContext& callContext)
{
    const efd::UInt32 passCount = m_passArray.GetSize();
    for (efd::UInt32 i = 0; i < passCount; i++)
    {
        D3D11Pass* pPass = m_passArray.GetAt(i);
        if (!pPass)
            continue;

        const efd::UInt32 samplerCount = pPass->GetShaderResourceCount();

        for (efd::UInt8 j = 0; j < samplerCount; j++)
            PrepareResource(callContext, j, pPass, true);
    }

    return D3D11ShaderCore::UpdatePipeline(callContext);
}

//------------------------------------------------------------------------------------------------
const NiShader::NiShaderInstanceDescriptor* D3D11FragmentShader::GetShaderInstanceDesc() const
{
    return &m_descriptor;
}

//------------------------------------------------------------------------------------------------
