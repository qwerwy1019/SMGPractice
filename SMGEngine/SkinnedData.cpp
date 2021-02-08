#include "stdafx.h"
#include "SkinnedData.h"
#include "FileHelper.h"
#include "MathHelper.h"

using namespace DirectX;

BoneAnimation::BoneAnimation(std::vector<KeyFrame>&& keyFrames) noexcept
	: _keyFrames(keyFrames)
{
}

void BoneAnimation::interpolate(const float t, DirectX::XMFLOAT4X4& matrix) const noexcept
{
	check(!_keyFrames.empty(), "keyFrame이 없습니다.");

	const XMVECTOR zero = XMVectorSet(0.f, 0.f, 0.f, 1.f);
	if (t <= _keyFrames.front()._timePos)
	{
		XMVECTOR S = XMLoadFloat3(&_keyFrames.front()._scale);
		XMVECTOR P = XMLoadFloat3(&_keyFrames.front()._translation);
		XMVECTOR Q = XMLoadFloat4(&_keyFrames.front()._rotationQuat);

		XMStoreFloat4x4(&matrix, XMMatrixAffineTransformation(S, zero, Q, P));
	}
	else if (t >= _keyFrames.back()._timePos)
	{
		XMVECTOR S = XMLoadFloat3(&_keyFrames.back()._scale);
		XMVECTOR P = XMLoadFloat3(&_keyFrames.back()._translation);
		XMVECTOR Q = XMLoadFloat4(&_keyFrames.back()._rotationQuat);

		XMStoreFloat4x4(&matrix, XMMatrixAffineTransformation(S, zero, Q, P));
	}
	else
	{
		auto it1 = lower_bound(_keyFrames.begin(), _keyFrames.end(), t, [](const KeyFrame& itor, const double t) {
			return itor._timePos < t; });
		auto it0 = prev(it1);

		float lerpPercent = (t - it0->_timePos) / (it1->_timePos - it0->_timePos);
		XMVECTOR s0 = XMLoadFloat3(&it0->_scale);
		XMVECTOR s1 = XMLoadFloat3(&it1->_scale);
		
		XMVECTOR p0 = XMLoadFloat3(&it0->_translation);
		XMVECTOR p1 = XMLoadFloat3(&it1->_translation);

		XMVECTOR q0 = XMLoadFloat4(&it0->_rotationQuat);
		XMVECTOR q1 = XMLoadFloat4(&it1->_rotationQuat);

		XMVECTOR S = XMVectorLerp(s0, s1, lerpPercent);
		XMVECTOR P = XMVectorLerp(p0, p1, lerpPercent);
		XMVECTOR Q = XMQuaternionSlerp(q0, q1, lerpPercent);

		XMStoreFloat4x4(&matrix, XMMatrixAffineTransformation(S, zero, Q, P));
	}
}

void BoneAnimation::loadXML(const XMLReaderNode& node, float timeOffset)
{
	const auto& childNodes = node.getChildNodes();

	_keyFrames.resize(childNodes.size());
	for (int i = 0; i < childNodes.size(); ++i)
	{
		childNodes[i].loadAttribute("Time", _keyFrames[i]._timePos);
		_keyFrames[i]._timePos -= timeOffset;
		if (i != 0 && _keyFrames[i - 1]._timePos > _keyFrames[i]._timePos)
		{
			ThrowErrCode(ErrCode::InvalidAnimationData, "시간이 역행합니다.");
		}
		if (_keyFrames[i]._timePos < 0.f)
		{
			ThrowErrCode(ErrCode::InvalidAnimationData, "시간 값이 음수입니다.");
		}
		childNodes[i].loadAttribute("Translation", _keyFrames[i]._translation);
		childNodes[i].loadAttribute("Scale", _keyFrames[i]._scale);
		childNodes[i].loadAttribute("RotationQuat", _keyFrames[i]._rotationQuat);
	}
}

const std::vector<KeyFrame>& BoneAnimation::getKeyFrameReferenceXXX(void) const noexcept
{
	return _keyFrames;
}

void BoneInfo::getFinalTransforms(const std::vector<DirectX::XMFLOAT4X4>& toParentTransforms,
								std::vector<DirectX::XMFLOAT4X4>& transformMatrixes) const noexcept
{
	check(toParentTransforms.size() == _boneOffsets.size(), "비정상입니다.");
	Index16 boneCount = getBoneCount();

	std::vector<XMFLOAT4X4> toRootTransforms(boneCount);
	toRootTransforms[0] = toParentTransforms[0];

	for (Index16 i = 1; i < boneCount; ++i)
	{
		XMMATRIX toParent = XMLoadFloat4x4(&toParentTransforms[i]);
		int parentIndex = _boneHierarchy[i];
		check(parentIndex < i, "parent index는 child보다 작아야 합니다.");
		XMMATRIX parentToRoot = XMLoadFloat4x4(&toRootTransforms[parentIndex]);

		XMMATRIX toRoot = XMMatrixMultiply(toParent, parentToRoot);
		
		XMStoreFloat4x4(&toRootTransforms[i], toRoot);
	}

	for (Index16 i = 0; i < boneCount; ++i)
	{
		XMMATRIX offset = XMLoadFloat4x4(&_boneOffsets[i]);
		XMMATRIX toRoot = XMLoadFloat4x4(&toRootTransforms[i]);
		XMMATRIX finalTransform = XMMatrixMultiply(offset, toRoot);
		XMStoreFloat4x4(&transformMatrixes[i], XMMatrixTranspose(finalTransform));
	}
}

void BoneInfo::loadXML(const XMLReaderNode& rootNode)
{
	rootNode.loadAttribute("Hierarchy", _boneHierarchy);
	if (BONE_INDEX_MAX < _boneHierarchy.size())
	{
		ThrowErrCode(ErrCode::Overflow, "boneHierarchy는 " + std::to_string(BONE_INDEX_MAX) + "이하여야 합니다.");
	}
	_boneOffsets.resize(_boneHierarchy.size());
	const auto& childNodes = rootNode.getChildNodes();
	if (_boneHierarchy.size() != childNodes.size())
	{
		ThrowErrCode(ErrCode::InvalidXmlData,
			"hierarchy:" + std::to_string(_boneHierarchy.size()) +
			"child count:" + std::to_string(childNodes.size()));
	}
	for (int i = 0; i < childNodes.size(); ++i)
	{
		childNodes[i].loadAttribute("Offset", _boneOffsets[i]);
	}
}

BoneIndex BoneInfo::getBoneCount(void) const noexcept
{
	check(_boneOffsets.size() < BONE_INDEX_MAX, "비정상입니다.");

	return _boneOffsets.size();
}

AnimationClip::AnimationClip(std::vector<BoneAnimation>&& boneAnimations, float clipEndTime) noexcept
{
	check(!boneAnimations.empty(), "비정상입니다.");
	check(clipEndTime > 0.f, "애니메이션 길이가 0이하입니다.");

	_boneAnimations = std::move(boneAnimations);
	_clipEndTime = clipEndTime;
}

void AnimationClip::interpolate(const float t, std::vector<DirectX::XMFLOAT4X4>& matrixes) const noexcept
{
	check(matrixes.empty(), "outMatrix는 빈 상태로 들어와야 합니다.");

	matrixes.resize(_boneAnimations.size());
	for (int i = 0; i < _boneAnimations.size(); ++i)
	{
		_boneAnimations[i].interpolate(t, matrixes[i]);
	}
}

// void AnimationClip::setBoneAnimationXXX(std::vector<BoneAnimation>&& boneAnimation) noexcept
// {
// 	check(!boneAnimation.empty(), "빈 데이터가 생성됩니다.");
// 
// 	_boneAnimations = boneAnimation;
// }

void AnimationClip::loadXML(const XMLReaderNode& node)
{
	float startTime, endTime;
	node.loadAttribute("StartTime", startTime);
	node.loadAttribute("EndTime", endTime);
	if (startTime >= endTime)
	{
		ThrowErrCode(ErrCode::InvalidTimeInfo, "StartTime이 EndTime보다 큽니다.");
	}
	_clipEndTime = endTime - startTime;

	const auto& childNodes = node.getChildNodes();
	_boneAnimations.resize(childNodes.size());
	for (int i = 0; i < childNodes.size(); ++i)
	{
		
		_boneAnimations[i].loadXML(childNodes[i], startTime);
	}
}

const std::vector<BoneAnimation>& AnimationClip::getBoneAnimationXXX(void) const noexcept
{
	return _boneAnimations;
}

KeyFrame::KeyFrame() noexcept
	: _timePos(0.f)
	, _translation(0.f, 0.f, 0.f)
	, _scale(0.f, 0.f, 0.f)
	, _rotationQuat(0.f, 0.f, 0.f, 0.f)
{
}

// KeyFrame::KeyFrame(float timePos,
// 	const DirectX::XMFLOAT3& translation,
// 	const DirectX::XMFLOAT3& scale,
// 	const DirectX::XMFLOAT4& rotationQuat) noexcept
// 	: _timePos(timePos)
// 	, _translation(translation)
// 	, _scale(scale)
// 	, _rotationQuat(rotationQuat)
// {
// 
// }

SkinnedModelInstance::SkinnedModelInstance(Index16 index, BoneInfo* boneInfo, AnimationInfo* animationInfo) noexcept
	: _timePos(0.f)
	, _animationClipName("BaseLayer")
	, _index(index)
	, _boneInfo(boneInfo)
	, _animationInfo(animationInfo)
{
	_transformMatrixes.resize(boneInfo->getBoneCount(), MathHelper::Identity4x4);
}

void SkinnedModelInstance::updateSkinnedAnimation(float dt) noexcept
{
	_timePos += dt;
	const AnimationClip* animationClip = _animationInfo->getAnimationClip(_animationClipName);
	check(animationClip != nullptr, "애니메이션을 찾을 수 없습니다. " + _animationClipName);
	if (_timePos > animationClip->getClipEndTime())
	{
		_timePos = 0.f;
	}

	std::vector<XMFLOAT4X4> toParentTransforms;
	animationClip->interpolate(_timePos, toParentTransforms);

	_boneInfo->getFinalTransforms(toParentTransforms, _transformMatrixes);
}

const AnimationClip* AnimationInfo::getAnimationClip(const std::string& clipName) const
{
	auto it = _animations.find(clipName);
	if (it == _animations.end())
	{
		ThrowErrCode(ErrCode::AnimationNotFound, clipName);
	}
	
	return &(it->second);
}

void AnimationInfo::loadXML(const XMLReaderNode& rootNode)
{
	const auto& childNodes = rootNode.getChildNodes();
	_animations.reserve(childNodes.size());
	for (int i = 0; i < childNodes.size(); ++i)
	{
		std::string clipName;
		childNodes[i].loadAttribute("Name", clipName);
		AnimationClip clip;
		clip.loadXML(childNodes[i]);

		auto it = _animations.emplace(clipName, std::move(clip));
		if (it.second == false)
		{
			ThrowErrCode(ErrCode::KeyDuplicated, "clipName : " + clipName + " 중복");
		}
	}
}
