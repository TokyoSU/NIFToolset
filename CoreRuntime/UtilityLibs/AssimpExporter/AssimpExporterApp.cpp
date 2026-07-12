#include "AssimpExporterApp.h"

#include "AssetLoader.h"
#include "MeshExtractor.h"
#include "TextureExporter.h"
#include "AnimationExporter.h"
#include "FbxWriter.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <cmath>

namespace fs = std::filesystem;

static std::string BuildOutputFbxPath(const ResolvedInputAsset& kAsset,
    const ExportOptions& kOptions)
{
    fs::path kStem = fs::path(kAsset.modelNifPath).stem();
    std::string kFolder = kOptions.outputFolder.empty()
        ? fs::path(kAsset.modelNifPath).parent_path().string()
        : kOptions.outputFolder;
    std::error_code ec;
    fs::create_directories(kFolder, ec);
    return (fs::path(kFolder) / (kStem.string() + ".fbx")).string();
}

int AssimpExporterApp::Run(int argc, char** argv)
{
    ExportOptions kOptions;
    std::string kError;
    if (!ParseCommandLine(argc, argv, kOptions, kError))
    {
        if (!kError.empty())
            std::cerr << kError << std::endl;
        PrintUsage();
        return 1;
    }

    RuntimeScope kRuntimeScope;
    AssetLoader kAssetLoader(kOptions);

    std::vector<ResolvedInputAsset> kResolvedAssets;
    if (!kAssetLoader.ResolveInputs(kResolvedAssets, kError))
    {
        std::cerr << kError << std::endl;
        return 1;
    }

    int iFailures = 0;

    for (const ResolvedInputAsset& kAsset : kResolvedAssets)
    {
        std::cout << "Exporting: " << kAsset.modelNifPath << std::endl;
        std::cout << "  Unit scale: " << kOptions.unitScale
            << " (NIF unit -> FBX cm), animation sample rate: "
            << kOptions.sampleRate << " fps, UV V flip: "
            << (kOptions.flipUvV ? "enabled" : "disabled")
            << ", smooth normals: "
            << (kOptions.smoothNormals ? "enabled" : "disabled")
            << ", axes: "
            << (kOptions.unrealAxes
                ? "Unreal (+X forward, +Z up)"
                : "native NIF (+Y forward, +Z up)");
        if (kOptions.smoothNormals)
            std::cout << " (angle " << kOptions.smoothNormalAngle << " degrees)";
        std::cout << std::endl;

        // Load NIF
        LoadedNifAsset kNifAsset;
        if (!kAssetLoader.LoadNifAsset(kAsset.modelNifPath, kNifAsset, kError))
        {
            std::cerr << "  " << kError << std::endl;
            ++iFailures;
            continue;
        }

        NiAVObject* pkRoot = kNifAsset.root;
        if (!pkRoot)
        {
            std::cerr << "  NIF has no root object." << std::endl;
            ++iFailures;
            continue;
        }

        // Extract meshes and materials
        const std::string kNifDir = fs::path(kAsset.modelNifPath).parent_path().string();
        const std::string kTexSearchFolder = kOptions.textureFolder.empty() ? kNifDir : kOptions.textureFolder;
        const std::string kTexOutputFolder = !kOptions.outputFolder.empty()  ? kOptions.outputFolder
                                       : !kOptions.textureFolder.empty() ? kOptions.textureFolder
                                       : kNifDir;

        MeshExtractor kMeshEx(kTexOutputFolder, true,
            kOptions.unitScale, kOptions.flipUvV,
            kOptions.smoothNormals, kOptions.smoothNormalAngle,
            kOptions.unrealAxes);
        TextureExporter kTexEx(kTexSearchFolder, kTexOutputFolder);

        std::vector<IntermediateMesh> kMeshes;
        std::vector<IntermediateMaterial> kMaterials;
        NodeIndexMap kNodeMap;
        kMeshEx.Extract(pkRoot, kMeshes, kMaterials, kNodeMap);

        if (kMeshes.empty())
        {
            std::cout << "  Warning: no meshes found in NIF." << std::endl;
        }
        else
        {
            size_t stVertices = 0;
            size_t stTriangles = 0;
            size_t stSkinnedMeshes = 0;
            for (const IntermediateMesh& kMesh : kMeshes)
            {
                stVertices += kMesh.positions.size();
                stTriangles += kMesh.indices.size() / 3u;
                stSkinnedMeshes += kMesh.isSkinned ? 1u : 0u;
            }

            std::cout << "  Meshes: " << kMeshes.size()
                << " (skinned: " << stSkinnedMeshes << ")"
                << ", vertices: " << stVertices
                << ", triangles: " << stTriangles << std::endl;
        }

        // Gather animations: KFM > KF > NIF fallback
        std::vector<aiAnimation*> kAnimations;

        if (!kAsset.kfmPath.empty())
        {
            NiKFMToolPtr spKfmTool;
            if (kAssetLoader.LoadKfmTool(kAsset.kfmPath, spKfmTool, kError))
            {
                // Gather sequence files from KFM
                for (const std::string& kKfPath : kAsset.kfPaths)
                {
                    std::vector<NiSequenceDataPtr> kSeqDatas;
                    if (kAssetLoader.LoadKfSequences(kKfPath, kSeqDatas, kError))
                    {
                        auto kNew = AnimationExporter::BuildFromSequenceDatas(
                            kSeqDatas, pkRoot, kOptions.unitScale,
                            kOptions.sampleRate, kOptions.unrealAxes);
                        kAnimations.insert(kAnimations.end(), kNew.begin(), kNew.end());
                    }
                    else
                    {
                        std::cerr << "  Warning: failed to load KF sequences from "
                            << kKfPath << ": " << kError << std::endl;
                    }
                }
            }
            else
            {
                std::cerr << "  Warning: failed to load KFM: " << kError << std::endl;
            }
        }
        else if (!kAsset.kfPaths.empty())
        {
            // Matching KF files (no KFM)
            for (const std::string& kKfPath : kAsset.kfPaths)
            {
                std::vector<NiSequenceDataPtr> kSeqDatas;
                if (kAssetLoader.LoadKfSequences(kKfPath, kSeqDatas, kError))
                {
                    auto kNew = AnimationExporter::BuildFromSequenceDatas(
                        kSeqDatas, pkRoot, kOptions.unitScale,
                        kOptions.sampleRate, kOptions.unrealAxes);
                    kAnimations.insert(kAnimations.end(), kNew.begin(), kNew.end());
                }
            }
        }

        // NIF fallback if still no animations
        if (kAnimations.empty())
        {
            kAnimations = AnimationExporter::BuildFromNifControllers(
                pkRoot, kOptions.unitScale, kOptions.sampleRate,
                kOptions.unrealAxes);
        }

        if (!kAnimations.empty())
            std::cout << "  Animations: " << kAnimations.size() << std::endl;

        // Write FBX
        std::string kOutputPath = BuildOutputFbxPath(kAsset, kOptions);
        FbxWriter kWriter(kTexEx, kOptions.unitScale,
            kOptions.unrealAxes);
        if (!kWriter.Write(kOutputPath, pkRoot, kMeshes, kMaterials, kAnimations, kError))
        {
            std::cerr << "  Export failed: " << kError << std::endl;
            ++iFailures;
        }
        else
        {
            std::cout << "  Written: " << kOutputPath << std::endl;
        }
    }

    return iFailures > 0 ? 1 : 0;
}

bool AssimpExporterApp::ParseCommandLine(int argc, char** argv,
    ExportOptions& kOptions, std::string& kError) const
{
    // Helper: check that a folder path exists
    auto ValidateFolder = [&](const std::string& kFlag,
        const std::string& kPath) -> bool
    {
        if (kPath.empty())
            return true;
        std::error_code ec;
        if (!fs::exists(kPath, ec) || !fs::is_directory(kPath, ec))
        {
            kError = "Folder for " + kFlag + " does not exist: \"" + kPath + "\"";
            return false;
        }
        return true;
    };

    // Helper: check that an input file exists and has a recognised extension
    auto ValidateInputFile = [&](const std::string& kPath) -> bool
    {
        std::error_code ec;
        if (!fs::exists(kPath, ec))
        {
            kError = "Input file not found: \"" + kPath + "\"";
            return false;
        }
        std::string kExt = fs::path(kPath).extension().string();
        std::transform(kExt.begin(), kExt.end(), kExt.begin(), ::tolower);
        if (kExt != ".nif" && kExt != ".kfm" && kExt != ".kf")
        {
            kError = "Unsupported input file type (expected .nif, .kfm or .kf): \""
                + kPath + "\"";
            return false;
        }
        return true;
    };

    for (int iArg = 1; iArg < argc; ++iArg)
    {
        const std::string kArg = argv[iArg] ? argv[iArg] : "";

        if (kArg == "-scale" || kArg == "-sample_rate" || kArg == "-smooth_angle")
        {
            if (iArg + 1 >= argc)
            {
                kError = "Missing value for argument: " + kArg;
                return false;
            }

            const std::string kValue = argv[++iArg] ? argv[iArg] : "";
            try
            {
                size_t stParsed = 0;
                const float fValue = std::stof(kValue, &stParsed);
                if (stParsed != kValue.size() || !std::isfinite(fValue))
                    throw std::invalid_argument("not a finite number");

                if (kArg == "-smooth_angle")
                {
                    if (fValue < 0.0f || fValue > 180.0f)
                        throw std::invalid_argument("angle outside 0..180");
                    kOptions.smoothNormalAngle = fValue;
                }
                else
                {
                    if (fValue <= 0.0f)
                        throw std::invalid_argument("not a positive number");
                    if (kArg == "-scale")
                        kOptions.unitScale = fValue;
                    else
                        kOptions.sampleRate = fValue;
                }
            }
            catch (const std::exception&)
            {
                kError = "Invalid numeric value for " + kArg + ": \"" + kValue + "\"";
                return false;
            }
            continue;
        }

        if (kArg == "-anim_folder"    ||
            kArg == "-nif_folder"     ||
            kArg == "-texture_folder" ||
            kArg == "-kfm_folder"     ||
            kArg == "-output")
        {
            if (iArg + 1 >= argc)
            {
                kError = "Missing value for argument: " + kArg;
                return false;
            }
            const std::string kValue = argv[++iArg] ? argv[iArg] : "";
            if      (kArg == "-anim_folder")    kOptions.animFolder    = kValue;
            else if (kArg == "-nif_folder")     kOptions.nifFolder     = kValue;
            else if (kArg == "-texture_folder") kOptions.textureFolder = kValue;
            else if (kArg == "-kfm_folder")     kOptions.kfmFolder     = kValue;
            else if (kArg == "-output")         kOptions.outputFolder  = kValue;
            continue;
        }

        if (kArg == "-all")
        {
            kOptions.exportAll = true;
            continue;
        }

        if (kArg == "-flip_uv_v")
        {
            kOptions.flipUvV = true;
            continue;
        }

        if (kArg == "-no_flip_uv_v")
        {
            kOptions.flipUvV = false;
            continue;
        }

        if (kArg == "-smooth_normals")
        {
            kOptions.smoothNormals = true;
            continue;
        }

        if (kArg == "-no_smooth_normals")
        {
            kOptions.smoothNormals = false;
            continue;
        }

        if (kArg == "-unreal_axes")
        {
            kOptions.unrealAxes = true;
            continue;
        }

        if (kArg == "-no_unreal_axes")
        {
            kOptions.unrealAxes = false;
            continue;
        }

        // Positional input file
        kOptions.inputPaths.push_back(kArg);
    }

    // Validate folder arguments
    if (!ValidateFolder("-nif_folder",     kOptions.nifFolder))     return false;
    if (!ValidateFolder("-anim_folder",    kOptions.animFolder))    return false;
    if (!ValidateFolder("-texture_folder", kOptions.textureFolder)) return false;
    if (!ValidateFolder("-kfm_folder",     kOptions.kfmFolder))     return false;
    // -output is not existence-checked; we create it later

    if (kOptions.exportAll)
    {
        // With -all: scan kfmFolder for .kfm first (priority), then nifFolder for .nif
        // if no kfmFolder is given, fall back to nifFolder only.
        const std::string& kScanKfm = kOptions.kfmFolder;
        const std::string& kScanNif = kOptions.nifFolder;

        if (kScanKfm.empty() && kScanNif.empty())
        {
            kError = "-all requires at least -kfm_folder or -nif_folder to know where to scan.";
            return false;
        }

        std::error_code ec;

        // Collect .kfm files from kfmFolder
        if (!kScanKfm.empty())
        {
            for (const auto& kEntry : fs::recursive_directory_iterator(kScanKfm, ec))
            {
                if (ec) break;
                if (!kEntry.is_regular_file(ec)) continue;
                std::string kExt = kEntry.path().extension().string();
                std::transform(kExt.begin(), kExt.end(), kExt.begin(), ::tolower);
                if (kExt == ".kfm")
                    kOptions.inputPaths.push_back(kEntry.path().string());
            }
        }

        // If no KFM found (or no kfmFolder), collect .nif from nifFolder
        if (kOptions.inputPaths.empty() && !kScanNif.empty())
        {
            for (const auto& kEntry : fs::recursive_directory_iterator(kScanNif, ec))
            {
                if (ec) break;
                if (!kEntry.is_regular_file(ec)) continue;
                std::string kExt = kEntry.path().extension().string();
                std::transform(kExt.begin(), kExt.end(), kExt.begin(), ::tolower);
                if (kExt == ".nif")
                    kOptions.inputPaths.push_back(kEntry.path().string());
            }
        }

        if (kOptions.inputPaths.empty())
        {
            kError = "-all was specified but no .kfm or .nif files were found in the scan folders.";
            return false;
        }

        std::cout << "[all] Found " << kOptions.inputPaths.size()
            << " file(s) to export." << std::endl;

        return true; // skip per-file validation; AssetLoader handles missing files gracefully
    }

    // Without -all: must have at least one explicit input file
    if (kOptions.inputPaths.empty())
    {
        kError = "No input files provided. Pass file paths or use -all to scan a folder.";
        return false;
    }

    // Validate every explicitly provided input file
    for (const std::string& kPath : kOptions.inputPaths)
    {
        if (!ValidateInputFile(kPath))
            return false;
    }

    return true;
}

void AssimpExporterApp::PrintUsage() const
{
    std::cout
        << "Usage:\n"
        << "  AssimpExporter [options] <file1.nif|.kfm|.kf> [file2 ...]\n"
        << "  AssimpExporter [options] -all\n"
        << "\n"
        << "  When -all is used, all .kfm files under -kfm_folder are exported\n"
        << "  automatically. If no .kfm files are found, all .nif files under\n"
        << "  -nif_folder are used instead. No positional file arguments are needed.\n"
        << "\nOptions:\n"
        << "  -nif_folder      <dir>  Search folder for NIF files\n"
        << "  -anim_folder     <dir>  Search folder for KF animation files\n"
        << "  -texture_folder  <dir>  Search/output folder for textures (PNG)\n"
        << "  -kfm_folder      <dir>  Search folder for KFM files\n"
        << "  -output          <dir>  Destination folder for exported FBX and PNG files\n"
        << "  -scale           <num>  Coordinate scale; default 100 for UE centimeters\n"
        << "  -sample_rate     <fps>  Bake animation curves at this rate; default 30\n"
        << "  -flip_uv_v              Flip V for D3D/Gamebryo -> FBX (default)\n"
        << "  -no_flip_uv_v           Preserve source V coordinates\n"
        << "  -smooth_normals         Rebuild smooth normals (default)\n"
        << "  -no_smooth_normals      Preserve normals stored in the NIF\n"
        << "  -smooth_angle    <deg>  Smoothing threshold from 0 to 180; default 80\n"
        << "  -unreal_axes            Export +X forward, +Z up (default)\n"
        << "  -no_unreal_axes         Preserve native +Y forward, +Z up axes\n"
        << "  -all                    Auto-discover all assets in the scan folders\n"
        << "\nSingle file example:\n"
        << "  AssimpExporter"
        << " -kfm_folder \"C:\\Game\\monster\""
        << " -output \"C:\\Export\\monster\""
        << " \"C:\\Game\\monster\\M001.kfm\"\n"
        << "\nBatch all example:\n"
        << "  AssimpExporter"
        << " -kfm_folder \"C:\\Game\\monster\""
        << " -anim_folder \"C:\\Game\\monster\\animation\""
        << " -texture_folder \"C:\\Game\\monster\\texture\""
        << " -output \"C:\\Export\\monster\""
        << " -all\n";
}

