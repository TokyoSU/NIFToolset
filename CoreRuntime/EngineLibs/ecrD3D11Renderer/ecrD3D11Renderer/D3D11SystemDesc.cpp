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

#include "D3D11SystemDesc.h"
#include "D3D11Error.h"
#include "D3D11PixelFormat.h"
#include "D3D11Renderer.h"

using namespace ecr;

D3D11SystemDesc* D3D11SystemDesc::ms_pSystemDesc = NULL;
efd::CriticalSection D3D11SystemDesc::ms_criticalSection;

//------------------------------------------------------------------------------------------------
void D3D11SystemDesc::GetSystemDesc(D3D11SystemDescPtr& spSystemDesc)
{
    ms_criticalSection.Lock();
    if (ms_pSystemDesc)
    {
        spSystemDesc = ms_pSystemDesc;
    }
    else
    {
        spSystemDesc = EE_NEW D3D11SystemDesc;
        ms_pSystemDesc = spSystemDesc;
    }
    ms_criticalSection.Unlock();
}

//------------------------------------------------------------------------------------------------
D3D11SystemDesc::D3D11SystemDesc() :
    m_hDXGI(NULL),
    m_pCreateDXGIFactoryFunction(NULL),
    m_pFactory(NULL),
    m_isEnumerationValid(false)
{
    if (LoadDXGI() == false)
    {
        // Error should have already been reported by this point, and
        // the library released.
        return;
    }

    if (CreateFactory() == false)
    {
        // Error should have already been reported by this point, and the
        // factory released.
        ReleaseDXGI();
        return;
    }

    if (EnumerateAdapters() == false)
    {
        // Error should have already been reported by this point, but
        // library and factory remain so enumeration can be attempted again.
        return;
    }
}

//------------------------------------------------------------------------------------------------
D3D11SystemDesc::~D3D11SystemDesc()
{
    ms_criticalSection.Lock();
    ms_pSystemDesc = NULL;
    ms_criticalSection.Unlock();

    ReleaseAdapters();

    ReleaseFactory();

    ReleaseDXGI();
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11SystemDesc::IsEnumerationValid() const
{
    return m_isEnumerationValid;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11SystemDesc::Enumerate()
{
    ReleaseAdapters();
    EnumerateAdapters();

    return m_isEnumerationValid;
}

//------------------------------------------------------------------------------------------------
efd::UInt32 D3D11SystemDesc::GetAdapterCount() const
{
    return m_adapters.GetEffectiveSize();
}

//------------------------------------------------------------------------------------------------
const D3D11AdapterDesc* D3D11SystemDesc::GetAdapterDesc(efd::UInt32 index) const
{
    // Effective size must equal actual size, since we only either
    // add all adapters to an empty array or remove all adapters at once.
    EE_ASSERT(m_adapters.GetEffectiveSize() == m_adapters.GetSize());
    if (index < m_adapters.GetSize())
        return m_adapters.GetAt(index);
    else
        return NULL;
}

//------------------------------------------------------------------------------------------------
IDXGIFactory* D3D11SystemDesc::GetFactory() const
{
    return m_pFactory;
}

//------------------------------------------------------------------------------------------------
IDXGIAdapter* D3D11SystemDesc::GetAdapter(efd::UInt32 index) const
{
    IDXGIAdapter* pAdapter = NULL;
    HRESULT hr = m_pFactory->EnumAdapters(index, &pAdapter);
    if (SUCCEEDED(hr))
        return pAdapter;
    else if (pAdapter)
        pAdapter->Release();
    return NULL;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11SystemDesc::LoadDXGI()
{
    ms_criticalSection.Lock();

    if (m_hDXGI != NULL)
    {
        ms_criticalSection.Unlock();
        return true;
    }

    m_hDXGI = LoadLibrary("DXGI.dll");

    if (m_hDXGI == NULL)
    {
        D3D11Error::ReportError(
            D3D11Error::D3D11ERROR_DXGI_LIB_MISSING);
        ms_criticalSection.Unlock();
        return false;
    }

    m_pCreateDXGIFactoryFunction = (EE_LPCREATEDXGIFACTORY)
        GetProcAddress(m_hDXGI, "CreateDXGIFactory");
    if (m_pCreateDXGIFactoryFunction == NULL)
    {
        D3D11Error::ReportError(D3D11Error::D3D11ERROR_DXGI_LIB_MISSING,
            "Library loaded but CreateDXGIFactory procedure not found; "
            "releasing library.");
        ms_criticalSection.Unlock();

        ReleaseDXGI();
        return false;
    }

    ms_criticalSection.Unlock();

    return true;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11SystemDesc::ReleaseDXGI()
{
    ms_criticalSection.Lock();
    if (m_hDXGI)
    {
        FreeLibrary(m_hDXGI);
        m_hDXGI = NULL;
        m_pCreateDXGIFactoryFunction = NULL;
    }

    ms_criticalSection.Unlock();
    return true;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11SystemDesc::CreateFactory()
{
    EE_ASSERT(m_pCreateDXGIFactoryFunction);

    HRESULT hr = m_pCreateDXGIFactoryFunction(__uuidof(IDXGIFactory), (void**)&m_pFactory);

    if (FAILED(hr) || m_pFactory == NULL)
    {
        if (FAILED(hr))
        {
            D3D11Error::ReportError(
                D3D11Error::D3D11ERROR_FACTORY_CREATION_FAILED,
                "Error HRESULT = 0x%08X.", 
                (efd::UInt32)hr);
        }
        else
        {
            D3D11Error::ReportError(
                D3D11Error::D3D11ERROR_FACTORY_CREATION_FAILED,
                "No error message from DXGI, but factory is NULL.");
        }

        ReleaseFactory();
        return false;
    }

    return true;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11SystemDesc::ReleaseFactory()
{
    if (m_pFactory)
    {
        m_pFactory->Release();
        m_pFactory = NULL;
    }
    return true;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11SystemDesc::EnumerateAdapters()
{
    EE_ASSERT(m_adapters.GetEffectiveSize() == 0);
    m_isEnumerationValid = false;

    if (!m_pFactory)
        return false;

    efd::UInt32 adapterIndex = 0;
    for (; ;)
    {
        IDXGIAdapter* pAdapter = NULL;
        HRESULT hr =
            m_pFactory->EnumAdapters(adapterIndex, &pAdapter);
        if (FAILED(hr))
        {
            if (hr != DXGI_ERROR_NOT_FOUND)
            {
                D3D11Error::ReportWarning(
                    "IDXGIFactory::EnumAdapters failed with error HRESULT = 0x%08X.",
                    (efd::UInt32)hr);
            }
            break;
        }

        D3D11AdapterDesc* pAdapterDesc = EE_NEW D3D11AdapterDesc;
        pAdapterDesc->m_index = adapterIndex;
        hr = pAdapter->GetDesc(&(pAdapterDesc->m_adapterDesc));

        adapterIndex++;

        if (FAILED(hr))
        {
            D3D11Error::ReportWarning(
                "IDXGIAdapter::GetDesc failed with error HRESULT = 0x%08X; "
                "deleting adapter descriptor.",
                (efd::UInt32)hr);

            EE_DELETE pAdapterDesc;
            continue;
        }

        if (pAdapterDesc->EnumerateDevices(m_pFactory) == false)
        {
            D3D11Error::ReportWarning(
                "D3D11AdapterDesc::EnumerateDevices failed; deleting adapter descriptor.");
            EE_DELETE pAdapterDesc;
            continue;
        }

        if (pAdapterDesc->EnumerateOutputs(m_pFactory) == false)
        {
            // EnumerateOutputs only fails on a NULL factory or a real API error;
            // a zero-output (headless) adapter returns true and is kept.
            D3D11Error::ReportWarning(
                "D3D11AdapterDesc::EnumerateOutputs failed; deleting adapter descriptor.");
            EE_DELETE pAdapterDesc;
            continue;
        }

        m_adapters.Add(pAdapterDesc);
    }

    if (m_adapters.GetSize() > 0)
        m_isEnumerationValid = true;

    return m_isEnumerationValid;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11SystemDesc::ReleaseAdapters()
{
    m_isEnumerationValid = false;

    efd::UInt32 adapterCount = m_adapters.GetSize();
    for (efd::UInt32 i = 0; i < adapterCount; i++)
    {
        D3D11AdapterDesc* pkAdapterDesc = m_adapters.GetAt(i);
        EE_DELETE pkAdapterDesc;
        m_adapters.RemoveAt(i);
    }

    return true;
}

//------------------------------------------------------------------------------------------------
D3D11AdapterDesc::D3D11AdapterDesc() :
    m_pHWDevice(NULL),
    m_pRefDevice(NULL),
    m_pWARPDevice(NULL),
    m_index(UINT_MAX),
    m_isPerfHUD(false)
{
    memset(&m_adapterDesc, 0, sizeof(m_adapterDesc));
}

//------------------------------------------------------------------------------------------------
D3D11AdapterDesc::~D3D11AdapterDesc()
{
    efd::UInt32 outputCount = m_outputs.GetSize();
    for (efd::UInt32 i = 0; i < outputCount; i++)
    {
        D3D11OutputDesc* pOutputDesc = m_outputs.GetAt(i);
        EE_DELETE pOutputDesc;
    }

    EE_DELETE m_pHWDevice;
    EE_DELETE m_pRefDevice;
    EE_DELETE m_pWARPDevice;
}

//------------------------------------------------------------------------------------------------
const D3D11DeviceDesc* D3D11AdapterDesc::GetHWDevice() const
{
    // For PerfHUD adapter, "ref" device is actually HW.
    return (m_isPerfHUD ? m_pRefDevice : m_pHWDevice);
}

//------------------------------------------------------------------------------------------------
const D3D11DeviceDesc* D3D11AdapterDesc::GetRefDevice() const
{
    return m_pRefDevice;
}

//------------------------------------------------------------------------------------------------
const D3D11DeviceDesc* D3D11AdapterDesc::GetWARPDevice() const
{
    return m_pWARPDevice;
}

//------------------------------------------------------------------------------------------------
efd::UInt32 D3D11AdapterDesc::GetOutputCount() const
{
    return m_outputs.GetEffectiveSize();
}

//------------------------------------------------------------------------------------------------
const D3D11OutputDesc* D3D11AdapterDesc::GetOutputDesc(efd::UInt32 index) const
{
    // Effective size must equal actual size, since we only either
    // add all outputs to an empty array or remove all outputs at once.
    EE_ASSERT(m_outputs.GetEffectiveSize() == m_outputs.GetSize());
    if (index < m_outputs.GetSize())
        return m_outputs.GetAt(index);
    else
        return NULL;
}

//------------------------------------------------------------------------------------------------
const DXGI_ADAPTER_DESC* D3D11AdapterDesc::GetDesc() const
{
    return &m_adapterDesc;
}

//------------------------------------------------------------------------------------------------
efd::UInt32 D3D11AdapterDesc::GetIndex() const
{
    return m_index;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11AdapterDesc::IsPerfHUD() const
{
    return m_isPerfHUD;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11AdapterDesc::EnumerateDevices(IDXGIFactory* pFactory)
{
    if (pFactory == NULL)
    {
        D3D11Error::ReportWarning("Can't call EnumerateDevices without "
            "a valid factory.");
        return false;
    }

    // Description is WCHAR, so wcsstr must be used instead of strstr.
    m_isPerfHUD =
        (wcsstr(m_adapterDesc.Description, L"PerfHUD") != NULL);

    EE_ASSERT(m_pHWDevice == NULL && m_pRefDevice == NULL &&
        m_pWARPDevice == NULL);

    const D3D_DRIVER_TYPE devTypeArray[] =
    {
        D3D_DRIVER_TYPE_HARDWARE,
        D3D_DRIVER_TYPE_REFERENCE,
        //D3D_DRIVER_TYPE_NULL,
        //D3D_DRIVER_TYPE_SOFTWARE,
        D3D_DRIVER_TYPE_WARP
    };
    const efd::UInt32 uiDevTypeArrayCount =
        sizeof(devTypeArray) / sizeof(*devTypeArray);

    for (efd::UInt32 i = 0; i < uiDevTypeArrayCount; i++)
    {
        D3D_DRIVER_TYPE driverType = devTypeArray[i];
        IDXGIAdapter* pAdapter = NULL;

        // PerfHUD adapter needs to be Reference, and needs the adapter to be
        // specified.
        if (m_isPerfHUD && (driverType != D3D_DRIVER_TYPE_REFERENCE))
            continue;

        D3D_FEATURE_LEVEL featureLevel;
        ID3D11Device* pD3DDevice = NULL;
        ID3D11DeviceContext* pDeviceContext = NULL;
        D3D11Renderer::CreateTempDevice(
            pAdapter, 
            driverType, 
            0, 
            featureLevel,
            pD3DDevice,
            pDeviceContext);
        if (pD3DDevice == NULL)
        {
            EE_ASSERT(pDeviceContext == NULL);
            continue;
        }
        EE_ASSERT(pDeviceContext != NULL);

        D3D11DeviceDesc* pDeviceDesc = EE_NEW D3D11DeviceDesc;
        pDeviceDesc->m_deviceType = driverType;
        pDeviceDesc->m_pAdapter = this;
        pDeviceDesc->m_featureLevel = featureLevel;

        efd::UInt32* pQualityDest = pDeviceDesc->m_msQualities;
        for (efd::UInt32 j = 1; j <= D3D11_MAX_MULTISAMPLE_SAMPLE_COUNT; j++)
        {
            HRESULT hr = pD3DDevice->CheckMultisampleQualityLevels(
                DXGI_FORMAT_R8G8B8A8_UNORM, j, pQualityDest);
            if (FAILED(hr))
                *pQualityDest = 0;
            else
                pDeviceDesc->m_highestMultisampleCount = j;
            pQualityDest++;
        }

        pD3DDevice->Release();
        pDeviceContext->Release();

        if (driverType == D3D_DRIVER_TYPE_HARDWARE)
        {
            m_pHWDevice = pDeviceDesc;
        }
        else if (driverType == D3D_DRIVER_TYPE_REFERENCE)
        {
            m_pRefDevice = pDeviceDesc;
        }
        else if (driverType == D3D_DRIVER_TYPE_WARP)
        {
            m_pWARPDevice = pDeviceDesc;
        }
    }

    return (m_pHWDevice != NULL || m_pRefDevice != NULL ||
        m_pWARPDevice != NULL);
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11AdapterDesc::EnumerateOutputs(IDXGIFactory* pFactory)
{
    if (pFactory == NULL)
    {
        D3D11Error::ReportWarning("Can't call EnumerateOutputs without "
            "a valid factory.");
        return false;
    }

    EE_ASSERT(m_outputs.GetEffectiveSize() == 0);

    efd::UInt32 uiOutputIndex = 0;
    for (; ;)
    {
        IDXGIOutput* pOutput = NULL;
        IDXGIAdapter* pAdapter = NULL;
        HRESULT hr = pFactory->EnumAdapters(m_index, &pAdapter);
        EE_ASSERT(SUCCEEDED(hr) && pAdapter);
        hr = pAdapter->EnumOutputs(uiOutputIndex, &pOutput);
        pAdapter->Release();
        if (FAILED(hr))
        {
            if (hr != DXGI_ERROR_NOT_FOUND)
            {
                D3D11Error::ReportWarning(
                    "IDXGIAdapter::EnumOutputs failed with error HRESULT = 0x%08X.",
                    (efd::UInt32)hr);
            }
            break;
        }

        D3D11OutputDesc* pOutputDesc = EE_NEW D3D11OutputDesc;
        pOutputDesc->m_index = uiOutputIndex;
        pOutputDesc->m_pAdapter = this;
        pOutputDesc->m_pOutput = pOutput;
        hr = pOutput->GetDesc(&(pOutputDesc->m_outputDesc));
        uiOutputIndex++;

        if (FAILED(hr))
        {
            D3D11Error::ReportWarning(
                "IDXGIOutput::GetDesc failed with error HRESULT = 0x%08X; "
                "deleting output descriptor.",
                (efd::UInt32)hr);

            EE_DELETE pOutputDesc;
            continue;
        }

        //Enumerate display modes
        if (pOutputDesc->EnumerateDisplayModes())
        {
            m_outputs.Add(pOutputDesc);
        }
        else
        {
            D3D11Error::ReportWarning(
                "D3D11OutputDesc::EnumerateDisplayModes failed; deleting output descriptor.");
            EE_DELETE pOutputDesc;
        }
    }

    // Zero outputs is not an error — the adapter may be a render-only GPU that
    // drives no display directly (e.g. Optimus / hybrid-graphics dGPU, headless
    // compute card).  Windowed-mode presentation works fine without outputs.
    // Fullscreen presentation checks GetOutputCount() before accessing outputs.
    return true;
}

//------------------------------------------------------------------------------------------------
D3D11DeviceDesc::D3D11DeviceDesc() :
    m_pAdapter(NULL),
    m_deviceType(D3D_DRIVER_TYPE_NULL),
    m_featureLevel(D3D_FEATURE_LEVEL_9_1),
    m_highestMultisampleCount(0)
{
    memset(m_msQualities, 0, sizeof(m_msQualities));
}

//------------------------------------------------------------------------------------------------
D3D11DeviceDesc::~D3D11DeviceDesc()
{
    /* */
}

//------------------------------------------------------------------------------------------------
D3D_DRIVER_TYPE D3D11DeviceDesc::GetDeviceType() const
{
    return m_deviceType;
}

//------------------------------------------------------------------------------------------------
D3D_FEATURE_LEVEL D3D11DeviceDesc::GetFeatureLevel() const
{
    return m_featureLevel;
}

//------------------------------------------------------------------------------------------------
efd::UInt32 D3D11DeviceDesc::GetHighestMultisampleCount() const
{
    return m_highestMultisampleCount;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11DeviceDesc::GetMultisampleSupport(efd::UInt32 samples,
    efd::UInt32& qualityLevels) const
{
    qualityLevels = 0;
    if (samples > 0 && samples <= D3D11_MAX_MULTISAMPLE_SAMPLE_COUNT)
    {
        qualityLevels = m_msQualities[samples - 1];
    }
    return (qualityLevels != 0);
}

//------------------------------------------------------------------------------------------------
D3D11OutputDesc::D3D11OutputDesc() :
    m_index(0),
    m_pAdapter(NULL),
    m_pOutput(NULL),
    m_pDisplayModes(NULL),
    m_displayModeCount(0)
{
    memset(&m_outputDesc, 0, sizeof(m_outputDesc));
}

//------------------------------------------------------------------------------------------------
D3D11OutputDesc::~D3D11OutputDesc()
{
    if (m_pOutput)
        m_pOutput->Release();
    EE_FREE(m_pDisplayModes);
}

//------------------------------------------------------------------------------------------------
const DXGI_OUTPUT_DESC* D3D11OutputDesc::GetDesc() const
{
    return &m_outputDesc;
}

//------------------------------------------------------------------------------------------------
efd::UInt32 D3D11OutputDesc::GetDisplayModeCount() const
{
    return m_displayModeCount;
}

//------------------------------------------------------------------------------------------------
const DXGI_MODE_DESC* D3D11OutputDesc::GetDisplayModeArray() const
{
    return m_pDisplayModes;
}

//------------------------------------------------------------------------------------------------
IDXGIOutput* D3D11OutputDesc::GetOutput() const
{
    return m_pOutput;
}

//------------------------------------------------------------------------------------------------
efd::UInt32 D3D11OutputDesc::GetIndex() const
{
    return m_index;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11OutputDesc::EnumerateDisplayModes()
{
    EE_ASSERT(m_pDisplayModes == NULL);

    // Some formats in this list come from DXUT
    // Some are obtained via running on down-level hardware
    const DXGI_FORMAT allowedAdapterFormatArray[] =
    {
        DXGI_FORMAT_R8G8B8A8_UNORM,
        DXGI_FORMAT_B8G8R8A8_UNORM,
        DXGI_FORMAT_B8G8R8X8_UNORM,
        DXGI_FORMAT_R16G16B16A16_FLOAT,
        DXGI_FORMAT_R10G10B10A2_UNORM,
        DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
        DXGI_FORMAT_B5G5R5A1_UNORM
    };

    const efd::UInt32 allowedAdapterFormatArrayCount =
        sizeof(allowedAdapterFormatArray) /
        sizeof(*allowedAdapterFormatArray);

    efd::UInt32 totalModeCount = 0;
    efd::UInt32 modeCountArray[allowedAdapterFormatArrayCount];

    efd::UInt32 i = 0;
    for (; i < allowedAdapterFormatArrayCount; i++)
    {
        DXGI_FORMAT format = allowedAdapterFormatArray[i];

        efd::UInt32 modeCount = 0;

        HRESULT hr = m_pOutput->GetDisplayModeList(format, 0, &modeCount,
            NULL);

        if (FAILED(hr))
        {
            if (hr != DXGI_ERROR_NOT_FOUND)
            {
                D3D11Error::ReportWarning(
                    "IDXGIOutput::GetDisplayModeList failed with error "
                    "HRESULT = 0x%08X.", 
                    (efd::UInt32)hr);
            }
            continue;
        }

        totalModeCount += modeCount;
        modeCountArray[i] = modeCount;
    }

    if (totalModeCount == 0)
        return false;

    m_pDisplayModes = EE_ALLOC(DXGI_MODE_DESC, totalModeCount);
    EE_ASSERT(m_pDisplayModes);

    DXGI_MODE_DESC* pIterator = m_pDisplayModes;
    for (i = 0; i < allowedAdapterFormatArrayCount; i++)
    {
        efd::UInt32 modeCount = modeCountArray[i];
        if (modeCount == 0)
            continue;

        DXGI_FORMAT format = allowedAdapterFormatArray[i];

        HRESULT hr = m_pOutput->GetDisplayModeList(
            format, 
            0, 
            &modeCount,
            pIterator);

        if (FAILED(hr) || modeCount != modeCountArray[i])
        {
            if (FAILED(hr))
            {
                D3D11Error::ReportWarning(
                    "IDXGIOutput::GetDisplayModeList failed with error HRESULT = 0x%08X.", 
                    (efd::UInt32)hr);
            }
            else
            {
                D3D11Error::ReportWarning(
                    "IDXGIOutput::GetDisplayModeList returned different mode counts on "
                    "subsequent calls for format %s",
                    D3D11PixelFormat::GetFormatName(format, false));
            }

            EE_FREE(m_pDisplayModes);
            m_pDisplayModes = NULL;
            return false;
        }

        pIterator += modeCount;
    }

    m_displayModeCount = totalModeCount;
    return true;
}

//------------------------------------------------------------------------------------------------
