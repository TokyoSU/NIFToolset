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

#include "NiEnvironmentPCH.h"

#include <NiMaterialFragmentNode.h>
#include <NiMaterialNodeLibrary.h>
#include <NiMaterialResource.h>
#include <NiCodeBlock.h>
#include "NiSkyMaterialNodeLibrary.h"

//--------------------------------------------------------------------------------------------------
EE_NOINLINE static void CreateFragment0(NiMaterialNodeLibrary* pkLib)
{
    NiMaterialFragmentNode* pkFrag = NiNew NiMaterialFragmentNode();

    pkFrag->SetType("Vertex");
    pkFrag->SetName("TransformPosition");
    pkFrag->SetDescription("\n"
        "            This fragment is responsible for applying a transform to the\n"
        "            incoming vertex position. used to transform to/from W, WV and WVP\n"
        "        ");
    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetSemantic("Position");
        pkRes->SetLabel("Local");
        pkRes->SetVariable("OriginalPos");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float4x4");
        pkRes->SetVariable("TransformMatrix");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float4");
        pkRes->SetSemantic("Position");
        pkRes->SetLabel("World");
        pkRes->SetVariable("TransformPos");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert a code block
    {
        NiCodeBlock* pkBlock = NiNew NiCodeBlock();

        pkBlock->SetLanguage("hlsl/Cg");
        pkBlock->SetPlatform("D3D11/D3D10/DX9/Xenon/PS3");
        pkBlock->SetTarget("vs_1_1/ps_2_0/vs_3_0/ps_3_0/vs_4_0/ps_4_0/vs_5_0/ps_5_0");

        pkBlock->SetText("\n"
             "            TransformPos = mul(float4(OriginalPos,1), TransformMatrix);\n"
             "        ");


        pkFrag->AddCodeBlock(pkBlock);
    }

    pkLib->AddNode(pkFrag);
}
//--------------------------------------------------------------------------------------------------
EE_NOINLINE static void CreateFragment1(NiMaterialNodeLibrary* pkLib)
{
    NiMaterialFragmentNode* pkFrag = NiNew NiMaterialFragmentNode();

    pkFrag->SetType("Vertex");
    pkFrag->SetName("TransformDirection");
    pkFrag->SetDescription("\n"
        "            This fragment is responsible for applying a transform to an incomming\n"
        "            direction. this allows the rotation of all directions into and out of\n"
        "            different spaces. \n"
        "        ");
    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetLabel("Local");
        pkRes->SetVariable("OriginalDir");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float4x4");
        pkRes->SetVariable("TransformMatrix");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetLabel("World");
        pkRes->SetVariable("TransformDir");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert a code block
    {
        NiCodeBlock* pkBlock = NiNew NiCodeBlock();

        pkBlock->SetLanguage("hlsl/Cg");
        pkBlock->SetPlatform("D3D11/D3D10/DX9/Xenon/PS3");
        pkBlock->SetTarget("vs_1_1/ps_2_0/vs_3_0/ps_3_0/vs_4_0/ps_4_0/vs_5_0/ps_5_0");

        pkBlock->SetText("\n"
             "            TransformDir = mul(OriginalDir, (float3x3)TransformMatrix);\n"
             "        ");


        pkFrag->AddCodeBlock(pkBlock);
    }

    pkLib->AddNode(pkFrag);
}
//--------------------------------------------------------------------------------------------------
EE_NOINLINE static void CreateFragment2(NiMaterialNodeLibrary* pkLib)
{
    NiMaterialFragmentNode* pkFrag = NiNew NiMaterialFragmentNode();

    pkFrag->SetType("Pixel");
    pkFrag->SetName("DestroyDepthInformation");
    pkFrag->SetDescription("\n"
        "            This fragment is responsible for destroying the depth value of\n"
        "            the particular vertex such that it is not clipped by the far plane.\n"
        "        ");
    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float4");
        pkRes->SetVariable("posProjectedIn");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float4");
        pkRes->SetVariable("posProjectedOut");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert a code block
    {
        NiCodeBlock* pkBlock = NiNew NiCodeBlock();

        pkBlock->SetLanguage("hlsl/Cg");
        pkBlock->SetPlatform("D3D11/D3D10/DX9/Xenon/PS3");
        pkBlock->SetTarget("vs_1_1/ps_2_0/vs_3_0/ps_3_0/vs_4_0/ps_4_0/vs_5_0/ps_5_0");

        pkBlock->SetText("\n"
             "            posProjectedOut = posProjectedIn.xyww;\n"
             "            posProjectedOut.z = 0.5f;\n"
             "        ");


        pkFrag->AddCodeBlock(pkBlock);
    }

    pkLib->AddNode(pkFrag);
}
//--------------------------------------------------------------------------------------------------
EE_NOINLINE static void CreateFragment3(NiMaterialNodeLibrary* pkLib)
{
    NiMaterialFragmentNode* pkFrag = NiNew NiMaterialFragmentNode();

    pkFrag->SetType("Vertex");
    pkFrag->SetName("CalcViewVector");
    pkFrag->SetDescription("\n"
        "            This fragment is responsible for calculating the view vector (from \n"
        "            camera to world position). \n"
        "        ");
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

        pkRes->SetType("float3");
        pkRes->SetVariable("WorldPos");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetSemantic("Position");
        pkRes->SetLabel("World");
        pkRes->SetVariable("ViewVector");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert a code block
    {
        NiCodeBlock* pkBlock = NiNew NiCodeBlock();

        pkBlock->SetLanguage("hlsl/Cg");
        pkBlock->SetPlatform("D3D11/D3D10/DX9/Xenon/PS3");
        pkBlock->SetTarget("vs_1_1/ps_2_0/vs_3_0/ps_3_0/vs_4_0/ps_4_0/vs_5_0/ps_5_0");

        pkBlock->SetText("\n"
             "            ViewVector = normalize(WorldPos - CameraPos);\n"
             "        ");


        pkFrag->AddCodeBlock(pkBlock);
    }

    pkLib->AddNode(pkFrag);
}
//--------------------------------------------------------------------------------------------------
EE_NOINLINE static void CreateFragment4(NiMaterialNodeLibrary* pkLib)
{
    NiMaterialFragmentNode* pkFrag = NiNew NiMaterialFragmentNode();

    pkFrag->SetType("Vertex");
    pkFrag->SetName("CalcViewVector");
    pkFrag->SetDescription("\n"
        "            This fragment is responsible for calculating the view vector (from\n"
        "            camera to world position).\n"
        "        ");
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

        pkRes->SetType("float3");
        pkRes->SetVariable("WorldPos");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetSemantic("Position");
        pkRes->SetLabel("World");
        pkRes->SetVariable("ViewVector");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert a code block
    {
        NiCodeBlock* pkBlock = NiNew NiCodeBlock();

        pkBlock->SetLanguage("hlsl/Cg");
        pkBlock->SetPlatform("D3D11/D3D10/DX9/Xenon/PS3");
        pkBlock->SetTarget("vs_1_1/ps_2_0/vs_3_0/ps_3_0/vs_4_0/ps_4_0/vs_5_0/ps_5_0");

        pkBlock->SetText("\n"
             "            ViewVector = normalize(WorldPos - CameraPos);\n"
             "        ");


        pkFrag->AddCodeBlock(pkBlock);
    }

    pkLib->AddNode(pkFrag);
}
//--------------------------------------------------------------------------------------------------
EE_NOINLINE static void CreateFragment5(NiMaterialNodeLibrary* pkLib)
{
    NiMaterialFragmentNode* pkFrag = NiNew NiMaterialFragmentNode();

    pkFrag->SetType("Pixel");
    pkFrag->SetName("SampleCubeMap");
    pkFrag->SetDescription("\n"
        "            This fragment is responsible for sampling a specific point in a \n"
        "            cube map for skyboxes.\n"
        "        ");
    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("samplerCUBE");
        pkRes->SetSemantic("Texture");
        pkRes->SetVariable("Sampler");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetVariable("SamplePoint");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float4");
        pkRes->SetVariable("OutColor");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert a code block
    {
        NiCodeBlock* pkBlock = NiNew NiCodeBlock();

        pkBlock->SetLanguage("hlsl/Cg");
        pkBlock->SetPlatform("D3D11/D3D10/DX9/Xenon/PS3");
        pkBlock->SetTarget("vs_1_1/ps_2_0/vs_3_0/ps_3_0/vs_4_0/ps_4_0/vs_5_0/ps_5_0");

        pkBlock->SetText("\n"
             "            OutColor = texCUBE(Sampler, normalize(-SamplePoint.xyz));\n"
             "        ");


        pkFrag->AddCodeBlock(pkBlock);
    }

    pkLib->AddNode(pkFrag);
}
//--------------------------------------------------------------------------------------------------
EE_NOINLINE static void CreateFragment6(NiMaterialNodeLibrary* pkLib)
{
    NiMaterialFragmentNode* pkFrag = NiNew NiMaterialFragmentNode();

    pkFrag->SetType("Pixel");
    pkFrag->SetName("SampleOrientedCubeMap");
    pkFrag->SetDescription("\n"
        "            This fragment is responsible for sampling a specific point in a \n"
        "            cube map for skyboxes.\n"
        "        ");
    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("samplerCUBE");
        pkRes->SetSemantic("Texture");
        pkRes->SetVariable("Sampler");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetVariable("SamplePoint");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float4");
        pkRes->SetVariable("OutColor");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert a code block
    {
        NiCodeBlock* pkBlock = NiNew NiCodeBlock();

        pkBlock->SetLanguage("hlsl/Cg");
        pkBlock->SetPlatform("D3D11/D3D10/DX9/Xenon/PS3");
        pkBlock->SetTarget("vs_1_1/ps_2_0/vs_3_0/ps_3_0/vs_4_0/ps_4_0/vs_5_0/ps_5_0");

        pkBlock->SetText("\n"
             "            OutColor = texCUBE(Sampler, normalize(-SamplePoint.xzy));\n"
             "        ");


        pkFrag->AddCodeBlock(pkBlock);
    }

    pkLib->AddNode(pkFrag);
}
//--------------------------------------------------------------------------------------------------
EE_NOINLINE static void CreateFragment7(NiMaterialNodeLibrary* pkLib)
{
    NiMaterialFragmentNode* pkFrag = NiNew NiMaterialFragmentNode();

    pkFrag->SetType("Pixel");
    pkFrag->SetName("BlendValueMultiply");
    pkFrag->SetDescription("\n"
        "            This fragment is responsible for blending two values together using\n"
        "            a multiply operation.\n"
        "        ");
    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float4");
        pkRes->SetVariable("ValueA");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float4");
        pkRes->SetVariable("ValueB");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float");
        pkRes->SetVariable("Modifier");
        pkRes->SetDefaultValue("(1)");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float4");
        pkRes->SetVariable("OutValue");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert a code block
    {
        NiCodeBlock* pkBlock = NiNew NiCodeBlock();

        pkBlock->SetLanguage("hlsl/Cg");
        pkBlock->SetPlatform("D3D11/D3D10/DX9/Xenon/PS3");
        pkBlock->SetTarget("vs_1_1/ps_2_0/vs_3_0/ps_3_0/vs_4_0/ps_4_0/vs_5_0/ps_5_0");

        pkBlock->SetText("\n"
             "            OutValue = ValueA * ValueB * Modifier;\n"
             "        ");


        pkFrag->AddCodeBlock(pkBlock);
    }

    pkLib->AddNode(pkFrag);
}
//--------------------------------------------------------------------------------------------------
EE_NOINLINE static void CreateFragment8(NiMaterialNodeLibrary* pkLib)
{
    NiMaterialFragmentNode* pkFrag = NiNew NiMaterialFragmentNode();

    pkFrag->SetType("Pixel");
    pkFrag->SetName("BlendValueAdd");
    pkFrag->SetDescription("\n"
        "            This fragment is responsible for blending two values together using\n"
        "            an addition operation.\n"
        "        ");
    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float4");
        pkRes->SetVariable("ValueA");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float4");
        pkRes->SetVariable("ValueB");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float");
        pkRes->SetVariable("Modifier");
        pkRes->SetDefaultValue("(1)");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float4");
        pkRes->SetVariable("OutValue");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert a code block
    {
        NiCodeBlock* pkBlock = NiNew NiCodeBlock();

        pkBlock->SetLanguage("hlsl/Cg");
        pkBlock->SetPlatform("D3D11/D3D10/DX9/Xenon/PS3");
        pkBlock->SetTarget("vs_1_1/ps_2_0/vs_3_0/ps_3_0/vs_4_0/ps_4_0/vs_5_0/ps_5_0");

        pkBlock->SetText("\n"
             "            OutValue = ValueA + ValueB * Modifier;\n"
             "        ");


        pkFrag->AddCodeBlock(pkBlock);
    }

    pkLib->AddNode(pkFrag);
}
//--------------------------------------------------------------------------------------------------
EE_NOINLINE static void CreateFragment9(NiMaterialNodeLibrary* pkLib)
{
    NiMaterialFragmentNode* pkFrag = NiNew NiMaterialFragmentNode();

    pkFrag->SetType("Pixel");
    pkFrag->SetName("BlendValueInterpolate");
    pkFrag->SetDescription("\n"
        "            This fragment is responsible for blending two values together using\n"
        "            a linear interpolation.\n"
        "        ");
    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float4");
        pkRes->SetVariable("ValueA");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float4");
        pkRes->SetVariable("ValueB");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float");
        pkRes->SetVariable("Modifier");
        pkRes->SetDefaultValue("(0.5)");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float4");
        pkRes->SetVariable("OutValue");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert a code block
    {
        NiCodeBlock* pkBlock = NiNew NiCodeBlock();

        pkBlock->SetLanguage("hlsl/Cg");
        pkBlock->SetPlatform("D3D11/D3D10/DX9/Xenon/PS3");
        pkBlock->SetTarget("vs_1_1/ps_2_0/vs_3_0/ps_3_0/vs_4_0/ps_4_0/vs_5_0/ps_5_0");

        pkBlock->SetText("\n"
             "            OutValue = lerp(ValueA, ValueB, Modifier);\n"
             "        ");


        pkFrag->AddCodeBlock(pkBlock);
    }

    pkLib->AddNode(pkFrag);
}
//--------------------------------------------------------------------------------------------------
EE_NOINLINE static void CreateFragment10(NiMaterialNodeLibrary* pkLib)
{
    NiMaterialFragmentNode* pkFrag = NiNew NiMaterialFragmentNode();

    pkFrag->SetType("Pixel");
    pkFrag->SetName("ExtractAlphaValue");
    pkFrag->SetDescription("\n"
        "            This fragment is responsible for extracting alpha information from\n"
        "            a color.\n"
        "        ");
    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float4");
        pkRes->SetVariable("Value");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float4");
        pkRes->SetVariable("OutValue");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert a code block
    {
        NiCodeBlock* pkBlock = NiNew NiCodeBlock();

        pkBlock->SetLanguage("hlsl/Cg");
        pkBlock->SetPlatform("D3D11/D3D10/DX9/Xenon/PS3");
        pkBlock->SetTarget("vs_1_1/ps_2_0/vs_3_0/ps_3_0/vs_4_0/ps_4_0/vs_5_0/ps_5_0");

        pkBlock->SetText("\n"
             "            OutValue = Value.a;\n"
             "        ");


        pkFrag->AddCodeBlock(pkBlock);
    }

    pkLib->AddNode(pkFrag);
}
//--------------------------------------------------------------------------------------------------
EE_NOINLINE static void CreateFragment11(NiMaterialNodeLibrary* pkLib)
{
    NiMaterialFragmentNode* pkFrag = NiNew NiMaterialFragmentNode();

    pkFrag->SetType("Pixel");
    pkFrag->SetName("CalcHorizonBias");
    pkFrag->SetDescription("\n"
        "            This fragment is responsible for extracting alpha information from\n"
        "            a color.\n"
        "        ");
    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetVariable("Normal");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetVariable("UpVector");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float");
        pkRes->SetVariable("Exponent");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float");
        pkRes->SetVariable("HorizonBias");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float4");
        pkRes->SetVariable("OutValue");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert a code block
    {
        NiCodeBlock* pkBlock = NiNew NiCodeBlock();

        pkBlock->SetLanguage("hlsl/Cg");
        pkBlock->SetPlatform("D3D11/D3D10/DX9/Xenon/PS3");
        pkBlock->SetTarget("vs_1_1/ps_2_0/vs_3_0/ps_3_0/vs_4_0/ps_4_0/vs_5_0/ps_5_0");

        pkBlock->SetText("\n"
             "            float DotValue = dot(normalize(-Normal), UpVector);\n"
             "            float InterpValue = asin(DotValue);\n"
             "            OutValue = lerp(InterpValue, pow(1-DotValue, Exponent), HorizonBias);"
             "\n"
             "        ");


        pkFrag->AddCodeBlock(pkBlock);
    }

    pkLib->AddNode(pkFrag);
}
//--------------------------------------------------------------------------------------------------
NiMaterialNodeLibrary* NiSkyMaterialNodeLibrary::CreateMaterialNodeLibrary()
{
    // Create a new NiMaterialNodeLibrary
    NiMaterialNodeLibrary* pkLib = NiNew NiMaterialNodeLibrary(1);

    CreateFragment0(pkLib);
    CreateFragment1(pkLib);
    CreateFragment2(pkLib);
    CreateFragment3(pkLib);
    CreateFragment4(pkLib);
    CreateFragment5(pkLib);
    CreateFragment6(pkLib);
    CreateFragment7(pkLib);
    CreateFragment8(pkLib);
    CreateFragment9(pkLib);
    CreateFragment10(pkLib);
    CreateFragment11(pkLib);

    return pkLib;
}
//--------------------------------------------------------------------------------------------------

