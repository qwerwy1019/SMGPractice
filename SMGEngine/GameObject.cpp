#include "stdafx.h"
#include "GameObject.h"
#include "MathHelper.h"
#include "SkinnedData.h"
#include "D3DApp.h"
#include "SMGFramework.h"

GameObject::GameObject(uint16_t objConstantBufferIndex,
						uint16_t skinnedConstantBufferIndex, 
						SkinnedModelInstance* skinnedModelInstance) noexcept
	: _worldMatrix(MathHelper::Identity4x4)
	, _textureTransform(MathHelper::Identity4x4)
	, _objConstantBufferIndex(objConstantBufferIndex)
	, _dirtyFrames(FRAME_RESOURCE_COUNT)
	, _skinnedConstantBufferIndex(skinnedConstantBufferIndex)
	, _skinnedModelInstance(skinnedModelInstance)
{
	check(_skinnedModelInstance != nullptr || _skinnedConstantBufferIndex == std::numeric_limits<uint16_t>::max());
}

GameObject::~GameObject()
{
	if (_skinnedModelInstance != nullptr)
	{
		delete _skinnedModelInstance;
	}
	for (auto renderItem : _renderItems)
	{
		SMGFramework::getD3DApp()->removeRenderItem(renderItem->_renderLayer, renderItem);
	}
	SMGFramework::getD3DApp()->removeSkinnedInstance(_skinnedModelInstance);
}

void GameObject::setWorldMatrix(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& direction, const DirectX::XMFLOAT3& upVector, float size) noexcept
{
	MathHelper::getWorldMatrix(position, direction, upVector, size, _worldMatrix);
	_dirtyFrames = FRAME_RESOURCE_COUNT;
}

const DirectX::XMFLOAT4X4& GameObject::getWorldMatrix(void) const noexcept
{
	return _worldMatrix;
}

const DirectX::XMFLOAT4X4& GameObject::getTextrueTransformMatrix(void) const noexcept
{
	return _textureTransform;
}

bool GameObject::isAnimationEnd() const noexcept
{
	check(_skinnedModelInstance != nullptr);

	return _skinnedModelInstance->isAnimationEnd();
}

void GameObject::setAnimation(const std::string& animationName, const TickCount64& blendTick) noexcept
{
	check(_skinnedModelInstance != nullptr);
	check(animationName.empty() == false);
	check(blendTick > 0);

	_skinnedModelInstance->setAnimation(animationName, blendTick);
}

void GameObject::setRenderItemsXXX(std::vector<RenderItem*>&& renderItems) noexcept
{
	check(_renderItems.empty());
	_renderItems = renderItems;
}

bool GameObject::popDirtyFrame(void) noexcept
{
	if (_dirtyFrames > 0)
	{
		--_dirtyFrames;
		return true;
	}
	return false;
}

const std::vector<RenderItem*>& GameObject::getRenderItems(void) const noexcept
{
	return _renderItems;
}
