#pragma once

#include "ExportAxes.h"

#include <string>
#include <vector>

struct ExportOptions
{
    std::string animFolder;     // -anim_folder    : search folder for KF files
    std::string nifFolder;      // -nif_folder     : search folder for NIF files
    std::string textureFolder;  // -texture_folder : search/output folder for textures
    std::string kfmFolder;      // -kfm_folder     : search folder for KFM files
    std::string outputFolder;   // -output         : destination for FBX + PNG output
    std::string terrainFolder;  // -terrain_folder : recursively batch-convert .fsm terrain files
    std::string terrainTextureFolder; // -terrain_texture_folder : root containing terrain diffuse/base textures
    std::string terrainAlphaTextureFolder; // -terrain_alpha_texture_folder : root containing terrain alpha maps
    bool exportAll = false;     // -all            : discover all .nif/.kfm/.kf under nifFolder/kfmFolder
    bool preserveFolders = false; // -preserve_folders: with -all, keep each FBX/textures in its own model folder
    float unitScale = 100.0f;   // -scale          : NIF units -> FBX centimeters (UE5 default: 100)
    float sampleRate = 30.0f;   // -sample_rate    : baked animation samples per second
    ExportAxisPreset axisPreset = ExportAxisPreset::Unreal; // -unreal_axes (default), -unity_axes, or -native_axes
    ExportHandedness handedness = ExportHandedness::Right; // right-handed by default; -left-handed opts in
    std::vector<std::string> inputPaths; // positional: .nif / .kfm / .kf files
    std::vector<std::string> terrainInputPaths; // positional/scanned .fsm files
};
