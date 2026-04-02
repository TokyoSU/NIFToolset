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
// This file has been automatically generated using the
// NiMaterialNodeXMLLibraryParser tool. It should not be directly edited.
//--------------------------------------------------------------------------------------------------

#include "NiDecorationPCH.h"

#include <NiMaterialFragmentNode.h>
#include <NiMaterialNodeLibrary.h>
#include <NiMaterialResource.h>
#include <NiCodeBlock.h>
#include "NiDecorationMaterialNodeLibrary.h"

//--------------------------------------------------------------------------------------------------
EE_NOINLINE static void CreateFragment0(NiMaterialNodeLibrary* pkLib)
{
    NiMaterialFragmentNode* pkFrag = NiNew NiMaterialFragmentNode();

    pkFrag->SetType("Vertex/Pixel");
    pkFrag->SetName("WorldPositionToLayerUV");
    pkFrag->SetDescription("\n"
        "        Given world space coordinates, transforms the given\n"
        "    ");
    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetSemantic("Position");
        pkRes->SetVariable("WorldPos");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float4x4");
        pkRes->SetSemantic("LayerProj");
        pkRes->SetVariable("LayerProj");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float2");
        pkRes->SetSemantic("LayerInvWorldDimension");
        pkRes->SetVariable("LayerInvWorldDimension");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float2");
        pkRes->SetSemantic("TexCoord");
        pkRes->SetVariable("DecorationLayerUV");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert a code block
    {
        NiCodeBlock* pkBlock = NiNew NiCodeBlock();

        pkBlock->SetLanguage("hlsl/Cg");
        pkBlock->SetPlatform("D3D10/DX9/Xenon/PS3");
        pkBlock->SetTarget("vs_1_1/ps_2_0/vs_3_0/ps_3_0/vs_4_0/ps_4_0");

        pkBlock->SetText("\n"
             "          DecorationLayerUV = mul(WorldPos, LayerProj).xy;\n"
             "          DecorationLayerUV.x += LayerProj[3][0];\n"
             "          DecorationLayerUV.y += LayerProj[3][1];\n"
             "          DecorationLayerUV.x *= LayerInvWorldDimension.x;\n"
             "          DecorationLayerUV.y *= LayerInvWorldDimension.y;\n"
             "          DecorationLayerUV.x += 0.5;\n"
             "          // Flip the Y UV\n"
             "          DecorationLayerUV.y = 0.5 - DecorationLayerUV.y;\n"
             "    ");


        pkFrag->AddCodeBlock(pkBlock);
    }

    pkLib->AddNode(pkFrag);
}
//--------------------------------------------------------------------------------------------------
EE_NOINLINE static void CreateFragment1(NiMaterialNodeLibrary* pkLib)
{
    NiMaterialFragmentNode* pkFrag = NiNew NiMaterialFragmentNode();

    pkFrag->SetType("Vertex/Pixel");
    pkFrag->SetName("DistanceAlpha");
    pkFrag->SetDescription("\n"
        "          Given a maximum and minimum value, this fragment will fade out a mesh\n"
        "          using screen door transparency. It relies on a single channel texture\n"
        "          containing random noise which it uses in combination with a threshold\n"
        "          obtained from the current distance of the pixel from the camera in \n"
        "          relation to the min/max distances.\n"
        "    ");
    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetSemantic("Position");
        pkRes->SetVariable("WorldPos");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetSemantic("Position");
        pkRes->SetVariable("CameraPos");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float");
        pkRes->SetVariable("FadeOuterMinDistSqr");
        pkRes->SetDefaultValue("(6.0)");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float");
        pkRes->SetVariable("FadeOuterMaxDistSqr");
        pkRes->SetDefaultValue("(10.0)");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float");
        pkRes->SetVariable("FadeInnerMinDistSqr");
        pkRes->SetDefaultValue("(0.01)");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float");
        pkRes->SetVariable("FadeInnerMaxDistSqr");
        pkRes->SetDefaultValue("(0.0)");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float2");
        pkRes->SetSemantic("TexCoord");
        pkRes->SetVariable("MapUV");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("sampler2D");
        pkRes->SetSemantic("Texture");
        pkRes->SetVariable("FadeMask");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert a code block
    {
        NiCodeBlock* pkBlock = NiNew NiCodeBlock();

        pkBlock->SetLanguage("hlsl/Cg");
        pkBlock->SetPlatform("D3D10/DX9/Xenon/PS3");
        pkBlock->SetTarget("vs_1_1/ps_2_0/vs_3_0/ps_3_0/vs_4_0/ps_4_0");

        pkBlock->SetText("\n"
             "          float3 diff = WorldPos - CameraPos;     \n"
             "         diff.z = 0.0f;  \n"
             "          float fDiffDistSqr = dot(diff, diff);\n"
             "          float fInvOpacity = 0.0f;\n"
             "          if (fDiffDistSqr > FadeOuterMinDistSqr)\n"
             "          {\n"
             "            fInvOpacity = smoothstep(\n"
             "              FadeOuterMinDistSqr, FadeOuterMaxDistSqr, fDiffDistSqr);\n"
             "          }\n"
             "          else if (fDiffDistSqr < FadeInnerMaxDistSqr)\n"
             "          {\n"
             "            fInvOpacity = 1.0f - smoothstep(FadeInnerMinDistSqr, FadeInnerMaxDist"
             "Sqr, fDiffDistSqr);\n"
             "          }\n"
             "         \n"
             "          float fOpacity = tex2D(FadeMask, MapUV).x;\n"
             "          clip(fOpacity - fInvOpacity);\n"
             "    ");


        pkFrag->AddCodeBlock(pkBlock);
    }

    pkLib->AddNode(pkFrag);
}
//--------------------------------------------------------------------------------------------------
NiMaterialNodeLibrary* 
    NiDecorationMaterialNodeLibrary::CreateMaterialNodeLibrary()
{
    // Create a new NiMaterialNodeLibrary
    NiMaterialNodeLibrary* pkLib = NiNew NiMaterialNodeLibrary(1);

    CreateFragment0(pkLib);
    CreateFragment1(pkLib);

    return pkLib;
}
//--------------------------------------------------------------------------------------------------

