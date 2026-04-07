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
#include "NiTerrainPCH.h"

#include "NiFragmentOperations.h"
#include "NiStandardMaterialNodeLibrary.h"
#include "NiFragmentOperationsNodeLibrary.h"

//--------------------------------------------------------------------------------------------------
NiImplementRTTI(NiFragmentOperations, NiFragment);
//--------------------------------------------------------------------------------------------------
NiFragmentOperations::NiFragmentOperations():
    NiFragment(VERTEX_VERSION,GEOMETRY_VERSION,PIXEL_VERSION)
{
    // Append the required node libraries
    m_kLibraries.Add(
        NiFragmentOperationsNodeLibrary::CreateMaterialNodeLibrary());
    m_kLibraries.Add(
        NiStandardMaterialNodeLibrary::CreateMaterialNodeLibrary());
}

//--------------------------------------------------------------------------------------------------
bool NiFragmentOperations::HandleAlphaTest(
    Context& kContext,
    bool bAlphaTest,
    NiMaterialResource* pkAlphaTestInput,
    bool bForceTest)
{
	return HandleAlphaTest(kContext, bAlphaTest, pkAlphaTestInput, NULL, NULL, bForceTest);
}

//--------------------------------------------------------------------------------------------------
bool NiFragmentOperations::HandleAlphaTest(Context& kContext, 
	bool bAlphaTest, 
	NiMaterialResource* pkAlphaTestInput, 
	NiMaterialResource* pkTestFunction, 
	NiMaterialResource* pkTestRef, 
	bool bForceTest)
{
	NiFixedString kPlatform = kContext.m_spConfigurator->GetPlatformString();
	// D3D10 and D3D11 require alpha testing in the pixel shader
	if (kPlatform != "D3D10" && kPlatform != "D3D11" && !bForceTest)
	{
		return true;
	}

	if (bAlphaTest)
	{
		NiMaterialNode* pkAlphaTestNode =
			GetAttachableNodeFromLibrary("ApplyAlphaTest");
		kContext.m_spConfigurator->AddNode(pkAlphaTestNode);

		if (!pkTestFunction)
		{
			pkTestFunction = AddOutputPredefined(
				kContext.m_spUniforms,
				NiShaderConstantMap::SCM_DEF_ALPHA_TEST_FUNC);
		}
		
		kContext.m_spConfigurator->AddBinding(pkTestFunction,
			pkAlphaTestNode->GetInputResourceByVariableName(
			"AlphaTestFunction"));

		if (!pkTestRef)
		{
			pkTestRef = AddOutputPredefined(
				kContext.m_spUniforms,
				NiShaderConstantMap::SCM_DEF_ALPHA_TEST_REF);
		}

		kContext.m_spConfigurator->AddBinding(pkTestRef,
			pkAlphaTestNode->GetInputResourceByVariableName(
			"AlphaTestRef"));

		kContext.m_spConfigurator->AddBinding(pkAlphaTestInput,
			pkAlphaTestNode->GetInputResourceByVariableName(
			"AlphaTestValue"));
	}

	return true;
	
}

//--------------------------------------------------------------------------------------------------
bool NiFragmentOperations::NormalizeVector(
    Context& kContext,
    NiMaterialResource*& pkVector)
{
    NiMaterialNode* pkNode = NULL;
    if (pkVector && pkVector->GetType() == "float4")
    {
        pkNode = GetAttachableNodeFromLibrary(
            "NormalizeFloat4");
    }
    else if (pkVector && pkVector->GetType() == "float3")
    {
        pkNode = GetAttachableNodeFromLibrary(
            "NormalizeFloat3");
    }
    else if (pkVector && pkVector->GetType() == "float2")
    {
        pkNode = GetAttachableNodeFromLibrary(
            "NormalizeFloat2");
    }

    if (!pkNode)
    {
        EE_FAIL("Error in fragment");
        return false;
    }

    kContext.m_spConfigurator->AddNode(pkNode);

    kContext.m_spConfigurator->AddBinding(pkVector,
        pkNode->GetInputResourceByVariableName("VectorIn"));

    pkVector =  pkNode->GetOutputResourceByVariableName("VectorOut");
    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiFragmentOperations::ColorToLuminance(Context& kContext, NiMaterialResource* pkColor, 
    NiMaterialResource*& pkLuminance)
{
    NiMaterialNode* pkNode = GetAttachableNodeFromLibrary("ColorToLuminance");
    EE_ASSERT(pkNode);
    kContext.m_spConfigurator->AddNode(pkNode);

    // Bind input
    EE_ASSERT(pkColor);
    kContext.m_spConfigurator->AddBinding(pkColor, "Color", pkNode);

    // Bind output
    pkLuminance = pkNode->GetOutputResourceByVariableName("Luma");

    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiFragmentOperations::ExtractChannel(Context& kContext, NiMaterialResource* pkVector,
    efd::UInt32 uiChannel, NiMaterialResource*& pkOutput)
{
    NiMaterialNode* pkNode = GetAttachableNodeFromLibrary("ExtractChannel");
    EE_ASSERT(pkNode);
    kContext.m_spConfigurator->AddNode(pkNode);

    // Bind input
    EE_ASSERT(pkVector);
    TypeCastBind(kContext, pkVector, pkNode->GetInputResourceByVariableName("Input"));
    NiMaterialResource* pkChannel = 
        GenerateShaderConstant(kContext, efd::SInt32(uiChannel));
    kContext.m_spConfigurator->AddBinding(pkChannel, "Channel", pkNode);

    // Bind output
    pkOutput = pkNode->GetOutputResourceByVariableName("Output");

    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiFragmentOperations::AddVector(
    Context& kContext,
    NiMaterialResource* pkVector1, NiMaterialResource* pkVector2,
    NiMaterialResource*& pkValue)
{
    NiMaterialNode* pkNode = NULL;


    if (pkVector1 && pkVector1->GetType() == "float4" && pkVector2 &&
        pkVector2->GetType() =="float4")
    {
        pkNode = GetAttachableNodeFromLibrary(
            "AddFloat4");
    }
    else if (pkVector1 && pkVector1->GetType() == "float3" && pkVector2 &&
        pkVector2->GetType() =="float3")
    {
        pkNode = GetAttachableNodeFromLibrary(
            "AddFloat3");
    }
    else if (pkVector1 && pkVector1->GetType() == "float2" && pkVector2 &&
        pkVector2->GetType() =="float2")
    {
        pkNode = GetAttachableNodeFromLibrary(
            "AddFloat2");
    }
    else if (pkVector1 && pkVector1->GetType() == "float" && pkVector2 &&
        pkVector2->GetType() =="float")
    {
        pkNode = GetAttachableNodeFromLibrary(
            "AddFloat");
    }
    else if (pkVector1)
    {
        pkValue = pkVector1;
        return true;
    }
    else if (pkVector2)
    {
        pkValue = pkVector2;
        return true;
    }

    if (!pkNode)
    {
        EE_FAIL("Error in fragment");
        return false;
    }

    kContext.m_spConfigurator->AddNode(pkNode);

    kContext.m_spConfigurator->AddBinding(pkVector1,
        pkNode->GetInputResourceByVariableName("V1"));
    kContext.m_spConfigurator->AddBinding(pkVector2,
        pkNode->GetInputResourceByVariableName("V2"));

    pkValue =  pkNode->GetOutputResourceByVariableName("Output");
    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiFragmentOperations::MultiplyVector(
    Context& kContext,
    NiMaterialResource* pkVector1, NiMaterialResource* pkVector2,
    NiMaterialResource*& pkValue)
{
    NiMaterialNode* pkNode = NULL;

    if (pkVector1 && pkVector1->GetType() == "float4" && pkVector2 &&
        pkVector2->GetType() =="float4")
    {
        pkNode = GetAttachableNodeFromLibrary(
            "MultiplyFloat4");
    }
    else if (pkVector1 && pkVector1->GetType() == "float3" && pkVector2 &&
        pkVector2->GetType() =="float3")
    {
        pkNode = GetAttachableNodeFromLibrary(
            "MultiplyFloat3");
    }
    else if (pkVector1 && pkVector1->GetType() == "float2" && pkVector2 &&
        pkVector2->GetType() =="float2")
    {
        pkNode = GetAttachableNodeFromLibrary(
            "MultiplyFloat2");
    }
    else if (pkVector1 && pkVector1->GetType() == "float" && pkVector2 &&
        pkVector2->GetType() =="float")
    {
        pkNode = GetAttachableNodeFromLibrary(
            "MultiplyFloat");
    }
    else if (pkVector1)
    {
        pkValue = pkVector1;
        return true;
    }
    else if (pkVector2)
    {
        pkValue = pkVector2;
        return true;
    }


    if (!pkNode)
    {
        EE_FAIL("Error in fragment");
        return false;
    }

    kContext.m_spConfigurator->AddNode(pkNode);

    kContext.m_spConfigurator->AddBinding(pkVector1,
        pkNode->GetInputResourceByVariableName("V1"));
    kContext.m_spConfigurator->AddBinding(pkVector2,
        pkNode->GetInputResourceByVariableName("V2"));

    pkValue = pkNode->GetOutputResourceByVariableName("Output");
    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiFragmentOperations::CrossVector(Context& kContext, NiMaterialResource* pkVector1, 
    NiMaterialResource* pkVector2, NiMaterialResource*& pkValue)
{
    NiMaterialNode* pkNode = NULL;

    if (pkVector1 && pkVector1->GetType() == "float3" && pkVector2 &&
        pkVector2->GetType() =="float3")
    {
        pkNode = GetAttachableNodeFromLibrary("CrossFloat3");
    }

    if (!pkNode)
    {
        EE_FAIL("Error in fragment");
        return false;
    }

    kContext.m_spConfigurator->AddNode(pkNode);

    kContext.m_spConfigurator->AddBinding(pkVector1,
        pkNode->GetInputResourceByVariableName("V1"));
    kContext.m_spConfigurator->AddBinding(pkVector2,
        pkNode->GetInputResourceByVariableName("V2"));

    pkValue = pkNode->GetOutputResourceByVariableName("Output");

    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiFragmentOperations::MultiplyAddVector(
    Context& kContext,
    NiMaterialResource* pkVector1, NiMaterialResource* pkVector2,
    NiMaterialResource* pkVector3, NiMaterialResource*& pkValue)
{
    NiMaterialNode* pkNode = NULL;

    if (pkVector1 && pkVector2 && pkVector3)
    {
        pkNode = GetAttachableNodeFromLibrary(
            "MultiplyAddFloat3");
    }


    if (!pkNode)
    {
        EE_FAIL("Error in fragment");
        return false;
    }

    kContext.m_spConfigurator->AddNode(pkNode);

    TypeCastBind(kContext, pkVector1,
        pkNode->GetInputResourceByVariableName("V1"));
    TypeCastBind(kContext, pkVector2,
        pkNode->GetInputResourceByVariableName("V2"));
    TypeCastBind(kContext, pkVector3,
        pkNode->GetInputResourceByVariableName("V3"));

    pkValue = pkNode->GetOutputResourceByVariableName("Output");
    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiFragmentOperations::LerpVector(
    Context& kContext,
    NiMaterialResource* pkVector1, NiMaterialResource* pkVector2,
    NiMaterialResource* pkLerpAmount, NiMaterialResource*& pkValue)
{
    NiMaterialNode* pkNode = NULL;

    if (pkVector1 && pkVector1->GetType() == "float4" && pkVector2 &&
        pkVector2->GetType() =="float4")
    {
        pkNode = GetAttachableNodeFromLibrary(
            "LerpFloat4");
    }
    else if (pkVector1 && pkVector1->GetType() == "float3" && pkVector2 &&
        pkVector2->GetType() =="float3")
    {
        pkNode = GetAttachableNodeFromLibrary(
            "LerpFloat3");
    }
    else if (pkVector1 && pkVector1->GetType() == "float2" && pkVector2 &&
        pkVector2->GetType() =="float2")
    {
        pkNode = GetAttachableNodeFromLibrary(
            "LerpFloat2");
    }
    else if (pkVector1 && pkVector1->GetType() == "float" && pkVector2 &&
        pkVector2->GetType() =="float")
    {
        pkNode = GetAttachableNodeFromLibrary(
            "LerpFloat");
    }

    if (!pkNode)
    {
        EE_FAIL("Error in fragment");
        return false;
    }

    kContext.m_spConfigurator->AddNode(pkNode);

    kContext.m_spConfigurator->AddBinding(pkVector1,
        pkNode->GetInputResourceByVariableName("V1"));
    kContext.m_spConfigurator->AddBinding(pkVector2,
        pkNode->GetInputResourceByVariableName("V2"));
    kContext.m_spConfigurator->AddBinding(pkLerpAmount,
        pkNode->GetInputResourceByVariableName("LerpAmount"));

    pkValue = pkNode->GetOutputResourceByVariableName("Output");
    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiFragmentOperations::ScaleVector(
    Context& kContext,
    NiMaterialResource* pkVector, NiMaterialResource* pkScale,
    NiMaterialResource*& pkValue)
{
    NiMaterialNode* pkNode = NULL;
    const char* pcScaleInputName = "Scale";

    if (!pkScale && pkVector)
    {
        pkValue = pkVector;
        return true;
    }
    else if (pkVector && pkVector->GetType() == "float4" && pkScale &&
        pkScale->GetType() =="float")
    {
        pkNode = GetAttachableNodeFromLibrary(
            "ScaleFloat4");
    }
    else if (pkVector && pkVector->GetType() == "float3" && pkScale &&
        pkScale->GetType() =="float")
    {
        pkNode = GetAttachableNodeFromLibrary(
            "ScaleFloat3");
    }
    else if (pkVector && pkVector->GetType() == "float2" && pkScale &&
        pkScale->GetType() =="float")
    {
        pkNode = GetAttachableNodeFromLibrary(
            "ScaleFloat2");
    }
    else if (pkVector && pkVector->GetType() == "float" && pkScale &&
        pkScale->GetType() =="float")
    {
        pkNode = GetAttachableNodeFromLibrary(
            "MultiplyFloat");
        pcScaleInputName = "V2";
    }

    if (!pkNode)
    {
        EE_FAIL("Error in fragment");
        return false;
    }

    kContext.m_spConfigurator->AddNode(pkNode);

    kContext.m_spConfigurator->AddBinding(pkVector,
        pkNode->GetInputResourceByVariableName("V1"));
    kContext.m_spConfigurator->AddBinding(pkScale,
        pkNode->GetInputResourceByVariableName(pcScaleInputName));

    pkValue = pkNode->GetOutputResourceByVariableName("Output");
    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiFragmentOperations::SaturateVector(
    Context& kContext,
    NiMaterialResource* pkVector, NiMaterialResource*& pkValue)
{
    NiMaterialNode* pkNode = NULL;

    if (pkVector && pkVector->GetType() == "float4")
    {
        pkNode = GetAttachableNodeFromLibrary(
            "SaturateFloat4");
    }
    else if (pkVector && pkVector->GetType() == "float3")
    {
        pkNode = GetAttachableNodeFromLibrary(
            "SaturateFloat3");
    }
    else if (pkVector && pkVector->GetType() == "float2")
    {
        pkNode = GetAttachableNodeFromLibrary(
            "SaturateFloat2");
    }
    else if (pkVector && pkVector->GetType() == "float")
    {
        pkNode = GetAttachableNodeFromLibrary(
            "SaturateFloat");
    }

    if (!pkNode)
    {
        EE_FAIL("Error in fragment");
        return false;
    }

    kContext.m_spConfigurator->AddNode(pkNode);

    kContext.m_spConfigurator->AddBinding(pkVector,
        pkNode->GetInputResourceByVariableName("V1"));

    pkValue = pkNode->GetOutputResourceByVariableName("Output");
    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiFragmentOperations::MultiplyScalarSatAddVector(
    Context& kContext,
    NiMaterialResource* pkVector1, NiMaterialResource* pkVector2,
    NiMaterialResource* pkScale, NiMaterialResource*& pkValue)
{
    NiMaterialNode* pkNode = NULL;

    if ((pkVector1 && pkVector1->GetType() == "float3") || (pkVector2 &&
        pkVector2->GetType() =="float3"))
    {
        pkNode = GetAttachableNodeFromLibrary(
            "MultiplyScalarSatAddFloat3");
    }

    if (!pkNode)
    {
        EE_FAIL("Error in fragment");
        return false;
    }

    kContext.m_spConfigurator->AddNode(pkNode);

    if (pkVector1)
    {
        kContext.m_spConfigurator->AddBinding(pkVector1,
            pkNode->GetInputResourceByVariableName("V1"));
    }

    if (pkVector2)
    {
        kContext.m_spConfigurator->AddBinding(pkVector2,
            pkNode->GetInputResourceByVariableName("V2"));
    }

    if (pkScale)
    {
        kContext.m_spConfigurator->AddBinding(pkScale,
            pkNode->GetInputResourceByVariableName("Scalar"));
    }

    pkValue = pkNode->GetOutputResourceByVariableName("Output");
    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiFragmentOperations::SplitColorAndOpacity(
    Context& kContext,
    NiMaterialResource* pkColorAndOpacity, NiMaterialResource*& pkColor,
    NiMaterialResource*& pkOpacity)
{
    NiMaterialNode* pkNode = NULL;

    if (pkColorAndOpacity && pkColorAndOpacity->GetType() == "float4")
    {
        pkNode = GetAttachableNodeFromLibrary(
            "SplitColorAndOpacity");
    }

    if (!pkNode)
    {
        EE_FAIL("Error in fragment");
        return false;
    }

    kContext.m_spConfigurator->AddNode(pkNode);

    kContext.m_spConfigurator->AddBinding(pkColorAndOpacity,
        pkNode->GetInputResourceByVariableName("ColorAndOpacity"));

    pkColor = pkNode->GetOutputResourceByVariableName("Color");
    pkOpacity = pkNode->GetOutputResourceByVariableName("Opacity");
    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiFragmentOperations::SplitRGBA(
    Context& kContext,
    NiMaterialResource* pkColorAndOpacity, NiMaterialResource*& pkRed,
    NiMaterialResource*& pkGreen, NiMaterialResource*& pkBlue,
    NiMaterialResource*& pkAlpha)
{
    NiMaterialNode* pkNode = NULL;

    if (pkColorAndOpacity && pkColorAndOpacity->GetType() == "float4")
    {
        pkNode = GetAttachableNodeFromLibrary(
            "SplitRGBA");
    }

    if (!pkNode)
    {
        EE_FAIL("Error in fragment");
        return false;
    }

    kContext.m_spConfigurator->AddNode(pkNode);

    kContext.m_spConfigurator->AddBinding(pkColorAndOpacity,
        pkNode->GetInputResourceByVariableName("ColorAndOpacity"));

    pkRed = pkNode->GetOutputResourceByVariableName("Red");
    pkGreen = pkNode->GetOutputResourceByVariableName("Green");
    pkBlue = pkNode->GetOutputResourceByVariableName("Blue");
    pkAlpha = pkNode->GetOutputResourceByVariableName("Alpha");

    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiFragmentOperations::TypeCast(Context& kContext, NiFixedString kDstType,
    NiMaterialResource* pkInput, NiMaterialResource*& pkOutput)
{
    NiMaterialNode* pkNode = NULL;
    if (pkInput && pkInput->GetType() == kDstType)
    {
        pkOutput = pkInput;
        return true;
    }
    else if (pkInput  && pkInput->GetType()  == "float" && kDstType == "float3")
    {
        pkNode = GetAttachableNodeFromLibrary("FloatToFloat3");
    }
    else if (pkInput  && pkInput->GetType()  == "float" && kDstType == "float4")
    {
        pkNode = GetAttachableNodeFromLibrary("FloatToFloat4");
    }
    else if (pkInput  && pkInput->GetType()  == "float2" && kDstType == "float3")
    {
        pkNode = GetAttachableNodeFromLibrary("Float2ToFloat3");
    }
    else if (pkInput  && pkInput->GetType()  == "float2" && kDstType == "float4")
    {
        pkNode = GetAttachableNodeFromLibrary("Float2ToFloat4");
    }
    else if (pkInput  && pkInput->GetType()  == "float4" && kDstType == "float")
    {
        pkNode = GetAttachableNodeFromLibrary("Float4ToFloat");
    }
    else if (pkInput  && pkInput->GetType()  == "float4" && kDstType == "float3")
    {
        pkNode = GetAttachableNodeFromLibrary("Float4ToFloat3");
    }
    else if (pkInput  && pkInput->GetType()  == "float3" && kDstType == "float")
    {
        pkNode = GetAttachableNodeFromLibrary("Float3ToFloat");
    }
    else if (pkInput  && pkInput->GetType()  == "float4x4" && kDstType == "float3x3")
    {
        pkNode = GetAttachableNodeFromLibrary("Float4x4ToFloat3x3");
    }

    if (!pkNode)
    {
        EE_FAIL("Error Generating fragment: Unable to typecast input type to output type");
        return false;
    }
    kContext.m_spConfigurator->AddNode(pkNode);

    // Bind Inputs
    kContext.m_spConfigurator->AddBinding(pkInput, 
        pkNode->GetInputResourceByVariableName("Input"));   

    // Bind outputs
    pkOutput = pkNode->GetOutputResourceByVariableName("Output");

    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiFragmentOperations::TypeCastBind(
    Context& kContext, 
    NiMaterialResource* pkSrc, NiMaterialResource* pkDest)
{
    // Attempt the cast
    NiMaterialResource* pkCastOutput = NULL;
    if (pkSrc && pkDest &&
        TypeCast(kContext, pkDest->GetType(), pkSrc, pkCastOutput))
    {
        // Bind with the input
        kContext.m_spConfigurator->AddBinding(pkCastOutput, pkDest);
    }
    else if (pkSrc && pkDest && kContext.m_spConfigurator->CanBindTypes(
        pkSrc->GetType(), pkDest->GetType()))
    {
        return kContext.m_spConfigurator->AddBinding(pkSrc, pkDest);
    }
    else
    {
        EE_FAIL("Failed Binding");
        return false;
    }

    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiFragmentOperations::Combine(Context& kContext,
    NiMaterialResource* pkInputA, NiMaterialResource* pkInputB, NiMaterialResource*& pkOutput)
{
    if (pkInputA == NULL || pkInputB == NULL)
        return false;

    NiMaterialNode* pkNode = NULL;
    if (pkInputA->GetType() == "float3" && pkInputB->GetType() == "float")
    {
        pkNode = GetAttachableNodeFromLibrary("CombineFloat3Float1");
    }
    else if (pkInputA->GetType() == "float2" && pkInputB->GetType() == "float2")
    {
        pkNode = GetAttachableNodeFromLibrary("CombineFloat2Float2");
    }
    else if (pkInputA->GetType() == "float" && pkInputB->GetType() == "float3")
    {
        pkNode = GetAttachableNodeFromLibrary("CombineFloat1Float3");
    }

    if (!pkNode)
    {
        EE_FAIL("Error Generating fragment: Unable to combine input types");
        return false;
    }
    kContext.m_spConfigurator->AddNode(pkNode);

    // Bind Inputs
    kContext.m_spConfigurator->AddBinding(pkInputA, 
        pkNode->GetInputResourceByVariableName("InputA"));
    kContext.m_spConfigurator->AddBinding(pkInputB, 
        pkNode->GetInputResourceByVariableName("InputB"));

    // Bind outputs
    pkOutput = pkNode->GetOutputResourceByVariableName("Output");

    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiFragmentOperations::CombineBind(Context& kContext, NiMaterialResource* pkInputA, 
    NiMaterialResource* pkInputB, NiMaterialResource* pkOutput)
{
    // Attempt the cast
    NiMaterialResource* pkCombinedOutput = NULL;
    if (pkInputA && pkInputB && pkOutput && Combine(kContext, pkInputA, pkInputB, pkOutput))
    {
        // Bind with the input
        kContext.m_spConfigurator->AddBinding(pkCombinedOutput, pkOutput);
    }
    else
    {
        EE_FAIL("Failed Binding");
        return false;
    }

    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiFragmentOperations::OptionalBind(
    Context& kContext, NiFixedString kSrcName, NiMaterialNode* pkSrcNode, 
    NiFixedString kDestName, NiMaterialNode* pkDestNode)
{
    NiMaterialResource* pkSrc = pkSrcNode->GetOutputResourceByVariableName(kSrcName);
    NiMaterialResource* pkDest = pkDestNode->GetInputResourceByVariableName(kDestName);
    return OptionalBind(kContext, pkSrc, pkDest);
}

//--------------------------------------------------------------------------------------------------
bool NiFragmentOperations::OptionalBind(
    Context& kContext, NiMaterialResource* pkSrc, 
    NiFixedString kDestName, NiMaterialNode* pkDestNode)
{
    NiMaterialResource* pkDest = pkDestNode->GetInputResourceByVariableName(kDestName);
    return OptionalBind(kContext, pkSrc, pkDest);
}

//--------------------------------------------------------------------------------------------------
bool NiFragmentOperations::OptionalBind(
    Context& kContext, NiFixedString kSrcName, NiMaterialNode* pkSrcNode, 
    NiMaterialResource* pkDest)
{
    NiMaterialResource* pkSrc = pkSrcNode->GetOutputResourceByVariableName(kSrcName);
    return OptionalBind(kContext, pkSrc, pkDest);
}

//--------------------------------------------------------------------------------------------------
bool NiFragmentOperations::OptionalBind(
    Context& kContext, 
    NiMaterialResource* pkSrc, NiMaterialResource* pkDest)
{
    if (pkSrc && pkDest)
        return kContext.m_spConfigurator->AddBinding(pkSrc, pkDest);
    else
        return false;
}

//--------------------------------------------------------------------------------------------------
NiMaterialResource* NiFragmentOperations::GenerateShaderConstant(Context& kContext, bool bValue)
{
    return kContext.m_spStatics->AddOutputConstant("bool", (bValue)?("(true)"):("(false)"));
}

//--------------------------------------------------------------------------------------------------
NiMaterialResource* NiFragmentOperations::GenerateShaderConstant(Context& kContext, 
    efd::SInt32 iValue)
{
    NiString kValue;
    kValue.Format("(%d)", iValue);
    return kContext.m_spStatics->AddOutputConstant("int", (const char*)kValue);
}

//--------------------------------------------------------------------------------------------------
NiMaterialResource* NiFragmentOperations::GenerateShaderConstant(Context& kContext, 
    float fValue)
{
    NiString kValue;
    kValue.Format("(%f)", fValue);
    return kContext.m_spStatics->AddOutputConstant("float", (const char*)kValue);
}

//--------------------------------------------------------------------------------------------------
NiMaterialResource* NiFragmentOperations::GenerateShaderConstant(Context& kContext, 
    const NiPoint4& kPoint)
{
    NiString kValue;
    kValue.Format("(%f, %f, %f, %f)", kPoint.X(), kPoint.Y(), kPoint.Z(), kPoint.W());
    return kContext.m_spStatics->AddOutputConstant("float4", (const char*)kValue);
}

//--------------------------------------------------------------------------------------------------
NiMaterialResource* NiFragmentOperations::GenerateShaderConstant(Context& kContext, 
    const NiPoint3& kPoint)
{
    NiString kValue;
    kValue.Format("(%f, %f, %f)", kPoint.x, kPoint.y, kPoint.z);
    return kContext.m_spStatics->AddOutputConstant("float3", (const char*)kValue);
}

//--------------------------------------------------------------------------------------------------
NiMaterialResource* NiFragmentOperations::GenerateShaderConstant(Context& kContext, 
    const NiPoint2& kPoint)
{
    NiString kValue;
    kValue.Format("(%f, %f)", kPoint.x, kPoint.y);
    return kContext.m_spStatics->AddOutputConstant("float2", (const char*)kValue);
}

//--------------------------------------------------------------------------------------------------
NiMaterialResource* NiFragmentOperations::GenerateShaderConstant(Context& kContext, 
    const NiColor& kColor)
{
    NiString kValue;
    kValue.Format("(%f, %f, %f)", kColor.r, kColor.g, kColor.b);
    return kContext.m_spStatics->AddOutputConstant("float3", (const char*)kValue);
}

//--------------------------------------------------------------------------------------------------
bool NiFragmentOperations::TransformPosition(Context& kContext, 
   NiMaterialResource* pkPosition, NiMaterialResource* pkTransform,
   NiMaterialResource*& pkTransformPos)
{
    EE_ASSERT(pkPosition && pkTransform);
    pkTransformPos = NULL;

    // Convert the local position into the transformed position
    NiMaterialNode* pkTransformPosFrag = GetAttachableNodeFromLibrary(
        "HandleTransformPosition");
    EE_ASSERT(pkTransformPosFrag);
    kContext.m_spConfigurator->AddNode(pkTransformPosFrag);

    // Bind Inputs
    kContext.m_spConfigurator->AddBinding(pkPosition,
        "OriginalPos", pkTransformPosFrag);
    kContext.m_spConfigurator->AddBinding(pkTransform,
        "TransformMatrix", pkTransformPosFrag);

    // Bind Outputs
    pkTransformPos = pkTransformPosFrag->GetOutputResourceByVariableName(
        "TransformPos");
    EE_ASSERT(pkTransformPos);

    return true;
}
//--------------------------------------------------------------------------------------------------
bool NiFragmentOperations::TransformDirection(Context& kContext,
    NiMaterialResource* pkDirection, NiMaterialResource* pkTransform,
    NiMaterialResource*& pkTransformDir)
{
    EE_ASSERT(pkDirection && pkTransform);
    pkTransformDir = NULL;

    // Convert the local direction into the transformed space
    NiMaterialNode* pkTransformDirFrag = GetAttachableNodeFromLibrary(
        "HandleTransformDirection");
    EE_ASSERT(pkTransformDirFrag);
    kContext.m_spConfigurator->AddNode(pkTransformDirFrag);

    // Bind Inputs
    TypeCastBind(kContext, pkDirection, 
        pkTransformDirFrag->GetInputResourceByVariableName("OriginalDir"));
    TypeCastBind(kContext, pkTransform, 
        pkTransformDirFrag->GetInputResourceByVariableName("TransformMatrix"));

    // Bind Outputs
    pkTransformDir = pkTransformDirFrag->GetOutputResourceByVariableName(
        "TransformDir");
    EE_ASSERT(pkTransformDir);

    return true;
}
//--------------------------------------------------------------------------------------------------
