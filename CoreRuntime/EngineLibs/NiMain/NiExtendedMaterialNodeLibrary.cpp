#include "NiMainPCH.h"

#include "NiExtendedMaterialNodeLibrary.h"

#include <NiMaterialFragmentNode.h>
#include <NiMaterialNodeLibrary.h>
#include <NiMaterialResource.h>
#include <NiCodeBlock.h>
#include <NiStandardMaterialNodeLibrary.h>

static const char* TERRAIN_SPLAT_TEXTURE_ARRAY_HLSL = R"(
    int layerCount = min((int)TerrainInfo.x, 32);

    if (layerCount <= 0)
    {
        ColorOut = float3(1.0f, 1.0f, 1.0f);
    }
    else
    {
        float4 baseData = LayerData[0];

        float3 color = DiffuseArray.Sample(
            DiffuseArraySampler,
            float3(UV * baseData.xy, 0.0f)).rgb;

        [loop]
        for (int i = 1; i < 32; ++i)
        {
            if (i >= layerCount)
                break;

            float4 data = LayerData[i];

            float alpha = AlphaArray.Sample(
                AlphaArraySampler,
                float3(UV, (float)i)).r;

            if (data.w > 0.5f)
                alpha = 1.0f - alpha;

            float coverage = saturate(alpha * data.z);

            float3 layerColor = DiffuseArray.Sample(
                DiffuseArraySampler,
                float3(UV * data.xy, (float)i)).rgb;

            color = lerp(color, layerColor, coverage);
        }

        ColorOut = color;
    }
)";

static void AddTerrainSplatTextureArrayNode(NiMaterialNodeLibrary* pkLib)
{
    NiMaterialFragmentNode* pkFrag = NiNew NiMaterialFragmentNode();

    pkFrag->SetType("Pixel");
    pkFrag->SetName("TerrainSplatTextureArray");
    pkFrag->SetDescription(
        "Samples terrain diffuse/alpha texture arrays and "
        "returns the final terrain diffuse color before standard lighting.");

    // float2 UV
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();
        pkRes->SetType("float2");
        pkRes->SetSemantic("TexCoord");
        pkRes->SetVariable("UV");
        pkFrag->AddInputResource(pkRes);
    }

    // Texture2DArray DiffuseArray
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();
        pkRes->SetType("Texture2DArray");
        pkRes->SetSemantic("DiffuseTexture");
        pkRes->SetVariable("DiffuseArray");
        pkFrag->AddInputResource(pkRes);
    }

	// SamplerState DiffuseSampler
	{
		NiMaterialResource* pkRes = NiNew NiMaterialResource();
		pkRes->SetType("SamplerState");
		pkRes->SetSemantic("DiffuseSampler");
		pkRes->SetVariable("DiffuseArraySampler");
		pkFrag->AddInputResource(pkRes);
	}

    // Texture2DArray AlphaArray
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();
        pkRes->SetType("Texture2DArray");
        pkRes->SetSemantic("AlphaTexture");
        pkRes->SetVariable("AlphaArray");
        pkFrag->AddInputResource(pkRes);
    }

	// SamplerState AlphaSampler
	{
		NiMaterialResource* pkRes = NiNew NiMaterialResource();
		pkRes->SetType("SamplerState");
		pkRes->SetSemantic("AlphaSampler");
		pkRes->SetVariable("AlphaArraySampler");
		pkFrag->AddInputResource(pkRes);
	}

    // float4 TerrainInfo
    // x = layer count
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();
        pkRes->SetType("float4");
        pkRes->SetVariable("TerrainInfo");
        pkFrag->AddInputResource(pkRes);
    }

    // float4 LayerData[32]
    // x = scale U
    // y = scale V
    // z = weight
    // w = invert alpha
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();
        pkRes->SetType("float4");
        pkRes->SetVariable("LayerData");
        pkRes->SetCount(32);
        pkFrag->AddInputResource(pkRes);
    }

    // float3 ColorOut
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();
        pkRes->SetType("float3");
        pkRes->SetSemantic("Color");
        pkRes->SetVariable("ColorOut");
        pkFrag->AddOutputResource(pkRes);
    }

    {
        NiCodeBlock* pkBlock = NiNew NiCodeBlock();
        pkBlock->SetLanguage("hlsl/Cg");
        pkBlock->SetPlatform("D3D11/D3D10");
        pkBlock->SetTarget("ps_4_0/ps_5_0");
        pkBlock->SetText(TERRAIN_SPLAT_TEXTURE_ARRAY_HLSL);
        pkFrag->AddCodeBlock(pkBlock);
    }

    pkLib->AddNode(pkFrag);
}

NiMaterialNodeLibrary* NiExtendedMaterialNodeLibrary::CreateMaterialNodeLibrary()
{
    NiMaterialNodeLibrary* pkLib =
        NiStandardMaterialNodeLibrary::CreateMaterialNodeLibrary();

    AddTerrainSplatTextureArrayNode(pkLib);

    return pkLib;
}
