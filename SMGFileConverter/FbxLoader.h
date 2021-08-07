#pragma once
#include <fbxsdk.h>
#include <string.h>
#include <vector>
#include <unordered_map>
#include "SMGEngine/TypeGeometry.h"
#include "SMGEngine/TypeD3d.h"
#include <set>
#include "SMGEngine/PreDefines.h"

using namespace std;
using namespace DirectX;
// 전방선언
class AnimationClip;
class BoneAnimation;
struct KeyFrame;
class XMLWriter;

class FbxLoader
{
private:
	static constexpr int VERTEX_PER_POLYGON = 3;

	// normalIndex가 오버플로우되는 일이 있었는데 시간날때 다른 변수들도 문제 일어나지 않게 고칠것 [6/13/2021 qwerw]
	struct FbxPolygonVertexInfo
	{
		uint16_t _materialIndex = std::numeric_limits<uint16_t>::max();
		uint16_t _controlPointIndex = std::numeric_limits<uint16_t>::max();
		uint16_t _uvIndex = std::numeric_limits<uint16_t>::max();
		uint16_t _colorIndex = std::numeric_limits<uint16_t>::max();
		uint32_t _normalIndex = std::numeric_limits<uint32_t>::max();
	};

	enum class IndexMappingType
	{
		ControlPointIndex,
		IndexArrayAtControlPoint,
		PolygonVertexIndex,
		IndexArrayAtPolygonVertex,

		Undefined,
	};

	struct FbxVertexSkinningInfo
	{
		FbxVertexSkinningInfo() noexcept;
		void insert(uint16_t boneIndex, float weight) noexcept;
		std::array<BoneIndex, BONE_WEIGHT_COUNT> _boneIndex;
		std::array<float, BONE_WEIGHT_COUNT> _weight;
	};

	struct FbxMeshInfo
	{
		FbxMeshInfo(FbxMeshInfo&&) = default;
		FbxMeshInfo& operator=(FbxMeshInfo&&) = default;
		FbxMeshInfo(const FbxMeshInfo&) = delete;
		FbxMeshInfo& operator=(const FbxMeshInfo&) = delete;

		FbxMeshInfo(string&& name,
			vector<vector<Vertex>>&& vertices,
			vector<vector<SkinnedVertex>>&& skinnedVertices,
			vector<vector<GeoIndex>>&& indices,
			const DirectX::XMFLOAT3& min,
			const DirectX::XMFLOAT3& max);

		string _name;
		vector<vector<Vertex>> _vertices;
		vector<vector<SkinnedVertex>> _skinnedVertices;
		vector<vector<GeoIndex>> _indices;
		XMFLOAT3 _min;
		XMFLOAT3 _max;
	};

	struct FbxMaterialStaticInfo
	{
		std::string _name;
		DirectX::XMFLOAT4 _diffuseAlbedo;
		DirectX::XMFLOAT3 _fresnelR0;
		float _roughness;
		std::vector<string> _useTextureNames;
		FbxMaterialStaticInfo(const std::string& name,
			const DirectX::XMFLOAT4& diffuseAlbedo,
			const DirectX::XMFLOAT3& fresnelR0,
			const float roughness)
			: _name(name)
			, _diffuseAlbedo(diffuseAlbedo)
			, _fresnelR0(fresnelR0)
			, _roughness(roughness) {}
	};

	struct AnimClipTimeInfo
	{
		std::string _name;
		uint32_t _start;
		uint32_t _end;
		bool _isReverse;
	};

	void loadFbxAnimation(FbxAnimLayer* animLayer);
	void writeXmlMesh(XMLWriter& xmlMeshGeometry, const FbxMeshInfo& meshInfo, const string& fileName) const;
	void writeXmlMaterial(XMLWriter& xmlMaterial) const;
	void writeXmlSkeleton(XMLWriter& xmlSkeleton) const;
	void writeXmlAnimation(XMLWriter& xmlAnimation) const;

	void loadFbxParseInfo(const std::string& fbxFilePath);
public:
	FbxLoader(void);
	~FbxLoader(void);
	void ConvertFbxFiles(const string& filePath, const string& objectFolderPath);

private:
	FbxManager* _fbxManager;

	std::vector<FbxMaterialStaticInfo> _materialsInfos;

	std::vector<FbxMeshInfo> _meshInfos;
	std::vector<int> _boneHierarchy;
	std::vector<DirectX::XMFLOAT4X4> _boneOffsets;
	std::vector<std::string> _boneNames;
	std::unordered_map<std::string, AnimationClip> _animations;

	std::unordered_map<FbxNode*, BoneIndex> _boneIndexMap;
	std::vector<AnimClipTimeInfo> _animTimeList;
	const std::string fbxFolderPath = "../ResourcesRaw/FbxFiles";

	void loadFbxMeshNode(FbxNode* node);
	void loadFbxSkeletonNode(FbxNode* node, bool& nodeFound);

	void loadFbxMesh(FbxNode* node);

	void loadFbxSkeleton(FbxNode* node, BoneIndex parentIndex);

	void loadFbxMaterial(FbxScene* node);

	void loadFbxPolygons(FbxMesh* mesh, std::vector<FbxPolygonVertexInfo>& polygons) const;

	void loadFbxOptimizedMesh(const FbxMesh* mesh,
								const std::vector<FbxPolygonVertexInfo>& polygonVertices,
								const std::vector<FbxVertexSkinningInfo>& skinningInfos);

	void loadFbxSkin(FbxNode* node, std::vector<FbxVertexSkinningInfo>& skinningInfos);

	void loadFbxAnimations(FbxScene* fbxScene);

	void writeXmlFile(const string& path, const string& fileName) const;

	void clearCachedFbxFileInfo(void) noexcept;

	// 유틸 함수
	inline static DirectX::XMFLOAT3 fbxVector4ToXMFLOAT3(const FbxVector4& fbxVector) noexcept
	{
		return DirectX::XMFLOAT3(fbxVector.mData[0], fbxVector.mData[1], fbxVector.mData[2]);
	}
	inline static DirectX::XMFLOAT2 fbxVector2ToXMFLOAT2(const FbxVector2& fbxVector) noexcept
	{
		return DirectX::XMFLOAT2(fbxVector.mData[0], fbxVector.mData[1]);
	}
	inline static DirectX::XMFLOAT4 fbxQuaternionToXMFLOAT4(const FbxQuaternion& fbxVector) noexcept
	{
		return DirectX::XMFLOAT4(fbxVector.mData[0], fbxVector.mData[1], fbxVector.mData[2], fbxVector.mData[3]);
	}
	static DirectX::XMFLOAT4X4 fbxMatrixToXMFLOAT4X4(const FbxAMatrix& matrix) noexcept;

	static IndexMappingType getIndexMappingType(const FbxLayerElement::EReferenceMode referenceMode, const FbxLayerElement::EMappingMode mappingMode);

	static std::array<float, BONE_WEIGHT_COUNT - 1> getWeight(const std::array<float, BONE_WEIGHT_COUNT>& weightArray) noexcept;

	void getKeyFrameTimes(const FbxAnimCurve* curve, std::vector<std::set<uint32_t>>& keyFrameTimes) const noexcept;

	void getKeyFrames(FbxAnimLayer* animLayer, FbxNode* linkNode, std::vector<std::set<uint32_t>>& keyFramesTimes) const noexcept;
	static FbxAMatrix getGeometryTransformation(const FbxNode* inNode)
	{
		if (!inNode)
		{
			throw std::exception("Null for mesh geometry");
		}

		const FbxVector4 lT = inNode->GetGeometricTranslation(FbxNode::eSourcePivot);
		const FbxVector4 lR = inNode->GetGeometricRotation(FbxNode::eSourcePivot);
		const FbxVector4 lS = inNode->GetGeometricScaling(FbxNode::eSourcePivot);

		return FbxAMatrix(lT, lR, lS);
	}

	FbxNode* getParentLinkNode(FbxNode* linkNode) const;
};