#include "stdafx.h"
#include "TypeGeometry.h"
#include "MathHelper.h"
#include <DirectXColors.h>

Light::Light() noexcept
	: _strength( 0.5f, 0.5f, 0.5f )
	, _falloffStart( 1.f )
	, _direction( 0.f, -1.f, 0.f )
	, _falloffEnd(10.f)
	, _position( 0.f, 0.f, 0.f )
	, _spotPower(64.f)
{

}

ObjectConstants::ObjectConstants() noexcept
	: _world(MathHelper::Identity4x4)
	, _textureTransform(MathHelper::Identity4x4)
{

}

PassConstants::PassConstants() noexcept
	: _view(MathHelper::Identity4x4)
	, _invView(MathHelper::Identity4x4)
	, _proj(MathHelper::Identity4x4)
	, _invProj(MathHelper::Identity4x4)
	, _viewProj(MathHelper::Identity4x4)
	, _invViewProj(MathHelper::Identity4x4)
	, _cameraPos(0.f, 0.f, 0.f)
	, pad1(0.0f)
	, _renderTargetSize(0.f, 0.f)
	, _invRenderTargetSize(0.f, 0.f)
	, _nearZ(0.f)
	, _farZ(0.f)
	, _totalTime(0.f)
	, _deltaTime(0.f)
	, _fogColor(DirectX::XMFLOAT4(DirectX::Colors::LightSteelBlue))
	, _fogStart(20.f)
	, _fogEnd(100.f)
	, pad2(0.f, 0.f)
	, _ambientLight(0.25, 0.25, 0.35f, 1.f)
	, _lights()
	, _orthogonalView(MathHelper::Identity4x4)
{

}

MaterialConstants::MaterialConstants() noexcept
	: _diffuseAlbedo(1.f, 1.f, 1.f, 1.f)
	, _fresnelR0(0.01f, 0.01f, 0.01f)
	, _roughness(0.25f)
	, _materialTransform(MathHelper::Identity4x4)
{

}

Vertex::Vertex(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& normal, const DirectX::XMFLOAT2& textureCoord) noexcept
	: _position(position)
	, _normal(normal)
	, _textureCoord(textureCoord)
{

}

Vertex::Vertex() noexcept
	: _position(0.f, 0.f, 0.f)
	, _normal(0.f, 0.f, 0.f)
	, _textureCoord(0.f, 0.f)
{

}

// MaterialStaticInfo::MaterialStaticInfo(const std::string name,
// 	const DirectX::XMFLOAT4& diffuseAlbedo,
// 	const DirectX::XMFLOAT3& fresnelR0,
// 	const float roughness)
// 	: _name(name)
// 	, _diffuseAlbedo(diffuseAlbedo)
// 	, _fresnelR0(fresnelR0)
// 	, _roughness(roughness) 
// {}

// SkinnedVertex::SkinnedVertex(const DirectX::XMFLOAT3& position,
// 							 const DirectX::XMFLOAT3& normal,
// 							 const DirectX::XMFLOAT2& textureCoord,
// 							 const std::array<float, BONE_WEIGHT_COUNT - 1>& boneWeights,
// 							 const std::array<BoneIndex, BONE_WEIGHT_COUNT>& boneIndices) noexcept
// 	: _position(position)
// 	, _normal(normal)
// 	, _textureCoord(textureCoord)
// 	, _boneWeights(boneWeights)
// 	, _boneIndices(boneIndices)
// {}

SkinnedVertex::SkinnedVertex() noexcept
	: _position(0.f, 0.f, 0.f)
	, _normal(0.f, 0.f, 0.f)
	, _textureCoord(0.f, 0.f)
	, _boneWeights{ 0.f }
	, _boneIndices{ UNDEFINED_BONE_INDEX, UNDEFINED_BONE_INDEX, UNDEFINED_BONE_INDEX, UNDEFINED_BONE_INDEX }
{

}

SkinnedConstants::SkinnedConstants() noexcept
{
	for (int i = 0; i < 96; ++i)
	{
		_boneTransforms[i] = MathHelper::Identity4x4;
	}
}
