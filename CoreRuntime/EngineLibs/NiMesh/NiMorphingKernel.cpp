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

#include "NiMeshPCH.h"

#include "NiMorphingKernel.h"
#include "NiMorphingKernelData.h"

#include <NiUniversalTypes.h>
#include <NiPoint3.h>
#include <NiFloat16.h>
#include <NiOS.h>

//--------------------------------------------------------------------------------------------------
// Define the iteration so we don't need to copy and paste
#define OUT_ITERATION \
    fOutSource0 = 0.0f; \
    fOutSource1 = 0.0f; \
    fOutSource2 = 0.0f; \
    for (NiUInt32 uiTarget = 0; uiTarget < uiTargetCount; uiTarget++) \
    { \
        TTarget* pkTarget = apkTargets[uiTarget]; \
        apkTargets[uiTarget] = (TTarget*)((NiUInt8*)apkTargets[uiTarget] + auiStrides[uiTarget]); \
        float fWeight = afWeights[uiTarget]; \
        fOutSource0 += fWeight * pkTarget[0]; \
        fOutSource1 += fWeight * pkTarget[1]; \
        fOutSource2 += fWeight * pkTarget[2]; \
    } \
    pkSource[0] = fOutSource0; \
    pkSource[1] = fOutSource1; \
    pkSource[2] = fOutSource2; \
    pkSource = (TSource*)((NiUInt8*)pkSource + uiSourceStride)
//--------------------------------------------------------------------------------------------------
#define NORM_ITERATION \
    kPoint.x = pkSource[0]; \
    kPoint.y = pkSource[1]; \
    kPoint.z = pkSource[2]; \
    kPoint.Unitize(); \
    pkSource[0] = kPoint.x; \
    pkSource[1] = kPoint.y; \
    pkSource[2] = kPoint.z; \
    pkSource = (TSource*)((NiUInt8*)pkSource + uiSourceStride)
//--------------------------------------------------------------------------------------------------
template<typename TSource, typename TTarget, bool bNormalize>
class MorphImpl
{
public:
    EE_NOINLINE void Execute(NiSPWorkload& kWorkload)
    {
        // Only one output
        TSource* pkOutSource = kWorkload.GetOutput<TSource>(0);
        NiUInt32 uiSourceCount = kWorkload.GetBlockCount();
        NiUInt32 uiSourceStride = kWorkload.GetOutputStride(0);

        // Get target count - target vertices are workload inputs 2..n-1.
        NiMorphingKernelData* pkKernelData = kWorkload.GetInput<NiMorphingKernelData>(1);
        NiUInt32 uiTargetCount = pkKernelData->GetActiveCount();

        TTarget* apkTargets[MAX_MORPH_TARGETS];
        NiUInt32 auiStrides[MAX_MORPH_TARGETS];

        // Weights are the first.
        float* afWeights = kWorkload.GetInput<float>(0);

        NiUInt16 ui;
        for (ui = 0; ui < uiTargetCount; ++ui)
        {
            apkTargets[ui] = kWorkload.GetInput<TTarget>((NiUInt16)ui + 2);
            auiStrides[ui] = kWorkload.GetInputStride((NiUInt16)ui + 2);
        }

        TSource* pkSource = pkOutSource;
        float fOutSource0;
        float fOutSource1;
        float fOutSource2;

        ui = 8;
        while (ui < uiSourceCount)
        {
            OUT_ITERATION;
            OUT_ITERATION;
            OUT_ITERATION;
            OUT_ITERATION;
            OUT_ITERATION;
            OUT_ITERATION;
            OUT_ITERATION;
            OUT_ITERATION;
            ui += 8;
        }

        ui -= 8;
        while (ui < uiSourceCount)
        {
            OUT_ITERATION;
            ui++;
        }

        bool bNormalizeParam = bNormalize;
        if (bNormalizeParam)
        {

#ifndef __SPU__

            size_t stTSourceSize = sizeof(TSource);
            if (stTSourceSize == sizeof(float))
            {
                NiPoint3::UnitizeVectors((NiPoint3*)pkOutSource,
                    uiSourceCount, sizeof(NiPoint3));
            }
            else
            {
                TSource* pkSource = pkOutSource;
#else
            pkSource = pkOutSource;
#endif
            NiPoint3 kPoint;

            ui = 8;
            while (ui < uiSourceCount)
            {
                NORM_ITERATION;
                NORM_ITERATION;
                NORM_ITERATION;
                NORM_ITERATION;
                NORM_ITERATION;
                NORM_ITERATION;
                NORM_ITERATION;
                NORM_ITERATION;
                ui += 8;
            }

            ui -= 8;
            while (ui < uiSourceCount)
            {
                NORM_ITERATION;
                ui++;
            }
#ifndef __SPU__
            }
#endif
        }
    }
};

//--------------------------------------------------------------------------------------------------
NiSPBeginKernelImpl(NiMorphingKernel)
{
    NiMorphingKernelData* pkKernelData = kWorkload.GetInput<NiMorphingKernelData>(1);

    switch (pkKernelData->GetCombination())
    {
    case MKDC_16_16:
        {
            MorphImpl<efd::Float16, efd::Float16, false> kFunctor;
            kFunctor.Execute(kWorkload);
        }
        break;
    case MKDC_32_16:
        {
            MorphImpl<float, efd::Float16, false> kFunctor;
            kFunctor.Execute(kWorkload);
        }
        break;
    case MKDC_16_32:
        {
            MorphImpl<efd::Float16, float, false> kFunctor;
            kFunctor.Execute(kWorkload);
        }
        break;
    case MKDC_32_32:
        {
            MorphImpl<float, float, false> kFunctor;
            kFunctor.Execute(kWorkload);
        }
        break;
    case MKDC_16_16_N:
        {
            MorphImpl<efd::Float16, efd::Float16, true> kFunctor;
            kFunctor.Execute(kWorkload);
        }
        break;
    case MKDC_32_16_N:
        {
            MorphImpl<float, efd::Float16, true> kFunctor;
            kFunctor.Execute(kWorkload);
        }
        break;
    case MKDC_16_32_N:
        {
            MorphImpl<efd::Float16, float, true> kFunctor;
            kFunctor.Execute(kWorkload);
        }
        break;
    case MKDC_32_32_N:
        {
            MorphImpl<float, float, true> kFunctor;
            kFunctor.Execute(kWorkload);
        }
        break;
    default:
        KernelDebugOutput("Unsupported combination for morphing (%d).)\n",
            pkKernelData->GetCombination());
    }
}
NiSPEndKernelImpl(NiMorphingKernel)
