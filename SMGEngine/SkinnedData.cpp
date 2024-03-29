#include "stdafx.h"
#include "SkinnedData.h"
#include "FileHelper.h"
#include "MathHelper.h"

using namespace DirectX;

BoneAnimation::BoneAnimation(std::vector<KeyFrame>&& keyFrames) noexcept
	: _keyFrames(keyFrames)
{
}

DirectX::XMMATRIX BoneAnimation::interpolate(const TickCount64& currentTick) const noexcept
{
	check(!_keyFrames.empty(), "keyFrame이 없습니다.");
	const XMVECTOR zero = XMVectorSet(0.f, 0.f, 0.f, 1.f);

	XMVECTOR S, P, Q;
	interpolateXXX(currentTick, S, P, Q);
	
	return XMMatrixAffineTransformation(S, zero, Q, P);
}

DirectX::XMMATRIX BoneAnimation::interpolateWithBlend(const TickCount64& currentTick,
													const TickCount64& blendTick,
													const BoneAnimationBlendInstance& blendInstance) const noexcept
{
	check(!_keyFrames.empty(), "keyFrame이 없습니다.");
	check(currentTick < blendTick);
	float blendLerpPecent = 1 - (currentTick / static_cast<float>(blendTick));

	XMVECTOR blendS = XMLoadFloat3(&blendInstance._scaling);
	XMVECTOR blendP = XMLoadFloat3(&blendInstance._translation);
	XMVECTOR blendQ = XMLoadFloat4(&blendInstance._rotationQuat);

	const XMVECTOR zero = XMVectorSet(0.f, 0.f, 0.f, 1.f);

	XMVECTOR S, P, Q;
	interpolateXXX(currentTick, S, P, Q);

	S = XMVectorLerp(S, blendS, blendLerpPecent);
	P = XMVectorLerp(P, blendP, blendLerpPecent);
	Q = XMQuaternionSlerp(Q, blendQ, blendLerpPecent);

	return XMMatrixAffineTransformation(S, zero, Q, P);
}

void BoneAnimation::interpolateXXX(const TickCount64& currentTick, DirectX::XMVECTOR& S, DirectX::XMVECTOR& P, DirectX::XMVECTOR& Q) const noexcept
{
	check(currentTick < std::numeric_limits<uint32_t>::max());
	if (currentTick <= _keyFrames.front()._tick)
	{
		S = XMLoadFloat3(&_keyFrames.back()._scaling);
		P = XMLoadFloat3(&_keyFrames.back()._translation);
		Q = XMLoadFloat4(&_keyFrames.back()._rotationQuat);
	}
	else if (currentTick >= _keyFrames.back()._tick)
	{
		S = XMLoadFloat3(&_keyFrames.back()._scaling);
		P = XMLoadFloat3(&_keyFrames.back()._translation);
		Q = XMLoadFloat4(&_keyFrames.back()._rotationQuat);
	}
	else
	{
		auto it1 = lower_bound(_keyFrames.begin(), _keyFrames.end(), static_cast<uint32_t>(currentTick), [](const KeyFrame& itor, const uint32_t& t) {
			return itor._tick < t; });
		auto it0 = prev(it1);

		float lerpPercent = (currentTick - it0->_tick) / static_cast<float>(it1->_tick - it0->_tick);

		XMVECTOR s0 = XMLoadFloat3(&it0->_scaling);
		XMVECTOR s1 = XMLoadFloat3(&it1->_scaling);

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
		uint32_t frame = 0;
		childNodes[i].loadAttribute("Frame", frame);
		_keyFrames[i]._tick = (frame - start) * FRAME_TO_TICKCOUNT;
		if (i != 0 && _keyFrames[i - 1]._tick >= _keyFrames[i]._tick)
		{
			ThrowErrCode(ErrCode::InvalidAnimationData, "시간이 역행합니다.");
		}
		if (frame > end || frame < start)
		{
			ThrowErrCode(ErrCode::InvalidAnimationData, "시간이 범위 밖입니다. frame: " + std::to_string(frame));
		}
		childNodes[i].loadAttribute("Translation", _keyFrames[i]._translation);
		childNodes[i].loadAttribute("Scaling", _keyFrames[i]._scaling);
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

BoneInfo::BoneInfo(const XMLReaderNode& rootNode)
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

AnimationClip::AnimationClip(std::vector<BoneAnimation>&& boneAnimations, const uint32_t clipEndFrame) noexcept
{
	check(!boneAnimations.empty(), "비정상입니다.");

	_boneAnimations = std::move(boneAnimations);
	_clipEndFrame = clipEndFrame;
}

void AnimationClip::interpolate(const TickCount64& currentTick,
								std::vector<DirectX::XMMATRIX>& matrixes) const noexcept
{
	check(matrixes.empty(), "outMatrix는 빈 상태로 들어와야 합니다.");

	matrixes.reserve(_boneAnimations.size());
	for (int i = 0; i < _boneAnimations.size(); ++i)
	{
		matrixes.emplace_back(_boneAnimations[i].interpolate(currentTick));
	}
}

void AnimationClip::interpolateWithBlend(const TickCount64& currentTick,
	const TickCount64& blendTick,
	const std::vector<BoneAnimationBlendInstance>& blendInstances,
	std::vector<DirectX::XMMATRIX>& matrixes) const noexcept
{
	check(matrixes.empty(), "outMatrix는 빈 상태로 들어와야 합니다.");
	check(blendInstances.size() == blendInstances.size(),
		"blendInstance 값이 비정상입니다. " + std::to_string(blendInstances.size()) + " " + std::to_string(blendInstances.size()));

	matrixes.reserve(_boneAnimations.size());
	for (int i = 0; i < _boneAnimations.size(); ++i)
	{
		matrixes.emplace_back(_boneAnimations[i].interpolateWithBlend(currentTick, blendTick, blendInstances[i]));
	}
}

AnimationClip::AnimationClip(const XMLReaderNode& node)
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

void AnimationClip::getBlendValue(const TickCount64& currentTick, std::vector<BoneAnimationBlendInstance>& blendInstances) const noexcept
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
	: _tick(0)
	, _translation(0.f, 0.f, 0.f)
	, _scaling(0.f, 0.f, 0.f)
	, _rotationQuat(0.f, 0.f, 0.f, 0.f)
{
}

SkinnedModelInstance::SkinnedModelInstance(uint16_t index, const BoneInfo* boneInfo, const AnimationInfo* animationInfo) noexcept
	: _currentTick(0)
	, _animationClipName("IDLE")
	, _index(index)
	, _boneInfo(boneInfo)
	, _animationInfo(animationInfo)
	, _blendTick(0)
	, _animationSpeed(1.f)
{
	const AnimationClip* animationClip = _animationInfo->getAnimationClip(_animationClipName);
	check(animationClip != nullptr, "애니메이션을 찾을 수 없습니다. " + _animationClipName);
	_currentAnimationClip = animationClip;

	_transformMatrixes.resize(boneInfo->getBoneCount(), MathHelper::Identity4x4);
}

void SkinnedModelInstance::updateSkinnedAnimation(const TickCount64& dt) noexcept
{
	_currentTick += static_cast<TickCount64>(dt * _animationSpeed);
	
	std::vector<XMMATRIX> toParentTransforms;

	if (_currentTick < _blendTick)
	{
		_currentAnimationClip->interpolateWithBlend(_currentTick, _blendTick, _blendInstances, toParentTransforms);
	}
	else
	{
		_currentAnimationClip->interpolate(_currentTick, toParentTransforms);
	}

	_boneInfo->getFinalTransforms(toParentTransforms, _transformMatrixes);
}

void SkinnedModelInstance::setAnimation(const std::string& animationClipName, const TickCount64& blendTick) noexcept
{
	_blendTick = blendTick;
	if (blendTick != 0)
	{
		const AnimationClip* animationClip = _animationInfo->getAnimationClip(_animationClipName);
		check(animationClip != nullptr, "애니메이션을 찾을 수 없습니다. " + _animationClipName);

		animationClip->getBlendValue(_currentTick, _blendInstances);
	}

	const AnimationClip* newAnimationClip = _animationInfo->getAnimationClip(animationClipName);
	check(newAnimationClip != nullptr, "애니메이션을 찾을 수 없습니다. " + animationClipName);

	_currentAnimationClip = newAnimationClip;
	_animationClipName = animationClipName;
	_currentTick = 0;
	//_currentFrame = 0;

}

bool SkinnedModelInstance::isAnimationEnd(void) const noexcept
{
	if (_currentAnimationClip->getClipEndFrame() * FRAME_TO_TICKCOUNT <= _currentTick)
	{
		return true;
	}
	return false;
}

void SkinnedModelInstance::setAnimationSpeed(float speed) noexcept
{
	_animationSpeed = speed;
}

const AnimationClip* AnimationInfo::getAnimationClip(const std::string& clipName) const noexcept
{
	auto it = _animations.find(clipName);
	if (it == _animations.end())
	{
		return nullptr;
	}
	
	return it->second.get();
}

AnimationInfo::AnimationInfo(const XMLReaderNode& rootNode)
{
	const auto& childNodes = rootNode.getChildNodes();
	_animations.reserve(childNodes.size());
	for (int i = 0; i < childNodes.size(); ++i)
	{
		std::string clipName;
		childNodes[i].loadAttribute("Name", clipName);

		auto it = _animations.emplace(clipName, std::make_unique<AnimationClip>(childNodes[i]));
		if (it.second == false)
		{
			ThrowErrCode(ErrCode::KeyDuplicated, "clipName : " + clipName + " 중복");
		}
	}
}

BoneAnimationBlendInstance::BoneAnimationBlendInstance(DirectX::FXMVECTOR scaling, DirectX::FXMVECTOR translation, DirectX::FXMVECTOR rotationQuat) noexcept
{
	XMStoreFloat3(&_scaling, scaling);
	XMStoreFloat3(&_translation, translation);
	XMStoreFloat4(&_rotationQuat, rotationQuat);
}
