#pragma once
#include "TypeGeometry.h"
#include "TypeD3d.h"

class XMLReaderNode;

class Material
{
public:
	Material(const int materialCBIndex, const XMLReaderNode& node);

	inline int getMaterialCBIndex(void) const noexcept { return _materialCBIndex; }
	inline int getDiffuseSRVHeapIndex(void) const noexcept { return _diffuseSRVHeapIndex; }

	inline const DirectX::XMFLOAT4& getDiffuseAlbedo(void) const noexcept { return _diffuseAlbedo; }
	inline const DirectX::XMFLOAT3& getFresnelR0(void) const noexcept { return _fresnelR0; }
	inline float getRoughness(void) const noexcept { return _roughness; }
	inline const DirectX::XMFLOAT4X4& getMaterialTransform(void) const noexcept { return _materialTransform; }
	inline RenderLayer getRenderLayer(void) const noexcept { return _renderLayer; }
	int _dirtyFrames;
private:
	int _materialCBIndex;
	uint16_t _diffuseSRVHeapIndex;
	uint16_t _normalSRVHeapIndex;

	DirectX::XMFLOAT4 _diffuseAlbedo;
	DirectX::XMFLOAT3 _fresnelR0;
	float _roughness;

	RenderLayer _renderLayer;
	// material을 animating할때만 사용할듯 [1/14/2021 qwerw]
	DirectX::XMFLOAT4X4 _materialTransform;
};
