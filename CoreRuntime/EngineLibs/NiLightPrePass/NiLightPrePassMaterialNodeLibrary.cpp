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

#include "NiLightPrePassPCH.h"

#include <NiMaterialFragmentNode.h>
#include <NiMaterialNodeLibrary.h>
#include <NiMaterialResource.h>
#include <NiCodeBlock.h>
#include "NiLightPrePassMaterialNodeLibrary.h"

//--------------------------------------------------------------------------------------------------
EE_NOINLINE static void CreateFragment0(NiMaterialNodeLibrary* pkLib)
{
    NiMaterialFragmentNode* pkFrag = NiNew NiMaterialFragmentNode();

    pkFrag->SetType("Vertex/Pixel");
    pkFrag->SetName("LPPDepthNormal");
    pkFrag->SetDescription("\n"
        "    This fragment outputs a camera-relative (screen-space)\n"
        "    normal in RG, and a 16 bit depth value in BA.\n"
        "    ");
    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float4");
        pkRes->SetVariable("Position");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetVariable("WorldNormal");
        pkRes->SetDefaultValue("(0.0f,0.0f,1.0f)");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float4x4");
        pkRes->SetVariable("ViewMatrix");
        pkRes->SetSemantic("ViewMatrix");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float4");
        pkRes->SetVariable("DepthScale");
        pkRes->SetSemantic("DepthScale");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float");
        pkRes->SetVariable("SpecularPower");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float4");
        pkRes->SetVariable("Output");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert a code block
    {
        NiCodeBlock* pkBlock = NiNew NiCodeBlock();

        pkBlock->SetLanguage("hlsl/Cg");
        pkBlock->SetPlatform("D3D11/D3D10/DX9/Xenon/PS3");
        pkBlock->SetTarget("vs_2_0/ps_2_0/vs_4_0/ps_4_0/vs_5_0/ps_5_0");

        pkBlock->SetText("\n"
             "    \n"
             "    // calculate the depth value to store\n"
             "    Position = mul(Position, ViewMatrix);\n"
             "    float z = abs(Position.z);\n"
             "    \n"
             "    // Pack the depth\n"
             "    z = (z / DepthScale.x) - DepthScale.y;\n"
             "    z = min(z, 255.0f); // clamp to representable range\n"
             "    float msb; // most significant byte\n"
             "    float lsb = modf(z, msb); // least significant byte\n"
             "    msb /= 256.0f; // scale to 0-1 range\n"
             "    \n"
             "    // calculate screen space normal\n"
             "    float p = sqrt(WorldNormal.z * 8 + 8);\n"
             "    float2 scaledNorm = WorldNormal.xy / p + 0.5;\n"
             "    \n"
             "    // output RG=normal, BA=depth\n"
             "    Output = float4(scaledNorm.xy, msb, lsb);\n"
             "    \n"
             "    Output.xy = scaledNorm.xy;\n"
             "    Output.z = SpecularPower / 255;\n"
             "    Output.w = z / 255;;\n"
             "    \n"
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
    pkFrag->SetName("PositionPassThrough");
    pkFrag->SetDescription("\n"
        "    This fragment takes in a position and passes it out unmodified.\n"
        "    ");
    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float4");
        pkRes->SetVariable("Input");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float4");
        pkRes->SetVariable("Output");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert a code block
    {
        NiCodeBlock* pkBlock = NiNew NiCodeBlock();

        pkBlock->SetLanguage("hlsl/Cg");
        pkBlock->SetPlatform("D3D11/D3D10/DX9/Xenon/PS3");
        pkBlock->SetTarget("vs_2_0/ps_2_0/vs_4_0/ps_4_0/vs_5_0/ps_5_0");

        pkBlock->SetText("\n"
             "    \n"
             "    Output = Input;\n"
             "    \n"
             "    ");


        pkFrag->AddCodeBlock(pkBlock);
    }

    pkLib->AddNode(pkFrag);
}
//--------------------------------------------------------------------------------------------------
EE_NOINLINE static void CreateFragment2(NiMaterialNodeLibrary* pkLib)
{
    NiMaterialFragmentNode* pkFrag = NiNew NiMaterialFragmentNode();

    pkFrag->SetType("Vertex/Pixel");
    pkFrag->SetName("ReconstructLightingFromTexture");
    pkFrag->SetDescription("\n"
        "    This fragment samples the light accumulation buffer for\n"
        "    diffuse color and specular intensity.\n"
        "    ");
    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float2");
        pkRes->SetVariable("ScreenUV");
        pkRes->SetDefaultValue("(0.0f,0.0f)");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("sampler2D");
        pkRes->SetVariable("SamplerID");
        pkRes->SetSemantic("Texture");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float");
        pkRes->SetVariable("SpecPower");
        pkRes->SetDefaultValue("(1.0f)");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetVariable("OutputDiffuse");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetVariable("OutputSpecular");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert a code block
    {
        NiCodeBlock* pkBlock = NiNew NiCodeBlock();

        pkBlock->SetLanguage("hlsl/Cg");
        pkBlock->SetPlatform("D3D11/D3D10/DX9/Xenon/PS3");
        pkBlock->SetTarget("vs_2_0/ps_2_0/vs_4_0/ps_4_0/vs_5_0/ps_5_0");

        pkBlock->SetText("\n"
             "    \n"
             "    float4 data = tex2D(SamplerID, ScreenUV.xy);\n"
             "    OutputDiffuse = data.xyz;\n"
             "    // approximate specular coefficient and exponentiate\n"
             "    float specIn = data.w;\n"
             "    float approxAtten = (data.x + data.y + data.z + 0.003f) / 3.0f;\n"
             "    float specCoeff = saturate(specIn);\n"
             "    float specOut = pow(specCoeff,SpecPower);\n"
             "    OutputSpecular = (OutputDiffuse) * specIn;\n"
             "    \n"
             "    ");


        pkFrag->AddCodeBlock(pkBlock);
    }

    pkLib->AddNode(pkFrag);
}
//--------------------------------------------------------------------------------------------------
EE_NOINLINE static void CreateFragment3(NiMaterialNodeLibrary* pkLib)
{
    NiMaterialFragmentNode* pkFrag = NiNew NiMaterialFragmentNode();

    pkFrag->SetType("Pixel");
    pkFrag->SetName("LPPScreenUV");
    pkFrag->SetDescription("\n"
        "    Takes the projected position output from \n"
        "    the vertex shader, and constructs the screen position from it.\n"
        "    ");
    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float4");
        pkRes->SetVariable("Input");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float4");
        pkRes->SetVariable("DepthScale");
        pkRes->SetSemantic("DepthScale");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float2");
        pkRes->SetVariable("Output");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert a code block
    {
        NiCodeBlock* pkBlock = NiNew NiCodeBlock();

        pkBlock->SetLanguage("hlsl/Cg");
        pkBlock->SetPlatform("D3D11/D3D10/DX9/Xenon");
        pkBlock->SetTarget("ps_3_0/ps_4_0/ps_5_0");

        pkBlock->SetText("\n"
             "    \n"
             "    float2 NDC = Input.xy / Input.w;\n"
             "\n"
             "    // Y texcoord is inverted except on PS3\n"
             "    float2 TexCoord = (float2(NDC.x, -NDC.y) * 0.5f) + float2(0.5f, 0.5f);\n"
             "    TexCoord += DepthScale.zw; // PS3 half pixel offset\n"
             "\n"
             "    Output = TexCoord.xy;\n"
             "    \n"
             "    ");


        pkFrag->AddCodeBlock(pkBlock);
    }

    // Insert a code block
    {
        NiCodeBlock* pkBlock = NiNew NiCodeBlock();

        pkBlock->SetLanguage("hlsl/Cg");
        pkBlock->SetPlatform("PS3");
        pkBlock->SetTarget("ps_3_0/ps_4_0/ps_5_0");

        pkBlock->SetText("\n"
             "    \n"
             "    float2 NDC = Input.xy / Input.w;\n"
             "\n"
             "    // Y texcoord is inverted except on PS3\n"
             "    float2 TexCoord = (float2(NDC.x, NDC.y) * 0.5f) + float2(0.5f, 0.5f);\n"
             "\n"
             "    Output = TexCoord.xy;\n"
             "    \n"
             "    ");


        pkFrag->AddCodeBlock(pkBlock);
    }

    pkLib->AddNode(pkFrag);
}
//--------------------------------------------------------------------------------------------------
EE_NOINLINE static void CreateFragment4(NiMaterialNodeLibrary* pkLib)
{
    NiMaterialFragmentNode* pkFrag = NiNew NiMaterialFragmentNode();

    pkFrag->SetType("Pixel");
    pkFrag->SetName("LPPNormal");
    pkFrag->SetDescription("\n"
        "    Calculates the normal from the GBuffer fragment.\n"
        "    ");
    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float4");
        pkRes->SetVariable("GBuffer");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetVariable("CamR");
        pkRes->SetSemantic("CamR");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetVariable("CamU");
        pkRes->SetSemantic("CamU");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetVariable("CamD");
        pkRes->SetSemantic("CamD");

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
        pkBlock->SetTarget("ps_3_0/ps_4_0/ps_5_0");

        pkBlock->SetText("\n"
             "    \n"
             "    float4 s = GBuffer;\n"
             "    \n"
             "    float2 fenc = s.xy*4-2;\n"
             "    float f = dot(fenc,fenc);\n"
             "    float g = sqrt(1-f/4);\n"
             "    Output.xy = fenc*g;\n"
             "    Output.z = 1-f/2;\n"
             "    \n"
             "    ");


        pkFrag->AddCodeBlock(pkBlock);
    }

    pkLib->AddNode(pkFrag);
}
//--------------------------------------------------------------------------------------------------
EE_NOINLINE static void CreateFragment5(NiMaterialNodeLibrary* pkLib)
{
    NiMaterialFragmentNode* pkFrag = NiNew NiMaterialFragmentNode();

    pkFrag->SetType("Pixel");
    pkFrag->SetName("LPPPos");
    pkFrag->SetDescription("\n"
        "    Calculates the position from the GBuffer fragment.\n"
        "    ");
    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float4");
        pkRes->SetVariable("GBuffer");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float2");
        pkRes->SetVariable("ScreenUV");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float2");
        pkRes->SetVariable("ProjectionSwitch");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float4x4");
        pkRes->SetVariable("InvViewMatrix");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float4");
        pkRes->SetVariable("DepthScale");
        pkRes->SetSemantic("DepthScale");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float2");
        pkRes->SetVariable("PosScale");
        pkRes->SetSemantic("PosScale");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetVariable("CamPos");
        pkRes->SetSemantic("CamPos");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float4");
        pkRes->SetVariable("Output");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetVariable("WorldView");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float4");
        pkRes->SetVariable("SpecularPower");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert a code block
    {
        NiCodeBlock* pkBlock = NiNew NiCodeBlock();

        pkBlock->SetLanguage("hlsl/Cg");
        pkBlock->SetPlatform("D3D11/D3D10/DX9/Xenon/PS3");
        pkBlock->SetTarget("ps_3_0/ps_4_0/ps_5_0");

        pkBlock->SetText("\n"
             "\n"
             "        float4 s = GBuffer;\n"
             "\n"
             "        // ------------------------------------\n"
             "        // Decode depth from 2 channels (BA)\n"
             "        //float depth = ((255.0f * s.z) + (s.w));\n"
             "\n"
             "        // Decode depth from 1 channel (A)\n"
             "        float depth = s.w * 255;\n"
             "\n"
             "        // Scale depth from 0->1 to 0->FarPlane\n"
             "        depth = (depth + DepthScale.y) * DepthScale.x;\n"
             "\n"
             "        // ------------------------------------\n"
             "        // Calculate the view position from projected pos\n"
             "\n"
             "        float2 coord = ScreenUV + float2(-0.5f, -0.5f);\n"
             "\n"
             "        // Select between orthographic and perspective\n"
             "        float perspectiveCorrection = dot(float2(depth, 1), ProjectionSwitch);\n"
             "\n"
             "        float3 viewPos = float3(\n"
             "            perspectiveCorrection * PosScale.x * coord.x,\n"
             "            -perspectiveCorrection * PosScale.y * coord.y,\n"
             "            depth);\n"
             "        \n"
             "        Output.xyz = CamPos + mul(viewPos, InvViewMatrix);\n"
             "        Output.w = 1.0f;\n"
             "\n"
             "        // Calculate world view vector\n"
             "        WorldView = normalize(CamPos - Output);\n"
             "\n"
             "        // ------------------------------------\n"
             "        // Decode specular power (B channel)\n"
             "        SpecularPower = float4(s.z, s.z, s.z, s.z) * 255;\n"
             "\n"
             "    ");


        pkFrag->AddCodeBlock(pkBlock);
    }

    pkLib->AddNode(pkFrag);
}
//--------------------------------------------------------------------------------------------------
EE_NOINLINE static void CreateFragment6(NiMaterialNodeLibrary* pkLib)
{
    NiMaterialFragmentNode* pkFrag = NiNew NiMaterialFragmentNode();

    pkFrag->SetType("Pixel");
    pkFrag->SetName("LPPFinal");
    pkFrag->SetDescription("\n"
        "    Composites the light calculation and produces the final output.\n"
        "    ");
    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetVariable("Diffuse");
        pkRes->SetDefaultValue("(0.0f,0.0f,0.0f)");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetVariable("Specular");
        pkRes->SetDefaultValue("(0.0f,0.0f,0.0f)");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetVariable("Ambient");
        pkRes->SetDefaultValue("(0.0f, 0.0f, 0.0f)");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float");
        pkRes->SetVariable("Atten");
        pkRes->SetDefaultValue("(1.0f)");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float4");
        pkRes->SetVariable("Output");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert a code block
    {
        NiCodeBlock* pkBlock = NiNew NiCodeBlock();

        pkBlock->SetLanguage("hlsl/Cg");
        pkBlock->SetPlatform("D3D11/D3D10/DX9/Xenon/PS3");
        pkBlock->SetTarget("ps_3_0/ps_4_0/ps_5_0");

        pkBlock->SetText("\n"
             "\n"
             "    Output.rgb = Diffuse + Ambient;\n"
             "    Output.w = 0.333 * (Specular.r + Specular.g + Specular.b);\n"
             "    Output *= Atten;\n"
             "\n"
             "    ");


        pkFrag->AddCodeBlock(pkBlock);
    }

    pkLib->AddNode(pkFrag);
}
//--------------------------------------------------------------------------------------------------
NiMaterialNodeLibrary* 
    NiLightPrePassMaterialNodeLibrary::CreateMaterialNodeLibrary()
{
    // Create a new NiMaterialNodeLibrary
    NiMaterialNodeLibrary* pkLib = NiNew NiMaterialNodeLibrary(4);

    CreateFragment0(pkLib);
    CreateFragment1(pkLib);
    CreateFragment2(pkLib);
    CreateFragment3(pkLib);
    CreateFragment4(pkLib);
    CreateFragment5(pkLib);
    CreateFragment6(pkLib);

    return pkLib;
}
//--------------------------------------------------------------------------------------------------

