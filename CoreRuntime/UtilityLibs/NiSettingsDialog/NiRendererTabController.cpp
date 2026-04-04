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

#include "NiSettingsDialogPCH.h"
#include "NiSettingsDialog.h"
#include "NiRendererTabController.h"
#include "NiBaseRendererSetup.h"
#include "NiBaseRendererOptionsView.h"
#include "NiSettingsDialogResource.hrc"

//--------------------------------------------------------------------------------------------------
// Functions to create, initialize, delete NiXXXRendererOptionsView's
//--------------------------------------------------------------------------------------------------
bool NiRendererTabController::CreateOptionsViews(
    NiRendererSettings* pkSettings)
{
    unsigned int uiInitialRenderView = 0;

    // Create OptionsView classes for all supported renderers.
    NiTListIterator kIter = 0;
    NiBaseRendererSetup* pkRendererSetup = 
        NiBaseRendererSetup::GetFirstRendererSetup(kIter);
    if (pkRendererSetup == NULL)
    {
        NILOG("%s: No NiBaseRendererSetup classes were found.\n"
            "Make sure the NiBaseRendererSetup classes for the supported "
            "rendereres are linked in to the application.", __FUNCTION__);
        return false;
    }

    while (pkRendererSetup)
    {
        EE_ASSERT(pkRendererSetup);

        NiBaseRendererOptionsView* pkROV = 
            pkRendererSetup->GetRenderOptionsView(pkSettings);
        if (pkROV)
        {
            if (InitOptionsView(pkROV))
            {
                unsigned int uiPosition = m_kViewArray.Add(pkROV);
                if (pkSettings->m_eRendererID == pkROV->GetRendererID())
                    uiInitialRenderView = uiPosition;
            }
            else
            {
                pkRendererSetup->ReleaseRendererOptionsView(pkROV);
            }
        }
        pkRendererSetup = NiBaseRendererSetup::GetNextRendererSetup(kIter);

    }

    m_uiCurrentViewIdx = uiInitialRenderView;

    // If array is not empty, return success
    if (m_kViewArray.GetSize() > 0)
    {
        return true;
    }
    else
    {
        NILOG("%s: No suitable renderer could be initialized.", __FUNCTION__);
        return false;
    }
}

//--------------------------------------------------------------------------------------------------
bool NiRendererTabController::InitOptionsView(NiBaseRendererOptionsView* pkOV)
{
    NiWindowRef pDlg = pkOV->InitDialog(m_pDlgHandle);
    if (pDlg == NULL)
        return false;

    // Offset child dialog to be below renderer selection combo
    SetWindowPos(
        pDlg,
        HWND_TOP,
        0,
        m_uiChildDlgOffset,
        0,
        0,
        SWP_NOSIZE);

    return true;
}

//--------------------------------------------------------------------------------------------------
void NiRendererTabController::DeleteOptionsViews()
{
    m_kViewArray.RemoveAll();
}

//--------------------------------------------------------------------------------------------------
// Functions to create dialog and initialize controls
//--------------------------------------------------------------------------------------------------
NiWindowRef NiRendererTabController::InitDialog(
    NiRendererSettings* pkSettings,
    NiWindowRef pParentWnd)
{
    LONG_PTR pkTemp = GetWindowLongPtr(pParentWnd, GWL_HINSTANCE);
    NiInstanceRef pInstance = (NiInstanceRef)pkTemp;

    // Create dialog
    m_pDlgHandle = CreateDialog(
        pInstance,
        MAKEINTRESOURCE(IDD_RENDERERTAB),
        pParentWnd,
        RendererTabWndProc);

    if (m_pDlgHandle == NULL)
    {
        DWORD dwError = GetLastError();
        NiSettingsDialog::ReportWinAPIError(
            dwError,
            "Error in NiRendererTabController::InitDialog()\n"
            "Cannot create a dialog.");

        return NULL;
    }

    // Save our pointer for wndproc
    SetLastError(0);
    //DT32329 Casting to a LONG is not the 64-bit correct thing to do here,
    // but it silences warnings.
    SetWindowLongPtr(m_pDlgHandle, GWL_USERDATA, (LONG)(LONG_PTR)this);

    // Check for error - if class pointer was not saved correctly,
    // we'll probably receive a crash. So, exit with error code.
    DWORD dwError = GetLastError();
    if (dwError)
    {
        NiSettingsDialog::ReportWinAPIError(
            dwError,
            "Error in NiRendererTabController::InitDialog()\n"
            "Unable to save class pointer using SetWindowLongPtr().");

        // Since we used CreateDialog() to create the dialog window, we must use DestroyWindow()
        // to close it instead of EndDialog()
        DestroyWindow(m_pDlgHandle); 
        m_pDlgHandle = NULL; 

        return NULL;
    }

    NiWindowRef pRendererCombo =
        GetDlgItem(m_pDlgHandle, IDC_RENDERER_SEL_COMBO);

    // Save bottom point of renderer selection combo for child dlg repos
    RECT kRect;
    POINT kPoint;
    GetWindowRect(pRendererCombo, &kRect);
    kPoint.x = kRect.right;
    kPoint.y = kRect.bottom;
    ScreenToClient(m_pDlgHandle, &kPoint);
    m_uiChildDlgOffset = kPoint.y;

    if (!CreateOptionsViews(pkSettings))
    {
        NILOG(
            "Error in NiRendererTabController::InitDialog()\n"
            "Failed to create at least one NiRendererOptionsView class.",
            "NiRendererTabController Error");

        // Since we used CreateDialog() to create the dialog window, we must use DestroyWindow()
        // to close it instead of EndDialog()
        DestroyWindow(m_pDlgHandle); 
        m_pDlgHandle = NULL; 

        return NULL;
    }

    // Init and fill controls in this dialog
    InitDialogControls();

    return m_pDlgHandle;
}

//--------------------------------------------------------------------------------------------------
void NiRendererTabController::InitDialogControls()
{
    EE_ASSERT(m_pDlgHandle);

    // Create entries in renderer selection combo
    for (unsigned int i = 0; i < m_kViewArray.GetSize(); i++)
    {
        NiBaseRendererOptionsView* pkROV = m_kViewArray[i];

        // Insert OptionsView's name in renderer selection combobox
        LPSTR pcName = pkROV->GetName();
        SendDlgItemMessage(
            m_pDlgHandle,
            IDC_RENDERER_SEL_COMBO,
            CB_ADDSTRING,
            0,
            (LPARAM)pcName);
    }

    // Set default selection according to default renderer setting
    SendDlgItemMessage(
        m_pDlgHandle,
        IDC_RENDERER_SEL_COMBO,
        CB_SETCURSEL,
        m_uiCurrentViewIdx,
        0);

    m_kViewArray[m_uiCurrentViewIdx]->Activate();
}

//--------------------------------------------------------------------------------------------------
// Height adjusting functions for basic / advanced view
//--------------------------------------------------------------------------------------------------
unsigned int NiRendererTabController::SetBasicHeight()
{
    // Get maximum advanced OptionsView height
    unsigned int uiMaxHeight = 0;
    for (unsigned int i = 0; i < m_kViewArray.GetSize(); i++)
    {
        unsigned int uiHeight = m_kViewArray[i]->SetBasicHeight();
        if (uiHeight > uiMaxHeight)
            uiMaxHeight = uiHeight;
    }

    // Set size of control to size of OV dialog + rend sel combo
    RECT kRect;
    GetWindowRect(m_pDlgHandle, &kRect);
    SetWindowPos(
        m_pDlgHandle,
        HWND_TOP,
        0,
        0,
        kRect.right - kRect.left,
        uiMaxHeight + m_uiChildDlgOffset,
        SWP_NOMOVE);

    return uiMaxHeight + m_uiChildDlgOffset;
}

//--------------------------------------------------------------------------------------------------
unsigned int NiRendererTabController::SetAdvancedHeight()
{
    // Get maximum basic OptionsView height
    unsigned int uiMaxHeight = 0;
    for (unsigned int i = 0; i < m_kViewArray.GetSize(); i++)
    {
        unsigned int uiHeight = m_kViewArray[i]->SetAdvancedHeight();
        if (uiHeight > uiMaxHeight)
            uiMaxHeight = uiHeight;
    }

    // Set size of control to size of OV dialog + rend sel combo
    RECT kRect;
    GetWindowRect(m_pDlgHandle, &kRect);
    SetWindowPos(
        m_pDlgHandle,
        HWND_TOP,
        0,
        0,
        kRect.right - kRect.left,
        uiMaxHeight + m_uiChildDlgOffset,
        SWP_NOMOVE);

    return uiMaxHeight + m_uiChildDlgOffset;
}

//--------------------------------------------------------------------------------------------------
// Window messages processing functions
//--------------------------------------------------------------------------------------------------
bool NiRendererTabController::ProcessCommand(
    NiWindowRef,
    WORD wID,
    WORD wNotifyCode)
{
    // Process renderer type change
    if (wID == IDC_RENDERER_SEL_COMBO && wNotifyCode == CBN_SELCHANGE)
    {
        LRESULT lResult = SendDlgItemMessage(
            m_pDlgHandle,
            IDC_RENDERER_SEL_COMBO,
            CB_GETCURSEL,
            0,
            0);

        if (lResult == CB_ERR)
            return false;

        m_kViewArray[m_uiCurrentViewIdx]->Deactivate();
        m_uiCurrentViewIdx = PtrToUint(lResult);
        m_kViewArray[m_uiCurrentViewIdx]->Activate();

        return true;
    }
    return false;
}

//--------------------------------------------------------------------------------------------------
BOOL CALLBACK NiRendererTabController::RendererTabWndProc(
    HWND pDlg,
    UINT uiMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    // Get pointer to class to pass it messages
    LONG_PTR pkPtr = GetWindowLongPtr(pDlg, GWL_USERDATA);
    NiRendererTabController* pkTabCtrl =
        reinterpret_cast<NiRendererTabController*>(pkPtr);

    switch (uiMsg)
    {
    case WM_INITDIALOG:
        return TRUE;

    case WM_COMMAND:
        // Pass command to TabController and optionally then to OptionsView
        WORD wID = LOWORD(wParam);
        WORD wNotifyCode = HIWORD(wParam);
        NiWindowRef pDlgHandle = (NiWindowRef)lParam;

        if (!pkTabCtrl)
            return false;

        if (pkTabCtrl->ProcessCommand(pDlgHandle, wID, wNotifyCode))
            return TRUE;
    }

    return FALSE;
}

//--------------------------------------------------------------------------------------------------
