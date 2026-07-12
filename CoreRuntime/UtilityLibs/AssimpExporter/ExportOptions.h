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
    std::vector<std::string> inputPaths; // positional: .nif / .kfm / .kf files
};
