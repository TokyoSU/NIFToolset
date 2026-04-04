#pragma once
#ifndef NIMODELLOADER_H
#define NIMODELLOADER_H

#include <NiStream.h>
#include <NiAVObject.h>
#include <NiActorManager.h>

// ---------------------------------------------------------------------------
// NiModelLoader
//
// Inline helpers for safely loading NIF and KFM+NIF assets.
//
// Usage (static):
//   NiPointer<NiAVObject> spMesh = NiModelLoader::LoadStatic("Data\\Mesh.nif");
//
// Usage (animated):
//   NiPointer<NiActorManager> spActor = NiModelLoader::LoadAnimated("Data\\Char.kfm");
// ---------------------------------------------------------------------------
namespace NiModelLoader
{
    // Loads a static .nif file.
    // Returns nullptr when the file is missing, empty, or the root is not
    // an NiAVObject.
    inline NiPointer<NiAVObject> LoadStatic(const char* pcNIFPath)
    {
        if (!pcNIFPath)
            return nullptr;

        NiStream kStream;
        if (!kStream.Load(pcNIFPath))
            return nullptr;

        if (kStream.GetObjectCount() == 0)
            return nullptr;

        return NiDynamicCast(NiAVObject, kStream.GetObjectAt(0));
    }

    // Loads an animated .kfm file (which internally loads the .nif and all
    // .kf sequence files referenced by the .kfm).
    // Returns nullptr when any part of the load fails.
    // Set bCumulativeAnimations = true for root-motion animations.
    inline NiPointer<NiActorManager> LoadAnimated(const char* pcKFMPath,
        bool bCumulativeAnimations = false)
    {
        if (!pcKFMPath)
            return nullptr;

        return NiActorManager::Create(pcKFMPath, bCumulativeAnimations);
    }
}

#endif // NIMODELLOADER_H