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
	if (_keyFrames.empty())
	{
		matrix = MathHelper::Identity4x4;
		return;
	}
	const XMVECTOR zero = XMVectorSet(0.f, 0.f, 0.f, 1.f);
	if (t <= _keyFrames.front()._timePos)
	{
		XMVECTOR S = XMLoadFloat3(&_keyFrames.front()._scale);
		XMVECTOR P = XMLoadFloat3(&_keyFrames.front()._translation);
		XMVECTOR Q = XMLoadFloat4(&_keyFrames.front()._rotationQuat);

		XMStoreFloat4x4(&matrix, XMMatrixAffineTransformation(S, zero, Q, P));
		//matrix = _keyFrames.front()._transform;
	}
	else if (t >= _keyFrames.back()._timePos)
	{
		XMVECTOR S = XMLoadFloat3(&_keyFrames.back()._scale);
		XMVECTOR P = XMLoadFloat3(&_keyFrames.back()._translation);
		XMVECTOR Q = XMLoadFloat4(&_keyFrames.back()._rotationQuat);

		XMStoreFloat4x4(&matrix, XMMatrixAffineTransformation(S, zero, Q, P));
		//matrix = _keyFrames.back()._transform;
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

		//MathHelper::interpolateMatrix(it0->_transform, it1->_transform, lerpPercent, matrix);
	}

	//matrix = MathHelper::Identity4x4;
}

HRESULT BoneAnimation::loadXML(const XMLReaderNode& node, float timeOffset) noexcept
{
	HRESULT rv;
	const auto& childNodes = node.getChildNodes();
	_keyFrames.resize(childNodes.size());
	for (int i = 0; i < childNodes.size(); ++i)
	{
		childNodes[i].loadAttribute("Time", _keyFrames[i]._timePos);
		_keyFrames[i]._timePos -= timeOffset;
		childNodes[i].loadAttribute("Translation", _keyFrames[i]._translation);
		childNodes[i].loadAttribute("Scale", _keyFrames[i]._scale);
		childNodes[i].loadAttribute("RotationQuat", _keyFrames[i]._rotationQuat);

		// directX랑 fbx랑 quaternion순서가 다른것같음 [2/3/2021 qwerw]
// 		{
// 			float tempX = _keyFrames[i]._rotationQuat.x;
// 			_keyFrames[i]._rotationQuat.x = _keyFrames[i]._rotationQuat.y;
// 			_keyFrames[i]._rotationQuat.y = _keyFrames[i]._rotationQuat.z;
// 			_keyFrames[i]._rotationQuat.z = _keyFrames[i]._rotationQuat.w;
// 			_keyFrames[i]._rotationQuat.w = tempX;
// 		}

		//childNodes[i].loadAttribute("Transform", _keyFrames[i]._transform);
	}
	return S_OK;
}

const std::vector<KeyFrame>& BoneAnimation::getKeyFrameReferenceXXX(void) const noexcept
{
	return _keyFrames;
}

void BoneInfo::getFinalTransforms(const std::vector<DirectX::XMFLOAT4X4>& toParentTransforms,
								std::vector<DirectX::XMFLOAT4X4>& transformMatrixes) const noexcept
{
	assert(toParentTransforms.size() == _boneOffsets.size());
	CommonIndex boneCount = _boneOffsets.size();

	std::vector<XMFLOAT4X4> toRootTransforms(boneCount);
	toRootTransforms[0] = toParentTransforms[0];

	for (CommonIndex i = 1; i < boneCount; ++i)
	{
		XMMATRIX toParent = XMLoadFloat4x4(&toParentTransforms[i]);
		int parentIndex = _boneHierarchy[i];
		assert(!MathHelper::equal(toRootTransforms[parentIndex], MathHelper::Zero4x4));
		XMMATRIX parentToRoot = XMLoadFloat4x4(&toRootTransforms[parentIndex]);

		XMMATRIX toRoot = XMMatrixMultiply(toParent, parentToRoot);
		
		XMStoreFloat4x4(&toRootTransforms[i], toRoot);
	}

	for (CommonIndex i = 0; i < boneCount; ++i)
	{
		XMMATRIX offset = XMLoadFloat4x4(&_boneOffsets[i]);
		XMMATRIX toRoot = XMLoadFloat4x4(&toRootTransforms[i]);
		XMMATRIX finalTransform = XMMatrixMultiply(offset, toRoot);
		XMStoreFloat4x4(&transformMatrixes[i], XMMatrixTranspose(finalTransform));
	}
}

HRESULT BoneInfo::loadXML(const XMLReaderNode& rootNode) noexcept
{
	HRESULT rv;
	rootNode.loadAttribute("Hierarchy", _boneHierarchy);
	_boneOffsets.resize(_boneHierarchy.size());
	const auto& childNodes = rootNode.getChildNodes();
	assert(_boneHierarchy.size() == childNodes.size());
	for (int i = 0; i < childNodes.size(); ++i)
	{
		childNodes[i].loadAttribute("Offset", _boneOffsets[i]);
	}

	return S_OK;
}

// 쓰나?? [1/29/2021 qwerw]
AnimationClip::AnimationClip(const CommonIndex boneCount) noexcept
{
	_boneAnimations.resize(boneCount);
}

void AnimationClip::interpolate(const float t, std::vector<DirectX::XMFLOAT4X4>& matrixes) const noexcept
{
	assert(matrixes.empty());

	matrixes.resize(_boneAnimations.size());
	for (int i = 0; i < _boneAnimations.size(); ++i)
	{
		_boneAnimations[i].interpolate(t, matrixes[i]);
	}
}

void AnimationClip::setBoneAnimationXXX(const CommonIndex boneIndex, BoneAnimation&& boneAnimation) noexcept
{
	assert(boneIndex < _boneAnimations.size());
	_boneAnimations[boneIndex] = std::move(boneAnimation);
}

HRESULT AnimationClip::loadXML(const XMLReaderNode& node) noexcept
{
	float startTime, endTime;
	node.loadAttribute("StartTime", startTime);
	node.loadAttribute("EndTime", endTime);
	if (startTime >= endTime)
	{
		assert(false);
		return E_FAIL;
	}
	_clipEndTime = endTime - startTime;

	const auto& childNodes = node.getChildNodes();
	_boneAnimations.resize(childNodes.size());
	for (int i = 0; i < childNodes.size(); ++i)
	{
		
		HRESULT rv = _boneAnimations[i].loadXML(childNodes[i], startTime);
		if (FAILED(rv))
		{
			assert(false);
			return rv;
		}
	}
	return S_OK;
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

KeyFrame::KeyFrame(float timePos,
	const DirectX::XMFLOAT3& translation,
	const DirectX::XMFLOAT3& scale,
	const DirectX::XMFLOAT4& rotationQuat) noexcept
	: _timePos(timePos)
	, _translation(translation)
	, _scale(scale)
	, _rotationQuat(rotationQuat)
{

}

SkinnedModelInstance::SkinnedModelInstance(CommonIndex index, BoneInfo* boneInfo, AnimationInfo* animationInfo) noexcept
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
	assert(animationClip != nullptr);
	if (_timePos > animationClip->getClipEndTime())
	{
		_timePos = 0.f;
	}

	std::vector<XMFLOAT4X4> toParentTransforms;
	animationClip->interpolate(_timePos, toParentTransforms);

	_boneInfo->getFinalTransforms(toParentTransforms, _transformMatrixes);

// 	for (int i = 0; i < _transformMatrixes.size(); ++i)
// 	{
// 		XMMATRIX identityMatrix = XMLoadFloat4x4(&MathHelper::Identity4x4);
// 		XMStoreFloat4x4(&_transformMatrixes[i], XMMatrixTranspose(identityMatrix));
// 	}
}

const AnimationClip* AnimationInfo::getAnimationClip(const std::string& clipName) const noexcept
{
	auto it = _animations.find(clipName);
	if (it == _animations.end())
	{
		assert(false);
		return nullptr;
	}
	
	return &(it->second);
}

HRESULT AnimationInfo::loadXML(const XMLReaderNode& rootNode) noexcept
{
	const auto& childNodes = rootNode.getChildNodes();
	_animations.reserve(childNodes.size());
	for (int i = 0; i < childNodes.size(); ++i)
	{
		std::string clipName;
		childNodes[i].loadAttribute("Name", clipName);
		AnimationClip clip;
		HRESULT rv = clip.loadXML(childNodes[i]);
		if (FAILED(rv))
		{
			assert(false);
			return rv;
		}
		auto it = _animations.emplace(clipName, std::move(clip));
		if (it.second == false)
		{
			assert(false && L"이름 중복");
			return E_FAIL;
		}
	}
	return S_OK;
}
