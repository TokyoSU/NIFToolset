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
// Write NiPixelData (any format, converted to RGBA8 first) as PNG using WIC
static bool WritePngWIC(const std::string& kDstPath,
	unsigned int uiWidth, unsigned int uiHeight,
	const unsigned char* pPixels, unsigned int uiStride)
{
	CoInitializeEx(nullptr, COINIT_MULTITHREADED);

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

	WICPixelFormatGUID pixFmt = GUID_WICPixelFormat32bppRGBA;
	hr = pFrame->SetPixelFormat(&pixFmt);
	if (FAILED(hr)) return false;

	hr = pFrame->WritePixels(uiHeight, uiStride, uiHeight * uiStride,
		const_cast<BYTE*>(pPixels));
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

	// Already converted: skip
	if (fs::exists(kDstPath))
		return kDstPath;

	std::string kExt = kSrcFsPath.extension().string();
	std::transform(kExt.begin(), kExt.end(), kExt.begin(), ::tolower);

	if (kExt == ".png")
	{
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
	// against aiScene::mTextures (embedded textures) internally; if that
	// array never contains a matching entry, it dereferences an end()
	// iterator and crashes. Embed the PNG data as an aiTexture and reference
	// it via the "*N" convention instead of an external file path so the
	// exporter's internal lookup always succeeds.
	aiTexture* pkEmbedded = kPngPath.empty() ? nullptr : LoadEmbeddedPngTexture(kPngPath);
	if (pkEmbedded)
	{
		kEmbeddedTextures.push_back(pkEmbedded);
		int iTexIndex = static_cast<int>(kEmbeddedTextures.size() - 1);
		aiString kTexRef("*" + std::to_string(iTexIndex));
		pkMat->AddProperty(&kTexRef, AI_MATKEY_TEXTURE_DIFFUSE(0));
	}
	else if (!kPngPath.empty())
	{
		// Could not read the PNG back from disk; fall back to referencing it
		// by external path (may still hit the Assimp bug for untextured
		// materials, but keeps the export from failing outright).
		aiString kTexPath(kPngPath.c_str());
		pkMat->AddProperty(&kTexPath, AI_MATKEY_TEXTURE_DIFFUSE(0));
	}

	return pkMat;
}
