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
#include "NiAtmosphereMaterialNodeLibrary.h"

//--------------------------------------------------------------------------------------------------
EE_NOINLINE static void CreateFragment0(NiMaterialNodeLibrary* pkLib)
{
    NiMaterialFragmentNode* pkFrag = NiNew NiMaterialFragmentNode();

    pkFrag->SetType("Vertex/Pixel");
    pkFrag->SetName("SplitAtmosphericScatteringConsts");
    pkFrag->SetDescription("\n"
        "            This fragment is responsible for splitting the data stored in\n"
        "            the atmospheric scattering global constant\n"
        "        ");
    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float4");
        pkRes->SetVariable("scAtmosphericScatteringConsts");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float");
        pkRes->SetVariable("fKm4PI");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float");
        pkRes->SetVariable("fKr4PI");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float");
        pkRes->SetVariable("fKmESun");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float");
        pkRes->SetVariable("fKrESun");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert a code block
    {
        NiCodeBlock* pkBlock = NiNew NiCodeBlock();

        pkBlock->SetLanguage("hlsl/Cg");
        pkBlock->SetPlatform("D3D11/D3D10/DX9/Xenon/PS3");
        pkBlock->SetTarget("vs_1_1/ps_2_0/vs_3_0/ps_3_0/vs_4_0/ps_4_0/vs_5_0/ps_5_0");

        pkBlock->SetText("\n"
             "            fKrESun = scAtmosphericScatteringConsts.x;\n"
             "            fKmESun = scAtmosphericScatteringConsts.y;\n"
             "            fKr4PI = scAtmosphericScatteringConsts.z;\n"
             "            fKm4PI = scAtmosphericScatteringConsts.w;\n"
             "        ");


        pkFrag->AddCodeBlock(pkBlock);
    }

    pkLib->AddNode(pkFrag);
}
//--------------------------------------------------------------------------------------------------
EE_NOINLINE static void CreateFragment1(NiMaterialNodeLibrary* pkLib)
{
    NiMaterialFragmentNode* pkFrag = NiNew NiMaterialFragmentNode();

    pkFrag->SetType("Vertex/Pixel");
    pkFrag->SetName("SplitPlanetDimensions");
    pkFrag->SetDescription("\n"
        "            This fragment is responsible for extracting the data out of the \n"
        "            planet dimensions shader constant\n"
        "        ");
    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float4");
        pkRes->SetVariable("scPlanetDimensions");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float");
        pkRes->SetVariable("fOuterRadius");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float");
        pkRes->SetVariable("fOuterRadius2");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float");
        pkRes->SetVariable("fInnerRadius");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float");
        pkRes->SetVariable("fInnerRadius2");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert a code block
    {
        NiCodeBlock* pkBlock = NiNew NiCodeBlock();

        pkBlock->SetLanguage("hlsl/Cg");
        pkBlock->SetPlatform("D3D11/D3D10/DX9/Xenon/PS3");
        pkBlock->SetTarget("vs_1_1/ps_2_0/vs_3_0/ps_3_0/vs_4_0/ps_4_0/vs_5_0/ps_5_0");

        pkBlock->SetText("\n"
             "            fOuterRadius = scPlanetDimensions.x;\n"
             "            fOuterRadius2 = scPlanetDimensions.y;\n"
             "            fInnerRadius = scPlanetDimensions.z;\n"
             "            fInnerRadius2 = scPlanetDimensions.w;\n"
             "        ");


        pkFrag->AddCodeBlock(pkBlock);
    }

    pkLib->AddNode(pkFrag);
}
//--------------------------------------------------------------------------------------------------
EE_NOINLINE static void CreateFragment2(NiMaterialNodeLibrary* pkLib)
{
    NiMaterialFragmentNode* pkFrag = NiNew NiMaterialFragmentNode();

    pkFrag->SetType("Vertex/Pixel");
    pkFrag->SetName("SplitAtmosphericScaleDepth");
    pkFrag->SetDescription("\n"
        "            This fragment is responsible for extracting the information from\n"
        "            the atmospheric scale depth shader constant\n"
        "        ");
    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float4");
        pkRes->SetVariable("scAtmosphericScaleDepth");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float");
        pkRes->SetVariable("fScale");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float");
        pkRes->SetVariable("fScaleDepth");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float");
        pkRes->SetVariable("fScaleOverScaleDepth");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float");
        pkRes->SetVariable("fSunSizeMultiplier");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert a code block
    {
        NiCodeBlock* pkBlock = NiNew NiCodeBlock();

        pkBlock->SetLanguage("hlsl/Cg");
        pkBlock->SetPlatform("D3D11/D3D10/DX9/Xenon/PS3");
        pkBlock->SetTarget("vs_1_1/ps_2_0/vs_3_0/ps_3_0/vs_4_0/ps_4_0/vs_5_0/ps_5_0");

        pkBlock->SetText("\n"
             "            fScale = scAtmosphericScaleDepth.x;\n"
             "            fScaleDepth = scAtmosphericScaleDepth.y;\n"
             "            fScaleOverScaleDepth = scAtmosphericScaleDepth.z;\n"
             "            fSunSizeMultiplier = scAtmosphericScaleDepth.w;\n"
             "        ");


        pkFrag->AddCodeBlock(pkBlock);
    }

    pkLib->AddNode(pkFrag);
}
//--------------------------------------------------------------------------------------------------
EE_NOINLINE static void CreateFragment3(NiMaterialNodeLibrary* pkLib)
{
    NiMaterialFragmentNode* pkFrag = NiNew NiMaterialFragmentNode();

    pkFrag->SetType("Vertex/Pixel");
    pkFrag->SetName("SplitFrameData");
    pkFrag->SetDescription("\n"
        "            This fragment is responsible for extracting the frame data from\n"
        "            the frame data shader constant\n"
        "        ");
    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float4");
        pkRes->SetVariable("scFrameData");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float");
        pkRes->SetVariable("fPhaseConstant");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float");
        pkRes->SetVariable("fPhaseConstant2");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float");
        pkRes->SetVariable("fCameraHeight");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float");
        pkRes->SetVariable("fCameraHeight2");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert a code block
    {
        NiCodeBlock* pkBlock = NiNew NiCodeBlock();

        pkBlock->SetLanguage("hlsl/Cg");
        pkBlock->SetPlatform("D3D11/D3D10/DX9/Xenon/PS3");
        pkBlock->SetTarget("vs_1_1/ps_2_0/vs_3_0/ps_3_0/vs_4_0/ps_4_0/vs_5_0/ps_5_0");

        pkBlock->SetText("\n"
             "            fPhaseConstant = scFrameData.x;\n"
             "            fPhaseConstant2 = scFrameData.y;\n"
             "            fCameraHeight = scFrameData.z;\n"
             "            fCameraHeight2 = scFrameData.w;\n"
             "        ");


        pkFrag->AddCodeBlock(pkBlock);
    }

    pkLib->AddNode(pkFrag);
}
//--------------------------------------------------------------------------------------------------
EE_NOINLINE static void CreateFragment4(NiMaterialNodeLibrary* pkLib)
{
    NiMaterialFragmentNode* pkFrag = NiNew NiMaterialFragmentNode();

    pkFrag->SetType("Pixel");
    pkFrag->SetName("CalcPathThroughAtmosphere");
    pkFrag->SetDescription("\n"
        "            This fragment is responsible for calculating the path travelled by\n"
        "            rays of light through the atmosphere.\n"
        "        ");
    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetVariable("WorldView");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetVariable("upVector");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float");
        pkRes->SetVariable("RA");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float");
        pkRes->SetVariable("RA2");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float");
        pkRes->SetVariable("RE");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float");
        pkRes->SetVariable("RE2");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetVariable("CameraPos");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float");
        pkRes->SetVariable("fCameraHeight");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float");
        pkRes->SetVariable("fFar");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float");
        pkRes->SetVariable("fNear");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert a code block
    {
        NiCodeBlock* pkBlock = NiNew NiCodeBlock();

        pkBlock->SetLanguage("hlsl/Cg");
        pkBlock->SetPlatform("D3D11/D3D10/DX9/Xenon/PS3");
        pkBlock->SetTarget("vs_1_1/ps_2_0/vs_3_0/ps_3_0/vs_4_0/ps_4_0/vs_5_0/ps_5_0");

        pkBlock->SetText("\n"
             "            // Convert our planar representation into a celestial/spherical\n"
             "            // representation (calculate the far point of the ray passing\n"
             "            // through the atmosphere)\n"
             "            // RE = radius of the planet\n"
             "            // RA = radius of the atmosphere\n"
             "            // cosTheta = the view direction with respect to directly \"up\"\n"
             "            // Uses the Cosine Rule to calculate the required distance\n"
             "            float cosTheta = dot(WorldView, -upVector);\n"
             "            fFar = ((2 * RE * cosTheta) + sqrt( pow((-2 * RE * cosTheta),2) - (4 "
             "* (RE2 - RA2))))/2;\n"
             "\n"
             "            // Assume camera is \"in\" the atmosphere\n"
             "            fNear = 0.0f;\n"
             "\n"
             "            // for atmosphere's sake, position can be a single position on\n"
             "            // the planet, and the sun will orbit over it\n"
             "            fCameraHeight = RE;\n"
             "            CameraPos = upVector * fCameraHeight;\n"
             "        ");


        pkFrag->AddCodeBlock(pkBlock);
    }

    pkLib->AddNode(pkFrag);
}
//--------------------------------------------------------------------------------------------------
EE_NOINLINE static void CreateFragment5(NiMaterialNodeLibrary* pkLib)
{
    NiMaterialFragmentNode* pkFrag = NiNew NiMaterialFragmentNode();

    pkFrag->SetType("Vertex/Pixel");
    pkFrag->SetName("CalcAtmosphericScatteringValues");
    pkFrag->SetDescription("\n"
        "            This fragment is responsible for calculating the scattering amounts for\n"
        "            the RGB wavelengths along a particular path through the atmosphere\n"
        "            \n"
        "            This code is based on the code from GPU Gems 2, Chapter 16, Accurate Atmos"
        "pheric Scattering by Sean O'Neil. \n"
        "            The licence for this code is below:\n"
        "            \n"
        "            s_p_oneil@hotmail.com\n"
        "            Copyright (c) 2000, Sean O'Neil\n"
        "            All rights reserved.\n"
        "            \n"
        "            Redistribution and use in source and binary forms, with or without\n"
        "            modification, are permitted provided that the following conditions are met"
        ":\n"
        "            \n"
        "            * Redistributions of source code must retain the above copyright notice,\n"
        "              this list of conditions and the following disclaimer.\n"
        "            * Redistributions in binary form must reproduce the above copyright notice"
        ",\n"
        "              this list of conditions and the following disclaimer in the documentatio"
        "n\n"
        "              and/or other materials provided with the distribution.\n"
        "            * Neither the name of this project nor the names of its contributors\n"
        "              may be used to endorse or promote products derived from this software\n"
        "              without specific prior written permission.\n"
        "            \n"
        "            THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS \"AS I"
        "S\"\n"
        "            AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE\n"
        "            IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE"
        "\n"
        "            ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE\n"
        "            LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR\n"
        "            CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF\n"
        "            SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS\n"
        "            INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN\n"
        "            CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)\n"
        "            ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE"
        "\n"
        "            POSSIBILITY OF SUCH DAMAGE.\n"
        "            \n"
        "        ");
    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetVariable("CameraPos");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetVariable("WorldView");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float");
        pkRes->SetVariable("fNear");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float");
        pkRes->SetVariable("fFar");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float");
        pkRes->SetVariable("fG");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float");
        pkRes->SetVariable("fG2");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float");
        pkRes->SetVariable("fCameraHeight");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float");
        pkRes->SetVariable("fCameraHeight2");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float");
        pkRes->SetVariable("fScale");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float");
        pkRes->SetVariable("fScaleDepth");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float");
        pkRes->SetVariable("fScaleOverScaleDepth");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float");
        pkRes->SetVariable("fSunSizeMultiplier");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float");
        pkRes->SetVariable("fOuterRadius");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float");
        pkRes->SetVariable("fOuterRadius2");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float");
        pkRes->SetVariable("fInnerRadius");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float");
        pkRes->SetVariable("fInnerRadius2");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float");
        pkRes->SetVariable("fKm4PI");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float");
        pkRes->SetVariable("fKr4PI");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float");
        pkRes->SetVariable("fKmESun");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float");
        pkRes->SetVariable("fKrESun");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("int");
        pkRes->SetVariable("nSamples");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float");
        pkRes->SetVariable("fSamples");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetVariable("RGBInvWavelength4");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetVariable("SunDirection");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float");
        pkRes->SetVariable("FakeDepthValue");
        pkRes->SetDefaultValue("(0)");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetVariable("RayleighRGBScattering");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetVariable("MieRGBScattering");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert a code block
    {
        NiCodeBlock* pkBlock = NiNew NiCodeBlock();

        pkBlock->SetLanguage("hlsl/Cg");
        pkBlock->SetPlatform("D3D11/D3D10/DX9/Xenon/PS3");
        pkBlock->SetTarget("vs_1_1/ps_2_0/vs_3_0/ps_3_0/vs_4_0/ps_4_0/vs_5_0/ps_5_0");

        pkBlock->SetText("\n"
             "            float3 posScatterStart = CameraPos + WorldView * fNear;\n"
             "            float fHeight = length(posScatterStart);\n"
             "            float fStartAngle = dot(WorldView, posScatterStart) / fHeight;\n"
             "            \n"
             "            float fStartOffset;\n"
             "            CalcOpticalDepth(fStartAngle, fScaleDepth, fScaleOverScaleDepth, (fIn"
             "nerRadius - fCameraHeight), fStartOffset);\n"
             "\n"
             "            // Initialize the scattering loop variables\n"
             "            float fSampleLength = fFar / fSamples;\n"
             "            float fScaledLength = fSampleLength * fScale;\n"
             "            float3 dirSample = WorldView * fSampleLength;\n"
             "            float3 posSample = posScatterStart + dirSample * 0.5;\n"
             "\n"
             "            // Now loop through the sample rays\n"
             "            float3 colScatter = float3(0.0, 0.0, 0.0);\n"
             "            for(int i=0; i < 5; i++)\n"
             "            {\n"
             "                float fSampleLength = length(posSample);\n"
             "                float fHeightAboveSurface = fInnerRadius - fSampleLength;\n"
             "                \n"
             "                float fLightAngle = dot(-SunDirection, posSample) / fSampleLength"
             ";\n"
             "                float fLightDepth;\n"
             "                CalcOpticalDepth(fLightAngle, fScaleDepth, fScaleOverScaleDepth, "
             "fHeightAboveSurface, fLightDepth);\n"
             "\n"
             "                float fCameraAngle = dot(WorldView, posSample) / fSampleLength;\n"
             "                float fCameraDepth;\n"
             "                CalcOpticalDepth(fCameraAngle, fScaleDepth, fScaleOverScaleDepth,"
             " fHeightAboveSurface, fCameraDepth);\n"
             "                     \n"
             "                float fScatter = (fStartOffset + (fLightDepth - fCameraDepth));\n"
             "                float3 colAttenuate = exp(-fScatter * (RGBInvWavelength4 * fKr4PI"
             " + fKm4PI));\n"
             "                \n"
             "                float fDepth = exp(fScaleOverScaleDepth * fHeightAboveSurface);\n"
             "                colScatter += colAttenuate * (fDepth * fScaledLength);\n"
             "                posSample += dirSample;\n"
             "            }\n"
             "\n"
             "            // Finally, scale the Mie and Rayleigh colors and set up\n"
             "            // the varying variables for the pixel shader\n"
             "            MieRGBScattering.xyz = colScatter * fKmESun;\n"
             "            RayleighRGBScattering.xyz = colScatter * (RGBInvWavelength4 * fKrESun"
             ");\n"
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
    pkFrag->SetName("CalcAtmosphericColor");
    pkFrag->SetDescription("\n"
        "            This fragment is responsible for calculating the atmospheric color\n"
        "            in a particular direction based on the scattering values previously\n"
        "            calculated\n"
        "        ");
    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetVariable("WorldView");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetVariable("SunDirection");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetVariable("RayleighRGBScattering");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetVariable("MieRGBScattering");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float");
        pkRes->SetVariable("g");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float");
        pkRes->SetVariable("g2");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float");
        pkRes->SetVariable("fSunSizeMultiplier");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetVariable("result");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert a code block
    {
        NiCodeBlock* pkBlock = NiNew NiCodeBlock();

        pkBlock->SetLanguage("hlsl/Cg");
        pkBlock->SetPlatform("D3D11/D3D10/DX9/Xenon/PS3");
        pkBlock->SetTarget("vs_1_1/ps_2_0/vs_3_0/ps_3_0/vs_4_0/ps_4_0/vs_5_0/ps_5_0");

        pkBlock->SetText("\n"
             "            float fCos = dot(SunDirection, WorldView) / length(WorldView);\n"
             "\n"
             "            float fRayleighPhase = 0.75 * (1.0 + 1* fCos*fCos);\n"
             "            float fMiePhase = 1.5 * ((1.0 - g2) / (2.0 + g2)) * (fSunSizeMultipli"
             "er * fCos*fCos) / pow(1.0 + g2 - 2.0*g*fCos, 1.5);\n"
             "\n"
             "            result = fRayleighPhase * RayleighRGBScattering + fMiePhase * MieRGBS"
             "cattering;\n"
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
    pkFrag->SetName("HDRToneMapping");
    pkFrag->SetDescription("\n"
        "            This fragment is responsible for mapping the dynamic colors of the\n"
        "            atmosphere into the normal color range.\n"
        "        ");
    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float3");
        pkRes->SetVariable("inColor");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float");
        pkRes->SetVariable("HDRExposure");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float4");
        pkRes->SetVariable("outColor");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert a code block
    {
        NiCodeBlock* pkBlock = NiNew NiCodeBlock();

        pkBlock->SetLanguage("hlsl/Cg");
        pkBlock->SetPlatform("D3D11/D3D10/DX9/Xenon/PS3");
        pkBlock->SetTarget("vs_1_1/ps_2_0/vs_3_0/ps_3_0/vs_4_0/ps_4_0/vs_5_0/ps_5_0");

        pkBlock->SetText("\n"
             "            outColor.rgb = 1.0f - exp(inColor * -g_HDRExposure);\n"
             "            outColor.a = 1.0f;\n"
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
    pkFrag->SetName("CalcOpticalDepth");
    pkFrag->SetDescription("\n"
        "            This fragment is responsible for calculating value for \n"
        "            the optical depth in a certain direction from a certain height \n"
        "            above the surface of a planet.\n"
        "        ");
    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float");
        pkRes->SetVariable("fCos");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float");
        pkRes->SetVariable("fScaleDepth");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float");
        pkRes->SetVariable("fScaleOverScaleDepth");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an input resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float");
        pkRes->SetVariable("fHeightFromSurface");

        pkFrag->AddInputResource(pkRes);
    }

    // Insert an output resource
    {
        NiMaterialResource* pkRes = NiNew NiMaterialResource();

        pkRes->SetType("float");
        pkRes->SetVariable("depth");

        pkFrag->AddOutputResource(pkRes);
    }

    // Insert a code block
    {
        NiCodeBlock* pkBlock = NiNew NiCodeBlock();

        pkBlock->SetLanguage("hlsl/Cg");
        pkBlock->SetPlatform("D3D11/D3D10/DX9/Xenon/PS3");
        pkBlock->SetTarget("vs_1_1/ps_2_0/vs_3_0/ps_3_0/vs_4_0/ps_4_0/vs_5_0/ps_5_0");

        pkBlock->SetText("\n"
             "            float x = 1.0 - fCos;\n"
             "            depth = exp(fScaleOverScaleDepth * fHeightFromSurface);\n"
             "            depth *= fScaleDepth * exp(-0.00287 + x*(0.459 + x*(3.83 + x*(-6.80 +"
             " x*5.25))));\n"
             "        ");


        pkFrag->AddCodeBlock(pkBlock);
    }

    pkLib->AddNode(pkFrag);
}
//--------------------------------------------------------------------------------------------------
NiMaterialNodeLibrary* 
    NiAtmosphereMaterialNodeLibrary::CreateMaterialNodeLibrary()
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

    return pkLib;
}
//--------------------------------------------------------------------------------------------------

