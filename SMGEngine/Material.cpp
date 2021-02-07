#include "stdafx.h"
#include "Material.h"
#include "MathHelper.h"
#include "FileHelper.h"

Material::Material(const int materialCBIndex,
	const int diffuseSRVHeapIndex,
	const int normalSRVHeapIndex,
	const DirectX::XMFLOAT4& diffuseAlbedo,
	const DirectX::XMFLOAT3& fresnelR0,
	const float roughness) noexcept
	: _materialCBIndex(materialCBIndex)
	, _diffuseSRVHeapIndex(diffuseSRVHeapIndex)
	, _normalSRVHeapIndex(normalSRVHeapIndex)
	, _dirtyFrames(FRAME_RESOURCE_COUNT)
	, _materialTransform(MathHelper::Identity4x4)
	, _diffuseAlbedo(diffuseAlbedo)
	, _fresnelR0(fresnelR0)
	, _roughness(roughness)
{}

// Material::Material(const int materialCBIndex,
// 	const int diffuseSRVHeapIndex,
// 	const int normalSRVHeapIndex,
// 	const MaterialStaticInfo& info) noexcept
// 	: _materialCBIndex(materialCBIndex)
// 	, _diffuseSRVHeapIndex(diffuseSRVHeapIndex)
// 	, _normalSRVHeapIndex(normalSRVHeapIndex)
// 	, _dirtyFrames(FRAME_RESOURCE_COUNT)
// 	, _materialTransform(MathHelper::Identity4x4)
// 	, _info(info)
// {}

//Material::Material() noexcept
//	: _materialCBIndex(-1)
//	, _diffuseSRVHeapIndex(UNDEFINED_COMMON_INDEX)
//	, _normalSRVHeapIndex(UNDEFINED_COMMON_INDEX)
//	, _dirtyFrames(FRAME_RESOURCE_COUNT)
//	, _materialTransform(MathHelper::Identity4x4)
//	, _diffuseAlbedo(0.f, 0.f, 0.f, 0.f)
//	, _fresnelR0(0.f, 0.f, 0.f)
//	, _roughness(0.f)
//{
//
//}