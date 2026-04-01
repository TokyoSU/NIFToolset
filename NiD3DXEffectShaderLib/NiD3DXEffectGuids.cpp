// This file must NOT use the precompiled header.
// INITGUID causes every DEFINE_GUID() in d3dx9.h to emit a real definition
// instead of just a declaration, satisfying the linker for IID_ID3DXEffect
// and IID_ID3DXEffectStateManager.
#define INITGUID
#include <dxsdk-d3dx/d3dx9.h>
