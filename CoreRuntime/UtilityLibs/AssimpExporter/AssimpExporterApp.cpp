#include "AssimpExporterApp.h"

#include "AssetLoader.h"
#include "MeshExtractor.h"
#include "TextureExporter.h"
#include "AnimationExporter.h"
#include "FbxWriter.h"
#include "TerrainExporter.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace fs = std::filesystem;

namespace
{
    std::string ToLower(std::string kValue)
    {
        std::transform(kValue.begin(), kValue.end(), kValue.begin(),
            [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return kValue;
    }

    std::string GetLowerExtension(const fs::path& kPath)
    {
        return ToLower(kPath.extension().string());
    }

    bool CaseInsensitiveEquals(const std::string& kLeft,
        const std::string& kRight)
    {
        return ToLower(kLeft) == ToLower(kRight);
    }

    void SortAndUniquePaths(std::vector<std::string>& kPaths)
    {
        std::sort(kPaths.begin(), kPaths.end(),
            [](const std::string& kLeft, const std::string& kRight)
            {
                return ToLower(fs::path(kLeft).lexically_normal().string()) <
                    ToLower(fs::path(kRight).lexically_normal().string());
            });

        kPaths.erase(std::unique(kPaths.begin(), kPaths.end(),
            [](const std::string& kLeft, const std::string& kRight)
            {
                return CaseInsensitiveEquals(
                    fs::path(kLeft).lexically_normal().string(),
                    fs::path(kRight).lexically_normal().string());
            }), kPaths.end());
    }

    fs::path BuildModelOutputFolder(const ResolvedInputAsset& kAsset,
        const ExportOptions& kOptions)
    {
        const fs::path kModelPath(kAsset.modelNifPath);
        fs::path kOutputRoot = kOptions.outputFolder.empty()
            ? kModelPath.parent_path()
            : fs::path(kOptions.outputFolder);
        if (kOutputRoot.empty())
            kOutputRoot = fs::path(".");

        // Keep the FBX and every texture generated/copied for this asset in a
        // dedicated folder named after the source model.
        return kOutputRoot / kModelPath.stem();
    }

    fs::path BuildOutputFbxPath(const ResolvedInputAsset& kAsset,
        const fs::path& kOutputFolder)
    {
        const fs::path kStem = fs::path(kAsset.modelNifPath).stem();
        return kOutputFolder / (kStem.string() + ".fbx");
    }

    fs::path BuildTerrainOutputFolder(const fs::path& kFsmPath,
        const ExportOptions& kOptions)
    {
        fs::path kOutputFolder;
        if (kOptions.outputFolder.empty())
        {
            kOutputFolder = kFsmPath.parent_path();
            if (kOutputFolder.empty())
                kOutputFolder = fs::path(".");
        }
        else
        {
            kOutputFolder = fs::path(kOptions.outputFolder);

            // A recursive -terrain_folder batch keeps its relative directory
            // structure before adding the final per-terrain folder.
            if (!kOptions.terrainFolder.empty())
            {
                std::error_code kError;
                fs::path kRelativeParent = fs::relative(
                    kFsmPath.parent_path(), fs::path(kOptions.terrainFolder),
                    kError);
                if (!kError && !kRelativeParent.empty() &&
                    *kRelativeParent.begin() != "..")
                {
                    kOutputFolder /= kRelativeParent;
                }
            }
        }

        // Each terrain gets its own folder containing the FBX and the baked
        // final terrain PNG.
        return kOutputFolder / kFsmPath.stem();
    }

    void AddUniqueSearchFolder(std::vector<std::string>& kFolders,
        const fs::path& kFolder)
    {
        if (kFolder.empty())
            return;

        const std::string kNormalized =
            ToLower(kFolder.lexically_normal().string());
        for (const std::string& kExisting : kFolders)
        {
            if (ToLower(fs::path(kExisting).lexically_normal().string()) ==
                kNormalized)
            {
                return;
            }
        }

        kFolders.push_back(kFolder.string());
    }

    std::vector<std::string> BuildTerrainAssetSearchFolders(
        const fs::path& kFsmPath, const ExportOptions& kOptions,
        const std::string& kPrimaryRoot,
        const std::vector<std::string>& kFallbackRoots)
    {
        std::vector<std::string> kFolders;
        AddUniqueSearchFolder(kFolders, kFsmPath.parent_path());

        auto AddRootAndRelativeFolder = [&](const std::string& kRootString)
        {
            if (kRootString.empty())
                return;

            const fs::path kRoot(kRootString);
            if (!kOptions.terrainFolder.empty())
            {
                std::error_code kRelativeError;
                const fs::path kRelativeParent = fs::relative(
                    kFsmPath.parent_path(), fs::path(kOptions.terrainFolder),
                    kRelativeError);
                if (!kRelativeError && !kRelativeParent.empty() &&
                    *kRelativeParent.begin() != "..")
                {
                    // Prefer the corresponding map subfolder before the
                    // shared root. TerrainExporter also searches recursively.
                    AddUniqueSearchFolder(kFolders,
                        kRoot / kRelativeParent);
                }
            }

            AddUniqueSearchFolder(kFolders, kRoot);
        };

        AddRootAndRelativeFolder(kPrimaryRoot);
        for (const std::string& kFallbackRoot : kFallbackRoots)
            AddRootAndRelativeFolder(kFallbackRoot);
        return kFolders;
    }

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
    if (!kOptions.inputPaths.empty() &&
        !kAssetLoader.ResolveInputs(kResolvedAssets, kError))
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
            << kOptions.sampleRate << " fps, normals: source + generated fallback"
            << ", handedness/UVs: Assimp ConvertToLeftHanded"
            << ", axes: "
            << (kOptions.unrealAxes
                ? "Unreal (+X forward, +Z up)"
                : "native NIF (+Y forward, +Z up)")
            << std::endl;

        LoadedNifAsset kNifAsset;
        if (!kAssetLoader.LoadNifAsset(kAsset.modelNifPath,
            kNifAsset, kError))
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

        const std::string kNifDir =
            fs::path(kAsset.modelNifPath).parent_path().string();
        const std::string kTexSearchFolder = kOptions.textureFolder.empty()
            ? kNifDir : kOptions.textureFolder;
        const fs::path kAssetOutputFolder =
            BuildModelOutputFolder(kAsset, kOptions);
        std::error_code kAssetDirectoryError;
        fs::create_directories(kAssetOutputFolder, kAssetDirectoryError);
        if (kAssetDirectoryError)
        {
            std::cerr << "  Failed to create asset output directory: "
                << kAssetOutputFolder.string() << " ("
                << kAssetDirectoryError.message() << ")" << std::endl;
            ++iFailures;
            continue;
        }
        const std::string kTexOutputFolder = kAssetOutputFolder.string();

        MeshExtractor kMeshEx(kTexOutputFolder, true,
            kOptions.unitScale, kOptions.unrealAxes);
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

        std::vector<aiAnimation*> kAnimations;
        std::vector<NiSequenceDataPtr> kSequenceDatas;

        bool bLoadExternalSequences = true;
        if (!kAsset.kfmPath.empty())
        {
            NiKFMToolPtr spKfmTool;
            if (!kAssetLoader.LoadKfmTool(kAsset.kfmPath,
                spKfmTool, kError))
            {
                std::cerr << "  Warning: failed to load KFM: "
                    << kError << std::endl;
                bLoadExternalSequences = false;
            }
        }

        if (bLoadExternalSequences)
        {
            for (const std::string& kKfPath : kAsset.kfPaths)
            {
                std::vector<NiSequenceDataPtr> kLoadedSequences;
                if (kAssetLoader.LoadKfSequences(
                    kKfPath, kLoadedSequences, kError))
                {
                    kSequenceDatas.insert(kSequenceDatas.end(),
                        kLoadedSequences.begin(), kLoadedSequences.end());
                }
                else
                {
                    std::cerr << "  Warning: failed to load KF sequences from "
                        << kKfPath << ": " << kError << std::endl;
                }
            }
        }

        AnimationExporter::AppendFromControllerManagers(
            pkRoot, kSequenceDatas);

        if (!kSequenceDatas.empty())
        {
            kAnimations = AnimationExporter::BuildFromSequenceDatas(
                kSequenceDatas, pkRoot, kOptions.unitScale,
                kOptions.sampleRate, kOptions.unrealAxes,
                kNifAsset.pStream);
        }

        if (kAnimations.empty())
        {
            kAnimations = AnimationExporter::BuildFromNifControllers(
                pkRoot, kOptions.unitScale, kOptions.sampleRate,
                kOptions.unrealAxes);
        }

        if (!kAnimations.empty())
            std::cout << "  Animations: " << kAnimations.size() << std::endl;

        const std::string kOutputPath =
            BuildOutputFbxPath(kAsset, kAssetOutputFolder).string();
        FbxWriter kWriter(kTexEx, kOptions.unitScale,
            kOptions.unrealAxes);
        if (!kWriter.Write(kOutputPath, pkRoot, kMeshes, kMaterials,
            kAnimations, kError))
        {
            std::cerr << "  Export failed: " << kError << std::endl;
            ++iFailures;
        }
        else
        {
            std::cout << "  Written: " << kOutputPath << std::endl;
        }
    }

    for (const std::string& kTerrainInput : kOptions.terrainInputPaths)
    {
        const fs::path kFsmPath(kTerrainInput);
        std::cout << "Exporting terrain: " << kFsmPath.string() << std::endl;

        const fs::path kOutputFolder = BuildTerrainOutputFolder(
            kFsmPath, kOptions);
        std::error_code kDirectoryError;
        fs::create_directories(kOutputFolder, kDirectoryError);
        if (kDirectoryError)
        {
            std::cerr << "  Failed to create terrain output directory: "
                << kOutputFolder.string() << " ("
                << kDirectoryError.message() << ")" << std::endl;
            ++iFailures;
            continue;
        }

        const fs::path kFinalTexturePath = kOutputFolder /
            (kFsmPath.stem().string() + "_terrain.png");
        const std::vector<std::string> kTerrainDiffuseSearchFolders =
            BuildTerrainAssetSearchFolders(kFsmPath, kOptions,
                kOptions.terrainTextureFolder,
                { kOptions.textureFolder });
        const std::vector<std::string> kTerrainAlphaSearchFolders =
            BuildTerrainAssetSearchFolders(kFsmPath, kOptions,
                kOptions.terrainAlphaTextureFolder,
                { kOptions.terrainTextureFolder, kOptions.textureFolder });

        std::vector<IntermediateMesh> kTerrainMeshes;
        std::vector<IntermediateMaterial> kTerrainMaterials;
        TerrainExporter kTerrainExporter(kTerrainDiffuseSearchFolders,
            kTerrainAlphaSearchFolders);
        if (!kTerrainExporter.Build(kFsmPath.string(),
            kFinalTexturePath.string(), kTerrainMeshes,
            kTerrainMaterials, kError))
        {
            std::cerr << "  Terrain parse failed: " << kError << std::endl;
            ++iFailures;
            continue;
        }

        std::cout << "  Generated final texture from FSM splat layers: "
            << kFinalTexturePath.string() << std::endl;

        size_t stVertices = 0;
        size_t stTriangles = 0;
        for (const IntermediateMesh& kMesh : kTerrainMeshes)
        {
            stVertices += kMesh.positions.size();
            stTriangles += kMesh.indices.size() / 3u;
        }

        std::cout << "  Terrain mesh: vertices: " << stVertices
            << ", triangles: " << stTriangles
            << ", normals: heightmap/source + generated fallback"
            << ", handedness/UVs: Assimp ConvertToLeftHanded"
            << ", axes: "
            << (kOptions.unrealAxes
                ? "Unreal (+X forward, +Z up)"
                : "native Z-up") << std::endl;

        TextureExporter kTerrainTextureExporter(
            kFinalTexturePath.parent_path().string(),
            kOutputFolder.string());
        FbxWriter kTerrainWriter(kTerrainTextureExporter,
            kOptions.unitScale, kOptions.unrealAxes);

        const fs::path kOutputPath = kOutputFolder /
            (kFsmPath.stem().string() + ".fbx");
        const std::vector<aiAnimation*> kNoAnimations;
        if (!kTerrainWriter.Write(kOutputPath.string(), nullptr,
            kTerrainMeshes, kTerrainMaterials, kNoAnimations, kError))
        {
            std::cerr << "  Terrain export failed: " << kError << std::endl;
            ++iFailures;
        }
        else
        {
            std::cout << "  Written: " << kOutputPath.string() << std::endl;
        }
    }

    return iFailures > 0 ? 1 : 0;
}

bool AssimpExporterApp::ParseCommandLine(int argc, char** argv,
    ExportOptions& kOptions, std::string& kError) const
{
    auto ValidateFolder = [&](const std::string& kFlag,
        const std::string& kPath) -> bool
    {
        if (kPath.empty())
            return true;
        std::error_code kFsError;
        if (!fs::exists(kPath, kFsError) ||
            !fs::is_directory(kPath, kFsError))
        {
            kError = "Folder for " + kFlag + " does not exist: \"" +
                kPath + "\"";
            return false;
        }
        return true;
    };

    auto ValidateFile = [&](const std::string& kPath,
        const std::vector<std::string>& kExtensions,
        const std::string& kExpectedDescription) -> bool
    {
        std::error_code kFsError;
        if (!fs::exists(kPath, kFsError) ||
            !fs::is_regular_file(kPath, kFsError))
        {
            kError = "Input file not found: \"" + kPath + "\"";
            return false;
        }

        const std::string kExtension = GetLowerExtension(fs::path(kPath));
        if (std::find(kExtensions.begin(), kExtensions.end(), kExtension) ==
            kExtensions.end())
        {
            kError = "Unsupported input file type (expected " +
                kExpectedDescription + "): \"" + kPath + "\"";
            return false;
        }
        return true;
    };

    for (int iArg = 1; iArg < argc; ++iArg)
    {
        const std::string kArg = argv[iArg] ? argv[iArg] : "";

        if (kArg == "-scale" || kArg == "-sample_rate")
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

                if (fValue <= 0.0f)
                    throw std::invalid_argument("not a positive number");
                if (kArg == "-scale")
                    kOptions.unitScale = fValue;
                else
                    kOptions.sampleRate = fValue;
            }
            catch (const std::exception&)
            {
                kError = "Invalid numeric value for " + kArg + ": \"" +
                    kValue + "\"";
                return false;
            }
            continue;
        }

        if (kArg == "-anim_folder" ||
            kArg == "-nif_folder" ||
            kArg == "-texture_folder" ||
            kArg == "-kfm_folder" ||
            kArg == "-output" ||
            kArg == "-terrain_folder" ||
            kArg == "-terrain_texture_folder" ||
            kArg == "-terrain_alpha_texture_folder")
        {
            if (iArg + 1 >= argc)
            {
                kError = "Missing value for argument: " + kArg;
                return false;
            }

            const std::string kValue = argv[++iArg] ? argv[iArg] : "";
            if (kArg == "-anim_folder")
                kOptions.animFolder = kValue;
            else if (kArg == "-nif_folder")
                kOptions.nifFolder = kValue;
            else if (kArg == "-texture_folder")
                kOptions.textureFolder = kValue;
            else if (kArg == "-kfm_folder")
                kOptions.kfmFolder = kValue;
            else if (kArg == "-output")
                kOptions.outputFolder = kValue;
            else if (kArg == "-terrain_folder")
                kOptions.terrainFolder = kValue;
            else if (kArg == "-terrain_texture_folder")
                kOptions.terrainTextureFolder = kValue;
            else if (kArg == "-terrain_alpha_texture_folder")
                kOptions.terrainAlphaTextureFolder = kValue;
            continue;
        }

        if (kArg == "-all")
        {
            kOptions.exportAll = true;
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

        if (!kArg.empty() && kArg[0] == '-')
        {
            kError = "Unknown argument: " + kArg;
            return false;
        }

        const std::string kExtension = GetLowerExtension(fs::path(kArg));
        if (kExtension == ".fsm")
            kOptions.terrainInputPaths.push_back(kArg);
        else if (kExtension == ".png")
        {
            kError = "A final terrain PNG must not be supplied. It is now "
                "generated from the diffuse layers and alpha maps stored in "
                "the .fsm. Use -terrain_texture_folder for diffuse textures "
                "and -terrain_alpha_texture_folder for alpha maps (or "
                "-texture_folder as a fallback search root).";
            return false;
        }
        else
            kOptions.inputPaths.push_back(kArg);
    }

    if (!ValidateFolder("-nif_folder", kOptions.nifFolder)) return false;
    if (!ValidateFolder("-anim_folder", kOptions.animFolder)) return false;
    if (!ValidateFolder("-texture_folder", kOptions.textureFolder)) return false;
    if (!ValidateFolder("-kfm_folder", kOptions.kfmFolder)) return false;
    if (!ValidateFolder("-terrain_folder", kOptions.terrainFolder)) return false;
    if (!ValidateFolder("-terrain_texture_folder",
        kOptions.terrainTextureFolder)) return false;
    if (!ValidateFolder("-terrain_alpha_texture_folder",
        kOptions.terrainAlphaTextureFolder)) return false;

    // Merely specifying -terrain_folder enables terrain batch mode. Search is
    // recursive, so maps in all nested subfolders are included without -all.
    if (!kOptions.terrainFolder.empty())
    {
        std::error_code kScanError;
        for (const fs::directory_entry& kEntry :
            fs::recursive_directory_iterator(kOptions.terrainFolder,
                fs::directory_options::skip_permission_denied, kScanError))
        {
            if (kScanError)
            {
                kError = "Failed while recursively scanning -terrain_folder: " +
                    kScanError.message();
                return false;
            }

            if (kEntry.is_regular_file(kScanError) &&
                GetLowerExtension(kEntry.path()) == ".fsm")
            {
                kOptions.terrainInputPaths.push_back(kEntry.path().string());
            }
        }

        SortAndUniquePaths(kOptions.terrainInputPaths);
        if (kOptions.terrainInputPaths.empty())
        {
            kError = "-terrain_folder was specified but no .fsm files were "
                "found in it or its subfolders.";
            return false;
        }

        std::cout << "[terrain] Batch mode enabled by -terrain_folder. Found "
            << kOptions.terrainInputPaths.size()
            << " .fsm file(s), including subfolders." << std::endl;
    }

    if (kOptions.exportAll)
    {
        const std::string& kScanKfm = kOptions.kfmFolder;
        const std::string& kScanNif = kOptions.nifFolder;

        if (kScanKfm.empty() && kScanNif.empty() &&
            kOptions.terrainInputPaths.empty())
        {
            kError = "-all requires at least -kfm_folder, -nif_folder, or "
                "-terrain_folder to know where to scan.";
            return false;
        }

        std::error_code kScanError;
        const std::size_t stExplicitModelInputs = kOptions.inputPaths.size();

        if (!kScanKfm.empty())
        {
            for (const fs::directory_entry& kEntry :
                fs::recursive_directory_iterator(kScanKfm,
                    fs::directory_options::skip_permission_denied, kScanError))
            {
                if (kScanError)
                    break;
                if (kEntry.is_regular_file(kScanError) &&
                    GetLowerExtension(kEntry.path()) == ".kfm")
                {
                    kOptions.inputPaths.push_back(kEntry.path().string());
                }
            }
        }

        if (kOptions.inputPaths.size() == stExplicitModelInputs &&
            !kScanNif.empty())
        {
            for (const fs::directory_entry& kEntry :
                fs::recursive_directory_iterator(kScanNif,
                    fs::directory_options::skip_permission_denied, kScanError))
            {
                if (kScanError)
                    break;
                if (kEntry.is_regular_file(kScanError) &&
                    GetLowerExtension(kEntry.path()) == ".nif")
                {
                    kOptions.inputPaths.push_back(kEntry.path().string());
                }
            }
        }

        SortAndUniquePaths(kOptions.inputPaths);
        if (kOptions.inputPaths.empty() &&
            kOptions.terrainInputPaths.empty())
        {
            kError = "-all was specified but no supported model or terrain "
                "files were found in the scan folders.";
            return false;
        }

        if (!kOptions.inputPaths.empty())
        {
            std::cout << "[all] Found " << kOptions.inputPaths.size()
                << " model file(s) to export." << std::endl;
        }
    }

    SortAndUniquePaths(kOptions.inputPaths);
    SortAndUniquePaths(kOptions.terrainInputPaths);

    for (const std::string& kPath : kOptions.inputPaths)
    {
        if (!ValidateFile(kPath, { ".nif", ".kfm", ".kf" },
            ".nif, .kfm or .kf"))
        {
            return false;
        }
    }

    for (const std::string& kPath : kOptions.terrainInputPaths)
    {
        if (!ValidateFile(kPath, { ".fsm" }, ".fsm"))
            return false;
    }

    if (kOptions.inputPaths.empty() && kOptions.terrainInputPaths.empty())
    {
        kError = "No input files provided. Pass .nif/.kfm/.kf/.fsm files, "
            "use -all, or specify -terrain_folder for recursive terrain batch.";
        return false;
    }

    return true;
}

void AssimpExporterApp::PrintUsage() const
{
    std::cout
        << "Usage:\n"
        << "  AssimpExporter [options] <file1.nif|.kfm|.kf|.fsm> [file2 ...]\n"
        << "  AssimpExporter [options] <terrain.fsm>\n"
        << "  AssimpExporter [options] -all\n"
        << "  AssimpExporter -terrain_folder <dir> [options]\n"
        << "\n"
        << "  -terrain_folder automatically enables recursive FSM batch mode;\n"
        << "  -all is not required. Output subfolders mirror the input tree,\n"
        << "  and every exported asset is placed in a folder named after it.\n"
        << "\nOptions:\n"
        << "  -nif_folder             <dir>  Search folder for NIF files\n"
        << "  -anim_folder            <dir>  Search folder for KF animation files\n"
        << "  -texture_folder         <dir>  Search/output folder for model textures\n"
        << "  -kfm_folder             <dir>  Search folder for KFM files\n"
        << "  -terrain_folder         <dir>  Recursively batch-convert all .fsm files\n"
        << "  -terrain_texture_folder <dir>  Root containing terrain diffuse/base textures\n"
        << "  -terrain_alpha_texture_folder <dir>  Root containing terrain alpha maps\n"
        << "  -output                 <dir>  Root for per-file FBX/texture folders\n"
        << "  -scale                  <num>  Source units -> FBX centimeters; default 100\n"
        << "  -sample_rate            <fps>  Bake animation curves; default 30\n"
        << "                                  Complete normals are exported; Assimp converts to left-handed FBX\n"
        << "  -unreal_axes                   Export +X forward, +Z up (default)\n"
        << "  -no_unreal_axes                Preserve native source axes\n"
        << "  -all                           Auto-discover model assets\n"
        << "\nSingle terrain example:\n"
        << "  AssimpExporter"
        << " -output \"C:\\Export\\Map\""
        << " -terrain_texture_folder \"C:\\Game\\Map\\Textures\""
        << " -terrain_alpha_texture_folder \"C:\\Game\\Map\\AlphaMaps\""
        << " \"C:\\Game\\Map\\Map01.fsm\"\n"
        << "\nRecursive terrain batch example:\n"
        << "  AssimpExporter"
        << " -terrain_folder \"C:\\Game\\Maps\""
        << " -terrain_texture_folder \"C:\\Game\\TerrainTextures\""
        << " -terrain_alpha_texture_folder \"C:\\Game\\TerrainAlpha\""
        << " -output \"C:\\Export\\Maps\"\n"
        << "\nModel batch example:\n"
        << "  AssimpExporter"
        << " -kfm_folder \"C:\\Game\\monster\""
        << " -anim_folder \"C:\\Game\\monster\\animation\""
        << " -texture_folder \"C:\\Game\\monster\\texture\""
        << " -output \"C:\\Export\\monster\""
        << " -all\n";
}
