#pragma once

#include "AssetLoader.h"

#include <assimp/scene.h>

#include <string>
#include <vector>

// Builds a list of aiAnimation objects from the resolved animation sources.
// KFM/KF skeletal sequences are baked together with NIF-level
// NiTransformControllers so auxiliary/effect nodes keep animating while a
// skinned sequence is active. If no sequence exists, NIF controllers are
// exported as a standalone fallback animation.
class AnimationExporter
{
public:
	// Build animations from NiSequenceData (KFM or KF). kNifRoot supplies
	// bind/rest local transforms and any attached NiTransformControllers.
	// Sequence-authored components take precedence; NIF controllers fill
	// effect/auxiliary-node channels and missing transform components.
	static std::vector<aiAnimation*> BuildFromSequenceDatas(
		const std::vector<NiSequenceDataPtr>& kSeqDatas,
		NiAVObject* pkNifRoot,
		float fUnitScale,
		float fSampleRate,
		bool bConvertToUnrealAxes = true,
		const NiStream* pkNifStream = nullptr);

	// Append sequences stored inside NiControllerManager controllers in the
	// model NIF. Older NIFs stream these objects as NiControllerSequence; the
	// Gamebryo compatibility loader exposes them as NiSequenceData. Existing
	// sequence names in kSequenceDatas take precedence, so external KFM/KF
	// clips are not exported twice while NIF-only clips are still included.
	static unsigned int AppendFromControllerManagers(
		NiAVObject* pkRoot,
		std::vector<NiSequenceDataPtr>& kSequenceDatas);

	// NIF fallback: scan the NIF scene graph for NiTransformControllers.
	// Curves are sampled through Gamebryo's interpolator implementation so
	// Bezier/Euler interpolation is baked correctly for FBX/Unreal.
	static std::vector<aiAnimation*> BuildFromNifControllers(
		NiAVObject* pkRoot,
		float fUnitScale,
		float fSampleRate,
		bool bConvertToUnrealAxes = true);
};
