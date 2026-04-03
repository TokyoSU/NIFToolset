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

// Pre-compiled header
#include "ecrPCH.h"

#include "RenderableModel.h"

using namespace efd;
using namespace egf;
using namespace ecr;

EE_IMPLEMENT_CONCRETE_CLASS_INFO(RenderableModel);

EE_IMPLEMENT_BUILTINMODEL_PROPERTIES(RenderableModel);

vector<IPropertyCallback*>* RenderableModel::ms_pCallbackList;

//--------------------------------------------------------------------------------------------------
void RenderableModel::Initialize()
{
    ms_pCallbackList = EE_EXTERNAL_NEW vector<IPropertyCallback*>;
}

//--------------------------------------------------------------------------------------------------
void RenderableModel::Shutdown()
{
    EE_EXTERNAL_DELETE ms_pCallbackList;
}

//--------------------------------------------------------------------------------------------------
IBuiltinModel* RenderableModel::RenderableModelFactory()
{
    return EE_NEW RenderableModel();
}

//--------------------------------------------------------------------------------------------------
RenderableModel::RenderableModel()
    : IPropertyCallbackInvoker(ms_pCallbackList)
    , m_isVisible(true)
    , m_isFogAffected(true)
	, m_queue(0)
{
}
    
//--------------------------------------------------------------------------------------------------
RenderableModel::~RenderableModel()
{
}

//--------------------------------------------------------------------------------------------------
bool RenderableModel::Initialize(egf::Entity* pOwner, const egf::PropertyDescriptorList& defaults)
{
    IBuiltinModel::Initialize(pOwner, defaults);

    for (PropertyDescriptorList::const_iterator iter = defaults.begin();
        iter != defaults.end();
        ++iter)
    {
        switch ((*iter)->GetPropertyID())
        {
            case kPropertyID_StandardModelLibrary_IsVisible:
                (*iter)->GetDefaultProperty()->GetValue(
                    kPropertyID_StandardModelLibrary_IsVisible,
                    &m_isVisible);
                break;
			case kPropertyID_StandardModelLibrary_IsFogAffected:
				(*iter)->GetDefaultProperty()->GetValue(
					kPropertyID_StandardModelLibrary_IsFogAffected,
					&m_isFogAffected);
				break;
			case kPropertyID_StandardModelLibrary_Queue:
				(*iter)->GetDefaultProperty()->GetValue(
					kPropertyID_StandardModelLibrary_Queue,
					&m_queue);
				break;

            default:;
        }
    }

    return true;
}

//--------------------------------------------------------------------------------------------------
bool RenderableModel::ResetProperty(const egf::PropertyDescriptor* pDefault)
{
    EE_ASSERT(pDefault->GetPropertyID() == kPropertyID_StandardModelLibrary_IsVisible);

	switch (pDefault->GetPropertyID())
	{
	case kPropertyID_StandardModelLibrary_IsVisible:
		{
			bool isVisible = true;
			pDefault->GetDefaultProperty()->GetValue(
				kPropertyID_StandardModelLibrary_IsVisible,
				&isVisible);
			SetIsVisible(isVisible);
		}
		break;
	case kPropertyID_StandardModelLibrary_IsFogAffected:
		{
			bool isFogAffected = true;
			pDefault->GetDefaultProperty()->GetValue(
				kPropertyID_StandardModelLibrary_IsFogAffected,
				&isFogAffected);
			SetIsFogAffected(isFogAffected);
		}
		break;
	case kPropertyID_StandardModelLibrary_Queue:
		{
			efd::UInt32 queueID;
			pDefault->GetDefaultProperty()->GetValue(
				kPropertyID_StandardModelLibrary_Queue,
				&queueID);
			SetQueue(queueID);
		}
		break;
	}
    
    return true;
}

//--------------------------------------------------------------------------------------------------
bool RenderableModel::operator==(const IProperty& other) const
{
    if (!IBuiltinModel::operator ==(other))
        return false;

    const RenderableModel* otherClass = static_cast<const RenderableModel *>(&other);

    if (m_isVisible != otherClass->m_isVisible)
    {
        return false;
    }

	if (m_isFogAffected != otherClass->m_isFogAffected)
	{
		return false;
	}

	if (m_queue != otherClass->m_queue)
	{
		return false;
	}

    return true;
}

//--------------------------------------------------------------------------------------------------
