#pragma once
#include "TypeGeometry.h"

class XMLReaderNode;

struct KeyFrame
{
	KeyFrame() noexcept;
// 	KeyFrame(float timePos,
// 		const DirectX::XMFLOAT3& translation,
// 		const DirectX::XMFLOAT3& scale,
// 		const DirectX::XMFLOAT4& rotationQuat) noexcept;
	float _timePos;
	DirectX::XMFLOAT3 _translation;
	DirectX::XMFLOAT3 _scale;
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

	void interpolate(const float t, DirectX::XMFLOAT4X4& matrix) const noexcept;

	void loadXML(const XMLReaderNode& rootNode, float timeOffset);

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

	AnimationClip(std::vector<BoneAnimation>&& boneAnimations, float clipEndTime) noexcept;

	void interpolate(const float t, std::vector<DirectX::XMFLOAT4X4>& matrixes) const noexcept;

	float getClipEndTime(void) const noexcept { return _clipEndTime; }

	void loadXML(const XMLReaderNode& rootNode);

	// fbxLoader때문에 만들긴 했는데 다른 방법을 찾으면 좋겠다. [1/26/2021 qwerw]
	const std::vector<BoneAnimation>& getBoneAnimationXXX(void) const noexcept;
private:
	std::vector<BoneAnimation> _boneAnimations;
	float _clipEndTime;
};

class AnimationInfo
{
public:
	const AnimationClip* getAnimationClip(const std::string& clipName) const;

	void loadXML(const XMLReaderNode& rootNode);
private:
	std::unordered_map<std::string, AnimationClip> _animations;
};

class BoneInfo
{
public:
	void getFinalTransforms(const std::vector<DirectX::XMFLOAT4X4>& toParentTransforms,
		std::vector<DirectX::XMFLOAT4X4>& finalTransforms) const noexcept;
	void loadXML(const XMLReaderNode& rootNode);
	BoneIndex getBoneCount(void) const noexcept;
private:
	std::vector<Index16> _boneHierarchy;
	std::vector<DirectX::XMFLOAT4X4> _boneOffsets;
};

class SkinnedModelInstance
{
public:
	SkinnedModelInstance(Index16 index, BoneInfo* boneInfo, AnimationInfo* animationInfo) noexcept;
	void updateSkinnedAnimation(float dt) noexcept;
	const std::vector<DirectX::XMFLOAT4X4>& getTransformMatrixes(void) const noexcept { return _transformMatrixes; }
	Index16 getIndex(void) const noexcept { return _index; }

private:
	float _timePos;
	std::string _animationClipName;
	Index16 _index;
	
	BoneInfo* _boneInfo;
	AnimationInfo* _animationInfo;

	std::vector<DirectX::XMFLOAT4X4> _transformMatrixes;
};