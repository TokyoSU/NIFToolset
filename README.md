# NiApplication
The core of the library, you can create a base window and renderer using it, this is my custom implementation, you can still take it as base to create your own :)
Example:
```cpp
NiApplication::Settings settings;
settings.m_bFullscreen = false;
settings.m_kClearColor = NiColorA(0.0f, 0.0f, 0.0f, 1.0f);
settings.m_pszTitle = "Your game application name";
settings.m_uiWidth = 1920;
settings.m_uiHeight = 1080;
settings.m_fNear = 0.01f;
settings.m_fFar = 10000.0f;
settings.m_fFov = 45.0f;
settings.m_bAlphaSorting = true; // Enable alpha sorting.
settings.m_bShadows = false; // Enable shadows, still WIP (not work yet).
settings.m_bResizable = true;
settings.m_eDriverType = ecr::D3D11Renderer::DriverType::DRIVER_TYPE_HARDWARE;
settings.m_eSwapChainBuffer = DXGI_FORMAT_R8G8B8A8_UNORM;
settings.m_eDepthFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
settings.m_pszShaderCacheFolder = "shaders"; // The folder can be empty, the library compile and cache it in this folder.
m_pApp = new NiApplication();
m_pApp->SetInitCallback(&CApplication::OnInit, this);
m_pApp->SetShutdownCallback(&CApplication::OnShutdown, this);
m_pApp->SetEventCallback(&CApplication::OnSDLEvent, this);
m_pApp->SetUpdateCallback(&CApplication::OnUpdate, this);
m_pApp->SetRenderCallback(&CApplication::OnRender, this);
// You can also have SetResizeCallback(your_callback, this).
if (m_pApp->Initialize(settings)) {
	m_pCamera = m_pApp->GetCamera();
	m_pApp->Run();
}

bool CApplication::OnInit(NiApplication* pApp, void* pUserData)
{
	CApplication* pThis = static_cast<CApplication*>(pUserData);
    
	return true;
}

void CApplication::OnShutdown(NiApplication* pApp, void* pUserData)
{
	CApplication* pThis = static_cast<CApplication*>(pUserData);
    
}

void CApplication::OnUpdate(NiApplication* pApp, float fDeltaTime, void* pUserData)
{
	CApplication* pThis = static_cast<CApplication*>(pUserData);
    
}

void CApplication::OnRender(NiApplication* pApp, void* pUserData)
{
	CApplication* pThis = static_cast<CApplication*>(pUserData);
	pApp->BeginFrame(); // Only 1 is required alongside EndFrame().

	pApp->BeginScene(); // You can have multiple begin/end scene (with different RenderGroup if required).
	// Draw here.
	pApp->EndScene();
    
	pApp->EndFrame();
	pApp->Present(); // This should always be last !
}

bool CApplication::OnSDLEvent(NiApplication* pApp, const SDL_Event& kEvent, void* pUserData) // Return true to close the application, false to continue.
{
	CApplication* pThis = static_cast<CApplication*>(pUserData);
    
	return false;
}
```

# NiAudio:
NiMilesAudio was removed and replaced with bass audio system using NiBASSAudio,
Miles used a proprietary library called mss and bink and no source code is currently available to build it correctly.

# Format supported:

- As image:
    - BMP
    - TGA
    - SGI
    - DDS
    - BMP
 
- As file:
    - KFM
    - NIF
    - KF

- As terrain:
    - Any format you have specified, through sector is saved in your_archive\quadtree.dof
