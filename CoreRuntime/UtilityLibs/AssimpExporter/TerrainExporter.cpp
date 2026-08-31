#include "TerrainExporter.h"

#include <NiImageConverter.h>
#include <NiPixelData.h>
#include <NiPixelFormat.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <limits>
#include <iostream>
#include <sstream>
#include <unordered_map>
#include <vector>

// Windows Imaging Component for PNG output.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <wincodec.h>
#include <wrl/client.h>

namespace
{
    namespace fs = std::filesystem;
    using Microsoft::WRL::ComPtr;

    constexpr std::uint64_t FSM_SCENE_SIGNATURE_SIZE = 0x54u;
    constexpr std::uint64_t FSM_LAYER_SIGNATURE_SIZE = 4u;
    constexpr std::uint64_t FSM_TEXTURE_NAME_SIZE = 0x28u;
    constexpr std::uint64_t MAX_TERRAIN_VERTICES =
        256ull * 1024ull * 1024ull;
    constexpr std::int32_t MAX_FSM_TEXTURE_LAYERS = 256;
    constexpr std::int32_t MAX_BAKED_TERRAIN_LAYERS = 32;

    // Matches SceneTerrainBuilder's current shader constants.
    constexpr float TERRAIN_ALPHA_BLUR_RADIUS = 1.5f;
    constexpr float TERRAIN_ALPHA_EDGE_SOFTNESS = 0.20f;
    constexpr float DEFAULT_LAYER_SCALE = 16.0f;

    // Preserve the native detail of a common 256x256 texture tiled 16 times,
    // while preventing unexpectedly huge images in recursive batch exports.
    constexpr unsigned int MIN_FINAL_TEXTURE_SIZE = 256u;
    constexpr unsigned int MAX_FINAL_TEXTURE_SIZE = 4096u;

    struct TerrainHeightmap
    {
        std::int32_t width = 0;
        std::int32_t height = 0;
        float cellSize = 0.0f;
        std::vector<float> heights;
    };

    struct RawTerrainLayer
    {
        std::string materialName;
        std::string alphaMapName;
    };

    struct RgbaImage
    {
        unsigned int width = 0;
        unsigned int height = 0;
        std::vector<std::uint8_t> pixels;
    };

    struct TerrainLayer
    {
        RawTerrainLayer source;
        fs::path diffusePath;
        fs::path alphaPath;
        float scaleU = DEFAULT_LAYER_SCALE;
        float scaleV = DEFAULT_LAYER_SCALE;
        RgbaImage diffuse;
        RgbaImage alpha;
        std::vector<float> smoothedCoverage;
    };

    struct TerrainScene
    {
        TerrainHeightmap heightmap;
        std::vector<RawTerrainLayer> layers;
    };

    struct TextureScale
    {
        float u = DEFAULT_LAYER_SCALE;
        float v = DEFAULT_LAYER_SCALE;
    };

    template <class T>
    bool ReadValue(std::ifstream& kFile, T& kValue)
    {
        kFile.read(reinterpret_cast<char*>(&kValue), sizeof(T));
        return static_cast<bool>(kFile);
    }

    bool ReadBytes(std::ifstream& kFile, void* pDestination,
        std::uint64_t uiSize)
    {
        if (uiSize == 0)
            return true;

        if (uiSize > static_cast<std::uint64_t>(
            std::numeric_limits<std::streamsize>::max()))
        {
            return false;
        }

        kFile.read(static_cast<char*>(pDestination),
            static_cast<std::streamsize>(uiSize));
        return static_cast<bool>(kFile);
    }

    bool SafeMultiply(std::uint64_t uiLeft, std::uint64_t uiRight,
        std::uint64_t& uiResult)
    {
        if (uiLeft != 0 && uiRight >
            std::numeric_limits<std::uint64_t>::max() / uiLeft)
        {
            return false;
        }

        uiResult = uiLeft * uiRight;
        return true;
    }

    std::string Trim(std::string kValue)
    {
        const auto kNotSpace = [](unsigned char c)
        {
            return !std::isspace(c);
        };

        kValue.erase(kValue.begin(), std::find_if(
            kValue.begin(), kValue.end(), kNotSpace));
        kValue.erase(std::find_if(kValue.rbegin(), kValue.rend(),
            kNotSpace).base(), kValue.end());
        return kValue;
    }

    std::string ToLower(std::string kValue)
    {
        std::transform(kValue.begin(), kValue.end(), kValue.begin(),
            [](unsigned char c)
            {
                return static_cast<char>(std::tolower(c));
            });
        return kValue;
    }

    bool CaseInsensitiveEquals(const std::string& kLeft,
        const std::string& kRight)
    {
        return ToLower(kLeft) == ToLower(kRight);
    }

    std::string ReadFixedString(const char* pcData, std::size_t stSize)
    {
        std::size_t stLength = 0;
        while (stLength < stSize && pcData[stLength] != '\0')
            ++stLength;
        return Trim(std::string(pcData, stLength));
    }

    bool ContainsIfl(const std::string& kValue)
    {
        return ToLower(kValue).find(".ifl") != std::string::npos;
    }

    // Load only the first heightmap and its immediately following texture
    // layer block. This is all that is required for the final ground terrain;
    // water and later scene blocks are never parsed.
    bool LoadTerrainScene(const std::string& kFsmPath,
        TerrainScene& kOut, std::string& kError)
    {
        std::ifstream kFile(kFsmPath, std::ios::binary);
        if (!kFile)
        {
            kError = "Unable to open FSM file: \"" + kFsmPath + "\"";
            return false;
        }

        char acSceneSignature[FSM_SCENE_SIGNATURE_SIZE]{};
        if (!ReadBytes(kFile, acSceneSignature, sizeof(acSceneSignature)))
        {
            kError = "FSM file is truncated before the scene header: \"" +
                kFsmPath + "\"";
            return false;
        }

        std::int32_t iHeightmapCount = 0;
        if (!ReadValue(kFile, iHeightmapCount) || iHeightmapCount <= 0 ||
            iHeightmapCount > 128)
        {
            kError = "FSM contains an invalid heightmap count: \"" +
                kFsmPath + "\"";
            return false;
        }

        char acHeightSignature[FSM_LAYER_SIGNATURE_SIZE]{};
        std::int32_t iLinearSize = 0;
        std::int32_t iWidth = 0;
        std::int32_t iHeight = 0;
        float fScale = 0.0f;

        if (!ReadBytes(kFile, acHeightSignature, sizeof(acHeightSignature)) ||
            !ReadValue(kFile, iLinearSize) ||
            !ReadValue(kFile, iWidth) ||
            !ReadValue(kFile, iHeight) ||
            !ReadValue(kFile, fScale))
        {
            kError = "FSM is truncated in the terrain heightmap header: \"" +
                kFsmPath + "\"";
            return false;
        }

        if (iWidth < 2 || iHeight < 2 || iWidth > 65536 ||
            iHeight > 65536 || !std::isfinite(fScale) || fScale <= 0.0f)
        {
            kError = "FSM terrain has invalid dimensions or scale: \"" +
                kFsmPath + "\"";
            return false;
        }

        std::uint64_t uiSampleCount = 0;
        if (!SafeMultiply(static_cast<std::uint64_t>(iWidth),
            static_cast<std::uint64_t>(iHeight), uiSampleCount) ||
            uiSampleCount > MAX_TERRAIN_VERTICES ||
            uiSampleCount > static_cast<std::uint64_t>(
                std::numeric_limits<unsigned int>::max()))
        {
            kError = "FSM terrain is too large to export as one mesh: \"" +
                kFsmPath + "\"";
            return false;
        }

        kOut.heightmap.width = iWidth;
        kOut.heightmap.height = iHeight;
        kOut.heightmap.cellSize = fScale;
        kOut.heightmap.heights.resize(
            static_cast<std::size_t>(uiSampleCount));

        for (std::uint64_t i = 0; i < uiSampleCount; ++i)
        {
            float fHeight = 0.0f;
            std::uint8_t aucColor[4]{};
            if (!ReadValue(kFile, fHeight) ||
                !ReadBytes(kFile, aucColor, sizeof(aucColor)))
            {
                kError = "FSM is truncated in terrain height samples: \"" +
                    kFsmPath + "\"";
                return false;
            }

            if (!std::isfinite(fHeight))
                fHeight = 0.0f;

            kOut.heightmap.heights[static_cast<std::size_t>(i)] = fHeight;
        }

        char acTextureSignature[FSM_LAYER_SIGNATURE_SIZE]{};
        std::int32_t iTextureLinearSize = 0;
        std::int32_t iMaterialCount = 0;
        if (!ReadBytes(kFile, acTextureSignature,
                sizeof(acTextureSignature)) ||
            !ReadValue(kFile, iTextureLinearSize) ||
            !ReadValue(kFile, iMaterialCount))
        {
            kError = "FSM is truncated before the terrain texture layers: \"" +
                kFsmPath + "\"";
            return false;
        }

        if (iMaterialCount <= 0 ||
            iMaterialCount > MAX_FSM_TEXTURE_LAYERS)
        {
            kError = "FSM terrain has an invalid texture-layer count (" +
                std::to_string(iMaterialCount) + "): \"" + kFsmPath + "\"";
            return false;
        }

        kOut.layers.clear();
        kOut.layers.reserve(static_cast<std::size_t>(iMaterialCount));

        for (std::int32_t i = 0; i < iMaterialCount; ++i)
        {
            char acMaterial[FSM_TEXTURE_NAME_SIZE]{};
            char acAlpha[FSM_TEXTURE_NAME_SIZE]{};
            if (!ReadBytes(kFile, acMaterial, sizeof(acMaterial)) ||
                !ReadBytes(kFile, acAlpha, sizeof(acAlpha)))
            {
                kError = "FSM is truncated in terrain texture layer " +
                    std::to_string(i) + ": \"" + kFsmPath + "\"";
                return false;
            }

            RawTerrainLayer kLayer;
            kLayer.materialName = ReadFixedString(
                acMaterial, sizeof(acMaterial));
            kLayer.alphaMapName = ReadFixedString(acAlpha, sizeof(acAlpha));

            // IFL entries belong to animated water materials, not the ground.
            if (kLayer.materialName.empty() ||
                ContainsIfl(kLayer.materialName))
            {
                continue;
            }

            if (kLayer.alphaMapName.empty())
            {
                kError = "FSM terrain layer \"" + kLayer.materialName +
                    "\" has no alpha-map name.";
                return false;
            }

            if (kOut.layers.size() <
                static_cast<std::size_t>(MAX_BAKED_TERRAIN_LAYERS))
            {
                kOut.layers.push_back(std::move(kLayer));
            }
        }

        if (kOut.layers.empty())
        {
            kError = "FSM contains no ground terrain texture layers: \"" +
                kFsmPath + "\"";
            return false;
        }

        return true;
    }

    float SampleHeight(const TerrainHeightmap& kTerrain, int iX, int iZ)
    {
        iX = std::clamp(iX, 0, kTerrain.width - 1);
        iZ = std::clamp(iZ, 0, kTerrain.height - 1);
        return kTerrain.heights[static_cast<std::size_t>(iZ) *
            static_cast<std::size_t>(kTerrain.width) +
            static_cast<std::size_t>(iX)];
    }

    aiVector3D MakeTerrainNormal(const TerrainHeightmap& kTerrain,
        int iX, int iZ)
    {
        const float fInv2Cell = 0.5f / kTerrain.cellSize;
        const float fDX = (SampleHeight(kTerrain, iX + 1, iZ) -
            SampleHeight(kTerrain, iX - 1, iZ)) * fInv2Cell;
        const float fDZ = (SampleHeight(kTerrain, iX, iZ + 1) -
            SampleHeight(kTerrain, iX, iZ - 1)) * fInv2Cell;

        // Terrain positions are exported as (X, -Z, height), so the gradient
        // normal in this Z-up source basis is (-dH/dX, dH/dZ, 1).
        aiVector3D kNormal(-fDX, fDZ, 1.0f);
        const float fLengthSquared = kNormal.x * kNormal.x +
            kNormal.y * kNormal.y + kNormal.z * kNormal.z;
        if (std::isfinite(fLengthSquared) && fLengthSquared > 1.0e-20f)
            kNormal /= std::sqrt(fLengthSquared);
        else
            kNormal = aiVector3D(0.0f, 0.0f, 1.0f);
        return kNormal;
    }

    void AddUniquePath(std::vector<fs::path>& kPaths, const fs::path& kPath)
    {
        if (kPath.empty())
            return;

        const std::string kNormalized =
            ToLower(kPath.lexically_normal().string());
        for (const fs::path& kExisting : kPaths)
        {
            if (ToLower(kExisting.lexically_normal().string()) == kNormalized)
                return;
        }
        kPaths.push_back(kPath);
    }

    std::vector<fs::path> BuildAssetCandidates(
        const std::string& kRawName, bool bAlphaMap)
    {
        std::string kNormalized = kRawName;
        std::replace(kNormalized.begin(), kNormalized.end(), '\\', '/');
        fs::path kRawPath(kNormalized);

        std::vector<fs::path> kCandidates;
        const auto Add = [&](const fs::path& kCandidate)
        {
            if (kCandidate.empty())
                return;
            const std::string kLower = ToLower(kCandidate.string());
            for (const fs::path& kExisting : kCandidates)
            {
                if (ToLower(kExisting.string()) == kLower)
                    return;
            }
            kCandidates.push_back(kCandidate);
        };

        if (bAlphaMap)
        {
            // Grand Fantasia FSM names commonly end in .bmp while the actual
            // alpha map used by the engine is the same stem stored as .dds.
            const std::array<const char*, 4> kPreferredExtensions =
            {
                ".dds", ".png", ".bmp", ".tga"
            };
            for (const char* pcExtension : kPreferredExtensions)
            {
                fs::path kCandidate = kRawPath;
                kCandidate.replace_extension(pcExtension);
                Add(kCandidate);
            }
        }

        Add(kRawPath);
        return kCandidates;
    }

    fs::path FindAssetFile(const std::string& kRawName, bool bAlphaMap,
        const std::vector<fs::path>& kSearchRoots)
    {
        const std::vector<fs::path> kCandidates =
            BuildAssetCandidates(kRawName, bAlphaMap);
        std::error_code kError;

        for (const fs::path& kCandidate : kCandidates)
        {
            if (kCandidate.is_absolute() &&
                fs::is_regular_file(kCandidate, kError))
            {
                return kCandidate;
            }
        }

        for (const fs::path& kRoot : kSearchRoots)
        {
            for (const fs::path& kCandidate : kCandidates)
            {
                const fs::path kFullPath = kRoot / kCandidate;
                kError.clear();
                if (fs::is_regular_file(kFullPath, kError))
                    return kFullPath;

                const fs::path kFileOnly = kRoot / kCandidate.filename();
                kError.clear();
                if (fs::is_regular_file(kFileOnly, kError))
                    return kFileOnly;
            }
        }

        // Last resort: recursively search by filename. Search-root order is
        // retained, so a map-specific subfolder wins over a global asset root.
        for (const fs::path& kRoot : kSearchRoots)
        {
            kError.clear();
            if (!fs::is_directory(kRoot, kError))
                continue;

            for (const fs::directory_entry& kEntry :
                fs::recursive_directory_iterator(kRoot,
                    fs::directory_options::skip_permission_denied, kError))
            {
                if (kError)
                    break;
                if (!kEntry.is_regular_file(kError))
                    continue;

                for (const fs::path& kCandidate : kCandidates)
                {
                    if (CaseInsensitiveEquals(
                        kEntry.path().filename().string(),
                        kCandidate.filename().string()))
                    {
                        return kEntry.path();
                    }
                }
            }
        }

        return {};
    }

    std::vector<std::string> ParseCsvFields(const std::string& kLine)
    {
        char cSeparator = ',';
        if (kLine.find(',') == std::string::npos)
        {
            if (kLine.find(';') != std::string::npos)
                cSeparator = ';';
            else if (kLine.find('\t') != std::string::npos)
                cSeparator = '\t';
        }

        std::vector<std::string> kFields;
        std::string kField;
        bool bQuoted = false;
        for (std::size_t i = 0; i < kLine.size(); ++i)
        {
            const char c = kLine[i];
            if (c == '"')
            {
                if (bQuoted && i + 1 < kLine.size() && kLine[i + 1] == '"')
                {
                    kField.push_back('"');
                    ++i;
                }
                else
                {
                    bQuoted = !bQuoted;
                }
            }
            else if (c == cSeparator && !bQuoted)
            {
                kFields.push_back(Trim(kField));
                kField.clear();
            }
            else
            {
                kField.push_back(c);
            }
        }
        kFields.push_back(Trim(kField));
        return kFields;
    }

    fs::path FindTextureScalingCsv(const fs::path& kFsmPath,
        const std::vector<fs::path>& kSearchRoots)
    {
        const std::string kFilename =
            kFsmPath.stem().string() + "_texscaling.csv";

        std::error_code kError;
        const fs::path kBesideFsm = kFsmPath.parent_path() / kFilename;
        if (fs::is_regular_file(kBesideFsm, kError))
            return kBesideFsm;

        for (const fs::path& kRoot : kSearchRoots)
        {
            const fs::path kDirect = kRoot / kFilename;
            kError.clear();
            if (fs::is_regular_file(kDirect, kError))
                return kDirect;
        }

        for (const fs::path& kRoot : kSearchRoots)
        {
            kError.clear();
            if (!fs::is_directory(kRoot, kError))
                continue;

            for (const fs::directory_entry& kEntry :
                fs::recursive_directory_iterator(kRoot,
                    fs::directory_options::skip_permission_denied, kError))
            {
                if (kError)
                    break;
                if (kEntry.is_regular_file(kError) &&
                    CaseInsensitiveEquals(
                        kEntry.path().filename().string(), kFilename))
                {
                    return kEntry.path();
                }
            }
        }
        return {};
    }

    std::unordered_map<std::string, TextureScale> LoadTextureScales(
        const fs::path& kCsvPath)
    {
        std::unordered_map<std::string, TextureScale> kScales;
        if (kCsvPath.empty())
            return kScales;

        std::ifstream kFile(kCsvPath);
        if (!kFile)
            return kScales;

        std::string kLine;
        bool bFirstLine = true;
        while (std::getline(kFile, kLine))
        {
            if (bFirstLine && kLine.size() >= 3 &&
                static_cast<unsigned char>(kLine[0]) == 0xEF &&
                static_cast<unsigned char>(kLine[1]) == 0xBB &&
                static_cast<unsigned char>(kLine[2]) == 0xBF)
            {
                kLine.erase(0, 3);
            }
            bFirstLine = false;

            kLine = Trim(kLine);
            if (kLine.empty() || kLine[0] == '#')
                continue;

            const std::vector<std::string> kFields = ParseCsvFields(kLine);
            if (kFields.size() < 3 || kFields[0].empty())
                continue;

            try
            {
                TextureScale kScale;
                kScale.u = std::stof(kFields[1]);
                kScale.v = std::stof(kFields[2]);
                if (!std::isfinite(kScale.u) || kScale.u <= 0.0f ||
                    !std::isfinite(kScale.v) || kScale.v <= 0.0f)
                {
                    continue;
                }

                kScales[ToLower(kFields[0])] = kScale;
                kScales[ToLower(fs::path(kFields[0]).filename().string())] =
                    kScale;
            }
            catch (const std::exception&)
            {
                continue;
            }
        }
        return kScales;
    }

    bool LoadImageRgba8(const fs::path& kPath, RgbaImage& kOut,
        std::string& kError)
    {
        NiImageConverter* pkConverter = NiImageConverter::GetImageConverter();
        if (!pkConverter ||
            !pkConverter->CanReadImageFile(kPath.string().c_str()))
        {
            kError = "No image converter can read terrain texture: \"" +
                kPath.string() + "\"";
            return false;
        }

        NiPixelData* pkPixels = pkConverter->ReadImageFile(
            kPath.string().c_str(), nullptr);
        if (!pkPixels)
        {
            kError = "Failed to read terrain texture: \"" +
                kPath.string() + "\"";
            return false;
        }

        NiPixelDataPtr spPixels(pkPixels);
        if (spPixels->GetPixelFormat() != NiPixelFormat::RGBA32)
        {
            NiPixelData* pkConverted = pkConverter->ConvertPixelData(
                *spPixels, NiPixelFormat::RGBA32, nullptr, false);
            if (!pkConverted)
            {
                kError = "Failed to convert terrain texture to RGBA8: \"" +
                    kPath.string() + "\"";
                return false;
            }
            spPixels = pkConverted;
        }

        kOut.width = spPixels->GetWidth();
        kOut.height = spPixels->GetHeight();
        if (kOut.width == 0 || kOut.height == 0)
        {
            kError = "Terrain texture has invalid dimensions: \"" +
                kPath.string() + "\"";
            return false;
        }

        std::uint64_t uiPixelCount = 0;
        if (!SafeMultiply(kOut.width, kOut.height, uiPixelCount) ||
            uiPixelCount > std::numeric_limits<std::size_t>::max() / 4u)
        {
            kError = "Terrain texture is too large: \"" +
                kPath.string() + "\"";
            return false;
        }

        const std::size_t stByteCount =
            static_cast<std::size_t>(uiPixelCount) * 4u;
        const unsigned char* pPixels = spPixels->GetPixels();
        if (!pPixels)
        {
            kError = "Terrain texture has no pixel data: \"" +
                kPath.string() + "\"";
            return false;
        }

        kOut.pixels.assign(pPixels, pPixels + stByteCount);
        return true;
    }

    int ClampIndex(int iValue, int iMaximum)
    {
        return std::clamp(iValue, 0, iMaximum - 1);
    }

    int WrapIndex(int iValue, int iMaximum)
    {
        iValue %= iMaximum;
        if (iValue < 0)
            iValue += iMaximum;
        return iValue;
    }

    float ReadRed(const RgbaImage& kImage, int iX, int iY)
    {
        const std::size_t stOffset =
            (static_cast<std::size_t>(iY) * kImage.width +
                static_cast<std::size_t>(iX)) * 4u;
        return static_cast<float>(kImage.pixels[stOffset]) / 255.0f;
    }

    std::array<float, 3> ReadRgb(const RgbaImage& kImage, int iX, int iY)
    {
        const std::size_t stOffset =
            (static_cast<std::size_t>(iY) * kImage.width +
                static_cast<std::size_t>(iX)) * 4u;
        return
        {
            static_cast<float>(kImage.pixels[stOffset + 0]) / 255.0f,
            static_cast<float>(kImage.pixels[stOffset + 1]) / 255.0f,
            static_cast<float>(kImage.pixels[stOffset + 2]) / 255.0f
        };
    }

    float SampleRedClamp(const RgbaImage& kImage, float fU, float fV)
    {
        const float fX = fU * static_cast<float>(kImage.width) - 0.5f;
        const float fY = fV * static_cast<float>(kImage.height) - 0.5f;
        const int iX0 = static_cast<int>(std::floor(fX));
        const int iY0 = static_cast<int>(std::floor(fY));
        const float fTX = fX - static_cast<float>(iX0);
        const float fTY = fY - static_cast<float>(iY0);

        const int iX1 = iX0 + 1;
        const int iY1 = iY0 + 1;
        const float f00 = ReadRed(kImage,
            ClampIndex(iX0, static_cast<int>(kImage.width)),
            ClampIndex(iY0, static_cast<int>(kImage.height)));
        const float f10 = ReadRed(kImage,
            ClampIndex(iX1, static_cast<int>(kImage.width)),
            ClampIndex(iY0, static_cast<int>(kImage.height)));
        const float f01 = ReadRed(kImage,
            ClampIndex(iX0, static_cast<int>(kImage.width)),
            ClampIndex(iY1, static_cast<int>(kImage.height)));
        const float f11 = ReadRed(kImage,
            ClampIndex(iX1, static_cast<int>(kImage.width)),
            ClampIndex(iY1, static_cast<int>(kImage.height)));

        const float fTop = f00 + (f10 - f00) * fTX;
        const float fBottom = f01 + (f11 - f01) * fTX;
        return fTop + (fBottom - fTop) * fTY;
    }

    std::array<float, 3> SampleRgbWrap(const RgbaImage& kImage,
        float fU, float fV)
    {
        fU -= std::floor(fU);
        fV -= std::floor(fV);

        const float fX = fU * static_cast<float>(kImage.width) - 0.5f;
        const float fY = fV * static_cast<float>(kImage.height) - 0.5f;
        const int iX0 = static_cast<int>(std::floor(fX));
        const int iY0 = static_cast<int>(std::floor(fY));
        const int iX1 = iX0 + 1;
        const int iY1 = iY0 + 1;
        const float fTX = fX - static_cast<float>(iX0);
        const float fTY = fY - static_cast<float>(iY0);

        const auto k00 = ReadRgb(kImage,
            WrapIndex(iX0, static_cast<int>(kImage.width)),
            WrapIndex(iY0, static_cast<int>(kImage.height)));
        const auto k10 = ReadRgb(kImage,
            WrapIndex(iX1, static_cast<int>(kImage.width)),
            WrapIndex(iY0, static_cast<int>(kImage.height)));
        const auto k01 = ReadRgb(kImage,
            WrapIndex(iX0, static_cast<int>(kImage.width)),
            WrapIndex(iY1, static_cast<int>(kImage.height)));
        const auto k11 = ReadRgb(kImage,
            WrapIndex(iX1, static_cast<int>(kImage.width)),
            WrapIndex(iY1, static_cast<int>(kImage.height)));

        std::array<float, 3> kResult{};
        for (int i = 0; i < 3; ++i)
        {
            const float fTop = k00[i] + (k10[i] - k00[i]) * fTX;
            const float fBottom = k01[i] + (k11[i] - k01[i]) * fTX;
            kResult[i] = fTop + (fBottom - fTop) * fTY;
        }
        return kResult;
    }

    float SmoothStep(float fEdge0, float fEdge1, float fValue)
    {
        const float fT = std::clamp(
            (fValue - fEdge0) / (fEdge1 - fEdge0), 0.0f, 1.0f);
        return fT * fT * (3.0f - 2.0f * fT);
    }

    void BuildSmoothedCoverage(TerrainLayer& kLayer)
    {
        const unsigned int uiWidth = kLayer.alpha.width;
        const unsigned int uiHeight = kLayer.alpha.height;
        kLayer.smoothedCoverage.resize(
            static_cast<std::size_t>(uiWidth) * uiHeight);

        constexpr std::array<std::array<float, 3>, 3> kWeights =
        {{
            {{ 1.0f, 2.0f, 1.0f }},
            {{ 2.0f, 4.0f, 2.0f }},
            {{ 1.0f, 2.0f, 1.0f }}
        }};

        for (unsigned int y = 0; y < uiHeight; ++y)
        {
            for (unsigned int x = 0; x < uiWidth; ++x)
            {
                const float fU = (static_cast<float>(x) + 0.5f) /
                    static_cast<float>(uiWidth);
                const float fV = (static_cast<float>(y) + 0.5f) /
                    static_cast<float>(uiHeight);

                float fAlpha = 0.0f;
                for (int iOffsetY = -1; iOffsetY <= 1; ++iOffsetY)
                {
                    for (int iOffsetX = -1; iOffsetX <= 1; ++iOffsetX)
                    {
                        const float fSampleU = fU +
                            static_cast<float>(iOffsetX) *
                            TERRAIN_ALPHA_BLUR_RADIUS /
                            static_cast<float>(uiWidth);
                        const float fSampleV = fV +
                            static_cast<float>(iOffsetY) *
                            TERRAIN_ALPHA_BLUR_RADIUS /
                            static_cast<float>(uiHeight);
                        fAlpha += SampleRedClamp(kLayer.alpha,
                            fSampleU, fSampleV) *
                            kWeights[static_cast<std::size_t>(iOffsetY + 1)]
                                [static_cast<std::size_t>(iOffsetX + 1)];
                    }
                }

                fAlpha /= 16.0f;
                fAlpha = SmoothStep(
                    0.5f - TERRAIN_ALPHA_EDGE_SOFTNESS,
                    0.5f + TERRAIN_ALPHA_EDGE_SOFTNESS,
                    fAlpha);

                kLayer.smoothedCoverage[
                    static_cast<std::size_t>(y) * uiWidth + x] = fAlpha;
            }
        }
    }

    float ReadCoverage(const TerrainLayer& kLayer, int iX, int iY)
    {
        return kLayer.smoothedCoverage[
            static_cast<std::size_t>(iY) * kLayer.alpha.width +
            static_cast<std::size_t>(iX)];
    }

    float SampleCoverageClamp(const TerrainLayer& kLayer, float fU, float fV)
    {
        const float fX = fU * static_cast<float>(kLayer.alpha.width) - 0.5f;
        const float fY = fV * static_cast<float>(kLayer.alpha.height) - 0.5f;
        const int iX0 = static_cast<int>(std::floor(fX));
        const int iY0 = static_cast<int>(std::floor(fY));
        const int iX1 = iX0 + 1;
        const int iY1 = iY0 + 1;
        const float fTX = fX - static_cast<float>(iX0);
        const float fTY = fY - static_cast<float>(iY0);

        const int iWidth = static_cast<int>(kLayer.alpha.width);
        const int iHeight = static_cast<int>(kLayer.alpha.height);
        const float f00 = ReadCoverage(kLayer,
            ClampIndex(iX0, iWidth), ClampIndex(iY0, iHeight));
        const float f10 = ReadCoverage(kLayer,
            ClampIndex(iX1, iWidth), ClampIndex(iY0, iHeight));
        const float f01 = ReadCoverage(kLayer,
            ClampIndex(iX0, iWidth), ClampIndex(iY1, iHeight));
        const float f11 = ReadCoverage(kLayer,
            ClampIndex(iX1, iWidth), ClampIndex(iY1, iHeight));

        const float fTop = f00 + (f10 - f00) * fTX;
        const float fBottom = f01 + (f11 - f01) * fTX;
        return fTop + (fBottom - fTop) * fTY;
    }

    unsigned int RoundUpPowerOfTwo(unsigned int uiValue)
    {
        uiValue = std::max(1u, uiValue);
        --uiValue;
        uiValue |= uiValue >> 1u;
        uiValue |= uiValue >> 2u;
        uiValue |= uiValue >> 4u;
        uiValue |= uiValue >> 8u;
        uiValue |= uiValue >> 16u;
        return uiValue + 1u;
    }

    unsigned int DetermineOutputDimension(
        const std::vector<TerrainLayer>& kLayers, bool bWidth)
    {
        float fDesired = static_cast<float>(MIN_FINAL_TEXTURE_SIZE);
        for (const TerrainLayer& kLayer : kLayers)
        {
            const unsigned int uiAlphaDimension = bWidth
                ? kLayer.alpha.width : kLayer.alpha.height;
            const unsigned int uiDiffuseDimension = bWidth
                ? kLayer.diffuse.width : kLayer.diffuse.height;
            const float fScale = bWidth ? kLayer.scaleU : kLayer.scaleV;

            fDesired = std::max(fDesired,
                static_cast<float>(uiAlphaDimension));
            fDesired = std::max(fDesired,
                static_cast<float>(uiDiffuseDimension) * fScale);
        }

        fDesired = std::min(fDesired,
            static_cast<float>(MAX_FINAL_TEXTURE_SIZE));
        const unsigned int uiRounded = RoundUpPowerOfTwo(
            static_cast<unsigned int>(std::ceil(fDesired)));
        return std::clamp(uiRounded,
            MIN_FINAL_TEXTURE_SIZE, MAX_FINAL_TEXTURE_SIZE);
    }

    class ComScope
    {
    public:
        ComScope()
        {
            const HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
            m_bUninitialize = SUCCEEDED(hr);
            m_bValid = SUCCEEDED(hr) || hr == RPC_E_CHANGED_MODE;
        }

        ~ComScope()
        {
            if (m_bUninitialize)
                CoUninitialize();
        }

        bool IsValid() const { return m_bValid; }

    private:
        bool m_bValid = false;
        bool m_bUninitialize = false;
    };

    bool WritePngRgba8(const fs::path& kOutputPath,
        unsigned int uiWidth, unsigned int uiHeight,
        const std::vector<std::uint8_t>& kPixels, std::string& kError)
    {
        if (uiWidth == 0 || uiHeight == 0 ||
            kPixels.size() !=
                static_cast<std::size_t>(uiWidth) * uiHeight * 4u)
        {
            kError = "Invalid generated terrain PNG pixel buffer.";
            return false;
        }

        std::error_code kDirectoryError;
        if (!kOutputPath.parent_path().empty())
            fs::create_directories(kOutputPath.parent_path(), kDirectoryError);
        if (kDirectoryError)
        {
            kError = "Failed to create terrain texture output folder: " +
                kDirectoryError.message();
            return false;
        }

        ComScope kCom;
        if (!kCom.IsValid())
        {
            kError = "Failed to initialize COM for terrain PNG output.";
            return false;
        }

        ComPtr<IWICImagingFactory> pFactory;
        HRESULT hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr,
            CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pFactory));
        if (FAILED(hr))
        {
            kError = "Failed to create the WIC imaging factory.";
            return false;
        }

        ComPtr<IWICStream> pStream;
        hr = pFactory->CreateStream(&pStream);
        if (FAILED(hr) || FAILED(pStream->InitializeFromFilename(
                kOutputPath.wstring().c_str(), GENERIC_WRITE)))
        {
            kError = "Failed to open generated terrain PNG for writing: \"" +
                kOutputPath.string() + "\"";
            return false;
        }

        ComPtr<IWICBitmapEncoder> pEncoder;
        hr = pFactory->CreateEncoder(
            GUID_ContainerFormatPng, nullptr, &pEncoder);
        if (FAILED(hr) || FAILED(pEncoder->Initialize(
                pStream.Get(), WICBitmapEncoderNoCache)))
        {
            kError = "Failed to initialize the terrain PNG encoder.";
            return false;
        }

        ComPtr<IWICBitmapFrameEncode> pFrame;
        ComPtr<IPropertyBag2> pProperties;
        hr = pEncoder->CreateNewFrame(&pFrame, &pProperties);
        if (FAILED(hr) || FAILED(pFrame->Initialize(nullptr)) ||
            FAILED(pFrame->SetSize(uiWidth, uiHeight)))
        {
            kError = "Failed to initialize the terrain PNG frame.";
            return false;
        }

        WICPixelFormatGUID kPixelFormat = GUID_WICPixelFormat32bppBGRA;
        hr = pFrame->SetPixelFormat(&kPixelFormat);
        if (FAILED(hr) ||
            !IsEqualGUID(kPixelFormat, GUID_WICPixelFormat32bppBGRA))
        {
            kError = "WIC did not accept 32-bit BGRA terrain pixels.";
            return false;
        }

        const unsigned int uiStride = uiWidth * 4u;
        std::vector<std::uint8_t> kBgra(kPixels.size());
        for (std::size_t i = 0; i < kPixels.size(); i += 4u)
        {
            kBgra[i + 0] = kPixels[i + 2];
            kBgra[i + 1] = kPixels[i + 1];
            kBgra[i + 2] = kPixels[i + 0];
            kBgra[i + 3] = kPixels[i + 3];
        }

        if (kBgra.size() > std::numeric_limits<UINT>::max())
        {
            kError = "Generated terrain PNG is too large for WIC.";
            return false;
        }

        hr = pFrame->WritePixels(uiHeight, uiStride,
            static_cast<UINT>(kBgra.size()), kBgra.data());
        if (FAILED(hr) || FAILED(pFrame->Commit()) ||
            FAILED(pEncoder->Commit()))
        {
            kError = "Failed while encoding generated terrain PNG: \"" +
                kOutputPath.string() + "\"";
            return false;
        }

        return true;
    }

    bool PrepareTerrainLayers(const fs::path& kFsmPath,
        const TerrainScene& kScene,
        const std::vector<fs::path>& kDiffuseSearchRoots,
        const std::vector<fs::path>& kAlphaSearchRoots,
        std::vector<TerrainLayer>& kLayers, std::string& kError)
    {
        const fs::path kScalingCsv =
            FindTextureScalingCsv(kFsmPath, kDiffuseSearchRoots);
        const auto kTextureScales = LoadTextureScales(kScalingCsv);

        kLayers.clear();
        kLayers.reserve(kScene.layers.size());
        for (const RawTerrainLayer& kRawLayer : kScene.layers)
        {
            TerrainLayer kLayer;
            kLayer.source = kRawLayer;
            kLayer.diffusePath = FindAssetFile(
                kRawLayer.materialName, false, kDiffuseSearchRoots);
            kLayer.alphaPath = FindAssetFile(
                kRawLayer.alphaMapName, true, kAlphaSearchRoots);

            if (kLayer.diffusePath.empty())
            {
                kError = "Terrain diffuse texture was not found for layer \"" +
                    kRawLayer.materialName + "\". Searched beside the FSM and "
                    "inside -terrain_texture_folder/-texture_folder, including "
                    "subfolders.";
                return false;
            }
            if (kLayer.alphaPath.empty())
            {
                kError = "Terrain alpha map was not found for layer \"" +
                    kRawLayer.alphaMapName + "\". Search roots include "
                    "-terrain_alpha_texture_folder, then -terrain_texture_folder, "
                    "then -texture_folder. The exporter also tried the same "
                    "filename stem with .dds, .png, .bmp and .tga.";
                return false;
            }
            const auto ApplyScale = [&](const std::string& kKey) -> bool
            {
                const auto kIt = kTextureScales.find(ToLower(kKey));
                if (kIt == kTextureScales.end())
                    return false;
                kLayer.scaleU = kIt->second.u;
                kLayer.scaleV = kIt->second.v;
                return true;
            };

            if (!ApplyScale(kRawLayer.materialName))
                ApplyScale(fs::path(kRawLayer.materialName).filename().string());

            if (!LoadImageRgba8(kLayer.diffusePath,
                    kLayer.diffuse, kError) ||
                !LoadImageRgba8(kLayer.alphaPath,
                    kLayer.alpha, kError))
            {
                return false;
            }

            BuildSmoothedCoverage(kLayer);
            std::cout << "  Terrain splat layer " << kLayers.size()
                << ": diffuse=\"" << kLayer.diffusePath.string()
                << "\", alpha=\"" << kLayer.alphaPath.string()
                << "\", UV scale=(" << kLayer.scaleU << ", "
                << kLayer.scaleV << ")" << std::endl;
            kLayers.push_back(std::move(kLayer));
        }

        return !kLayers.empty();
    }

    bool GenerateFinalTerrainTexture(const fs::path& kFsmPath,
        const TerrainScene& kScene,
        const std::vector<fs::path>& kDiffuseSearchRoots,
        const std::vector<fs::path>& kAlphaSearchRoots,
        const fs::path& kOutputPath, std::string& kError)
    {
        std::vector<TerrainLayer> kLayers;
        if (!PrepareTerrainLayers(kFsmPath, kScene,
                kDiffuseSearchRoots, kAlphaSearchRoots, kLayers, kError))
        {
            return false;
        }

        const unsigned int uiWidth =
            DetermineOutputDimension(kLayers, true);
        const unsigned int uiHeight =
            DetermineOutputDimension(kLayers, false);
        std::cout << "  Baking final terrain texture: " << uiWidth
            << "x" << uiHeight << ", layers: " << kLayers.size()
            << std::endl;

        std::vector<std::uint8_t> kOutput(
            static_cast<std::size_t>(uiWidth) * uiHeight * 4u, 0u);

        // Reproduce TerrainSplatTextureArray exactly:
        //   color starts at black;
        //   each layer samples diffuse at UV * scale;
        //   its alpha map is sampled in unscaled terrain UV space;
        //   color = lerp(color, layerColor, coverage), in FSM order.
        for (unsigned int y = 0; y < uiHeight; ++y)
        {
            const float fV = (static_cast<float>(y) + 0.5f) /
                static_cast<float>(uiHeight);
            for (unsigned int x = 0; x < uiWidth; ++x)
            {
                const float fU = (static_cast<float>(x) + 0.5f) /
                    static_cast<float>(uiWidth);
                std::array<float, 3> kColor{ 0.0f, 0.0f, 0.0f };

                for (const TerrainLayer& kLayer : kLayers)
                {
                    const float fCoverage = std::clamp(
                        SampleCoverageClamp(kLayer, fU, fV), 0.0f, 1.0f);
                    if (fCoverage <= 0.001f)
                        continue;

                    const auto kLayerColor = SampleRgbWrap(kLayer.diffuse,
                        fU * kLayer.scaleU, fV * kLayer.scaleV);
                    for (int i = 0; i < 3; ++i)
                    {
                        kColor[i] +=
                            (kLayerColor[i] - kColor[i]) * fCoverage;
                    }
                }

                const std::size_t stOffset =
                    (static_cast<std::size_t>(y) * uiWidth + x) * 4u;
                for (int i = 0; i < 3; ++i)
                {
                    kOutput[stOffset + static_cast<std::size_t>(i)] =
                        static_cast<std::uint8_t>(std::lround(
                            std::clamp(kColor[i], 0.0f, 1.0f) * 255.0f));
                }
                kOutput[stOffset + 3u] = 255u;
            }
        }

        return WritePngRgba8(kOutputPath,
            uiWidth, uiHeight, kOutput, kError);
    }
}

//--------------------------------------------------------------------------------------------------
TerrainExporter::TerrainExporter(
    const std::vector<std::string>& kDiffuseTextureSearchFolders,
    const std::vector<std::string>& kAlphaTextureSearchFolders)
    : m_kDiffuseTextureSearchFolders(kDiffuseTextureSearchFolders),
      m_kAlphaTextureSearchFolders(kAlphaTextureSearchFolders)
{
}

//--------------------------------------------------------------------------------------------------
bool TerrainExporter::Build(const std::string& kFsmPath,
    const std::string& kGeneratedTexturePngPath,
    std::vector<IntermediateMesh>& kMeshes,
    std::vector<IntermediateMaterial>& kMaterials,
    std::string& kError) const
{
    kMeshes.clear();
    kMaterials.clear();

    TerrainScene kScene;
    if (!LoadTerrainScene(kFsmPath, kScene, kError))
        return false;

    std::vector<fs::path> kDiffuseSearchRoots;
    std::vector<fs::path> kAlphaSearchRoots;
    AddUniquePath(kDiffuseSearchRoots, fs::path(kFsmPath).parent_path());
    for (const std::string& kSearchFolder : m_kDiffuseTextureSearchFolders)
        AddUniquePath(kDiffuseSearchRoots, fs::path(kSearchFolder));

    if (m_kAlphaTextureSearchFolders.empty())
    {
        kAlphaSearchRoots = kDiffuseSearchRoots;
    }
    else
    {
        AddUniquePath(kAlphaSearchRoots, fs::path(kFsmPath).parent_path());
        for (const std::string& kSearchFolder : m_kAlphaTextureSearchFolders)
            AddUniquePath(kAlphaSearchRoots, fs::path(kSearchFolder));
    }

    if (!GenerateFinalTerrainTexture(fs::path(kFsmPath), kScene,
            kDiffuseSearchRoots, kAlphaSearchRoots,
            fs::path(kGeneratedTexturePngPath), kError))
    {
        return false;
    }

    IntermediateMaterial kMaterial;
    kMaterial.name = fs::path(kFsmPath).stem().string() +
        "_TerrainMaterial";
    kMaterial.diffuseTexturePath = kGeneratedTexturePngPath;
    kMaterial.useTextureAlpha = false;
    kMaterial.alphaBlend = false;
    kMaterial.alphaTest = false;
    kMaterial.opacity = 1.0f;
    kMaterials.push_back(std::move(kMaterial));

    const TerrainHeightmap& kTerrain = kScene.heightmap;
    const std::size_t stVertexCount =
        static_cast<std::size_t>(kTerrain.width) *
        static_cast<std::size_t>(kTerrain.height);
    const std::size_t stQuadCount =
        static_cast<std::size_t>(kTerrain.width - 1) *
        static_cast<std::size_t>(kTerrain.height - 1);

    if (stQuadCount > static_cast<std::size_t>(
            std::numeric_limits<unsigned int>::max()) ||
        stQuadCount > std::numeric_limits<std::size_t>::max() / 6u)
    {
        kError = "FSM terrain has too many faces for one Assimp mesh: \"" +
            kFsmPath + "\"";
        return false;
    }

    IntermediateMesh kMesh;
    kMesh.name = fs::path(kFsmPath).stem().string() + "_Terrain";
    kMesh.materialIndex = 0;
    kMesh.positions.resize(stVertexCount);
    kMesh.normals.resize(stVertexCount);
    kMesh.uvs.resize(stVertexCount);
    kMesh.indices.reserve(stQuadCount * 6u);

    for (int iZ = 0; iZ < kTerrain.height; ++iZ)
    {
        for (int iX = 0; iX < kTerrain.width; ++iX)
        {
            const std::size_t stVertex =
                static_cast<std::size_t>(iZ) *
                static_cast<std::size_t>(kTerrain.width) +
                static_cast<std::size_t>(iX);

            const float fHeight = SampleHeight(kTerrain, iX, iZ);

            kMesh.positions[stVertex] = aiVector3D(
                static_cast<float>(iX) * kTerrain.cellSize,
                -static_cast<float>(iZ) * kTerrain.cellSize,
                fHeight);
            kMesh.normals[stVertex] = MakeTerrainNormal(kTerrain, iX, iZ);
            const float fU = (static_cast<float>(iX) + 0.5f) /
                static_cast<float>(kTerrain.width);
            const float fV = (static_cast<float>(iZ) + 0.5f) /
                static_cast<float>(kTerrain.height);
            kMesh.uvs[stVertex] = aiVector2D(fU, fV);
        }
    }

    for (int iZ = 0; iZ < kTerrain.height - 1; ++iZ)
    {
        for (int iX = 0; iX < kTerrain.width - 1; ++iX)
        {
            const std::size_t st00 =
                static_cast<std::size_t>(iZ) *
                static_cast<std::size_t>(kTerrain.width) +
                static_cast<std::size_t>(iX);
            const std::size_t st01 =
                static_cast<std::size_t>(iZ + 1) *
                static_cast<std::size_t>(kTerrain.width) +
                static_cast<std::size_t>(iX);

            const unsigned int ui00 = static_cast<unsigned int>(st00);
            const unsigned int ui10 = static_cast<unsigned int>(st00 + 1u);
            const unsigned int ui01 = static_cast<unsigned int>(st01);
            const unsigned int ui11 = static_cast<unsigned int>(st01 + 1u);

            const float fD0 = std::abs(
                kMesh.positions[ui00].z - kMesh.positions[ui11].z);
            const float fD1 = std::abs(
                kMesh.positions[ui10].z - kMesh.positions[ui01].z);

            // Reordering SceneTerrainBuilder's Y-up vertices into a Z-up
            // basis changes handedness, so reverse each triangle to preserve
            // upward-facing winding. Optional FBX handedness conversion is
            // applied later to the completed Assimp scene.
            if (fD0 <= fD1)
            {
                kMesh.indices.push_back(ui00);
                kMesh.indices.push_back(ui11);
                kMesh.indices.push_back(ui10);
                kMesh.indices.push_back(ui00);
                kMesh.indices.push_back(ui01);
                kMesh.indices.push_back(ui11);
            }
            else
            {
                kMesh.indices.push_back(ui00);
                kMesh.indices.push_back(ui01);
                kMesh.indices.push_back(ui10);
                kMesh.indices.push_back(ui10);
                kMesh.indices.push_back(ui01);
                kMesh.indices.push_back(ui11);
            }
        }
    }

    kMeshes.push_back(std::move(kMesh));
    return true;
}
