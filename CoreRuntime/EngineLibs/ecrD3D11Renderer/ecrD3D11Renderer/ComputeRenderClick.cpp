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
#include "ecrD3D11RendererPCH.h"

#include "ComputeRenderClick.h"
#include "D3D11ShaderCore.h"

using namespace ecr;

NiImplementRTTI(ComputeRenderClick, NiRenderClick);

//------------------------------------------------------------------------------------------------
ComputeRenderClick::ComputeRenderClick() : 
    m_numObjectsDrawn(0), 
    m_renderTime(0.0f)
{
    /* */
}

//------------------------------------------------------------------------------------------------
inline ComputeRenderClick::~ComputeRenderClick()
{
    /* */
}

//------------------------------------------------------------------------------------------------
efd::UInt32 ComputeRenderClick::GetNumObjectsDrawn() const
{
    return m_numObjectsDrawn;
}

//------------------------------------------------------------------------------------------------
efd::Float32 ComputeRenderClick::GetCullTime() const
{
    return 0.0f;
}

//------------------------------------------------------------------------------------------------
efd::Float32 ComputeRenderClick::GetRenderTime() const
{
    return m_renderTime;
}

//------------------------------------------------------------------------------------------------
void ComputeRenderClick::PerformRendering(efd::UInt32)
{
    EE_PUSH_GPU_MARKER_VA("ComputeRC(%s)", (const char*)GetName());

    // If there are no views, return without rendering.
    if (m_shaderList.GetSize() == 0)
    {
        EE_POP_GPU_MARKER();
        return;
    }

    // Reset rendering statistics.
    m_numObjectsDrawn = 0;
    m_renderTime = 0.0f;

    // Iterate over all render views.
    NiTListIterator iter = m_shaderList.GetHeadPos();
    while (iter)
    {
        NiShader* pShader = m_shaderList.GetNext(iter);
        EE_ASSERT(pShader);

        efd::Float32 beginTime = (efd::Float32)efd::GetCurrentTimeInSec();

        reinterpret_cast<D3D11ShaderCore*>(pShader)->InvokeShader();

        // Update rendering statistics.
        m_renderTime += (efd::Float32)efd::GetCurrentTimeInSec() - beginTime;
        m_numObjectsDrawn++;
    }

    EE_POP_GPU_MARKER();
}

//------------------------------------------------------------------------------------------------
