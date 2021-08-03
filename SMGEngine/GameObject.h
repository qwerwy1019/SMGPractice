#pragma once
#include "TypeD3d.h"
#include "TypeCommon.h"

class SkinnedModelInstance;
struct RenderItem;

class GameObject
{
public:
	GameObject(uint16_t objConstantBufferIndex,
				uint16_t skinnedConstantBufferIndex, 
				SkinnedModelInstance* skinnedModelInstance) noexcept;
	~GameObject();
	void setRenderItemsXXX(std::vector<RenderItem*>&& renderItems) noexcept;

	void setWorldMatrix(const DirectX::XMFLOAT3& position,
						const DirectX::XMFLOAT3& direction, 
						const DirectX::XMFLOAT3& upVector,
						float size) noexcept;
	bool popDirtyFrame(void) noexcept;

	inline uint16_t getObjectConstantBufferIndex(void) const noexcept { return _objConstantBufferIndex; }
	inline uint16_t getSkinnedConstantBufferIndex(void) const noexcept { return _skinnedConstantBufferIndex; }
	 
	const std::vector<RenderItem*>& getRenderItems(void) const noexcept;
	const DirectX::XMFLOAT4X4& getWorldMatrix(void) const noexcept;
	const DirectX::XMFLOAT4X4& getTextrueTransformMatrix(void) const noexcept;

	inline bool isSkinnedAnimationObject(void) const noexcept { return _skinnedModelInstance != nullptr; }
	bool isAnimationEnd() const noexcept;
	void setAnimation(const std::string& animationName, const TickCount64& blendTick) noexcept;
private:
	DirectX::XMFLOAT4X4 _worldMatrix;
	DirectX::XMFLOAT4X4 _textureTransform;
	uint16_t _objConstantBufferIndex;
	int _dirtyFrames;

	uint16_t _skinnedConstantBufferIndex;
	SkinnedModelInstance* _skinnedModelInstance;

	std::vector<RenderItem*> _renderItems;
};