#include "FbxWriter.h"
#include "AxisConversion.h"

#include <assimp/scene.h>
#include <assimp/Exporter.hpp>
#include <assimp/Importer.hpp>
#include <assimp/version.h>
#include <assimp/postprocess.h>
#include <assimp/metadata.h>

#include <NiNode.h>
#include <NiMesh.h>
#include <NiGeometry.h>

#include <filesystem>
#include <algorithm>
#include <cstring>
#include <iostream>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <set>
#include <sstream>

namespace fs = std::filesystem;

namespace
{
	bool NormalizeNormal(aiVector3D& kNormal)
	{
		if (!std::isfinite(kNormal.x) || !std::isfinite(kNormal.y) ||
			!std::isfinite(kNormal.z))
		{
			return false;
		}

		const float fLengthSquared = kNormal.x * kNormal.x +
			kNormal.y * kNormal.y + kNormal.z * kNormal.z;
		if (!std::isfinite(fLengthSquared) || fLengthSquared <= 1.0e-20f)
			return false;

		const float fInvLength = 1.0f / std::sqrt(fLengthSquared);
		kNormal.x *= fInvLength;
		kNormal.y *= fInvLength;
		kNormal.z *= fInvLength;
		return true;
	}

	std::vector<aiVector3D> BuildCompleteNormals(
		const IntermediateMesh& kMesh, unsigned int& uiGeneratedCount,
		unsigned int& uiFallbackCount)
	{
		const std::size_t stVertexCount = kMesh.positions.size();
		std::vector<aiVector3D> kAccumulated(stVertexCount,
			aiVector3D(0.0f, 0.0f, 0.0f));

		for (std::size_t i = 0; i + 2u < kMesh.indices.size(); i += 3u)
		{
			const unsigned int i0 = kMesh.indices[i + 0u];
			const unsigned int i1 = kMesh.indices[i + 1u];
			const unsigned int i2 = kMesh.indices[i + 2u];
			if (i0 >= stVertexCount || i1 >= stVertexCount ||
				i2 >= stVertexCount || i0 == i1 || i0 == i2 || i1 == i2)
			{
				continue;
			}

			const aiVector3D& p0 = kMesh.positions[i0];
			const aiVector3D& p1 = kMesh.positions[i1];
			const aiVector3D& p2 = kMesh.positions[i2];
			const float e1x = p1.x - p0.x;
			const float e1y = p1.y - p0.y;
			const float e1z = p1.z - p0.z;
			const float e2x = p2.x - p0.x;
			const float e2y = p2.y - p0.y;
			const float e2z = p2.z - p0.z;
			const aiVector3D kFace(
				e1y * e2z - e1z * e2y,
				e1z * e2x - e1x * e2z,
				e1x * e2y - e1y * e2x);
			aiVector3D kUnit = kFace;
			if (!NormalizeNormal(kUnit))
				continue;

			const unsigned int auiVertices[3] = { i0, i1, i2 };
			for (unsigned int uiVertex : auiVertices)
			{
				kAccumulated[uiVertex].x += kFace.x;
				kAccumulated[uiVertex].y += kFace.y;
				kAccumulated[uiVertex].z += kFace.z;
			}
		}

		const bool bHasSource = kMesh.normals.size() == stVertexCount;
		std::vector<aiVector3D> kResult(stVertexCount);
		uiGeneratedCount = 0;
		uiFallbackCount = 0;
		for (std::size_t v = 0; v < stVertexCount; ++v)
		{
			aiVector3D kNormal;
			bool bValid = false;
			if (bHasSource)
			{
				kNormal = kMesh.normals[v];
				bValid = NormalizeNormal(kNormal);
			}
			if (!bValid)
			{
				kNormal = kAccumulated[v];
				bValid = NormalizeNormal(kNormal);
				if (bValid)
					++uiGeneratedCount;
			}
			if (!bValid)
			{
				kNormal = aiVector3D(0.0f, 0.0f, 1.0f);
				++uiFallbackCount;
			}
			kResult[v] = kNormal;
		}
		return kResult;
	}

	bool IsFiniteVector(const aiVector3D& kValue)
	{
		return std::isfinite(kValue.x) && std::isfinite(kValue.y) &&
			std::isfinite(kValue.z);
	}

	bool IsFiniteQuaternion(const aiQuaternion& kValue)
	{
		if (!std::isfinite(kValue.w) || !std::isfinite(kValue.x) ||
			!std::isfinite(kValue.y) || !std::isfinite(kValue.z))
		{
			return false;
		}

		const double dLengthSquared =
			static_cast<double>(kValue.w) * kValue.w +
			static_cast<double>(kValue.x) * kValue.x +
			static_cast<double>(kValue.y) * kValue.y +
			static_cast<double>(kValue.z) * kValue.z;
		return std::isfinite(dLengthSquared) && dLengthSquared > 1.0e-20;
	}

	bool IsFiniteMatrix(const aiMatrix4x4& kValue)
	{
		const float afValues[16] =
		{
			kValue.a1, kValue.a2, kValue.a3, kValue.a4,
			kValue.b1, kValue.b2, kValue.b3, kValue.b4,
			kValue.c1, kValue.c2, kValue.c3, kValue.c4,
			kValue.d1, kValue.d2, kValue.d3, kValue.d4
		};
		for (float fValue : afValues)
		{
			if (!std::isfinite(fValue))
				return false;
		}
		return true;
	}

	bool ValidateFiniteNode(const aiNode* pkNode, const std::string& kPath,
		std::string& kError)
	{
		if (!pkNode)
			return true;

		const std::string kName = pkNode->mName.length > 0
			? pkNode->mName.C_Str() : "<unnamed>";
		const std::string kNodePath = kPath.empty()
			? kName : kPath + "/" + kName;
		if (!IsFiniteMatrix(pkNode->mTransformation))
		{
			kError = "Non-finite node transform at '" + kNodePath + "'.";
			return false;
		}

		for (unsigned int i = 0; i < pkNode->mNumChildren; ++i)
		{
			if (!ValidateFiniteNode(pkNode->mChildren[i], kNodePath, kError))
				return false;
		}
		return true;
	}

	bool ValidateFiniteScene(const aiScene* pkScene, std::string& kError)
	{
		if (!pkScene || !pkScene->mRootNode)
		{
			kError = "Assimp scene has no root node.";
			return false;
		}

		if (!ValidateFiniteNode(pkScene->mRootNode, std::string(), kError))
			return false;

		for (unsigned int m = 0; m < pkScene->mNumMeshes; ++m)
		{
			const aiMesh* pkMesh = pkScene->mMeshes[m];
			if (!pkMesh)
				continue;
			const std::string kMeshName = pkMesh->mName.length > 0
				? pkMesh->mName.C_Str() : ("mesh_" + std::to_string(m));

			for (unsigned int v = 0; v < pkMesh->mNumVertices; ++v)
			{
				if (!IsFiniteVector(pkMesh->mVertices[v]))
				{
					kError = "Non-finite vertex " + std::to_string(v) +
						" in mesh '" + kMeshName + "'.";
					return false;
				}
				if (pkMesh->mNormals && !IsFiniteVector(pkMesh->mNormals[v]))
				{
					kError = "Non-finite normal " + std::to_string(v) +
						" in mesh '" + kMeshName + "'.";
					return false;
				}
			}

			for (unsigned int b = 0; b < pkMesh->mNumBones; ++b)
			{
				const aiBone* pkBone = pkMesh->mBones[b];
				if (!pkBone)
					continue;
				const std::string kBoneName = pkBone->mName.length > 0
					? pkBone->mName.C_Str() : ("bone_" + std::to_string(b));
				if (!IsFiniteMatrix(pkBone->mOffsetMatrix))
				{
					kError = "Non-finite inverse-bind matrix for bone '" +
						kBoneName + "' in mesh '" + kMeshName + "'.";
					return false;
				}
				for (unsigned int w = 0; w < pkBone->mNumWeights; ++w)
				{
					const aiVertexWeight& kWeight = pkBone->mWeights[w];
					if (kWeight.mVertexId >= pkMesh->mNumVertices ||
						!std::isfinite(kWeight.mWeight) || kWeight.mWeight < 0.0f)
					{
						kError = "Invalid weight " + std::to_string(w) +
							" for bone '" + kBoneName + "' in mesh '" +
							kMeshName + "'.";
						return false;
					}
				}
			}
		}

		for (unsigned int a = 0; a < pkScene->mNumAnimations; ++a)
		{
			const aiAnimation* pkAnimation = pkScene->mAnimations[a];
			if (!pkAnimation)
				continue;
			const std::string kAnimationName = pkAnimation->mName.length > 0
				? pkAnimation->mName.C_Str() : ("animation_" + std::to_string(a));
			if (!std::isfinite(pkAnimation->mDuration) ||
				!std::isfinite(pkAnimation->mTicksPerSecond))
			{
				kError = "Non-finite timing in animation '" + kAnimationName + "'.";
				return false;
			}

			for (unsigned int c = 0; c < pkAnimation->mNumChannels; ++c)
			{
				const aiNodeAnim* pkChannel = pkAnimation->mChannels[c];
				if (!pkChannel)
					continue;
				const std::string kChannelName = pkChannel->mNodeName.C_Str();
				for (unsigned int i = 0; i < pkChannel->mNumPositionKeys; ++i)
				{
					if (!std::isfinite(pkChannel->mPositionKeys[i].mTime) ||
						!IsFiniteVector(pkChannel->mPositionKeys[i].mValue))
					{
						kError = "Non-finite position key " + std::to_string(i) +
							" for node '" + kChannelName + "' in animation '" +
							kAnimationName + "'.";
						return false;
					}
				}
				for (unsigned int i = 0; i < pkChannel->mNumRotationKeys; ++i)
				{
					if (!std::isfinite(pkChannel->mRotationKeys[i].mTime) ||
						!IsFiniteQuaternion(pkChannel->mRotationKeys[i].mValue))
					{
						kError = "Invalid rotation key " + std::to_string(i) +
							" for node '" + kChannelName + "' in animation '" +
							kAnimationName + "'.";
						return false;
					}
				}
				for (unsigned int i = 0; i < pkChannel->mNumScalingKeys; ++i)
				{
					if (!std::isfinite(pkChannel->mScalingKeys[i].mTime) ||
						!IsFiniteVector(pkChannel->mScalingKeys[i].mValue))
					{
						kError = "Non-finite scale key " + std::to_string(i) +
							" for node '" + kChannelName + "' in animation '" +
							kAnimationName + "'.";
						return false;
					}
				}
			}
		}

		return true;
	}

	struct AxisMetadata
	{
		int32_t upAxis = 1;
		int32_t upAxisSign = 1;
		int32_t frontAxis = 2;
		int32_t frontAxisSign = 1;
		int32_t coordAxis = 0;
		int32_t coordAxisSign = 1;
	};

	AxisMetadata GetExpectedAxisMetadata(ExportAxisPreset ePreset,
		ExportHandedness eHandedness)
	{
		AxisMetadata kResult;
		switch (ePreset)
		{
		case ExportAxisPreset::Unreal:
			kResult.upAxis = 2;
			kResult.upAxisSign = 1;
			kResult.frontAxis = 0;
			kResult.frontAxisSign = -1;
			kResult.coordAxis = 1;
			kResult.coordAxisSign =
				eHandedness == ExportHandedness::Left ? 1 : -1;
			break;
		case ExportAxisPreset::Unity:
			kResult.upAxis = 1;
			kResult.upAxisSign = 1;
			kResult.frontAxis = 2;
			kResult.frontAxisSign =
				eHandedness == ExportHandedness::Left ? 1 : -1;
			kResult.coordAxis = 0;
			kResult.coordAxisSign = 1;
			break;
		case ExportAxisPreset::Native:
		default:
			kResult.upAxis = 2;
			kResult.upAxisSign = 1;
			kResult.frontAxis = 1;
			kResult.frontAxisSign = -1;
			kResult.coordAxis = 0;
			kResult.coordAxisSign =
				eHandedness == ExportHandedness::Left ? -1 : 1;
			break;
		}
		return kResult;
	}

	bool IsAssimpVersionAtLeast(unsigned int uiMajor,
		unsigned int uiMinor, unsigned int uiRevision)
	{
		const unsigned int uiRuntimeMajor = aiGetVersionMajor();
		const unsigned int uiRuntimeMinor = aiGetVersionMinor();
		const unsigned int uiRuntimeRevision = aiGetVersionRevision();
		if (uiRuntimeMajor != uiMajor)
			return uiRuntimeMajor > uiMajor;
		if (uiRuntimeMinor != uiMinor)
			return uiRuntimeMinor > uiMinor;
		return uiRuntimeRevision >= uiRevision;
	}

	const aiExportFormatDesc* FindExporterFormat(
		const Assimp::Exporter& kExporter, const char* pcId)
	{
		for (size_t i = 0; i < kExporter.GetExportFormatCount(); ++i)
		{
			const aiExportFormatDesc* pkDescription =
				kExporter.GetExportFormatDescription(i);
			if (pkDescription && pkDescription->id &&
				std::strcmp(pkDescription->id, pcId) == 0)
			{
				return pkDescription;
			}
		}
		return nullptr;
	}

	bool ReadWrittenFbxVersion(const std::string& kPath,
		uint32_t& uiVersion, std::string& kError)
	{
		uiVersion = 0;
		std::ifstream kFile(kPath, std::ios::binary);
		if (!kFile)
		{
			kError = "Could not reopen the written FBX file.";
			return false;
		}

		char acHeader[27] = {};
		kFile.read(acHeader, sizeof(acHeader));
		if (kFile.gcount() < static_cast<std::streamsize>(sizeof(acHeader)))
		{
			kError = "Written FBX file is too small.";
			return false;
		}

		static constexpr char BINARY_PREFIX[] = "Kaydara FBX Binary";
		if (std::memcmp(acHeader, BINARY_PREFIX,
			sizeof(BINARY_PREFIX) - 1) == 0)
		{
			const unsigned char* p = reinterpret_cast<const unsigned char*>(
				acHeader + 23);
			uiVersion = static_cast<uint32_t>(p[0]) |
				(static_cast<uint32_t>(p[1]) << 8u) |
				(static_cast<uint32_t>(p[2]) << 16u) |
				(static_cast<uint32_t>(p[3]) << 24u);
			return true;
		}

		kFile.clear();
		kFile.seekg(0);
		std::string kFirstLine;
		std::getline(kFile, kFirstLine);
		const size_t stFbx = kFirstLine.find("FBX ");
		if (stFbx == std::string::npos)
		{
			kError = "Written file does not contain an FBX header.";
			return false;
		}

		unsigned int uiMajor = 0;
		unsigned int uiMinor = 0;
		unsigned int uiPatch = 0;
		if (std::sscanf(kFirstLine.c_str() + stFbx, "FBX %u.%u.%u",
			&uiMajor, &uiMinor, &uiPatch) != 3)
		{
			kError = "Could not parse the ASCII FBX version.";
			return false;
		}
		uiVersion = uiMajor * 1000u + uiMinor * 100u + uiPatch * 10u;
		return true;
	}

	bool GetMetadataInt(const aiMetadata* pkMetadata,
		const char* pcKey, int32_t& iValue)
	{
		return pkMetadata && pkMetadata->Get(pcKey, iValue);
	}

	void CollectSkeletonNodeNames(const aiScene* pkScene,
		std::set<std::string>& kOut)
	{
		if (!pkScene || !pkScene->mRootNode)
			return;

		for (unsigned int m = 0; m < pkScene->mNumMeshes; ++m)
		{
			const aiMesh* pkMesh = pkScene->mMeshes[m];
			if (!pkMesh)
				continue;
			for (unsigned int b = 0; b < pkMesh->mNumBones; ++b)
			{
				const aiBone* pkBone = pkMesh->mBones[b];
				if (!pkBone)
					continue;
				const aiNode* pkNode = pkScene->mRootNode->FindNode(pkBone->mName);
				for (; pkNode && pkNode != pkScene->mRootNode;
					pkNode = pkNode->mParent)
				{
					kOut.emplace(pkNode->mName.C_Str());
				}
			}
		}
	}

	struct ScaleReadbackStats
	{
		unsigned int decomposedNonUniformKeys = 0;
		float maximumRelativeMagnitudeSpread = 0.0f;
		std::string firstNode;
		std::string firstAnimation;
		unsigned int firstKey = 0;
		aiVector3D firstScale = aiVector3D(1.0f, 1.0f, 1.0f);
	};

	std::string FormatScale(const aiVector3D& kScale)
	{
		std::ostringstream kStream;
		kStream << "(" << kScale.x << ", " << kScale.y << ", "
			<< kScale.z << ")";
		return kStream.str();
	}

	bool ValidateSourceScalarScaleTracks(const aiScene* pkScene,
		std::string& kError)
	{
		std::set<std::string> kSkeletonNodes;
		CollectSkeletonNodeNames(pkScene, kSkeletonNodes);
		for (unsigned int a = 0; a < pkScene->mNumAnimations; ++a)
		{
			const aiAnimation* pkAnimation = pkScene->mAnimations[a];
			if (!pkAnimation)
				continue;
			for (unsigned int c = 0; c < pkAnimation->mNumChannels; ++c)
			{
				const aiNodeAnim* pkChannel = pkAnimation->mChannels[c];
				if (!pkChannel || kSkeletonNodes.find(
					pkChannel->mNodeName.C_Str()) == kSkeletonNodes.end())
				{
					continue;
				}

				for (unsigned int i = 0; i < pkChannel->mNumScalingKeys; ++i)
				{
					const aiVector3D& kScale = pkChannel->mScalingKeys[i].mValue;
					const float fScaleMagnitude = std::max({1.0f,
						std::abs(kScale.x), std::abs(kScale.y),
						std::abs(kScale.z)});
					if (std::abs(kScale.x) > 1000.0f ||
						std::abs(kScale.y) > 1000.0f ||
						std::abs(kScale.z) > 1000.0f)
					{
						kError = "Exporter generated excessive source scale " +
							FormatScale(kScale) + " for skeleton node '" +
							std::string(pkChannel->mNodeName.C_Str()) +
							"' in animation '" +
							std::string(pkAnimation->mName.C_Str()) +
							"' at key " + std::to_string(i) + ".";
						return false;
					}
					const float fUniformTolerance = 0.000001f * fScaleMagnitude;
					if (std::abs(kScale.x - kScale.y) > fUniformTolerance ||
						std::abs(kScale.x - kScale.z) > fUniformTolerance)
					{
						kError = "Exporter generated non-uniform source scale " +
							FormatScale(kScale) + " for scalar-scale NIF node '" +
							std::string(pkChannel->mNodeName.C_Str()) +
							"' in animation '" +
							std::string(pkAnimation->mName.C_Str()) +
							"' at key " + std::to_string(i) + ".";
						return false;
					}
				}
			}
		}
		return true;
	}

	bool ValidateReadbackSkeletonScales(const aiScene* pkScene,
		ScaleReadbackStats& kStats, std::string& kError)
	{
		std::set<std::string> kSkeletonNodes;
		CollectSkeletonNodeNames(pkScene, kSkeletonNodes);
		for (unsigned int a = 0; a < pkScene->mNumAnimations; ++a)
		{
			const aiAnimation* pkAnimation = pkScene->mAnimations[a];
			if (!pkAnimation)
				continue;
			for (unsigned int c = 0; c < pkAnimation->mNumChannels; ++c)
			{
				const aiNodeAnim* pkChannel = pkAnimation->mChannels[c];
				if (!pkChannel || kSkeletonNodes.find(
					pkChannel->mNodeName.C_Str()) == kSkeletonNodes.end())
				{
					continue;
				}

				for (unsigned int i = 0; i < pkChannel->mNumScalingKeys; ++i)
				{
					const aiVector3D& kScale = pkChannel->mScalingKeys[i].mValue;
					const float fMaxAbs = std::max({std::abs(kScale.x),
						std::abs(kScale.y), std::abs(kScale.z)});
					if (fMaxAbs > 1000.0f)
					{
						kError = "Excessive animated skeleton scale " +
							FormatScale(kScale) + " for node '" +
							std::string(pkChannel->mNodeName.C_Str()) +
							"' in animation '" +
							std::string(pkAnimation->mName.C_Str()) +
							"' at key " + std::to_string(i) + ".";
						return false;
					}

					const float fMinAbs = std::min({std::abs(kScale.x),
						std::abs(kScale.y), std::abs(kScale.z)});
					const float fDenominator = std::max(0.000001f, fMaxAbs);
					const float fRelativeSpread =
						(fMaxAbs - fMinAbs) / fDenominator;
					// FBX importers can redistribute the sign of a reflected
					// uniform scale and introduce small decomposition drift. The
					// source aiScene is validated strictly before writing, so only
					// report meaningful magnitude drift during read-back.
					const bool bMagnitudeUniform = fRelativeSpread <= 0.005f;
					if (!bMagnitudeUniform)
					{
						if (kStats.decomposedNonUniformKeys == 0)
						{
							kStats.firstNode = pkChannel->mNodeName.C_Str();
							kStats.firstAnimation = pkAnimation->mName.C_Str();
							kStats.firstKey = i;
							kStats.firstScale = kScale;
						}
						++kStats.decomposedNonUniformKeys;
						kStats.maximumRelativeMagnitudeSpread = std::max(
							kStats.maximumRelativeMagnitudeSpread, fRelativeSpread);
					}
				}
			}
		}
		return true;
	}

	bool VerifyWrittenFbx(const std::string& kPath,
		const AxisMetadata& kExpectedAxes,
		unsigned int uiExpectedAnimations, bool bStrictAxisMetadata,
		std::string& kError)
	{
		uint32_t uiFbxVersion = 0;
		std::string kHeaderError;
		if (!ReadWrittenFbxVersion(kPath, uiFbxVersion, kHeaderError))
		{
			kError = "FBX read-back verification failed: " + kHeaderError;
			return false;
		}

		if (uiFbxVersion < 7400u)
		{
			std::ostringstream kMessage;
			kMessage << "Written FBX version " << uiFbxVersion
				<< " is older than FBX 7.4 and is not supported by the "
					"post-export verifier.";
			kError = kMessage.str();
			return false;
		}

		if (uiFbxVersion < 7500u)
		{
			std::cerr << "Warning: linked Assimp wrote FBX " << uiFbxVersion
				<< " instead of FBX 7500. Finite data, animation count, and "
					"skeleton scale will still be validated." << std::endl;
		}

		Assimp::Importer kImporter;
		const aiScene* pkImported = kImporter.ReadFile(kPath,
			aiProcess_ValidateDataStructure);
		if (!pkImported)
		{
			kError = std::string("FBX read-back import failed: ") +
				kImporter.GetErrorString();
			return false;
		}

		std::string kFiniteError;
		if (!ValidateFiniteScene(pkImported, kFiniteError))
		{
			kError = "Written FBX contains invalid data after read-back: " +
				kFiniteError;
			return false;
		}

		ScaleReadbackStats kScaleReadbackStats;
		std::string kScaleError;
		if (!ValidateReadbackSkeletonScales(pkImported,
			kScaleReadbackStats, kScaleError))
		{
			kError = "Written FBX failed skeleton-scale validation: " +
				kScaleError;
			return false;
		}

		if (kScaleReadbackStats.decomposedNonUniformKeys > 0)
		{
			std::cerr << "Warning: Assimp FBX read-back decomposed "
				<< kScaleReadbackStats.decomposedNonUniformKeys
				<< " scalar-scale key(s) into signed/non-uniform XYZ values. "
				<< "The pre-export curves were verified strictly uniform; this "
				<< "round-trip decomposition is not used to rewrite the FBX. "
				<< "First: node='" << kScaleReadbackStats.firstNode
				<< "' animation='" << kScaleReadbackStats.firstAnimation
				<< "' key=" << kScaleReadbackStats.firstKey
				<< " value=" << FormatScale(kScaleReadbackStats.firstScale)
				<< "; maximum absolute-magnitude spread="
				<< (kScaleReadbackStats.maximumRelativeMagnitudeSpread * 100.0f)
				<< "%." << std::endl;
		}

		int32_t iUpAxis = 0;
		int32_t iUpAxisSign = 0;
		int32_t iFrontAxis = 0;
		int32_t iFrontAxisSign = 0;
		int32_t iCoordAxis = 0;
		int32_t iCoordAxisSign = 0;
		const bool bHasAxisMetadata =
			GetMetadataInt(pkImported->mMetaData, "UpAxis", iUpAxis) &&
			GetMetadataInt(pkImported->mMetaData, "UpAxisSign", iUpAxisSign) &&
			GetMetadataInt(pkImported->mMetaData, "FrontAxis", iFrontAxis) &&
			GetMetadataInt(pkImported->mMetaData, "FrontAxisSign", iFrontAxisSign) &&
			GetMetadataInt(pkImported->mMetaData, "CoordAxis", iCoordAxis) &&
			GetMetadataInt(pkImported->mMetaData, "CoordAxisSign", iCoordAxisSign);

		if (!bHasAxisMetadata)
		{
			const char* pcMessage =
				"Written FBX did not expose complete axis metadata during read-back.";
			if (bStrictAxisMetadata)
			{
				kError = pcMessage;
				return false;
			}
			std::cerr << "Warning: " << pcMessage
				<< " Continuing because this Assimp version predates the "
					"strict metadata requirement." << std::endl;
		}

		const bool bAxisMetadataMatches = bHasAxisMetadata &&
			iUpAxis == kExpectedAxes.upAxis &&
			iUpAxisSign == kExpectedAxes.upAxisSign &&
			iFrontAxis == kExpectedAxes.frontAxis &&
			iFrontAxisSign == kExpectedAxes.frontAxisSign &&
			iCoordAxis == kExpectedAxes.coordAxis &&
			iCoordAxisSign == kExpectedAxes.coordAxisSign;

		if (bHasAxisMetadata && !bAxisMetadataMatches)
		{
			std::ostringstream kMessage;
			kMessage << "Written FBX axis metadata mismatch: got Up="
				<< iUpAxis << "/" << iUpAxisSign << " Front="
				<< iFrontAxis << "/" << iFrontAxisSign << " Coord="
				<< iCoordAxis << "/" << iCoordAxisSign << ", expected Up="
				<< kExpectedAxes.upAxis << "/" << kExpectedAxes.upAxisSign
				<< " Front=" << kExpectedAxes.frontAxis << "/"
				<< kExpectedAxes.frontAxisSign << " Coord="
				<< kExpectedAxes.coordAxis << "/"
				<< kExpectedAxes.coordAxisSign << ".";
			if (bStrictAxisMetadata)
			{
				kError = kMessage.str();
				return false;
			}
			std::cerr << "Warning: " << kMessage.str()
				<< " The FBX remains usable, but automatic axis conversion by "
					"the destination application may differ." << std::endl;
		}

		if (pkImported->mNumAnimations != uiExpectedAnimations)
		{
			std::ostringstream kMessage;
			kMessage << "Written FBX read-back animation count differs: got "
				<< pkImported->mNumAnimations << ", expected "
				<< uiExpectedAnimations << ".";

			if (uiExpectedAnimations > 0 && pkImported->mNumAnimations == 0)
			{
				kError = kMessage.str() +
					" The written file exposed no animation during verification.";
				return false;
			}

			std::cerr << "Warning: " << kMessage.str()
				<< " Assimp 6.0.x can omit or merge one AnimationStack during "
					"its own FBX round-trip import. The file is retained because "
					"geometry, finite transforms, source curves, and skeleton "
					"scales passed validation. Read-back clips:";
			for (unsigned int i = 0; i < pkImported->mNumAnimations; ++i)
			{
				const aiAnimation* pkAnimation = pkImported->mAnimations[i];
				std::cerr << " '" << (pkAnimation ? pkAnimation->mName.C_Str() : "<null>")
					<< "'";
			}
			std::cerr << "." << std::endl;
		}

		std::cout << "    Verified written FBX: version=" << uiFbxVersion;
		if (bHasAxisMetadata)
		{
			std::cout << " axes=Up(" << iUpAxis << "," << iUpAxisSign
				<< ") Front(" << iFrontAxis << "," << iFrontAxisSign
				<< ") Coord(" << iCoordAxis << "," << iCoordAxisSign << ")";
		}
		else
		{
			std::cout << " axes=<not exposed by read-back importer>";
		}
		std::cout << " animations=" << pkImported->mNumAnimations << "."
			<< std::endl;
		return true;
	}

	enum class ReflectionAxis
	{
		X,
		Y,
		Z
	};

	ReflectionAxis GetLeftHandedReflectionAxis(ExportAxisPreset ePreset)
	{
		switch (ePreset)
		{
		case ExportAxisPreset::Unreal:
			// Preserve +X forward and +Z up; change -Y right to +Y right.
			return ReflectionAxis::Y;
		case ExportAxisPreset::Unity:
			// Preserve +X right and +Y up; change -Z forward to +Z forward.
			return ReflectionAxis::Z;
		case ExportAxisPreset::Native:
		default:
			// Preserve +Y forward and +Z up by reflecting the remaining X axis.
			return ReflectionAxis::X;
		}
	}

	void ReflectVector(aiVector3D& kValue, ReflectionAxis eAxis)
	{
		switch (eAxis)
		{
		case ReflectionAxis::X:
			kValue.x = -kValue.x;
			break;
		case ReflectionAxis::Y:
			kValue.y = -kValue.y;
			break;
		case ReflectionAxis::Z:
			kValue.z = -kValue.z;
			break;
		}
	}

	void ReflectQuaternion(aiQuaternion& kValue, ReflectionAxis eAxis)
	{
		// A quaternion vector is an axial vector. For an improper orthogonal
		// basis C, R' = C R C^-1 maps its vector part as det(C) * C * v.
		switch (eAxis)
		{
		case ReflectionAxis::X:
			kValue.y = -kValue.y;
			kValue.z = -kValue.z;
			break;
		case ReflectionAxis::Y:
			kValue.x = -kValue.x;
			kValue.z = -kValue.z;
			break;
		case ReflectionAxis::Z:
			kValue.x = -kValue.x;
			kValue.y = -kValue.y;
			break;
		}
	}

	void ReflectMatrix(aiMatrix4x4& kValue, ReflectionAxis eAxis)
	{
		// Conjugate by the reflection matrix: M' = S * M * S. The diagonal
		// intersection is negated twice and therefore remains unchanged.
		switch (eAxis)
		{
		case ReflectionAxis::X:
			kValue.a2 = -kValue.a2;
			kValue.a3 = -kValue.a3;
			kValue.a4 = -kValue.a4;
			kValue.b1 = -kValue.b1;
			kValue.c1 = -kValue.c1;
			kValue.d1 = -kValue.d1;
			break;
		case ReflectionAxis::Y:
			kValue.b1 = -kValue.b1;
			kValue.b3 = -kValue.b3;
			kValue.b4 = -kValue.b4;
			kValue.a2 = -kValue.a2;
			kValue.c2 = -kValue.c2;
			kValue.d2 = -kValue.d2;
			break;
		case ReflectionAxis::Z:
			kValue.c1 = -kValue.c1;
			kValue.c2 = -kValue.c2;
			kValue.c4 = -kValue.c4;
			kValue.a3 = -kValue.a3;
			kValue.b3 = -kValue.b3;
			kValue.d3 = -kValue.d3;
			break;
		}
	}

	void ReflectNode(aiNode* pkNode, ReflectionAxis eAxis)
	{
		if (!pkNode)
			return;

		ReflectMatrix(pkNode->mTransformation, eAxis);
		for (unsigned int i = 0; i < pkNode->mNumChildren; ++i)
			ReflectNode(pkNode->mChildren[i], eAxis);
	}

	void ReflectMesh(aiMesh* pkMesh, ReflectionAxis eAxis)
	{
		if (!pkMesh)
			return;

		for (unsigned int i = 0; i < pkMesh->mNumVertices; ++i)
		{
			ReflectVector(pkMesh->mVertices[i], eAxis);
			if (pkMesh->mNormals)
				ReflectVector(pkMesh->mNormals[i], eAxis);
			if (pkMesh->mTangents)
				ReflectVector(pkMesh->mTangents[i], eAxis);
			if (pkMesh->mBitangents)
				ReflectVector(pkMesh->mBitangents[i], eAxis);
		}

		for (unsigned int i = 0; i < pkMesh->mNumBones; ++i)
		{
			if (pkMesh->mBones[i])
				ReflectMatrix(pkMesh->mBones[i]->mOffsetMatrix, eAxis);
		}

		for (unsigned int a = 0; a < pkMesh->mNumAnimMeshes; ++a)
		{
			aiAnimMesh* pkAnimMesh = pkMesh->mAnimMeshes[a];
			if (!pkAnimMesh)
				continue;

			for (unsigned int i = 0; i < pkAnimMesh->mNumVertices; ++i)
			{
				if (pkAnimMesh->mVertices)
					ReflectVector(pkAnimMesh->mVertices[i], eAxis);
				if (pkAnimMesh->mNormals)
					ReflectVector(pkAnimMesh->mNormals[i], eAxis);
				if (pkAnimMesh->mTangents)
					ReflectVector(pkAnimMesh->mTangents[i], eAxis);
				if (pkAnimMesh->mBitangents)
					ReflectVector(pkAnimMesh->mBitangents[i], eAxis);
			}
		}
	}

	void ReflectAnimation(aiAnimation* pkAnimation, ReflectionAxis eAxis)
	{
		if (!pkAnimation)
			return;

		for (unsigned int c = 0; c < pkAnimation->mNumChannels; ++c)
		{
			aiNodeAnim* pkChannel = pkAnimation->mChannels[c];
			if (!pkChannel)
				continue;

			for (unsigned int i = 0; i < pkChannel->mNumPositionKeys; ++i)
				ReflectVector(pkChannel->mPositionKeys[i].mValue, eAxis);
			for (unsigned int i = 0; i < pkChannel->mNumRotationKeys; ++i)
				ReflectQuaternion(pkChannel->mRotationKeys[i].mValue, eAxis);
		}
	}

	void ReflectSceneToLeftHanded(aiScene* pkScene, ExportAxisPreset ePreset)
	{
		if (!pkScene)
			return;

		const ReflectionAxis eAxis = GetLeftHandedReflectionAxis(ePreset);
		ReflectNode(pkScene->mRootNode, eAxis);

		for (unsigned int i = 0; i < pkScene->mNumMeshes; ++i)
			ReflectMesh(pkScene->mMeshes[i], eAxis);
		for (unsigned int i = 0; i < pkScene->mNumAnimations; ++i)
			ReflectAnimation(pkScene->mAnimations[i], eAxis);

		for (unsigned int i = 0; i < pkScene->mNumCameras; ++i)
		{
			aiCamera* pkCamera = pkScene->mCameras[i];
			if (!pkCamera)
				continue;
			ReflectVector(pkCamera->mPosition, eAxis);
			ReflectVector(pkCamera->mLookAt, eAxis);
			ReflectVector(pkCamera->mUp, eAxis);
		}

		for (unsigned int i = 0; i < pkScene->mNumLights; ++i)
		{
			aiLight* pkLight = pkScene->mLights[i];
			if (!pkLight)
				continue;
			ReflectVector(pkLight->mPosition, eAxis);
			ReflectVector(pkLight->mDirection, eAxis);
			ReflectVector(pkLight->mUp, eAxis);
		}
	}
}

//--------------------------------------------------------------------------------------------------
FbxWriter::FbxWriter(const TextureExporter& kTexExporter, float fUnitScale,
	ExportAxisPreset eAxisPreset, ExportHandedness eHandedness)
	: m_kTexExporter(kTexExporter)
	, m_fUnitScale(std::isfinite(fUnitScale) && fUnitScale > 0.0f
		? fUnitScale : 1.0f)
	, m_eAxisPreset(eAxisPreset)
	, m_eHandedness(eHandedness)
{
}

//--------------------------------------------------------------------------------------------------
aiMesh* FbxWriter::BuildAiMesh(const IntermediateMesh& kIn) const
{
	if (kIn.positions.empty() || kIn.indices.empty())
		return nullptr;

	aiMesh* pkMesh = new aiMesh();
	pkMesh->mName = kIn.name.c_str();
	pkMesh->mPrimitiveTypes = aiPrimitiveType_TRIANGLE;
	pkMesh->mMaterialIndex = kIn.materialIndex;

	// ---- Vertices ----
	unsigned int uiVertCount = static_cast<unsigned int>(kIn.positions.size());
	pkMesh->mNumVertices = uiVertCount;
	pkMesh->mVertices = new aiVector3D[uiVertCount];
	for (unsigned int i = 0; i < uiVertCount; ++i)
	{
		const aiVector3D kScaledPosition(
			kIn.positions[i].x * m_fUnitScale,
			kIn.positions[i].y * m_fUnitScale,
			kIn.positions[i].z * m_fUnitScale);
		pkMesh->mVertices[i] = AxisConversion::ToTargetVector(
			kScaledPosition, m_eAxisPreset);
	}

	// ---- Normals ----
	// Always provide a complete, finite normal stream to Assimp. Source NIF
	// normals are preserved when valid; missing/invalid entries are generated
	// from the exported triangle topology as a final safety net.
	unsigned int uiGeneratedNormals = 0;
	unsigned int uiFallbackNormals = 0;
	const std::vector<aiVector3D> kNormals = BuildCompleteNormals(
		kIn, uiGeneratedNormals, uiFallbackNormals);
	pkMesh->mNormals = new aiVector3D[uiVertCount];
	for (unsigned int i = 0; i < uiVertCount; ++i)
	{
		pkMesh->mNormals[i] = AxisConversion::ToTargetVector(
			kNormals[i], m_eAxisPreset);
	}
	if (uiGeneratedNormals != 0u || uiFallbackNormals != 0u)
	{
		std::cerr << "Warning: completed normal stream for mesh '"
			<< kIn.name << "': generated=" << uiGeneratedNormals
			<< " fallback=" << uiFallbackNormals << std::endl;
	}

	// ---- UVs ----
	if (kIn.uvs.size() == uiVertCount)
	{
		pkMesh->mTextureCoords[0] = new aiVector3D[uiVertCount];
		pkMesh->mNumUVComponents[0] = 2;
		for (unsigned int i = 0; i < uiVertCount; ++i)
			pkMesh->mTextureCoords[0][i] = aiVector3D(kIn.uvs[i].x, kIn.uvs[i].y, 0.0f);
	}

	// ---- Faces ----
	unsigned int uiFaceCount = static_cast<unsigned int>(kIn.indices.size()) / 3u;
	pkMesh->mNumFaces = uiFaceCount;
	pkMesh->mFaces = new aiFace[uiFaceCount];
	for (unsigned int f = 0; f < uiFaceCount; ++f)
	{
		aiFace& kFace = pkMesh->mFaces[f];
		kFace.mNumIndices = 3;
		kFace.mIndices = new unsigned int[3];
		kFace.mIndices[0] = kIn.indices[f * 3 + 0];
		kFace.mIndices[1] = kIn.indices[f * 3 + 1];
		kFace.mIndices[2] = kIn.indices[f * 3 + 2];
	}

	// ---- Bones (skinning) ----
	if (!kIn.boneNames.empty() && !kIn.boneWeights.empty())
	{
		const unsigned int uiBoneCount =
			static_cast<unsigned int>(kIn.boneNames.size());

		auto IsValidBone = [&](unsigned int uiBoneIndex)
		{
			if (uiBoneIndex >= uiBoneCount)
				return false;
			if (!kIn.boneValid.empty())
			{
				return uiBoneIndex < kIn.boneValid.size() &&
					kIn.boneValid[uiBoneIndex];
			}
			return kIn.boneNames[uiBoneIndex].rfind("missing_bone_", 0) != 0;
		};

		const bool bHasFallback = IsValidBone(kIn.fallbackBoneIndex);

		// Revalidate every vertex immediately before constructing Assimp bones.
		// This guarantees that an invalid palette index, missing source bone, NaN
		// weight, or short boneWeights array cannot reach the FBX writer as an
		// unweighted vertex.
		struct BoneAccum { std::vector<aiVertexWeight> weights; };
		std::vector<BoneAccum> kBoneAccum(uiBoneCount);

		unsigned int uiFinalFallbackVertices = 0;
		unsigned int uiFinalInvalidInfluences = 0;
		unsigned int uiStillUnweighted = 0;
		unsigned int uiMinInfluences = 4;
		unsigned int uiMaxInfluences = 0;
		float fMaxNormalizedWeightError = 0.0f;

		for (unsigned int v = 0; v < uiVertCount; ++v)
		{
			std::vector<VertexBoneWeight> kWeights;
			if (v < kIn.boneWeights.size())
			{
				for (const VertexBoneWeight& kWeight : kIn.boneWeights[v])
				{
					if (!IsValidBone(kWeight.boneIndex) ||
						!std::isfinite(kWeight.weight) ||
						kWeight.weight <= 0.0f)
					{
						++uiFinalInvalidInfluences;
						continue;
					}

					auto kExisting = std::find_if(kWeights.begin(), kWeights.end(),
						[&](const VertexBoneWeight& kMerged)
						{
							return kMerged.boneIndex == kWeight.boneIndex;
						});
					if (kExisting != kWeights.end())
						kExisting->weight += kWeight.weight;
					else
						kWeights.push_back(kWeight);
				}
			}

			std::stable_sort(kWeights.begin(), kWeights.end(),
				[](const VertexBoneWeight& kLeft, const VertexBoneWeight& kRight)
				{
					return kLeft.weight > kRight.weight;
				});
			if (kWeights.size() > 4)
				kWeights.resize(4);

			float fTotal = 0.0f;
			for (const VertexBoneWeight& kWeight : kWeights)
				fTotal += kWeight.weight;

			if (!std::isfinite(fTotal) || fTotal <= 0.000001f)
			{
				kWeights.clear();
				if (bHasFallback)
				{
					kWeights.push_back({kIn.fallbackBoneIndex, 1.0f});
					++uiFinalFallbackVertices;
				}
				else
				{
					++uiStillUnweighted;
					continue;
				}
			}
			else
			{
				const float fInvTotal = 1.0f / fTotal;
				for (VertexBoneWeight& kWeight : kWeights)
					kWeight.weight *= fInvTotal;
			}

			float fFinalWeightSum = 0.0f;
			for (const VertexBoneWeight& kWeight : kWeights)
			{
				fFinalWeightSum += kWeight.weight;
				kBoneAccum[kWeight.boneIndex].weights.emplace_back(
					v, kWeight.weight);
			}

			uiMinInfluences = std::min(uiMinInfluences,
				static_cast<unsigned int>(kWeights.size()));
			uiMaxInfluences = std::max(uiMaxInfluences,
				static_cast<unsigned int>(kWeights.size()));
			fMaxNormalizedWeightError = std::max(fMaxNormalizedWeightError,
				std::abs(fFinalWeightSum - 1.0f));
		}

		if (uiVertCount > 0 && uiMinInfluences == 4 && uiMaxInfluences == 0)
			uiMinInfluences = 0;

		std::cerr << "    Final FBX skin weights for mesh '" << kIn.name
			<< "': vertices=" << uiVertCount
			<< " influences/vertex=" << uiMinInfluences << ".."
			<< uiMaxInfluences
			<< " maxSumError=" << fMaxNormalizedWeightError << "."
			<< std::endl;

		if (uiFinalInvalidInfluences > 0)
		{
			std::cerr << "    FbxWriter discarded " << uiFinalInvalidInfluences
				<< " invalid skin influence(s) from mesh '" << kIn.name
				<< "'." << std::endl;
		}
		if (uiFinalFallbackVertices > 0)
		{
			std::cerr << "    FbxWriter assigned " << uiFinalFallbackVertices
				<< " vertex/vertices to final fallback bone '"
				<< kIn.boneNames[kIn.fallbackBoneIndex] << "' in mesh '"
				<< kIn.name << "'." << std::endl;
		}
		if (uiStillUnweighted > 0)
		{
			std::cerr << "    Error: " << uiStillUnweighted
				<< " vertex/vertices remain unweighted in mesh '" << kIn.name
				<< "' because no real fallback bone exists." << std::endl;
		}

		// Build aiBone array only for real bones that have weights.
		std::vector<aiBone*> kAiBones;
		kAiBones.reserve(uiBoneCount);
		for (unsigned int b = 0; b < uiBoneCount; ++b)
		{
			if (!IsValidBone(b) || kBoneAccum[b].weights.empty())
				continue;

			aiBone* pkBone = new aiBone();
			pkBone->mName = kIn.boneNames[b].c_str();
			// MeshExtractor already applies both the unit conversion and the
			// selected target-axis basis change to skin-to-bone matrices. Scaling the offset
			// translation again here would apply the unit scale twice.
			pkBone->mOffsetMatrix = (b < kIn.boneOffsetMatrices.size())
				? kIn.boneOffsetMatrices[b] : aiMatrix4x4();
			pkBone->mNumWeights =
				static_cast<unsigned int>(kBoneAccum[b].weights.size());
			pkBone->mWeights = new aiVertexWeight[pkBone->mNumWeights];
			for (unsigned int w = 0; w < pkBone->mNumWeights; ++w)
				pkBone->mWeights[w] = kBoneAccum[b].weights[w];
			kAiBones.push_back(pkBone);
		}

		if (!kAiBones.empty())
		{
			pkMesh->mNumBones = static_cast<unsigned int>(kAiBones.size());
			pkMesh->mBones = new aiBone*[kAiBones.size()];
			for (unsigned int b = 0; b < kAiBones.size(); ++b)
				pkMesh->mBones[b] = kAiBones[b];
		}
	}

	return pkMesh;
}

//--------------------------------------------------------------------------------------------------
bool FbxWriter::Write(const std::string& kOutputPath,
					  NiAVObject* pkRoot,
					  const std::vector<IntermediateMesh>& kMeshes,
					  const std::vector<IntermediateMaterial>& kMaterials,
					  const std::vector<aiAnimation*>& kAnimations,
					  std::string& kError) const
{
	if (kMeshes.empty())
	{
		kError = "No meshes to export.";
		return false;
	}

	// Ensure output directory exists
	{
		std::error_code ec;
		fs::path kDir = fs::path(kOutputPath).parent_path();
		if (!kDir.empty())
			fs::create_directories(kDir, ec);
	}

	aiScene* pkScene = new aiScene();
	pkScene->mFlags = 0;

	// Assimp's FBX exporter reads these exact aiScene metadata keys when
	// writing GlobalSettings. Without them it defaults to Y-up/Z-front,
	// causing importers to add an unwanted root-axis conversion.
	pkScene->mMetaData = new aiMetadata();
	const AxisMetadata kExpectedAxes = GetExpectedAxisMetadata(
		m_eAxisPreset, m_eHandedness);
	pkScene->mMetaData->Add("UpAxis", kExpectedAxes.upAxis);
	pkScene->mMetaData->Add("UpAxisSign", kExpectedAxes.upAxisSign);
	pkScene->mMetaData->Add("FrontAxis", kExpectedAxes.frontAxis);
	pkScene->mMetaData->Add("FrontAxisSign", kExpectedAxes.frontAxisSign);
	pkScene->mMetaData->Add("CoordAxis", kExpectedAxes.coordAxis);
	pkScene->mMetaData->Add("CoordAxisSign", kExpectedAxes.coordAxisSign);
	pkScene->mMetaData->Add("OriginalUpAxis", int32_t(2));
	pkScene->mMetaData->Add("OriginalUpAxisSign", int32_t(1));
	pkScene->mMetaData->Add("UnitScaleFactor", 1.0);
	pkScene->mMetaData->Add("OriginalUnitScaleFactor", 1.0);

	// ---- Materials ----
	// Textures embedded by BuildAiMaterial (referenced from materials via "*N")
	// are collected here and attached to pkScene->mTextures below. This is
	// required so Assimp's FBX exporter can correlate texture references
	// against aiScene::mTextures internally; otherwise it dereferences an
	// end() iterator into an empty lookup map and crashes (Assimp 6.0.4 bug).
	std::vector<aiTexture*> kEmbeddedTextures;

	pkScene->mNumMaterials = static_cast<unsigned int>(kMaterials.size());
	if (pkScene->mNumMaterials > 0)
	{
		pkScene->mMaterials = new aiMaterial*[pkScene->mNumMaterials];
		for (unsigned int m = 0; m < pkScene->mNumMaterials; ++m)
		{
			pkScene->mMaterials[m] =
				m_kTexExporter.BuildAiMaterial(kMaterials[m], kEmbeddedTextures);
		}
	}
	else
	{
		// Assimp requires at least one material
		pkScene->mNumMaterials = 1;
		pkScene->mMaterials = new aiMaterial*[1];
		pkScene->mMaterials[0] = new aiMaterial();
	}

	if (!kEmbeddedTextures.empty())
	{
		pkScene->mNumTextures = static_cast<unsigned int>(kEmbeddedTextures.size());
		pkScene->mTextures = new aiTexture*[pkScene->mNumTextures];
		for (unsigned int t = 0; t < pkScene->mNumTextures; ++t)
			pkScene->mTextures[t] = kEmbeddedTextures[t];
	}

	// ---- Meshes ----
	unsigned int uiMeshCount = static_cast<unsigned int>(kMeshes.size());
	pkScene->mNumMeshes = uiMeshCount;
	pkScene->mMeshes = new aiMesh*[uiMeshCount];
	for (unsigned int m = 0; m < uiMeshCount; ++m)
	{
		pkScene->mMeshes[m] = BuildAiMesh(kMeshes[m]);
		if (!pkScene->mMeshes[m])
		{
			kError = "Failed to build Assimp mesh " + std::to_string(m) + ".";
			delete pkScene;
			return false;
		}
	}

	// ---- Animations ----
	if (!kAnimations.empty())
	{
		pkScene->mNumAnimations = static_cast<unsigned int>(kAnimations.size());
		pkScene->mAnimations = new aiAnimation*[pkScene->mNumAnimations];
		for (unsigned int a = 0; a < pkScene->mNumAnimations; ++a)
			pkScene->mAnimations[a] = kAnimations[a];
	}

	// ---- Root node ----
	// Preserve the original NIF hierarchy and attach each Assimp mesh to the
	// exact NiAVObject from which it was extracted. Flattening all meshes onto
	// an identity root loses static local transforms and breaks the mesh-space
	// assumptions used by aiBone::mOffsetMatrix.
	aiNode* pkRootNode = new aiNode();
	pkRootNode->mName = "NIFToolset_Root";
	pkRootNode->mTransformation = aiMatrix4x4();

	MeshNodeAssignmentMap kMeshAssignments;
	std::vector<unsigned int> kUnassignedMeshes;
	for (unsigned int m = 0; m < uiMeshCount; ++m)
	{
		if (kMeshes[m].sourceObject)
			kMeshAssignments[kMeshes[m].sourceObject].push_back(m);
		else
			kUnassignedMeshes.push_back(m);
	}

	// If a NIF hierarchy is available, attach it as a child for transforms,
	// skeleton lookup, and animation channel lookup.
	if (pkRoot)
	{
		MeshExtractor kDummy("", false, m_fUnitScale,
			m_eAxisPreset);
		aiNode* pkHierarchy = kDummy.BuildNodeHierarchy(pkRoot, kMeshAssignments);
		if (pkHierarchy)
		{
			pkHierarchy->mParent = pkRootNode;
			pkRootNode->mNumChildren = 1;
			pkRootNode->mChildren = new aiNode*[1];
			pkRootNode->mChildren[0] = pkHierarchy;
		}
	}

	// This path should only be used for malformed or externally-created
	// intermediate meshes. Keeping it prevents Assimp from silently dropping
	// a mesh when no source node was recorded.
	if (!kUnassignedMeshes.empty())
	{
		pkRootNode->mNumMeshes = static_cast<unsigned int>(kUnassignedMeshes.size());
		pkRootNode->mMeshes = new unsigned int[pkRootNode->mNumMeshes];
		for (unsigned int i = 0; i < pkRootNode->mNumMeshes; ++i)
			pkRootNode->mMeshes[i] = kUnassignedMeshes[i];

		if (pkRoot)
		{
			std::cerr << "Warning: " << kUnassignedMeshes.size()
				<< " mesh(es) had no source NIF node and were attached to the export root."
				<< std::endl;
		}
	}

	pkScene->mRootNode = pkRootNode;


	// ---- Export via Assimp ----
	// Intermediate data and axis presets are right-handed. Left-handed output is
	// applied here as an axis-aware scene reflection so +Z-up Unreal exports do
	// not accidentally mirror their up axis. The winding order must be reversed
	// after any reflection. UV V coordinates retain the legacy exporter behavior
	// in both handedness modes.
	// The intermediate scene is already triangulated, skinned, named, and
	// animation-baked. The realtime preset is intended for imported runtime
	// assets and includes destructive cleanup steps (notably
	// aiProcess_FindInvalidData) that can collapse or remove baked animation
	// tracks. Keep export post-processing deliberately minimal.
	unsigned int uiPostProcessFlags =
		aiProcess_FlipUVs |
		aiProcess_ValidateDataStructure;

	if (m_eHandedness == ExportHandedness::Left)
	{
		ReflectSceneToLeftHanded(pkScene, m_eAxisPreset);
		uiPostProcessFlags |= aiProcess_FlipWindingOrder;
	}

	std::string kSourceScaleError;
	if (!ValidateSourceScalarScaleTracks(pkScene, kSourceScaleError))
	{
		kError = "Refusing to export invalid scalar-scale animation: " +
			kSourceScaleError;
		std::cerr << "Error: " << kError << std::endl;
		delete pkScene;
		return false;
	}

	std::string kFiniteError;
	if (!ValidateFiniteScene(pkScene, kFiniteError))
	{
		kError = "Refusing to export invalid FBX scene: " + kFiniteError;
		std::cerr << "Error: " << kError << std::endl;
		delete pkScene;
		return false;
	}


	const bool bStrictAxisMetadata = IsAssimpVersionAtLeast(6, 0, 4);
	if (!bStrictAxisMetadata)
	{
		std::cerr << "Warning: Assimp " << aiGetVersionMajor() << "."
			<< aiGetVersionMinor() << "." << aiGetVersionRevision()
			<< " predates the strict FBX 7.5/axis-metadata validation path. "
				"Export will continue and the written FBX will still be checked "
				"for finite geometry, readable animation data, and safe skeleton scales."
			<< std::endl;
		if (m_eAxisPreset != ExportAxisPreset::Unity ||
			m_eHandedness != ExportHandedness::Left)
		{
			std::cerr << "Warning: for Unity with Assimp older than 6.0.4, "
				"use -unity_axes -left-handed. Older FBX exporters may ignore "
				"custom axis metadata." << std::endl;
		}
	}

	Assimp::Exporter kExporter;
	const aiExportFormatDesc* pkFbxFormat =
		FindExporterFormat(kExporter, "fbx");
	if (!pkFbxFormat)
	{
		kError = "The linked Assimp build does not provide the binary FBX exporter.";
		delete pkScene;
		return false;
	}
	std::cout << "    FBX animation safeguards: complete sampled local TRS, "
		"exact scalar-scale isolation, missing-sample repair, and "
		"post-write verification enabled."
		<< std::endl;
	std::cout << "    Assimp runtime " << aiGetVersionMajor() << "."
		<< aiGetVersionMinor() << "." << aiGetVersionRevision()
		<< ": exporter='" << pkFbxFormat->id << "' description='"
		<< (pkFbxFormat->description ? pkFbxFormat->description : "<unknown>")
		<< "' extension='"
		<< (pkFbxFormat->fileExtension ? pkFbxFormat->fileExtension : "fbx")
		<< "'." << std::endl;
	const unsigned int uiExpectedAnimationCount = pkScene->mNumAnimations;
	aiReturn eResult = kExporter.Export(pkScene, "fbx",
		kOutputPath.c_str(), uiPostProcessFlags);

	// Assimp::Exporter::Export does not own the input scene.
	delete pkScene;

	if (eResult != aiReturn_SUCCESS)
	{
		kError = std::string("Assimp export failed: ") + kExporter.GetErrorString();
		return false;
	}

	if (!VerifyWrittenFbx(kOutputPath, kExpectedAxes,
		uiExpectedAnimationCount, bStrictAxisMetadata, kError))
	{
		std::cerr << "Error: " << kError << std::endl;
		std::error_code kRemoveError;
		fs::remove(kOutputPath, kRemoveError);
		if (kRemoveError)
		{
			std::cerr << "Warning: could not remove rejected FBX file: "
				<< kRemoveError.message() << std::endl;
		}
		return false;
	}

	return true;
}
