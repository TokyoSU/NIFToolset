#pragma once

#include <NiMesh.h>
#include <NiGeometry.h>
#include <NiNode.h>
#include <NiSkinningMeshModifier.h>
#include <NiSkinInstance.h>
#include <NiTexturingProperty.h>
#include <NiSourceTexture.h>
#include <NiAlphaProperty.h>
#include <NiMaterialProperty.h>
#include <NiTransform.h>

#include <assimp/scene.h>

#include <string>
#include <unordered_map>
#include <vector>

// Maps an NiAVObject to its aiNode index so animation channels can reference it
using NodeIndexMap = std::unordered_map<NiAVObject*, unsigned int>;

// Maps each source NIF object to the Assimp mesh indices that must be attached
// to its corresponding aiNode. Keeping the mesh on its original node is
// required for both static local transforms and skeletal bind-space math.
using MeshNodeAssignmentMap =
	std::unordered_map<NiAVObject*, std::vector<unsigned int>>;

// Local bind-pose overrides derived from NiSkinInstance/NiSkinningMeshModifier.
// They keep the exported FBX skeleton bind pose consistent with aiBone offset matrices.
using BindPoseOverrideMap = std::unordered_map<NiAVObject*, NiTransform>;

// An intermediate description of a bone influence (bone index + weight per vertex)
struct VertexBoneWeight
{
	unsigned int boneIndex;
	float weight;
};

// Intermediate representation for one exported mesh before writing aiScene
struct IntermediateMesh
{
	std::string name;
	NiAVObject* sourceObject = nullptr;
	std::vector<aiVector3D> positions;
	std::vector<aiVector3D> normals;
	std::vector<aiVector2D> uvs;
	std::vector<unsigned int> indices; // flat triangle list (3 per face)

	// Bone weights per vertex (up to 4 influences each)
	std::vector<std::vector<VertexBoneWeight>> boneWeights;

	// If skinned, list of bone nodes (by aiScene node name)
	std::vector<std::string> boneNames;
	std::vector<aiMatrix4x4> boneOffsetMatrices; // bindpose inverse
	// True only when the corresponding source bone pointer is valid. Invalid
	// placeholder bones must
	// never receive vertex influences because Blender/Unity discard them.
	std::vector<bool> boneValid;
	// Real skeleton bone used for rigid/unweighted fallback vertices. This is
	// selected from the source hierarchy rather than assuming bone index 0.
	unsigned int fallbackBoneIndex = ~0u;

	// Material index into the scene material list
	unsigned int materialIndex = 0;
	bool isSkinned = false;
};

// Tracks the aiMaterial objects we build; key = source texture path (or "" for default)
struct IntermediateMaterial
{
	std::string diffuseTexturePath; // empty = no texture
	std::string name;
	bool useTextureAlpha = false;   // Preserve the diffuse texture alpha channel
	bool alphaBlend = false;        // NIF alpha blending was enabled
	bool alphaTest = false;         // NIF alpha testing/cutout was enabled
	float opacity = 1.0f;           // NiMaterialProperty scalar alpha
	float alphaCutoff = 0.5f;       // NiAlphaProperty test reference (0..1)
};

class MeshExtractor
{
public:
	MeshExtractor(const std::string& kTextureOutputFolder,
		bool bConvertTexturesToPng = true,
		float fTransformUnitScale = 1.0f,
		bool bConvertToUnrealAxes = true);

	// Recursively traverse the NIF scene graph and extract all geometry.
	// Fills kMeshes and kMaterials; builds kNodeIndexMap for later animation use.
	void Extract(NiAVObject* pkRoot,
		std::vector<IntermediateMesh>& kMeshes,
		std::vector<IntermediateMaterial>& kMaterials,
		NodeIndexMap& kNodeIndexMap) const;

	// Build the aiScene node hierarchy from the NIF scene graph.
	// Mesh indices are attached to the exact source NiAVObject node rather than
	// being flattened onto an identity root.
	aiNode* BuildNodeHierarchy(NiAVObject* pkRoot,
		const MeshNodeAssignmentMap& kMeshNodeAssignments) const;

private:
	void TraverseNode(NiAVObject* pkObject,
		std::vector<IntermediateMesh>& kMeshes,
		std::vector<IntermediateMaterial>& kMaterials,
		NodeIndexMap& kNodeIndexMap) const;

	bool ExtractNiMesh(NiMesh* pkMesh,
		std::vector<IntermediateMesh>& kMeshes,
		std::vector<IntermediateMaterial>& kMaterials) const;

	bool ExtractNiGeometry(NiGeometry* pkGeom,
		std::vector<IntermediateMesh>& kMeshes,
		std::vector<IntermediateMaterial>& kMaterials) const;

	unsigned int FindOrAddMaterial(const std::string& kTexturePath,
		const std::string& kMeshName,
		const NiAlphaProperty* pkAlphaProperty,
		const NiMaterialProperty* pkMaterialProperty,
		std::vector<IntermediateMaterial>& kMaterials) const;

	// Extract bone data from NiMesh skinning modifier
	void InitializeSkinningFromNiMesh(NiSkinningMeshModifier* pkModifier,
		IntermediateMesh& kOut) const;

	void AppendSkinningFromNiMeshSubmesh(NiMesh* pkMesh,
		NiSkinningMeshModifier* pkModifier, unsigned int uiSubmesh,
		unsigned int uiVertexCount, IntermediateMesh& kOut) const;

	// Extract bone data from legacy NiSkinInstance
	void ExtractSkinningFromNiGeometry(NiGeometry* pkGeom,
		IntermediateMesh& kOut) const;

	// Fallback: some NIFs free NiSkinData::BoneData per-vertex weight lists
	// once a NiSkinPartition exists; read weights directly from the
	// partition (bone palette + vertex map) in that case.
	void ExtractSkinningFromPartition(NiSkinInstance* pkSkin,
		NiSkinData* pkSkinData, IntermediateMesh& kOut,
		unsigned int uiVertCount) const;

	void BuildSkinBindPoseOverrides(NiAVObject* pkRoot,
		BindPoseOverrideMap& kOut) const;
	void CollectLegacySkinBindPose(NiGeometry* pkGeom,
		BindPoseOverrideMap& kOut) const;
	void CollectModernSkinBindPose(NiMesh* pkMesh,
		BindPoseOverrideMap& kOut) const;

	aiMatrix4x4 MakeAiMatrix(const NiTransform& kTransform) const;
	aiNode* BuildNodeRecursive(NiAVObject* pkObject,
		const MeshNodeAssignmentMap& kMeshNodeAssignments,
		const BindPoseOverrideMap& kBindPoseOverrides) const;

	std::string ResolveTexturePath(NiTexturingProperty* pkTexProp) const;

	std::string m_kTextureOutputFolder;
	bool m_bConvertTexturesToPng;
	float m_fTransformUnitScale;
	bool m_bConvertToUnrealAxes;
};
