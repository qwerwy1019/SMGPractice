#pragma once
#include "TypeGeometry.h"

//class XMLReaderNode;

class Material
{
public:
	//Material() noexcept;
	Material(const int materialCBIndex,
		const int diffuseSRVHeapIndex,
		const int normalSRVHeapIndex,
		const DirectX::XMFLOAT4& diffuseAlbedo,
		const DirectX::XMFLOAT3& fresnelR0,
		const float roughness) noexcept;

	inline int getMaterialCBIndex(void) const noexcept { return _materialCBIndex; }
	inline int getDiffuseSRVHeapIndex(void) const noexcept { return _diffuseSRVHeapIndex; }

	inline const DirectX::XMFLOAT4& getDiffuseAlbedo(void) const noexcept { return _diffuseAlbedo; }
	inline const DirectX::XMFLOAT3& getFresnelR0(void) const noexcept { return _fresnelR0; }
	inline float getRoughness(void) const noexcept { return _roughness; }
	inline const DirectX::XMFLOAT4X4& getMaterialTransform(void) const noexcept { return _materialTransform; }

	int _dirtyFrames;
private:
	int _materialCBIndex;
	CommonIndex _diffuseSRVHeapIndex;
	CommonIndex _normalSRVHeapIndex;

	DirectX::XMFLOAT4 _diffuseAlbedo;
	DirectX::XMFLOAT3 _fresnelR0;
	float _roughness;

	// material을 animating할때만 사용할듯 [1/14/2021 qwerw]
	DirectX::XMFLOAT4X4 _materialTransform;
};
