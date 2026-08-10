#include "MeshExtractor.h"
#include "ExportNaming.h"
#include "AxisConversion.h"

#include <NiNode.h>
#include <NiMesh.h>
#include <NiGeometry.h>
#include <NiTriShape.h>
#include <NiTriStrips.h>
#include <NiTriShapeData.h>
#include <NiTriStripsData.h>
#include <NiTriBasedGeomData.h>
#include <NiSkinInstance.h>
#include <NiSkinData.h>
#include <NiSkinPartition.h>
#include <NiSkinningMeshModifier.h>
#include <NiTexturingProperty.h>
#include <NiTextureTransform.h>
#include <NiSourceTexture.h>
#include <NiDataStreamElementLock.h>
#include <NiCommonSemantics.h>
#include <NiFloat16.h>
#include <NiLODNode.h>
#include <NiSwitchNode.h>

#include <assimp/scene.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <iostream>
#include <limits>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace fs = std::filesystem;

namespace
{
    // NIF render properties are inherited down the scene graph. GetProperty()
    // only checks the current object, so an alpha-test property placed on a
    // parent NiNode would otherwise be missed and masked fur/cards would be
    // exported as opaque black polygons.
    template <class T>
    T* FindEffectiveProperty(NiAVObject* pkObject, int iPropertyType)
    {
        for (NiAVObject* pkCurrent = pkObject; pkCurrent;
            pkCurrent = pkCurrent->GetParent())
        {
            NiProperty* pkProperty = pkCurrent->GetProperty(iPropertyType);
            if (pkProperty)
                return static_cast<T*>(pkProperty);
        }

        return nullptr;
    }

    //--------------------------------------------------------------------------------------------------
    std::string ToLowerName(const char* pcName)
    {
        std::string kName = pcName ? pcName : "";
        std::transform(kName.begin(), kName.end(), kName.begin(),
            [](unsigned char c)
            {
                return static_cast<char>(std::tolower(c));
            });
        return kName;
    }

    //--------------------------------------------------------------------------------------------------
    template <class T>
    unsigned int SelectFallbackBoneIndex(T* const* ppkBones,
        unsigned int uiBoneCount)
    {
        if (!ppkBones || uiBoneCount == 0)
            return ~0u;

        // Prefer the common Grand Fantasia/Gamebryo skeleton roots. These are
        // safer rigid-attachment targets than blindly using palette slot 0.
        static const char* s_apcPreferredNames[] =
        {
            "bip01 nonaccum",
            "bip01",
            "root",
            "scene root"
        };

        for (const char* pcPreferred : s_apcPreferredNames)
        {
            for (unsigned int b = 0; b < uiBoneCount; ++b)
            {
                const NiAVObject* pkBone = ppkBones[b];
                if (pkBone && ToLowerName(pkBone->GetName().c_str()) == pcPreferred)
                    return b;
            }
        }

        std::unordered_set<const NiAVObject*> kBoneSet;
        kBoneSet.reserve(uiBoneCount);
        for (unsigned int b = 0; b < uiBoneCount; ++b)
        {
            if (ppkBones[b])
                kBoneSet.insert(ppkBones[b]);
        }

        // Otherwise choose a top-level bone from this skin's own bone set.
        for (unsigned int b = 0; b < uiBoneCount; ++b)
        {
            const NiAVObject* pkBone = ppkBones[b];
            if (!pkBone)
                continue;

            const NiNode* pkParent = pkBone->GetParent();
            if (!pkParent || kBoneSet.find(pkParent) == kBoneSet.end())
                return b;
        }

        for (unsigned int b = 0; b < uiBoneCount; ++b)
        {
            if (ppkBones[b])
                return b;
        }

        return ~0u;
    }

    //--------------------------------------------------------------------------------------------------
    bool IsUsableBoneIndex(const IntermediateMesh& kMesh,
        unsigned int uiBoneIndex)
    {
        if (uiBoneIndex >= kMesh.boneNames.size())
            return false;

        if (!kMesh.boneValid.empty())
        {
            return uiBoneIndex < kMesh.boneValid.size() &&
                kMesh.boneValid[uiBoneIndex];
        }

        return kMesh.boneNames[uiBoneIndex].rfind("missing_bone_", 0) != 0;
    }

    //--------------------------------------------------------------------------------------------------
    NiTransform MakeIdentityTransform()
    {
        NiTransform kTransform;
        kTransform.MakeIdentity();
        return kTransform;
    }

    //--------------------------------------------------------------------------------------------------
    NiTransform ComputeWorldTransformFromParents(const NiAVObject* pkObject)
    {
        if (!pkObject)
            return MakeIdentityTransform();

        const NiNode* pkParent = pkObject->GetParent();
        if (!pkParent)
            return pkObject->GetLocalTransform();

        return ComputeWorldTransformFromParents(pkParent) *
            pkObject->GetLocalTransform();
    }


    //--------------------------------------------------------------------------------------------------
    NiAVObject* FindExactNodeByName(NiAVObject* pkObject, const char* pcExpectedName)
    {
        if (!pkObject || !pcExpectedName)
            return nullptr;

        if (ToLowerName(pkObject->GetName().c_str()) == ToLowerName(pcExpectedName))
            return pkObject;

        if (!NiIsKindOf(NiNode, pkObject))
            return nullptr;

        NiNode* pkNode = NiStaticCast(NiNode, pkObject);
        for (unsigned int i = 0; i < pkNode->GetArrayCount(); ++i)
        {
            NiAVObject* pkFound = FindExactNodeByName(pkNode->GetAt(i), pcExpectedName);
            if (pkFound)
                return pkFound;
        }

        return nullptr;
    }

    //--------------------------------------------------------------------------------------------------
    unsigned int CountGeometryDescendants(NiAVObject* pkObject)
    {
        if (!pkObject)
            return 0;

        unsigned int uiCount = 0;

        if (NiIsKindOf(NiMesh, pkObject) || NiIsKindOf(NiGeometry, pkObject))
            ++uiCount;

        if (NiIsKindOf(NiNode, pkObject))
        {
            NiNode* pkNode = NiStaticCast(NiNode, pkObject);
            for (unsigned int i = 0; i < pkNode->GetArrayCount(); ++i)
                uiCount += CountGeometryDescendants(pkNode->GetAt(i));
        }

        return uiCount;
    }

    //--------------------------------------------------------------------------------------------------
    NiAVObject* PickBestLODChild(NiNode* pkNode)
    {
        if (!pkNode)
            return nullptr;

        NiAVObject* pkRootChild = nullptr;
        NiAVObject* pkBestGeometryChild = nullptr;
        unsigned int uiBestGeometryCount = 0;

        for (unsigned int i = 0; i < pkNode->GetArrayCount(); ++i)
        {
            NiAVObject* pkChild = pkNode->GetAt(i);
            if (!pkChild)
                continue;

            const std::string kName = ToLowerName(pkChild->GetName().c_str());

            // Grand Fantasia convention used by several object models.
            if (kName == "lodobj01")
                return pkChild;

            // Grand Fantasia player-model convention: the full-quality set of
            // separately skinned body parts is stored below a child named ROOT.
            if (kName == "root")
                pkRootChild = pkChild;

            const unsigned int uiGeometryCount = CountGeometryDescendants(pkChild);
            if (uiGeometryCount > uiBestGeometryCount)
            {
                uiBestGeometryCount = uiGeometryCount;
                pkBestGeometryChild = pkChild;
            }
        }

        if (pkRootChild)
            return pkRootChild;

        // Unknown naming convention: prefer the child containing the largest
        // number of geometry descendants instead of blindly selecting index 0.
        if (pkBestGeometryChild)
            return pkBestGeometryChild;

        for (unsigned int i = 0; i < pkNode->GetArrayCount(); ++i)
        {
            NiAVObject* pkChild = pkNode->GetAt(i);
            if (pkChild)
                return pkChild;
        }

        return nullptr;
    }

    //--------------------------------------------------------------------------------------------------
    NiAVObject* PickSwitchChild(NiSwitchNode* pkSwitch)
    {
        if (!pkSwitch)
            return nullptr;

        NiAVObject* pkActive = pkSwitch->GetActiveChild();
        if (pkActive)
            return pkActive;

        for (unsigned int i = 0; i < pkSwitch->GetArrayCount(); ++i)
        {
            NiAVObject* pkChild = pkSwitch->GetAt(i);
            if (pkChild)
                return pkChild;
        }

        return nullptr;
    }

    //--------------------------------------------------------------------------------------------------
    bool ComputeModernSkinWorld(NiSkinningMeshModifier* pkSkin,
        NiTransform& kOutSkinWorld)
    {
        if (!pkSkin || !pkSkin->GetRootBoneParent())
            return false;

        NiTransform kSkinToRootParent;
        pkSkin->GetRootBoneParentToSkinTransform().Invert(kSkinToRootParent);
        kOutSkinWorld =
            ComputeWorldTransformFromParents(pkSkin->GetRootBoneParent()) *
            kSkinToRootParent;
        return true;
    }

    //--------------------------------------------------------------------------------------------------
    bool ComputeLegacySkinWorld(NiSkinInstance* pkSkin,
        NiTransform& kOutSkinWorld)
    {
        if (!pkSkin || !pkSkin->GetSkinData() || !pkSkin->GetRootParent())
            return false;

        NiTransform kSkinToRootParent;
        pkSkin->GetSkinData()->GetRootParentToSkin().Invert(kSkinToRootParent);
        kOutSkinWorld =
            ComputeWorldTransformFromParents(pkSkin->GetRootParent()) *
            kSkinToRootParent;
        return true;
    }

    //--------------------------------------------------------------------------------------------------
    bool IsFiniteTransform(const NiTransform& kTransform)
    {
        if (!std::isfinite(kTransform.m_fScale) ||
            !std::isfinite(kTransform.m_Translate.x) ||
            !std::isfinite(kTransform.m_Translate.y) ||
            !std::isfinite(kTransform.m_Translate.z))
        {
            return false;
        }

        for (unsigned int r = 0; r < 3; ++r)
        {
            for (unsigned int c = 0; c < 3; ++c)
            {
                if (!std::isfinite(kTransform.m_Rotate.GetEntry(r, c)))
                    return false;
            }
        }

        return true;
    }

    //--------------------------------------------------------------------------------------------------
    bool ComputeHierarchySkinToBone(const NiAVObject* pkBone,
        const NiTransform& kSkinWorld, NiTransform& kOutSkinToBone)
    {
        if (!pkBone)
            return false;

        const NiTransform kBoneWorld =
            ComputeWorldTransformFromParents(pkBone);
        NiTransform kWorldToBone;
        kBoneWorld.Invert(kWorldToBone);
        kOutSkinToBone = kWorldToBone * kSkinWorld;
        return true;
    }

    //--------------------------------------------------------------------------------------------------
    float MaxTransformDifference(const NiTransform& kA,
        const NiTransform& kB)
    {
        float fMax = std::abs(kA.m_fScale - kB.m_fScale);
        fMax = std::max(fMax, std::abs(kA.m_Translate.x - kB.m_Translate.x));
        fMax = std::max(fMax, std::abs(kA.m_Translate.y - kB.m_Translate.y));
        fMax = std::max(fMax, std::abs(kA.m_Translate.z - kB.m_Translate.z));
        for (unsigned int r = 0; r < 3; ++r)
        {
            for (unsigned int c = 0; c < 3; ++c)
            {
                fMax = std::max(fMax, std::abs(
                    kA.m_Rotate.GetEntry(r, c) -
                    kB.m_Rotate.GetEntry(r, c)));
            }
        }
        return fMax;
    }

    //--------------------------------------------------------------------------------------------------
    bool GetModernSkinNodeTransform(NiAVObject* pkObject, NiTransform& kOutLocal)
    {
        if (!pkObject || !NiIsKindOf(NiMesh, pkObject))
            return false;

        NiMesh* pkMesh = NiStaticCast(NiMesh, pkObject);
        NiSkinningMeshModifier* pkSkin = nullptr;
        for (NiUInt32 m = 0; m < pkMesh->GetModifierCount(); ++m)
        {
            NiMeshModifier* pkModifier = pkMesh->GetModifierAt(m);
            if (pkModifier && NiIsKindOf(NiSkinningMeshModifier, pkModifier))
            {
                pkSkin = NiStaticCast(NiSkinningMeshModifier, pkModifier);
                break;
            }
        }

        if (!pkSkin || !pkSkin->GetRootBoneParent())
            return false;

        // NiSkinningMeshModifier deforms bind vertices in skin space:
        //
        //   WorldToSkin * BoneWorld * SkinToBone * position
        //
        // Assimp evaluates a bone relative to the aiNode that owns the mesh.
        // Therefore the exported mesh node bind world transform must be the
        // same SkinWorld used to derive the bone bind transforms. If the raw
        // NiMesh local transform is kept instead, every bone has the same
        // residual MeshWorld^-1 * SkinWorld translation: the pose is shaped
        // correctly, but the complete character is displaced from (0,0,0).
        NiTransform kSkinWorld;
        if (!ComputeModernSkinWorld(pkSkin, kSkinWorld))
            return false;

        const NiNode* pkParent = pkObject->GetParent();
        if (!pkParent)
        {
            kOutLocal = kSkinWorld;
            return true;
        }

        const NiTransform kParentWorld =
            ComputeWorldTransformFromParents(pkParent);
        NiTransform kWorldToParent;
        kParentWorld.Invert(kWorldToParent);
        kOutLocal = kWorldToParent * kSkinWorld;

        const NiPoint3& kOriginalTranslate = pkObject->GetLocalTransform().m_Translate;
        const NiPoint3& kSkinTranslate = kOutLocal.m_Translate;
        std::cerr << "    Applied NiSkinningMeshModifier skin-space node transform to mesh '"
            << (pkMesh->GetName().c_str() ? pkMesh->GetName().c_str() : "<unnamed>")
            << "': original local translate=("
            << kOriginalTranslate.x << ", " << kOriginalTranslate.y << ", "
            << kOriginalTranslate.z << "), skin local translate=("
            << kSkinTranslate.x << ", " << kSkinTranslate.y << ", "
            << kSkinTranslate.z << ")." << std::endl;
        return true;
    }

    //--------------------------------------------------------------------------------------------------
    bool GetLegacySkinNodeTransform(NiAVObject* pkObject, NiTransform& kOutLocal)
    {
        if (!pkObject || !NiIsKindOf(NiGeometry, pkObject))
            return false;

        NiGeometry* pkGeom = NiStaticCast(NiGeometry, pkObject);
        NiSkinInstance* pkSkin = pkGeom->GetSkinInstance();
        if (!pkSkin || !pkSkin->GetSkinData() || !pkSkin->GetRootParent())
            return false;

        // For legacy NiSkinInstance, NiSkinData::RootParentToSkin defines the
        // mesh's skin space. The aiNode that owns the skinned mesh must use
        // that skin-space transform, otherwise aiBone::mOffsetMatrix
        // (SkinToBone) is evaluated against a different mesh bind space and
        // hands/feet can stretch during some KF/KFM clips.
        const NiNode* pkParent = pkObject->GetParent();

        NiTransform kSkinWorld;
        if (!ComputeLegacySkinWorld(pkSkin, kSkinWorld))
            return false;

        if (!pkParent)
        {
            kOutLocal = kSkinWorld;
            return true;
        }

        const NiTransform kParentWorld = ComputeWorldTransformFromParents(pkParent);
        NiTransform kWorldToParent;
        kParentWorld.Invert(kWorldToParent);
        kOutLocal = kWorldToParent * kSkinWorld;
        return true;
    }

	using Index4 = std::array<unsigned int, 4>;
	using Weight4 = std::array<float, 4>;

	struct Float2
	{
		float v[2];
	};

	struct Float3
	{
		float v[3];
	};

	struct Float4
	{
		float v[4];
	};

	struct Half2
	{
		NiFloat16 v[2];
	};

	struct Half3
	{
		NiFloat16 v[3];
	};

	struct Half4
	{
		NiFloat16 v[4];
	};

	struct UInt8x4
	{
		NiUInt8 v[4];
	};

	struct Int8x4
	{
		NiInt8 v[4];
	};

	struct UInt16x4
	{
		NiUInt16 v[4];
	};

	struct Int16x4
	{
		NiInt16 v[4];
	};



	//--------------------------------------------------------------------------------------------------
	bool ReadVec3Stream(NiMesh* pkMesh, const NiFixedString& kSemantic,
		unsigned int uiSubmesh, std::vector<aiVector3D>& kOut)
	{
		kOut.clear();

		NiDataStreamElementLock kLock(pkMesh, kSemantic, 0,
			NiDataStreamElement::F_UNKNOWN, NiDataStream::LOCK_READ | NiDataStream::LOCK_TOOL_READ);
		if (!kLock.IsLocked() || uiSubmesh >= kLock.GetSubmeshCount())
			return false;

		const NiUInt32 uiCount = kLock.count(uiSubmesh);
		kOut.reserve(uiCount);

		switch (kLock.GetDataStreamElement().GetFormat())
		{
		case NiDataStreamElement::F_FLOAT32_3:
		{
			auto it = kLock.begin<Float3>(uiSubmesh);
			auto itEnd = kLock.end<Float3>(uiSubmesh);
			for (; it != itEnd; ++it)
				kOut.emplace_back(it->v[0], it->v[1], it->v[2]);
			break;
		}
		case NiDataStreamElement::F_FLOAT32_4:
		{
			auto it = kLock.begin<Float4>(uiSubmesh);
			auto itEnd = kLock.end<Float4>(uiSubmesh);
			for (; it != itEnd; ++it)
				kOut.emplace_back(it->v[0], it->v[1], it->v[2]);
			break;
		}
		case NiDataStreamElement::F_FLOAT16_3:
		{
			auto it = kLock.begin<Half3>(uiSubmesh);
			auto itEnd = kLock.end<Half3>(uiSubmesh);
			for (; it != itEnd; ++it)
				kOut.emplace_back((float)it->v[0], (float)it->v[1], (float)it->v[2]);
			break;
		}
		case NiDataStreamElement::F_FLOAT16_4:
		{
			auto it = kLock.begin<Half4>(uiSubmesh);
			auto itEnd = kLock.end<Half4>(uiSubmesh);
			for (; it != itEnd; ++it)
				kOut.emplace_back((float)it->v[0], (float)it->v[1], (float)it->v[2]);
			break;
		}
		default:
			return false;
		}

		return kOut.size() == uiCount;
	}

	//--------------------------------------------------------------------------------------------------
	bool NormalizeNormal(aiVector3D& kNormal)
	{
		if (!std::isfinite(kNormal.x) || !std::isfinite(kNormal.y) ||
			!std::isfinite(kNormal.z))
		{
			return false;
		}

		const float fLengthSquared = kNormal.x * kNormal.x +
			kNormal.y * kNormal.y + kNormal.z * kNormal.z;
		if (!std::isfinite(fLengthSquared) || fLengthSquared <= 1.0e-20f)
			return false;

		const float fInvLength = 1.0f / std::sqrt(fLengthSquared);
		kNormal.x *= fInvLength;
		kNormal.y *= fInvLength;
		kNormal.z *= fInvLength;
		return true;
	}

	//--------------------------------------------------------------------------------------------------
	aiVector3D MakeFaceNormal(const aiVector3D& kP0,
		const aiVector3D& kP1, const aiVector3D& kP2)
	{
		const float fE1X = kP1.x - kP0.x;
		const float fE1Y = kP1.y - kP0.y;
		const float fE1Z = kP1.z - kP0.z;
		const float fE2X = kP2.x - kP0.x;
		const float fE2Y = kP2.y - kP0.y;
		const float fE2Z = kP2.z - kP0.z;
		return aiVector3D(
			fE1Y * fE2Z - fE1Z * fE2Y,
			fE1Z * fE2X - fE1X * fE2Z,
			fE1X * fE2Y - fE1Y * fE2X);
	}

	//--------------------------------------------------------------------------------------------------
	void EnsureCompleteNormals(IntermediateMesh& kMesh)
	{
		const std::size_t stVertexCount = kMesh.positions.size();
		if (stVertexCount == 0)
		{
			kMesh.normals.clear();
			return;
		}

		const bool bHasCompleteSource =
			kMesh.normals.size() == stVertexCount;
		std::vector<aiVector3D> kSourceNormals;
		if (bHasCompleteSource)
			kSourceNormals = kMesh.normals;

		std::vector<aiVector3D> kGenerated(stVertexCount,
			aiVector3D(0.0f, 0.0f, 0.0f));
		unsigned int uiValidFaces = 0;
		for (std::size_t i = 0; i + 2u < kMesh.indices.size(); i += 3u)
		{
			const unsigned int i0 = kMesh.indices[i + 0u];
			const unsigned int i1 = kMesh.indices[i + 1u];
			const unsigned int i2 = kMesh.indices[i + 2u];
			if (i0 >= stVertexCount || i1 >= stVertexCount ||
				i2 >= stVertexCount || i0 == i1 || i0 == i2 || i1 == i2)
			{
				continue;
			}

			const aiVector3D kFaceNormal = MakeFaceNormal(
				kMesh.positions[i0], kMesh.positions[i1], kMesh.positions[i2]);
			aiVector3D kUnitNormal = kFaceNormal;
			if (!NormalizeNormal(kUnitNormal))
				continue;

			// Accumulate the unnormalized cross product so larger triangles have
			// proportionally more influence on the generated smooth normal.
			for (unsigned int uiVertex : { i0, i1, i2 })
			{
				kGenerated[uiVertex].x += kFaceNormal.x;
				kGenerated[uiVertex].y += kFaceNormal.y;
				kGenerated[uiVertex].z += kFaceNormal.z;
			}
			++uiValidFaces;
		}

		kMesh.normals.resize(stVertexCount);
		unsigned int uiGeneratedVertices = 0;
		unsigned int uiFallbackVertices = 0;
		for (std::size_t v = 0; v < stVertexCount; ++v)
		{
			aiVector3D kNormal;
			bool bValid = false;
			if (bHasCompleteSource)
			{
				kNormal = kSourceNormals[v];
				bValid = NormalizeNormal(kNormal);
			}

			if (!bValid)
			{
				kNormal = kGenerated[v];
				bValid = NormalizeNormal(kNormal);
				if (bValid)
					++uiGeneratedVertices;
			}

			if (!bValid)
			{
				kNormal = aiVector3D(0.0f, 0.0f, 1.0f);
				++uiFallbackVertices;
			}
			kMesh.normals[v] = kNormal;
		}

		if (!bHasCompleteSource || uiGeneratedVertices != 0u ||
			uiFallbackVertices != 0u)
		{
			std::cerr << "    Completed normals for '" << kMesh.name
				<< "': source="
				<< (bHasCompleteSource ? "partial/invalid" : "missing")
				<< " generatedVertices=" << uiGeneratedVertices
				<< " fallbackVertices=" << uiFallbackVertices
				<< " validFaces=" << uiValidFaces << std::endl;
		}
	}

	//--------------------------------------------------------------------------------------------------
	bool ReadVec2Stream(NiMesh* pkMesh, const NiFixedString& kSemantic,
		unsigned int uiSemanticIndex, unsigned int uiSubmesh,
		std::vector<aiVector2D>& kOut)
	{
		kOut.clear();

		NiDataStreamElementLock kLock(pkMesh, kSemantic, uiSemanticIndex,
			NiDataStreamElement::F_UNKNOWN, NiDataStream::LOCK_READ | NiDataStream::LOCK_TOOL_READ);
		if (!kLock.IsLocked() || uiSubmesh >= kLock.GetSubmeshCount())
			return false;

		const NiUInt32 uiCount = kLock.count(uiSubmesh);
		kOut.reserve(uiCount);

		switch (kLock.GetDataStreamElement().GetFormat())
		{
		case NiDataStreamElement::F_FLOAT32_2:
		{
			auto it = kLock.begin<Float2>(uiSubmesh);
			auto itEnd = kLock.end<Float2>(uiSubmesh);
			for (; it != itEnd; ++it)
				kOut.emplace_back(it->v[0], it->v[1]);
			break;
		}
		case NiDataStreamElement::F_FLOAT16_2:
		{
			auto it = kLock.begin<Half2>(uiSubmesh);
			auto itEnd = kLock.end<Half2>(uiSubmesh);
			for (; it != itEnd; ++it)
				kOut.emplace_back((float)it->v[0], (float)it->v[1]);
			break;
		}
		default:
			return false;
		}

		return kOut.size() == uiCount;
	}

	//--------------------------------------------------------------------------------------------------
	void ApplyTextureUvConversion(std::vector<aiVector2D>& kUvs,
		const NiTextureTransform* pkTransform)
	{
		const NiMatrix3* pkMatrix = pkTransform ? pkTransform->GetMatrix() : nullptr;

		for (aiVector2D& kUv : kUvs)
		{
			float fU = kUv.x;
			float fV = kUv.y;

			// Apply the Gamebryo texture transform in the source UV space
			// before converting the V-axis convention.
			if (pkMatrix)
			{
				const float fOutU =
					pkMatrix->GetEntry(0, 0) * fU +
					pkMatrix->GetEntry(0, 1) * fV +
					pkMatrix->GetEntry(0, 2);
				const float fOutV =
					pkMatrix->GetEntry(1, 0) * fU +
					pkMatrix->GetEntry(1, 1) * fV +
					pkMatrix->GetEntry(1, 2);
				fU = fOutU;
				fV = fOutV;
			}


			kUv.x = fU;
			kUv.y = fV;
		}
	}

	//--------------------------------------------------------------------------------------------------
	void LogUvRange(const std::vector<aiVector2D>& kUvs, unsigned int uiSet,
		bool bHasTransform)
	{
		if (kUvs.empty())
			return;

		float fMinU = std::numeric_limits<float>::max();
		float fMinV = std::numeric_limits<float>::max();
		float fMaxU = std::numeric_limits<float>::lowest();
		float fMaxV = std::numeric_limits<float>::lowest();
		for (const aiVector2D& kUv : kUvs)
		{
			fMinU = std::min(fMinU, kUv.x);
			fMinV = std::min(fMinV, kUv.y);
			fMaxU = std::max(fMaxU, kUv.x);
			fMaxV = std::max(fMaxV, kUv.y);
		}

		std::cerr << "      UV set " << uiSet
			<< ", range U=[" << fMinU << ", " << fMaxU
			<< "] V=[" << fMinV << ", " << fMaxV << "]"
			<< ", transform=" << (bHasTransform ? "yes" : "no")
			<< ", V-flip=Assimp aiProcess_FlipUVs" << std::endl;
	}

	//--------------------------------------------------------------------------------------------------
	bool ReadBlendIndices(NiMesh* pkMesh, unsigned int uiSubmesh,
		unsigned int uiVertexCount, std::vector<Index4>& kOut)
	{
		kOut.assign(uiVertexCount, Index4{0, 0, 0, 0});

		NiDataStreamElementLock kLock(pkMesh, NiCommonSemantics::BLENDINDICES(), 0,
			NiDataStreamElement::F_UNKNOWN, NiDataStream::LOCK_READ | NiDataStream::LOCK_TOOL_READ);
		if (!kLock.IsLocked() || uiSubmesh >= kLock.GetSubmeshCount())
		{
			std::cerr << "      Missing BLENDINDICES stream for submesh "
				<< uiSubmesh << "." << std::endl;
			return false;
		}

		const unsigned int uiCount = kLock.count(uiSubmesh);
		const NiDataStreamElement& kElement = kLock.GetDataStreamElement();
		if (uiCount != uiVertexCount)
		{
			std::cerr << "      Invalid BLENDINDICES stream for submesh "
				<< uiSubmesh << ": format=" << static_cast<const char*>(kElement.GetFormatString())
				<< " streamVertices=" << uiCount
				<< " positionVertices=" << uiVertexCount
				<< ". The complete submesh will use rigid fallback skinning."
				<< std::endl;
			return false;
		}

		switch (kElement.GetFormat())
		{
		case NiDataStreamElement::F_UINT8_4:
		case NiDataStreamElement::F_NORMUINT8_4:
		{
			auto it = kLock.begin<UInt8x4>(uiSubmesh);
			for (unsigned int i = 0; i < uiCount; ++i, ++it)
				for (unsigned int k = 0; k < 4; ++k)
					kOut[i][k] = it->v[k];
			return true;
		}
		case NiDataStreamElement::F_NORMUINT8_4_BGRA:
		{
			auto it = kLock.begin<UInt8x4>(uiSubmesh);
			for (unsigned int i = 0; i < uiCount; ++i, ++it)
			{
				kOut[i][0] = it->v[2];
				kOut[i][1] = it->v[1];
				kOut[i][2] = it->v[0];
				kOut[i][3] = it->v[3];
			}
			return true;
		}
		case NiDataStreamElement::F_INT8_4:
		case NiDataStreamElement::F_NORMINT8_4:
		{
			auto it = kLock.begin<Int8x4>(uiSubmesh);
			for (unsigned int i = 0; i < uiCount; ++i, ++it)
				for (unsigned int k = 0; k < 4; ++k)
					kOut[i][k] = static_cast<unsigned int>(std::max<int>(0, it->v[k]));
			return true;
		}
		case NiDataStreamElement::F_UINT16_4:
		{
			auto it = kLock.begin<UInt16x4>(uiSubmesh);
			for (unsigned int i = 0; i < uiCount; ++i, ++it)
				for (unsigned int k = 0; k < 4; ++k)
					kOut[i][k] = it->v[k];
			return true;
		}
		case NiDataStreamElement::F_INT16_4:
		{
			auto it = kLock.begin<Int16x4>(uiSubmesh);
			for (unsigned int i = 0; i < uiCount; ++i, ++it)
				for (unsigned int k = 0; k < 4; ++k)
					kOut[i][k] = static_cast<unsigned int>(std::max<int>(0, it->v[k]));
			return true;
		}
		default:
			std::cerr << "      Unsupported BLENDINDICES format "
				<< static_cast<const char*>(kElement.GetFormatString()) << " for submesh "
				<< uiSubmesh << "." << std::endl;
			return false;
		}
	}

	//--------------------------------------------------------------------------------------------------
	bool ReadBlendWeights(NiMesh* pkMesh, unsigned int uiSubmesh,
		unsigned int uiVertexCount, std::vector<Weight4>& kOut)
	{
		kOut.assign(uiVertexCount, Weight4{0.0f, 0.0f, 0.0f, 0.0f});

		NiDataStreamElementLock kLock(pkMesh, NiCommonSemantics::BLENDWEIGHT(), 0,
			NiDataStreamElement::F_UNKNOWN, NiDataStream::LOCK_READ | NiDataStream::LOCK_TOOL_READ);
		if (!kLock.IsLocked() || uiSubmesh >= kLock.GetSubmeshCount())
		{
			std::cerr << "      Missing BLENDWEIGHT stream for submesh "
				<< uiSubmesh << "." << std::endl;
			return false;
		}

		const unsigned int uiCount = kLock.count(uiSubmesh);
		const NiDataStreamElement& kElement = kLock.GetDataStreamElement();
		if (uiCount != uiVertexCount)
		{
			std::cerr << "      Invalid BLENDWEIGHT stream for submesh "
				<< uiSubmesh << ": format=" << static_cast<const char*>(kElement.GetFormatString())
				<< " streamVertices=" << uiCount
				<< " positionVertices=" << uiVertexCount
				<< ". The complete submesh will use rigid fallback skinning."
				<< std::endl;
			return false;
		}

		switch (kElement.GetFormat())
		{
		case NiDataStreamElement::F_FLOAT32_3:
		{
			auto it = kLock.begin<Float3>(uiSubmesh);
			for (unsigned int i = 0; i < uiCount; ++i, ++it)
			{
				kOut[i][0] = it->v[0];
				kOut[i][1] = it->v[1];
				kOut[i][2] = it->v[2];
				kOut[i][3] = std::max(0.0f, 1.0f - it->v[0] - it->v[1] - it->v[2]);
			}
			break;
		}
		case NiDataStreamElement::F_FLOAT32_4:
		{
			auto it = kLock.begin<Float4>(uiSubmesh);
			for (unsigned int i = 0; i < uiCount; ++i, ++it)
				for (unsigned int k = 0; k < 4; ++k)
					kOut[i][k] = it->v[k];
			break;
		}
		case NiDataStreamElement::F_FLOAT16_3:
		{
			auto it = kLock.begin<Half3>(uiSubmesh);
			for (unsigned int i = 0; i < uiCount; ++i, ++it)
			{
				kOut[i][0] = (float)it->v[0];
				kOut[i][1] = (float)it->v[1];
				kOut[i][2] = (float)it->v[2];
				kOut[i][3] = std::max(0.0f,
					1.0f - kOut[i][0] - kOut[i][1] - kOut[i][2]);
			}
			break;
		}
		case NiDataStreamElement::F_FLOAT16_4:
		{
			auto it = kLock.begin<Half4>(uiSubmesh);
			for (unsigned int i = 0; i < uiCount; ++i, ++it)
				for (unsigned int k = 0; k < 4; ++k)
					kOut[i][k] = (float)it->v[k];
			break;
		}
		case NiDataStreamElement::F_NORMUINT8_4:
		{
			auto it = kLock.begin<UInt8x4>(uiSubmesh);
			for (unsigned int i = 0; i < uiCount; ++i, ++it)
				for (unsigned int k = 0; k < 4; ++k)
					kOut[i][k] = it->v[k] / 255.0f;
			break;
		}
		case NiDataStreamElement::F_NORMUINT8_4_BGRA:
		{
			auto it = kLock.begin<UInt8x4>(uiSubmesh);
			for (unsigned int i = 0; i < uiCount; ++i, ++it)
			{
				kOut[i][0] = it->v[2] / 255.0f;
				kOut[i][1] = it->v[1] / 255.0f;
				kOut[i][2] = it->v[0] / 255.0f;
				kOut[i][3] = it->v[3] / 255.0f;
			}
			break;
		}
		case NiDataStreamElement::F_NORMUINT16_4:
		{
			auto it = kLock.begin<UInt16x4>(uiSubmesh);
			for (unsigned int i = 0; i < uiCount; ++i, ++it)
				for (unsigned int k = 0; k < 4; ++k)
					kOut[i][k] = it->v[k] / 65535.0f;
			break;
		}
		default:
			std::cerr << "      Unsupported BLENDWEIGHT format "
				<< static_cast<const char*>(kElement.GetFormatString()) << " for submesh "
				<< uiSubmesh << "." << std::endl;
			return false;
		}

		for (Weight4& kWeights : kOut)
		{
			float fTotal = 0.0f;
			for (float& fWeight : kWeights)
			{
				fWeight = std::max(0.0f, fWeight);
				fTotal += fWeight;
			}

			if (fTotal > 0.000001f)
			{
				const float fInvTotal = 1.0f / fTotal;
				for (float& fWeight : kWeights)
					fWeight *= fInvTotal;
			}
		}

		return true;
	}

	//--------------------------------------------------------------------------------------------------
	bool ReadBonePalette(NiMesh* pkMesh, unsigned int uiSubmesh,
		std::vector<unsigned int>& kOut)
	{
		kOut.clear();

		NiDataStreamElementLock kLock(pkMesh, NiCommonSemantics::BONE_PALETTE(), 0,
			NiDataStreamElement::F_UNKNOWN, NiDataStream::LOCK_READ | NiDataStream::LOCK_TOOL_READ);
		if (!kLock.IsLocked() || uiSubmesh >= kLock.GetSubmeshCount())
			return false;

		const unsigned int uiCount = kLock.count(uiSubmesh);
		kOut.reserve(uiCount);

		switch (kLock.GetDataStreamElement().GetFormat())
		{
		case NiDataStreamElement::F_UINT8_1:
		{
			auto it = kLock.begin<NiUInt8>(uiSubmesh);
			auto itEnd = kLock.end<NiUInt8>(uiSubmesh);
			for (; it != itEnd; ++it)
				kOut.push_back(*it);
			break;
		}
		case NiDataStreamElement::F_UINT16_1:
		{
			auto it = kLock.begin<NiUInt16>(uiSubmesh);
			auto itEnd = kLock.end<NiUInt16>(uiSubmesh);
			for (; it != itEnd; ++it)
				kOut.push_back(*it);
			break;
		}
		case NiDataStreamElement::F_UINT32_1:
		{
			auto it = kLock.begin<NiUInt32>(uiSubmesh);
			auto itEnd = kLock.end<NiUInt32>(uiSubmesh);
			for (; it != itEnd; ++it)
				kOut.push_back(*it);
			break;
		}
		default:
			std::cerr << "      Unsupported BONE_PALETTE format "
				<< static_cast<const char*>(
					kLock.GetDataStreamElement().GetFormatString())
				<< " for submesh " << uiSubmesh << "." << std::endl;
			return false;
		}

		return !kOut.empty();
	}

	//--------------------------------------------------------------------------------------------------
	bool ReadTriangleIndices(NiMesh* pkMesh, unsigned int uiSubmesh,
		unsigned int uiVertexCount, std::vector<unsigned int>& kOut)
	{
		kOut.clear();
		if (!pkMesh || uiVertexCount == 0)
			return false;

		std::vector<unsigned int> kRawIndices;
		unsigned int uiRestartIndex = 0xFFFFFFFFu;

		NiDataStreamElementLock kIndexLock(pkMesh, NiCommonSemantics::INDEX(), 0,
			NiDataStreamElement::F_UNKNOWN,
			NiDataStream::LOCK_READ | NiDataStream::LOCK_TOOL_READ);

		if (kIndexLock.IsLocked())
		{
			if (uiSubmesh >= kIndexLock.GetSubmeshCount())
				return false;

			NiDataStreamRef* pkIndexRef = kIndexLock.GetDataStreamRef();
			NiDataStream* pkIndexStream = kIndexLock.GetDataStream();
			if (!pkIndexRef || !pkIndexStream)
				return false;

			// NiDataStreamElementLock reports the mesh submesh count, not the
			// number of mappings actually stored on this particular stream ref.
			// Validate the remap before calling GetRegionForSubmesh()/count().
			if (uiSubmesh >= pkIndexRef->GetSubmeshRemapCount())
			{
				std::cerr << "    Submesh " << uiSubmesh
					<< " has no index-stream region mapping." << std::endl;
				return false;
			}

			const unsigned int uiRegionIndex =
				pkIndexRef->GetRegionIndexForSubmesh(uiSubmesh);
			if (uiRegionIndex >= pkIndexStream->GetRegionCount())
			{
				std::cerr << "    Submesh " << uiSubmesh
					<< " maps to invalid index region " << uiRegionIndex
					<< " (region count=" << pkIndexStream->GetRegionCount()
					<< ")." << std::endl;
				return false;
			}

			const NiDataStreamElement& kElement =
				kIndexLock.GetDataStreamElement();
			const NiDataStream::Region& kRegion =
				pkIndexStream->GetRegion(uiRegionIndex);

			const unsigned int uiIndexCount = kRegion.GetRange();
			const unsigned int uiStride = pkIndexRef->GetStride();
			const unsigned int uiElementOffset = kElement.GetOffset();
			const size_t stElementSize = kElement.SizeOf();
			const size_t stStreamSize = pkIndexRef->GetSize();

			// NiDataStreamPrimitiveLock builds its end iterator from the raw region
			// end. For a triangle list whose region count is not exactly divisible
			// by three, incrementing by three can never equal that end iterator and
			// eventually reads past the buffer. It also assumes a tightly packed
			// index element. Validate the metadata and read a bounded number of
			// elements through the stride-aware element iterator instead.
			const std::uint64_t uiRegionStart =
				static_cast<std::uint64_t>(kRegion.GetStartIndex()) * uiStride;
			const std::uint64_t uiLastElementEnd = uiIndexCount == 0 ? uiRegionStart :
				uiRegionStart + static_cast<std::uint64_t>(uiIndexCount - 1) * uiStride +
				uiElementOffset + stElementSize;

			if (uiIndexCount == 0 || uiStride == 0 || stElementSize == 0 ||
				uiElementOffset + stElementSize > uiStride ||
				uiLastElementEnd > stStreamSize)
			{
				std::cerr << "    Submesh " << uiSubmesh
					<< " has an invalid index region: count=" << uiIndexCount
					<< " start=" << kRegion.GetStartIndex()
					<< " stride=" << uiStride
					<< " elementOffset=" << uiElementOffset
					<< " elementSize=" << stElementSize
					<< " streamSize=" << stStreamSize << std::endl;
				return false;
			}

			kRawIndices.reserve(uiIndexCount);
			switch (kElement.GetFormat())
			{
			case NiDataStreamElement::F_UINT8_1:
			{
				uiRestartIndex = 0xFFu;
				auto it = kIndexLock.begin<NiUInt8>(uiSubmesh);
				for (unsigned int i = 0; i < uiIndexCount; ++i, ++it)
					kRawIndices.push_back(static_cast<unsigned int>(*it));
				break;
			}
			case NiDataStreamElement::F_UINT16_1:
			{
				uiRestartIndex = 0xFFFFu;
				auto it = kIndexLock.begin<NiUInt16>(uiSubmesh);
				for (unsigned int i = 0; i < uiIndexCount; ++i, ++it)
					kRawIndices.push_back(static_cast<unsigned int>(*it));
				break;
			}
			case NiDataStreamElement::F_UINT32_1:
			{
				uiRestartIndex = 0xFFFFFFFFu;
				auto it = kIndexLock.begin<NiUInt32>(uiSubmesh);
				for (unsigned int i = 0; i < uiIndexCount; ++i, ++it)
					kRawIndices.push_back(static_cast<unsigned int>(*it));
				break;
			}
			default:
				std::cerr << "    Submesh " << uiSubmesh
					<< " uses unsupported index format "
					<< static_cast<unsigned int>(kElement.GetFormat()) << std::endl;
				return false;
			}

			std::cerr << "    Submesh " << uiSubmesh
				<< " index stream: count=" << uiIndexCount
				<< " stride=" << uiStride
				<< " elementSize=" << stElementSize << std::endl;
		}
		else
		{
			// Non-indexed NiMesh: the vertex region is the primitive stream.
			kRawIndices.resize(uiVertexCount);
			for (unsigned int i = 0; i < uiVertexCount; ++i)
				kRawIndices[i] = i;
		}

		auto AppendTriangle = [&](unsigned int i0, unsigned int i1,
			unsigned int i2)
		{
			if (i0 == uiRestartIndex || i1 == uiRestartIndex ||
				i2 == uiRestartIndex)
			{
				return;
			}
			if (i0 >= uiVertexCount || i1 >= uiVertexCount ||
				i2 >= uiVertexCount)
			{
				return;
			}
			if (i0 == i1 || i0 == i2 || i1 == i2)
				return;

			kOut.push_back(i0);
			kOut.push_back(i1);
			kOut.push_back(i2);
		};

		switch (pkMesh->GetPrimitiveType())
		{
		case NiPrimitiveType::PRIMITIVE_TRIANGLES:
			if ((kRawIndices.size() % 3) != 0)
			{
				std::cerr << "    Submesh " << uiSubmesh << " has "
					<< (kRawIndices.size() % 3)
					<< " trailing triangle-list index value(s); ignoring them."
					<< std::endl;
			}
			for (size_t i = 0; i + 2 < kRawIndices.size(); i += 3)
				AppendTriangle(kRawIndices[i], kRawIndices[i + 1],
					kRawIndices[i + 2]);
			break;

		case NiPrimitiveType::PRIMITIVE_TRISTRIPS:
		{
			unsigned int uiStripVertexCount = 0;
			unsigned int i0 = 0;
			unsigned int i1 = 0;
			bool bOddTriangle = false;

			for (unsigned int i2 : kRawIndices)
			{
				if (i2 == uiRestartIndex)
				{
					uiStripVertexCount = 0;
					bOddTriangle = false;
					continue;
				}

				if (uiStripVertexCount == 0)
				{
					i0 = i2;
					uiStripVertexCount = 1;
					continue;
				}
				if (uiStripVertexCount == 1)
				{
					i1 = i2;
					uiStripVertexCount = 2;
					continue;
				}

				if (bOddTriangle)
					AppendTriangle(i1, i0, i2);
				else
					AppendTriangle(i0, i1, i2);

				// Degenerate triangles still consume a strip step and therefore
				// still flip the winding of the following primitive.
				bOddTriangle = !bOddTriangle;
				i0 = i1;
				i1 = i2;
				++uiStripVertexCount;
			}
			break;
		}

		default:
			return false;
		}

		return !kOut.empty();
	}

	//--------------------------------------------------------------------------------------------------
	bool HasValidBoneWeights(const IntermediateMesh& kMesh, unsigned int uiVertex)
	{
		if (uiVertex >= kMesh.boneWeights.size() || kMesh.boneNames.empty())
			return false;

		for (const VertexBoneWeight& kWeight : kMesh.boneWeights[uiVertex])
		{
			if (kWeight.weight > 0.0f &&
				IsUsableBoneIndex(kMesh, kWeight.boneIndex))
			{
				return true;
			}
		}

		return false;
	}

	//--------------------------------------------------------------------------------------------------
	unsigned int CountUnweightedVertices(const IntermediateMesh& kMesh)
	{
		if (kMesh.boneWeights.size() != kMesh.positions.size() ||
			kMesh.boneNames.empty())
		{
			return static_cast<unsigned int>(kMesh.positions.size());
		}

		unsigned int uiMissing = 0;
		for (unsigned int v = 0; v < kMesh.positions.size(); ++v)
		{
			if (!HasValidBoneWeights(kMesh, v))
				++uiMissing;
		}

		return uiMissing;
	}

	//--------------------------------------------------------------------------------------------------
	void NormalizeLegacyWeights(IntermediateMesh& kMesh)
	{
		if (kMesh.boneWeights.size() != kMesh.positions.size() ||
			kMesh.boneNames.empty())
		{
			return;
		}

		const bool bHasFallback =
			IsUsableBoneIndex(kMesh, kMesh.fallbackBoneIndex);
		unsigned int uiFallbackVertices = 0;
		unsigned int uiInvalidInfluences = 0;

		for (std::vector<VertexBoneWeight>& kWeights : kMesh.boneWeights)
		{
			const size_t stBeforeFilter = kWeights.size();
			kWeights.erase(std::remove_if(kWeights.begin(), kWeights.end(),
				[&](const VertexBoneWeight& kWeight)
				{
					return !IsUsableBoneIndex(kMesh, kWeight.boneIndex) ||
						!std::isfinite(kWeight.weight) ||
						kWeight.weight <= 0.0f;
				}), kWeights.end());
			uiInvalidInfluences += static_cast<unsigned int>(
				stBeforeFilter - kWeights.size());

			// A partition palette can legally map more than one local slot to
			// the same global bone. Merge those entries before enforcing the
			// four-influence limit so a duplicate does not consume a slot.
			std::vector<VertexBoneWeight> kMergedWeights;
			for (const VertexBoneWeight& kWeight : kWeights)
			{
				auto kExisting = std::find_if(kMergedWeights.begin(),
				kMergedWeights.end(), [&](const VertexBoneWeight& kMerged)
				{
					return kMerged.boneIndex == kWeight.boneIndex;
				});
				if (kExisting != kMergedWeights.end())
					kExisting->weight += kWeight.weight;
				else
					kMergedWeights.push_back(kWeight);
			}
			kWeights.swap(kMergedWeights);

			std::stable_sort(kWeights.begin(), kWeights.end(),
				[](const VertexBoneWeight& kLeft, const VertexBoneWeight& kRight)
				{
					return kLeft.weight > kRight.weight;
				});
			if (kWeights.size() > 4)
				kWeights.resize(4);

			float fTotal = 0.0f;
			for (const VertexBoneWeight& kWeight : kWeights)
				fTotal += kWeight.weight;

			if (!std::isfinite(fTotal) || fTotal <= 0.000001f)
			{
				kWeights.clear();
				if (bHasFallback)
				{
					kWeights.push_back({kMesh.fallbackBoneIndex, 1.0f});
					++uiFallbackVertices;
				}
				continue;
			}

			const float fInvTotal = 1.0f / fTotal;
			for (VertexBoneWeight& kWeight : kWeights)
				kWeight.weight *= fInvTotal;
		}

		if (uiInvalidInfluences > 0)
		{
			std::cerr << "    Warning: discarded " << uiInvalidInfluences
				<< " invalid/missing-bone influence(s) in mesh '"
				<< kMesh.name << "'." << std::endl;
		}

		if (uiFallbackVertices > 0)
		{
			std::cerr << "    Assigned " << uiFallbackVertices
				<< " unweighted vertices to real fallback bone '"
				<< kMesh.boneNames[kMesh.fallbackBoneIndex] << "' in mesh '"
				<< kMesh.name << "'." << std::endl;
		}
		else if (!bHasFallback && CountUnweightedVertices(kMesh) > 0)
		{
			std::cerr << "    Error: mesh '" << kMesh.name
				<< "' has unweighted vertices but no real exported bone is "
				<< "available for fallback skinning." << std::endl;
		}
	}

}

//--------------------------------------------------------------------------------------------------
MeshExtractor::MeshExtractor(const std::string& kTextureOutputFolder,
	bool bConvertTexturesToPng, float fTransformUnitScale,
	ExportAxisPreset eAxisPreset)
	: m_kTextureOutputFolder(kTextureOutputFolder)
	, m_bConvertTexturesToPng(bConvertTexturesToPng)
	, m_fTransformUnitScale(fTransformUnitScale)
	, m_eAxisPreset(eAxisPreset)
{
}

//--------------------------------------------------------------------------------------------------
void MeshExtractor::Extract(NiAVObject* pkRoot,
	std::vector<IntermediateMesh>& kMeshes,
	std::vector<IntermediateMaterial>& kMaterials,
	NodeIndexMap& kNodeIndexMap) const
{
	if (!pkRoot)
		return;

	TraverseNode(pkRoot, kMeshes, kMaterials, kNodeIndexMap);
}

//--------------------------------------------------------------------------------------------------
void MeshExtractor::TraverseNode(NiAVObject* pkObject,
	std::vector<IntermediateMesh>& kMeshes,
	std::vector<IntermediateMaterial>& kMaterials,
	NodeIndexMap& kNodeIndexMap) const
{
	if (!pkObject)
		return;

	const unsigned int uiIndex = static_cast<unsigned int>(kNodeIndexMap.size());
	kNodeIndexMap[pkObject] = uiIndex;

	const char* pcObjName = pkObject->GetName().c_str();
	std::cerr << "  Visiting: " << (pcObjName ? pcObjName : "<unnamed>")
		<< " (" << pkObject->GetRTTI()->GetName() << ")" << std::endl;

	// NiLODNode/NiSwitchNode are selection nodes. Export only the chosen child;
	// otherwise all LOD meshes are written and Blender/Unreal imports several
	// overlapping copies of the same model. NiLODNode must be checked before
	// NiSwitchNode because it derives from NiSwitchNode.
	if (NiIsKindOf(NiLODNode, pkObject))
	{
		NiNode* pkLODNode = NiStaticCast(NiNode, pkObject);
		NiAVObject* pkBestChild = PickBestLODChild(pkLODNode);
		std::cerr << "    NiLODNode selected high-detail child: "
			<< (pkBestChild && pkBestChild->GetName().c_str()
				? pkBestChild->GetName().c_str() : "<none>")
			<< ", geometry descendants="
			<< CountGeometryDescendants(pkBestChild) << std::endl;

		if (pkBestChild)
			TraverseNode(pkBestChild, kMeshes, kMaterials, kNodeIndexMap);
		return;
	}

	if (NiIsKindOf(NiSwitchNode, pkObject))
	{
		NiSwitchNode* pkSwitch = NiStaticCast(NiSwitchNode, pkObject);
		NiAVObject* pkChild = PickSwitchChild(pkSwitch);
		std::cerr << "    NiSwitchNode selected child: "
			<< (pkChild && pkChild->GetName().c_str()
				? pkChild->GetName().c_str() : "<none>") << std::endl;

		if (pkChild)
			TraverseNode(pkChild, kMeshes, kMaterials, kNodeIndexMap);
		return;
	}

	if (NiIsKindOf(NiMesh, pkObject))
	{
		NiMesh* pkMesh = NiStaticCast(NiMesh, pkObject);
		const bool bOk = ExtractNiMesh(pkMesh, kMeshes, kMaterials);
		std::cerr << "    NiMesh extraction " << (bOk ? "succeeded" : "failed")
			<< std::endl;
	}
	else if (NiIsKindOf(NiGeometry, pkObject))
	{
		NiGeometry* pkGeom = NiStaticCast(NiGeometry, pkObject);
		const bool bOk = ExtractNiGeometry(pkGeom, kMeshes, kMaterials);
		std::cerr << "    NiGeometry extraction " << (bOk ? "succeeded" : "failed")
			<< std::endl;
	}

	if (NiIsKindOf(NiNode, pkObject))
	{
		NiNode* pkNode = NiStaticCast(NiNode, pkObject);
		std::cerr << "    Node child count: " << pkNode->GetArrayCount() << std::endl;
		for (unsigned int i = 0; i < pkNode->GetArrayCount(); ++i)
		{
			NiAVObject* pkChild = pkNode->GetAt(i);
			if (pkChild)
				TraverseNode(pkChild, kMeshes, kMaterials, kNodeIndexMap);
		}
	}
}

//--------------------------------------------------------------------------------------------------
std::string MeshExtractor::ResolveTexturePath(NiTexturingProperty* pkTexProp) const
{
	if (!pkTexProp)
		return std::string();

	NiTexture* pkTex = pkTexProp->GetBaseTexture();
	if (!pkTex)
		return std::string();

	if (NiIsKindOf(NiSourceTexture, pkTex))
	{
		NiSourceTexture* pkSrc = NiStaticCast(NiSourceTexture, pkTex);
		const NiFixedString& kFilename = pkSrc->GetFilename();
		if ((const char*)kFilename)
			return std::string((const char*)kFilename);
	}
	return std::string();
}

//--------------------------------------------------------------------------------------------------
unsigned int MeshExtractor::FindOrAddMaterial(const std::string& kTexturePath,
	const std::string& kMeshName,
	const NiAlphaProperty* pkAlphaProperty,
	const NiMaterialProperty* pkMaterialProperty,
	std::vector<IntermediateMaterial>& kMaterials) const
{
	IntermediateMaterial kCandidate;
	kCandidate.diffuseTexturePath = kTexturePath;
	kCandidate.opacity = pkMaterialProperty
		? std::clamp(pkMaterialProperty->GetAlpha(), 0.0f, 1.0f) : 1.0f;

	if (pkAlphaProperty)
	{
		kCandidate.alphaBlend = pkAlphaProperty->GetAlphaBlending();
		kCandidate.alphaTest = pkAlphaProperty->GetAlphaTesting() &&
			pkAlphaProperty->GetTestMode() != NiAlphaProperty::TEST_ALWAYS;
		kCandidate.alphaCutoff = static_cast<float>(pkAlphaProperty->GetTestRef()) / 255.0f;

		if (pkAlphaProperty->GetTestMode() == NiAlphaProperty::TEST_NEVER &&
			pkAlphaProperty->GetAlphaTesting())
		{
			kCandidate.opacity = 0.0f;
		}
	}

	// The black jagged polygons visible around fur/cards are transparent
	// texels whose alpha channel was never represented in the FBX material.
	// Only enable per-pixel transparency when the NIF explicitly asks for
	// blending/testing; opaque textures remain opaque materials.
	kCandidate.useTextureAlpha = !kTexturePath.empty() &&
		(kCandidate.alphaBlend || kCandidate.alphaTest);

	for (unsigned int i = 0; i < static_cast<unsigned int>(kMaterials.size()); ++i)
	{
		const IntermediateMaterial& kExisting = kMaterials[i];
		if (kExisting.diffuseTexturePath == kCandidate.diffuseTexturePath &&
			kExisting.useTextureAlpha == kCandidate.useTextureAlpha &&
			kExisting.alphaBlend == kCandidate.alphaBlend &&
			kExisting.alphaTest == kCandidate.alphaTest &&
			std::abs(kExisting.opacity - kCandidate.opacity) <= 1.0e-6f &&
			std::abs(kExisting.alphaCutoff - kCandidate.alphaCutoff) <= 1.0e-6f)
		{
			return i;
		}
	}

	if (!kTexturePath.empty())
		kCandidate.name = fs::path(kTexturePath).stem().string();
	else
		kCandidate.name = kMeshName + "_mat";

	if (kCandidate.useTextureAlpha)
	{
		std::cerr << "    Material '" << kCandidate.name
			<< "' preserves texture alpha: mode="
			<< (kCandidate.alphaTest ? "cutout" : "blend")
			<< " cutoff=" << kCandidate.alphaCutoff
			<< " opacity=" << kCandidate.opacity << std::endl;
	}

	kMaterials.push_back(std::move(kCandidate));
	return static_cast<unsigned int>(kMaterials.size()) - 1;
}

//--------------------------------------------------------------------------------------------------
aiMatrix4x4 MeshExtractor::MakeAiMatrix(const NiTransform& kT) const
{
	const float fScale = kT.m_fScale;
	const NiMatrix3& kRotate = kT.m_Rotate;
	const NiPoint3& kTranslate = kT.m_Translate;

	const aiMatrix4x4 kSourceMatrix(
		fScale * kRotate.GetEntry(0, 0), fScale * kRotate.GetEntry(0, 1),
		fScale * kRotate.GetEntry(0, 2), kTranslate.x * m_fTransformUnitScale,
		fScale * kRotate.GetEntry(1, 0), fScale * kRotate.GetEntry(1, 1),
		fScale * kRotate.GetEntry(1, 2), kTranslate.y * m_fTransformUnitScale,
		fScale * kRotate.GetEntry(2, 0), fScale * kRotate.GetEntry(2, 1),
		fScale * kRotate.GetEntry(2, 2), kTranslate.z * m_fTransformUnitScale,
		0.0f, 0.0f, 0.0f, 1.0f);

	return AxisConversion::ToTargetMatrix(kSourceMatrix,
		m_eAxisPreset);
}

//--------------------------------------------------------------------------------------------------
bool MeshExtractor::ExtractNiMesh(NiMesh* pkMesh,
	std::vector<IntermediateMesh>& kMeshes,
	std::vector<IntermediateMaterial>& kMaterials) const
{
	if (!pkMesh)
		return false;

	const NiPrimitiveType::Type ePrimitive = pkMesh->GetPrimitiveType();
	const unsigned int uiSubmeshCount = pkMesh->GetSubmeshCount();
	std::cerr << "    NiMesh primitive type: " << static_cast<int>(ePrimitive)
		<< ", submesh count: " << uiSubmeshCount
		<< ", modifier count: " << pkMesh->GetModifierCount() << std::endl;

	if (ePrimitive != NiPrimitiveType::PRIMITIVE_TRIANGLES &&
		ePrimitive != NiPrimitiveType::PRIMITIVE_TRISTRIPS)
	{
		std::cerr << "    Skipping: unsupported primitive type" << std::endl;
		return false;
	}

	if (uiSubmeshCount == 0)
	{
		std::cerr << "    Skipping: mesh has no submeshes" << std::endl;
		return false;
	}

	const char* pcName = pkMesh->GetName().c_str();
	const std::string kBaseName = pcName && pcName[0] != '\0'
		? std::string(pcName) : "mesh";

	NiTexturingProperty* pkTexProp = FindEffectiveProperty<NiTexturingProperty>(
		pkMesh, NiProperty::TEXTURING);
	NiAlphaProperty* pkAlphaProp = FindEffectiveProperty<NiAlphaProperty>(
		pkMesh, NiProperty::ALPHA);
	NiMaterialProperty* pkMaterialProp = FindEffectiveProperty<NiMaterialProperty>(
		pkMesh, NiProperty::MATERIAL);
	const std::string kTexPath = ResolveTexturePath(pkTexProp);
	const unsigned int uiMaterialIndex = FindOrAddMaterial(kTexPath, kBaseName,
		pkAlphaProp, pkMaterialProp, kMaterials);

	const unsigned int uiBaseUvSet = pkTexProp
		? pkTexProp->GetBaseTextureIndex() : 0u;
	const NiTextureTransform* pkBaseUvTransform = pkTexProp
		? pkTexProp->GetBaseTextureTransform() : nullptr;

	NiSkinningMeshModifier* pkSkinModifier = nullptr;
	for (NiUInt32 m = 0; m < pkMesh->GetModifierCount(); ++m)
	{
		NiMeshModifier* pkModifier = pkMesh->GetModifierAt(m);
		if (pkModifier && NiIsKindOf(NiSkinningMeshModifier, pkModifier))
		{
			pkSkinModifier = NiStaticCast(NiSkinningMeshModifier, pkModifier);
			break;
		}
	}

	// Static NiMesh submeshes must remain independent. Combining them into one
	// Assimp mesh can mix unrelated vertex/index regions and loses the original
	// submesh boundaries used by some models. Multiple output meshes can safely
	// be attached to the same aiNode through MeshNodeAssignmentMap.
	if (!pkSkinModifier)
	{
		unsigned int uiExportedSubmeshes = 0;
		for (unsigned int uiSubmesh = 0; uiSubmesh < uiSubmeshCount; ++uiSubmesh)
		{
			IntermediateMesh kOut;
			kOut.name = kBaseName;
			if (uiSubmeshCount > 1)
				kOut.name += "_submesh_" + std::to_string(uiSubmesh);
			kOut.sourceObject = pkMesh;
			kOut.materialIndex = uiMaterialIndex;

			if (!ReadVec3Stream(pkMesh, NiCommonSemantics::POSITION(),
				uiSubmesh, kOut.positions) || kOut.positions.empty())
			{
				std::cerr << "    Static submesh " << uiSubmesh
					<< " skipped: no readable position stream." << std::endl;
				continue;
			}

			if (!ReadVec3Stream(pkMesh, NiCommonSemantics::NORMAL(),
				uiSubmesh, kOut.normals) ||
				kOut.normals.size() != kOut.positions.size())
			{
				kOut.normals.clear();
			}

			unsigned int uiReadUvSet = uiBaseUvSet;
			bool bGotUvs = ReadVec2Stream(pkMesh,
				NiCommonSemantics::TEXCOORD(), uiReadUvSet, uiSubmesh, kOut.uvs);
			if (!bGotUvs && uiReadUvSet != 0u)
			{
				std::cerr << "      Base texture requests UV set " << uiReadUvSet
					<< ", but it is unavailable for submesh " << uiSubmesh
					<< "; falling back to UV set 0." << std::endl;
				uiReadUvSet = 0u;
				bGotUvs = ReadVec2Stream(pkMesh,
					NiCommonSemantics::TEXCOORD(), uiReadUvSet, uiSubmesh, kOut.uvs);
			}

			if (!bGotUvs || kOut.uvs.size() != kOut.positions.size())
			{
				kOut.uvs.clear();
			}
			else
			{
				ApplyTextureUvConversion(kOut.uvs, pkBaseUvTransform);
				LogUvRange(kOut.uvs, uiReadUvSet, pkBaseUvTransform != nullptr);
			}

			if (!ReadTriangleIndices(pkMesh, uiSubmesh,
				static_cast<unsigned int>(kOut.positions.size()), kOut.indices))
			{
				std::cerr << "    Static submesh " << uiSubmesh
					<< " skipped: no readable triangles." << std::endl;
				continue;
			}

			EnsureCompleteNormals(kOut);

			std::cerr << "    Exported static submesh " << uiSubmesh
				<< " separately: vertices=" << kOut.positions.size()
				<< " triangles=" << (kOut.indices.size() / 3) << std::endl;

			kMeshes.push_back(std::move(kOut));
			++uiExportedSubmeshes;
		}

		if (uiExportedSubmeshes == 0)
		{
			std::cerr << "    Skipping: no complete static submesh could be extracted."
				<< std::endl;
			return false;
		}

		std::cerr << "    Exported " << uiExportedSubmeshes << "/"
			<< uiSubmeshCount << " static submeshes as separate Assimp meshes."
			<< std::endl;
		return true;
	}

	// Skin partitions are still combined into one logical skinned mesh. This
	// preserves smoothing across partition boundaries and the existing bind-pose
	// and bone-weight behavior.
	IntermediateMesh kOut;
	kOut.name = kBaseName;
	kOut.sourceObject = pkMesh;
	kOut.materialIndex = uiMaterialIndex;
	InitializeSkinningFromNiMesh(pkSkinModifier, kOut);

	bool bAllUvs = true;
	unsigned int uiExportedSubmeshes = 0;

	for (unsigned int uiSubmesh = 0; uiSubmesh < uiSubmeshCount; ++uiSubmesh)
	{
		std::vector<aiVector3D> kPositions;
		std::vector<aiVector3D> kNormals;
		std::vector<aiVector2D> kUvs;
		std::vector<unsigned int> kIndices;

		bool bGotPositions = ReadVec3Stream(pkMesh,
			NiCommonSemantics::POSITION_BP(), uiSubmesh, kPositions);
		if (!bGotPositions)
		{
			bGotPositions = ReadVec3Stream(pkMesh,
				NiCommonSemantics::POSITION(), uiSubmesh, kPositions);
		}

		if (!bGotPositions || kPositions.empty())
		{
			std::cerr << "    Skinned submesh " << uiSubmesh
				<< " skipped: no readable position stream." << std::endl;
			continue;
		}

		bool bGotNormals = ReadVec3Stream(pkMesh,
			NiCommonSemantics::NORMAL_BP(), uiSubmesh, kNormals);
		if (!bGotNormals)
		{
			bGotNormals = ReadVec3Stream(pkMesh,
				NiCommonSemantics::NORMAL(), uiSubmesh, kNormals);
		}
		if (!bGotNormals || kNormals.size() != kPositions.size())
			kNormals.assign(kPositions.size(), aiVector3D(0.0f, 0.0f, 0.0f));

		unsigned int uiReadUvSet = uiBaseUvSet;
		bool bGotUvs = ReadVec2Stream(pkMesh,
			NiCommonSemantics::TEXCOORD(), uiReadUvSet, uiSubmesh, kUvs);
		if (!bGotUvs && uiReadUvSet != 0u)
		{
			std::cerr << "      Base texture requests UV set " << uiReadUvSet
				<< ", but it is unavailable for submesh " << uiSubmesh
				<< "; falling back to UV set 0." << std::endl;
			uiReadUvSet = 0u;
			bGotUvs = ReadVec2Stream(pkMesh,
				NiCommonSemantics::TEXCOORD(), uiReadUvSet, uiSubmesh, kUvs);
		}

		if (!bGotUvs || kUvs.size() != kPositions.size())
		{
			bAllUvs = false;
			kUvs.assign(kPositions.size(), aiVector2D());
		}
		else
		{
			ApplyTextureUvConversion(kUvs, pkBaseUvTransform);
			LogUvRange(kUvs, uiReadUvSet, pkBaseUvTransform != nullptr);
		}

		if (!ReadTriangleIndices(pkMesh, uiSubmesh,
			static_cast<unsigned int>(kPositions.size()), kIndices))
		{
			std::cerr << "    Skinned submesh " << uiSubmesh
				<< " skipped: no readable triangles." << std::endl;
			continue;
		}

		const unsigned int uiVertexBase =
			static_cast<unsigned int>(kOut.positions.size());
		kOut.positions.insert(kOut.positions.end(),
			kPositions.begin(), kPositions.end());
		kOut.normals.insert(kOut.normals.end(),
			kNormals.begin(), kNormals.end());
		kOut.uvs.insert(kOut.uvs.end(), kUvs.begin(), kUvs.end());

		kOut.indices.reserve(kOut.indices.size() + kIndices.size());
		for (unsigned int uiIndex : kIndices)
			kOut.indices.push_back(uiVertexBase + uiIndex);

		AppendSkinningFromNiMeshSubmesh(pkMesh, pkSkinModifier, uiSubmesh,
			static_cast<unsigned int>(kPositions.size()), kOut);
		++uiExportedSubmeshes;
	}

	if (kOut.positions.empty() || kOut.indices.empty())
	{
		std::cerr << "    Skipping: no complete skinned submesh could be extracted."
			<< std::endl;
		return false;
	}

	if (!bAllUvs)
		kOut.uvs.clear();

	EnsureCompleteNormals(kOut);
	NormalizeLegacyWeights(kOut);

	std::cerr << "    Combined " << uiExportedSubmeshes << "/" << uiSubmeshCount
		<< " skinned partitions: vertices=" << kOut.positions.size()
		<< " triangles=" << (kOut.indices.size() / 3)
		<< " bones=" << kOut.boneNames.size() << std::endl;

	kMeshes.push_back(std::move(kOut));
	return true;
}

//--------------------------------------------------------------------------------------------------
void MeshExtractor::InitializeSkinningFromNiMesh(
	NiSkinningMeshModifier* pkModifier, IntermediateMesh& kOut) const
{
	if (!pkModifier)
		return;

	const NiUInt32 uiBoneCount = pkModifier->GetBoneCount();
	if (uiBoneCount == 0)
		return;

	NiAVObject** ppkBones = pkModifier->GetBones();
	NiTransform* pkStoredSkinToBone = pkModifier->GetSkinToBoneTransforms();
	NiTransform kSkinWorld;
	const bool bCanReconstructFallback =
		ComputeModernSkinWorld(pkModifier, kSkinWorld);
	unsigned int uiStoredOffsets = 0;
	unsigned int uiHierarchyFallbacks = 0;
	unsigned int uiInvalidOffsets = 0;

	kOut.isSkinned = true;
	kOut.boneNames.resize(uiBoneCount);
	kOut.boneOffsetMatrices.resize(uiBoneCount);
	kOut.boneValid.assign(uiBoneCount, false);
	kOut.fallbackBoneIndex = SelectFallbackBoneIndex(ppkBones, uiBoneCount);

	for (NiUInt32 b = 0; b < uiBoneCount; ++b)
	{
		NiAVObject* pkBone = ppkBones ? ppkBones[b] : nullptr;
		kOut.boneValid[b] = pkBone != nullptr;
		kOut.boneNames[b] = pkBone
			? GetExportNodeName(pkBone)
			: "missing_bone_" + std::to_string(b);

		NiTransform kSkinToBone;
		bool bHaveOffset = false;

		// NiSkinningMeshModifier::GetSkinToBoneTransforms() is the native
		// inverse-bind data used by Gamebryo at runtime. It is already expressed
		// in skin/mesh space, exactly matching aiBone::mOffsetMatrix. Do not
		// rebuild it from the current node hierarchy: that hierarchy can contain
		// an animation/rest pose that differs from the skin's authored bind pose.
		if (pkStoredSkinToBone && IsFiniteTransform(pkStoredSkinToBone[b]))
		{
			kSkinToBone = pkStoredSkinToBone[b];
			bHaveOffset = true;
			++uiStoredOffsets;
		}
		else if (bCanReconstructFallback && pkBone &&
			ComputeHierarchySkinToBone(pkBone, kSkinWorld, kSkinToBone) &&
			IsFiniteTransform(kSkinToBone))
		{
			bHaveOffset = true;
			++uiHierarchyFallbacks;
		}
		else
		{
			++uiInvalidOffsets;
		}

		kOut.boneOffsetMatrices[b] = bHaveOffset
			? MakeAiMatrix(kSkinToBone) : aiMatrix4x4();
	}

	std::cerr << "    Skin inverse-bind matrices: stored=" << uiStoredOffsets
		<< ", hierarchy fallback=" << uiHierarchyFallbacks
		<< ", invalid=" << uiInvalidOffsets << "." << std::endl;

	if (IsUsableBoneIndex(kOut, kOut.fallbackBoneIndex))
	{
		std::cerr << "    Skin fallback bone: index="
			<< kOut.fallbackBoneIndex << " name='"
			<< kOut.boneNames[kOut.fallbackBoneIndex] << "'." << std::endl;
	}
	else
	{
		std::cerr << "    Warning: skin has no real bone available for "
			<< "rigid/unweighted fallback vertices." << std::endl;
	}
}

//--------------------------------------------------------------------------------------------------
void MeshExtractor::AppendSkinningFromNiMeshSubmesh(NiMesh* pkMesh,
	NiSkinningMeshModifier* pkModifier, unsigned int uiSubmesh,
	unsigned int uiVertexCount, IntermediateMesh& kOut) const
{
	if (!pkMesh || !pkModifier || kOut.boneNames.empty())
		return;

	std::vector<Index4> kBlendIndices;
	std::vector<Weight4> kBlendWeights;
	std::vector<unsigned int> kBonePalette;

	const bool bHasIndices = ReadBlendIndices(pkMesh, uiSubmesh,
		uiVertexCount, kBlendIndices);
	const bool bHasWeights = ReadBlendWeights(pkMesh, uiSubmesh,
		uiVertexCount, kBlendWeights);
	const bool bHasPalette = ReadBonePalette(pkMesh, uiSubmesh, kBonePalette);
	const bool bCanReadSkin = bHasIndices && bHasWeights;
	const bool bHasFallback =
		IsUsableBoneIndex(kOut, kOut.fallbackBoneIndex);

	const size_t stOldSize = kOut.boneWeights.size();
	kOut.boneWeights.resize(stOldSize + uiVertexCount);

	unsigned int uiFallbackVertices = 0;
	unsigned int uiInvalidPaletteIndices = 0;
	unsigned int uiInvalidGlobalBones = 0;
	unsigned int uiMissingBonePointers = 0;

	for (unsigned int v = 0; v < uiVertexCount; ++v)
	{
		std::vector<VertexBoneWeight>& kVertexWeights =
			kOut.boneWeights[stOldSize + v];

		if (bCanReadSkin)
		{
			for (unsigned int k = 0; k < 4; ++k)
			{
				const float fWeight = kBlendWeights[v][k];
				if (!std::isfinite(fWeight) || fWeight <= 0.000001f)
					continue;

				unsigned int uiBoneIndex = kBlendIndices[v][k];
				if (bHasPalette)
				{
					if (uiBoneIndex >= kBonePalette.size())
					{
						++uiInvalidPaletteIndices;
						continue;
					}
					uiBoneIndex = kBonePalette[uiBoneIndex];
				}

				if (uiBoneIndex >= kOut.boneNames.size())
				{
					++uiInvalidGlobalBones;
					continue;
				}

				if (!IsUsableBoneIndex(kOut, uiBoneIndex))
				{
					++uiMissingBonePointers;
					continue;
				}

				kVertexWeights.push_back({uiBoneIndex, fWeight});
			}
		}

		if (kVertexWeights.empty() && bHasFallback)
		{
			kVertexWeights.push_back({kOut.fallbackBoneIndex, 1.0f});
			++uiFallbackVertices;
		}
	}

	if (!bCanReadSkin)
	{
		std::cerr << "    Warning: submesh " << uiSubmesh
			<< " does not have a complete, exact-size BLENDINDICES/"
			<< "BLENDWEIGHT pair; treated as a rigid skinned section";
		if (bHasFallback)
		{
			std::cerr << " attached to '"
				<< kOut.boneNames[kOut.fallbackBoneIndex] << "'";
		}
		std::cerr << "." << std::endl;
	}

	if (uiInvalidPaletteIndices > 0 || uiInvalidGlobalBones > 0 ||
		uiMissingBonePointers > 0)
	{
		std::cerr << "    Warning: submesh " << uiSubmesh
			<< " discarded invalid skin influences: paletteIndex="
			<< uiInvalidPaletteIndices << " globalBone="
			<< uiInvalidGlobalBones << " missingBone="
			<< uiMissingBonePointers << "." << std::endl;
	}

	if (uiFallbackVertices > 0)
	{
		std::cerr << "    Submesh " << uiSubmesh << " assigned "
			<< uiFallbackVertices << " vertex/vertices to real fallback bone '"
			<< kOut.boneNames[kOut.fallbackBoneIndex] << "'." << std::endl;
	}
	else if (!bHasFallback)
	{
		std::cerr << "    Error: submesh " << uiSubmesh
			<< " has no valid fallback bone; unweighted vertices cannot be "
			<< "made export-safe." << std::endl;
	}
}

//--------------------------------------------------------------------------------------------------
bool MeshExtractor::ExtractNiGeometry(NiGeometry* pkGeom,
	std::vector<IntermediateMesh>& kMeshes,
	std::vector<IntermediateMaterial>& kMaterials) const
{
	if (!pkGeom)
		return false;

	NiGeometryData* pkData = pkGeom->GetModelData();
	if (!pkData)
	{
		std::cerr << "  Skipped geometry without model data: " << pkGeom->GetName().c_str()
			<< std::endl;
		return false;
	}

	const bool bIsShape = NiIsKindOf(NiTriShape, pkGeom);
	const bool bIsStrips = NiIsKindOf(NiTriStrips, pkGeom);
	if (!bIsShape && !bIsStrips)
	{
		std::cerr << "  Skipped unsupported legacy geometry: " << pkGeom->GetName().c_str()
			<< std::endl;
		return false;
	}

	IntermediateMesh kOut;
	const char* pcName = pkGeom->GetName().c_str();
	kOut.name = pcName ? std::string(pcName) : "legacy_mesh";
	kOut.sourceObject = pkGeom;

	NiTexturingProperty* pkTexProp = FindEffectiveProperty<NiTexturingProperty>(
		pkGeom, NiProperty::TEXTURING);
	NiAlphaProperty* pkAlphaProp = FindEffectiveProperty<NiAlphaProperty>(
		pkGeom, NiProperty::ALPHA);
	NiMaterialProperty* pkMaterialProp = FindEffectiveProperty<NiMaterialProperty>(
		pkGeom, NiProperty::MATERIAL);
	const std::string kTexPath = ResolveTexturePath(pkTexProp);
	kOut.materialIndex = FindOrAddMaterial(kTexPath, kOut.name,
		pkAlphaProp, pkMaterialProp, kMaterials);

	const unsigned short usVertexCount = pkData->GetVertexCount();
	if (usVertexCount == 0)
		return false;

	const NiPoint3* pkVertices = pkData->GetVertices();
	if (!pkVertices)
		return false;

	kOut.positions.reserve(usVertexCount);
	for (unsigned short v = 0; v < usVertexCount; ++v)
		kOut.positions.emplace_back(pkVertices[v].x, pkVertices[v].y, pkVertices[v].z);

	const NiPoint3* pkNormals = pkData->GetNormals();
	if (pkNormals)
	{
		kOut.normals.reserve(usVertexCount);
		for (unsigned short v = 0; v < usVertexCount; ++v)
			kOut.normals.emplace_back(pkNormals[v].x, pkNormals[v].y, pkNormals[v].z);
	}

	unsigned int uiLegacyUvSet = pkTexProp
		? pkTexProp->GetBaseTextureIndex() : 0u;
	const NiTextureTransform* pkLegacyUvTransform = pkTexProp
		? pkTexProp->GetBaseTextureTransform() : nullptr;

	const NiPoint2* pkUvs = pkData->GetTextureSet(
		static_cast<unsigned short>(uiLegacyUvSet));
	if (!pkUvs && uiLegacyUvSet != 0u)
	{
		std::cerr << "    Base texture requests UV set " << uiLegacyUvSet
			<< ", but legacy geometry only has " << pkData->GetTextureSets()
			<< " set(s); falling back to UV set 0." << std::endl;
		uiLegacyUvSet = 0u;
		pkUvs = pkData->GetTextureSet(0);
	}

	if (pkUvs)
	{
		kOut.uvs.reserve(usVertexCount);
		for (unsigned short v = 0; v < usVertexCount; ++v)
			kOut.uvs.emplace_back(pkUvs[v].x, pkUvs[v].y);

		ApplyTextureUvConversion(kOut.uvs, pkLegacyUvTransform);
		LogUvRange(kOut.uvs, uiLegacyUvSet, pkLegacyUvTransform != nullptr);
	}

	NiTriBasedGeomData* pkTriData = NiStaticCast(NiTriBasedGeomData, pkData);
	const unsigned short usTriangleCount = pkTriData->GetTriangleCount();
	kOut.indices.reserve(usTriangleCount * 3u);

	for (unsigned short t = 0; t < usTriangleCount; ++t)
	{
		unsigned short i0 = 0;
		unsigned short i1 = 0;
		unsigned short i2 = 0;
		pkTriData->GetTriangleIndices(t, i0, i1, i2);
		if (i0 >= usVertexCount || i1 >= usVertexCount || i2 >= usVertexCount)
			continue;
		if (i0 == i1 || i0 == i2 || i1 == i2)
			continue;

		kOut.indices.push_back(i0);
		kOut.indices.push_back(i1);
		kOut.indices.push_back(i2);
	}

	if (kOut.indices.empty())
		return false;

	EnsureCompleteNormals(kOut);

	if (pkGeom->GetSkinInstance())
	{
		ExtractSkinningFromNiGeometry(pkGeom, kOut);
		NormalizeLegacyWeights(kOut);
	}

	kMeshes.push_back(std::move(kOut));
	return true;
}

//--------------------------------------------------------------------------------------------------
void MeshExtractor::ExtractSkinningFromNiGeometry(NiGeometry* pkGeom,
	IntermediateMesh& kOut) const
{
	NiSkinInstance* pkSkin = pkGeom->GetSkinInstance();
	if (!pkSkin)
		return;

	NiSkinData* pkSkinData = pkSkin->GetSkinData();
	if (!pkSkinData)
		return;

	const unsigned int uiBoneCount = pkSkinData->GetBoneCount();
	NiSkinData::BoneData* pkBoneData = pkSkinData->GetBoneData();
	if (uiBoneCount == 0 || !pkBoneData)
		return;

	const NiAVObject* const* ppkBones = pkSkin->GetBones();
	unsigned int uiStoredOffsets = 0;
	unsigned int uiInvalidOffsets = 0;

	kOut.isSkinned = true;
	kOut.boneNames.resize(uiBoneCount);
	kOut.boneOffsetMatrices.resize(uiBoneCount);
	kOut.boneValid.assign(uiBoneCount, false);
	kOut.fallbackBoneIndex = SelectFallbackBoneIndex(ppkBones, uiBoneCount);

	for (unsigned int b = 0; b < uiBoneCount; ++b)
	{
		const NiAVObject* pkBone = ppkBones ? ppkBones[b] : nullptr;
		kOut.boneValid[b] = pkBone != nullptr;
		kOut.boneNames[b] = pkBone
			? GetExportNodeName(pkBone)
			: "missing_bone_" + std::to_string(b);

		if (IsFiniteTransform(pkBoneData[b].m_kSkinToBone))
		{
			kOut.boneOffsetMatrices[b] =
				MakeAiMatrix(pkBoneData[b].m_kSkinToBone);
			++uiStoredOffsets;
		}
		else
		{
			kOut.boneOffsetMatrices[b] = aiMatrix4x4();
			++uiInvalidOffsets;
		}
	}

	std::cerr << "    Legacy skin inverse-bind matrices: stored="
		<< uiStoredOffsets << ", invalid=" << uiInvalidOffsets << "."
		<< std::endl;

	if (IsUsableBoneIndex(kOut, kOut.fallbackBoneIndex))
	{
		std::cerr << "    Legacy skin fallback bone: index="
			<< kOut.fallbackBoneIndex << " name='"
			<< kOut.boneNames[kOut.fallbackBoneIndex] << "'." << std::endl;
	}

	const unsigned int uiVertexCount = static_cast<unsigned int>(kOut.positions.size());
	kOut.boneWeights.resize(uiVertexCount);

	bool bHasAnyBoneVertexData = false;
	for (unsigned int b = 0; b < uiBoneCount; ++b)
	{
		NiSkinData::BoneData& kBone = pkBoneData[b];
		if (kBone.m_usVerts == 0 || !kBone.m_pkBoneVertData)
			continue;

		bHasAnyBoneVertexData = true;
		for (unsigned short vi = 0; vi < kBone.m_usVerts; ++vi)
		{
			const unsigned int uiVertex = kBone.m_pkBoneVertData[vi].m_usVert;
			const float fWeight = kBone.m_pkBoneVertData[vi].m_fWeight;
			if (uiVertex < uiVertexCount && fWeight > 0.0f)
				kOut.boneWeights[uiVertex].push_back({b, fWeight});
		}
	}

	const unsigned int uiMissingBeforePartition = CountUnweightedVertices(kOut);
	if (uiMissingBeforePartition == 0)
		return;

	IntermediateMesh kPartitionWeights;
	kPartitionWeights.name = kOut.name;
	kPartitionWeights.positions.resize(uiVertexCount);
	kPartitionWeights.boneNames = kOut.boneNames;
	kPartitionWeights.boneValid = kOut.boneValid;
	kPartitionWeights.fallbackBoneIndex = kOut.fallbackBoneIndex;
	kPartitionWeights.boneWeights.resize(uiVertexCount);

	ExtractSkinningFromPartition(pkSkin, pkSkinData, kPartitionWeights,
		uiVertexCount);

	const unsigned int uiMissingInPartition =
		CountUnweightedVertices(kPartitionWeights);
	if (uiMissingInPartition >= uiMissingBeforePartition)
		return;

	if (!bHasAnyBoneVertexData)
	{
		kOut.boneWeights = std::move(kPartitionWeights.boneWeights);
		std::cerr << "    Used NiSkinPartition weights for mesh '"
			<< kOut.name << "': missing vertices "
			<< uiMissingBeforePartition << " -> "
			<< uiMissingInPartition << std::endl;
		return;
	}

	unsigned int uiFilledFromPartition = 0;
	for (unsigned int v = 0; v < uiVertexCount; ++v)
	{
		if (HasValidBoneWeights(kOut, v) ||
			!HasValidBoneWeights(kPartitionWeights, v))
		{
			continue;
		}

		kOut.boneWeights[v] = std::move(kPartitionWeights.boneWeights[v]);
		++uiFilledFromPartition;
	}

	if (uiFilledFromPartition > 0)
	{
		std::cerr << "    Filled " << uiFilledFromPartition
			<< " missing NiSkinData vertex weight(s) from NiSkinPartition in mesh '"
			<< kOut.name << "'." << std::endl;
	}
}

//--------------------------------------------------------------------------------------------------
void MeshExtractor::ExtractSkinningFromPartition(NiSkinInstance* pkSkin,
	NiSkinData* pkSkinData, IntermediateMesh& kOut,
	unsigned int uiVertexCount) const
{
	// Since Gamebryo 1.2 the authoritative partition is stored on
	// NiSkinInstance. Older files may still keep it on NiSkinData.
	NiSkinPartition* pkPartition = pkSkin ? pkSkin->GetSkinPartition() : nullptr;
	if (!pkPartition && pkSkinData)
		pkPartition = pkSkinData->GetSkinPartition(true);
	if (!pkPartition)
		return;

	if (kOut.boneWeights.size() < uiVertexCount)
		kOut.boneWeights.resize(uiVertexCount);

	const unsigned int uiPartitionCount = pkPartition->GetPartitionCount();
	NiSkinPartition::Partition* pkParts = pkPartition->GetPartitions();
	if (!pkParts)
		return;

	unsigned int uiInvalidVertexMap = 0;
	unsigned int uiInvalidLocalPalette = 0;
	unsigned int uiInvalidGlobalBone = 0;

	for (unsigned int p = 0; p < uiPartitionCount; ++p)
	{
		const NiSkinPartition::Partition& kPart = pkParts[p];
		if (!kPart.m_pusVertexMap || !kPart.m_pusBones || !kPart.m_pfWeights)
			continue;

		for (unsigned short vLocal = 0; vLocal < kPart.m_usVertices; ++vLocal)
		{
			const unsigned int uiFullVertex = kPart.m_pusVertexMap[vLocal];
			if (uiFullVertex >= uiVertexCount)
			{
				++uiInvalidVertexMap;
				continue;
			}

			for (unsigned short k = 0; k < kPart.m_usBonesPerVertex; ++k)
			{
				const unsigned int uiInfluence =
					vLocal * kPart.m_usBonesPerVertex + k;
				const float fWeight = kPart.m_pfWeights[uiInfluence];
				if (!std::isfinite(fWeight) || fWeight <= 0.0f)
					continue;

				const unsigned int uiLocalBone = kPart.m_pucBonePalette
					? kPart.m_pucBonePalette[uiInfluence]
					: static_cast<unsigned int>(k);
				if (uiLocalBone >= kPart.m_usBones)
				{
					++uiInvalidLocalPalette;
					continue;
				}

				const unsigned int uiGlobalBone = kPart.m_pusBones[uiLocalBone];
				if (!IsUsableBoneIndex(kOut, uiGlobalBone))
				{
					++uiInvalidGlobalBone;
					continue;
				}

				kOut.boneWeights[uiFullVertex].push_back({uiGlobalBone, fWeight});
			}
		}
	}

	if (uiInvalidVertexMap > 0 || uiInvalidLocalPalette > 0 ||
		uiInvalidGlobalBone > 0)
	{
		std::cerr << "    Warning: NiSkinPartition discarded invalid entries: "
			<< "vertexMap=" << uiInvalidVertexMap
			<< " paletteIndex=" << uiInvalidLocalPalette
			<< " globalBone=" << uiInvalidGlobalBone
			<< " in mesh '" << kOut.name << "'." << std::endl;
	}
}


//--------------------------------------------------------------------------------------------------
aiNode* MeshExtractor::BuildNodeRecursive(NiAVObject* pkObject,
	const MeshNodeAssignmentMap& kMeshNodeAssignments) const
{
	if (!pkObject)
		return nullptr;

	aiNode* pkAiNode = new aiNode();
	const std::string kExportName = GetExportNodeName(pkObject);
	pkAiNode->mName = kExportName.c_str();

	NiTransform kNodeTransform;
	if (!GetLegacySkinNodeTransform(pkObject, kNodeTransform) &&
		!GetModernSkinNodeTransform(pkObject, kNodeTransform))
	{
		// Bone nodes deliberately keep their original NIF local transforms.
		// Animation evaluators also produce transforms in this frame.
		kNodeTransform = pkObject->GetLocalTransform();
	}

	// FBX/Unity evaluates scaling curves whenever a node has an animation
	// channel. A negative static scale on an animated bone can therefore be
	// decomposed into unstable per-axis scale curves. Move the complete static
	// rest transform onto a synthetic parent and leave the original named node
	// at identity. AnimationExporter writes the named node as a rest-relative
	// transform, so the bind world matrix remains unchanged while animated scale
	// stays positive.
	aiNode* pkReturnNode = pkAiNode;
	if (std::isfinite(kNodeTransform.m_fScale) &&
		kNodeTransform.m_fScale < -0.000001f)
	{
		aiNode* pkStaticParent = new aiNode();
		pkStaticParent->mName =
			("NIFToolset_StaticRest_" + kExportName).c_str();
		pkStaticParent->mTransformation = MakeAiMatrix(kNodeTransform);
		pkStaticParent->mNumChildren = 1;
		pkStaticParent->mChildren = new aiNode*[1];
		pkStaticParent->mChildren[0] = pkAiNode;
		pkAiNode->mParent = pkStaticParent;
		pkAiNode->mTransformation = aiMatrix4x4();
		pkReturnNode = pkStaticParent;

		std::cerr << "    Isolated negative static scale for node '"
			<< kExportName << "' on parent '"
			<< pkStaticParent->mName.C_Str() << "'." << std::endl;
	}
	else
	{
		pkAiNode->mTransformation = MakeAiMatrix(kNodeTransform);
	}

	auto kAssignment = kMeshNodeAssignments.find(pkObject);
	if (kAssignment != kMeshNodeAssignments.end() && !kAssignment->second.empty())
	{
		pkAiNode->mNumMeshes = static_cast<unsigned int>(kAssignment->second.size());
		pkAiNode->mMeshes = new unsigned int[pkAiNode->mNumMeshes];
		for (unsigned int i = 0; i < pkAiNode->mNumMeshes; ++i)
			pkAiNode->mMeshes[i] = kAssignment->second[i];
	}

	if (NiIsKindOf(NiLODNode, pkObject))
	{
		NiNode* pkLODNode = NiStaticCast(NiNode, pkObject);
		NiAVObject* pkBestChild = PickBestLODChild(pkLODNode);
		if (pkBestChild)
		{
			aiNode* pkChildNode = BuildNodeRecursive(pkBestChild,
				kMeshNodeAssignments);
			if (pkChildNode)
			{
				pkChildNode->mParent = pkAiNode;
				pkAiNode->mNumChildren = 1;
				pkAiNode->mChildren = new aiNode*[1];
				pkAiNode->mChildren[0] = pkChildNode;
			}
		}
		return pkReturnNode;
	}

	if (NiIsKindOf(NiSwitchNode, pkObject))
	{
		NiSwitchNode* pkSwitch = NiStaticCast(NiSwitchNode, pkObject);
		NiAVObject* pkChild = PickSwitchChild(pkSwitch);
		if (pkChild)
		{
			aiNode* pkChildNode = BuildNodeRecursive(pkChild,
				kMeshNodeAssignments);
			if (pkChildNode)
			{
				pkChildNode->mParent = pkAiNode;
				pkAiNode->mNumChildren = 1;
				pkAiNode->mChildren = new aiNode*[1];
				pkAiNode->mChildren[0] = pkChildNode;
			}
		}
		return pkReturnNode;
	}

	if (NiIsKindOf(NiNode, pkObject))
	{
		NiNode* pkNode = NiStaticCast(NiNode, pkObject);
		std::vector<aiNode*> kChildren;
		kChildren.reserve(pkNode->GetArrayCount());

		for (unsigned int i = 0; i < pkNode->GetArrayCount(); ++i)
		{
			NiAVObject* pkChild = pkNode->GetAt(i);
			if (!pkChild)
				continue;

			aiNode* pkChildNode = BuildNodeRecursive(pkChild, kMeshNodeAssignments);
			if (pkChildNode)
			{
				pkChildNode->mParent = pkAiNode;
				kChildren.push_back(pkChildNode);
			}
		}

		pkAiNode->mNumChildren = static_cast<unsigned int>(kChildren.size());
		if (!kChildren.empty())
		{
			pkAiNode->mChildren = new aiNode*[kChildren.size()];
			for (unsigned int i = 0; i < kChildren.size(); ++i)
				pkAiNode->mChildren[i] = kChildren[i];
		}
	}

	return pkReturnNode;
}

//--------------------------------------------------------------------------------------------------
aiNode* MeshExtractor::BuildNodeHierarchy(NiAVObject* pkRoot,
	const MeshNodeAssignmentMap& kMeshNodeAssignments) const
{
	// Keep the exported skeleton in the same original local frame used by
	// KF/KFM and NiTransformController animation channels. Negative static
	// transforms are isolated on synthetic parents while the original named
	// bone remains available as the animation/skin target.
	aiNode* pkHierarchy = BuildNodeRecursive(pkRoot, kMeshNodeAssignments);
	if (!pkHierarchy)
		return nullptr;

	// Keep every original mesh/bone local transform and every skin bind-space
	// relationship untouched. Centering is performed by one parent transform
	// above the complete character, exactly like moving the Armature/Object in
	// Blender instead of editing Bip01 or the skinned mesh node themselves.
	NiAVObject* pkBip01 = FindExactNodeByName(pkRoot, "bip01");
	if (!pkBip01)
		return pkHierarchy;

	const NiTransform kBip01World = ComputeWorldTransformFromParents(pkBip01);
	NiTransform kPlacement;
	kBip01World.Invert(kPlacement);

	aiNode* pkPlacementNode = new aiNode();
	pkPlacementNode->mName = "NIFToolset_CharacterPlacement";
	pkPlacementNode->mTransformation = MakeAiMatrix(kPlacement);
	pkPlacementNode->mNumChildren = 1;
	pkPlacementNode->mChildren = new aiNode*[1];
	pkPlacementNode->mChildren[0] = pkHierarchy;
	pkHierarchy->mParent = pkPlacementNode;

	const NiPoint3& kBipTranslate = kBip01World.m_Translate;
	const NiPoint3& kPlacementTranslate = kPlacement.m_Translate;
	std::cerr << "  Centered complete character above unchanged hierarchy using exact Bip01 world transform: "
		<< "Bip01 world translate=(" << kBipTranslate.x << ", "
		<< kBipTranslate.y << ", " << kBipTranslate.z << "), placement translate=("
		<< kPlacementTranslate.x << ", " << kPlacementTranslate.y << ", "
		<< kPlacementTranslate.z << ")." << std::endl;

	return pkPlacementNode;
}
