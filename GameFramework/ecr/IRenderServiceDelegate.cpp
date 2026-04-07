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
#include "IRenderServiceDelegate.h"

using namespace ecr;

//--------------------------------------------------------------------------------------------------
IRenderServiceDelegate::~IRenderServiceDelegate()
{
}
//--------------------------------------------------------------------------------------------------
void IRenderServiceDelegate::OnSurfaceAdded(RenderService* pService, RenderSurface* pSurface)
{
    EE_UNUSED_ARG(pService);
    EE_UNUSED_ARG(pSurface);
}
//--------------------------------------------------------------------------------------------------
void IRenderServiceDelegate::OnSurfaceRecreated(RenderService* pService, RenderSurface* pSurface)
{
    EE_UNUSED_ARG(pService);
    EE_UNUSED_ARG(pSurface);
}
//--------------------------------------------------------------------------------------------------
void IRenderServiceDelegate::OnSurfaceRemoved(RenderService* pService, RenderSurface* pSurface)
{
    EE_UNUSED_ARG(pService);
    EE_UNUSED_ARG(pSurface);
}
//--------------------------------------------------------------------------------------------------
void IRenderServiceDelegate::OnSurfacePreDraw(RenderService* pService, RenderSurface* pSurface)
{
    EE_UNUSED_ARG(pService);
    EE_UNUSED_ARG(pSurface);
}
//--------------------------------------------------------------------------------------------------
void IRenderServiceDelegate::OnSurfacePostDraw(RenderService* pService, RenderSurface* pSurface)
{
    EE_UNUSED_ARG(pService);
    EE_UNUSED_ARG(pSurface);
}
//--------------------------------------------------------------------------------------------------
void IRenderServiceDelegate::OnSurfaceActiveChanged(RenderService* pService,
    RenderSurface* pActiveSurface, RenderSurface* pPriorActiveSurface)
{
    EE_UNUSED_ARG(pService);
    EE_UNUSED_ARG(pActiveSurface);
    EE_UNUSED_ARG(pPriorActiveSurface);
}
//--------------------------------------------------------------------------------------------------
void IRenderServiceDelegate::OnRenderServiceShutdown(RenderService* pService)
{
    EE_UNUSED_ARG(pService);
}
//--------------------------------------------------------------------------------------------------
void IRenderServiceDelegate::OnRenderedEntityAdded(
    RenderService* pService, egf::Entity* pEntity, NiAVObject* pAVObject)
{
    EE_UNUSED_ARG(pService);
    EE_UNUSED_ARG(pEntity);
    EE_UNUSED_ARG(pAVObject);
}
//--------------------------------------------------------------------------------------------------
void IRenderServiceDelegate::OnRenderedEntityRemoved(
    RenderService* pService, egf::Entity* pEntity, NiAVObject* pAVObject)
{
    EE_UNUSED_ARG(pService);
    EE_UNUSED_ARG(pEntity);
    EE_UNUSED_ARG(pAVObject);
}
//--------------------------------------------------------------------------------------------------
