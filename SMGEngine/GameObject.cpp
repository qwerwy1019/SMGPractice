#include "stdafx.h"
#include "GameObject.h"
#include "MathHelper.h"
#include "SkinnedData.h"
#include "D3DApp.h"
#include "SMGFramework.h"
#include "MeshGeometry.h"
#include "Camera.h"
#include "StageManager.h"

GameObject::GameObject(const MeshGeometry* meshGeometry,
						uint16_t objConstantBufferIndex,
						uint16_t skinnedConstantBufferIndex, 
						SkinnedModelInstance* skinnedModelInstance) noexcept
	: _meshGeometry(meshGeometry)
	, _worldMatrix(MathHelper::Identity4x4)
	, _textureTransform(MathHelper::Identity4x4)
	, _objConstantBufferIndex(objConstantBufferIndex)
	, _dirtyFrames(FRAME_RESOURCE_COUNT)
	, _skinnedConstantBufferIndex(skinnedConstantBufferIndex)
	, _skinnedModelInstance(skinnedModelInstance)
	, _isCulled(false)
{
	check(_skinnedModelInstance != nullptr || _skinnedConstantBufferIndex == std::numeric_limits<uint16_t>::max());
}

GameObject::~GameObject()
{
	if (SMGFramework::getStageManager()->isLoading())
	{
		return;
	}
	SMGFramework::getD3DApp()->pushObjectContantBufferIndex(_objConstantBufferIndex);
	if (_skinnedConstantBufferIndex != std::numeric_limits<uint16_t>::max())
	{
		SMGFramework::getD3DApp()->pushSkinnedContantBufferIndex(_skinnedConstantBufferIndex);
	}
	for (auto renderItem : _renderItems)
	{
		SMGFramework::getD3DApp()->removeRenderItem(renderItem->_renderLayer, renderItem);
	}
	if (_skinnedModelInstance != nullptr)
	{
		SMGFramework::getD3DApp()->removeSkinnedInstance(_skinnedModelInstance);
	}
}

void GameObject::setWorldMatrix(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& direction, const DirectX::XMFLOAT3& upVector, float size) noexcept
{
	MathHelper::getWorldMatrix(position, direction, upVector, size, _worldMatrix);
	_dirtyFrames = FRAME_RESOURCE_COUNT;
#if defined DEBUG | defined _DEBUG
	for (const auto& devObject : _devObjects)
	{
		devObject->setWorldMatrix(position, direction, upVector, size);
	}
#endif
}

XMVECTOR GameObject::transformLocalToWorld(DirectX::FXMVECTOR vector) const noexcept
{
	XMMATRIX world = XMLoadFloat4x4(&_worldMatrix);
	return XMVector3Transform(vector, world);
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
	check(blendTick >= 0);

	_skinnedModelInstance->setAnimation(animationName, blendTick);
}

void GameObject::setCulled(void) noexcept
{
	if (_meshGeometry == nullptr)
	{
		return;
	}
	XMMATRIX worldMat = XMLoadFloat4x4(&_worldMatrix);
	_isCulled = SMGFramework::getD3DApp()->checkCulled(_meshGeometry->getBoundingBox(), worldMat);
}

void GameObject::setCulledBackground() noexcept
{
	if (_meshGeometry == nullptr)
	{
		return;
	}
	XMMATRIX worldMat = XMLoadFloat4x4(&_worldMatrix);
	XMVECTOR camPosition = XMLoadFloat3(&SMGFramework::getCamera()->getPosition());
	XMMATRIX camPositionMatrix = XMMatrixTranslationFromVector(camPosition);
	_isCulled = SMGFramework::getD3DApp()->checkCulled(_meshGeometry->getBoundingBox(), worldMat * camPositionMatrix);
}

void GameObject::setAnimationSpeed(float speed) noexcept
{
	if (_skinnedModelInstance == nullptr)
	{
		check(false);
		return;
	}
	_skinnedModelInstance->setAnimationSpeed(speed);
}

void GameObject::changeMaterial(uint8_t renderItemIndex, const std::string& materialFileName, const std::string& materialName) noexcept
{
	if (_renderItems.size() <= renderItemIndex)
	{
		check(false);
		return;
	}
	const Material* material = SMGFramework::getD3DApp()->getMaterial(materialFileName, materialName);
	if (material == nullptr)
	{
		check(false, materialFileName + "/" + materialName);
		return;
	}
	_renderItems[renderItemIndex]->changeMaterial(material);
}

void GameObject::setRenderItemsXXX(std::vector<RenderItem*>&& renderItems) noexcept
{
	check(_meshGeometry != nullptr || renderItems.empty(), "Mesh가 없는 gameobject는 rendterItem을 생성할수 없습니다.");
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

const MeshGeometry* GameObject::getMeshGeometry(void) const noexcept
{
	return _meshGeometry;
}

const std::vector<RenderItem*>& GameObject::getRenderItems(void) const noexcept
{
	return _renderItems;
}