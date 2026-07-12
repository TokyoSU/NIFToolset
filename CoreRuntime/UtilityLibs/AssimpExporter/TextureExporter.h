#pragma once

#include "MeshExtractor.h"

#include <assimp/scene.h>

#include <string>
#include <vector>

// TextureExporter handles:
//  1. Locating the source texture file from the NIF reference path
//  2. Converting it to PNG in the output folder (using WIC / Windows Imaging Component)
//  3. Returning the final output PNG path for use in aiMaterial
class TextureExporter
{
public:
	TextureExporter(const std::string& kTextureSearchFolder,
		const std::string& kTextureOutputFolder);

	// Given the raw texture path from NiSourceTexture, resolve the source file,
	// convert (if needed) to PNG, write to output folder, and return the output path.
	// Returns empty string on failure.
	std::string ExportTexture(const std::string& kSourcePath) const;

	// Export an FBX TransparentColor map derived from the source texture alpha.
	// FBX transparency uses black=opaque and white=transparent, so this writes
	// RGB = 1-alpha rather than reusing the diffuse color image directly.
	std::string ExportTransparencyTexture(const std::string& kSourcePath) const;

	// Build an aiMaterial from an IntermediateMaterial.
	// Calls ExportTexture internally to resolve the diffuse PNG path.
	// Any embedded aiTexture created for the diffuse map (referenced from the
	// material via "*N") is appended to kEmbeddedTextures so the caller can
	// attach it to aiScene::mTextures. Assimp's FBX exporter correlates
	// material texture references against aiScene::mTextures internally; if
	// that array is left empty, it dereferences an end() iterator and crashes
	// (Assimp 6.0.4 bug), so embedding is required rather than referencing an
	// external file path.
	aiMaterial* BuildAiMaterial(const IntermediateMaterial& kMat,
		std::vector<aiTexture*>& kEmbeddedTextures) const;

private:
	std::string FindSourceFile(const std::string& kRawPath) const;
	bool ConvertToPng(const std::string& kSrcPath, const std::string& kDstPath) const;
	bool ConvertAlphaToTransparencyPng(const std::string& kSrcPath,
		const std::string& kDstPath) const;
	bool CopyAsPng(const std::string& kSrcPath, const std::string& kDstPath) const;
	std::string GetFallbackTexturePath() const;
	aiTexture* LoadEmbeddedPngTexture(const std::string& kPngPath) const;

	std::string m_kSearchFolder;
	std::string m_kOutputFolder;
};
