#pragma once
#include "TypeGeometry.h"

class XMLReaderNode;

struct KeyFrame
{
	KeyFrame() noexcept;
	KeyFrame(float timePos,
		const DirectX::XMFLOAT3& translation,
		const DirectX::XMFLOAT3& scale,
		const DirectX::XMFLOAT4& rotationQuat) noexcept;
	float _timePos;
	DirectX::XMFLOAT3 _translation;
	DirectX::XMFLOAT3 _scale;
	DirectX::XMFLOAT4 _rotationQuat;

	//DirectX::XMFLOAT4X4 _transform;
};

class BoneAnimation
{
public:
	// reserve나 resize후 사용할수있게 해야할듯 [1/22/2021 qwerw]
	//BoneAnimation(const CommonIndex keyFrameCount) noexcept;
	BoneAnimation() = default;
	BoneAnimation(BoneAnimation&&) = default;
	BoneAnimation& operator=(BoneAnimation&&) = default;

	BoneAnimation(const BoneAnimation&) = delete;
	BoneAnimation& operator=(const BoneAnimation&) = delete;

	BoneAnimation(std::vector<KeyFrame>&& keyFrames) noexcept;

	void interpolate(const float t, DirectX::XMFLOAT4X4& matrix) const noexcept;

	HRESULT loadXML(const XMLReaderNode& rootNode, float timeOffset) noexcept;
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

	AnimationClip(const CommonIndex boneCount) noexcept;
	void interpolate(const float t, std::vector<DirectX::XMFLOAT4X4>& matrixes) const noexcept;

	float getClipEndTime(void) const noexcept { return _clipEndTime; }

	HRESULT loadXML(const XMLReaderNode& rootNode) noexcept;
	// fbxLoader때문에 만들긴 했는데 다른 방법을 찾으면 좋겠다. [1/26/2021 qwerw]
	const std::vector<BoneAnimation>& getBoneAnimationXXX(void) const noexcept;
	void setBoneAnimationXXX(const CommonIndex boneIndex, BoneAnimation&& boneAnimation) noexcept;
	void setClipEndTimeXXX(float clipEndTime) { _clipEndTime = clipEndTime; }
private:
	std::vector<BoneAnimation> _boneAnimations;
	float _clipEndTime;
};

// class SkinnedData
// {
// public:
// 	SkinnedData(const CommonIndex boneCount) noexcept;
// 	// 같은 데이터를 자주 만들게 될 상황이면 캐싱할 방법을 찾는게 좋다고함 [1/19/2021 qwerw]
// 	void getFinalTransforms(const std::string& clipName,
// 							const float t,
// 							std::vector<DirectX::XMFLOAT4X4>& transformMatrixes) const noexcept;
// 	void addBoneHierarchy(const CommonIndex parentIndex) noexcept;
// 
// private:
// 	std::vector<CommonIndex> _boneHierarchy;
// 	std::vector<DirectX::XMFLOAT4X4> _boneOffsets;
// 	std::unordered_map<std::string, AnimationClip> _animations;
// 
// };

class AnimationInfo
{
public:
	const AnimationClip* getAnimationClip(const std::string& clipName) const noexcept;

	HRESULT loadXML(const XMLReaderNode& rootNode) noexcept;
private:
	std::unordered_map<std::string, AnimationClip> _animations;
};

class BoneInfo
{
public:
	void getFinalTransforms(const std::vector<DirectX::XMFLOAT4X4>& toParentTransforms,
		std::vector<DirectX::XMFLOAT4X4>& finalTransforms) const noexcept;
	HRESULT loadXML(const XMLReaderNode& rootNode) noexcept;
	int getBoneCount(void) const noexcept { return _boneOffsets.size(); }
private:
	std::vector<CommonIndex> _boneHierarchy;
	std::vector<DirectX::XMFLOAT4X4> _boneOffsets;
};

class SkinnedModelInstance
{
public:
	SkinnedModelInstance(CommonIndex index, BoneInfo* boneInfo, AnimationInfo* animationInfo) noexcept;
	void updateSkinnedAnimation(float dt) noexcept;
	const std::vector<DirectX::XMFLOAT4X4>& getTransformMatrixes(void) const noexcept { return _transformMatrixes; }
	CommonIndex getIndex(void) const noexcept { return _index; }

private:
	float _timePos;
	std::string _animationClipName;
	CommonIndex _index;
	
	BoneInfo* _boneInfo;
	AnimationInfo* _animationInfo;

	std::vector<DirectX::XMFLOAT4X4> _transformMatrixes;
};