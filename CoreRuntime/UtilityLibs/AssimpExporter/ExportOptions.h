#pragma once

#include <string>
#include <vector>

struct ExportOptions
{
    std::string animFolder;     // -anim_folder    : search folder for KF files
    std::string nifFolder;      // -nif_folder     : search folder for NIF files
    std::string textureFolder;  // -texture_folder : search/output folder for textures
    std::string kfmFolder;      // -kfm_folder     : search folder for KFM files
    std::string outputFolder;   // -output         : destination for FBX + PNG output
    bool exportAll = false;     // -all            : discover all .nif/.kfm/.kf under nifFolder/kfmFolder
    float unitScale = 100.0f;   // -scale          : NIF units -> FBX centimeters (UE5 default: 100)
    float sampleRate = 30.0f;   // -sample_rate    : baked animation samples per second
    bool flipUvV = true;        // D3D/Gamebryo top-left UV origin -> FBX/Blender bottom-left
    bool smoothNormals = true;  // Rebuild smooth normals from exported triangles
    bool unrealAxes = true;     // Convert +Y-forward/Z-up NIF data to +X-forward/Z-up Unreal axes
    float smoothNormalAngle = 80.0f; // Maximum smoothing angle in degrees
    std::vector<std::string> inputPaths; // positional: .nif / .kfm / .kf files
};
