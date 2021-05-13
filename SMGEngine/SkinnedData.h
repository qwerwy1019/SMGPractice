#pragma once
#include "TypeGeometry.h"
#include "TypeCommon.h"

class XMLReaderNode;
struct KeyFrame
{
	KeyFrame() noexcept;

	uint32_t _frame;
	DirectX::XMFLOAT3 _translation;
	DirectX::XMFLOAT3 _scaling;
	DirectX::XMFLOAT4 _rotationQuat;
};

struct BoneAnimationBlendInstance
{
	BoneAnimationBlendInstance(DirectX::FXMVECTOR scaling, DirectX::FXMVECTOR translation, DirectX::FXMVECTOR rotationQuat) noexcept;
	DirectX::XMFLOAT3 _translation;
	DirectX::XMFLOAT3 _scaling;
	DirectX::XMFLOAT4 _rotationQuat;
};

class BoneAnimation
{
public:
	BoneAnimation() = default;
	BoneAnimation(BoneAnimation&&) = default;
	BoneAnimation& operator=(BoneAnimation&&) = default;

	BoneAnimation(const BoneAnimation&) = delete;
	BoneAnimation& operator=(const BoneAnimation&) = delete;

	BoneAnimation(std::vector<KeyFrame>&& keyFrames) noexcept;

	DirectX::XMMATRIX interpolate(const TickCount64& currentTick) const noexcept;
	DirectX::XMMATRIX interpolateWithBlend(const TickCount64& currentTick,
		const TickCount64& blendTick,
		const BoneAnimationBlendInstance& blendInstance) const noexcept;

	void interpolateXXX(const TickCount64& currentTick, DirectX::XMVECTOR& S, DirectX::XMVECTOR& P, DirectX::XMVECTOR& Q) const noexcept;

	void loadXML(const XMLReaderNode& rootNode, const uint32_t start, const uint32_t end);

	// fbxLoader때문에 만들긴 했는데 다른 방법을 찾으면 좋겠다. [1/26/2021 qwerw]
	const std::vector<KeyFrame>& getKeyFrameReferenceXXX(void) const noexcept;
	
private:
	std::vector<KeyFrame> _keyFrames;
};

class AnimationClip
{
public:
	AnimationClip() = default;
	AnimationClip(AnimationClip&&) = default;
	AnimationClip& operator=(AnimationClip&&) = default;

	AnimationClip(const AnimationClip&) = delete;
	AnimationClip& operator=(const AnimationClip&) = delete;

	AnimationClip(std::vector<BoneAnimation>&& boneAnimations, const uint32_t clipEndFrame) noexcept;

	void interpolate(const TickCount64& currentTick, std::vector<DirectX::XMMATRIX>& matrixes) const noexcept;
	void interpolateWithBlend(const TickCount64& currentTick,
							const TickCount64& blendTick,
							const std::vector<BoneAnimationBlendInstance>& blendInstances,
							std::vector<DirectX::XMMATRIX>& matrixes) const noexcept;

	uint32_t getClipEndFrame(void) const noexcept { return _clipEndFrame; }

	void loadXML(const XMLReaderNode& rootNode);

	// fbxLoader때문에 만들긴 했는데 다른 방법을 찾으면 좋겠다. [1/26/2021 qwerw]
	const std::vector<BoneAnimation>& getBoneAnimationXXX(void) const noexcept;
	void getBlendValue(const TickCount64& currentTick, std::vector<BoneAnimationBlendInstance>& blendInstances) const;
private:
	std::vector<BoneAnimation> _boneAnimations;
	uint32_t _clipEndFrame;
	
};

class AnimationInfo
{
public:
	AnimationClip* getAnimationClip(const std::string& clipName) noexcept;

	void loadXML(const XMLReaderNode& rootNode);
	std::vector<std::string> getAnimationNameListDev(void) const noexcept
	{
		std::vector<std::string> nameVector;
		nameVector.reserve(_animations.size());
		for (const auto& e : _animations)
		{
			nameVector.emplace_back(e.first);
		}
		return nameVector;
	}
private:
	std::unordered_map<std::string, AnimationClip> _animations;
};

class BoneInfo
{
public:
	void getFinalTransforms(const std::vector<DirectX::XMMATRIX>& toParentTransforms,
		std::vector<DirectX::XMFLOAT4X4>& finalTransforms) const noexcept;
	void loadXML(const XMLReaderNode& rootNode);
	BoneIndex getBoneCount(void) const noexcept;
private:
	std::vector<BoneIndex> _boneHierarchy;
	std::vector<DirectX::XMFLOAT4X4> _boneOffsets;
};

class SkinnedModelInstance
{
public:
	SkinnedModelInstance(uint16_t index, BoneInfo* boneInfo, AnimationInfo* animationInfo) noexcept;
	void updateSkinnedAnimation(const TickCount64& dt) noexcept;
	const std::vector<DirectX::XMFLOAT4X4>& getTransformMatrixes(void) const noexcept { return _transformMatrixes; }
	uint16_t getIndex(void) const noexcept { return _index; }
	uint32_t getLocalTickCount(void) const noexcept { return _currentTick; }
	//uint32_t getCurrentFrame(void) const noexcept { return _currentFrame; }
	void setAnimation(const std::string& animationClipName, const TickCount64& blendTick) noexcept;

private:
	TickCount64 _currentTick;
	//uint32_t _currentFrame;
	std::string _animationClipName;
	uint16_t _index;
	
	BoneInfo* _boneInfo;
	AnimationInfo* _animationInfo;

	AnimationClip* _currentAnimationClip;

	std::vector<DirectX::XMFLOAT4X4> _transformMatrixes;

	std::vector<BoneAnimationBlendInstance> _blendInstances;
	TickCount64 _blendTick;
	float _animationSpeed;
};