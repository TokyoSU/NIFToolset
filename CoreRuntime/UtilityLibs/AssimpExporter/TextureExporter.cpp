#include "TextureExporter.h"

#include <NiImageConverter.h>
#include <NiPixelData.h>
#include <NiPixelFormat.h>
#include <NiSourceTexture.h>

#include <assimp/scene.h>
#include <assimp/material.h>

#include <filesystem>
#include <algorithm>
#include <cstring>
#include <cstdio>
#include <string>
#include <limits>
#include <vector>

// Windows Imaging Component for PNG write
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <wincodec.h>
#include <wrl/client.h>

namespace fs = std::filesystem;
using Microsoft::WRL::ComPtr;

//--------------------------------------------------------------------------------------------------
TextureExporter::TextureExporter(const std::string& kSearchFolder,
	const std::string& kOutputFolder)
	: m_kSearchFolder(kSearchFolder)
	, m_kOutputFolder(kOutputFolder)
{
}

//--------------------------------------------------------------------------------------------------
std::string TextureExporter::FindSourceFile(const std::string& kRawPath) const
{
	if (kRawPath.empty())
		return std::string();

	// 1. Try the exact path as-is
	fs::path kPath(kRawPath);
	if (fs::exists(kPath))
		return kPath.string();

	// 2. Try relative to the search folder
	fs::path kRelPath = fs::path(m_kSearchFolder) / kPath.filename();
	if (fs::exists(kRelPath))
		return kRelPath.string();

	// 3. Try the full original path but under the search folder
	fs::path kRootRelative = fs::path(m_kSearchFolder) / kPath;
	if (fs::exists(kRootRelative))
		return kRootRelative.string();

	// 4. Recursive search by filename in the search folder
	if (!m_kSearchFolder.empty() && fs::exists(m_kSearchFolder))
	{
		std::string kFileName = kPath.filename().string();
		std::error_code ec;
		for (auto& kEntry : fs::recursive_directory_iterator(m_kSearchFolder, ec))
		{
			if (!ec && kEntry.is_regular_file(ec) &&
				kEntry.path().filename().string() == kFileName)
				return kEntry.path().string();
		}
	}

	return std::string();
}

//--------------------------------------------------------------------------------------------------
// Write RGBA8 NiPixelData as PNG using WIC.
//
// NiPixelFormat::RGBA32 is byte ordered R, G, B, A. The Windows PNG encoder,
// however, normally accepts 32bpp BGRA. IWICBitmapFrameEncode::SetPixelFormat
// is an in/out operation and may silently replace a requested RGBA format with
// BGRA. The old exporter ignored that negotiated format and then supplied RGBA
// memory, swapping red and blue in the generated PNG.
static bool WritePngWIC(const std::string& kDstPath,
	unsigned int uiWidth, unsigned int uiHeight,
	const unsigned char* pRgbaPixels, unsigned int uiRgbaStride)
{
	if (!pRgbaPixels || uiWidth == 0 || uiHeight == 0 ||
		uiRgbaStride < uiWidth * 4u)
	{
		return false;
	}

	const HRESULT hrCom = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
	if (FAILED(hrCom) && hrCom != RPC_E_CHANGED_MODE)
		return false;

	ComPtr<IWICImagingFactory> pFactory;
	HRESULT hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr,
		CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pFactory));
	if (FAILED(hr))
		return false;

	std::wstring kWPath(kDstPath.begin(), kDstPath.end());

	ComPtr<IWICStream> pStream;
	hr = pFactory->CreateStream(&pStream);
	if (FAILED(hr)) return false;
	hr = pStream->InitializeFromFilename(kWPath.c_str(), GENERIC_WRITE);
	if (FAILED(hr)) return false;

	ComPtr<IWICBitmapEncoder> pEncoder;
	hr = pFactory->CreateEncoder(GUID_ContainerFormatPng, nullptr, &pEncoder);
	if (FAILED(hr)) return false;
	hr = pEncoder->Initialize(pStream.Get(), WICBitmapEncoderNoCache);
	if (FAILED(hr)) return false;

	ComPtr<IWICBitmapFrameEncode> pFrame;
	ComPtr<IPropertyBag2> pProps;
	hr = pEncoder->CreateNewFrame(&pFrame, &pProps);
	if (FAILED(hr)) return false;
	hr = pFrame->Initialize(nullptr);
	if (FAILED(hr)) return false;
	hr = pFrame->SetSize(uiWidth, uiHeight);
	if (FAILED(hr)) return false;

	// Use the PNG encoder's native 32-bit byte layout explicitly instead of
	// relying on WIC to negotiate RGBA into another format behind our back.
	WICPixelFormatGUID kPixelFormat = GUID_WICPixelFormat32bppBGRA;
	hr = pFrame->SetPixelFormat(&kPixelFormat);
	if (FAILED(hr) || !IsEqualGUID(kPixelFormat, GUID_WICPixelFormat32bppBGRA))
		return false;

	const unsigned int uiBgraStride = uiWidth * 4u;
	std::vector<unsigned char> kBgraPixels(
		static_cast<size_t>(uiBgraStride) * uiHeight);

	for (unsigned int y = 0; y < uiHeight; ++y)
	{
		const unsigned char* pSrc = pRgbaPixels +
			static_cast<size_t>(y) * uiRgbaStride;
		unsigned char* pDst = kBgraPixels.data() +
			static_cast<size_t>(y) * uiBgraStride;

		for (unsigned int x = 0; x < uiWidth; ++x)
		{
			pDst[0] = pSrc[2]; // B
			pDst[1] = pSrc[1]; // G
			pDst[2] = pSrc[0]; // R
			pDst[3] = pSrc[3]; // A
			pSrc += 4;
			pDst += 4;
		}
	}

	const size_t stBufferSize = kBgraPixels.size();
	if (stBufferSize > static_cast<size_t>(std::numeric_limits<UINT>::max()))
		return false;

	hr = pFrame->WritePixels(uiHeight, uiBgraStride,
		static_cast<UINT>(stBufferSize), kBgraPixels.data());
	if (FAILED(hr)) return false;

	hr = pFrame->Commit();
	if (FAILED(hr)) return false;
	hr = pEncoder->Commit();
	return SUCCEEDED(hr);
}

//--------------------------------------------------------------------------------------------------
bool TextureExporter::ConvertToPng(const std::string& kSrcPath,
	const std::string& kDstPath) const
{
	NiImageConverter* pkConv = NiImageConverter::GetImageConverter();
	if (!pkConv || !pkConv->CanReadImageFile(kSrcPath.c_str()))
		return false;

	NiPixelData* pkPixels = pkConv->ReadImageFile(kSrcPath.c_str(), nullptr);
	if (!pkPixels)
		return false;

	// Convert to RGBA8 if needed
	const NiPixelFormat& kFmt = pkPixels->GetPixelFormat();
	NiPixelDataPtr spPixels(pkPixels);

	if (kFmt != NiPixelFormat::RGBA32)
	{
		NiPixelData* pkConverted = pkConv->ConvertPixelData(
			*pkPixels, NiPixelFormat::RGBA32, nullptr, false);
		if (!pkConverted)
			return false;
		spPixels = pkConverted;
	}

	unsigned int uiW = spPixels->GetWidth();
	unsigned int uiH = spPixels->GetHeight();
	const unsigned char* pBuf = spPixels->GetPixels();
	unsigned int uiStride = uiW * 4u;

	return WritePngWIC(kDstPath, uiW, uiH, pBuf, uiStride);
}

//--------------------------------------------------------------------------------------------------
bool TextureExporter::ConvertAlphaToTransparencyPng(
	const std::string& kSrcPath, const std::string& kDstPath) const
{
	NiImageConverter* pkConv = NiImageConverter::GetImageConverter();
	if (!pkConv || !pkConv->CanReadImageFile(kSrcPath.c_str()))
		return false;

	NiPixelData* pkPixels = pkConv->ReadImageFile(kSrcPath.c_str(), nullptr);
	if (!pkPixels)
		return false;

	NiPixelDataPtr spPixels(pkPixels);
	if (pkPixels->GetPixelFormat() != NiPixelFormat::RGBA32)
	{
		NiPixelData* pkConverted = pkConv->ConvertPixelData(
			*pkPixels, NiPixelFormat::RGBA32, nullptr, false);
		if (!pkConverted)
			return false;
		spPixels = pkConverted;
	}

	const unsigned int uiWidth = spPixels->GetWidth();
	const unsigned int uiHeight = spPixels->GetHeight();
	const unsigned char* pSource = spPixels->GetPixels();
	if (!pSource || uiWidth == 0 || uiHeight == 0)
		return false;

	const size_t stPixelCount = static_cast<size_t>(uiWidth) * uiHeight;
	if (stPixelCount > std::numeric_limits<size_t>::max() / 4u)
		return false;

	std::vector<unsigned char> kTransparency(stPixelCount * 4u);
	for (size_t i = 0; i < stPixelCount; ++i)
	{
		// FBX's TransparentColor convention is black=opaque and
		// white=transparent, which is the inverse of texture alpha.
		const unsigned char ucTransparency =
			static_cast<unsigned char>(255u - pSource[i * 4u + 3u]);
		kTransparency[i * 4u + 0u] = ucTransparency;
		kTransparency[i * 4u + 1u] = ucTransparency;
		kTransparency[i * 4u + 2u] = ucTransparency;
		kTransparency[i * 4u + 3u] = 255u;
	}

	return WritePngWIC(kDstPath, uiWidth, uiHeight,
		kTransparency.data(), uiWidth * 4u);
}

//--------------------------------------------------------------------------------------------------
bool TextureExporter::CopyAsPng(const std::string& kSrcPath,
	const std::string& kDstPath) const
{
	// If already PNG, just copy
	std::error_code ec;
	fs::copy_file(kSrcPath, kDstPath,
		fs::copy_options::overwrite_existing, ec);
	return !ec;
}

//--------------------------------------------------------------------------------------------------
std::string TextureExporter::GetFallbackTexturePath() const
{
	if (m_kOutputFolder.empty())
		return "__niftoolset_default_white.png";

	std::error_code ec;
	fs::create_directories(m_kOutputFolder, ec);
	if (ec)
		return "__niftoolset_default_white.png";

	const fs::path kFallbackPath = fs::path(m_kOutputFolder) / "__niftoolset_default_white.png";
	if (fs::exists(kFallbackPath))
		return kFallbackPath.string();

	const unsigned char kWhitePixel[] = { 255, 255, 255, 255 };
	if (!WritePngWIC(kFallbackPath.string(), 1, 1, kWhitePixel, sizeof(kWhitePixel)))
		return "__niftoolset_default_white.png";

	return kFallbackPath.string();
}

//--------------------------------------------------------------------------------------------------
std::string TextureExporter::ExportTexture(const std::string& kSourcePath) const
{
	if (kSourcePath.empty())
		return std::string();

	std::string kSrcFile = FindSourceFile(kSourcePath);
	if (kSrcFile.empty())
		return std::string();

	// Build output path
	std::error_code ec;
	if (!m_kOutputFolder.empty())
		fs::create_directories(m_kOutputFolder, ec);

	fs::path kSrcFsPath(kSrcFile);
	std::string kStem = kSrcFsPath.stem().string();
	std::string kDstPath = (fs::path(m_kOutputFolder) / (kStem + ".png")).string();

	// Always regenerate converted files. Older exporter builds could have
	// produced red/blue-swapped PNGs at this same path, so reusing an existing
	// file would preserve the bad result after upgrading the executable.

	std::string kExt = kSrcFsPath.extension().string();
	std::transform(kExt.begin(), kExt.end(), kExt.begin(), ::tolower);

	if (kExt == ".png")
	{
		// The source may already be the destination file when the texture
		// folder also serves as the exporter output folder.
		std::error_code kEquivalentError;
		if (fs::exists(kDstPath) &&
			fs::equivalent(kSrcFile, kDstPath, kEquivalentError) &&
			!kEquivalentError)
		{
			return kDstPath;
		}

		if (CopyAsPng(kSrcFile, kDstPath))
			return kDstPath;
	}
	else
	{
		if (ConvertToPng(kSrcFile, kDstPath))
			return kDstPath;
	}

	return std::string();
}

//--------------------------------------------------------------------------------------------------
std::string TextureExporter::ExportTransparencyTexture(
	const std::string& kSourcePath) const
{
	if (kSourcePath.empty())
		return std::string();

	const std::string kSrcFile = FindSourceFile(kSourcePath);
	if (kSrcFile.empty())
		return std::string();

	std::error_code ec;
	if (!m_kOutputFolder.empty())
		fs::create_directories(m_kOutputFolder, ec);

	const fs::path kSrcFsPath(kSrcFile);
	const std::string kDstPath = (fs::path(m_kOutputFolder) /
		(kSrcFsPath.stem().string() + "_transparency.png")).string();

	// Always regenerate this map because it is derived from the source alpha
	// and older exporter builds did not create it at all.
	if (ConvertAlphaToTransparencyPng(kSrcFile, kDstPath))
		return kDstPath;

	return std::string();
}

//--------------------------------------------------------------------------------------------------
// Read a PNG file from disk and wrap its raw bytes in an aiTexture as a
// compressed ("png") embedded texture. Returns nullptr on failure.
aiTexture* TextureExporter::LoadEmbeddedPngTexture(const std::string& kPngPath) const
{
	std::error_code ec;
	auto uiFileSize = fs::file_size(kPngPath, ec);
	if (ec || uiFileSize == 0)
		return nullptr;

	FILE* pFile = nullptr;
	if (fopen_s(&pFile, kPngPath.c_str(), "rb") != 0 || !pFile)
		return nullptr;

	auto* pkTex = new aiTexture();
	pkTex->mWidth = static_cast<unsigned int>(uiFileSize); // byte count for compressed data
	pkTex->mHeight = 0; // 0 => compressed/encoded data (PNG)
	pkTex->pcData = reinterpret_cast<aiTexel*>(new unsigned char[uiFileSize]);
	std::strncpy(pkTex->achFormatHint, "png", sizeof(pkTex->achFormatHint) - 1);

	size_t uiRead = fread(pkTex->pcData, 1, uiFileSize, pFile);
	fclose(pFile);

	if (uiRead != uiFileSize)
	{
		delete pkTex;
		return nullptr;
	}

	return pkTex;
}

//--------------------------------------------------------------------------------------------------
aiMaterial* TextureExporter::BuildAiMaterial(const IntermediateMaterial& kMat,
	std::vector<aiTexture*>& kEmbeddedTextures) const
{
	aiMaterial* pkMat = new aiMaterial();

	// Material name
	aiString kName(kMat.name.c_str());
	pkMat->AddProperty(&kName, AI_MATKEY_NAME);

	// Resolve which PNG file backs the diffuse map: either the exported/
	// converted texture, or a generated white fallback when the NIF material
	// has no texture assigned at all.
	std::string kPngPath;
	if (!kMat.diffuseTexturePath.empty())
	{
		kPngPath = ExportTexture(kMat.diffuseTexturePath);
		if (kPngPath.empty())
			kPngPath = GetFallbackTexturePath();
	}
	else
	{
		kPngPath = GetFallbackTexturePath();
	}

	// Assimp 6.0.4's FBX exporter correlates material texture references
	// against aiScene::mTextures internally. Keep every referenced image in
	// that array and use the "*N" embedded-texture convention.
	auto AddEmbeddedTexture = [&](const std::string& kPath,
		aiTextureType eType) -> bool
	{
		if (kPath.empty())
			return false;

		aiTexture* pkEmbedded = LoadEmbeddedPngTexture(kPath);
		if (pkEmbedded)
		{
			kEmbeddedTextures.push_back(pkEmbedded);
			const int iTexIndex =
				static_cast<int>(kEmbeddedTextures.size() - 1);
			aiString kTexRef("*" + std::to_string(iTexIndex));
			pkMat->AddProperty(&kTexRef, "$tex.file",
				static_cast<unsigned int>(eType), 0);
			return true;
		}

		aiString kTexPath(kPath.c_str());
		pkMat->AddProperty(&kTexPath, "$tex.file",
			static_cast<unsigned int>(eType), 0);
		return true;
	};

	AddEmbeddedTexture(kPngPath, aiTextureType_DIFFUSE);

	if (kMat.useTextureAlpha)
	{
		const int iUseAlpha = aiTextureFlags_UseAlpha;
		pkMat->AddProperty(&iUseAlpha, 1, "$tex.flags",
			static_cast<unsigned int>(aiTextureType_DIFFUSE), 0);
	}

	const float fOpacity = std::clamp(kMat.opacity, 0.0f, 1.0f);
	pkMat->AddProperty(&fOpacity, 1, AI_MATKEY_OPACITY);

	if (kMat.useTextureAlpha && !kMat.diffuseTexturePath.empty())
	{
		const std::string kTransparencyPath =
			ExportTransparencyTexture(kMat.diffuseTexturePath);
		if (AddEmbeddedTexture(kTransparencyPath, aiTextureType_OPACITY))
		{
			// The FBX exporter connects aiTextureType_OPACITY to
			// TransparentColor. White therefore means transparent.
			const aiColor3D kTransparentColor(1.0f, 1.0f, 1.0f);
			pkMat->AddProperty(&kTransparentColor, 1,
				AI_MATKEY_COLOR_TRANSPARENT);
		}
	}

	return pkMat;
}
