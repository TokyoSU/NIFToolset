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

#include "D3D11DataStreamFactory.h"

using namespace ecr;

//------------------------------------------------------------------------------------------------
D3D11DataStreamFactory::D3D11DataStreamFactory() :
    NiDataStreamFactory()
{
    NiDataStream::SetFactory(this);
}

//------------------------------------------------------------------------------------------------
D3D11DataStreamFactory::~D3D11DataStreamFactory()
{
    NiDataStream::SetFactory(NULL);
}

//------------------------------------------------------------------------------------------------
NiDataStream* D3D11DataStreamFactory::CreateDataStreamImpl(
    const NiDataStreamElementSet& kElements,
    efd::UInt32 count,
    efd::UInt8 accessMask,
    NiDataStream::Usage usage)
{
    D3D11_USAGE d3d11Usage;
    efd::UInt32 bindFlags;
    efd::UInt32 cpuAccessFlags;

    if (!D3D11DataStream::InterpretDataStreamFlags(
        accessMask, 
        usage,
        d3d11Usage, 
        bindFlags, 
        cpuAccessFlags))
    {
        return NULL;
    }

    if (accessMask == (NiDataStream::ACCESS_GPU_READ | NiDataStream::ACCESS_GPU_WRITE))
    {
        // This is the case used for Stream Output.
        // Use a NON-lockable data stream here; if they want to access the
        // contents (which is very slow), they should use another path.
        EE_ASSERT((accessMask & (NiDataStream::ACCESS_CPU_WRITE_ANY)) == 0);
        return EE_NEW D3D11NonLockableDataStream(
            kElements, 
            count,
            accessMask, 
            usage, 
            d3d11Usage, 
            bindFlags, 
            cpuAccessFlags);
    }
    else if ((accessMask & NiDataStream::ACCESS_CPU_WRITE_MUTABLE) != 0)
    {
        return EE_NEW D3D11LockableDataStream<MutableLockPolicy<D3D11DataStream> >(
            kElements, 
            count, 
            accessMask, 
            usage, 
            d3d11Usage,
            bindFlags, 
            cpuAccessFlags);
    }
    else if ((accessMask & NiDataStream::ACCESS_CPU_WRITE_VOLATILE) != 0)
    {
        return EE_NEW D3D11LockableDataStream<VolatileLockPolicy<D3D11DataStream> >(
            kElements, 
            count, 
            accessMask, 
            usage, 
            d3d11Usage,
            bindFlags, 
            cpuAccessFlags);
    }
    else
    {
        EE_ASSERT((accessMask & NiDataStream::ACCESS_CPU_WRITE_STATIC) != 0);
        return EE_NEW D3D11LockableDataStream<StaticLockPolicy<D3D11DataStream> >(
            kElements, 
            count, 
            accessMask, 
            usage, 
            d3d11Usage,
            bindFlags, 
            cpuAccessFlags);
    }
}

//------------------------------------------------------------------------------------------------
NiDataStream* D3D11DataStreamFactory::CreateDataStreamImpl(
    efd::UInt8 accessMask,
    NiDataStream::Usage usage,
    efd::Bool)
{
    D3D11_USAGE d3d11Usage;
    efd::UInt32 bindFlags;
    efd::UInt32 cpuAccessFlags;

    if (!D3D11DataStream::InterpretDataStreamFlags(
        accessMask, 
        usage,
        d3d11Usage, 
        bindFlags, 
        cpuAccessFlags))
    {
        return NULL;
    }

    if ((accessMask & NiDataStream::ACCESS_CPU_WRITE_MUTABLE) != 0)
    {
        return EE_NEW D3D11LockableDataStream<MutableLockPolicy<D3D11DataStream> >(
            accessMask, 
            usage, 
            d3d11Usage, 
            bindFlags, 
            cpuAccessFlags);
    }
    else if ((accessMask & NiDataStream::ACCESS_CPU_WRITE_VOLATILE) != 0)
    {
        return EE_NEW D3D11LockableDataStream<VolatileLockPolicy<D3D11DataStream> >(
            accessMask, 
            usage, 
            d3d11Usage, 
            bindFlags, 
            cpuAccessFlags);
    }
    else
    {
        EE_ASSERT((accessMask & NiDataStream::ACCESS_CPU_WRITE_STATIC) != 0);
        return EE_NEW D3D11LockableDataStream<StaticLockPolicy<D3D11DataStream> >(
            accessMask, 
            usage, 
            d3d11Usage, 
            bindFlags, 
            cpuAccessFlags);
    }
}

//------------------------------------------------------------------------------------------------
