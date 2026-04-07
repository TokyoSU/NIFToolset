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

//-------------------------------------------------------------------------------------------------
inline efd::Bool ecr::RenderableModel::GetIsVisible() const
{
    return m_isVisible;
}

//-------------------------------------------------------------------------------------------------
inline void ecr::RenderableModel::SetIsVisible(const efd::Bool& isVisible)
{
    if (m_isVisible != isVisible)
    {
        m_isVisible = isVisible;
        
        InvokeCallbacks(
            egf::kFlatModelID_StandardModelLibrary_Renderable,
            m_pOwningEntity,
            egf::kPropertyID_StandardModelLibrary_IsVisible,
            this,
            0,
            0);
    }
}

//-------------------------------------------------------------------------------------------------
inline void ecr::RenderableModel::SetInternalIsVisible(
    const efd::Bool isVisible,
    egf::IPropertyCallback* ignoreCallback)
{
    if (m_isVisible != isVisible)
    {
        m_isVisible = isVisible;
    
        m_pOwningEntity->BuiltinPropertyChanged(
            egf::kPropertyID_StandardModelLibrary_IsVisible,
            this);

        InvokeCallbacks(
            egf::kFlatModelID_StandardModelLibrary_Renderable,
            m_pOwningEntity,
            egf::kPropertyID_StandardModelLibrary_IsVisible,
            this,
            0,
            ignoreCallback);
    }
}

//-------------------------------------------------------------------------------------------------
inline efd::Bool ecr::RenderableModel::GetIsFogAffected() const
{
    return m_isFogAffected;
}

//-------------------------------------------------------------------------------------------------
inline void ecr::RenderableModel::SetIsFogAffected(const efd::Bool& isFogAffected)
{
    if (m_isFogAffected != isFogAffected)
    {
        m_isFogAffected = isFogAffected;

        InvokeCallbacks(
            egf::kFlatModelID_StandardModelLibrary_Renderable,
            m_pOwningEntity,
            egf::kPropertyID_StandardModelLibrary_IsFogAffected,
            this,
            0,
            0);
    }
}

//-------------------------------------------------------------------------------------------------
inline efd::UInt32 ecr::RenderableModel::GetQueue() const
{
	return m_queue;
}

//-------------------------------------------------------------------------------------------------
inline void ecr::RenderableModel::SetQueue(const efd::UInt32& queueID)
{
	if (m_queue != queueID)
	{
		m_queue = queueID;

		InvokeCallbacks(
			egf::kFlatModelID_StandardModelLibrary_Renderable,
			m_pOwningEntity,
			egf::kPropertyID_StandardModelLibrary_Queue,
			this,
			0,
			0);
	}
}

//-------------------------------------------------------------------------------------------------
inline void ecr::RenderableModel::AddCallback(egf::IPropertyCallback* pCallback)
{
    RenderableModel temp;
    temp.AddPropertyCallback(pCallback);
}

//-------------------------------------------------------------------------------------------------
inline void ecr::RenderableModel::RemoveCallback(egf::IPropertyCallback* pCallback)
{
    RenderableModel temp;
    temp.RemovePropertyCallback(pCallback);
}

//-------------------------------------------------------------------------------------------------
