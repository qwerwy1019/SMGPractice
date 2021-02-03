#pragma once
#include <fbxsdk.h>
#include <string.h>
#include <vector>
#include <unordered_map>
#include "SMGEngine/TypeGeometry.h"
#include "SMGEngine/TypeData.h"
#include <set>

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

	struct FbxPolygonVertexInfo
	{
		CommonIndex _materialIndex = UNDEFINED_COMMON_INDEX;
		CommonIndex _controlPointIndex = UNDEFINED_COMMON_INDEX;
		CommonIndex _uvIndex = UNDEFINED_COMMON_INDEX;
		CommonIndex _normalIndex = UNDEFINED_COMMON_INDEX;
		CommonIndex _colorIndex = UNDEFINED_COMMON_INDEX;
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
		void insert(CommonIndex boneIndex, float weight) noexcept;
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
			vector<vector<Index>>&& indices);

		string _name;
		vector<vector<Vertex>> _vertices;
		vector<vector<SkinnedVertex>> _skinnedVertices;
		vector<vector<Index>> _indices;
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
	
	enum class FbxKeyFrameType
	{
		TranslateX,
		TranslateY,
		TranslateZ,
		ScaleX,
		ScaleY,
		ScaleZ,
		RotationX,
		RotationY,
		RotationZ,

		Count,
	};
	struct FbxKeyFrame
	{
		FbxLongLong _frame;
		FbxKeyFrameType _type;
		float _value;
	};

	HRESULT loadKeyFrame(FbxAnimLayer* animLayer, const string& animationName);
	HRESULT writeXmlMesh(XMLWriter& xmlMeshGeometry, int meshIndex, const string& fileName) const noexcept;
	HRESULT writeXmlMaterial(XMLWriter& xmlMaterial) const noexcept;
	HRESULT writeXmlSkeleton(XMLWriter& xmlSkeleton) const noexcept;
	HRESULT writeXmlAnimation(XMLWriter& xmlAnimation) const noexcept;
	FbxNode* getParentLinkNode(FbxNode* linkNode, int boneIndex) const noexcept;
public:
	FbxLoader(void);
	~FbxLoader(void);
	HRESULT LoadFbxFiles(const string& filePath, const string& objectFolderPath);

private:
	FbxManager* _fbxManager;

	std::vector<FbxMaterialStaticInfo> _materialsInfos;

	std::vector<FbxMeshInfo> _meshInfos;
	std::vector<int> _boneHierarchy;
	std::vector<DirectX::XMFLOAT4X4> _boneOffsets;
	std::vector<std::string> _boneNames;
	std::unordered_map<std::string, AnimationClip> _animations;

	std::unordered_map<FbxNode*, CommonIndex> _boneIndexMap;
	bool _isZUpvectorSystem;

	const std::string fbxFolderPath = "../ResourcesRaw/FbxFiles";

	HRESULT loadFbxMeshNode(FbxNode* node);
	HRESULT loadFbxSkeletonNode(FbxNode* node, bool& nodeFound);

	HRESULT loadFbxMesh(FbxNode* node);

	HRESULT loadFbxSkeleton(FbxNode* node, CommonIndex parentIndex);

	HRESULT loadFbxMaterial(FbxScene* node);

	HRESULT loadFbxPolygons(FbxMesh* mesh, std::vector<FbxPolygonVertexInfo>& polygons) const noexcept;

	HRESULT loadFbxOptimizedMesh(const FbxMesh* mesh,
								const std::vector<FbxPolygonVertexInfo>& polygonVertices,
								const std::vector<FbxVertexSkinningInfo>& skinningInfos);

	HRESULT loadFbxSkin(FbxNode* node, std::vector<FbxVertexSkinningInfo>& skinningInfos);

	HRESULT loadFbxAnimations(FbxScene* fbxScene) noexcept;

	HRESULT writeXmlFile(const string& path, const string& fileName) const;

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

	static IndexMappingType getUVIndexType(const FbxLayerElement::EReferenceMode referenceMode, const FbxLayerElement::EMappingMode mappingMode) noexcept;

	static std::array<float, BONE_WEIGHT_COUNT - 1> getWeight(const std::array<float, BONE_WEIGHT_COUNT>& weightArray) noexcept;

	static DirectX::XMFLOAT4 EulerVectorToQuaternion(float x, float y, float z) noexcept;

	//static BoneAnimation convertToBoneAnimation(const std::map<FbxLongLong, KeyFrame>& keyFrame, float& endTime) noexcept;

	static void getKeyFrameTimes(const FbxAnimCurve* curve, std::set<FbxTime>& keyFrameTimes) noexcept;

	HRESULT getKeyFrames(FbxAnimLayer* animLayer, FbxNode* linkNode, std::set<FbxTime>& keyFramesTimes);
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
};

