#include "stdafx.h"
#include "SkinnedData.h"
#include "FileHelper.h"
#include "MathHelper.h"

using namespace DirectX;

BoneAnimation::BoneAnimation(std::vector<KeyFrame>&& keyFrames) noexcept
	: _keyFrames(keyFrames)
{
}

DirectX::XMMATRIX BoneAnimation::interpolate(const uint32_t currentTime) const noexcept
{
	check(!_keyFrames.empty(), "keyFrame이 없습니다.");
	const XMVECTOR zero = XMVectorSet(0.f, 0.f, 0.f, 1.f);

	XMVECTOR S, P, Q;
	interpolateXXX(currentTime, S, P, Q);
	
	return XMMatrixAffineTransformation(S, zero, Q, P);
}

DirectX::XMMATRIX BoneAnimation::interpolateWithBlend(const uint32_t currentTime, const BoneAnimationBlendInstance& blendInstance, const uint32_t blendTick) const noexcept
{
	check(!_keyFrames.empty(), "keyFrame이 없습니다.");
	float blendLerpPecent = currentTime / static_cast<float>(blendTick);

	XMVECTOR blendS = XMLoadFloat3(&blendInstance._scale);
	XMVECTOR blendP = XMLoadFloat3(&blendInstance._translation);
	XMVECTOR blendQ = XMLoadFloat4(&blendInstance._rotationQuat);

	const XMVECTOR zero = XMVectorSet(0.f, 0.f, 0.f, 1.f);

	XMVECTOR S, P, Q;
	interpolateXXX(currentTime, S, P, Q);

	S = XMVectorLerp(blendS, S, blendLerpPecent);
	P = XMVectorLerp(blendP, P, blendLerpPecent);
	Q = XMQuaternionSlerp(blendQ, Q, blendLerpPecent);

	return XMMatrixAffineTransformation(S, zero, Q, P);
}

void BoneAnimation::interpolateXXX(const uint32_t currentTime, DirectX::XMVECTOR& S, DirectX::XMVECTOR& P, DirectX::XMVECTOR& Q) const noexcept
{
	if (currentTime <= _keyFrames.front()._frame * FRAME_TO_TICKCOUNT)
	{
		S = XMLoadFloat3(&_keyFrames.back()._scale);
		P = XMLoadFloat3(&_keyFrames.back()._translation);
		Q = XMLoadFloat4(&_keyFrames.back()._rotationQuat);
	}
	else if (currentTime >= _keyFrames.back()._frame * FRAME_TO_TICKCOUNT)
	{
		S = XMLoadFloat3(&_keyFrames.back()._scale);
		P = XMLoadFloat3(&_keyFrames.back()._translation);
		Q = XMLoadFloat4(&_keyFrames.back()._rotationQuat);
	}
	else
	{
		auto it1 = lower_bound(_keyFrames.begin(), _keyFrames.end(), currentTime, [](const KeyFrame& itor, const uint32_t t) {
			return itor._frame * FRAME_TO_TICKCOUNT < t; });
		auto it0 = prev(it1);

		float lerpPercent = (currentTime - it0->_frame * FRAME_TO_TICKCOUNT) / static_cast<float>((it1->_frame - it0->_frame) * FRAME_TO_TICKCOUNT);
		XMVECTOR s0 = XMLoadFloat3(&it0->_scale);
		XMVECTOR s1 = XMLoadFloat3(&it1->_scale);

		XMVECTOR p0 = XMLoadFloat3(&it0->_translation);
		XMVECTOR p1 = XMLoadFloat3(&it1->_translation);

		XMVECTOR q0 = XMLoadFloat4(&it0->_rotationQuat);
		XMVECTOR q1 = XMLoadFloat4(&it1->_rotationQuat);

		S = XMVectorLerp(s0, s1, lerpPercent);
		P = XMVectorLerp(p0, p1, lerpPercent);
		Q = XMQuaternionSlerp(q0, q1, lerpPercent);
	}
}

void BoneAnimation::loadXML(const XMLReaderNode& node, const uint32_t start, const uint32_t end)
{
	const auto& childNodes = node.getChildNodes();

	_keyFrames.resize(childNodes.size());
	for (int i = 0; i < childNodes.size(); ++i)
	{
		childNodes[i].loadAttribute("Frame", _keyFrames[i]._frame);
		_keyFrames[i]._frame -= start;
		if (i != 0 && _keyFrames[i - 1]._frame >= _keyFrames[i]._frame)
		{
			ThrowErrCode(ErrCode::InvalidAnimationData, "시간이 역행합니다.");
		}
		if (_keyFrames[i]._frame > end || _keyFrames[i]._frame < start)
		{
			ThrowErrCode(ErrCode::InvalidAnimationData, "시간이 범위 밖입니다. frame: " + std::to_string(_keyFrames[i]._frame));
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

void BoneInfo::getFinalTransforms(const std::vector<DirectX::XMMATRIX>& toParentTransforms,
								std::vector<DirectX::XMFLOAT4X4>& transformMatrixes) const noexcept
{
	check(toParentTransforms.size() == _boneOffsets.size(), "비정상입니다.");
	BoneIndex boneCount = getBoneCount();

	std::vector<XMMATRIX> toRootTransforms(boneCount);
	toRootTransforms[0] = toParentTransforms[0];

	for (BoneIndex i = 1; i < boneCount; ++i)
	{
		BoneIndex parentIndex = _boneHierarchy[i];
		check(parentIndex < i, "parent index는 child보다 작아야 합니다.");

		XMMATRIX toRoot = XMMatrixMultiply(toParentTransforms[i], toRootTransforms[parentIndex]);
		
		toRootTransforms[i] = toRoot;
	}

	for (BoneIndex i = 0; i < boneCount; ++i)
	{
		XMMATRIX offset = XMLoadFloat4x4(&_boneOffsets[i]);
		XMMATRIX finalTransform = XMMatrixMultiply(offset, toRootTransforms[i]);
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

AnimationClip::AnimationClip(std::vector<BoneAnimation>&& boneAnimations, const uint32_t clipEndTime) noexcept
{
	check(!boneAnimations.empty(), "비정상입니다.");

	_boneAnimations = std::move(boneAnimations);
	_clipEndFrame = clipEndTime;
}

void AnimationClip::interpolate(const uint32_t currentTime,
								std::vector<DirectX::XMMATRIX>& matrixes) const noexcept
{
	check(matrixes.empty(), "outMatrix는 빈 상태로 들어와야 합니다.");

	matrixes.reserve(_boneAnimations.size());
	for (int i = 0; i < _boneAnimations.size(); ++i)
	{
		matrixes.emplace_back(_boneAnimations[i].interpolate(currentTime));
	}
}

void AnimationClip::interpolateWithBlend(uint32_t currentTick,
	uint32_t blendTick,
	const std::vector<BoneAnimationBlendInstance>& blendInstances,
	std::vector<DirectX::XMMATRIX>& matrixes) const noexcept
{
	check(matrixes.empty(), "outMatrix는 빈 상태로 들어와야 합니다.");
	check(blendInstances.size() == blendInstances.size(),
		"blendInstance 값이 비정상입니다. " + std::to_string(blendInstances.size()) + " " + std::to_string(blendInstances.size()));

	matrixes.reserve(_boneAnimations.size());
	for (int i = 0; i < _boneAnimations.size(); ++i)
	{
		XMVECTOR scale = XMLoadFloat3(&blendInstances[i]._scale);
		XMVECTOR translate = XMLoadFloat3(&blendInstances[i]._translation);
		XMVECTOR rotationQuat = XMLoadFloat4(&blendInstances[i]._rotationQuat);

		matrixes.emplace_back(_boneAnimations[i].interpolateWithBlend(currentTick, blendInstances[i], blendTick));
	}
}

void AnimationClip::loadXML(const XMLReaderNode& node)
{
	uint32_t start, end;
	node.loadAttribute("Start", start);
	node.loadAttribute("End", end);
	if (start > end)
	{
		ThrowErrCode(ErrCode::InvalidTimeInfo, "StartTime이 EndTime보다 큽니다.");
	}
	_clipEndFrame = end - start;

	const auto& childNodes = node.getChildNodes();
	_boneAnimations.resize(childNodes.size());
	for (int i = 0; i < childNodes.size(); ++i)
	{
		_boneAnimations[i].loadXML(childNodes[i], start, end);
	}
}

const std::vector<BoneAnimation>& AnimationClip::getBoneAnimationXXX(void) const noexcept
{
	return _boneAnimations;
}

void AnimationClip::getBlendValue(uint32_t currentTick, std::vector<BoneAnimationBlendInstance>& blendInstances) const
{
	blendInstances.clear();
	blendInstances.reserve(_boneAnimations.size());
	for (int i = 0; i < _boneAnimations.size(); ++i)
	{
		XMVECTOR S, P, Q;
		_boneAnimations[i].interpolateXXX(currentTick, S, P, Q);
		blendInstances.emplace_back(S, P, Q);
	}
}

KeyFrame::KeyFrame() noexcept
	: _frame(0)
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

SkinnedModelInstance::SkinnedModelInstance(uint16_t index, BoneInfo* boneInfo, AnimationInfo* animationInfo) noexcept
	: _currentTick(0)
	, _animationClipName("IDLE")
	, _index(index)
	, _boneInfo(boneInfo)
	, _animationInfo(animationInfo)
	, _blendTick(0)
	, _animationSpeed(1.f)
{
	_transformMatrixes.resize(boneInfo->getBoneCount(), MathHelper::Identity4x4);
}

void SkinnedModelInstance::updateSkinnedAnimation(const uint32_t dt) noexcept
{
	const AnimationClip* animationClip = _animationInfo->getAnimationClip(_animationClipName);
	if (animationClip == nullptr)
	{
		// 허용해도 되는 오류일까? [2/21/2021 qwerwy]
		check(animationClip != nullptr, "애니메이션을 찾을 수 없습니다. " + _animationClipName);
		return;
	}
	// 임시처리. actionState코드가 완성되면 삭제되어야함. [2/21/2021 qwerwy]
	if (_currentTick * _animationSpeed > ((animationClip->getClipEndFrame() + 1) * FRAME_TO_TICKCOUNT))
	{
		setAnimation(_animationClipName, 100);
	}
	_currentTick = _currentTick + dt;

	std::vector<XMMATRIX> toParentTransforms;

	if (_currentTick < _blendTick)
	{
		animationClip->interpolateWithBlend(_currentTick * _animationSpeed, _blendTick, _blendInstances, toParentTransforms);
	}
	else
	{
		animationClip->interpolate(_currentTick * _animationSpeed, toParentTransforms);
	}

	_boneInfo->getFinalTransforms(toParentTransforms, _transformMatrixes);
}

void SkinnedModelInstance::setAnimation(const std::string& animationClipName, const uint32_t blendTick) noexcept
{
	if (blendTick != 0)
	{
		const AnimationClip* animationClip = _animationInfo->getAnimationClip(_animationClipName);
		if (animationClip == nullptr)
		{
			// 허용해도 되는 오류일까? [2/21/2021 qwerwy]
			check(animationClip != nullptr, "애니메이션을 찾을 수 없습니다. " + _animationClipName);
			return;
		}

		animationClip->getBlendValue(_currentTick, _blendInstances);

		_blendTick = blendTick;
	}
	_animationClipName = animationClipName;
	_currentTick = 0;
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

BoneAnimationBlendInstance::BoneAnimationBlendInstance(DirectX::FXMVECTOR scale, DirectX::FXMVECTOR translate, DirectX::FXMVECTOR rotationQuat) noexcept
{
	XMStoreFloat3(&_scale, scale);
	XMStoreFloat3(&_translation, translate);
	XMStoreFloat4(&_rotationQuat, rotationQuat);
}
