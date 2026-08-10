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
#include <cstring>
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

        // Batch export is flat by default: every FBX and exported texture is
        // written directly into the single -output directory.
        //
        // -preserve_folders changes only that layout by restoring one
        // model-named directory per FBX. Explicit non-batch exports retain
        // their historical per-model folder layout.
        if (kOptions.preserveFolders || !kOptions.exportAll)
            return kOutputRoot / kModelPath.stem();

        return kOutputRoot;
    }

    fs::path BuildOutputFbxPath(const fs::path& kOutputFolder,
        const std::string& kOutputStem)
    {
        return kOutputFolder / (kOutputStem + ".fbx");
    }

    std::string SanitizePathComponent(std::string kName,
        const std::string& kFallback)
    {
        constexpr const char* pcInvalidCharacters = "<>:\"/\\|?*";
        for (char& c : kName)
        {
            const unsigned char uc = static_cast<unsigned char>(c);
            if (uc < 32u || std::strchr(pcInvalidCharacters, c))
                c = '_';
        }

        // Windows silently strips trailing spaces and dots from file names.
        // Remove them ourselves so folder/FBX names remain predictable.
        while (!kName.empty() &&
            (kName.back() == ' ' || kName.back() == '.'))
        {
            kName.pop_back();
        }
        while (!kName.empty() && kName.front() == ' ')
            kName.erase(kName.begin());

        if (kName.empty() || kName == "." || kName == "..")
            kName = kFallback;

        // Avoid Win32 device names even when they are followed by an extension.
        const std::string kLowerStem = ToLower(fs::path(kName).stem().string());
        const bool bReserved = kLowerStem == "con" || kLowerStem == "prn" ||
            kLowerStem == "aux" || kLowerStem == "nul" ||
            (kLowerStem.size() == 4u &&
                ((kLowerStem.rfind("com", 0) == 0) ||
                 (kLowerStem.rfind("lpt", 0) == 0)) &&
                kLowerStem[3] >= '1' && kLowerStem[3] <= '9');
        if (bReserved)
            kName += '_';

        // Leave enough room for the suffix, extension and a reasonably long
        // output root while staying below common Win32 path limits.
        constexpr std::size_t stMaxComponentLength = 120u;
        if (kName.size() > stMaxComponentLength)
        {
            kName.resize(stMaxComponentLength);
            while (!kName.empty() &&
                (kName.back() == ' ' || kName.back() == '.'))
            {
                kName.pop_back();
            }
        }

        return kName.empty() ? kFallback : kName;
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
            << ", handedness: "
            << GetExportHandednessDescription(kOptions.handedness)
            << ", UVs: V flipped for FBX"
            << ", axes: "
            << GetExportAxisDescription(kOptions.axisPreset, kOptions.handedness)
            << std::endl;

        LoadedNifAsset kNifAsset;
        if (!kAssetLoader.LoadNifAsset(kAsset.modelNifPath,
            kNifAsset, kError))
        {
            std::cerr << "  " << kError << std::endl;
            ++iFailures;
            continue;
        }

        if (kNifAsset.roots.empty() || !kNifAsset.root)
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

        // KF/KFM data is loaded once for this source asset.
        std::vector<NiSequenceDataPtr> kExternalSequenceDatas;
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
                    kExternalSequenceDatas.insert(
                        kExternalSequenceDatas.end(),
                        kLoadedSequences.begin(), kLoadedSequences.end());
                }
                else
                {
                    std::cerr << "  Warning: failed to load KF sequences from "
                        << kKfPath << ": " << kError << std::endl;
                }
            }
        }

        auto ExportRoot = [&](NiAVObject* pkRoot,
            const fs::path& kOutputFolder,
            const std::string& kOutputStem) -> bool
        {
            if (!pkRoot)
            {
                std::cerr << "  Export object is null." << std::endl;
                return false;
            }

            std::error_code kAssetDirectoryError;
            fs::create_directories(kOutputFolder, kAssetDirectoryError);
            if (kAssetDirectoryError)
            {
                std::cerr << "  Failed to create object output directory: "
                    << kOutputFolder.string() << " ("
                    << kAssetDirectoryError.message() << ")" << std::endl;
                return false;
            }

            // Re-evaluate world transforms before each extraction because a
            // previous export may have evaluated controllers on another root.
            pkRoot->Update(0.0f, false);

            const std::string kTexOutputFolder = kOutputFolder.string();
            MeshExtractor kMeshEx(kTexOutputFolder, true,
                kOptions.unitScale, kOptions.axisPreset);
            TextureExporter kTexEx(kTexSearchFolder, kTexOutputFolder);

            std::vector<IntermediateMesh> kMeshes;
            std::vector<IntermediateMaterial> kMaterials;
            NodeIndexMap kNodeMap;
            kMeshEx.Extract(pkRoot, kMeshes, kMaterials, kNodeMap);

            if (kMeshes.empty())
            {
                std::cout << "    Warning: no meshes found in object."
                    << std::endl;
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

                std::cout << "    Meshes: " << kMeshes.size()
                    << " (skinned: " << stSkinnedMeshes << ")"
                    << ", vertices: " << stVertices
                    << ", triangles: " << stTriangles << std::endl;
            }

            std::vector<NiSequenceDataPtr> kSequenceDatas =
                kExternalSequenceDatas;
            AnimationExporter::AppendFromControllerManagers(
                pkRoot, kSequenceDatas);

            std::vector<aiAnimation*> kAnimations;
            if (!kSequenceDatas.empty())
            {
                kAnimations = AnimationExporter::BuildFromSequenceDatas(
                    kSequenceDatas, pkRoot, kOptions.unitScale,
                    kOptions.sampleRate, kOptions.axisPreset,
                    kNifAsset.pStream);
            }

            if (kAnimations.empty())
            {
                kAnimations = AnimationExporter::BuildFromNifControllers(
                    pkRoot, kOptions.unitScale, kOptions.sampleRate,
                    kOptions.axisPreset);
            }

            if (!kAnimations.empty())
            {
                std::cout << "    Animations: " << kAnimations.size()
                    << std::endl;
            }

            const std::string kOutputPath =
                BuildOutputFbxPath(kOutputFolder, kOutputStem).string();
            FbxWriter kWriter(kTexEx, kOptions.unitScale,
                kOptions.axisPreset, kOptions.handedness);
            if (!kWriter.Write(kOutputPath, pkRoot, kMeshes, kMaterials,
                kAnimations, kError))
            {
                std::cerr << "    Export failed: " << kError << std::endl;
                return false;
            }

            std::cout << "    Written: " << kOutputPath << std::endl;
            return true;
        };

        const std::string kModelStem = SanitizePathComponent(
            fs::path(kAsset.modelNifPath).stem().string(), "Model");
        if (!ExportRoot(kNifAsset.root, kAssetOutputFolder,
            kModelStem))
        {
            ++iFailures;
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
            << ", handedness: "
            << GetExportHandednessDescription(kOptions.handedness)
            << ", UVs: V flipped for FBX"
            << ", axes: "
            << GetExportAxisDescription(kOptions.axisPreset, kOptions.handedness) << std::endl;

        TextureExporter kTerrainTextureExporter(
            kFinalTexturePath.parent_path().string(),
            kOutputFolder.string());
        FbxWriter kTerrainWriter(kTerrainTextureExporter,
            kOptions.unitScale, kOptions.axisPreset, kOptions.handedness);

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
        if (kArg == "-preserve_folders" ||
            kArg == "-per_fbx_folder" ||
            kArg == "-mirror_folders")
        {
            kOptions.preserveFolders = true;
            continue;
        }
        if (kArg == "-unreal_axes")
        {
            kOptions.axisPreset = ExportAxisPreset::Unreal;
            continue;
        }
        if (kArg == "-unity_axes")
        {
            kOptions.axisPreset = ExportAxisPreset::Unity;
            continue;
        }
        if (kArg == "-native_axes" || kArg == "-no_unreal_axes")
        {
            kOptions.axisPreset = ExportAxisPreset::Native;
            continue;
        }
        if (kArg == "-left-handed" || kArg == "-left_handed")
        {
            kOptions.handedness = ExportHandedness::Left;
            continue;
        }
        if (kArg == "-right-handed" || kArg == "-right_handed")
        {
            kOptions.handedness = ExportHandedness::Right;
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
        << "  -output                 <dir>  Destination for exported FBX/textures\n"
        << "  -preserve_folders             With -all, create one folder per FBX\n"
        << "                                  (-per_fbx_folder and -mirror_folders are aliases)\n"
        << "  -scale                  <num>  Source units -> FBX centimeters; default 100\n"
        << "  -sample_rate            <fps>  Bake animation curves; default 30\n"
        << "                                  Complete normals are exported; UV V coordinates are flipped for FBX\n"
        << "  -unreal_axes                   Export +X forward, +Z up (default axes)\n"
        << "  -unity_axes                    Export Y-up Unity axes (-Z forward in right-handed mode; +Z in left-handed mode)\n"
        << "  -native_axes                   Preserve native +Y forward, +Z up axes\n"
        << "  -no_unreal_axes                Compatibility alias for -native_axes\n"
        << "  -left-handed                  Export a left-handed FBX (alias: -left_handed)\n"
        << "                                  Right-handed output is the default\n"
        << "  -right-handed                 Explicitly select the default right-handed mode\n"
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
        << "\nFlat model batch example (default):\n"
        << "  AssimpExporter"
        << " -kfm_folder \"C:\\Game\\monster\""
        << " -anim_folder \"C:\\Game\\monster\\animation\""
        << " -texture_folder \"C:\\Game\\monster\\texture\""
        << " -output \"C:\\Export\\monster\""
        << " -all\n"
        << "  Result: C:\\Export\\monster\\M001.fbx, M002.fbx, ...\n"
        << "\nOne folder per FBX example:\n"
        << "  AssimpExporter"
        << " -nif_folder \"C:\\Game\\models\""
        << " -output \"C:\\Export\\models\""
        << " -all -preserve_folders\n"
        << "  Result: C:\\Export\\models\\M001\\M001.fbx, ...\n"
        << "\nUnity-axis batch example:\n"
        << "  AssimpExporter"
        << " -nif_folder \"C:\\Game\\models\""
        << " -output \"C:\\Export\\Unity\""
        << " -all -unity_axes -left-handed\n";
}
