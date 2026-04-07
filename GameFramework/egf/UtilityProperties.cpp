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

#include "egfPCH.h"

#include <egf/UtilityProperties.h>
#include <egf/FlatModelManager.h>
#include <efd/ILogger.h>

using namespace efd;
using namespace egf;

//-------------------------------------------------------------------------------------------------
EE_IMPLEMENT_PRIMITIVE_SCALAR_PROPERTY(egf::Point2);
EE_IMPLEMENT_PRIMITIVE_SCALAR_PROPERTY(egf::Point3);
EE_IMPLEMENT_PRIMITIVE_SCALAR_PROPERTY(egf::Matrix3);
EE_IMPLEMENT_PRIMITIVE_SCALAR_PROPERTY(egf::Color);
EE_IMPLEMENT_PRIMITIVE_SCALAR_PROPERTY(egf::ColorA);
EE_IMPLEMENT_PRIMITIVE_SCALAR_PROPERTY(egf::AssetID);
EE_IMPLEMENT_PRIMITIVE_ASSOCIATIVE_ARRAY_PROPERTY(egf::Point2);
EE_IMPLEMENT_PRIMITIVE_ASSOCIATIVE_ARRAY_PROPERTY(egf::Point3);
EE_IMPLEMENT_PRIMITIVE_ASSOCIATIVE_ARRAY_PROPERTY(egf::Matrix3);
EE_IMPLEMENT_PRIMITIVE_ASSOCIATIVE_ARRAY_PROPERTY(egf::Color);
EE_IMPLEMENT_PRIMITIVE_ASSOCIATIVE_ARRAY_PROPERTY(egf::ColorA);
EE_IMPLEMENT_PRIMITIVE_ASSOCIATIVE_ARRAY_PROPERTY(egf::AssetID);


//-------------------------------------------------------------------------------------------------
efd::utf8string egf::TranslatePropertyResult(egf::PropertyResult value)
{
    efd::utf8string response;
    switch (value)
    {
    case egf::PropertyResult_OK:
        response = "OK";
        break;
    case egf::PropertyResult_TypeMismatch:
        response = "Type Mismatch";
        break;
    case PropertyResult_NoDefaultValue:
        response = "No Default Value";
        break;
    case PropertyResult_PropertyNotFound:
        response = "Property Not Found";
        break;
    case PropertyResult_DefaultValueAlreadySet:
        response = "Default Value Already Set";
        break;
    case PropertyResult_UnknownPropertyType:
        response = "Unknown Property Type";
        break;
    case PropertyResult_ModelNotFound:
        response = "Model Not Found";
        break;
    case PropertyResult_PropertyNotScalar:
        response = "Property Not Scalar";
        break;
    case PropertyResult_PropertyNotAssociativeArray:
        response = "Property Not Associative Array";
        break;
    case PropertyResult_KeyNotFound:
        response = "Key Not Found";
        break;
    case PropertyResult_ReadOnlyError:
        response = "Read Only Error";
        break;
    case PropertyResult_EntityNotOwned:
        response = "Entity Not Owned";
        break;
    case PropertyResult_EntityNotFound:
        response = "Entity Not Found";
        break;
    case PropertyResult_NoMoreKeys:
        response = "No More Keys";
        break;

    case PropertyResult_UnknownError:
    default:
        response = "Unknown Error";
        break;
    }

    return response;
}

