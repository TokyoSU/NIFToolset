#pragma once

#include "MeshExtractor.h"
#include "ExportAxes.h"
#include "TextureExporter.h"
#include "AnimationExporter.h"
#include "AssetLoader.h"

#include <string>

// FbxWriter assembles an aiScene from the intermediate mesh, material, and animation data
// and writes an FBX file using Assimp's exporter API.
class FbxWriter
{
public:
	FbxWriter(const TextureExporter& kTexExporter, float fUnitScale,
		ExportAxisPreset eAxisPreset = ExportAxisPreset::Unreal,
		ExportHandedness eHandedness = ExportHandedness::Right);

	// Write a complete FBX for one input asset.
	// kOutputPath: path of the output .fbx file (directory must exist).
	// kMeshes / kMaterials from MeshExtractor::Extract.
	// kAnimations from AnimationExporter::BuildFromSequenceDatas or BuildFromNifControllers.
	// pkRoot used to build the node hierarchy.
	bool Write(const std::string& kOutputPath,
			   NiAVObject* pkRoot,
			   const std::vector<IntermediateMesh>& kMeshes,
			   const std::vector<IntermediateMaterial>& kMaterials,
			   const std::vector<aiAnimation*>& kAnimations,
			   std::string& kError) const;

private:
	aiMesh* BuildAiMesh(const IntermediateMesh& kMesh) const;

	const TextureExporter& m_kTexExporter;
	float m_fUnitScale;
	ExportAxisPreset m_eAxisPreset;
	ExportHandedness m_eHandedness;
};
