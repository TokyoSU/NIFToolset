#pragma once
#ifndef BGFXDATASTREAMFACTORY_H
#define BGFXDATASTREAMFACTORY_H

#include <NiDataStreamFactory.h>

// bgfx owns the GPU-side mesh buffers in BgfxRenderer. Gamebryo data streams
// intentionally remain CPU-backed so the renderer can repack arbitrary legacy
// vertex declarations into bgfx layouts and can rebuild mutable streams safely.
class BgfxDataStreamFactory final : public NiDataStreamFactory
{
public:
    BgfxDataStreamFactory();
    ~BgfxDataStreamFactory() override;

protected:
    NiDataStream* CreateDataStreamImpl(
        const NiDataStreamElementSet& elements,
        NiUInt32 count,
        NiUInt8 accessMask,
        NiDataStream::Usage usage) override;

    NiDataStream* CreateDataStreamImpl(
        NiUInt8 accessMask,
        NiDataStream::Usage usage,
        bool canOverrideAccessMask) override;
};

#endif
