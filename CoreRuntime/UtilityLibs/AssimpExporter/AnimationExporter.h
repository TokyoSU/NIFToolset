#pragma once

#include "AssetLoader.h"

#include <assimp/scene.h>

#include <string>
#include <vector>

// Builds a list of aiAnimation objects from the resolved animation sources.
// Priority: KFM sequences -> matching KF sequences -> NIF NiTransformController fallback.
// Each sequence becomes one aiAnimation with per-node channels.
class AnimationExporter
{
public:
	// Build animations from NiSequenceData (KFM or KF). kNifRoot supplies
	// the bind/rest local transforms for channels whose evaluator only animates
	// position, rotation, or scale instead of all three components.
	static std::vector<aiAnimation*> BuildFromSequenceDatas(
		const std::vector<NiSequenceDataPtr>& kSeqDatas,
		NiAVObject* pkNifRoot);

	// NIF fallback: scan the NIF scene graph for NiTransformControllers
	static std::vector<aiAnimation*> BuildFromNifControllers(NiAVObject* pkRoot);
};
