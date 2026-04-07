// GAMEBASE USA LLC PROPRIETARY INFORMATION
//
// This software is supplied under the terms of a license agreement or
// nondisclosure agreement with Gamebase USA LLC and may not
// be copied or disclosed except in accordance with the terms of that
// agreement.
//
//      Copyright (c) 1996-2011 Gamebase USA LLC.
//      All Rights Reserved.
//
// Gamebase USA LLC, Research Triangle Park, NC 27709
// http://www.gamebryo.com

#include "NiLightPrePassPCH.h"

#include "NiLPPMaterialSwapProcessor.h"
#include "NiLPPDepthNormalMaterial.h"
#include "NiLPPFinalMaterial.h"
#include <NiShaderFactory.h>
#include <NiDirectionalLight.h>
#include <NiPointLight.h>
#include <NiSpotLight.h>
#include <NiTextureEffect.h>
#include <NiStencilProperty.h>

#if defined(EE_PLATFORM_XBOX360) && defined(XBOX360_LPP_TILING)
#include <XenonNiRenderer.h>
#endif

//--------------------------------------------------------------------------------------------------
NiImplementRTTI(NiLPPMaterialSwapProcessorG, NiMaterialSwapProcessor);
NiImplementRTTI(NiLPPMaterialSwapProcessorF, NiMaterialSwapProcessor);
//-------------------------------------------------------------------------------------------------
NiLPPMaterialSwapProcessorG::NiLPPMaterialSwapProcessorG()
{
    m_bTransparentPass = false;
}

//-------------------------------------------------------------------------------------------------
void NiLPPMaterialSwapProcessorG::PreRenderProcessList(const NiVisibleArray* pkInput, 
    NiVisibleArray& kOutput, void* pvExtraData)
{
    if (!pkInput)
    {
        return;
    }
    const unsigned int uiInputCount = pkInput->GetCount();

    // Swap NiMaterials and stencil property
    kStencilFlags.RemoveAll();
    kStencilRefs.RemoveAll();
    kStencilMasks.RemoveAll();
    kStencilWriteMasks.RemoveAll();

	{
	unsigned short uFlags;
	unsigned int uiRef;
	unsigned int uiMask;
	unsigned int uiWriteMask;
    for (unsigned int ui = 0; ui < uiInputCount; ui++)
    {
        NiRenderObject& kMesh = pkInput->GetAt(ui);
        const NiMaterial* pkMaterial = kMesh.GetActiveMaterial();

        if (pkMaterial)
        {
            pkMaterial = pkMaterial->GetSwapMaterial(NiMaterial::SWAP_LPP_G);
        }

        kMesh.SwapMaterial(const_cast<NiMaterial*>(pkMaterial), NULL);

		NiStencilProperty* pkStencil = kMesh.GetPropertyState()->GetStencil();
        if (!pkStencil) // create a default stencil property if it doesn't exist
        {
            pkStencil = EE_NEW NiStencilProperty();
            kMesh.AttachProperty(pkStencil);
            kMesh.UpdateProperties();
        }

		// store stencil property before overwriting
        pkStencil->GetData(uFlags, uiRef, uiMask, uiWriteMask);
        kStencilFlags.Add(uFlags);
        kStencilRefs.Add(uiRef);
        kStencilMasks.Add(uiMask);
        kStencilWriteMasks.Add(uiWriteMask);

        // use stencil property for light groups
        pkStencil->SetStencilOn(true);
        pkStencil->SetStencilFunction(NiStencilProperty::TEST_ALWAYS);
        pkStencil->SetStencilReference(kMesh.GetLightGroupMask());
        pkStencil->SetStencilMask(0xFF);
        pkStencil->SetStencilPassAction(NiStencilProperty::ACTION_REPLACE);
        pkStencil->SetStencilPassZFailAction(NiStencilProperty::ACTION_KEEP);
        pkStencil->SetStencilFailAction(NiStencilProperty::ACTION_KEEP);
    }
	}

    NiShaderSortProcessor::PreRenderProcessList(pkInput, kOutput, pvExtraData);

    // Restore NiMaterials and stencil property
    for (efd::SInt32 index = uiInputCount - 1; index >= 0; --index)
    {
        NiRenderObject& kMesh = pkInput->GetAt(index);

        NiStencilProperty* pkStencil = kMesh.GetPropertyState()->GetStencil();

		pkStencil->SetData(
			kStencilFlags.GetAt(index),
			kStencilRefs.GetAt(index),
			kStencilMasks.GetAt(index),
			kStencilWriteMasks.GetAt(index));

		kMesh.RestoreSwapMaterial();
    }
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
NiLPPMaterialSwapProcessorF::NiLPPMaterialSwapProcessorF()
{
    m_bTransparentPass = false;
}

//-------------------------------------------------------------------------------------------------
void NiLPPMaterialSwapProcessorF::PreRenderProcessList(const NiVisibleArray* pkInput, NiVisibleArray& kOutput, void* pvExtraData)
{
    if (!pkInput)
    {
        return;
    }
    const unsigned int uiInputCount = pkInput->GetCount();

    // Swap NiMaterials
    for (unsigned int ui = 0; ui < uiInputCount; ui++)
    {
        NiRenderObject& kMesh = pkInput->GetAt(ui);
        const NiMaterial* pkMaterial = kMesh.GetActiveMaterial();
        if (pkMaterial)
        {
            pkMaterial = pkMaterial->GetSwapMaterial(NiMaterial::SWAP_LPP_F);
        }
        kMesh.SwapMaterial(const_cast<NiMaterial*>(pkMaterial), NULL);
    }

    NiShaderSortProcessor::PreRenderProcessList(pkInput, kOutput, pvExtraData);

    // Restore NiMaterials
    for (unsigned int ui = 0; ui < uiInputCount; ui++)
    {
        NiRenderObject& kMesh = pkInput->GetAt(ui);
        kMesh.RestoreSwapMaterial();
    }
}

//-------------------------------------------------------------------------------------------------
