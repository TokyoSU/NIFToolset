#include "BgfxDataStreamFactory.h"

#include <NiToolDataStream.h>

BgfxDataStreamFactory::BgfxDataStreamFactory()
{
    NiDataStream::SetFactory(this);
}

BgfxDataStreamFactory::~BgfxDataStreamFactory()
{
    // Do not tear down a factory installed by another renderer after us.
    if (NiDataStream::GetFactory() == this)
        NiDataStream::SetFactory(nullptr);
}

NiDataStream* BgfxDataStreamFactory::CreateDataStreamImpl(
    const NiDataStreamElementSet& elements,
    NiUInt32 count,
    NiUInt8 accessMask,
    NiDataStream::Usage usage)
{
    return NiNew NiToolDataStream(elements, count, accessMask, usage);
}

NiDataStream* BgfxDataStreamFactory::CreateDataStreamImpl(
    NiUInt8 accessMask,
    NiDataStream::Usage usage,
    bool canOverrideAccessMask)
{
    (void)canOverrideAccessMask;
    return NiNew NiToolDataStream(accessMask, usage);
}
