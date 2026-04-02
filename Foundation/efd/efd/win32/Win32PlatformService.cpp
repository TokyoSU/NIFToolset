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

#include "efdPCH.h"

#include <efd/Win32/Win32PlatformService.h>


EE_IMPLEMENT_CONCRETE_CLASS_INFO(efd::Win32PlatformService);

static const efd::UInt32 MAX_LOADSTRING = 100;

static const UINT EE_APP_ACTIVATE = WM_USER + 1000;
static const UINT EE_APP_DEACTIVATE = WM_USER + 1001;


//------------------------------------------------------------------------------------------------
efd::Win32PlatformService::Win32PlatformService(
    HINSTANCE m_hInstance,
    HINSTANCE hPrevInstance,
    LPTSTR lpCmdLine,
    int m_nCmdShow)
    : m_hInstance(m_hInstance)
    , m_hPrevInstance(hPrevInstance)
    , m_lpCmdLine(lpCmdLine)
    , m_nCmdShow(m_nCmdShow)
    , m_hWnd(0)
    , m_idWindowMenu(0)
    , m_idLargeIcon(0)
    , m_idSmallIcon(0)
    , m_windowWidth(1024)
    , m_windowHeight(768)
    , m_windowLeft(0)
    , m_windowTop(0)
    , m_isDirty(false)
    , m_maxMessagesPerTick(5)
    , m_pWndProc(&WndProc)
{
    // If this default priority is changed, also update the service quick reference documentation
    m_defaultPriority = 6500;
}

//------------------------------------------------------------------------------------------------
efd::Win32PlatformService::~Win32PlatformService()
{
}

//------------------------------------------------------------------------------------------------
const char* efd::Win32PlatformService::GetDisplayName() const
{
return "Win32PlatformService";
}

//------------------------------------------------------------------------------------------------
BOOL efd::Win32PlatformService::InitInstance(HINSTANCE m_hInstance, int m_nCmdShow)
{
    RECT rect = { (LONG)m_windowLeft, (LONG)m_windowTop, (LONG)m_windowWidth, (LONG)m_windowHeight };
    DWORD dwStyle = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
    AdjustWindowRect(&rect, dwStyle, false);
    m_hWnd = CreateWindow(m_strWindowClass.c_str(), m_strWindowTitle.c_str(), dwStyle, 0, 0,
        rect.right-rect.left, rect.bottom-rect.top, NULL, NULL, m_hInstance, NULL);
    if (m_hWnd)
    {
        ShowWindow(m_hWnd, m_nCmdShow);
        UpdateWindow(m_hWnd);

        return TRUE;
    }

    return FALSE;
}

//------------------------------------------------------------------------------------------------
ATOM efd::Win32PlatformService::RegisterClass(HINSTANCE m_hInstance)
{
    WNDCLASSEX wcex = {};
    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = *m_pWndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = m_hInstance;
    if (m_idLargeIcon)
    {
        wcex.hIcon = LoadIcon(m_hInstance, MAKEINTRESOURCE(m_idLargeIcon));
    }
    else
    {
        wcex.hIcon = LoadIcon(m_hInstance, IDI_APPLICATION);
    }
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
    if (m_idWindowMenu)
    {
        wcex.lpszMenuName = MAKEINTRESOURCE(m_idWindowMenu);
    }
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = m_strWindowClass.c_str();
    if (m_idSmallIcon)
    {
        wcex.hIconSm = LoadIcon(m_hInstance, MAKEINTRESOURCE(m_idSmallIcon));
    }
    else
    {
        wcex.hIconSm = LoadIcon(m_hInstance, IDI_APPLICATION);
    }

    return RegisterClassEx(&wcex);
}

//------------------------------------------------------------------------------------------------
efd::InstanceRef efd::Win32PlatformService::GetInstanceRef() const
{
    return m_hInstance;
}

//------------------------------------------------------------------------------------------------
efd::WindowRef efd::Win32PlatformService::GetWindowRef() const
{
    return m_hWnd;
}

//------------------------------------------------------------------------------------------------
void efd::Win32PlatformService::SetWindowTitle(const efd::utf8string& strTitle)
{
    m_strWindowTitle = strTitle;
}

//------------------------------------------------------------------------------------------------
void efd::Win32PlatformService::SetWindowTitle(efd::UInt32 titleResourceID)
{
    TCHAR buffer[MAX_LOADSTRING];
    LoadString(m_hInstance, titleResourceID, buffer, MAX_LOADSTRING);
    m_strWindowTitle = buffer;
}

//------------------------------------------------------------------------------------------------
void efd::Win32PlatformService::SetWindowClass(const efd::utf8string& strClass)
{
    m_strWindowClass = strClass;
}

//------------------------------------------------------------------------------------------------
void efd::Win32PlatformService::SetWindowClass(efd::UInt32 classResourceID)
{
    TCHAR buffer[MAX_LOADSTRING];
    LoadString(m_hInstance, classResourceID, buffer, MAX_LOADSTRING);
    m_strWindowClass = buffer;
}

//------------------------------------------------------------------------------------------------
void efd::Win32PlatformService::SetWindowIcon(efd::UInt32 largeIconID, efd::UInt32 smallIconID)
{
    m_idLargeIcon = largeIconID;

    if (smallIconID)
    {
        m_idSmallIcon = smallIconID;
    }
    else
    {
        m_idSmallIcon = largeIconID;
    }
}

//------------------------------------------------------------------------------------------------
void efd::Win32PlatformService::SetWindowWidth(efd::UInt32 width)
{
    m_windowWidth = width;
    m_isDirty = true;
}

//------------------------------------------------------------------------------------------------
void efd::Win32PlatformService::SetWindowHeight(efd::UInt32 height)
{
    m_windowHeight = height;
    m_isDirty = true;
}

//------------------------------------------------------------------------------------------------
void efd::Win32PlatformService::SetWindowLeft(efd::UInt32 left)
{
    m_windowLeft = left;
    m_isDirty = true;
}

//------------------------------------------------------------------------------------------------
void efd::Win32PlatformService::SetWindowTop(efd::UInt32 top)
{
    m_windowTop = top;
    m_isDirty = true;
}

//------------------------------------------------------------------------------------------------
void efd::Win32PlatformService::SetWndProc(WNDPROC pFunc)
{
    m_pWndProc = pFunc;
}

//------------------------------------------------------------------------------------------------
efd::SyncResult efd::Win32PlatformService::OnPreInit(efd::IDependencyRegistrar* pDependencyRegistrar)
{
    EE_UNUSED_ARG(pDependencyRegistrar);

    // Register window class.
    RegisterClass(m_hInstance);

    // Initialize window instance.
    if (InitInstance(m_hInstance, m_nCmdShow))
    {
        return efd::SyncResult_Success;
    }

    return efd::SyncResult_Failure;
}

//------------------------------------------------------------------------------------------------
efd::AsyncResult efd::Win32PlatformService::OnTick()
{
    efd::AsyncResult result = efd::AsyncResult_Pending;

    if (m_isDirty)
    {
        MoveWindow(m_hWnd, m_windowLeft, m_windowTop, m_windowWidth, m_windowHeight, TRUE);
        m_isDirty = false;
    }

    UInt32 cMessage = 0;
    // message pump
    MSG msg;
    while (cMessage < m_maxMessagesPerTick && PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
    {
        ++cMessage;
        if (msg.message == WM_QUIT)
        {
            // Tell all the other services to cleanup and shut down:
            m_pServiceManager->Shutdown();

            result = efd::AsyncResult_Complete;
        }
        else if (msg.message == EE_APP_ACTIVATE)
        {
            // Resume normal tick rate
            m_pServiceManager->UseDeactivatedSleepTime(false);
        }
        else if (msg.message == EE_APP_DEACTIVATE)
        {
            // Reduce tick rate to share CPU with other apps
            m_pServiceManager->UseDeactivatedSleepTime(true);
        }
        else
        {
            if (!TranslateAccelerator(m_hWnd, NULL, &msg))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
    }

    return result;
}

//------------------------------------------------------------------------------------------------
LRESULT CALLBACK efd::Win32PlatformService::WndProc(
    HWND hWnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    switch (message)
    {
    case WM_CHAR:
        {
            switch ((unsigned char)wParam)
            {
            case VK_ESCAPE:
                PostMessage(hWnd, WM_DESTROY, 0, 0);
                break;
            }
        }
        break;

    case WM_ACTIVATEAPP:
        {
            // Activation and deactivation events can be used to reduce CPU usage when the game does
            // not have focus.
            if (wParam == TRUE)
                PostMessage(hWnd, EE_APP_ACTIVATE, 0, 0);
            else
                PostMessage(hWnd, EE_APP_DEACTIVATE, 0, 0);
        }
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

//------------------------------------------------------------------------------------------------
