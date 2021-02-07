#pragma once
#include "stdafx.h"
#include "TypeData.h"

// MAX_LIGHT_COUNT 값 수정 시 LightingUtil.hlsl 의 값 수정해야함.
constexpr int MAX_LIGHT_COUNT = 16;

struct Light
{
	Light() noexcept;
	DirectX::XMFLOAT3 _strength;
	float _falloffStart;
	DirectX::XMFLOAT3 _direction;
	float _falloffEnd;
	DirectX::XMFLOAT3 _position;
	float _spotPower;
};

struct ObjectConstants
{
	ObjectConstants() noexcept;
	DirectX::XMFLOAT4X4 _world;
	DirectX::XMFLOAT4X4 _textureTransform;
};

struct PassConstants
{
	PassConstants() noexcept;
	DirectX::XMFLOAT4X4 _view;
	DirectX::XMFLOAT4X4 _invView;
	DirectX::XMFLOAT4X4 _proj;
	DirectX::XMFLOAT4X4 _invProj;
	DirectX::XMFLOAT4X4 _viewProj;
	DirectX::XMFLOAT4X4 _invViewProj;
	DirectX::XMFLOAT3 _cameraPos;
	float pad1;
	DirectX::XMFLOAT2 _renderTargetSize;
	DirectX::XMFLOAT2 _invRenderTargetSize;
	float _nearZ;
	float _farZ;
	float _totalTime;
	float _deltaTime;
	DirectX::XMFLOAT4 _fogColor;
	float _fogStart;
	float _fogEnd;
	DirectX::XMFLOAT2 pad2;
	// 조명관련
	DirectX::XMFLOAT4 _ambientLight;
	std::array<Light, MAX_LIGHT_COUNT> _lights;
};
struct MaterialConstants
{
	MaterialConstants() noexcept;
	DirectX::XMFLOAT4 _diffuseAlbedo;
	DirectX::XMFLOAT3 _fresnelR0;
	float _roughness;
	DirectX::XMFLOAT4X4 _materialTransform;
};
struct SkinnedConstants
{
	SkinnedConstants() noexcept;
	DirectX::XMFLOAT4X4 _boneTransforms[96];
};
struct Vertex
{
	Vertex() noexcept;
	Vertex(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& normal, const DirectX::XMFLOAT2& textureCoord) noexcept;
	DirectX::XMFLOAT3 _position;
	DirectX::XMFLOAT3 _normal;
	DirectX::XMFLOAT2 _textureCoord;
};
using BoneIndex = BYTE;
constexpr BoneIndex UNDEFINED_BONE_INDEX = static_cast<BoneIndex>(-1);
constexpr int BONE_WEIGHT_COUNT = 4;
struct SkinnedVertex
{
	SkinnedVertex() noexcept;
// 	SkinnedVertex(const DirectX::XMFLOAT3& position,
// 				const DirectX::XMFLOAT3& normal,
// 				const DirectX::XMFLOAT2& textureCoord,
// 				const std::array<float, BONE_WEIGHT_COUNT - 1>& boneWeights,
// 				const std::array<BoneIndex, BONE_WEIGHT_COUNT>& boneIndices) noexcept;

	DirectX::XMFLOAT3 _position;
	DirectX::XMFLOAT3 _normal;
	DirectX::XMFLOAT2 _textureCoord;
	std::array<float, BONE_WEIGHT_COUNT - 1> _boneWeights;
	std::array<BoneIndex, BONE_WEIGHT_COUNT> _boneIndices;
};

using Index = uint16_t;
constexpr Index UNDEFINED_INDEX = static_cast<Index>(-1);

struct Texture
{
	std::string _name;
	std::wstring _fileName;

	CommonIndex _index;
	WComPtr<ID3D12Resource> _resource;
	WComPtr<ID3D12Resource> _uploader;
};