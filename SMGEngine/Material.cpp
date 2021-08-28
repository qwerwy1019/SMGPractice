#include "stdafx.h"
#include "Material.h"
#include "MathHelper.h"
#include "FileHelper.h"
#include "SMGFramework.h"
#include "D3DApp.h"

Material::Material(int materialCBIndex, const XMLReaderNode& node)
	: _materialCBIndex(materialCBIndex)
	, _normalSRVHeapIndex(0)
	, _dirtyFrames(FRAME_RESOURCE_COUNT)
	, _materialTransform(MathHelper::Identity4x4)
{
	node.loadAttribute("DiffuseAlbedo", _diffuseAlbedo);
	node.loadAttribute("FresnelR0", _fresnelR0);
	node.loadAttribute("Roughness", _roughness);

	std::string textureName;
	node.loadAttribute("DiffuseTexture", textureName);
	
	_textureIndex = SMGFramework::getD3DApp()->loadTexture(textureName);

	std::string renderLayerString;
	node.loadAttribute("RenderLayer", renderLayerString);

	if (renderLayerString == "Opaque")
	{
		_renderLayer = RenderLayer::Opaque;
	}
	else if (renderLayerString == "OpaqueSkinned")
	{
		_renderLayer = RenderLayer::OpaqueSkinned;
	}
	else if (renderLayerString == "AlphaTested")
	{
		_renderLayer = RenderLayer::AlphaTested;
	}
	else if (renderLayerString == "Shadow")
	{
		_renderLayer = RenderLayer::Shadow;
	}
	else if (renderLayerString == "Transparent")
	{
		_renderLayer = RenderLayer::Transparent;
	}
	else if (renderLayerString == "Background")
	{
		_renderLayer = RenderLayer::Background;
	}
	else if (renderLayerString == "GameObjectDev")
	{
		_renderLayer = RenderLayer::GameObjectDev;
	}
	else
	{
		ThrowErrCode(ErrCode::UndefinedType, renderLayerString);
		static_assert(static_cast<int>(RenderLayer::Count) == 7, "타입 추가시 확인");
	}
}
