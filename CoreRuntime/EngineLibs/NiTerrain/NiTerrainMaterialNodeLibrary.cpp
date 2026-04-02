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

#include "NiTerrainPCH.h"

#include <NiMaterialFragmentNode.h>
#include <NiMaterialNodeLibrary.h>
#include <NiMaterialResource.h>
#include <NiCodeBlock.h>
#include "NiTerrainMaterialNodeLibrary.h"

//--------------------------------------------------------------------------------------------------
EE_NOINLINE static void CreateFragment0(NiMaterialNodeLibrary* pkLib)
{
    NiMaterialFragmentNode* pkFrag = NiNew NiMaterialFragmentNode();

    pkFrag->SetType("Vertex");
    pkFrag->SetName("SplitPosition");
    pkFrag->SetDescription("\n"
        "      This fragment takes an incoming float4 vertex position with the xyz\n"
        "      components containing the position, and the w component containing the\n"
        "      height in the vertex of the next level of detail. The height or z value of\n"
        "      the final vertex position is interpolated between the 2 levels of detail.\n"
        "    ");
    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float4");
        pkRes->SetSemantic("Position");
        pkRes->SetLabel("Local");
        pkRes->SetVariable("CombinedPosition");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetSemantic("Position");
        pkRes->SetLabel("Local");
        pkRes->SetVariable("PositionHigh");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetSemantic("Position");
        pkRes->SetLabel("Local");
        pkRes->SetVariable("PositionLow");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert a code block
    {
        NiCodeBlock* pkBlock = NiNew NiCodeBlock();

        pkBlock->SetLanguage("hlsl/Cg");
        pkBlock->SetPlatform("D3D11/D3D10/DX9/Xenon/PS3");
        pkBlock->SetTarget("vs_1_1/ps_2_0/vs_3_0/ps_3_0/vs_4_0/ps_4_0/vs_5_0/ps_5_0");

        pkBlock->SetText("\n"
             "      PositionHigh = CombinedPosition.xyz;\n"
             "      PositionLow = CombinedPosition.xyw;\n"
             "    ");


        pkFrag->AddCodeBlock(pkBlock);
    }

    pkLib->AddNode(pkFrag);
}
//--------------------------------------------------------------------------------------------------
EE_NOINLINE static void CreateFragment1(NiMaterialNodeLibrary* pkLib)
{
    NiMaterialFragmentNode* pkFrag = NiNew NiMaterialFragmentNode();

    pkFrag->SetType("Vertex");
    pkFrag->SetName("SplitFloat4ToFloat2");
    pkFrag->SetDescription("\n"
        "      This fragment takes an incoming float4 encoding the xy of the vertex normal.\n"
        "      The zw components encode the xy of the lower lod vertex normal. The output\n"
        "      is a 3 component vertex normal of the incoming normal and the lower lod\n"
        "      normal.\n"
        "    ");
    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float4");
        pkRes->SetVariable("Combined");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float2");
        pkRes->SetVariable("FirstHalf");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float2");
        pkRes->SetVariable("SecondHalf");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert a code block
    {
        NiCodeBlock* pkBlock = NiNew NiCodeBlock();

        pkBlock->SetLanguage("hlsl/Cg");
        pkBlock->SetPlatform("D3D11/D3D10/DX9/Xenon/PS3");
        pkBlock->SetTarget("vs_1_1/ps_2_0/vs_3_0/ps_3_0/vs_4_0/ps_4_0/vs_5_0/ps_5_0");

        pkBlock->SetText("\n"
             "      FirstHalf = Combined.xy;\n"
             "      SecondHalf = Combined.zw;\n"
             "    ");


        pkFrag->AddCodeBlock(pkBlock);
    }

    pkLib->AddNode(pkFrag);
}
//--------------------------------------------------------------------------------------------------
EE_NOINLINE static void CreateFragment2(NiMaterialNodeLibrary* pkLib)
{
    NiMaterialFragmentNode* pkFrag = NiNew NiMaterialFragmentNode();

    pkFrag->SetType("Vertex");
    pkFrag->SetName("DecompressNormal");
    pkFrag->SetDescription("\n"
        "      This fragment will decompress a normal from its 2 component state.\n"
        "    ");
    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float2");
        pkRes->SetVariable("Compressed");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetVariable("Normal");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert a code block
    {
        NiCodeBlock* pkBlock = NiNew NiCodeBlock();

        pkBlock->SetLanguage("hlsl/Cg");
        pkBlock->SetPlatform("D3D11/D3D10/DX9/Xenon/PS3");
        pkBlock->SetTarget("vs_1_1/ps_2_0/vs_3_0/ps_3_0/vs_4_0/ps_4_0/vs_5_0/ps_5_0");

        pkBlock->SetText("\n"
             "      Normal = float3(Compressed.xy, 1.0);\n"
             "    ");


        pkFrag->AddCodeBlock(pkBlock);
    }

    pkLib->AddNode(pkFrag);
}
//--------------------------------------------------------------------------------------------------
EE_NOINLINE static void CreateFragment3(NiMaterialNodeLibrary* pkLib)
{
    NiMaterialFragmentNode* pkFrag = NiNew NiMaterialFragmentNode();

    pkFrag->SetType("Vertex");
    pkFrag->SetName("DecompressTangent");
    pkFrag->SetDescription("\n"
        "      This fragment will decompress a Tangent from it's 2 component state\n"
        "    ");
    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float2");
        pkRes->SetVariable("Compressed");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetVariable("Tangent");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert a code block
    {
        NiCodeBlock* pkBlock = NiNew NiCodeBlock();

        pkBlock->SetLanguage("hlsl/Cg");
        pkBlock->SetPlatform("D3D11/D3D10/DX9/Xenon/PS3");
        pkBlock->SetTarget("vs_1_1/ps_2_0/vs_3_0/ps_3_0/vs_4_0/ps_4_0/vs_5_0/ps_5_0");

        pkBlock->SetText("\n"
             "      Tangent = float3(Compressed.x, 0.0, Compressed.y);\n"
             "    ");


        pkFrag->AddCodeBlock(pkBlock);
    }

    pkLib->AddNode(pkFrag);
}
//--------------------------------------------------------------------------------------------------
EE_NOINLINE static void CreateFragment4(NiMaterialNodeLibrary* pkLib)
{
    NiMaterialFragmentNode* pkFrag = NiNew NiMaterialFragmentNode();

    pkFrag->SetType("Vertex");
    pkFrag->SetName("TransformPositionToWorld");
    pkFrag->SetDescription("\n"
        "      This fragment is responsible for applying the world transform to the\n"
        "      incoming local vertex position.\n"
        "    ");
    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetSemantic("Position");
        pkRes->SetLabel("Local");
        pkRes->SetVariable("LocalPosition");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float4x4");
        pkRes->SetSemantic("WorldMatrix");
        pkRes->SetVariable("WorldMatrix");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float4");
        pkRes->SetSemantic("Position");
        pkRes->SetLabel("World");
        pkRes->SetVariable("WorldPos");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert a code block
    {
        NiCodeBlock* pkBlock = NiNew NiCodeBlock();

        pkBlock->SetLanguage("hlsl/Cg");
        pkBlock->SetPlatform("D3D11/D3D10/DX9/Xenon/PS3");
        pkBlock->SetTarget("vs_1_1/ps_2_0/vs_3_0/ps_3_0/vs_4_0/ps_4_0/vs_5_0/ps_5_0");

        pkBlock->SetText("\n"
             "      WorldPos = mul(float4(LocalPosition,1), WorldMatrix);\n"
             "    ");


        pkFrag->AddCodeBlock(pkBlock);
    }

    pkLib->AddNode(pkFrag);
}
//--------------------------------------------------------------------------------------------------
EE_NOINLINE static void CreateFragment5(NiMaterialNodeLibrary* pkLib)
{
    NiMaterialFragmentNode* pkFrag = NiNew NiMaterialFragmentNode();

    pkFrag->SetType("Vertex");
    pkFrag->SetName("TransformPositionToView");
    pkFrag->SetDescription("\n"
        "      This fragment is responsible for applying the view transform to the\n"
        "      incoming world space vertex position.\n"
        "    ");
    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float4");
        pkRes->SetSemantic("Position");
        pkRes->SetLabel("World");
        pkRes->SetVariable("WorldPosition");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float4x4");
        pkRes->SetSemantic("ViewMatrix");
        pkRes->SetVariable("ViewMatrix");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetSemantic("Position");
        pkRes->SetLabel("View");
        pkRes->SetVariable("ViewPos");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert a code block
    {
        NiCodeBlock* pkBlock = NiNew NiCodeBlock();

        pkBlock->SetLanguage("hlsl/Cg");
        pkBlock->SetPlatform("D3D11/D3D10/DX9/Xenon/PS3");
        pkBlock->SetTarget("vs_1_1/ps_2_0/vs_3_0/ps_3_0/vs_4_0/ps_4_0/vs_5_0/ps_5_0");

        pkBlock->SetText("\n"
             "      ViewPos = mul(WorldPosition, ViewMatrix).xyz;\n"
             "    ");


        pkFrag->AddCodeBlock(pkBlock);
    }

    pkLib->AddNode(pkFrag);
}
//--------------------------------------------------------------------------------------------------
EE_NOINLINE static void CreateFragment6(NiMaterialNodeLibrary* pkLib)
{
    NiMaterialFragmentNode* pkFrag = NiNew NiMaterialFragmentNode();

    pkFrag->SetType("Vertex");
    pkFrag->SetName("ApplyWorldRotation");
    pkFrag->SetDescription("\n"
        "      This fragment only applys the rotation portion of the world transformation to\n"
        "      the incoming float3.\n"
        "    ");
    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetVariable("ValueToRotate");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float4x4");
        pkRes->SetSemantic("WorldMatrix");
        pkRes->SetVariable("WorldMatrix");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetLabel("World");
        pkRes->SetVariable("WorldRotationOfValue");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert a code block
    {
        NiCodeBlock* pkBlock = NiNew NiCodeBlock();

        pkBlock->SetLanguage("hlsl/Cg");
        pkBlock->SetPlatform("D3D11/D3D10/DX9/Xenon/PS3");
        pkBlock->SetTarget("vs_1_1/ps_2_0/vs_3_0/ps_3_0/vs_4_0/ps_4_0/vs_5_0/ps_5_0");

        pkBlock->SetText("\n"
             "      WorldRotationOfValue = mul(ValueToRotate, (float3x3)WorldMatrix);\n"
             "    ");


        pkFrag->AddCodeBlock(pkBlock);
    }

    pkLib->AddNode(pkFrag);
}
//--------------------------------------------------------------------------------------------------
EE_NOINLINE static void CreateFragment7(NiMaterialNodeLibrary* pkLib)
{
    NiMaterialFragmentNode* pkFrag = NiNew NiMaterialFragmentNode();

    pkFrag->SetType("Vertex");
    pkFrag->SetName("TransformPositionToClip");
    pkFrag->SetDescription("\n"
        "      This fragment is responsible for applying the view-projection matrix\n"
        "      to the incoming world space vertex position.\n"
        "    ");
    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float4");
        pkRes->SetSemantic("Position");
        pkRes->SetLabel("World");
        pkRes->SetVariable("WorldPosition");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float4x4");
        pkRes->SetSemantic("ViewProjMatrix");
        pkRes->SetVariable("ViewProjMatrix");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float4");
        pkRes->SetSemantic("Position");
        pkRes->SetLabel("World");
        pkRes->SetVariable("WorldProj");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert a code block
    {
        NiCodeBlock* pkBlock = NiNew NiCodeBlock();

        pkBlock->SetLanguage("hlsl/Cg");
        pkBlock->SetPlatform("D3D11/D3D10/DX9/Xenon/PS3");
        pkBlock->SetTarget("vs_1_1/ps_2_0/vs_3_0/ps_3_0/vs_4_0/ps_4_0/vs_5_0/ps_5_0");

        pkBlock->SetText("\n"
             "      WorldProj = mul(WorldPosition, ViewProjMatrix);\n"
             "    ");


        pkFrag->AddCodeBlock(pkBlock);
    }

    pkLib->AddNode(pkFrag);
}
//--------------------------------------------------------------------------------------------------
EE_NOINLINE static void CreateFragment8(NiMaterialNodeLibrary* pkLib)
{
    NiMaterialFragmentNode* pkFrag = NiNew NiMaterialFragmentNode();

    pkFrag->SetType("Vertex");
    pkFrag->SetName("CalculateTextureCoordinates");
    pkFrag->SetDescription("\n"
        "      Scales and offsets the specified texture coordinate based on the block's\n"
        "      level of detail. This is too allow blocks to share the same UV set instead\n"
        "      of allocate once large UV set for each layer.\n"
        "    ");
    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float2");
        pkRes->SetSemantic("TexCoord");
        pkRes->SetVariable("UVCoord");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float2");
        pkRes->SetVariable("UVBlendScale");
        pkRes->SetDefaultValue("(1.0, 1.0)");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float2");
        pkRes->SetVariable("UVBlendOffset");
        pkRes->SetDefaultValue("(0.0, 0.0)");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float2");
        pkRes->SetVariable("UVLowResScale");
        pkRes->SetDefaultValue("(1.0, 1.0)");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float2");
        pkRes->SetVariable("UVLowResOffset");
        pkRes->SetDefaultValue("(0.0, 0.0)");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float4");
        pkRes->SetSemantic("TexCoord");
        pkRes->SetVariable("OutputUVCoord");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert a code block
    {
        NiCodeBlock* pkBlock = NiNew NiCodeBlock();

        pkBlock->SetLanguage("hlsl/Cg");
        pkBlock->SetPlatform("D3D11/D3D10/DX9/Xenon/PS3");
        pkBlock->SetTarget("vs_1_1/ps_2_0/vs_3_0/ps_3_0/vs_4_0/ps_4_0/vs_5_0/ps_5_0");

        pkBlock->SetText("\n"
             "        OutputUVCoord.zw = UVCoord * UVBlendScale + UVBlendOffset;\n"
             "        OutputUVCoord.xy = UVCoord * UVLowResScale + UVLowResOffset;\n"
             "    ");


        pkFrag->AddCodeBlock(pkBlock);
    }

    pkLib->AddNode(pkFrag);
}
//--------------------------------------------------------------------------------------------------
EE_NOINLINE static void CreateFragment9(NiMaterialNodeLibrary* pkLib)
{
    NiMaterialFragmentNode* pkFrag = NiNew NiMaterialFragmentNode();

    pkFrag->SetType("Pixel");
    pkFrag->SetName("PulseUnpaintedArea");
    pkFrag->SetDescription("\n"
        "      This fragment will pulse a gradient color in areas of the terrain that\n"
        "      aren't painted completely in time with the game timer.\n"
        "    ");
    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetVariable("LowColor");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetVariable("HighColor");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetVariable("NormalColor");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float");
        pkRes->SetVariable("TotalMask");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float");
        pkRes->SetVariable("Time");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetVariable("OutputColor");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert a code block
    {
        NiCodeBlock* pkBlock = NiNew NiCodeBlock();

        pkBlock->SetLanguage("hlsl/Cg");
        pkBlock->SetPlatform("D3D11/D3D10/DX9/Xenon/PS3");
        pkBlock->SetTarget("vs_1_1/ps_2_0/vs_3_0/ps_3_0/vs_4_0/ps_4_0/vs_5_0/ps_5_0");

        pkBlock->SetText("\n"
             "      OutputColor = NormalColor;\n"
             "      \n"
             "      if (TotalMask < 0.99f)\n"
             "      {\n"
             "        float3 gradientColor = lerp(HighColor, LowColor, saturate(pow(TotalMask, "
             "11.0)));\n"
             "        OutputColor = lerp(NormalColor, gradientColor, ((sin(Time * 1.5) + 1.0) /"
             " 2.0) * 0.6);\n"
             "      }\n"
             "      \n"
             "    ");


        pkFrag->AddCodeBlock(pkBlock);
    }

    pkLib->AddNode(pkFrag);
}
//--------------------------------------------------------------------------------------------------
EE_NOINLINE static void CreateFragment10(NiMaterialNodeLibrary* pkLib)
{
    NiMaterialFragmentNode* pkFrag = NiNew NiMaterialFragmentNode();

    pkFrag->SetType("Vertex/Pixel");
    pkFrag->SetName("LerpFloat4");
    pkFrag->SetDescription("\n"
        "      Linearly interpolates a float4.\n"
        "    ");
    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float4");
        pkRes->SetVariable("InputOne");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float4");
        pkRes->SetVariable("InputTwo");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float");
        pkRes->SetVariable("Amount");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float4");
        pkRes->SetVariable("InterpolatedOutput");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert a code block
    {
        NiCodeBlock* pkBlock = NiNew NiCodeBlock();

        pkBlock->SetLanguage("hlsl/Cg");
        pkBlock->SetPlatform("D3D11/D3D10/DX9/Xenon/PS3");
        pkBlock->SetTarget("vs_1_1/ps_2_0/vs_3_0/ps_3_0/vs_4_0/ps_4_0/vs_5_0/ps_5_0");

        pkBlock->SetText("\n"
             "      InterpolatedOutput = lerp(InputOne, InputTwo, Amount);\n"
             "    ");


        pkFrag->AddCodeBlock(pkBlock);
    }

    pkLib->AddNode(pkFrag);
}
//--------------------------------------------------------------------------------------------------
EE_NOINLINE static void CreateFragment11(NiMaterialNodeLibrary* pkLib)
{
    NiMaterialFragmentNode* pkFrag = NiNew NiMaterialFragmentNode();

    pkFrag->SetType("Vertex/Pixel");
    pkFrag->SetName("LerpFloat4ToFloat3Result");
    pkFrag->SetDescription("\n"
        "      Linearly interpolates a two float4 values but only outputs the xyz components\n"
        "      of the result.\n"
        "    ");
    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float4");
        pkRes->SetVariable("InputOne");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float4");
        pkRes->SetVariable("InputTwo");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float");
        pkRes->SetVariable("Amount");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetVariable("InterpolatedOutput");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert a code block
    {
        NiCodeBlock* pkBlock = NiNew NiCodeBlock();

        pkBlock->SetLanguage("hlsl/Cg");
        pkBlock->SetPlatform("D3D11/D3D10/DX9/Xenon/PS3");
        pkBlock->SetTarget("vs_1_1/ps_2_0/vs_3_0/ps_3_0/vs_4_0/ps_4_0/vs_5_0/ps_5_0");

        pkBlock->SetText("\n"
             "      InterpolatedOutput = lerp(InputOne, InputTwo, Amount).xyz;\n"
             "    ");


        pkFrag->AddCodeBlock(pkBlock);
    }

    pkLib->AddNode(pkFrag);
}
//--------------------------------------------------------------------------------------------------
EE_NOINLINE static void CreateFragment12(NiMaterialNodeLibrary* pkLib)
{
    NiMaterialFragmentNode* pkFrag = NiNew NiMaterialFragmentNode();

    pkFrag->SetType("Vertex/Pixel");
    pkFrag->SetName("LerpFloat3AndNormalize");
    pkFrag->SetDescription("\n"
        "      Linearly interpolates a float3 value and normalizes the result.\n"
        "    ");
    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetVariable("InputOne");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetVariable("InputTwo");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float");
        pkRes->SetVariable("Amount");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetVariable("InterpolatedOutput");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert a code block
    {
        NiCodeBlock* pkBlock = NiNew NiCodeBlock();

        pkBlock->SetLanguage("hlsl/Cg");
        pkBlock->SetPlatform("D3D11/D3D10/DX9/Xenon/PS3");
        pkBlock->SetTarget("vs_1_1/ps_2_0/vs_3_0/ps_3_0/vs_4_0/ps_4_0/vs_5_0/ps_5_0");

        pkBlock->SetText("\n"
             "      InterpolatedOutput = normalize(lerp(InputOne, InputTwo, Amount));\n"
             "    ");


        pkFrag->AddCodeBlock(pkBlock);
    }

    pkLib->AddNode(pkFrag);
}
//--------------------------------------------------------------------------------------------------
EE_NOINLINE static void CreateFragment13(NiMaterialNodeLibrary* pkLib)
{
    NiMaterialFragmentNode* pkFrag = NiNew NiMaterialFragmentNode();

    pkFrag->SetType("Vertex/Pixel");
    pkFrag->SetName("Calculate2DVertexMorph");
    pkFrag->SetDescription("\n"
        "        Calculates the amount in which to morph one vertex in the terrain to the\n"
        "        target vertex in the next terrain lod. This function uses the 2D distance\n"
        "        of the camera to the vertex to select the morphing value\n"
        "    ");
    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float");
        pkRes->SetVariable("LODThresholdDistance");
        pkRes->SetDefaultValue("(200.0)");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float");
        pkRes->SetVariable("LODMorphDistance");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetVariable("TerrainCameraPos");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetVariable("LocalPos");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float");
        pkRes->SetVariable("LODMorphValue");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert a code block
    {
        NiCodeBlock* pkBlock = NiNew NiCodeBlock();

        pkBlock->SetLanguage("hlsl/Cg");
        pkBlock->SetPlatform("D3D11/D3D10/DX9/Xenon/PS3");
        pkBlock->SetTarget("vs_1_1/ps_2_0/vs_3_0/ps_3_0/vs_4_0/ps_4_0/vs_5_0/ps_5_0");

        pkBlock->SetText("\n"
             "        float d = distance(LocalPos.xy, TerrainCameraPos.xy);\n"
             "        LODMorphValue = saturate((d - LODThresholdDistance) / LODMorphDistance);\n"
             "    ");


        pkFrag->AddCodeBlock(pkBlock);
    }

    pkLib->AddNode(pkFrag);
}
//--------------------------------------------------------------------------------------------------
EE_NOINLINE static void CreateFragment14(NiMaterialNodeLibrary* pkLib)
{
    NiMaterialFragmentNode* pkFrag = NiNew NiMaterialFragmentNode();

    pkFrag->SetType("Vertex/Pixel");
    pkFrag->SetName("Calculate25DVertexMorph");
    pkFrag->SetDescription("\n"
        "        Calculates the amount in which to morph one vertex in the terrain to the\n"
        "        target vertex in the next terrain lod. This function uses the distance\n"
        "        from the vert and the height of the camera above the terrain\n"
        "        to select the morphing value.\n"
        "    ");
    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float");
        pkRes->SetVariable("LODThresholdDistance");
        pkRes->SetDefaultValue("(200.0)");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float");
        pkRes->SetVariable("LODMorphDistance");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetVariable("TerrainCameraPos");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetVariable("LocalPos");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float");
        pkRes->SetVariable("LODMorphValue");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert a code block
    {
        NiCodeBlock* pkBlock = NiNew NiCodeBlock();

        pkBlock->SetLanguage("hlsl/Cg");
        pkBlock->SetPlatform("D3D11/D3D10/DX9/Xenon/PS3");
        pkBlock->SetTarget("vs_1_1/ps_2_0/vs_3_0/ps_3_0/vs_4_0/ps_4_0/vs_5_0/ps_5_0");

        pkBlock->SetText("\n"
             "        float d = distance(LocalPos.xy, TerrainCameraPos.xy);\n"
             "        d = max(d, TerrainCameraPos.z);\n"
             "        LODMorphValue = saturate((d - LODThresholdDistance) / LODMorphDistance);\n"
             "    ");


        pkFrag->AddCodeBlock(pkBlock);
    }

    pkLib->AddNode(pkFrag);
}
//--------------------------------------------------------------------------------------------------
EE_NOINLINE static void CreateFragment15(NiMaterialNodeLibrary* pkLib)
{
    NiMaterialFragmentNode* pkFrag = NiNew NiMaterialFragmentNode();

    pkFrag->SetType("Vertex/Pixel");
    pkFrag->SetName("CalculateNoVertexMorph");
    pkFrag->SetDescription("\n"
        "        Calculates the amount in which to morph one vertex in the terrain to the\n"
        "        target vertex in the next terrain lod. This function uses stitching\n"
        "        information given to the shader from the terrain system to detect\n"
        "        verts on the edge of a block that require stitching.\n"
        "    ");
    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float");
        pkRes->SetVariable("LODThresholdDistance");
        pkRes->SetDefaultValue("(200.0)");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float");
        pkRes->SetVariable("LODMorphDistance");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetVariable("TerrainCameraPos");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetVariable("LocalPos");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float4");
        pkRes->SetVariable("StitchingInfo");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float");
        pkRes->SetVariable("LODMorphValue");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert a code block
    {
        NiCodeBlock* pkBlock = NiNew NiCodeBlock();

        pkBlock->SetLanguage("hlsl/Cg");
        pkBlock->SetPlatform("D3D11/D3D10/DX9/Xenon/PS3");
        pkBlock->SetTarget("vs_1_1/ps_2_0/vs_3_0/ps_3_0/vs_4_0/ps_4_0/vs_5_0/ps_5_0");

        pkBlock->SetText("\n"
             "        // 0 = High Detail\n"
             "        // 1 = Low Detail\n"
             "        \n"
             "        float2 minXY = StitchingInfo.xy;\n"
             "        float2 maxXY = StitchingInfo.zw;\n"
             "        float2 normalised = (LocalPos.xy - minXY) / (maxXY - minXY);\n"
             "\n"
             "        LODMorphValue = 0;\n"
             "        if (normalised.x < 0 || normalised.y < 0 ||\n"
             "            normalised.x > 1 || normalised.y > 1)\n"
             "        {\n"
             "            LODMorphValue = 1;\n"
             "        }\n"
             "    ");


        pkFrag->AddCodeBlock(pkBlock);
    }

    pkLib->AddNode(pkFrag);
}
//--------------------------------------------------------------------------------------------------
EE_NOINLINE static void CreateFragment16(NiMaterialNodeLibrary* pkLib)
{
    NiMaterialFragmentNode* pkFrag = NiNew NiMaterialFragmentNode();

    pkFrag->SetType("Vertex/Pixel");
    pkFrag->SetName("Calculate3DVertexMorph");
    pkFrag->SetDescription("\n"
        "      Calculates the amount in which to morph one vertex in the terrain to the\n"
        "      target vertex in the next terrain lod. This function uses the 3D distance\n"
        "      to a vertex to select the morphing value.\n"
        "    ");
    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float");
        pkRes->SetVariable("LODThresholdDistance");
        pkRes->SetDefaultValue("(200.0)");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float");
        pkRes->SetVariable("LODMorphDistance");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetVariable("TerrainCameraPos");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetVariable("LocalPos");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float");
        pkRes->SetVariable("LODMorphValue");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert a code block
    {
        NiCodeBlock* pkBlock = NiNew NiCodeBlock();

        pkBlock->SetLanguage("hlsl/Cg");
        pkBlock->SetPlatform("D3D11/D3D10/DX9/Xenon/PS3");
        pkBlock->SetTarget("vs_1_1/ps_2_0/vs_3_0/ps_3_0/vs_4_0/ps_4_0/vs_5_0/ps_5_0");

        pkBlock->SetText("\n"
             "        float d = distance(LocalPos.xyz, TerrainCameraPos.xyz);\n"
             "        LODMorphValue = saturate((d - LODThresholdDistance) / LODMorphDistance);\n"
             "    ");


        pkFrag->AddCodeBlock(pkBlock);
    }

    pkLib->AddNode(pkFrag);
}
//--------------------------------------------------------------------------------------------------
EE_NOINLINE static void CreateFragment17(NiMaterialNodeLibrary* pkLib)
{
    NiMaterialFragmentNode* pkFrag = NiNew NiMaterialFragmentNode();

    pkFrag->SetType("Vertex/Pixel");
    pkFrag->SetName("CalculateFog");
    pkFrag->SetDescription("\n"
        "      This fragment is responsible for handling fogging calculations.\n"
        "      FogType can be one of 4 values:\n"
        "\n"
        "      NONE   - 0\n"
        "      EXP    - 1\n"
        "      EXP2   - 2\n"
        "      LINEAR - 3\n"
        "    ");
    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("int");
        pkRes->SetSemantic("FogType");
        pkRes->SetVariable("FogType");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float");
        pkRes->SetSemantic("FogDensity");
        pkRes->SetVariable("FogDensity");
        pkRes->SetDefaultValue("(1.0)");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetSemantic("Position");
        pkRes->SetLabel("View");
        pkRes->SetVariable("ViewPosition");
        pkRes->SetDefaultValue("(0.0, 0.0, 0.0)");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("bool");
        pkRes->SetSemantic("FogRange");
        pkRes->SetVariable("FogRange");
        pkRes->SetDefaultValue("(false)");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float2");
        pkRes->SetSemantic("FogStartEnd");
        pkRes->SetVariable("FogStartEnd");
        pkRes->SetDefaultValue("(0.0, 1.0)");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float");
        pkRes->SetSemantic("Fog");
        pkRes->SetVariable("FogOut");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert a code block
    {
        NiCodeBlock* pkBlock = NiNew NiCodeBlock();

        pkBlock->SetLanguage("hlsl/Cg");
        pkBlock->SetPlatform("D3D11/D3D10/DX9/Xenon/PS3");
        pkBlock->SetTarget("vs_1_1/ps_2_0/vs_4_0/ps_4_0/vs_5_0/ps_5_0");

        pkBlock->SetText("\n"
             "      float d;\n"
             "      if (FogRange)\n"
             "      {\n"
             "        d = length(ViewPosition);\n"
             "      }\n"
             "      else\n"
             "      {\n"
             "        d = ViewPosition.z;\n"
             "      }\n"
             "\n"
             "      if (FogType == 0) // NONE\n"
             "      {\n"
             "        FogOut = 1.0;\n"
             "      }\n"
             "      else if (FogType == 1) // EXP\n"
             "      {\n"
             "        FogOut = 1.0 / exp( d * FogDensity);\n"
             "      }\n"
             "      else if (FogType == 2) // EXP2\n"
             "      {\n"
             "        FogOut = 1.0 / exp( pow( d * FogDensity, 2));\n"
             "      }\n"
             "      else if (FogType == 3) // LINEAR\n"
             "      {\n"
             "        FogOut = saturate((FogStartEnd.y - d) /\n"
             "          (FogStartEnd.y - FogStartEnd.x));\n"
             "      }\n"
             "    ");


        pkFrag->AddCodeBlock(pkBlock);
    }

    pkLib->AddNode(pkFrag);
}
//--------------------------------------------------------------------------------------------------
EE_NOINLINE static void CreateFragment18(NiMaterialNodeLibrary* pkLib)
{
    NiMaterialFragmentNode* pkFrag = NiNew NiMaterialFragmentNode();

    pkFrag->SetType("Pixel/Vertex");
    pkFrag->SetName("ApplyFog");
    pkFrag->SetDescription("\n"
        "      This fragment is responsible for applying the fog amount.\n"
        "    ");
    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetSemantic("Color");
        pkRes->SetVariable("UnfoggedColor");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetSemantic("Color");
        pkRes->SetVariable("FogColor");
        pkRes->SetDefaultValue("(0.0, 0.0, 0.0)");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float");
        pkRes->SetSemantic("Fog");
        pkRes->SetVariable("FogAmount");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetSemantic("Color");
        pkRes->SetVariable("FoggedColor");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert a code block
    {
        NiCodeBlock* pkBlock = NiNew NiCodeBlock();

        pkBlock->SetLanguage("hlsl/Cg");
        pkBlock->SetPlatform("D3D11/D3D10/DX9/Xenon/PS3");
        pkBlock->SetTarget("vs_1_1/ps_2_0/vs_4_0/ps_4_0/vs_5_0/ps_5_0");

        pkBlock->SetText("\n"
             "      FoggedColor = lerp(FogColor, UnfoggedColor.rgb, FogAmount);\n"
             "    ");


        pkFrag->AddCodeBlock(pkBlock);
    }

    pkLib->AddNode(pkFrag);
}
//--------------------------------------------------------------------------------------------------
EE_NOINLINE static void CreateFragment19(NiMaterialNodeLibrary* pkLib)
{
    NiMaterialFragmentNode* pkFrag = NiNew NiMaterialFragmentNode();

    pkFrag->SetType("Vertex/Pixel");
    pkFrag->SetName("CalculateViewVector");
    pkFrag->SetDescription("\n"
        "      This fragment is responsible for calculating the camera view vector.\n"
        "    ");
    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float4");
        pkRes->SetSemantic("Position");
        pkRes->SetLabel("World");
        pkRes->SetVariable("WorldPos");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetSemantic("CameraPosition");
        pkRes->SetLabel("World");
        pkRes->SetVariable("CameraPos");
        pkRes->SetDefaultValue("(0.0, 0.0, 0.0)");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetSemantic("ViewVector");
        pkRes->SetLabel("World");
        pkRes->SetVariable("WorldViewVector");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert a code block
    {
        NiCodeBlock* pkBlock = NiNew NiCodeBlock();

        pkBlock->SetLanguage("hlsl/Cg");
        pkBlock->SetPlatform("D3D11/D3D10/DX9/Xenon/PS3");
        pkBlock->SetTarget("vs_1_1/ps_2_0/vs_3_0/ps_3_0/vs_4_0/ps_4_0/vs_5_0/ps_5_0");

        pkBlock->SetText("\n"
             "      WorldViewVector = CameraPos - WorldPos;\n"
             "    ");


        pkFrag->AddCodeBlock(pkBlock);
    }

    pkLib->AddNode(pkFrag);
}
//--------------------------------------------------------------------------------------------------
EE_NOINLINE static void CreateFragment20(NiMaterialNodeLibrary* pkLib)
{
    NiMaterialFragmentNode* pkFrag = NiNew NiMaterialFragmentNode();

    pkFrag->SetType("Vertex");
    pkFrag->SetName("CalculateBinormal");
    pkFrag->SetDescription("\n"
        "      Takes the cross product of a vertex normal and a tangent to create a binormal.  "
        "    \n"
        "      Assumes the vertex normal and tangent are normalized.\n"
        "    ");
    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetSemantic("Normal");
        pkRes->SetLabel("World");
        pkRes->SetVariable("WorldNormal");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetSemantic("Tangent");
        pkRes->SetLabel("World");
        pkRes->SetVariable("WorldTangent");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetSemantic("Binormal");
        pkRes->SetLabel("World");
        pkRes->SetVariable("WorldBinormal");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert a code block
    {
        NiCodeBlock* pkBlock = NiNew NiCodeBlock();

        pkBlock->SetLanguage("hlsl/Cg");
        pkBlock->SetPlatform("D3D11/D3D10/DX9/Xenon/PS3");
        pkBlock->SetTarget("vs_1_1/ps_2_0/vs_3_0/ps_3_0/vs_4_0/ps_4_0/vs_5_0/ps_5_0");

        pkBlock->SetText("\n"
             "        WorldBinormal = cross(WorldTangent, WorldNormal);\n"
             "    ");


        pkFrag->AddCodeBlock(pkBlock);
    }

    pkLib->AddNode(pkFrag);
}
//--------------------------------------------------------------------------------------------------
EE_NOINLINE static void CreateFragment21(NiMaterialNodeLibrary* pkLib)
{
    NiMaterialFragmentNode* pkFrag = NiNew NiMaterialFragmentNode();

    pkFrag->SetType("Pixel");
    pkFrag->SetName("CalculateNormalFromColor");
    pkFrag->SetDescription("\n"
        "      This fragment is responsible for sampling a normal map to generate a\n"
        "      world-space normal. Note, compressed normal maps are not supported.\n"
        "    ");
    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetSemantic("Color");
        pkRes->SetVariable("NormalMap");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetSemantic("Normal");
        pkRes->SetLabel("World");
        pkRes->SetVariable("WorldNormal");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetSemantic("Binormal");
        pkRes->SetLabel("World");
        pkRes->SetVariable("WorldBinormal");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetSemantic("Tangent");
        pkRes->SetLabel("World");
        pkRes->SetVariable("WorldTangent");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetSemantic("Normal");
        pkRes->SetLabel("World");
        pkRes->SetVariable("WorldNormalOut");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert a code block
    {
        NiCodeBlock* pkBlock = NiNew NiCodeBlock();

        pkBlock->SetLanguage("hlsl/Cg");
        pkBlock->SetPlatform("D3D11/D3D10/DX9/Xenon/PS3");
        pkBlock->SetTarget("ps_2_0/ps_3_0/ps_4_0/ps_5_0");

        pkBlock->SetText("\n"
             "      NormalMap = NormalMap * 2.0 - 1.0;\n"
             "\n"
             "      float3x3 xForm = float3x3(WorldTangent, WorldBinormal, WorldNormal);\n"
             "      xForm = transpose(xForm);\n"
             "      WorldNormalOut = mul(xForm, NormalMap.rgb);\n"
             "    ");


        pkFrag->AddCodeBlock(pkBlock);
    }

    pkLib->AddNode(pkFrag);
}
//--------------------------------------------------------------------------------------------------
EE_NOINLINE static void CreateFragment22(NiMaterialNodeLibrary* pkLib)
{
    NiMaterialFragmentNode* pkFrag = NiNew NiMaterialFragmentNode();

    pkFrag->SetType("Pixel");
    pkFrag->SetName("ApplyParallaxScaleAndBias");
    pkFrag->SetDescription("\n"
        "      This fragment is responsible for converting the color value retreived\n"
        "      from a parallax map into a height value. \n"
        "    ");
    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float");
        pkRes->SetSemantic("Color");
        pkRes->SetVariable("Color");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float");
        pkRes->SetSemantic("ParallaxOffsetScale");
        pkRes->SetVariable("OffsetScale");
        pkRes->SetDefaultValue("(0.05)");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float");
        pkRes->SetVariable("Height");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert a code block
    {
        NiCodeBlock* pkBlock = NiNew NiCodeBlock();

        pkBlock->SetLanguage("hlsl/Cg");
        pkBlock->SetPlatform("D3D11/D3D10/DX9/Xenon/PS3");
        pkBlock->SetTarget("ps_2_0/ps_3_0/ps_4_0/ps_5_0");

        pkBlock->SetText("\n"
             "      // Calculate scale/offset of the color to produce the height.\n"
             "      Height = (Color - 0.5f) * OffsetScale;\n"
             "      \n"
             "    ");


        pkFrag->AddCodeBlock(pkBlock);
    }

    pkLib->AddNode(pkFrag);
}
//--------------------------------------------------------------------------------------------------
EE_NOINLINE static void CreateFragment23(NiMaterialNodeLibrary* pkLib)
{
    NiMaterialFragmentNode* pkFrag = NiNew NiMaterialFragmentNode();

    pkFrag->SetType("Pixel");
    pkFrag->SetName("CalculateParallaxOffset");
    pkFrag->SetDescription("\n"
        "      This fragment is responsible for calculating the UV offset to apply\n"
        "      as a result of a parallax map height value.\n"
        "    ");
    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float4");
        pkRes->SetSemantic("TexCoord");
        pkRes->SetVariable("TexCoord");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float");
        pkRes->SetVariable("Height");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetSemantic("ViewVector");
        pkRes->SetVariable("TangentSpaceEyeVec");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float4");
        pkRes->SetSemantic("TexCoord");
        pkRes->SetVariable("ParallaxOffsetUV");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert a code block
    {
        NiCodeBlock* pkBlock = NiNew NiCodeBlock();

        pkBlock->SetLanguage("hlsl/Cg");
        pkBlock->SetPlatform("D3D11/D3D10/DX9/Xenon/PS3");
        pkBlock->SetTarget("ps_2_0/ps_3_0/ps_4_0/ps_5_0");

        pkBlock->SetText("\n"
             "      // Get texcoord.\n"
             "      ParallaxOffsetUV = float4(TexCoord.xy, \n"
             "        TexCoord.zw + Height * TangentSpaceEyeVec.xy);\n"
             "    ");


        pkFrag->AddCodeBlock(pkBlock);
    }

    pkLib->AddNode(pkFrag);
}
//--------------------------------------------------------------------------------------------------
EE_NOINLINE static void CreateFragment24(NiMaterialNodeLibrary* pkLib)
{
    NiMaterialFragmentNode* pkFrag = NiNew NiMaterialFragmentNode();

    pkFrag->SetType("Vertex/Pixel");
    pkFrag->SetName("WorldToTangent");
    pkFrag->SetDescription("\n"
        "      This fragment is responsible for transforming a vector from world space\n"
        "      to tangent space.\n"
        "    ");
    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetLabel("World");
        pkRes->SetVariable("VectorIn");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetSemantic("Normal");
        pkRes->SetLabel("World");
        pkRes->SetVariable("WorldNormal");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetSemantic("Binormal");
        pkRes->SetLabel("World");
        pkRes->SetVariable("WorldBinormal");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetSemantic("Tangent");
        pkRes->SetLabel("World");
        pkRes->SetVariable("WorldTangent");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetVariable("VectorOut");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert a code block
    {
        NiCodeBlock* pkBlock = NiNew NiCodeBlock();

        pkBlock->SetLanguage("hlsl/Cg");
        pkBlock->SetPlatform("D3D11/D3D10/DX9/Xenon/PS3");
        pkBlock->SetTarget("vs_1_1/ps_2_0/ps_3_0/vs_4_0/ps_4_0/vs_5_0/ps_5_0");

        pkBlock->SetText("\n"
             "      float3x3 xForm = float3x3(WorldTangent, WorldBinormal, WorldNormal);\n"
             "      VectorOut = mul(xForm, VectorIn);\n"
             "    ");


        pkFrag->AddCodeBlock(pkBlock);
    }

    pkLib->AddNode(pkFrag);
}
//--------------------------------------------------------------------------------------------------
EE_NOINLINE static void CreateFragment25(NiMaterialNodeLibrary* pkLib)
{
    NiMaterialFragmentNode* pkFrag = NiNew NiMaterialFragmentNode();

    pkFrag->SetType("Pixel");
    pkFrag->SetName("GeneratePerPixelTangentFrame");
    pkFrag->SetDescription("\n"
        "      This fragment calculates a tangent frame per pixel using the derivative of \n"
        "      the current pixel position and pixel uv coodinate. Assumes the world normal is \n"
        "      normalized.\n"
        "    ");
    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetVariable("WorldPosition");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetSemantic("Normal");
        pkRes->SetLabel("World");
        pkRes->SetVariable("WorldNormal");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float4");
        pkRes->SetSemantic("TexCoord");
        pkRes->SetVariable("UV");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetSemantic("Tangent");
        pkRes->SetLabel("World");
        pkRes->SetVariable("WorldTangentOut");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetSemantic("Binormal");
        pkRes->SetLabel("World");
        pkRes->SetVariable("WorldBinormalOut");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert a code block
    {
        NiCodeBlock* pkBlock = NiNew NiCodeBlock();

        pkBlock->SetLanguage("hlsl/Cg");
        pkBlock->SetPlatform("D3D11/D3D10/DX9/Xenon/PS3");
        pkBlock->SetTarget("ps_2_0/ps_3_0/ps_4_0/ps_5_0");

        pkBlock->SetText("\n"
             "      // get edge vectors of the pixel triangle\n"
             "      float3 dp1 = ddx(WorldPosition);\n"
             "      float3 dp2 = ddy(WorldPosition);\n"
             "      float2 duv1 = ddx(UV.xy);\n"
             "      float2 duv2 = ddy(UV.xy);\n"
             "\n"
             "      // Assume M is orthogonal.\n"
             "      float2x3 M = float2x3(dp1, dp2);\n"
             "      float3 T = mul(float2(duv1.x, duv2.x), M);\n"
             "      float3 B = mul(float2(duv1.y, duv2.y), M);\n"
             "\n"
             "      WorldTangentOut = normalize(T);\n"
             "      WorldBinormalOut = normalize(B);\n"
             "    ");


        pkFrag->AddCodeBlock(pkBlock);
    }

    pkLib->AddNode(pkFrag);
}
//--------------------------------------------------------------------------------------------------
EE_NOINLINE static void CreateFragment26(NiMaterialNodeLibrary* pkLib)
{
    NiMaterialFragmentNode* pkFrag = NiNew NiMaterialFragmentNode();

    pkFrag->SetType("Pixel");
    pkFrag->SetName("SampleBlendMap");
    pkFrag->SetDescription("\n"
        "      This fragment samples an RGBA texture and returns each component where \n"
        "      each component contains a mask for a given texture layer.\n"
        "    ");
    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float4");
        pkRes->SetSemantic("TexCoord");
        pkRes->SetVariable("MapUV");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("sampler2D");
        pkRes->SetSemantic("Texture");
        pkRes->SetVariable("Sampler");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float");
        pkRes->SetVariable("OutputRed");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float");
        pkRes->SetVariable("OutputGreen");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float");
        pkRes->SetVariable("OutputBlue");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float");
        pkRes->SetVariable("OutputAlpha");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float");
        pkRes->SetVariable("TotalMask");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert a code block
    {
        NiCodeBlock* pkBlock = NiNew NiCodeBlock();

        pkBlock->SetLanguage("hlsl/Cg");
        pkBlock->SetPlatform("D3D11/D3D10/DX9/Xenon/PS3");
        pkBlock->SetTarget("vs_1_1/ps_2_0/vs_3_0/ps_3_0/vs_4_0/ps_4_0/vs_5_0/ps_5_0");

        pkBlock->SetText("\n"
             "      float4 color = tex2D(Sampler, MapUV.zw);\n"
             "      OutputRed = color.r;\n"
             "      OutputGreen = color.g;\n"
             "      OutputBlue = color.b;\n"
             "      OutputAlpha = color.a;\n"
             "      TotalMask = dot(color, float4(1,1,1,1));\n"
             "    ");


        pkFrag->AddCodeBlock(pkBlock);
    }

    pkLib->AddNode(pkFrag);
}
//--------------------------------------------------------------------------------------------------
EE_NOINLINE static void CreateFragment27(NiMaterialNodeLibrary* pkLib)
{
    NiMaterialFragmentNode* pkFrag = NiNew NiMaterialFragmentNode();

    pkFrag->SetType("Pixel");
    pkFrag->SetName("SampleLowDetailDiffuseMap");
    pkFrag->SetDescription("\n"
        "          This fragment samples an RGB texture and returns the resulting color.\n"
        "      ");
    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float2");
        pkRes->SetSemantic("TexCoord");
        pkRes->SetVariable("UV");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("sampler2D");
        pkRes->SetSemantic("Texture");
        pkRes->SetVariable("Sampler");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetVariable("Diffuse");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float");
        pkRes->SetVariable("Glossiness");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert a code block
    {
        NiCodeBlock* pkBlock = NiNew NiCodeBlock();

        pkBlock->SetLanguage("hlsl/Cg");
        pkBlock->SetPlatform("D3D11/D3D10/DX9/Xenon/PS3");
        pkBlock->SetTarget("vs_1_1/ps_2_0/vs_3_0/ps_3_0/vs_4_0/ps_4_0/vs_5_0/ps_5_0");

        pkBlock->SetText("\n"
             "        float4 sample = tex2D(Sampler, UV);\n"
             "        Diffuse.rgb = sample.rgb;\n"
             "        Glossiness = sample.a;\n"
             "      ");


        pkFrag->AddCodeBlock(pkBlock);
    }

    pkLib->AddNode(pkFrag);
}
//--------------------------------------------------------------------------------------------------
EE_NOINLINE static void CreateFragment28(NiMaterialNodeLibrary* pkLib)
{
    NiMaterialFragmentNode* pkFrag = NiNew NiMaterialFragmentNode();

    pkFrag->SetType("Pixel");
    pkFrag->SetName("SampleLowDetailNormalMap");
    pkFrag->SetDescription("\n"
        "          This fragment samples an RGB texture and returns the resulting color.\n"
        "      ");
    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float2");
        pkRes->SetSemantic("TexCoord");
        pkRes->SetVariable("UV");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("sampler2D");
        pkRes->SetSemantic("Texture");
        pkRes->SetVariable("Sampler");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float4x4");
        pkRes->SetSemantic("WorldMatrix");
        pkRes->SetVariable("WorldMatrix");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetVariable("Output");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert a code block
    {
        NiCodeBlock* pkBlock = NiNew NiCodeBlock();

        pkBlock->SetLanguage("hlsl/Cg");
        pkBlock->SetPlatform("D3D11/D3D10/DX9/Xenon/PS3");
        pkBlock->SetTarget("vs_1_1/ps_2_0/vs_3_0/ps_3_0/vs_4_0/ps_4_0/vs_5_0/ps_5_0");

        pkBlock->SetText("\n"
             "          Output.rgb = tex2D(Sampler, UV);\n"
             "          Output.rg = Output.rg * 2.0 - 1.0;\n"
             "\n"
             "          Output = mul(Output, (float3x3)WorldMatrix);\n"
             "          Output = normalize(Output);\n"
             "      ");


        pkFrag->AddCodeBlock(pkBlock);
    }

    pkLib->AddNode(pkFrag);
}
//--------------------------------------------------------------------------------------------------
EE_NOINLINE static void CreateFragment29(NiMaterialNodeLibrary* pkLib)
{
    NiMaterialFragmentNode* pkFrag = NiNew NiMaterialFragmentNode();

    pkFrag->SetType("Pixel");
    pkFrag->SetName("ExtractTextureSize");
    pkFrag->SetDescription("\n"
        "          This fragment extracts a texture size from the texture size array\n"
        "      ");
    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("int");
        pkRes->SetVariable("Entry");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float2");
        pkRes->SetVariable("TextureSizes");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float");
        pkRes->SetVariable("Size");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert a code block
    {
        NiCodeBlock* pkBlock = NiNew NiCodeBlock();

        pkBlock->SetLanguage("hlsl/Cg");
        pkBlock->SetPlatform("D3D11/D3D10/DX9/Xenon/PS3");
        pkBlock->SetTarget("vs_1_1/ps_2_0/vs_3_0/ps_3_0/vs_4_0/ps_4_0/vs_5_0/ps_5_0");

        pkBlock->SetText("\n"
             "          if (Entry == 0)\n"
             "          {\n"
             "            Size = TextureSizes.x;\n"
             "          }\n"
             "          else if(Entry == 1)\n"
             "          {\n"
             "            Size = TextureSizes.y;\n"
             "          }\n"
             "          else\n"
             "          {\n"
             "            Size = 1;\n"
             "          }\n"
             "      ");


        pkFrag->AddCodeBlock(pkBlock);
    }

    pkLib->AddNode(pkFrag);
}
//--------------------------------------------------------------------------------------------------
EE_NOINLINE static void CreateFragment30(NiMaterialNodeLibrary* pkLib)
{
    NiMaterialFragmentNode* pkFrag = NiNew NiMaterialFragmentNode();

    pkFrag->SetType("Pixel");
    pkFrag->SetName("ApplyTextureMorphing");
    pkFrag->SetDescription("\n"
        "          Calculates the amount in which to morph one vertex in the terrain to the\n"
        "          target vertex in the next terrain lod. This function uses the 2D distance\n"
        "          of the camera to the vertex to select the morphing value\n"
        "      ");
    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetVariable("BaseMapColor");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetVariable("SurfaceColor");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetVariable("NormalMap");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetVariable("WorldNormal");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float");
        pkRes->SetVariable("MorphValue");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetVariable("DiffuseColor");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetVariable("MorphedNormal");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert a code block
    {
        NiCodeBlock* pkBlock = NiNew NiCodeBlock();

        pkBlock->SetLanguage("hlsl/Cg");
        pkBlock->SetPlatform("D3D11/D3D10/DX9/Xenon/PS3");
        pkBlock->SetTarget("vs_1_1/ps_2_0/vs_3_0/ps_3_0/vs_4_0/ps_4_0/vs_5_0/ps_5_0");

        pkBlock->SetText("\n"
             "          DiffuseColor = lerp(SurfaceColor.rgb, BaseMapColor.rgb, MorphValue);\n"
             "          MorphedNormal = lerp(WorldNormal.xyz, NormalMap.xyz, MorphValue);\n"
             "      ");


        pkFrag->AddCodeBlock(pkBlock);
    }

    pkLib->AddNode(pkFrag);
}
//--------------------------------------------------------------------------------------------------
EE_NOINLINE static void CreateFragment31(NiMaterialNodeLibrary* pkLib)
{
    NiMaterialFragmentNode* pkFrag = NiNew NiMaterialFragmentNode();

    pkFrag->SetType("Pixel");
    pkFrag->SetName("SampleSingleChannel");
    pkFrag->SetDescription("\n"
        "      This sample samples a channel based on the specified index (0 - 3).\n"
        "    ");
    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float4");
        pkRes->SetSemantic("TexCoord");
        pkRes->SetVariable("MapUV");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("sampler2D");
        pkRes->SetSemantic("Texture");
        pkRes->SetVariable("Sampler");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("int");
        pkRes->SetVariable("ChannelIndex");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float");
        pkRes->SetVariable("Output");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert a code block
    {
        NiCodeBlock* pkBlock = NiNew NiCodeBlock();

        pkBlock->SetLanguage("hlsl/Cg");
        pkBlock->SetPlatform("D3D11/D3D10/DX9/Xenon/PS3");
        pkBlock->SetTarget("vs_1_1/ps_2_0/vs_3_0/ps_3_0/vs_4_0/ps_4_0/vs_5_0/ps_5_0");

        pkBlock->SetText("\n"
             "        if (ChannelIndex == 0)\n"
             "        Output = tex2D(Sampler, MapUV.zw).r;\n"
             "        else if (ChannelIndex == 1)\n"
             "        Output =  tex2D(Sampler, MapUV.zw).g;\n"
             "        else if (ChannelIndex == 2)\n"
             "        Output =  tex2D(Sampler, MapUV.zw).b;\n"
             "        else if (ChannelIndex == 3)\n"
             "        Output =  tex2D(Sampler, MapUV.zw).a;\n"
             "\n"
             "    ");


        pkFrag->AddCodeBlock(pkBlock);
    }

    pkLib->AddNode(pkFrag);
}
//--------------------------------------------------------------------------------------------------
EE_NOINLINE static void CreateFragment32(NiMaterialNodeLibrary* pkLib)
{
    NiMaterialFragmentNode* pkFrag = NiNew NiMaterialFragmentNode();

    pkFrag->SetType("Pixel");
    pkFrag->SetName("ComputeTotalMaskValue");
    pkFrag->SetDescription("\n"
        "      This fragment combines mask values from the \"global\" mask with per layer \n"
        "      mask to produce a final composited mask.\n"
        "    ");
    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float");
        pkRes->SetVariable("lMask0");
        pkRes->SetDefaultValue("(1.0f)");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float");
        pkRes->SetVariable("lMask1");
        pkRes->SetDefaultValue("(1.0f)");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float");
        pkRes->SetVariable("lMask2");
        pkRes->SetDefaultValue("(1.0f)");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float");
        pkRes->SetVariable("lMask3");
        pkRes->SetDefaultValue("(1.0f)");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float");
        pkRes->SetVariable("gMask0");
        pkRes->SetDefaultValue("(0.0f)");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float");
        pkRes->SetVariable("gMask1");
        pkRes->SetDefaultValue("(0.0f)");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float");
        pkRes->SetVariable("gMask2");
        pkRes->SetDefaultValue("(0.0f)");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float");
        pkRes->SetVariable("gMask3");
        pkRes->SetDefaultValue("(0.0f)");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float4");
        pkRes->SetVariable("DistStrength");
        pkRes->SetDefaultValue("(1.0f, 1.0f, 1.0f, 1.0f)");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float");
        pkRes->SetVariable("gMask0_Out");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float");
        pkRes->SetVariable("gMask1_Out");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float");
        pkRes->SetVariable("gMask2_Out");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float");
        pkRes->SetVariable("gMask3_Out");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert a code block
    {
        NiCodeBlock* pkBlock = NiNew NiCodeBlock();

        pkBlock->SetLanguage("hlsl/Cg");
        pkBlock->SetPlatform("D3D11/D3D10/DX9/Xenon/PS3");
        pkBlock->SetTarget("vs_1_1/ps_2_0/vs_3_0/ps_3_0/vs_4_0/ps_4_0/vs_5_0/ps_5_0");

        pkBlock->SetText("\n"
             "                    \n"
             "          float4 gMaskTotal = float4(gMask0, gMask1, gMask2, gMask3);\n"
             "          float4 lMaskTotal = float4(lMask0, lMask1, lMask2, lMask3);\n"
             "          \n"
             "          float4 result = gMaskTotal * lMaskTotal * DistStrength;\n"
             "          \n"
             "          float fSum = dot(result, float4(1,1,1,1));\n"
             "          if (fSum != 0)\n"
             "          {\n"
             "            // Enforce the rule that sum(channels) == 1         \n"
             "            result /= fSum;\n"
             "            // Enforce the rule that these changes don't affect the overall amoun"
             "t of surface applied.\n"
             "            result *= dot(gMaskTotal, float4(1,1,1,1));         \n"
             "          }\n"
             "          \n"
             "          gMask0_Out = result.x;\n"
             "          gMask1_Out = result.y;\n"
             "          gMask2_Out = result.z;\n"
             "          gMask3_Out = result.w;\n"
             "    ");


        pkFrag->AddCodeBlock(pkBlock);
    }

    pkLib->AddNode(pkFrag);
}
//--------------------------------------------------------------------------------------------------
EE_NOINLINE static void CreateFragment33(NiMaterialNodeLibrary* pkLib)
{
    NiMaterialFragmentNode* pkFrag = NiNew NiMaterialFragmentNode();

    pkFrag->SetType("Pixel");
    pkFrag->SetName("SampleLayerTextureRGB");
    pkFrag->SetDescription("\n"
        "      This fragment samples an RGB texture for a given terrain layer and\n"
        "      returns the float3 value.\n"
        "    ");
    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float4");
        pkRes->SetSemantic("TexCoord");
        pkRes->SetVariable("MapUV");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("sampler2D");
        pkRes->SetSemantic("Texture");
        pkRes->SetVariable("Sampler");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetVariable("OutputColor");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert a code block
    {
        NiCodeBlock* pkBlock = NiNew NiCodeBlock();

        pkBlock->SetLanguage("hlsl/Cg");
        pkBlock->SetPlatform("D3D11/D3D10/DX9/Xenon/PS3");
        pkBlock->SetTarget("vs_1_1/ps_2_0/vs_3_0/ps_3_0/vs_4_0/ps_4_0/vs_5_0/ps_5_0");

        pkBlock->SetText("\n"
             "        OutputColor = tex2D(Sampler, MapUV.zw).rgb;\n"
             "    ");


        pkFrag->AddCodeBlock(pkBlock);
    }

    pkLib->AddNode(pkFrag);
}
//--------------------------------------------------------------------------------------------------
EE_NOINLINE static void CreateFragment34(NiMaterialNodeLibrary* pkLib)
{
    NiMaterialFragmentNode* pkFrag = NiNew NiMaterialFragmentNode();

    pkFrag->SetType("Pixel");
    pkFrag->SetName("SampleLayerTextureRGBA");
    pkFrag->SetDescription("\n"
        "        This fragment samples an RGBA texture for a given terrain layer and\n"
        "        returns the float3 value along with a single floating point value contained\n"
        "        in the alpha channel.\n"
        "    ");
    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float4");
        pkRes->SetSemantic("TexCoord");
        pkRes->SetVariable("MapUV");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("sampler2D");
        pkRes->SetSemantic("Texture");
        pkRes->SetVariable("Sampler");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetVariable("OutputColor");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float");
        pkRes->SetVariable("OutputAlpha");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert a code block
    {
        NiCodeBlock* pkBlock = NiNew NiCodeBlock();

        pkBlock->SetLanguage("hlsl/Cg");
        pkBlock->SetPlatform("D3D11/D3D10/DX9/Xenon/PS3");
        pkBlock->SetTarget("vs_1_1/ps_2_0/vs_3_0/ps_3_0/vs_4_0/ps_4_0/vs_5_0/ps_5_0");

        pkBlock->SetText("\n"
             "        float4 color = tex2D(Sampler, MapUV.zw);\n"
             "        OutputColor = color.rgb;\n"
             "        OutputAlpha = color.a;\n"
             "    ");


        pkFrag->AddCodeBlock(pkBlock);
    }

    pkLib->AddNode(pkFrag);
}
//--------------------------------------------------------------------------------------------------
EE_NOINLINE static void CreateFragment35(NiMaterialNodeLibrary* pkLib)
{
    NiMaterialFragmentNode* pkFrag = NiNew NiMaterialFragmentNode();

    pkFrag->SetType("Pixel");
    pkFrag->SetName("SampleLayerTextureAlpha");
    pkFrag->SetDescription("\n"
        "      This fragment samples the alpha channel of the specified texture.\n"
        "    ");
    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float4");
        pkRes->SetSemantic("TexCoord");
        pkRes->SetVariable("MapUV");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("sampler2D");
        pkRes->SetSemantic("Texture");
        pkRes->SetVariable("Sampler");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float");
        pkRes->SetVariable("OutputAlpha");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert a code block
    {
        NiCodeBlock* pkBlock = NiNew NiCodeBlock();

        pkBlock->SetLanguage("hlsl/Cg");
        pkBlock->SetPlatform("D3D11/D3D10/DX9/Xenon/PS3");
        pkBlock->SetTarget("vs_1_1/ps_2_0/vs_3_0/ps_3_0/vs_4_0/ps_4_0/vs_5_0/ps_5_0");

        pkBlock->SetText("\n"
             "        OutputAlpha = tex2D(Sampler, MapUV.zw).a;\n"
             "    ");


        pkFrag->AddCodeBlock(pkBlock);
    }

    pkLib->AddNode(pkFrag);
}
//--------------------------------------------------------------------------------------------------
EE_NOINLINE static void CreateFragment36(NiMaterialNodeLibrary* pkLib)
{
    NiMaterialFragmentNode* pkFrag = NiNew NiMaterialFragmentNode();

    pkFrag->SetType("Pixel");
    pkFrag->SetName("SampleBaseMapWithDetailMap");
    pkFrag->SetDescription("\n"
        "      Samples an RGBA texture where the RGB channels contain the base map \n"
        "      information and the alpha channel contains the detail map data. The detail\n"
        "      map is sampled at a much higher frequency that the base map.\n"
        "    ");
    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float4");
        pkRes->SetSemantic("TexCoord");
        pkRes->SetVariable("MapUV");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float2");
        pkRes->SetSemantic("TexCoord");
        pkRes->SetVariable("DetailMapUV");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("sampler2D");
        pkRes->SetSemantic("Texture");
        pkRes->SetVariable("Sampler");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float4");
        pkRes->SetVariable("OutputColor");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert a code block
    {
        NiCodeBlock* pkBlock = NiNew NiCodeBlock();

        pkBlock->SetLanguage("hlsl/Cg");
        pkBlock->SetPlatform("D3D11/D3D10/DX9/Xenon/PS3");
        pkBlock->SetTarget("vs_1_1/ps_2_0/vs_3_0/ps_3_0/vs_4_0/ps_4_0/vs_5_0/ps_5_0");

        pkBlock->SetText("\n"
             "        float3 base = tex2D(Sampler, MapUV.zw);\n"
             "        float detail = tex2D(Sampler, DetailMapUV).a;\n"
             "        OutputColor = float4(base, detail);\n"
             "    ");


        pkFrag->AddCodeBlock(pkBlock);
    }

    pkLib->AddNode(pkFrag);
}
//--------------------------------------------------------------------------------------------------
EE_NOINLINE static void CreateFragment37(NiMaterialNodeLibrary* pkLib)
{
    NiMaterialFragmentNode* pkFrag = NiNew NiMaterialFragmentNode();

    pkFrag->SetType("Pixel");
    pkFrag->SetName("ScaleUVForDetailMap");
    pkFrag->SetDescription("\n"
        "      Computes UV coordinates by multiplying the xy components of the input float4\n"
        "      and a scale factor.\n"
        "    ");
    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float4");
        pkRes->SetSemantic("TexCoord");
        pkRes->SetVariable("MapUV");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float");
        pkRes->SetVariable("ScaleFactor");
        pkRes->SetDefaultValue("(8.0)");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float2");
        pkRes->SetVariable("OutputUV");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert a code block
    {
        NiCodeBlock* pkBlock = NiNew NiCodeBlock();

        pkBlock->SetLanguage("hlsl/Cg");
        pkBlock->SetPlatform("D3D11/D3D10/DX9/Xenon/PS3");
        pkBlock->SetTarget("vs_1_1/ps_2_0/vs_3_0/ps_3_0/vs_4_0/ps_4_0/vs_5_0/ps_5_0");

        pkBlock->SetText("\n"
             "        OutputUV = MapUV.zw * ScaleFactor;\n"
             "    ");


        pkFrag->AddCodeBlock(pkBlock);
    }

    pkLib->AddNode(pkFrag);
}
//--------------------------------------------------------------------------------------------------
EE_NOINLINE static void CreateFragment38(NiMaterialNodeLibrary* pkLib)
{
    NiMaterialFragmentNode* pkFrag = NiNew NiMaterialFragmentNode();

    pkFrag->SetType("Pixel");
    pkFrag->SetName("CalcBlendRGBAndAccumulate");
    pkFrag->SetDescription("\n"
        "      This fragment multiplies an RGB color by a mask value from 0.0 - 1.0 and\n"
        "      adds the result to the additional input.\n"
        "    ");
    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float");
        pkRes->SetVariable("Mask");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetSemantic("Color");
        pkRes->SetVariable("InputColor");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetSemantic("Color");
        pkRes->SetVariable("AccumColor");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetVariable("OutputColor");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert a code block
    {
        NiCodeBlock* pkBlock = NiNew NiCodeBlock();

        pkBlock->SetLanguage("hlsl/Cg");
        pkBlock->SetPlatform("D3D11/D3D10/DX9/Xenon/PS3");
        pkBlock->SetTarget("vs_1_1/ps_2_0/vs_3_0/ps_3_0/vs_4_0/ps_4_0/vs_5_0/ps_5_0");

        pkBlock->SetText("\n"
             "      OutputColor = AccumColor + (InputColor * Mask);\n"
             "    ");


        pkFrag->AddCodeBlock(pkBlock);
    }

    pkLib->AddNode(pkFrag);
}
//--------------------------------------------------------------------------------------------------
EE_NOINLINE static void CreateFragment39(NiMaterialNodeLibrary* pkLib)
{
    NiMaterialFragmentNode* pkFrag = NiNew NiMaterialFragmentNode();

    pkFrag->SetType("Pixel");
    pkFrag->SetName("CalcBlendFloatAndAccumulate");
    pkFrag->SetDescription("\n"
        "      This fragment multiplies a single floating point value by a mask value from 0.0 "
        "- 1.0 and\n"
        "      adds the result to the additional input.\n"
        "    ");
    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float");
        pkRes->SetVariable("Mask");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float");
        pkRes->SetVariable("Input");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float");
        pkRes->SetVariable("AccumValue");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float");
        pkRes->SetVariable("Output");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert a code block
    {
        NiCodeBlock* pkBlock = NiNew NiCodeBlock();

        pkBlock->SetLanguage("hlsl/Cg");
        pkBlock->SetPlatform("D3D11/D3D10/DX9/Xenon/PS3");
        pkBlock->SetTarget("vs_1_1/ps_2_0/vs_3_0/ps_3_0/vs_4_0/ps_4_0/vs_5_0/ps_5_0");

        pkBlock->SetText("\n"
             "      Output = AccumValue + (Mask * Input);\n"
             "    ");


        pkFrag->AddCodeBlock(pkBlock);
    }

    pkLib->AddNode(pkFrag);
}
//--------------------------------------------------------------------------------------------------
EE_NOINLINE static void CreateFragment40(NiMaterialNodeLibrary* pkLib)
{
    NiMaterialFragmentNode* pkFrag = NiNew NiMaterialFragmentNode();

    pkFrag->SetType("Pixel");
    pkFrag->SetName("CalcBlendBaseMapWithDetailMap");
    pkFrag->SetDescription("\n"
        "      This fragment multiplies an RGBA color by a mask value from 0.0 - 1.0 and\n"
        "      adds the result to the additional input. The RGB and A channels are\n"
        "      seperated where the A channel represents a detail mask value.\n"
        "    ");
    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float");
        pkRes->SetVariable("Mask");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float4");
        pkRes->SetSemantic("Color");
        pkRes->SetVariable("InputColor");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetSemantic("Color");
        pkRes->SetVariable("AccumColor");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetVariable("OutputColor");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert a code block
    {
        NiCodeBlock* pkBlock = NiNew NiCodeBlock();

        pkBlock->SetLanguage("hlsl/Cg");
        pkBlock->SetPlatform("D3D11/D3D10/DX9/Xenon/PS3");
        pkBlock->SetTarget("vs_1_1/ps_2_0/vs_3_0/ps_3_0/vs_4_0/ps_4_0/vs_5_0/ps_5_0");

        pkBlock->SetText("\n"
             "      OutputColor = AccumColor + (InputColor.rgb * Mask * 2 * InputColor.a);\n"
             "    ");


        pkFrag->AddCodeBlock(pkBlock);
    }

    pkLib->AddNode(pkFrag);
}
//--------------------------------------------------------------------------------------------------
EE_NOINLINE static void CreateFragment41(NiMaterialNodeLibrary* pkLib)
{
    NiMaterialFragmentNode* pkFrag = NiNew NiMaterialFragmentNode();

    pkFrag->SetType("Vertex/Pixel");
    pkFrag->SetName("CalcSurfaceUVSet_ScaleAndOffset");
    pkFrag->SetDescription("\n"
        "      This fragment scales and offsets a uv texture coordinate value. \n"
        "      Usefull for linearly modifying a texture coordinate. Used by a surface\n"
        "      to modify its texture coordinates independantly of the other surfaces.\n"
        "    ");
    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float4");
        pkRes->SetSemantic("TexCoord");
        pkRes->SetVariable("InputUV");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float2");
        pkRes->SetSemantic("TexCoord");
        pkRes->SetVariable("Scale");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float2");
        pkRes->SetSemantic("TexCoord");
        pkRes->SetVariable("Offset");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float4");
        pkRes->SetSemantic("TexCoord");
        pkRes->SetVariable("OutputUV");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert a code block
    {
        NiCodeBlock* pkBlock = NiNew NiCodeBlock();

        pkBlock->SetLanguage("hlsl/Cg");
        pkBlock->SetPlatform("D3D11/D3D10/DX9/Xenon/PS3");
        pkBlock->SetTarget("vs_1_1/ps_2_0/vs_3_0/ps_3_0/vs_4_0/ps_4_0/vs_5_0/ps_5_0");

        pkBlock->SetText("\n"
             "        OutputUV.x = InputUV.x;\n"
             "        OutputUV.y = InputUV.y;\n"
             "        OutputUV.z = InputUV.x * Scale.x + Offset.x;\n"
             "        OutputUV.w = InputUV.y * Scale.y + Offset.y;\n"
             "    ");


        pkFrag->AddCodeBlock(pkBlock);
    }

    pkLib->AddNode(pkFrag);
}
//--------------------------------------------------------------------------------------------------
EE_NOINLINE static void CreateFragment42(NiMaterialNodeLibrary* pkLib)
{
    NiMaterialFragmentNode* pkFrag = NiNew NiMaterialFragmentNode();

    pkFrag->SetType("Vertex/Pixel");
    pkFrag->SetName("AccessSurfaceUVModifiersArray");
    pkFrag->SetDescription("\n"
        "      This fragment extracts the scale and offset parameters for the specified\n"
        "      surface from the UV Modifier arrays.\n"
        "    ");
    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float4");
        pkRes->SetVariable("UVModifierArray");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("int");
        pkRes->SetVariable("Index");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float2");
        pkRes->SetSemantic("TexCoord");
        pkRes->SetVariable("OutputScale");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float2");
        pkRes->SetSemantic("TexCoord");
        pkRes->SetVariable("OutputOffset");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert a code block
    {
        NiCodeBlock* pkBlock = NiNew NiCodeBlock();

        pkBlock->SetLanguage("hlsl/Cg");
        pkBlock->SetPlatform("D3D11/D3D10/DX9/Xenon/PS3");
        pkBlock->SetTarget("vs_1_1/ps_2_0/vs_3_0/ps_3_0/vs_4_0/ps_4_0/vs_5_0/ps_5_0");

        pkBlock->SetText("\n"
             "      OutputScale.xy = UVModifierArray[Index].xy;\n"
             "      OutputOffset.xy = UVModifierArray[Index].zw;\n"
             "    ");


        pkFrag->AddCodeBlock(pkBlock);
    }

    pkLib->AddNode(pkFrag);
}
//--------------------------------------------------------------------------------------------------
EE_NOINLINE static void CreateFragment43(NiMaterialNodeLibrary* pkLib)
{
    NiMaterialFragmentNode* pkFrag = NiNew NiMaterialFragmentNode();

    pkFrag->SetType("Vertex/Pixel");
    pkFrag->SetName("AccessSurfaceUVModifiers");
    pkFrag->SetDescription("\n"
        "      This fragment extracts the scale and offset parameters for the specified\n"
        "      surface from the UV Modifier arrays. \n"
        "    ");
    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float4");
        pkRes->SetVariable("UVModifierArray");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float2");
        pkRes->SetSemantic("TexCoord");
        pkRes->SetVariable("OutputScale");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float2");
        pkRes->SetSemantic("TexCoord");
        pkRes->SetVariable("OutputOffset");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert a code block
    {
        NiCodeBlock* pkBlock = NiNew NiCodeBlock();

        pkBlock->SetLanguage("hlsl/Cg");
        pkBlock->SetPlatform("D3D11/D3D10/DX9/Xenon/PS3");
        pkBlock->SetTarget("vs_1_1/ps_2_0/vs_3_0/ps_3_0/vs_4_0/ps_4_0/vs_5_0/ps_5_0");

        pkBlock->SetText("\n"
             "      OutputScale.xy = UVModifierArray.xy;\n"
             "      OutputOffset.xy = UVModifierArray.zw;\n"
             "    ");


        pkFrag->AddCodeBlock(pkBlock);
    }

    pkLib->AddNode(pkFrag);
}
//--------------------------------------------------------------------------------------------------
EE_NOINLINE static void CreateFragment44(NiMaterialNodeLibrary* pkLib)
{
    NiMaterialFragmentNode* pkFrag = NiNew NiMaterialFragmentNode();

    pkFrag->SetType("Pixel/Vertex");
    pkFrag->SetName("InverseRatio");
    pkFrag->SetDescription("\n"
        "    This fragment is responsible for linearly interpolating two float3's. \n"
        "    ");
    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float");
        pkRes->SetVariable("Ratio");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float");
        pkRes->SetVariable("Output");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert a code block
    {
        NiCodeBlock* pkBlock = NiNew NiCodeBlock();

        pkBlock->SetLanguage("hlsl/Cg");
        pkBlock->SetPlatform("D3D11/D3D10/DX9/Xenon/PS3");
        pkBlock->SetTarget("vs_1_1/ps_2_0/vs_3_0/ps_3_0/vs_4_0/ps_4_0/vs_5_0/ps_5_0");

        pkBlock->SetText("\n"
             "    Output = 1.0 - Ratio;\n"
             "    ");


        pkFrag->AddCodeBlock(pkBlock);
    }

    pkLib->AddNode(pkFrag);
}
//--------------------------------------------------------------------------------------------------
EE_NOINLINE static void CreateFragment45(NiMaterialNodeLibrary* pkLib)
{
    NiMaterialFragmentNode* pkFrag = NiNew NiMaterialFragmentNode();

    pkFrag->SetType("Pixel/Vertex");
    pkFrag->SetName("CompositeFinalRGBColor");
    pkFrag->SetDescription("\n"
        "        This fragment is responsible for computing the final RGB color.\n"
        "        ");
    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetSemantic("Color");
        pkRes->SetVariable("DiffuseColor");
        pkRes->SetDefaultValue("(0.0, 0.0, 0.0)");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetSemantic("Color");
        pkRes->SetVariable("SpecularColor");
        pkRes->SetDefaultValue("(0.0, 0.0, 0.0)");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetSemantic("Color");
        pkRes->SetVariable("OutputColor");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert a code block
    {
        NiCodeBlock* pkBlock = NiNew NiCodeBlock();

        pkBlock->SetLanguage("hlsl/Cg");
        pkBlock->SetPlatform("D3D11/D3D10/DX9/Xenon/PS3");
        pkBlock->SetTarget("vs_1_1/ps_2_0/vs_4_0/ps_4_0/vs_5_0/ps_5_0");

        pkBlock->SetText("\n"
             "        OutputColor.rgb = DiffuseColor.rgb + SpecularColor.rgb;\n"
             "        ");


        pkFrag->AddCodeBlock(pkBlock);
    }

    pkLib->AddNode(pkFrag);
}
//--------------------------------------------------------------------------------------------------
EE_NOINLINE static void CreateFragment46(NiMaterialNodeLibrary* pkLib)
{
    NiMaterialFragmentNode* pkFrag = NiNew NiMaterialFragmentNode();

    pkFrag->SetType("Pixel/Vertex");
    pkFrag->SetName("CompositeFinalRGBAColor");
    pkFrag->SetDescription("\n"
        "        This fragment is responsible for computing the final RGBA color.\n"
        "        ");
    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetSemantic("Color");
        pkRes->SetVariable("FinalColor");
        pkRes->SetDefaultValue("(0.0, 0.0, 0.0)");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float");
        pkRes->SetSemantic("Color");
        pkRes->SetVariable("FinalOpacity");
        pkRes->SetDefaultValue("(1.0)");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float4");
        pkRes->SetSemantic("Color");
        pkRes->SetVariable("OutputColor");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert a code block
    {
        NiCodeBlock* pkBlock = NiNew NiCodeBlock();

        pkBlock->SetLanguage("hlsl/Cg");
        pkBlock->SetPlatform("D3D11/D3D10/DX9/Xenon/PS3");
        pkBlock->SetTarget("vs_1_1/ps_2_0/vs_4_0/ps_4_0/vs_5_0/ps_5_0");

        pkBlock->SetText("\n"
             "        OutputColor.rgb = FinalColor.rgb;\n"
             "        OutputColor.a = saturate(FinalOpacity);\n"
             "        ");


        pkFrag->AddCodeBlock(pkBlock);
    }

    pkLib->AddNode(pkFrag);
}
//--------------------------------------------------------------------------------------------------
EE_NOINLINE static void CreateFragment47(NiMaterialNodeLibrary* pkLib)
{
    NiMaterialFragmentNode* pkFrag = NiNew NiMaterialFragmentNode();

    pkFrag->SetType("Pixel");
    pkFrag->SetName("ColorizeRange");
    pkFrag->SetDescription("\n"
        "      This fragment is responsible for mapping a float value\n"
        "      to a color.\n"
        "    ");
    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float");
        pkRes->SetVariable("Value");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float");
        pkRes->SetVariable("Min");
        pkRes->SetDefaultValue("(0)");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float");
        pkRes->SetVariable("Max");
        pkRes->SetDefaultValue("(1)");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetVariable("AColor");
        pkRes->SetDefaultValue("(0,0,0)");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetVariable("BColor");
        pkRes->SetDefaultValue("(0,0,1)");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetVariable("CColor");
        pkRes->SetDefaultValue("(0,1,0)");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetVariable("DColor");
        pkRes->SetDefaultValue("(1,1,0)");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetVariable("EColor");
        pkRes->SetDefaultValue("(1,0,0)");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetVariable("Color");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert a code block
    {
        NiCodeBlock* pkBlock = NiNew NiCodeBlock();

        pkBlock->SetLanguage("hlsl/Cg");
        pkBlock->SetPlatform("D3D11/D3D10/DX9/Xenon/PS3");
        pkBlock->SetTarget("vs_1_1/ps_2_0/vs_3_0/ps_3_0/vs_4_0/ps_4_0/vs_5_0/ps_5_0");

        pkBlock->SetText("\n"
             "      float Normalized = smoothstep(Min, Max, Value);\n"
             "      \n"
             "      Color  = AColor * (1.0 - saturate(abs(Normalized - (0.0 / 4.0)) * 4.0));\n"
             "      Color += BColor * (1.0 - saturate(abs(Normalized - (1.0 / 4.0)) * 4.0));\n"
             "      Color += CColor * (1.0 - saturate(abs(Normalized - (2.0 / 4.0)) * 4.0));\n"
             "      Color += DColor * (1.0 - saturate(abs(Normalized - (3.0 / 4.0)) * 4.0));\n"
             "      Color += EColor * (1.0 - saturate(abs(Normalized - (4.0 / 4.0)) * 4.0));\n"
             "    ");


        pkFrag->AddCodeBlock(pkBlock);
    }

    pkLib->AddNode(pkFrag);
}
//--------------------------------------------------------------------------------------------------
EE_NOINLINE static void CreateFragment48(NiMaterialNodeLibrary* pkLib)
{
    NiMaterialFragmentNode* pkFrag = NiNew NiMaterialFragmentNode();

    pkFrag->SetType("Pixel");
    pkFrag->SetName("CalculateLayerColor");
    pkFrag->SetDescription("\n"
        "            This fragment combines various different types of maps supported by a terr"
        "ain\n"
        "            surface and produces a final color for a given layer. Assumes the distribu"
        "tion map is \n"
        "            packed into the alpha channel of the base map and the detail map is in the"
        " alpha channel\n"
        "            of the specular map. (spec maps are RGB).\n"
        "        ");
    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("int");
        pkRes->SetVariable("layerIndex");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("bool");
        pkRes->SetVariable("enableBaseMap");
        pkRes->SetDefaultValue("true");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("bool");
        pkRes->SetVariable("enableDistributionMap");
        pkRes->SetDefaultValue("false");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("bool");
        pkRes->SetVariable("enableNormalMap");
        pkRes->SetDefaultValue("false");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("bool");
        pkRes->SetVariable("enableSpecMap");
        pkRes->SetDefaultValue("false");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("bool");
        pkRes->SetVariable("enableDetailMap");
        pkRes->SetDefaultValue("false");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float2");
        pkRes->SetSemantic("TexCoord");
        pkRes->SetVariable("layerUV");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("sampler2D");
        pkRes->SetSemantic("Texture");
        pkRes->SetVariable("baseMap");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("sampler2D");
        pkRes->SetSemantic("Texture");
        pkRes->SetVariable("normalMap");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("sampler2D");
        pkRes->SetSemantic("Texture");
        pkRes->SetVariable("specMap");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float4");
        pkRes->SetVariable("dRamp");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float4");
        pkRes->SetVariable("detailMapScale");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3x3");
        pkRes->SetVariable("nbtMat");
        pkRes->SetDefaultValue("(float3(1, 0, 0), float3(0, 1, 0), float3(1, 0, 0))");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float4");
        pkRes->SetVariable("layerMaskIn");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float4");
        pkRes->SetVariable("layerMaskOut");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetSemantic("Color");
        pkRes->SetVariable("layerColor");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetSemantic("Normal");
        pkRes->SetLabel("World");
        pkRes->SetVariable("layerNormal");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetSemantic("Color");
        pkRes->SetVariable("layerSpecular");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert a code block
    {
        NiCodeBlock* pkBlock = NiNew NiCodeBlock();

        pkBlock->SetLanguage("hlsl/Cg");
        pkBlock->SetPlatform("D3D11/D3D10/DX9/Xenon/PS3");
        pkBlock->SetTarget("vs_1_1/ps_2_0/vs_3_0/ps_3_0/vs_4_0/ps_4_0/vs_5_0/ps_5_0");

        pkBlock->SetText("\n"
             "\n"
             "          layerMaskOut = layerMaskIn;\n"
             "\n"
             "          // Base map and distribution map.\n"
             "                    // Sample the base and distrib maps\n"
             "          float4 sample = float4(1.0f, 1.0f, 1.0f, 0.0f);\n"
             "          if (enableBaseMap || enableDistributionMap)\n"
             "          {\n"
             "              sample = tex2D(baseMap, layerUV);\n"
             "          }\n"
             "          float distMapValue = 0.0f;\n"
             "          if (enableDistributionMap)\n"
             "             {\n"
             "            distMapValue = sample.a;\n"
             "            }\n"
             "          layerColor = float3(1.0, 1.0, 1.0);\n"
             "          if (enableBaseMap)\n"
             "          {\n"
             "                  layerColor = sample.rgb;\n"
             "          }\n"
             "            \n"
             "            // Blend the distib map in\n"
             "            if (enableDistributionMap)\n"
             "              {\n"
             "                      float dRampValue = dRamp[layerIndex];\n"
             "            float inMask = layerMaskIn[layerIndex];\n"
             "            layerMaskOut[layerIndex] = inMask * (1 + dRampValue*distMapValue);   "
             "            \n"
             "              }\n"
             "\n"
             "          // Normal map\n"
             "          if (enableNormalMap)\n"
             "          {\n"
             "            layerNormal = tex2D(normalMap, layerUV).rgg * 2.0f - 1.0f;\n"
             "            layerNormal.z = sqrt(1 - layerNormal.x * layerNormal.x - layerNormal."
             "y * layerNormal.y);\n"
             "            \n"
             "            float3x3 xForm = transpose(nbtMat);\n"
             "            layerNormal = mul(xForm, layerNormal);\n"
             "          }\n"
             "          else\n"
             "          {\n"
             "            layerNormal = nbtMat[2];\n"
             "          }\n"
             "\n"
             "          // Specular map.\n"
             "          if (enableSpecMap)\n"
             "            layerSpecular = tex2D(specMap, layerUV).rgb;\n"
             "          else\n"
             "            layerSpecular = float3(0.0f, 0.0f, 0.0f);\n"
             "\n"
             "          // Detail map.\n"
             "          if (enableDetailMap)\n"
             "          {\n"
             "            float detailMapValue = tex2D(specMap, layerUV * detailMapScale[layerI"
             "ndex]).a;\n"
             "            layerColor = layerColor * 2.0f * detailMapValue;\n"
             "          }\n"
             "\n"
             "        ");


        pkFrag->AddCodeBlock(pkBlock);
    }

    pkLib->AddNode(pkFrag);
}
//--------------------------------------------------------------------------------------------------
EE_NOINLINE static void CreateFragment49(NiMaterialNodeLibrary* pkLib)
{
    NiMaterialFragmentNode* pkFrag = NiNew NiMaterialFragmentNode();

    pkFrag->SetType("Pixel");
    pkFrag->SetName("CalculateTotalParallax");
    pkFrag->SetDescription("\n"
        "            This fragment combines the parallax maps from layers that have a parallax "
        "map using \n"
        "            the strength influence and mask values from each layer to calculate a fina"
        "l parallax\n"
        "            map strength.\n"
        "        ");
    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float4");
        pkRes->SetVariable("maskValues");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetSemantic("ViewVector");
        pkRes->SetVariable("worldViewTS");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("bool");
        pkRes->SetVariable("layer0Enabled");
        pkRes->SetDefaultValue("(false)");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("bool");
        pkRes->SetVariable("layer1Enabled");
        pkRes->SetDefaultValue("(false)");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("bool");
        pkRes->SetVariable("layer2Enabled");
        pkRes->SetDefaultValue("(false)");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("bool");
        pkRes->SetVariable("layer3Enabled");
        pkRes->SetDefaultValue("(false)");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("sampler2D");
        pkRes->SetVariable("pMap0");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("sampler2D");
        pkRes->SetVariable("pMap1");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("sampler2D");
        pkRes->SetVariable("pMap2");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("sampler2D");
        pkRes->SetVariable("pMap3");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float4");
        pkRes->SetVariable("strength");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float2");
        pkRes->SetSemantic("TexCoord");
        pkRes->SetVariable("layerUV0In");
        pkRes->SetDefaultValue("(0, 0)");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float2");
        pkRes->SetSemantic("TexCoord");
        pkRes->SetVariable("layerUV1In");
        pkRes->SetDefaultValue("(0, 0)");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float2");
        pkRes->SetSemantic("TexCoord");
        pkRes->SetVariable("layerUV2In");
        pkRes->SetDefaultValue("(0, 0)");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float2");
        pkRes->SetSemantic("TexCoord");
        pkRes->SetVariable("layerUV3In");
        pkRes->SetDefaultValue("(0, 0)");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float2");
        pkRes->SetSemantic("TexCoord");
        pkRes->SetVariable("layerUV0Out");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float2");
        pkRes->SetSemantic("TexCoord");
        pkRes->SetVariable("layerUV1Out");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float2");
        pkRes->SetSemantic("TexCoord");
        pkRes->SetVariable("layerUV2Out");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float2");
        pkRes->SetSemantic("TexCoord");
        pkRes->SetVariable("layerUV3Out");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float");
        pkRes->SetVariable("totalParallax");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert a code block
    {
        NiCodeBlock* pkBlock = NiNew NiCodeBlock();

        pkBlock->SetLanguage("hlsl/Cg");
        pkBlock->SetPlatform("D3D11/D3D10/DX9/Xenon/PS3");
        pkBlock->SetTarget("vs_1_1/ps_2_0/vs_3_0/ps_3_0/vs_4_0/ps_4_0/vs_5_0/ps_5_0");

        pkBlock->SetText("\n"
             "\n"
             "          layerUV0Out = layerUV0In;\n"
             "          layerUV1Out = layerUV1In;\n"
             "          layerUV2Out = layerUV2In;\n"
             "          layerUV3Out = layerUV3In;\n"
             "          float layerOffset = 0;\n"
             "          totalParallax = 0;\n"
             "          \n"
             "          // Get the total amount of parallax to apply. The total amount of paral"
             "lax is based\n"
             "          // on the combined contribution of parallax from each layer.\n"
             "          if (layer0Enabled)\n"
             "          {\n"
             "            float2 layerUV = layerUV0In;\n"
             "            float layerMask = maskValues.r;\n"
             "            float layerStrength = strength.x;\n"
             "\n"
             "            float layerSample = tex2D(pMap0, layerUV).a;\n"
             "            layerOffset = ((layerSample - 0.5)* layerMask) * layerStrength;\n"
             "            totalParallax += layerSample;\n"
             "            \n"
             "            layerUV0Out = layerUV0In + (layerOffset * worldViewTS.xy);\n"
             "          }\n"
             "\n"
             "          if (layer1Enabled)\n"
             "          {\n"
             "            float2 layerUV = layerUV1In;\n"
             "            float layerMask = maskValues.g;\n"
             "            float layerStrength = strength.y;\n"
             "\n"
             "            float layerSample = tex2D(pMap1, layerUV).a;\n"
             "            layerOffset = ((layerSample - 0.5)* layerMask) * layerStrength;\n"
             "            totalParallax += layerSample;\n"
             "            \n"
             "            layerUV1Out = layerUV1In + (layerOffset * worldViewTS.xy);\n"
             "          }\n"
             "\n"
             "          if (layer2Enabled)\n"
             "          {\n"
             "            float2 layerUV = layerUV2In;\n"
             "            float layerMask = maskValues.b;\n"
             "            float layerStrength = strength.z;\n"
             "\n"
             "            float layerSample = tex2D(pMap2, layerUV).a;\n"
             "            layerOffset = ((layerSample - 0.5)* layerMask) * layerStrength;\n"
             "            totalParallax += layerSample;\n"
             "            \n"
             "            layerUV2Out = layerUV2In + (layerOffset * worldViewTS.xy);\n"
             "          }\n"
             "\n"
             "          if (layer3Enabled)\n"
             "          {\n"
             "            float2 layerUV = layerUV3In;\n"
             "            float layerMask = maskValues.a;\n"
             "            float layerStrength = strength.w;\n"
             "\n"
             "            float layerSample = tex2D(pMap3, layerUV).a;\n"
             "            layerOffset = ((layerSample - 0.5)* layerMask) * layerStrength;\n"
             "            totalParallax += layerSample;\n"
             "\n"
             "            layerUV3Out = layerUV3In + (layerOffset * worldViewTS.xy);\n"
             "          }\n"
             "          \n"
             "        ");


        pkFrag->AddCodeBlock(pkBlock);
    }

    pkLib->AddNode(pkFrag);
}
//--------------------------------------------------------------------------------------------------
EE_NOINLINE static void CreateFragment50(NiMaterialNodeLibrary* pkLib)
{
    NiMaterialFragmentNode* pkFrag = NiNew NiMaterialFragmentNode();

    pkFrag->SetType("Vertex/Pixel");
    pkFrag->SetName("TerrainLight");
    pkFrag->SetDescription("\n"
        "        This fragment is responsible for accumulating the effect of a light\n"
        "        on the current pixel.\n"
        "\n"
        "        LightType can be one of three values:\n"
        "        0 - Directional\n"
        "        1 - Point\n"
        "        2 - Spot\n"
        "\n"
        "        Note that the LightType must be a compile-time variable,\n"
        "        not a runtime constant/uniform variable on most Shader Model 2.0 cards.\n"
        "\n"
        "        The compiler will optimize out any constants that aren't used.\n"
        "\n"
        "        Attenuation is an exponential curve from 1 at the light source to\n"
        "        0 at the range. Falloff > 1 is sharper, Falloff < 1 is wider.\n"
        "        Falloff <= 0 is invalid.\n"
        "\n"
        "        SpotAttenuation is stored as (cos(theta/2), cos(phi/2), falloff)\n"
        "        theta is the angle of the inner cone and phi is the angle of the outer\n"
        "        cone in the traditional DX manner. Gamebryo only allows setting of\n"
        "        phi, so cos(theta/2) will typically be cos(0) or 1. To disable spot\n"
        "        effects entirely, set cos(theta/2) and cos(phi/2) to -1 or lower.\n"
        "    ");
    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float4");
        pkRes->SetSemantic("Position");
        pkRes->SetLabel("World");
        pkRes->SetVariable("WorldPos");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetSemantic("Normal");
        pkRes->SetLabel("World");
        pkRes->SetVariable("WorldNrm");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("int");
        pkRes->SetSemantic("LightType");
        pkRes->SetVariable("LightType");
        pkRes->SetDefaultValue("(0)");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("bool");
        pkRes->SetSemantic("Specularity");
        pkRes->SetVariable("SpecularEnable");
        pkRes->SetDefaultValue("(false)");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float");
        pkRes->SetSemantic("Shadow");
        pkRes->SetVariable("Shadow");
        pkRes->SetDefaultValue("(1.0)");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetSemantic("ViewVector");
        pkRes->SetLabel("World");
        pkRes->SetVariable("WorldViewVector");
        pkRes->SetDefaultValue("(0.0, 0.0, 0.0)");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float4");
        pkRes->SetSemantic("LightPos");
        pkRes->SetLabel("World");
        pkRes->SetVariable("LightPos");
        pkRes->SetDefaultValue("(0.0, 0.0, 0.0, 0.0)");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetSemantic("LightAmbient");
        pkRes->SetVariable("LightAmbient");
        pkRes->SetDefaultValue("(1.0, 1.0, 1.0)");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetSemantic("LightDiffuse");
        pkRes->SetVariable("LightDiffuse");
        pkRes->SetDefaultValue("(1.0, 1.0, 1.0)");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetSemantic("LightSpecular");
        pkRes->SetVariable("LightSpecular");
        pkRes->SetDefaultValue("(1.0, 1.0, 1.0)");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float4");
        pkRes->SetSemantic("LightAttenuation");
        pkRes->SetVariable("LightAttenuation");
        pkRes->SetDefaultValue("(1.0, 1.0, 1.0, 0.0)");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetSemantic("LightSpotAttenuation");
        pkRes->SetVariable("LightSpotAttenuation");
        pkRes->SetDefaultValue("(-1.0, -1.0, 0.0)");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetSemantic("LightDirection");
        pkRes->SetVariable("LightDirection");
        pkRes->SetDefaultValue("(1.0, 0.0, 0.0)");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float4");
        pkRes->SetSemantic("Color");
        pkRes->SetLabel("Specular");
        pkRes->SetVariable("SpecularPower");
        pkRes->SetDefaultValue("(1.0, 1.0, 1.0, 1.0)");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetSemantic("Color");
        pkRes->SetLabel("Ambient");
        pkRes->SetVariable("AmbientAccum");
        pkRes->SetDefaultValue("(0.0, 0.0, 0.0)");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetSemantic("Color");
        pkRes->SetLabel("Diffuse");
        pkRes->SetVariable("DiffuseAccum");
        pkRes->SetDefaultValue("(0.0, 0.0, 0.0)");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetSemantic("Color");
        pkRes->SetLabel("Specular");
        pkRes->SetVariable("SpecularAccum");
        pkRes->SetDefaultValue("(0.0, 0.0, 0.0)");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float4");
        pkRes->SetVariable("maskValues");
        pkRes->SetDefaultValue("(0,0,0,0)");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float4");
        pkRes->SetVariable("layerSpecularPowers");
        pkRes->SetDefaultValue("(0,0,0,0)");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float4");
        pkRes->SetVariable("layerSpecularIntensities");
        pkRes->SetDefaultValue("(0,0,0,0)");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("bool");
        pkRes->SetVariable("layer0Enabled");
        pkRes->SetDefaultValue("(false)");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("bool");
        pkRes->SetVariable("layer1Enabled");
        pkRes->SetDefaultValue("(false)");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("bool");
        pkRes->SetVariable("layer2Enabled");
        pkRes->SetDefaultValue("(false)");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("bool");
        pkRes->SetVariable("layer3Enabled");
        pkRes->SetDefaultValue("(false)");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetVariable("layer0Specular");
        pkRes->SetDefaultValue("(0,0,0)");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetVariable("layer1Specular");
        pkRes->SetDefaultValue("(0,0,0)");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetVariable("layer2Specular");
        pkRes->SetDefaultValue("(0,0,0)");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetVariable("layer3Specular");
        pkRes->SetDefaultValue("(0,0,0)");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetVariable("lowDetailSpecular");
        pkRes->SetDefaultValue("(0,0,0)");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float");
        pkRes->SetVariable("lowDetailSpecularPower");
        pkRes->SetDefaultValue("(0)");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float");
        pkRes->SetVariable("lowDetailSpecularIntensity");
        pkRes->SetDefaultValue("(0)");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetSemantic("Color");
        pkRes->SetLabel("Ambient");
        pkRes->SetVariable("AmbientAccumOut");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetSemantic("Color");
        pkRes->SetLabel("Diffuse");
        pkRes->SetVariable("DiffuseAccumOut");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetSemantic("Color");
        pkRes->SetLabel("Specular");
        pkRes->SetVariable("SpecularAccumOut");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert a code block
    {
        NiCodeBlock* pkBlock = NiNew NiCodeBlock();

        pkBlock->SetLanguage("hlsl/Cg");
        pkBlock->SetPlatform("D3D11/D3D10/DX9/Xenon/PS3");
        pkBlock->SetTarget("vs_1_1/ps_2_0/vs_4_0/ps_4_0/vs_5_0/ps_5_0");

        pkBlock->SetText("   \n"
             "    // Get the world space light vector.\n"
             "    float3 LightVector;\n"
             "    float DistanceToLight;\n"
             "    float DistanceToLightSquared;\n"
             "    \n"
             "    LightVector = LightPos - WorldPos;\n"
             "    DistanceToLightSquared = dot(LightVector, LightVector);\n"
             "    DistanceToLight = length(LightVector);\n"
             "    \n"
             "    if (LightType == 0)\n"
             "    {\n"
             "        LightVector = -LightDirection;\n"
             "    }\n"
             "    else\n"
             "    {\n"
             "        LightVector = normalize(LightVector);\n"
             "    }\n"
             "    \n"
             "    // Take N dot L as intensity.\n"
             "    float LightNDotL = dot(LightVector, WorldNrm);\n"
             "    float LightIntensity = max(0, LightNDotL);\n"
             "\n"
             "    float Attenuate = Shadow;\n"
             "    \n"
             "    {\n"
             "        // Attenuate Here\n"
             "        float Range = LightAttenuation.w;\n"
             "        float Falloff = LightAttenuation.y;\n"
             "        Attenuate = saturate(1.0f - (DistanceToLight / Range));\n"
             "        Attenuate = Attenuate <= 0.0f ? 0.0f : pow(Attenuate, Falloff);\n"
             "        Attenuate = Range <= 0.0f ? 1.0f : Attenuate;\n"
             "        Attenuate *= Shadow;\n"
             "\n"
             "        if (LightType == 2)\n"
             "        {\n"
             "            // Get intensity as cosine of light vector and direction.\n"
             "            float CosAlpha = dot(-LightVector, LightDirection);\n"
             "\n"
             "            // Factor in inner and outer cone angles.\n"
             "            float AttenDiff = LightSpotAttenuation.x - LightSpotAttenuation.y;\n"
             "            CosAlpha = saturate((CosAlpha - LightSpotAttenuation.y) / \n"
             "                AttenDiff);\n"
             "\n"
             "            // Power to falloff.\n"
             "            // The pow() here can create a NaN if CosAlpha is 0 or less.\n"
             "            // On some cards (GeForce 6800), the NaN will propagate through\n"
             "            // a ternary instruction, so we need two to be safe.\n"
             "            float origCosAlpha = CosAlpha;\n"
             "            CosAlpha = origCosAlpha <= 0.0 ? 1.0 : CosAlpha;\n"
             "            CosAlpha = pow(CosAlpha, LightSpotAttenuation.z);\n"
             "            CosAlpha = origCosAlpha <= 0.0 ? 0.0 : CosAlpha;\n"
             "\n"
             "            // Multiply the spot attenuation into the overall attenuation.\n"
             "            Attenuate *= CosAlpha;\n"
             "        }\n"
             "    }\n"
             "    // Determine the interaction of diffuse color of light and material.\n"
             "    // Scale by the attenuated intensity.\n"
             "    DiffuseAccumOut = DiffuseAccum;\n"
             "    DiffuseAccumOut.rgb += LightDiffuse.rgb * LightIntensity * Attenuate;\n"
             "\n"
             "    // Determine ambient contribution - Is affected by shadow\n"
             "    AmbientAccumOut = AmbientAccum;\n"
             "    AmbientAccumOut.rgb += LightAmbient.rgb * Attenuate;\n"
             "\n"
             "    SpecularAccumOut = SpecularAccum;\n"
             "    if (SpecularEnable)\n"
             "    {\n"
             "        // Get the half vector.\n"
             "        float3 LightHalfVector = LightVector + WorldViewVector;\n"
             "        LightHalfVector = normalize(LightHalfVector);\n"
             "\n"
             "        // Determine specular intensity.\n"
             "        float LightNDotH = max(0.00001f, dot(WorldNrm, LightHalfVector));\n"
             "        //if (LightNDotL < 0.0)\n"
             "        //    LightSpecIntensity = 0.0;\n"
             "        // Must use the code below rather than code above.\n"
             "        // Using previous lines will cause the compiler to generate incorrect\n"
             "        // output.\n"
             "        float SpecularMultiplier = LightNDotL > 0.0 ? 1.0 : 0.0;\n"
             "        \n"
             "        // Attenuate Here\n"
             "        float LightSpecIntensity = Attenuate * SpecularMultiplier;\n"
             "        \n"
             "        // Handle the low detail specular        \n"
             "        float SpecIntensity = pow(LightNDotH, lowDetailSpecularPower) * lowDetail"
             "SpecularIntensity * LightSpecIntensity;\n"
             "        SpecularAccumOut.rgb += SpecIntensity * LightSpecular * lowDetailSpecular"
             ";\n"
             "        \n"
             "        // Handle all the layer's specular values for this light\n"
             "        if (layer0Enabled)\n"
             "        {\n"
             "           float layerSpecularPower = layerSpecularPowers.x;\n"
             "           float layerSpecularIntensity = layerSpecularIntensities.x;\n"
             "           float3 layerSpecular = layer0Specular;\n"
             "           float layerMask = maskValues.x;\n"
             "           \n"
             "           float SpecIntensity = pow(LightNDotH, layerSpecularPower) * layerSpecu"
             "larIntensity * LightSpecIntensity;\n"
             "           SpecularAccumOut.rgb += SpecIntensity * LightSpecular * layerSpecular "
             "* layerMask;\n"
             "        }\n"
             "        if (layer1Enabled)\n"
             "        {\n"
             "           float layerSpecularPower = layerSpecularPowers.y;\n"
             "           float layerSpecularIntensity = layerSpecularIntensities.y;\n"
             "           float3 layerSpecular = layer1Specular;\n"
             "           float layerMask = maskValues.y;\n"
             "           \n"
             "           float SpecIntensity = pow(LightNDotH, layerSpecularPower) * layerSpecu"
             "larIntensity * LightSpecIntensity;\n"
             "           SpecularAccumOut.rgb += SpecIntensity * LightSpecular * layerSpecular "
             "* layerMask;\n"
             "        }\n"
             "        if (layer2Enabled)\n"
             "        {\n"
             "           float layerSpecularPower = layerSpecularPowers.z;\n"
             "           float layerSpecularIntensity = layerSpecularIntensities.z;\n"
             "           float3 layerSpecular = layer2Specular;\n"
             "           float layerMask = maskValues.z;\n"
             "           \n"
             "           float SpecIntensity = pow(LightNDotH, layerSpecularPower) * layerSpecu"
             "larIntensity * LightSpecIntensity;\n"
             "           SpecularAccumOut.rgb += SpecIntensity * LightSpecular * layerSpecular "
             "* layerMask;\n"
             "        }\n"
             "        if (layer3Enabled)\n"
             "        {\n"
             "           float layerSpecularPower = layerSpecularPowers.w;\n"
             "           float layerSpecularIntensity = layerSpecularIntensities.w;\n"
             "           float3 layerSpecular = layer3Specular;\n"
             "           float layerMask = maskValues.w;\n"
             "           \n"
             "           float SpecIntensity = pow(LightNDotH, layerSpecularPower) * layerSpecu"
             "larIntensity * LightSpecIntensity;\n"
             "           SpecularAccumOut.rgb += SpecIntensity * LightSpecular * layerSpecular "
             "* layerMask;\n"
             "        }\n"
             "    }       \n"
             "\n"
             "    ");


        pkFrag->AddCodeBlock(pkBlock);
    }

    pkLib->AddNode(pkFrag);
}
//--------------------------------------------------------------------------------------------------
EE_NOINLINE static void CreateFragment51(NiMaterialNodeLibrary* pkLib)
{
    NiMaterialFragmentNode* pkFrag = NiNew NiMaterialFragmentNode();

    pkFrag->SetType("Vertex");
    pkFrag->SetName("GenerateTextureCoordinates");
    pkFrag->SetDescription("\n"
        "      Generates the various sets of texture coordinates that the cell requires\n"
        "    ");
    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float2");
        pkRes->SetSemantic("TexCoord");
        pkRes->SetVariable("uvIn");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float4");
        pkRes->SetVariable("layerScale");
        pkRes->SetDefaultValue("(1.0, 1.0, 1.0, 1.0)");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float2");
        pkRes->SetVariable("blendMapScale");
        pkRes->SetDefaultValue("(1.0, 1.0)");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float2");
        pkRes->SetVariable("blendMapOffset");
        pkRes->SetDefaultValue("(0.0, 0.0)");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float2");
        pkRes->SetVariable("lowResMapScale");
        pkRes->SetDefaultValue("(1.0, 1.0)");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float2");
        pkRes->SetVariable("lowResMapOffset");
        pkRes->SetDefaultValue("(0.0, 0.0)");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float2");
        pkRes->SetVariable("lowResMapSize");
        pkRes->SetDefaultValue("(1.0, 1.0)");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float2");
        pkRes->SetVariable("maskUV");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float2");
        pkRes->SetVariable("layer0UV");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float2");
        pkRes->SetVariable("layer1UV");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float2");
        pkRes->SetVariable("layer2UV");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float2");
        pkRes->SetVariable("layer3UV");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float2");
        pkRes->SetVariable("lowDetailDiffuseUV");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float2");
        pkRes->SetVariable("lowDetailNormalUV");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert a code block
    {
        NiCodeBlock* pkBlock = NiNew NiCodeBlock();

        pkBlock->SetLanguage("hlsl/Cg");
        pkBlock->SetPlatform("D3D11/D3D10/DX9/Xenon/PS3");
        pkBlock->SetTarget("vs_1_1/ps_2_0/vs_3_0/ps_3_0/vs_4_0/ps_4_0/vs_5_0/ps_5_0");

        pkBlock->SetText("\n"
             "\n"
             "      // Scale and translate the incoming UV coordinates into the right coordinat"
             "e space for the given cell\n"
             "      // being rendered. This results in a paramaterization from 0.0 - 1.0 across"
             " the cell being rendered.\n"
             "      maskUV = uvIn * blendMapScale + blendMapOffset;\n"
             "      float2 lowResMapCommonUV = uvIn * lowResMapScale + lowResMapOffset;\n"
             "\n"
             "      layer0UV = lowResMapCommonUV * layerScale.x;\n"
             "      layer1UV = lowResMapCommonUV * layerScale.y;\n"
             "      layer2UV = lowResMapCommonUV * layerScale.z;\n"
             "      layer3UV = lowResMapCommonUV * layerScale.w;\n"
             "\n"
             "      // In order to take into account border issues between sectors, the low det"
             "ail textures need\n"
             "      // to be sampled slightly differently.\n"
             "      \n"
             "      float lowResDiffuseBorder = 1.0f / lowResMapSize.x;\n"
             "      float lowResDiffuseScale = 1.0f - (2.0f * lowResDiffuseBorder);\n"
             "      float2 lowResDiffuseOffset = float2(lowResDiffuseBorder, lowResDiffuseBorde"
             "r) * 1.5f;\n"
             "      lowDetailDiffuseUV = lowResMapCommonUV * lowResDiffuseScale + lowResDiffuse"
             "Offset;\n"
             "\n"
             "      float lowResNormalBorder = 0.5f / lowResMapSize.y;\n"
             "      float lowResNormalScale = 1.0f - (2.0f * lowResNormalBorder);\n"
             "      float2 lowResNormalOffset = float2(lowResNormalBorder, lowResNormalBorder);"
             "\n"
             "      lowDetailNormalUV = lowResMapCommonUV * lowResNormalScale + lowResNormalOff"
             "set;\n"
             "\n"
             "    ");


        pkFrag->AddCodeBlock(pkBlock);
    }

    pkLib->AddNode(pkFrag);
}
//--------------------------------------------------------------------------------------------------
EE_NOINLINE static void CreateFragment52(NiMaterialNodeLibrary* pkLib)
{
    NiMaterialFragmentNode* pkFrag = NiNew NiMaterialFragmentNode();

    pkFrag->SetType("Pixel");
    pkFrag->SetName("DecodeInterpolators");
    pkFrag->SetDescription("\n"
        "            Extracts the interpolators coming into the shader into separate components"
        ".\n"
        "            This vesion is used for parallax mapping environments. \n"
        "        ");
    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("sampler2D");
        pkRes->SetSemantic("Texture");
        pkRes->SetVariable("layerMaskSampler");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("bool");
        pkRes->SetVariable("sampleBlendMask");
        pkRes->SetDefaultValue("(false)");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float4");
        pkRes->SetVariable("texcoord0");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float4");
        pkRes->SetVariable("texcoord1");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float4");
        pkRes->SetVariable("texcoord2");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float4");
        pkRes->SetVariable("texcoord3");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float4");
        pkRes->SetVariable("texcoord4");
        pkRes->SetDefaultValue("(0,0,0,0)");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float4");
        pkRes->SetVariable("texcoord5");
        pkRes->SetDefaultValue("(0,0,0,0)");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float4");
        pkRes->SetVariable("texcoord6");
        pkRes->SetDefaultValue("(0,0,0,0)");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float4");
        pkRes->SetVariable("texcoord7");
        pkRes->SetDefaultValue("(0,0,0,0)");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("bool");
        pkRes->SetVariable("bInputWorldNormal");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("bool");
        pkRes->SetVariable("bInputWorldNBT");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("bool");
        pkRes->SetVariable("bInputWorldPosition");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("bool");
        pkRes->SetVariable("bInputWorldView");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float");
        pkRes->SetVariable("morphValue");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float4");
        pkRes->SetVariable("maskValues");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float2");
        pkRes->SetSemantic("TexCoord");
        pkRes->SetVariable("layer0UV");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float2");
        pkRes->SetSemantic("TexCoord");
        pkRes->SetVariable("layer1UV");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float2");
        pkRes->SetSemantic("TexCoord");
        pkRes->SetVariable("layer2UV");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float2");
        pkRes->SetSemantic("TexCoord");
        pkRes->SetVariable("layer3UV");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float2");
        pkRes->SetSemantic("TexCoord");
        pkRes->SetVariable("lowResDiffuseUV");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float2");
        pkRes->SetSemantic("TexCoord");
        pkRes->SetVariable("lowResNormalUV");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetSemantic("ViewVector");
        pkRes->SetLabel("World");
        pkRes->SetVariable("worldViewOut");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetSemantic("ViewVector");
        pkRes->SetVariable("worldViewTSOut");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float4");
        pkRes->SetSemantic("Position");
        pkRes->SetLabel("World");
        pkRes->SetVariable("worldPosOut");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetSemantic("Normal");
        pkRes->SetLabel("World");
        pkRes->SetVariable("worldNormalOut");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3x3");
        pkRes->SetVariable("nbtMatrix");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert a code block
    {
        NiCodeBlock* pkBlock = NiNew NiCodeBlock();

        pkBlock->SetLanguage("hlsl/Cg");
        pkBlock->SetPlatform("D3D11/D3D10/DX9/Xenon/PS3");
        pkBlock->SetTarget("vs_1_1/ps_2_0/vs_3_0/ps_3_0/vs_4_0/ps_4_0/vs_5_0/ps_5_0");

        pkBlock->SetText("\n"
             "\n"
             "          // Decode the permanent interpolator values\n"
             "          float2 maskUV = texcoord0.xy;\n"
             "          layer0UV = texcoord0.zw;\n"
             "          layer1UV = texcoord1.xy;\n"
             "          layer2UV = texcoord1.zw;\n"
             "          lowResDiffuseUV = texcoord2.xy;\n"
             "          lowResNormalUV = texcoord2.zw;\n"
             "\n"
             "          // Decode the optional interpolators\n"
             "          if (bInputWorldNBT)\n"
             "          {\n"
             "            layer3UV = float2(texcoord3.w, texcoord4.w);\n"
             "            morphValue = texcoord5.w;\n"
             "\n"
             "            float3 n = normalize(texcoord3.xyz);\n"
             "            float3 t = normalize(texcoord4.xyz);\n"
             "            float3 b = normalize(texcoord5.xyz);\n"
             "\n"
             "            nbtMatrix = float3x3(t, b, n);\n"
             "            worldNormalOut = n;\n"
             "\n"
             "            if (bInputWorldPosition && bInputWorldView)\n"
             "            {\n"
             "              worldViewOut = normalize(texcoord6.xyz);\n"
             "              worldPosOut = float4(texcoord7.xyz, 1.0f);\n"
             "            }\n"
             "            else if (bInputWorldPosition && !bInputWorldView)\n"
             "            {\n"
             "              worldViewOut = float3(1,0,0);\n"
             "              worldPosOut = float4(texcoord6.xyz, 1.0f);\n"
             "            }\n"
             "            else if (!bInputWorldPosition && bInputWorldView)\n"
             "            {\n"
             "              worldViewOut = normalize(texcoord6.xyz);\n"
             "              worldPosOut = float4(0,0,0,1);\n"
             "            }\n"
             "            else\n"
             "            {\n"
             "               worldViewOut = float3(1,0,0);\n"
             "               worldPosOut = float4(0,0,0,1);\n"
             "            }\n"
             "            // Total 6-8 interpolators used\n"
             "          }\n"
             "          else if (!bInputWorldNBT && bInputWorldNormal)\n"
             "          {\n"
             "            worldNormalOut = normalize(texcoord3.xyz);\n"
             "            nbtMatrix = float3x3(float3(1,0,0), float3(0,1,0), worldNormalOut);\n"
             "\n"
             "            if(bInputWorldPosition && bInputWorldView)\n"
             "            {\n"
             "              layer3UV = float2(texcoord3.w, texcoord4.w);\n"
             "              morphValue = texcoord5.w;\n"
             "              worldViewOut = normalize(texcoord4.xyz);\n"
             "              worldPosOut = float4(texcoord5.xyz, 1.0f);\n"
             "            }\n"
             "            else if (bInputWorldPosition && !bInputWorldView)\n"
             "            {\n"
             "              layer3UV = texcoord5.xy;\n"
             "              morphValue = texcoord5.w;\n"
             "              worldViewOut = float3(1,0,0);\n"
             "              worldPosOut = float4(texcoord4.xyz, 1.0f);\n"
             "            }\n"
             "            else if (!bInputWorldPosition && bInputWorldView)\n"
             "            {\n"
             "              layer3UV = texcoord5.xy;\n"
             "              morphValue = texcoord5.w;\n"
             "              worldViewOut = normalize(texcoord4.xyz);\n"
             "              worldPosOut = float4(0,0,0,1);\n"
             "            }\n"
             "            else if (!bInputWorldPosition && !bInputWorldView)\n"
             "            {\n"
             "              layer3UV = texcoord4.xy;\n"
             "              morphValue = texcoord4.w;\n"
             "              worldViewOut = float3(1,0,0);\n"
             "              worldPosOut = float4(0,0,0,1);\n"
             "            }\n"
             "            // Total 5-6 interpolators used\n"
             "          }\n"
             "          else if (!bInputWorldNBT && !bInputWorldNormal)\n"
             "          {\n"
             "            worldNormalOut = float3(0,0,1);\n"
             "            nbtMatrix = float3x3(float3(1,0,0), float3(0,1,0), worldNormalOut);\n"
             "            layer3UV = texcoord3.xy;\n"
             "            morphValue = texcoord3.w;\n"
             "\n"
             "            if (bInputWorldPosition && bInputWorldView)\n"
             "            {\n"
             "              worldViewOut = normalize(texcoord4.xyz);\n"
             "              worldPosOut = float4(texcoord5.xyz, 1.0f);\n"
             "            }\n"
             "            else if (bInputWorldPosition && !bInputWorldView)\n"
             "            {\n"
             "              worldViewOut = float3(1,0,0);\n"
             "              worldPosOut = float4(texcoord4.xyz, 1.0f);\n"
             "            }\n"
             "            else if (!bInputWorldPosition && bInputWorldView)\n"
             "            {\n"
             "              worldViewOut = normalize(texcoord4.xyz);\n"
             "              worldPosOut = float4(0,0,0,1);\n"
             "            }\n"
             "            else\n"
             "            {\n"
             "              worldViewOut = float3(1,0,0);\n"
             "              worldPosOut = float4(0,0,0,1);\n"
             "            }\n"
             "            // Total 5-6 interpolators used\n"
             "          }\n"
             "\n"
             "          worldViewTSOut = mul(nbtMatrix, worldViewOut);\n"
             "          \n"
             "          // Sample the blend mask\n"
             "          if (sampleBlendMask)\n"
             "            maskValues = tex2D(layerMaskSampler, maskUV);\n"
             "          else\n"
             "            maskValues = float4(0.5f, 0.5f, 0.0f, 1.0f);\n"
             "\n"
             "        ");


        pkFrag->AddCodeBlock(pkBlock);
    }

    pkLib->AddNode(pkFrag);
}
//--------------------------------------------------------------------------------------------------
EE_NOINLINE static void CreateFragment53(NiMaterialNodeLibrary* pkLib)
{
    NiMaterialFragmentNode* pkFrag = NiNew NiMaterialFragmentNode();

    pkFrag->SetType("Vertex");
    pkFrag->SetName("EncodeInterpolators");
    pkFrag->SetDescription("\n"
        "      This version of encoded interpolators supports both normal and parallax mapping."
        "\n"
        "      These can only be used with pure per-pixel lighting.\n"
        "    ");
    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float2");
        pkRes->SetVariable("maskUV");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float2");
        pkRes->SetVariable("layer0UV");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float2");
        pkRes->SetVariable("layer1UV");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float2");
        pkRes->SetVariable("layer2UV");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float2");
        pkRes->SetVariable("layer3UV");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float2");
        pkRes->SetVariable("lowDetailDiffuseUV");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float2");
        pkRes->SetVariable("lowDetailNormalUV");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float");
        pkRes->SetVariable("morphValue");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetSemantic("ViewVector");
        pkRes->SetVariable("worldViewIn");
        pkRes->SetDefaultValue("(1,0,0)");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetSemantic("Position");
        pkRes->SetLabel("World");
        pkRes->SetVariable("worldPosIn");
        pkRes->SetDefaultValue("(0,0,0)");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetVariable("worldNormalIn");
        pkRes->SetDefaultValue("(0,0,1)");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetVariable("worldTangentIn");
        pkRes->SetDefaultValue("(1,0,0)");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetVariable("worldBinormalIn");
        pkRes->SetDefaultValue("(0,1,0)");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("bool");
        pkRes->SetVariable("bOutputWorldNormal");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("bool");
        pkRes->SetVariable("bOutputWorldNBT");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("bool");
        pkRes->SetVariable("bOutputWorldPosition");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("bool");
        pkRes->SetVariable("bOutputWorldView");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float4");
        pkRes->SetVariable("texcoord0");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float4");
        pkRes->SetVariable("texcoord1");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float4");
        pkRes->SetVariable("texcoord2");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float4");
        pkRes->SetVariable("texcoord3");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float4");
        pkRes->SetVariable("texcoord4");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float4");
        pkRes->SetVariable("texcoord5");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float4");
        pkRes->SetVariable("texcoord6");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float4");
        pkRes->SetVariable("texcoord7");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert a code block
    {
        NiCodeBlock* pkBlock = NiNew NiCodeBlock();

        pkBlock->SetLanguage("hlsl/Cg");
        pkBlock->SetPlatform("D3D11/D3D10/DX9/Xenon/PS3");
        pkBlock->SetTarget("vs_1_1/ps_2_0/vs_3_0/ps_3_0/vs_4_0/ps_4_0/vs_5_0/ps_5_0");

        pkBlock->SetText("\n"
             "\n"
             "      // Encode permanent interpolators\n"
             "      texcoord0 = float4(maskUV, layer0UV);\n"
             "      texcoord1 = float4(layer1UV, layer2UV);\n"
             "      texcoord2 = float4(lowDetailDiffuseUV, lowDetailNormalUV);\n"
             "\n"
             "      // Encode optional interpolators\n"
             "      if (bOutputWorldNBT)\n"
             "      {\n"
             "        texcoord3 = float4(worldNormalIn.xyz, layer3UV.x);\n"
             "        texcoord4 = float4(worldTangentIn.xyz, layer3UV.y);\n"
             "        texcoord5 = float4(worldBinormalIn.xyz, morphValue);\n"
             "\n"
             "        if (bOutputWorldPosition && bOutputWorldView)\n"
             "        {\n"
             "          texcoord6 = float4(worldViewIn, 1.0f);\n"
             "          texcoord7 = float4(worldPosIn, 1.0f);\n"
             "        }\n"
             "        else if (bOutputWorldPosition && !bOutputWorldView)\n"
             "        {\n"
             "          texcoord6 = float4(worldPosIn, 1.0f);\n"
             "          texcoord7 = float4(0,0,0,0);\n"
             "        }\n"
             "        else if (!bOutputWorldPosition && bOutputWorldView)\n"
             "        {\n"
             "          texcoord6 = float4(worldViewIn, 1.0f);\n"
             "          texcoord7 = float4(0,0,0,0);\n"
             "        }\n"
             "        else\n"
             "        {\n"
             "          texcoord6 = float4(0,0,0,0);\n"
             "          texcoord7 = float4(0,0,0,0);\n"
             "        }\n"
             "        // Total 6-8 interpolators used\n"
             "      }\n"
             "      else if (!bOutputWorldNBT && bOutputWorldNormal)\n"
             "      {\n"
             "        texcoord3 = float4(worldNormalIn.xyz, layer3UV.x);\n"
             "\n"
             "        if(bOutputWorldPosition && bOutputWorldView)\n"
             "        {\n"
             "          texcoord4 = float4(worldViewIn.xyz, layer3UV.y);\n"
             "          texcoord5 = float4(worldPosIn.xyz, morphValue);\n"
             "        }\n"
             "        else if (bOutputWorldPosition && !bOutputWorldView)\n"
             "        {\n"
             "          texcoord4 = float4(worldPosIn.xyz, 1.0f);\n"
             "          texcoord5 = float4(layer3UV, 1.0f, morphValue);\n"
             "        }\n"
             "        else if (!bOutputWorldPosition && bOutputWorldView)\n"
             "        {\n"
             "          texcoord4 = float4(worldViewIn.xyz, 1.0f);\n"
             "          texcoord5 = float4(layer3UV, 1.0f, morphValue);\n"
             "        }\n"
             "        else if (!bOutputWorldPosition && !bOutputWorldView)\n"
             "        {\n"
             "          texcoord4 = float4(layer3UV, 1.0f, morphValue);\n"
             "          texcoord5 = float4(0,0,0,0);\n"
             "        }\n"
             "        // Total 5-6 interpolators used\n"
             "        texcoord6 = float4(0,0,0,0);\n"
             "        texcoord7 = float4(0,0,0,0);\n"
             "      }\n"
             "      else if (!bOutputWorldNBT && !bOutputWorldNormal)\n"
             "      {\n"
             "        texcoord3 = float4(layer3UV, 1.0f, morphValue);\n"
             "\n"
             "        if (bOutputWorldPosition && bOutputWorldView)\n"
             "        {\n"
             "          texcoord4 = float4(worldViewIn, 1.0f);\n"
             "          texcoord5 = float4(worldPosIn, 1.0f);\n"
             "        }\n"
             "        else if (bOutputWorldPosition && !bOutputWorldView)\n"
             "        {\n"
             "          texcoord4 = float4(worldPosIn, 1.0f);\n"
             "          texcoord5 = float4(0,0,0,0);\n"
             "        }\n"
             "        else if (!bOutputWorldPosition && bOutputWorldView)\n"
             "        {\n"
             "          texcoord4 = float4(worldViewIn, 1.0f);\n"
             "          texcoord5 = float4(0,0,0,0);\n"
             "        }\n"
             "        else\n"
             "        {\n"
             "          texcoord4 = float4(0,0,0,0);\n"
             "          texcoord5 = float4(0,0,0,0);\n"
             "        }\n"
             "        // Total 5-6 interpolators used\n"
             "        texcoord6 = float4(0,0,0,0);\n"
             "        texcoord7 = float4(0,0,0,0);\n"
             "      }\n"
             "\n"
             "    ");


        pkFrag->AddCodeBlock(pkBlock);
    }

    pkLib->AddNode(pkFrag);
}
//--------------------------------------------------------------------------------------------------
EE_NOINLINE static void CreateFragment54(NiMaterialNodeLibrary* pkLib)
{
    NiMaterialFragmentNode* pkFrag = NiNew NiMaterialFragmentNode();

    pkFrag->SetType("Pixel");
    pkFrag->SetName("CombineLightingAndLayerColor");
    pkFrag->SetDescription("\n"
        "            Combines the accumulated lighting values with the final blended layer colo"
        "r.\n"
        "        ");
    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetSemantic("Color");
        pkRes->SetVariable("lightDiffuse");
        pkRes->SetDefaultValue("(0.0f, 0.0f, 0.0f)");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetSemantic("Color");
        pkRes->SetVariable("lightAmbient");
        pkRes->SetDefaultValue("(0.0f, 0.0f, 0.0f)");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetSemantic("Color");
        pkRes->SetVariable("finalLayerColor");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float4");
        pkRes->SetSemantic("Color");
        pkRes->SetVariable("colorOut");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert a code block
    {
        NiCodeBlock* pkBlock = NiNew NiCodeBlock();

        pkBlock->SetLanguage("hlsl/Cg");
        pkBlock->SetPlatform("D3D11/D3D10/DX9/Xenon/PS3");
        pkBlock->SetTarget("vs_1_1/ps_2_0/vs_3_0/ps_3_0/vs_4_0/ps_4_0/vs_5_0/ps_5_0");

        pkBlock->SetText("\n"
             "\n"
             "            colorOut = float4(lightAmbient + (lightDiffuse * finalLayerColor), 1."
             "0f);\n"
             "            \n"
             "        ");


        pkFrag->AddCodeBlock(pkBlock);
    }

    pkLib->AddNode(pkFrag);
}
//--------------------------------------------------------------------------------------------------
EE_NOINLINE static void CreateFragment55(NiMaterialNodeLibrary* pkLib)
{
    NiMaterialFragmentNode* pkFrag = NiNew NiMaterialFragmentNode();

    pkFrag->SetType("Pixel");
    pkFrag->SetName("CombineAllLayers");
    pkFrag->SetDescription("\n"
        "            Combines the results of each surface material layer to produce a final alb"
        "edo, normal, and specular\n"
        "            result. The mask values are also normalized to take into account and contr"
        "ibution from a layer's \n"
        "            distribution mask.\n"
        "        ");
    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float4");
        pkRes->SetVariable("maskValues");
        pkRes->SetDefaultValue("(0.0f, 0.0f, 0.0f, 0.0f)");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetVariable("layer0Color");
        pkRes->SetDefaultValue("(0.0f, 0.0f, 0.0f)");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetVariable("layer1Color");
        pkRes->SetDefaultValue("(0.0f, 0.0f, 0.0f)");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetVariable("layer2Color");
        pkRes->SetDefaultValue("(0.0f, 0.0f, 0.0f)");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetVariable("layer3Color");
        pkRes->SetDefaultValue("(0.0f, 0.0f, 0.0f)");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetVariable("layer0Normal");
        pkRes->SetDefaultValue("(0.0f, 0.0f, 0.0f)");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetVariable("layer1Normal");
        pkRes->SetDefaultValue("(0.0f, 0.0f, 0.0f)");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetVariable("layer2Normal");
        pkRes->SetDefaultValue("(0.0f, 0.0f, 0.0f)");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetVariable("layer3Normal");
        pkRes->SetDefaultValue("(0.0f, 0.0f, 0.0f)");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetVariable("layer0Spec");
        pkRes->SetDefaultValue("(0.0f, 0.0f, 0.0f)");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetVariable("layer1Spec");
        pkRes->SetDefaultValue("(0.0f, 0.0f, 0.0f)");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetVariable("layer2Spec");
        pkRes->SetDefaultValue("(0.0f, 0.0f, 0.0f)");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetVariable("layer3Spec");
        pkRes->SetDefaultValue("(0.0f, 0.0f, 0.0f)");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetVariable("defaultColor");
        pkRes->SetDefaultValue("(1.0f, 0.0f, 1.0f)");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetVariable("defaultNormal");
        pkRes->SetDefaultValue("(0.0f, 0.0f, 1.0f)");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetVariable("defaultSpec");
        pkRes->SetDefaultValue("(0.0f, 0.0f, 0.0f)");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float4");
        pkRes->SetVariable("maskValuesOut");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float");
        pkRes->SetVariable("finalMaskSum");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetVariable("finalColor");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetVariable("finalNormal");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetVariable("finalSpecular");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert a code block
    {
        NiCodeBlock* pkBlock = NiNew NiCodeBlock();

        pkBlock->SetLanguage("hlsl/Cg");
        pkBlock->SetPlatform("D3D11/D3D10/DX9/Xenon/PS3");
        pkBlock->SetTarget("vs_1_1/ps_2_0/vs_3_0/ps_3_0/vs_4_0/ps_4_0/vs_5_0/ps_5_0");

        pkBlock->SetText("\n"
             "\n"
             "          // Make sure mask values are normalized such that they total 1.0. Distr"
             "ibution\n"
             "          // masks could cause the mask values to total > 1.0\n"
             "          maskValuesOut = maskValues;\n"
             "          finalMaskSum = dot(maskValuesOut, float4(1,1,1,1));\n"
             "          if (finalMaskSum > 1.0)\n"
             "             maskValuesOut /= finalMaskSum;\n"
             "          float defaultMask = 1.0 - dot(maskValuesOut, float4(1,1,1,1));\n"
             "\n"
             "          finalColor = layer0Color * maskValuesOut.r + layer1Color * maskValuesOu"
             "t.g +\n"
             "            layer2Color * maskValuesOut.b + layer3Color * maskValuesOut.a + defau"
             "ltColor * defaultMask;\n"
             "\n"
             "          finalNormal = layer0Normal * maskValuesOut.r + layer1Normal * maskValue"
             "sOut.g +\n"
             "            layer2Normal * maskValuesOut.b + layer3Normal * maskValuesOut.a + def"
             "aultNormal * defaultMask;\n"
             "          finalNormal = normalize(finalNormal);\n"
             "\n"
             "          finalSpecular = layer0Spec * maskValuesOut.r + layer1Spec * maskValuesO"
             "ut.g +\n"
             "            layer2Spec * maskValuesOut.b + layer3Spec * maskValuesOut.a + default"
             "Spec * defaultMask;\n"
             "\n"
             "        ");


        pkFrag->AddCodeBlock(pkBlock);
    }

    pkLib->AddNode(pkFrag);
}
//--------------------------------------------------------------------------------------------------
EE_NOINLINE static void CreateFragment56(NiMaterialNodeLibrary* pkLib)
{
    NiMaterialFragmentNode* pkFrag = NiNew NiMaterialFragmentNode();

    pkFrag->SetType("Pixel");
    pkFrag->SetName("CombineSpecularPowers");
    pkFrag->SetDescription("\n"
        "            Combines the results of each surface material specular power to produce \n"
        "            a specular power based only on the layers that have specular enabled.\n"
        "        ");
    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float4");
        pkRes->SetVariable("maskValues");
        pkRes->SetDefaultValue("(0.0f, 0.0f, 0.0f, 0.0f)");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float4");
        pkRes->SetVariable("layerSpecularPowers");
        pkRes->SetDefaultValue("(0,0,0,0)");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float4");
        pkRes->SetVariable("layerSpecularIntensities");
        pkRes->SetDefaultValue("(0,0,0,0)");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("bool");
        pkRes->SetVariable("layer0Enabled");
        pkRes->SetDefaultValue("(false)");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("bool");
        pkRes->SetVariable("layer1Enabled");
        pkRes->SetDefaultValue("(false)");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("bool");
        pkRes->SetVariable("layer2Enabled");
        pkRes->SetDefaultValue("(false)");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("bool");
        pkRes->SetVariable("layer3Enabled");
        pkRes->SetDefaultValue("(false)");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float4");
        pkRes->SetVariable("finalSpecularPower");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float");
        pkRes->SetVariable("finalSpecularIntensity");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert a code block
    {
        NiCodeBlock* pkBlock = NiNew NiCodeBlock();

        pkBlock->SetLanguage("hlsl/Cg");
        pkBlock->SetPlatform("D3D11/D3D10/DX9/Xenon/PS3");
        pkBlock->SetTarget("vs_1_1/ps_2_0/vs_3_0/ps_3_0/vs_4_0/ps_4_0/vs_5_0/ps_5_0");

        pkBlock->SetText("\n"
             "\n"
             "            // Mask values are already normalized from the CombineAllLayers fragm"
             "ent but need\n"
             "            // renormalizing such that the available layers total 1.\n"
             "            float4 layerEnabledMask = float4(\n"
             "                layer0Enabled ? 1 : 0,\n"
             "                layer1Enabled ? 1 : 0,\n"
             "                layer2Enabled ? 1 : 0,\n"
             "                layer3Enabled ? 1 : 0);\n"
             "            float4 specMaskValues = maskValues * layerEnabledMask;\n"
             "\n"
             "            // Normalize the spec mask values\n"
             "            float maskSum = dot(specMaskValues, float4(1,1,1,1));\n"
             "            if (maskSum > 0)\n"
             "                specMaskValues /= maskSum;\n"
             "\n"
             "            // Calculate final specular power from mask values\n"
             "            finalSpecularPower.x = layerSpecularPowers.r * specMaskValues.r +\n"
             "                layerSpecularPowers.g * specMaskValues.g +\n"
             "                layerSpecularPowers.b * specMaskValues.b +\n"
             "                layerSpecularPowers.a * specMaskValues.a;\n"
             "            finalSpecularPower.yzw = finalSpecularPower.xxx;\n"
             "\n"
             "            // Calculate final specular intensity from original mask values\n"
             "            finalSpecularIntensity = layerSpecularIntensities.r * maskValues.r +\n"
             "                layerSpecularIntensities.g * maskValues.g +\n"
             "                layerSpecularIntensities.b * maskValues.b +\n"
             "                layerSpecularIntensities.a * maskValues.a;\n"
             "        ");


        pkFrag->AddCodeBlock(pkBlock);
    }

    pkLib->AddNode(pkFrag);
}
//--------------------------------------------------------------------------------------------------
EE_NOINLINE static void CreateFragment57(NiMaterialNodeLibrary* pkLib)
{
    NiMaterialFragmentNode* pkFrag = NiNew NiMaterialFragmentNode();

    pkFrag->SetType("Pixel");
    pkFrag->SetName("BreakoutLowDetailSpecularConfig");
    pkFrag->SetDescription("\n"
        "            Splits the data stored in the low detail specular config shader constant\n"
        "        ");
    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float2");
        pkRes->SetVariable("LowDetailSpecularData");
        pkRes->SetDefaultValue("(100.0f, 0.0f)");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float4");
        pkRes->SetVariable("lowDetailSpecularPower");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float");
        pkRes->SetVariable("lowDetailSpecularIntensity");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert a code block
    {
        NiCodeBlock* pkBlock = NiNew NiCodeBlock();

        pkBlock->SetLanguage("hlsl/Cg");
        pkBlock->SetPlatform("D3D11/D3D10/DX9/Xenon/PS3");
        pkBlock->SetTarget("vs_1_1/ps_2_0/vs_3_0/ps_3_0/vs_4_0/ps_4_0/vs_5_0/ps_5_0");

        pkBlock->SetText("\n"
             "            lowDetailSpecularPower = LowDetailSpecularData.xxxx;\n"
             "            lowDetailSpecularIntensity = LowDetailSpecularData.y;\n"
             "        ");


        pkFrag->AddCodeBlock(pkBlock);
    }

    pkLib->AddNode(pkFrag);
}
//--------------------------------------------------------------------------------------------------
NiMaterialNodeLibrary* 
    NiTerrainMaterialNodeLibrary::CreateMaterialNodeLibrary()
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
    CreateFragment12(pkLib);
    CreateFragment13(pkLib);
    CreateFragment14(pkLib);
    CreateFragment15(pkLib);
    CreateFragment16(pkLib);
    CreateFragment17(pkLib);
    CreateFragment18(pkLib);
    CreateFragment19(pkLib);
    CreateFragment20(pkLib);
    CreateFragment21(pkLib);
    CreateFragment22(pkLib);
    CreateFragment23(pkLib);
    CreateFragment24(pkLib);
    CreateFragment25(pkLib);
    CreateFragment26(pkLib);
    CreateFragment27(pkLib);
    CreateFragment28(pkLib);
    CreateFragment29(pkLib);
    CreateFragment30(pkLib);
    CreateFragment31(pkLib);
    CreateFragment32(pkLib);
    CreateFragment33(pkLib);
    CreateFragment34(pkLib);
    CreateFragment35(pkLib);
    CreateFragment36(pkLib);
    CreateFragment37(pkLib);
    CreateFragment38(pkLib);
    CreateFragment39(pkLib);
    CreateFragment40(pkLib);
    CreateFragment41(pkLib);
    CreateFragment42(pkLib);
    CreateFragment43(pkLib);
    CreateFragment44(pkLib);
    CreateFragment45(pkLib);
    CreateFragment46(pkLib);
    CreateFragment47(pkLib);
    CreateFragment48(pkLib);
    CreateFragment49(pkLib);
    CreateFragment50(pkLib);
    CreateFragment51(pkLib);
    CreateFragment52(pkLib);
    CreateFragment53(pkLib);
    CreateFragment54(pkLib);
    CreateFragment55(pkLib);
    CreateFragment56(pkLib);
    CreateFragment57(pkLib);

    return pkLib;
}
//--------------------------------------------------------------------------------------------------

