#pragma once

#include "MeshExtractor.h"

#include <string>
#include <vector>

// Reads the first Grand Fantasia .fsm heightmap and its terrain splat layers,
// bakes separate diffuse textures + alpha maps into one final PNG, and exports the
// ground as one complete mesh. Later heightmap layers such as water are
// intentionally ignored.
class TerrainExporter
{
public:
    explicit TerrainExporter(
        const std::vector<std::string>& kDiffuseTextureSearchFolders = {},
        const std::vector<std::string>& kAlphaTextureSearchFolders = {});

    bool Build(const std::string& kFsmPath,
        const std::string& kGeneratedTexturePngPath,
        std::vector<IntermediateMesh>& kMeshes,
        std::vector<IntermediateMaterial>& kMaterials,
        std::string& kError) const;

private:
    std::vector<std::string> m_kDiffuseTextureSearchFolders;
    std::vector<std::string> m_kAlphaTextureSearchFolders;
};
