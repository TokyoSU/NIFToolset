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

#include "D3D11DataStream.h"

#include "D3D11DataStreamFactory.h"
#include "D3D11Error.h"
#include "D3D11Renderer.h"
#include "D3D11ResourceManager.h"

using namespace ecr;

NiImplementRTTI(D3D11DataStream, NiDataStream, NiTypeMask::D3D11DataStream);

//------------------------------------------------------------------------------------------------
D3D11DataStream::D3D11DataStream(
    const NiDataStreamElementSet& elements,
    efd::UInt32 count, 
    efd::UInt8 accessMask, 
    Usage usage,
    D3D11_USAGE d3d11Usage, 
    efd::UInt32 bindFlags,
    efd::UInt32 cpuAccessFlags) :
    NiDataStream(elements, count, accessMask, usage),
    m_pD3D11Buffer(NULL),
    m_pBackingBuffer(NULL),
    m_d3d11Usage(d3d11Usage),
    m_d3d11BindFlags(bindFlags),
    m_d3d11CPUAccessFlags(cpuAccessFlags),
    m_dirty(true),
    m_hidingBufferInBackingBuffer(false)
{
    Allocate();
}

//------------------------------------------------------------------------------------------------
D3D11DataStream::D3D11DataStream(
    efd::UInt8 accessMask, 
    Usage usage,
    D3D11_USAGE d3d11Usage, 
    efd::UInt32 bindFlags,
    efd::UInt32 cpuAccessFlags) :
    NiDataStream(accessMask, usage),
    m_pD3D11Buffer(NULL),
    m_pBackingBuffer(NULL),
    m_d3d11Usage(d3d11Usage),
    m_d3d11BindFlags(bindFlags),
    m_d3d11CPUAccessFlags(cpuAccessFlags),
    m_dirty(true),
    m_hidingBufferInBackingBuffer(false)
{
    /* */
}

//------------------------------------------------------------------------------------------------
D3D11DataStream::~D3D11DataStream()
{
    Deallocate();
}

//------------------------------------------------------------------------------------------------
D3D11DataStream* D3D11DataStream::Create(ID3D11Buffer* pBuffer)
{
    EE_ASSERT(pBuffer);

    D3D11_BUFFER_DESC bufferDesc;
    pBuffer->GetDesc(&bufferDesc);

    Usage usage;
    efd::UInt8 accessMask;
    if (InterpretD3D11Parameters(
        bufferDesc.Usage, 
        bufferDesc.BindFlags,
        bufferDesc.CPUAccessFlags, 
        usage, 
        accessMask) == false)
    {
        D3D11Error::ReportWarning(
            "Unexpected values found in D3D11_BUFFER_DESC in "
            __FUNCTION__
            ". Usage = 0x%08X, BindFlags = 0x%08X, CPUAccessFlags = 0x%08X\n",
            bufferDesc.Usage, 
            bufferDesc.BindFlags,
            bufferDesc.CPUAccessFlags);
        return NULL;
    }

    D3D11DataStream* pkDataStream = (D3D11DataStream*)CreateDataStream(
        accessMask, 
        usage, 
        false, 
        false);

    EE_ASSERT(pkDataStream->m_uiAccessMask == accessMask);
    EE_ASSERT(pkDataStream->m_d3d11Usage == bufferDesc.Usage);
    EE_ASSERT(pkDataStream->m_d3d11BindFlags == bufferDesc.BindFlags);
    EE_ASSERT(pkDataStream->m_d3d11CPUAccessFlags == bufferDesc.CPUAccessFlags);

    pkDataStream->m_pD3D11Buffer = pBuffer;
    pkDataStream->m_pD3D11Buffer->AddRef();
    pkDataStream->m_uiSize = bufferDesc.ByteWidth;

    // Local buffers will be created for all Volatile and Mutable buffers, or
    // for buffers that don't get D3D11 buffers.
    if (NeedsLocalBuffer(pkDataStream->m_uiAccessMask))
    {
        EE_ASSERT(pkDataStream->m_pBackingBuffer == NULL &&
            !pkDataStream->m_hidingBufferInBackingBuffer);
        pkDataStream->m_pBackingBuffer = EE_ALLOC(efd::UInt8, pkDataStream->m_uiSize);
    }

    return pkDataStream;
}

//------------------------------------------------------------------------------------------------
void D3D11DataStream::Resize(efd::UInt32 size)
{
    Deallocate();

    m_uiSize = size;

    Allocate();
}

//------------------------------------------------------------------------------------------------
void D3D11DataStream::Allocate()
{
    // Check for existence of buffer already
    if (m_pD3D11Buffer != NULL || m_pBackingBuffer != NULL)
        return;

    EE_ASSERT(D3D11Renderer::GetRenderer());

    // Assertions:
    // - USER buffers and DISPLAYLISTS have bind flags of 0.
    EE_ASSERT((GetUsage() != USAGE_USER && GetUsage() != USAGE_DISPLAYLIST) || 
        (m_d3d11BindFlags == 0));
    // - Anything with GPU_READ has a non-zero bind flag
    EE_ASSERT(((m_uiAccessMask & ACCESS_GPU_READ) == 0) || (m_d3d11BindFlags != 0));
    // - USER buffers or DISPLAYLISTS
    //   are (CPU_READ | CPU_WRITE_STATIC) or
    //   (CPU_READ | CPU_WRITE_MUTABLE).
    EE_ASSERT((GetUsage() != USAGE_USER && GetUsage() != USAGE_DISPLAYLIST) ||
        (m_uiAccessMask == (ACCESS_CPU_READ | ACCESS_CPU_WRITE_STATIC) ||
        m_uiAccessMask == (ACCESS_CPU_READ | ACCESS_CPU_WRITE_MUTABLE)) ||
        (GetUsage() == USAGE_DISPLAYLIST));
    // - STAGING buffers have bind flags of 0
    //   and others have non-0 bind flags
    EE_ASSERT((m_d3d11Usage == D3D11_USAGE_STAGING) == (m_d3d11BindFlags == 0));

    // D3D11 buffers will not be created here - Only the CPU buffers.
    EE_ASSERT(m_pBackingBuffer == NULL && !m_hidingBufferInBackingBuffer);
    m_pBackingBuffer = EE_ALLOC(efd::UInt8, m_uiSize);
}

//------------------------------------------------------------------------------------------------
void D3D11DataStream::Deallocate()
{
    EE_ASSERT(!m_hidingBufferInBackingBuffer);
    if (m_pBackingBuffer)
    {
        EE_FREE(m_pBackingBuffer);
        m_pBackingBuffer = NULL;
    }

    if (m_pD3D11Buffer)
    {
        m_pD3D11Buffer->Release();
        m_pD3D11Buffer = NULL;
    }
}

//------------------------------------------------------------------------------------------------
void D3D11DataStream::D3D11Allocate(efd::Bool initialize)
{
    D3D11ResourceManager* pResourceManager =
        D3D11Renderer::GetRenderer()->GetResourceManager();
    EE_ASSERT(pResourceManager);

    // D3D11 buffers will be created for all buffers except CPU-only buffers,
    // which have 0 bind flags.
    if (m_d3d11BindFlags == 0)
        return;

    // Check for already-allocated buffers
    if (m_pD3D11Buffer)
        return;

    D3D11_SUBRESOURCE_DATA kData;
    if (initialize)
    {
        EE_ASSERT(!m_hidingBufferInBackingBuffer);
        kData.pSysMem = m_pBackingBuffer;

        // D3D11 docs recommendation:
        // If the resource being created does not require SysMemPitch
        // or SysMemSlicePitch, then it may be useful to use those
        // members when debugging by specifying the total size of the
        // resource in bytes.
        kData.SysMemPitch = m_uiSize;
        kData.SysMemSlicePitch = m_uiSize;
    }

    m_pD3D11Buffer = pResourceManager->CreateBuffer(
        m_uiSize,
        m_d3d11BindFlags, 
        m_d3d11Usage, 
        m_d3d11CPUAccessFlags,
        0,
        0,
        (initialize ? &kData : NULL));

    if (m_pD3D11Buffer == NULL)
    {
        // Resource creation would have thrown error message
        D3D11Error::ReportWarning("%s failed because buffer could not be created.", __FUNCTION__);
    }
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11DataStream::InterpretDataStreamFlags(
    efd::UInt8& accessMask,
    NiDataStream::Usage usage, 
    D3D11_USAGE& d3d11Usage,
    efd::UInt32& bindFlags, 
    efd::UInt32& cpuAccessFlags)
{
    d3d11Usage = D3D11_USAGE_STAGING;
    cpuAccessFlags = 0;

    // Vertex, index, and constant buffers must have the GPU_ACCESS_READ flag
    // set, while other buffers must not have that flag set.
    if (usage == USAGE_VERTEX ||
        usage == USAGE_VERTEX_INDEX ||
        usage == USAGE_SHADERCONSTANT)
    {
        accessMask |= ACCESS_GPU_READ;
    }
    else
    {
        accessMask &= ~ACCESS_GPU_READ;
    }

    // Interpret Usage parameter
    bindFlags = InterpretUsage(usage, accessMask);

    // Check validity of access mask
    if (!InterpretAccessMask(accessMask, d3d11Usage))
        return false;

    // USER buffers are CPU-only, and cannot have GPU_READ set on them.
    EE_ASSERT((usage != USAGE_USER) || ((accessMask & ACCESS_GPU_READ) == 0));

    // Other buffers may have GPU_READ set on them, but it is not required.
    // If they do have GPU_READ, then they must have non-0 bind flags.
    EE_ASSERT(((accessMask & ACCESS_GPU_READ) != 0) == (bindFlags != 0));

    // STAGING will be returned for actual STAGING buffers or for local
    // CPU-only non-D3D11 buffers. The way to differentiate is whether the
    // access flags have any CPU_WRITE flags set - those are non-D3D11 buffers.

    if ((d3d11Usage != D3D11_USAGE_STAGING) || ((accessMask & ACCESS_CPU_WRITE_ANY) == 0))
    {
        // Must be a GPU buffer, since the CPU can't write to it.
        if (bindFlags == 0)
            return false;

        // Get valid CPUAccess flags
        if (d3d11Usage == D3D11_USAGE_DYNAMIC)
        {
            cpuAccessFlags = D3D11_CPU_ACCESS_WRITE;
        }
        else if (d3d11Usage == D3D11_USAGE_STAGING)
        {
            cpuAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;
        }
    }
    else
    {
        // CPU buffer
        EE_ASSERT(bindFlags == 0);
        if ((accessMask & ACCESS_CPU_WRITE_MUTABLE) != 0)
            cpuAccessFlags |= D3D11_CPU_ACCESS_WRITE;
        if ((accessMask & ACCESS_CPU_READ) != 0)
            cpuAccessFlags |= D3D11_CPU_ACCESS_READ;
    }

    return true;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11DataStream::InterpretAccessMask(efd::UInt8 accessMask, D3D11_USAGE& usage)
{
    if ((accessMask & ACCESS_GPU_READ) == 0)
    {
        // No GPU read - treat as STAGING or local buffer

        // Only certain combinations should have gotten through
        // IsAccessRequestValid
        if (accessMask != ACCESS_CPU_READ &&
            accessMask != (ACCESS_CPU_READ | ACCESS_CPU_WRITE_STATIC) &&
            accessMask != (ACCESS_CPU_READ | ACCESS_CPU_WRITE_MUTABLE))
        {
            return false;
        }

        usage = D3D11_USAGE_STAGING;
        return true;
    }

    if ((accessMask & ACCESS_CPU_WRITE_VOLATILE) != 0)
    {
        // Volatile - treat as DYNAMIC buffer.

        // Only certain combinations should have gotten through
        // IsAccessRequestValid
        if (accessMask != (ACCESS_CPU_WRITE_VOLATILE | ACCESS_GPU_READ))
            return false;

        usage = D3D11_USAGE_DYNAMIC;
        return true;
    }

    if ((accessMask & ACCESS_CPU_WRITE_STATIC) != 0)
    {
        // Static - treat as IMMUTABLE buffer

        // Only certain combinations should have gotten through
        // IsAccessRequestValid
        if (accessMask != (ACCESS_CPU_WRITE_STATIC | ACCESS_GPU_READ) &&
            accessMask != (ACCESS_CPU_READ | ACCESS_CPU_WRITE_STATIC | ACCESS_GPU_READ))
        {
            return false;
        }

        usage = D3D11_USAGE_IMMUTABLE;
        return true;
    }

    // Mutable - treat as DEFAULT buffer.

    // Special case: certain combinations are not useful as there is no way to
    // fill them with data
    if (accessMask == ACCESS_GPU_READ ||
        accessMask == (ACCESS_CPU_READ | ACCESS_GPU_READ))
    {
        return false;
    }

    // Only certain combinations should have gotten through to this point
    if (accessMask != (ACCESS_GPU_READ | ACCESS_GPU_WRITE) &&
        accessMask != (ACCESS_CPU_WRITE_MUTABLE | ACCESS_GPU_READ | ACCESS_GPU_WRITE) &&
        accessMask != (ACCESS_CPU_WRITE_MUTABLE | ACCESS_GPU_READ) &&
        accessMask != (ACCESS_CPU_READ | ACCESS_CPU_WRITE_MUTABLE | ACCESS_GPU_READ))
    {
        return false;
    }

    usage = D3D11_USAGE_DEFAULT;
    return true;
}

//------------------------------------------------------------------------------------------------
efd::UInt32 D3D11DataStream::InterpretUsage(NiDataStream::Usage usage, efd::UInt32 accessMask)
{
    efd::UInt32 bindFlags = 0;
    if ((accessMask & ACCESS_GPU_READ) != 0)
    {
        switch (usage)
        {
        case USAGE_VERTEX_INDEX:
            bindFlags = D3D11_BIND_INDEX_BUFFER;
            break;
        case USAGE_VERTEX:
            bindFlags = D3D11_BIND_VERTEX_BUFFER;
            break;
        case USAGE_SHADERCONSTANT:
            bindFlags = D3D11_BIND_CONSTANT_BUFFER;
            break;
        case USAGE_DISPLAYLIST:
            // USAGE_DISPLAYLISTS can't be bound to pipeline.
        default:
            // USAGE_USER can't be bound to pipeline.
            break;
        }
    }

    if ((accessMask & ACCESS_GPU_WRITE) != 0)
    {
        // Support for Stream Output:
        bindFlags |= D3D11_BIND_STREAM_OUTPUT;
        if ((accessMask & ACCESS_GPU_READ) != 0)
            bindFlags |= D3D11_BIND_SHADER_RESOURCE;
    }

    return bindFlags;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11DataStream::InterpretD3D11Parameters(
    D3D11_USAGE d3d11Usage,
    efd::UInt32 d3d11BindFlags, 
    efd::UInt32 d3d11CPUAccessFlags,
    NiDataStream::Usage& usage, 
    efd::UInt8& accessMask)
{
    if ((d3d11BindFlags & D3D11_BIND_INDEX_BUFFER) != 0)
    {
        usage = USAGE_VERTEX_INDEX;
    }
    else if ((d3d11BindFlags & D3D11_BIND_VERTEX_BUFFER) != 0)
    {
        usage = USAGE_VERTEX;
    }
    else if ((d3d11BindFlags & D3D11_BIND_CONSTANT_BUFFER) != 0)
    {
        usage = USAGE_SHADERCONSTANT;
    }
    else
    {
        usage = USAGE_USER;
    }

    if (d3d11Usage == D3D11_USAGE_DYNAMIC)
    {
        accessMask = ACCESS_GPU_READ | ACCESS_CPU_WRITE_VOLATILE;
        // Dynamic buffers must not be readable.
        if ((d3d11CPUAccessFlags & D3D11_CPU_ACCESS_READ) != 0)
            return false;
        // Though it's not clear what it means for them to not be writable...
        if ((d3d11CPUAccessFlags & D3D11_CPU_ACCESS_WRITE) == 0)
            return false;
    }
    else if (d3d11Usage == D3D11_USAGE_IMMUTABLE)
    {
        accessMask = ACCESS_GPU_READ |
            ACCESS_CPU_WRITE_STATIC |
            ACCESS_CPU_WRITE_STATIC_INITIALIZED;
        // IMMUTABLE buffers can't be readable or writable, but the
        // ACCESS_CPU_WRITE_STATIC and ACCESS_CPU_WRITE_STATIC_INITIALIZED
        // access flags are still set to indicate that the IMMUTABLE buffers
        // have already been updated.
        if ((d3d11CPUAccessFlags & (D3D11_CPU_ACCESS_WRITE | D3D11_CPU_ACCESS_READ)) != 0)
            return false;
    }
    else if (d3d11Usage == D3D11_USAGE_DEFAULT)
    {
        accessMask = ACCESS_GPU_READ | ACCESS_CPU_WRITE_MUTABLE;
        // Default buffers can't be readable or writable, but the
        // ACCESS_CPU_WRITE_MUTABLE access flag is still set to indicate that
        // Gamebryo will allow writes (through the subresource update).
        if ((d3d11CPUAccessFlags & (D3D11_CPU_ACCESS_WRITE | D3D11_CPU_ACCESS_READ)) != 0)
            return false;
    }
    else
    {
        EE_ASSERT(d3d11Usage == D3D11_USAGE_STAGING);
        accessMask = 0;
        // Staging buffers can be readable and/or writable, though I don't
        // know how useful it would be for one to be set but not the other.
        if ((d3d11CPUAccessFlags & D3D11_CPU_ACCESS_WRITE) != 0)
            accessMask |= ACCESS_CPU_WRITE_MUTABLE;
        if ((d3d11CPUAccessFlags & D3D11_CPU_ACCESS_READ) != 0)
            accessMask |= ACCESS_CPU_READ;
    }

    // Support for Stream Output:
    if ((d3d11BindFlags & D3D11_BIND_SHADER_RESOURCE) != 0)
        accessMask |= ACCESS_GPU_READ;
    if ((d3d11BindFlags & D3D11_BIND_STREAM_OUTPUT) != 0)
    {
        accessMask |= ACCESS_GPU_READ;
        accessMask |= ACCESS_GPU_WRITE;
    }

    return (d3d11BindFlags == InterpretUsage(usage, accessMask));
}

//------------------------------------------------------------------------------------------------
void* D3D11DataStream::MapBuffer(efd::UInt8 uiLockMask, efd::UInt32, efd::UInt32)
{
    if (m_pBackingBuffer != NULL)
    {
        return m_pBackingBuffer;
    }
    else if ((uiLockMask & (LOCK_TOOL_READ | LOCK_TOOL_WRITE)) != 0)
    {
        // A lock type of TOOL_READ or TOOL_WRITE can force this path.
        // A D3D buffer must exist at this point; lock it as necessary.
        // Don't worry about concurrent locks
        EE_ASSERT(m_pD3D11Buffer);

        // If there's no backing store, then either the buffer is DYNAMIC,
        // in which case it can be written to (but not read), or the buffer
        // has to be copied back to system memory.

        efd::Bool readDesired = (uiLockMask & (LOCK_READ | LOCK_TOOL_READ)) != 0;
        efd::Bool writeDesired = (uiLockMask & (LOCK_WRITE | LOCK_TOOL_WRITE)) != 0;
        efd::Bool bufferCanRead = (m_d3d11CPUAccessFlags & D3D11_CPU_ACCESS_READ) != 0;
        efd::Bool bufferCanWrite = (m_d3d11CPUAccessFlags & D3D11_CPU_ACCESS_WRITE) != 0;

        D3D11_MAP mapFlag;
        if (readDesired)
        {
            if (writeDesired)
                mapFlag = D3D11_MAP_READ_WRITE;
            else
                mapFlag = D3D11_MAP_READ;
        }
        else
        {
            EE_ASSERT(writeDesired);
            mapFlag = D3D11_MAP_WRITE;
        }

        // DT33837 Support multiple device contexts.
        ID3D11DeviceContext* pContext = 
            D3D11Renderer::GetRenderer()->GetCurrentD3D11DeviceContext();
        EE_ASSERT(pContext);

        void* pData = NULL;
        D3D11_MAPPED_SUBRESOURCE mappedResource;
        // Check to see if the existing buffer can be locked directly
        if ((!readDesired || bufferCanRead) &&
            (!writeDesired || bufferCanWrite))
        {
#if defined(EE_ASSERTS_ARE_ENABLED)
            HRESULT hr =
#endif //#if defined(EE_ASSERTS_ARE_ENABLED)
                pContext->Map(m_pD3D11Buffer, 0, mapFlag, 0, &mappedResource);
            EE_ASSERT(SUCCEEDED(hr));
            pData = mappedResource.pData;
            return pData;
        }

        // Otherwise, must create a temporary resource and copy it over.
        // Must hide this resource in m_pBackingBuffer.

        D3D11ResourceManager* pResourceManager =
            D3D11Renderer::GetRenderer()->GetResourceManager();
        EE_ASSERT(pResourceManager);
        ID3D11Buffer* pTempBuffer = pResourceManager->CreateBuffer(
            m_uiSize, 
            0, 
            D3D11_USAGE_STAGING,
            D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE,
            0,
            0,
            NULL);
        EE_ASSERT(pTempBuffer);

        pContext->CopyResource(pTempBuffer, m_pD3D11Buffer);

        m_pBackingBuffer = (void*)pTempBuffer;
        m_hidingBufferInBackingBuffer = true;

        pContext->Map(pTempBuffer, 0, mapFlag, 0, &mappedResource);
        pData = mappedResource.pData;

        return pData;
    }
    else
    {
        EE_FAIL("Invalid lock mask specified for DataStream::LockImpl.");
        return NULL;
    }
}

//------------------------------------------------------------------------------------------------
void D3D11DataStream::UnmapBuffer(efd::UInt8 lockMask, efd::UInt32, efd::UInt32)
{
    EE_ASSERT(m_pBackingBuffer != NULL);

    efd::Bool bWrite = (lockMask & (LOCK_WRITE | LOCK_TOOL_WRITE)) != 0;

    // Check for a forced lock
    if (m_hidingBufferInBackingBuffer)
    {
        ID3D11Buffer* pTempBuffer = (ID3D11Buffer*)m_pBackingBuffer;
        m_pBackingBuffer = NULL;
        m_hidingBufferInBackingBuffer = false;

        // DT33837 Support multiple device contexts.
        ID3D11DeviceContext* pContext = 
            D3D11Renderer::GetRenderer()->GetCurrentD3D11DeviceContext();
        EE_ASSERT(pContext);

        pContext->Unmap(pTempBuffer, 0);

        // If writing data, upload it back to the buffer
        if (bWrite)
        {
            if (m_d3d11Usage == D3D11_USAGE_IMMUTABLE)
            {
                // Must replace old buffer with new.
                EE_ASSERT(m_pD3D11Buffer);
                m_pD3D11Buffer->Release();
                m_pD3D11Buffer = NULL;

                D3D11_MAPPED_SUBRESOURCE mappedResource;
                pContext->Map(pTempBuffer, 0, D3D11_MAP_READ, 0, &mappedResource);
                m_pBackingBuffer = mappedResource.pData;

                D3D11Allocate(true);

                pContext->Unmap(pTempBuffer, 0);
            }
            else
            {
                pContext->CopyResource(m_pD3D11Buffer, pTempBuffer);
            }

        }

        pTempBuffer->Release();
        return;
    }
    else if (bWrite)
    {
        // Check for a forced lock on an immutable buffer that has CPU Read
        // (and therefore has a valid m_pBackingBuffer)
        if (m_d3d11Usage == D3D11_USAGE_IMMUTABLE &&
            m_pD3D11Buffer != NULL)
        {
            // Must release old buffer so it can be replaced.
            m_pD3D11Buffer->Release();
            m_pD3D11Buffer = NULL;
        }

        m_dirty = true;
    }
}

//------------------------------------------------------------------------------------------------
void D3D11DataStream::UpdateD3D11Buffers(efd::Bool releaseSystemMemory)
{
    // Only want to update GPU-readable buffers
    EE_ASSERT((GetAccessMask() & ACCESS_GPU_READ) != 0);

    if (m_dirty)
    {
        EE_ASSERT(m_pBackingBuffer && !m_hidingBufferInBackingBuffer);
        // Allocate buffer, if necessary
        if (m_pD3D11Buffer == NULL)
        {
            // This will also initialize the buffer with the data in
            // m_pBackingBuffer
            D3D11Allocate(true);
        }
        else
        {
            PerformD3D11BufferUpdate();
        }

        // Can only release system memory if CPU_READ mask is not set and
        // data stream is static
        if (releaseSystemMemory &&
            (GetAccessMask() & ACCESS_CPU_READ) == 0 &&
            (GetAccessMask() & ACCESS_CPU_WRITE_STATIC) != 0)
        {
            if (m_pBackingBuffer)
                EE_FREE(m_pBackingBuffer);
            m_pBackingBuffer = NULL;
        }

        m_dirty = false;
    }

    EE_ASSERT(m_pD3D11Buffer);
}

//------------------------------------------------------------------------------------------------
void D3D11DataStream::PerformD3D11BufferUpdate()
{
    EE_ASSERT(m_pD3D11Buffer);

    // IMMUTABLE buffers cannot be updated.
    EE_ASSERT(m_d3d11Usage != D3D11_USAGE_IMMUTABLE);

    EE_ASSERT(!m_hidingBufferInBackingBuffer);

    // DT33837 Support multiple device contexts.
    ID3D11DeviceContext* pContext = D3D11Renderer::GetRenderer()->GetCurrentD3D11DeviceContext();
    EE_ASSERT(pContext);

    if (m_d3d11Usage == D3D11_USAGE_DEFAULT)
    {
        // DEFAULT buffers: Update using UpdateSubresource


        pContext->UpdateSubresource(
            m_pD3D11Buffer, 
            0, 
            NULL,
            m_pBackingBuffer, 
            m_uiSize, 
            m_uiSize);
    }
    else
    {
        // DYNAMIC and STAGING buffers: Update using Lock/update/unlock buffer

        D3D11_MAP mapFlag;
        if (m_d3d11Usage == D3D11_USAGE_DYNAMIC)
        {
            // DT33838 use WRITE_NO_OVERWRITE to share buffers
            mapFlag = D3D11_MAP_WRITE_DISCARD;
        }
        else
        {
            EE_ASSERT(m_d3d11Usage == D3D11_USAGE_STAGING);
                mapFlag = D3D11_MAP_WRITE;
        }

        // DT33839 Investigate D3D11_MAP_FLAG_DO_NOT_WAIT
        void* pLockedBuffer = NULL;

        D3D11_MAPPED_SUBRESOURCE mappedResource;
        HRESULT hr = pContext->Map(m_pD3D11Buffer, 0, mapFlag, 0, &mappedResource);
        pLockedBuffer = mappedResource.pData;

        if (FAILED(hr) || pLockedBuffer == NULL)
        {
            if (FAILED(hr))
            {
                D3D11Error::ReportWarning(
                    "ID3D11DeviceContext::Map failed in " 
                    __FUNCTION__ 
                    " with error HRESULT = 0x%08X.", 
                    (efd::UInt32)hr);
            }
            else
            {
                D3D11Error::ReportWarning(
                    "ID3D11DeviceContext::Map failed in " 
                    __FUNCTION__ 
                    " with no error message from D3D11, but mapped resource is NULL.");

            }
        }

        efd::Memcpy(pLockedBuffer, m_pBackingBuffer, m_uiSize);

        pContext->Unmap(m_pD3D11Buffer, 0);
    }
}

//------------------------------------------------------------------------------------------------
