#include "FbxWriter.h"
#include "AxisConversion.h"

#include <assimp/scene.h>
#include <assimp/Exporter.hpp>
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
}

//--------------------------------------------------------------------------------------------------
FbxWriter::FbxWriter(const TextureExporter& kTexExporter, float fUnitScale,
	bool bConvertToUnrealAxes)
	: m_kTexExporter(kTexExporter)
	, m_fUnitScale(std::isfinite(fUnitScale) && fUnitScale > 0.0f
		? fUnitScale : 1.0f)
	, m_bConvertToUnrealAxes(bConvertToUnrealAxes)
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
		pkMesh->mVertices[i] = AxisConversion::ToUnrealVector(
			kScaledPosition, m_bConvertToUnrealAxes);
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
		pkMesh->mNormals[i] = AxisConversion::ToUnrealVector(
			kNormals[i], m_bConvertToUnrealAxes);
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

			for (const VertexBoneWeight& kWeight : kWeights)
			{
				kBoneAccum[kWeight.boneIndex].weights.emplace_back(
					v, kWeight.weight);
			}
		}

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
			// Unreal basis change to skin-to-bone matrices. Scaling the offset
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
	// causing Blender/Unreal to add an unwanted X=90 degree conversion.
	pkScene->mMetaData = new aiMetadata();
	if (m_bConvertToUnrealAxes)
	{
		// Blender's FBX axis table for Up=Z, Forward=X:
		// Up (2,+1), Front (0,-1), Coord (1,-1).
		pkScene->mMetaData->Add("UpAxis", int32_t(2));
		pkScene->mMetaData->Add("UpAxisSign", int32_t(1));
		pkScene->mMetaData->Add("FrontAxis", int32_t(0));
		pkScene->mMetaData->Add("FrontAxisSign", int32_t(-1));
		pkScene->mMetaData->Add("CoordAxis", int32_t(1));
		pkScene->mMetaData->Add("CoordAxisSign", int32_t(-1));
	}
	else
	{
		// Native NIF convention used by these assets: Up=Z, Forward=Y.
		pkScene->mMetaData->Add("UpAxis", int32_t(2));
		pkScene->mMetaData->Add("UpAxisSign", int32_t(1));
		pkScene->mMetaData->Add("FrontAxis", int32_t(1));
		pkScene->mMetaData->Add("FrontAxisSign", int32_t(-1));
		pkScene->mMetaData->Add("CoordAxis", int32_t(0));
		pkScene->mMetaData->Add("CoordAxisSign", int32_t(1));
	}
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
			m_bConvertToUnrealAxes);
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
	// Every intermediate mesh now carries a complete normal stream. The Assimp
	// real-time preset can still generate tangents and validate the scene, while
	// its non-forced smooth-normal step preserves the supplied normals.
	// ConvertToLeftHanded handles final handedness, winding and V conversion.
	const unsigned int uiPostProcessFlags =
		aiProcessPreset_TargetRealtime_Quality |
		aiProcess_ConvertToLeftHanded |
		aiProcess_ValidateDataStructure;

	Assimp::Exporter kExporter;
	aiReturn eResult = kExporter.Export(pkScene, "fbx",
		kOutputPath.c_str(), uiPostProcessFlags);

	// Assimp takes ownership of the scene? No — we must delete it.
	// Actually Assimp::Exporter::Export does NOT delete the scene.
	delete pkScene;

	if (eResult != aiReturn_SUCCESS)
	{
		kError = std::string("Assimp export failed: ") + kExporter.GetErrorString();
		return false;
	}

	return true;
}
