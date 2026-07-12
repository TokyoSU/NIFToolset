#include "AnimationExporter.h"
#include "ExportNaming.h"

#include <NiNode.h>
#include <NiTimeController.h>
#include <NiTransformController.h>
#include <NiTransformInterpolator.h>
#include <NiTransformEvaluator.h>
#include <NiConstTransformEvaluator.h>
#include <NiSequenceData.h>
#include <NiPosKey.h>
#include <NiRotKey.h>
#include <NiFloatKey.h>
#include <NiQuaternion.h>

#include <assimp/scene.h>
#include <assimp/anim.h>

#include <cstring>
#include <iostream>
#include <cmath>
#include <algorithm>
#include <string>
#include <unordered_map>
#include <vector>

//--------------------------------------------------------------------------------------------------
// Internal helpers
//--------------------------------------------------------------------------------------------------

// Convert NiQuaternion (w,x,y,z) to aiQuaternion (w,x,y,z) — same layout
static aiQuaternion ToAiQuat(const NiQuaternion& q)
{
	return aiQuaternion(q.GetW(), q.GetX(), q.GetY(), q.GetZ());
}

// Forward declaration: fills any position/rotation/scaling component that has
// zero keys with a two-key default (rest pose from pkObj if given, else
// identity), so Assimp never writes an empty curve node. See definition
// below CollectTransformControllers for full rationale.
static void FillMissingKeysWithRestPose(aiNodeAnim* pkChan, NiAVObject* pkObj,
	double dStartTime, double dEndTime);

// Build a node channel from NiTransformEvaluator key data
static aiNodeAnim* BuildChannelFromTransformEvaluator(
	const std::string& kNodeName,
	NiTransformEvaluator* pkEval,
	float fTicksPerSecond,
	NiAVObject* pkRestObject,
	double dSequenceEndTime)
{
	if (!pkEval)
		return nullptr;

	aiNodeAnim* pkChan = new aiNodeAnim();
	pkChan->mNodeName = kNodeName.c_str();

	// -- Position keys --
	{
		unsigned int uiNumKeys = 0;
		NiPosKey::KeyType eType = NiPosKey::NOINTERP;
		unsigned char ucSize = 0;
		NiPosKey* pkKeys = pkEval->GetPosData(uiNumKeys, eType, ucSize);
		if (pkKeys && uiNumKeys > 0)
		{
			pkChan->mNumPositionKeys = uiNumKeys;
			pkChan->mPositionKeys = new aiVectorKey[uiNumKeys];
			for (unsigned int k = 0; k < uiNumKeys; ++k)
			{
				NiPosKey* pkKey = pkKeys->GetKeyAt(k, ucSize);
				double dTime = static_cast<double>(pkKey->GetTime()) * fTicksPerSecond;
				const NiPoint3& p = pkKey->GetPos();
				pkChan->mPositionKeys[k] = aiVectorKey(dTime, aiVector3D(p.x, p.y, p.z));
			}
		}
	}

	// -- Rotation keys --
	{
		unsigned int uiNumKeys = 0;
		NiRotKey::KeyType eType = NiRotKey::NOINTERP;
		unsigned char ucSize = 0;
		NiRotKey* pkKeys = pkEval->GetRotData(uiNumKeys, eType, ucSize);
		if (pkKeys && uiNumKeys > 0)
		{
			pkChan->mNumRotationKeys = uiNumKeys;
			pkChan->mRotationKeys = new aiQuatKey[uiNumKeys];
			for (unsigned int k = 0; k < uiNumKeys; ++k)
			{
				NiRotKey* pkKey = pkKeys->GetKeyAt(k, ucSize);
				double dTime = static_cast<double>(pkKey->GetTime()) * fTicksPerSecond;
				pkChan->mRotationKeys[k] = aiQuatKey(dTime, ToAiQuat(pkKey->GetQuaternion()));
			}
		}
	}

	// -- Scale keys --
	{
		unsigned int uiNumKeys = 0;
		NiFloatKey::KeyType eType = NiFloatKey::NOINTERP;
		unsigned char ucSize = 0;
		NiFloatKey* pkKeys = pkEval->GetScaleData(uiNumKeys, eType, ucSize);
		if (pkKeys && uiNumKeys > 0)
		{
			pkChan->mNumScalingKeys = uiNumKeys;
			pkChan->mScalingKeys = new aiVectorKey[uiNumKeys];
			for (unsigned int k = 0; k < uiNumKeys; ++k)
			{
				NiFloatKey* pkKey = pkKeys->GetKeyAt(k, ucSize);
				double dTime = static_cast<double>(pkKey->GetTime()) * fTicksPerSecond;
				float fS = pkKey->GetValue();
				pkChan->mScalingKeys[k] = aiVectorKey(dTime, aiVector3D(fS, fS, fS));
			}
		}
	}

	// Discard channels with no keys at all
	if (pkChan->mNumPositionKeys == 0 &&
		pkChan->mNumRotationKeys == 0 &&
		pkChan->mNumScalingKeys == 0)
	{
		delete pkChan;
		return nullptr;
	}

	// Any component left with zero keys (e.g. only rotation was animated)
	// must still be filled in; otherwise Assimp writes an empty curve node
	// that breaks FBX consumers like three.js's FBXLoader
	// ("no keyframes in track named X.position"). Use the matching NIF
	// node's local bind/rest transform for every missing component.
	double dLastTime = 0.0;
	if (pkChan->mNumPositionKeys > 0)
		dLastTime = std::max(dLastTime, pkChan->mPositionKeys[pkChan->mNumPositionKeys - 1].mTime);
	if (pkChan->mNumRotationKeys > 0)
		dLastTime = std::max(dLastTime, pkChan->mRotationKeys[pkChan->mNumRotationKeys - 1].mTime);
	if (pkChan->mNumScalingKeys > 0)
		dLastTime = std::max(dLastTime, pkChan->mScalingKeys[pkChan->mNumScalingKeys - 1].mTime);
	FillMissingKeysWithRestPose(pkChan, pkRestObject, 0.0,
		std::max(dLastTime, dSequenceEndTime));

	return pkChan;
}

// Build a node channel from NiConstTransformEvaluator (single posed value)
static aiNodeAnim* BuildChannelFromConstEvaluator(
	const std::string& kNodeName,
	NiConstTransformEvaluator* pkEval,
	float fStartTime,
	float fEndTime,
	float fTicksPerSecond,
	NiAVObject* pkRestObject)
{
	bool bHasPos = !pkEval->IsEvalChannelInvalid(NiEvaluator::EVALPOSINDEX);
	bool bHasRot = !pkEval->IsEvalChannelInvalid(NiEvaluator::EVALROTINDEX);
	bool bHasScale = !pkEval->IsEvalChannelInvalid(NiEvaluator::EVALSCALEINDEX);

	if (!bHasPos && !bHasRot && !bHasScale)
		return nullptr;

	NiPoint3 kPos(0, 0, 0);
	NiQuaternion kRot(1, 0, 0, 0);
	float fScale = 1.0f;

	// GetChannelPosedValue to read constant values
	if (bHasPos)
		pkEval->GetChannelPosedValue(NiEvaluator::EVALPOSINDEX, &kPos);
	if (bHasRot)
		pkEval->GetChannelPosedValue(NiEvaluator::EVALROTINDEX, &kRot);
	if (bHasScale)
		pkEval->GetChannelPosedValue(NiEvaluator::EVALSCALEINDEX, &fScale);

	aiNodeAnim* pkChan = new aiNodeAnim();
	pkChan->mNodeName = kNodeName.c_str();

	double dStart = static_cast<double>(fStartTime) * fTicksPerSecond;
	double dEnd = static_cast<double>(fEndTime) * fTicksPerSecond;

	if (bHasPos)
	{
		pkChan->mNumPositionKeys = 2;
		pkChan->mPositionKeys = new aiVectorKey[2];
		pkChan->mPositionKeys[0] = aiVectorKey(dStart, aiVector3D(kPos.x, kPos.y, kPos.z));
		pkChan->mPositionKeys[1] = aiVectorKey(dEnd,   aiVector3D(kPos.x, kPos.y, kPos.z));
	}
	if (bHasRot)
	{
		pkChan->mNumRotationKeys = 2;
		pkChan->mRotationKeys = new aiQuatKey[2];
		pkChan->mRotationKeys[0] = aiQuatKey(dStart, ToAiQuat(kRot));
		pkChan->mRotationKeys[1] = aiQuatKey(dEnd,   ToAiQuat(kRot));
	}
	if (bHasScale)
	{
		pkChan->mNumScalingKeys = 2;
		pkChan->mScalingKeys = new aiVectorKey[2];
		pkChan->mScalingKeys[0] = aiVectorKey(dStart, aiVector3D(fScale, fScale, fScale));
		pkChan->mScalingKeys[1] = aiVectorKey(dEnd,   aiVector3D(fScale, fScale, fScale));
	}

	// A constant evaluator may only provide one component. Preserve the NIF
	// node's bind/rest values for the other components instead of writing
	// identity curves, which would move a rotating bone to the origin.
	FillMissingKeysWithRestPose(pkChan, pkRestObject, dStart, dEnd);
	return pkChan;
}

//--------------------------------------------------------------------------------------------------
using NodeByNameMap = std::unordered_map<std::string, NiAVObject*>;

static void CollectNodesByName(NiAVObject* pkObject, NodeByNameMap& kOut)
{
	if (!pkObject)
		return;

	const char* pcName = pkObject->GetName();
	if (pcName && pcName[0] != '\0')
	{
		// Evaluator ID tags address AVObjects by name. Keep the first match,
		// matching Gamebryo's normal palette lookup behavior for valid assets.
		kOut.emplace(pcName, pkObject);
	}

	if (NiIsKindOf(NiNode, pkObject))
	{
		NiNode* pkNode = NiStaticCast(NiNode, pkObject);
		for (unsigned int i = 0; i < pkNode->GetArrayCount(); ++i)
			CollectNodesByName(pkNode->GetAt(i), kOut);
	}
}

//--------------------------------------------------------------------------------------------------
std::vector<aiAnimation*> AnimationExporter::BuildFromSequenceDatas(
	const std::vector<NiSequenceDataPtr>& kSeqDatas,
	NiAVObject* pkNifRoot)
{
	std::vector<aiAnimation*> kResult;

	NodeByNameMap kNodesByName;
	CollectNodesByName(pkNifRoot, kNodesByName);

	const float fTicksPerSecond = 24.0f;

	for (unsigned int s = 0; s < static_cast<unsigned int>(kSeqDatas.size()); ++s)
	{
		NiSequenceData* pkSeq = kSeqDatas[s];
		if (!pkSeq)
			continue;

		const char* pcName = pkSeq->GetName();
		const float fDuration = pkSeq->GetDuration();
		const float fFrequency = pkSeq->GetFrequency();
		const float fSafeFrequency = std::abs(fFrequency) > 0.000001f
			? std::abs(fFrequency) : 1.0f;
		const float fKeyTimeScale = fTicksPerSecond / fSafeFrequency;
		const double dDurationTicks =
			static_cast<double>(fDuration) * fKeyTimeScale;
		unsigned int uiNumEval = pkSeq->GetNumEvaluators();

		if (uiNumEval == 0 || fDuration <= 0.0f)
			continue;

		std::vector<aiNodeAnim*> kChannels;
		kChannels.reserve(uiNumEval);
		unsigned int uiUnsupportedEvaluators = 0;
		unsigned int uiMissingRestNodes = 0;

		for (unsigned int e = 0; e < uiNumEval; ++e)
		{
			NiEvaluator* pkEval = pkSeq->GetEvaluatorAt(e);
			if (!pkEval)
				continue;

			const char* pcNodeName = pkEval->GetAVObjectName();
			if (!pcNodeName)
				continue;
			std::string kNodeName(pcNodeName);
			NiAVObject* pkRestObject = nullptr;
			auto kNodeIt = kNodesByName.find(kNodeName);
			if (kNodeIt != kNodesByName.end())
				pkRestObject = kNodeIt->second;
			else
				++uiMissingRestNodes;

			aiNodeAnim* pkChan = nullptr;

			if (NiIsKindOf(NiTransformEvaluator, pkEval))
			{
				pkChan = BuildChannelFromTransformEvaluator(kNodeName,
					NiStaticCast(NiTransformEvaluator, pkEval),
					fKeyTimeScale, pkRestObject, dDurationTicks);
			}
			else if (NiIsKindOf(NiConstTransformEvaluator, pkEval))
			{
				pkChan = BuildChannelFromConstEvaluator(kNodeName,
					NiStaticCast(NiConstTransformEvaluator, pkEval),
					0.0f, fDuration, fKeyTimeScale, pkRestObject);
			}
			else
			{
				// Compressed/B-spline evaluators need scratch-pad evaluation and
				// are not converted by this direct-key path yet.
				++uiUnsupportedEvaluators;
			}

			if (pkChan)
				kChannels.push_back(pkChan);
		}

		if (uiUnsupportedEvaluators > 0)
		{
			std::cerr << "  Warning: animation '"
				<< (pcName ? pcName : "<unnamed>") << "' skipped "
				<< uiUnsupportedEvaluators
				<< " unsupported evaluator(s), such as compressed B-splines."
				<< std::endl;
		}
		if (uiMissingRestNodes > 0)
		{
			std::cerr << "  Warning: animation '"
				<< (pcName ? pcName : "<unnamed>") << "' referenced "
				<< uiMissingRestNodes
				<< " node name(s) not present in the model NIF."
				<< std::endl;
		}

		if (kChannels.empty())
			continue;

		aiAnimation* pkAnim = new aiAnimation();
		pkAnim->mName = pcName ? std::string(pcName).c_str() : ("anim_" + std::to_string(s)).c_str();
		pkAnim->mTicksPerSecond = fTicksPerSecond;
		pkAnim->mDuration = dDurationTicks;
		pkAnim->mNumChannels = static_cast<unsigned int>(kChannels.size());
		pkAnim->mChannels = new aiNodeAnim*[kChannels.size()];
		for (unsigned int c = 0; c < kChannels.size(); ++c)
			pkAnim->mChannels[c] = kChannels[c];
		pkAnim->mNumMeshChannels = 0;
		pkAnim->mMeshChannels = nullptr;
		pkAnim->mNumMorphMeshChannels = 0;
		pkAnim->mMorphMeshChannels = nullptr;

		kResult.push_back(pkAnim);
	}

	return kResult;
}

//--------------------------------------------------------------------------------------------------
// NIF fallback: traverse the NIF graph collecting NiTransformControllers
//--------------------------------------------------------------------------------------------------

struct NifControllerEntry
{
	std::string kName;
	NiTransformController* pkCtrl;
	NiAVObject* pkObj; // owning object, used to fill in a rest-pose default for any
					   // position/rotation/scale component that has no keys of its own
};

static void CollectTransformControllers(NiAVObject* pkObj,
	std::vector<NifControllerEntry>& kOut)
{
	if (!pkObj)
		return;

	std::string kName = GetExportNodeName(pkObj);

	// Walk the time controller chain
	NiTimeController* pkCtrl = pkObj->GetControllers();
	while (pkCtrl)
	{
		if (NiIsKindOf(NiTransformController, pkCtrl))
		{
			kOut.push_back({ kName, NiStaticCast(NiTransformController, pkCtrl), pkObj });
		}
		pkCtrl = pkCtrl->GetNext();
	}

	// Recurse
	if (NiIsKindOf(NiNode, pkObj))
	{
		NiNode* pkNode = NiStaticCast(NiNode, pkObj);
		for (unsigned int i = 0; i < pkNode->GetArrayCount(); ++i)
		{
			NiAVObject* pkChild = pkNode->GetAt(i);
			if (pkChild)
				CollectTransformControllers(pkChild, kOut);
		}
	}
}

// Assimp's FBX exporter (and downstream consumers such as three.js's
// FBXLoader) require every animated component (position/rotation/scaling)
// that is written out to have at least one keyframe. A NIF bone controller
// commonly only animates rotation (e.g. Bip01_Pelvis), leaving position and
// scaling with zero keys; Assimp still emits a curve node for those missing
// components, which three.js then rejects with
// "THREE.KeyframeTrack: no keyframes in track named X.position".
// Fill any empty component with two keys (start/end) holding the node's rest
// pose value so a valid, non-empty curve is always written.
static void FillMissingKeysWithRestPose(aiNodeAnim* pkChan, NiAVObject* pkObj,
	double dStartTime, double dEndTime)
{
	if (pkChan->mNumPositionKeys == 0)
	{
		NiPoint3 kPos = pkObj ? pkObj->GetTranslate() : NiPoint3(0.0f, 0.0f, 0.0f);
		pkChan->mNumPositionKeys = 2;
		pkChan->mPositionKeys = new aiVectorKey[2];
		aiVector3D kV(kPos.x, kPos.y, kPos.z);
		pkChan->mPositionKeys[0] = aiVectorKey(dStartTime, kV);
		pkChan->mPositionKeys[1] = aiVectorKey(dEndTime, kV);
	}

	if (pkChan->mNumRotationKeys == 0)
	{
		NiQuaternion kRot(1.0f, 0.0f, 0.0f, 0.0f);
		if (pkObj)
			pkObj->GetRotate(kRot);
		pkChan->mNumRotationKeys = 2;
		pkChan->mRotationKeys = new aiQuatKey[2];
		aiQuaternion kQ = ToAiQuat(kRot);
		pkChan->mRotationKeys[0] = aiQuatKey(dStartTime, kQ);
		pkChan->mRotationKeys[1] = aiQuatKey(dEndTime, kQ);
	}

	if (pkChan->mNumScalingKeys == 0)
	{
		float fScale = pkObj ? pkObj->GetScale() : 1.0f;
		pkChan->mNumScalingKeys = 2;
		pkChan->mScalingKeys = new aiVectorKey[2];
		aiVector3D kV(fScale, fScale, fScale);
		pkChan->mScalingKeys[0] = aiVectorKey(dStartTime, kV);
		pkChan->mScalingKeys[1] = aiVectorKey(dEndTime, kV);
	}
}

//--------------------------------------------------------------------------------------------------
std::vector<aiAnimation*> AnimationExporter::BuildFromNifControllers(NiAVObject* pkRoot)
{
	if (!pkRoot)
		return {};

	std::vector<NifControllerEntry> kControllers;
	CollectTransformControllers(pkRoot, kControllers);
	if (kControllers.empty())
		return {};

	const float fTicksPerSecond = 24.0f;

	// Determine overall duration from the max EndTime across controllers
	float fMaxTime = 0.0f;
	for (auto& kEntry : kControllers)
	{
		NiTransformController* pkCtrl = kEntry.pkCtrl;
		float fEnd = pkCtrl->GetLastTime();
		if (fEnd > fMaxTime)
			fMaxTime = fEnd;
	}
	if (fMaxTime <= 0.0f)
		return {};

	std::vector<aiNodeAnim*> kChannels;
	kChannels.reserve(kControllers.size());

	for (auto& kEntry : kControllers)
	{
		const std::string& kNodeName = kEntry.kName;
		NiTransformController* pkCtrl = kEntry.pkCtrl;

		NiInterpolator* pkInterp = pkCtrl->GetInterpolator(0);
		if (!pkInterp)
			continue;

		if (!NiIsKindOf(NiTransformInterpolator, pkInterp))
			continue;

		NiTransformInterpolator* pkTI = NiStaticCast(NiTransformInterpolator, pkInterp);

		aiNodeAnim* pkChan = new aiNodeAnim();
		pkChan->mNodeName = kNodeName.c_str();

		// -- Position keys --
		{
			unsigned int uiNumKeys = 0;
			NiPosKey::KeyType eType = NiPosKey::NOINTERP;
			unsigned char ucSize = 0;
			NiPosKey* pkKeys = pkTI->GetPosData(uiNumKeys, eType, ucSize);
			if (pkKeys && uiNumKeys > 0)
			{
				pkChan->mNumPositionKeys = uiNumKeys;
				pkChan->mPositionKeys = new aiVectorKey[uiNumKeys];
				for (unsigned int k = 0; k < uiNumKeys; ++k)
				{
					NiPosKey* pkKey = pkKeys->GetKeyAt(k, ucSize);
					double dTime = static_cast<double>(pkKey->GetTime()) * fTicksPerSecond;
					const NiPoint3& p = pkKey->GetPos();
					pkChan->mPositionKeys[k] = aiVectorKey(dTime, aiVector3D(p.x, p.y, p.z));
				}
			}
		}

		// -- Rotation keys --
		{
			unsigned int uiNumKeys = 0;
			NiRotKey::KeyType eType = NiRotKey::NOINTERP;
			unsigned char ucSize = 0;
			NiRotKey* pkKeys = pkTI->GetRotData(uiNumKeys, eType, ucSize);
			if (pkKeys && uiNumKeys > 0)
			{
				pkChan->mNumRotationKeys = uiNumKeys;
				pkChan->mRotationKeys = new aiQuatKey[uiNumKeys];
				for (unsigned int k = 0; k < uiNumKeys; ++k)
				{
					NiRotKey* pkKey = pkKeys->GetKeyAt(k, ucSize);
					double dTime = static_cast<double>(pkKey->GetTime()) * fTicksPerSecond;
					pkChan->mRotationKeys[k] = aiQuatKey(dTime, ToAiQuat(pkKey->GetQuaternion()));
				}
			}
		}

		// -- Scale keys --
		{
			unsigned int uiNumKeys = 0;
			NiFloatKey::KeyType eType = NiFloatKey::NOINTERP;
			unsigned char ucSize = 0;
			NiFloatKey* pkKeys = pkTI->GetScaleData(uiNumKeys, eType, ucSize);
			if (pkKeys && uiNumKeys > 0)
			{
				pkChan->mNumScalingKeys = uiNumKeys;
				pkChan->mScalingKeys = new aiVectorKey[uiNumKeys];
				for (unsigned int k = 0; k < uiNumKeys; ++k)
				{
					NiFloatKey* pkKey = pkKeys->GetKeyAt(k, ucSize);
					double dTime = static_cast<double>(pkKey->GetTime()) * fTicksPerSecond;
					float fS = pkKey->GetValue();
					pkChan->mScalingKeys[k] = aiVectorKey(dTime, aiVector3D(fS, fS, fS));
				}
			}
		}

		if (pkChan->mNumPositionKeys == 0 &&
			pkChan->mNumRotationKeys == 0 &&
			pkChan->mNumScalingKeys == 0)
		{
			delete pkChan;
			continue;
		}

		// Any component left with zero keys (e.g. a bone that only animates
		// rotation) must be filled in with a rest-pose default; otherwise
		// Assimp writes an empty curve node that breaks FBX consumers like
		// three.js's FBXLoader ("no keyframes in track named X.position").
		FillMissingKeysWithRestPose(pkChan, kEntry.pkObj, 0.0,
			static_cast<double>(fMaxTime) * fTicksPerSecond);

		kChannels.push_back(pkChan);
	}

	if (kChannels.empty())
		return {};

	aiAnimation* pkAnim = new aiAnimation();
	pkAnim->mName = "NifAnimation";
	pkAnim->mTicksPerSecond = fTicksPerSecond;
	pkAnim->mDuration = static_cast<double>(fMaxTime) * fTicksPerSecond;
	pkAnim->mNumChannels = static_cast<unsigned int>(kChannels.size());
	pkAnim->mChannels = new aiNodeAnim*[kChannels.size()];
	for (unsigned int c = 0; c < kChannels.size(); ++c)
		pkAnim->mChannels[c] = kChannels[c];
	pkAnim->mNumMeshChannels = 0;
	pkAnim->mMeshChannels = nullptr;
	pkAnim->mNumMorphMeshChannels = 0;
	pkAnim->mMorphMeshChannels = nullptr;

	return {pkAnim};
}
