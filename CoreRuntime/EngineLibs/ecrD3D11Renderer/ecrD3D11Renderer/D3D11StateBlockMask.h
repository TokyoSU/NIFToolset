// EMERGENT GAME TECHNOLOGIES PROPRIETARY INFORMATION
#pragma once
#ifndef EE_D3D11STATEBLOCKMASK_H
#define EE_D3D11STATEBLOCKMASK_H

#include <D3D11.h>

// Local copy of the state mask layout used by the renderer. This is not a D3DX
// dependency; keeping it local avoids requiring the deprecated Effects11 headers.
struct D3D11StateBlockMask
{
    BYTE VS;
    BYTE VSSamplers[2];
    BYTE VSShaderResources[16];
    BYTE VSConstantBuffers[2];
    BYTE VSInterfaces[32];
    BYTE HS;
    BYTE HSSamplers[2];
    BYTE HSShaderResources[16];
    BYTE HSConstantBuffers[2];
    BYTE HSInterfaces[32];
    BYTE DS;
    BYTE DSSamplers[2];
    BYTE DSShaderResources[16];
    BYTE DSConstantBuffers[2];
    BYTE DSInterfaces[32];
    BYTE GS;
    BYTE GSSamplers[2];
    BYTE GSShaderResources[16];
    BYTE GSConstantBuffers[2];
    BYTE GSInterfaces[32];
    BYTE PS;
    BYTE PSSamplers[2];
    BYTE PSShaderResources[16];
    BYTE PSConstantBuffers[2];
    BYTE PSInterfaces[32];
    BYTE CS;
    BYTE CSSamplers[2];
    BYTE CSShaderResources[16];
    BYTE CSConstantBuffers[2];
    BYTE CSInterfaces[32];
    BYTE CSUnorderedAccessViews[1];
    BYTE IAVertexBuffers[4];
    BYTE IAIndexBuffer;
    BYTE IAInputLayout;
    BYTE IAPrimitiveTopology;
    BYTE OMRenderTargets;
    BYTE OMDepthStencilState;
    BYTE OMBlendState;
    BYTE OMBlendFactor[4];
    BYTE OMSampleMask;
    BYTE RSViewports;
    BYTE RSScissorRects;
    BYTE RSRasterizerState;
    BYTE SOBuffers;
    BYTE Predication;
};

#endif
