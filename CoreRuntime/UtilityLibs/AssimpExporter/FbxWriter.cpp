#include "FbxWriter.h"

#include <assimp/scene.h>
#include <assimp/Exporter.hpp>
#include <assimp/postprocess.h>

#include <NiNode.h>
#include <NiMesh.h>
#include <NiGeometry.h>

#include <filesystem>
#include <cstring>
#include <iostream>

namespace fs = std::filesystem;

//--------------------------------------------------------------------------------------------------
FbxWriter::FbxWriter(const TextureExporter& kTexExporter)
	: m_kTexExporter(kTexExporter)
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
		pkMesh->mVertices[i] = kIn.positions[i];

	// ---- Normals ----
	if (kIn.normals.size() == uiVertCount)
	{
		pkMesh->mNormals = new aiVector3D[uiVertCount];
		for (unsigned int i = 0; i < uiVertCount; ++i)
			pkMesh->mNormals[i] = kIn.normals[i];
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
		unsigned int uiBoneCount = static_cast<unsigned int>(kIn.boneNames.size());

		// Accumulate per-bone vertex weights
		struct BoneAccum { std::vector<aiVertexWeight> weights; };
		std::vector<BoneAccum> kBoneAccum(uiBoneCount);

		for (unsigned int v = 0; v < static_cast<unsigned int>(kIn.boneWeights.size()); ++v)
		{
			for (const VertexBoneWeight& kBW : kIn.boneWeights[v])
			{
				if (kBW.boneIndex < uiBoneCount)
					kBoneAccum[kBW.boneIndex].weights.emplace_back(v, kBW.weight);
			}
		}

		// Build aiBone array (only for bones that have weights)
		std::vector<aiBone*> kAiBones;
		kAiBones.reserve(uiBoneCount);
		for (unsigned int b = 0; b < uiBoneCount; ++b)
		{
			if (kBoneAccum[b].weights.empty())
				continue;

			aiBone* pkBone = new aiBone();
			pkBone->mName = kIn.boneNames[b].c_str();
			pkBone->mOffsetMatrix = (b < kIn.boneOffsetMatrices.size())
				? kIn.boneOffsetMatrices[b] : aiMatrix4x4();
			pkBone->mNumWeights = static_cast<unsigned int>(kBoneAccum[b].weights.size());
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
		MeshExtractor kDummy("", false);
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

		std::cerr << "Warning: " << kUnassignedMeshes.size()
			<< " mesh(es) had no source NIF node and were attached to the export root."
			<< std::endl;
	}

	pkScene->mRootNode = pkRootNode;

	// ---- Export via Assimp ----
	Assimp::Exporter kExporter;
	aiReturn eResult = kExporter.Export(pkScene, "fbx", kOutputPath.c_str(),
		aiProcess_ValidateDataStructure);

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
