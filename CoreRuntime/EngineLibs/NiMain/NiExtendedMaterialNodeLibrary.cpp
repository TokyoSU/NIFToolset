#include "NiMainPCH.h"

#include "NiExtendedMaterialNodeLibrary.h"

#include <NiMaterialFragmentNode.h>
#include <NiMaterialNodeLibrary.h>
#include <NiMaterialResource.h>
#include <NiCodeBlock.h>
#include <NiStandardMaterialNodeLibrary.h>

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

    // sampler2DArray DiffuseArray
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();
        pkRes->SetType("sampler2DArray");
        pkRes->SetSemantic("Texture");
        pkRes->SetVariable("DiffuseArray");
        pkFrag->AddInputResource(pkRes);
    }

    // sampler2DArray AlphaArray
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();
        pkRes->SetType("sampler2DArray");
        pkRes->SetSemantic("Texture");
        pkRes->SetVariable("AlphaArray");
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

        pkBlock->SetText(
            "\n"
            "    int layerCount = min((int)TerrainInfo.x, 32);\n"
            "\n"
            "    float3 color = float3(0.0f, 0.0f, 0.0f);\n"
            "\n"
            "    [loop]\n"
            "    for (int i = 0; i < 32; ++i)\n"
            "    {\n"
            "        if (i >= layerCount)\n"
            "            break;\n"
            "\n"
            "        float4 data = LayerData[i];\n"
            "\n"
            "        float2 layerUV = UV * data.xy;\n"
            "\n"
            "        float alpha = tex2DArray(\n"
            "            AlphaArray,\n"
            "            float3(UV, (float)i)).r;\n"
            "\n"
            "        if (data.w > 0.5f)\n"
            "            alpha = 1.0f - alpha;\n"
            "\n"
            "        float coverage = saturate(alpha * data.z);\n"
            "\n"
            "        if (coverage > 0.001f)\n"
            "        {\n"
            "            float3 layerColor = tex2DArray(\n"
            "                DiffuseArray,\n"
            "                float3(layerUV, (float)i)).rgb;\n"
            "\n"
            "            color = lerp(color, layerColor, coverage);\n"
            "        }\n"
            "    }\n"
            "\n"
            "    ColorOut = color;\n"
            "    ");

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
